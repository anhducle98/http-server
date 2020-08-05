#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>
#include "tokenizer.hpp"

class HttpRequest {
	int parse_start_line(const std::string &line) {
		std::vector<std::string> tokens = Tokenizer::split(line, " ");
		if (tokens.size() != 3) return -1;

		method = tokens[0];
		uri = tokens[1];
		version = tokens[2];

		if (version == "HTTP/1.1") {
			keep_alive = true;
		}
		return 0;
	}

	int parse_header(const std::string &line) {
		std::vector<std::string> tokens = Tokenizer::split(line, ":");
		if (tokens.size() != 2) return -1;
		if (tokens[0] == "Connection" && tokens[1] == "Keep-Alive") {
			keep_alive = true;
		}
		return 0;
	}

public:
    std::string method;
	std::string uri;
	std::string version;
	bool keep_alive;

	HttpRequest(): keep_alive(false) {}

	int parse(const std::string &message) {
		//fprintf(stderr, "parse message\n%s", message.c_str());
		std::vector<std::string> lines = Tokenizer::split(message, "\r\n");
		if (lines.empty()) return -1;

		if (parse_start_line(lines[0]) < 0) return -1;
		for (size_t i = 1; i < lines.size(); ++i) {
			if (parse_header(lines[i]) < 0) return -1;
		}

		return 0;
	}
};

#endif // HTTP_REQUEST_HPP
