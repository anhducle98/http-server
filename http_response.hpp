#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <map>
#include <string>

class HttpResponse {
	static std::map<int, std::string> PHRASE;
	static std::map<std::string, std::string> EXT_TO_MIME;
	
	std::string version;
	int code;
	std::string headers;

public:
	HttpResponse() {
		code = 200;
		headers = "";
		version = "HTTP/1.1";
	}

	int set_code(int code) {
		if (!PHRASE.count(code)) return -1;
		this->code = code;
		return 0;
	}

	void append_header(const std::string &field, const std::string &value) {
		headers += field + ": " + value + "\r\n";
	}

	void set_content_type(const std::string &file_path) {
		size_t pos = file_path.find_last_of(".");
		std::string mime_type = "text/plain";
		if (pos != std::string::npos) {
			std::string ext = file_path.substr(pos + 1);
			auto it = EXT_TO_MIME.find(ext);
			if (it != EXT_TO_MIME.end()) {
				mime_type = it->second;
			}
		}
		append_header("Content-Type", mime_type);
	}

    bool ok() const {
        return code == 200;
    }

	std::string to_string() const {
		return version + " " + std::to_string(code) + " " + PHRASE[code] + "\r\n" + headers + "\r\n";
	}
};

std::map<int, std::string> HttpResponse::PHRASE = {
	{200, "OK"},
	{400, "Bad Request"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{500, "Internal Server Error"},
	{501, "Not Implemented"}
};

std::map<std::string, std::string> HttpResponse::EXT_TO_MIME = {
	{"html", "text/html"},
	{"js", "text/javascript"},
	{"jpeg", "image/jpeg"},
	{"jpg", "image/jpeg"},
	{"gif", "image/gif"},
	{"json", "application/json"},
	{"css", "text/css"},
	{"pdf", "application/pdf"},
	{"ico", "image/vnd.microsoft.icon"},
	{"mp3", "audio/mpeg"},
	{"mp4", "video/mp4"}
};

#endif // HTTP_RESPONSE_HPP
