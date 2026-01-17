#include "../include/cli.hpp"
#include <initializer_list>
#include <string_view>


namespace barosavnc {
	cli::cli(int* argc, char** argv) 
		: argc(argc), argv(argv), executable(argv[0]) {}

	bool cli::parse() {
		++argv; --*argc;
		if (*argc == 0) {
			this->display_usages();
			return false;
		}

		while (*argc > 0) {
			const std::string arg{argv[0]};

			if (arg.starts_with("-") && arg.length()>1) {
				if (*argc > 1 && argv[1][0] != '-') {
					++argv; --*argc;
					this->options[arg] = std::string_view{argv[0]};
				} else {
					this->flags.insert(arg);
				}
			}

			++argv; --*argc;
		}

		return true;
	}
}
