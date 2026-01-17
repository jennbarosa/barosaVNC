#ifndef barosavnc_CLI_H
#define barosavnc_CLI_H
#include <initializer_list>
#include <print>
#include <filesystem>
#include <unordered_map>
#include <functional>
#include <unordered_set>
#include <fmt/core.h>
#include <fmt/format.h>
#include "../include/util.hpp"

namespace fs = std::filesystem;

namespace barosavnc {
	struct cli_command_usage {
		const std::string_view flag;
		const std::string_view options_str;
		const std::string_view description;
	};

	struct cli {
		int* argc;
		char** argv;
		std::string_view executable;
		std::unordered_map<std::string, std::string> options;
		std::unordered_set<std::string> flags;
		std::vector<cli_command_usage> usages;

		cli(int* argc, char** argv);
		bool parse();

		inline void display_usages() {
			using namespace barosavnc::term;
			for (const auto& usage : this->usages) {
				fmt::print("  {:<10}{:<10}{:<10} {:<55}{}{}{}\n",
					YELLOW, usage.flag, 
					GREEN, usage.options_str,
					BLUE, usage.description,
					RESET);
			}
		}

		inline auto option(const std::string& key) {
			return this->options.at(key);
		}

		inline const std::string& option_or(const std::string& key, const std::string& otherwise) {
			if (this->options.contains(key))
				return this->options[key];
			return otherwise;
		}
		
		template<std::integral Intlike>
			requires (!std::same_as<Intlike, bool>)
		inline Intlike option_or(const std::string& key, Intlike otherwise) {
			if (this->options.contains(key))
				return barosavnc::safe_stoi<Intlike>(this->options[key]).value_or(otherwise);
			return otherwise;
		}
	};
}





#endif
