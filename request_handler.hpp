#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "http_request.hpp"
#include "http_response.hpp"

class RequestHandler {
	static const int BUFFER_SIZE = 4096;

    const std::string &root;

	char buffer[BUFFER_SIZE];
	std::string message;

	HttpRequest request;
	HttpResponse response;

	int do_recv() {
		int num_read = recv(fd, buffer, BUFFER_SIZE, 0);
		if (num_read == 0 || (num_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
			return -1;
		}
		if (num_read > 0) message.append(buffer, num_read);
		return 0;
	}

	int do_send_header(const std::string &response_header) {
		size_t n = response_header.size();
		const char* data = response_header.data();
		while (n > 0) {
			int num_sent = send(fd, data, n, 0);
			if (num_sent == 0 || (num_sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
				return -1;
			}
			n -= num_sent;
			data += num_sent;
		}
		return 0;
	}

	int do_send_file(int file_fd, size_t file_size) {
		size_t n = file_size;
		while (n > 0) {
			int num_sent = sendfile(fd, file_fd, NULL, n); // done in kernel
			if (num_sent == 0 || (num_sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
				return -1;
			}
			if (num_sent < 0) return -1;
			n -= num_sent;
		}
		return 0;
	}

	int do_send(const HttpResponse &response, int file_fd, size_t file_size) {
		int status = 0;
		std::string response_header = response.to_string();
		status = do_send_header(response_header);
		if (status == 0 && file_fd != -1) {
			status = do_send_file(file_fd, file_size);
			status |= close(file_fd);
			return status;
		}
		return response.ok() ? 0 : -1;
	}

	static std::string get_current_date() {
		time_t now = time(NULL);
		struct tm *gmt;
		static char buf[100];
		if (!(gmt = gmtime(&now))) return "";
		strftime(buf, sizeof buf, " %a, %d %b %Y %H:%M:%S GMT", gmt);
		return std::string(buf);
	}

public:
    int fd;

    int handle_request() {
		if (do_recv() < 0) return -1;

		size_t end_of_header = message.find("\r\n\r\n");
		if (end_of_header == std::string::npos) {
			fprintf(stderr, "cannot find end_of_header, fd=%d\n", fd);
			return 1; // retry later
		}

		std::string request_header = message.substr(0, end_of_header);
		message.erase(0, end_of_header + 4);
		request = HttpRequest();
		request.parse(request_header);

		return 0;
	}

	int handle_response() {
		response = HttpResponse();
		response.append_header("Server", "BasicServer");
		response.set_code(200);
		response.append_header("Date", get_current_date());

		if (request.method.empty() || request.uri.empty()) {
			response.set_code(400);
		} else if (request.method != "GET") {
			response.set_code(501);
		} else {
			// now handle GET
			std::string file_path = root + request.uri;
			if (file_path.back() == '/') file_path += "index.html";
			int file_fd = open(file_path.c_str(), O_RDONLY);

			if (file_fd < 0) {
				if (errno == EACCES) {
					response.set_code(403);
				} else if (errno == ENOENT) {
					response.set_code(404);
				} else {
					response.set_code(500);
				}
			} else {
				struct stat file_stat;
				if (fstat(file_fd, &file_stat) < 0) {
					perror("fstat");
					response.set_code(500);
				} else {
					// success
					if (request.keep_alive) {
						response.append_header("Connection", "Keep-Alive");
					}
					response.append_header("Content-Length", std::to_string(file_stat.st_size));
					response.set_content_type(file_path);
					return do_send(response, file_fd, file_stat.st_size);
				}
			}
		}
		return do_send(response, -1, 0);
	}

    bool keep_alive() {
        return request.keep_alive;
    }

    RequestHandler(const std::string &root, int fd): root(root), fd(fd) {
	}
};

#endif // REQUEST_HANDLER_HPP
