#include "../include/webp.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <webp/encode.h>

namespace fs = std::filesystem;

barosavnc::webp::webp(rfbClient* client, int x, int y, int w, int h, float quality) {
	// ********************
	// NOTE: this is a modified .ppm example ripped directly from the libvncserver repo to support WEBP
	// https://github.com/LibVNC/libvncserver/blob/master/examples/client/ppmtest.c
	// ********************
	
	if (!client || !client->frameBuffer) return;

    rfbPixelFormat* pf = &client->format;
    int bpp = pf->bitsPerPixel / 8;
    if (bpp != 1 && bpp != 2 && bpp != 3 && bpp != 4) {
        rfbClientLog("unsupported bpp=%d\n", bpp);
        return;
    }

    int row_stride = client->width * bpp;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > client->width)  w = client->width - x;
    if (y + h > client->height) h = client->height - y;
    if (w <= 0 || h <= 0) return;

    std::vector<uint8_t> rgb24((size_t)w * h * 3);
    uint8_t* dst = rgb24.data();

    for (int row = 0; row < h; ++row) {
        const uint8_t* src_row = client->frameBuffer + (size_t)(y + row) * row_stride;
        for (int col = 0; col < w; ++col) {
            const uint8_t* p = src_row + (size_t)(x + col) * bpp;

            unsigned int v = 0;
			switch (bpp) {
			case 4: v = *(const unsigned int*)p; break;
			case 3: v = (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16); break;
			case 2: v = *(const unsigned short*)p; break;
			case 1: v = *p; break;
			}

            uint8_t r8 = (uint8_t)((v >> pf->redShift)   * 256 / (pf->redMax + 1));
            uint8_t g8 = (uint8_t)((v >> pf->greenShift) * 256 / (pf->greenMax + 1));
            uint8_t b8 = (uint8_t)((v >> pf->blueShift)  * 256 / (pf->blueMax + 1));

            *dst++ = r8;
            *dst++ = g8;
            *dst++ = b8;
        }
	}

	// throw bytes at libwebp
    size_t webp_size = WebPEncodeRGB(rgb24.data(), w, h, w * 3, quality, &this->bytes);
    if (webp_size == 0 || !this->bytes) {
        rfbClientLog("WebP encoding failed (size=0)\n");
        if (this->bytes) WebPFree(this->bytes);
        return;
    }
	this->bytes_count = webp_size;
}

barosavnc::webp::~webp() {
	if (this->bytes) {
		WebPFree(this->bytes);
	}
}

bool barosavnc::webp::save_to_file(const fs::path& path) {
	if (!this->bytes || this->bytes_count == 0) return false;
	if (path.empty()) return false;

	fs::create_directories(path.parent_path());

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false; 

    file.write(reinterpret_cast<const char*>(this->bytes), this->bytes_count);

	return true;    
}



