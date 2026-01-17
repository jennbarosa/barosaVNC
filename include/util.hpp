#ifndef barosavnc_UTIL_H
#define barosavnc_UTIL_H

#include <limits>
#include <string_view>
#include <regex>
#include <cerrno>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <cstdlib>
#include <optional>
#include <print>
#include <charconv>
#include <format>

namespace barosavnc {
	namespace term {
		constexpr auto RED   = "\x1B[31m";
		constexpr auto GREEN = "\x1B[32m";
		constexpr auto YELLOW= "\x1B[33m";
		constexpr auto BLUE  = "\x1B[34m";
		constexpr auto RESET = "\x1B[0m";
		constexpr auto CLEAR_LINE = "\x1B[2K";
		constexpr auto CLEAR_SCREEN = "\x1B[2J";
		constexpr auto HOME =  "\x1B[H";
	}
	
	template<std::integral Intlike>
		requires(!std::same_as<Intlike, bool>)
	inline std::optional<Intlike> safe_stoi(const std::string_view s) {
		if (s.empty()) return std::nullopt;

		Intlike out{};
		auto res = std::from_chars(s.data(), s.data() + s.size(), out, 10);
		if (res.ec == std::errc() && res.ptr == s.data() + s.size())
			return out;
		return std::nullopt;
	}

	namespace file {
#ifndef _WIN32
		// quick mmap file read
		struct mapped_readonly {
			void*  data;
			size_t size;
			int    fd;
			bool is_open;
			bool has_sentinel;

			mapped_readonly(const std::string_view path);
			~mapped_readonly();
			const std::string_view string_view() const;
		};
#endif
	}

	namespace str {
		inline void trim(std::string& str) {
			str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1); // right trim
			str.erase(0, str.find_first_not_of("\t\n\v\f\r ")); // left trim
		}

		inline std::vector<std::string> regex_split(const std::string& str, const std::string& regex) {
			std::regex regexz{regex};
			return {std::sregex_token_iterator(str.begin(), str.end(), regexz, -1),
					std::sregex_token_iterator()};
		}

		inline bool is_ipv4_host_regex(const std::string& str) {
			static const std::regex regexz{R"((^((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\.){3}(25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)$))"};
			return std::regex_match(str, regexz);
		}
	}

	std::string env_quick(const std::string_view key);
}

#endif
