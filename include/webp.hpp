#ifndef barosavnc_WEBP_H
#define barosavnc_WEBP_H
#include <cstdint>
#include <filesystem>
#include <webp/encode.h>
#include <rfb/rfbclient.h>

namespace barosavnc {
	namespace fs = std::filesystem;
	struct webp {
		uint8_t* bytes;
		size_t bytes_count;

		webp(rfbClient* client, int x, int y, int w, int h, float quality = 75.f);
		~webp();

		bool save_to_file(const fs::path& path);
	};
}

#endif
