#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

namespace Tokenizer {
	std::string get_trimmed(const std::string &str, size_t from, size_t to) {
		while (from < str.size() && std::isspace(str[from])) ++from;
		while (to > 0 && std::isspace(str[to - 1])) --to;
		if (from >= to) return "";
		return str.substr(from, to - from);
	}

	std::vector<std::string> split(const std::string &str, const std::string &delim) {
		std::vector<std::string> tokens;
		for (size_t pos = 0; pos < str.size(); ) {
			size_t next_pos = str.find(delim, pos);
			if (next_pos == std::string::npos) {
				next_pos = str.size();
			}
			tokens.push_back(get_trimmed(str, pos, next_pos));
			pos = next_pos + delim.size();
		}
		return tokens;
	}
}

#endif // TOKENIZER_HPP
