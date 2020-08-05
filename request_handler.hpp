#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>

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
			if (num_sent <= 0) return -1;
			n -= num_sent;
			data += num_sent;
		}
		return 0;
	}

	int do_send_file(int file_fd, size_t file_size) {
		// fprintf(stderr, "do_send_file file_fd=%d size=%d\n", file_fd, file_size);
		size_t n = file_size;
		while (n > 0) {
			int num_sent = sendfile(fd, file_fd, NULL, n); // done in kernel
			if (num_sent < 0) return -1;
			n -= num_sent;
		}
		//fprintf(stderr, "done send_file n=%d\n", n);
		return close(file_fd);
	}

	int do_send(const HttpResponse &response, int file_fd, size_t file_size) {
		int status = 0;
		std::string response_header = response.to_string();
		// fprintf(stderr, "response_header=\n%s", response_header.c_str());
		status = do_send_header(response_header);
		if (status == 0 && file_fd != -1) {
			return do_send_file(file_fd, file_size);
		}
		return response.ok() ? 0 : -1;
	}

public:
    int fd;

    int handle_request() {
		if (do_recv() < 0) return -1;

		size_t end_of_header = message.find("\r\n\r\n");
		if (end_of_header == std::string::npos) return 1; // retry later

		std::string request_header = message.substr(0, end_of_header);
		message.erase(0, end_of_header + 4);
		request = HttpRequest();
		request.parse(request_header);

        fprintf(stderr, "handle_request done\n");

		return 0;
	}

	int handle_response() {
		response = HttpResponse();
		response.append_header("Server", "BasicServer");
		response.set_code(200);

		std::string file_path = root + request.uri;
		if (file_path.back() == '/') file_path += "index.html";
        fprintf(stderr, "file_path=%s\n", file_path.c_str());
		int file_fd = open(file_path.c_str(), O_RDONLY);

        fprintf(stderr, "file_fd=%d\n", file_fd);

		if (file_fd < 0) {
			response.set_code(404);
		} else {
			struct stat file_stat;
			if (fstat(file_fd, &file_stat) < 0) {
				perror("fstat");
				response.set_code(500);
			} else {
				// success
				// response.append_header("Last-Modified", to_date(file_stat.st_mtime));
                if (request.keep_alive) {
                    response.append_header("Connection", "Keep-Alive");
                }
				response.append_header("Content-Length", std::to_string(file_stat.st_size));
				response.set_content_type(file_path);
				return do_send(response, file_fd, file_stat.st_size);
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
