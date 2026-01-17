
#ifdef _WIN32
#include <fstream>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "../include/util.hpp"
#include <string_view>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

namespace barosavnc {
	namespace file {
#ifndef _WIN32
		mapped_readonly::mapped_readonly(const std::string_view path) {
			data = MAP_FAILED;
			fd = open(path.data(), O_RDONLY);
			if(fd<0) {
				is_open = false;
				return;
			}

			struct stat st;
			if (fstat(fd, &st) < 0) {
				close(fd);
				is_open = false;
				return;
			}

			size = st.st_size;
			data = mmap(nullptr, size+(has_sentinel?1:0), PROT_READ, MAP_PRIVATE, fd, 0);
			close(fd);
			if(data==MAP_FAILED) {
				is_open = false;
				return;
			}

			is_open = true;
		}

		mapped_readonly::~mapped_readonly() {
			if (data != MAP_FAILED) {
				munmap(data, size + (has_sentinel?1:0));
			}
		}

		const std::string_view mapped_readonly::string_view() const {
			return std::string_view{static_cast<char*>(data), size+(has_sentinel?1:0)};
		}
#endif

	} // namespace file
 
	std::string env_quick(const std::string_view key) {
		if (key.empty()) return {};

		std::ifstream file{".env"};
		if (!file.is_open()) return {};

		std::string line;
		while (std::getline(file, line)) {
			const auto eq = line.find_first_of('=');
			if (eq == std::string::npos) continue;
			
			std::string_view lvw{line};
			const auto k = lvw.substr(lvw.find_first_not_of(" \t\r"), eq);
			const auto v = lvw.substr(eq+1);
			if (k==key && !v.empty()) 
				return std::string(v);
		}

		return {};
	}
}
