#ifndef barosavnc_WIRE_H
#define barosavnc_WIRE_H
#include <string_view>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace barosavnc {
	struct basic_curl_mime_file {
		std::string fieldname;
		std::string filename;
		std::string content_type;
		const void* data;
		size_t data_size;
	};

	struct basic_curl {
		const std::string_view url;
		CURL* curl;
		bool ok;
		std::string response;
		char curl_err[CURL_ERROR_SIZE]{0};
		
		basic_curl(const std::string_view url, bool verbose = false);
		~basic_curl();

		basic_curl(const basic_curl&) = delete;
		basic_curl& operator=(const basic_curl&) = delete;

		std::pair<std::string&, int> GET();
		std::pair<std::string&, int> POST(nlohmann::json payload);
		std::pair<std::string&, int> POST(nlohmann::json payload, basic_curl_mime_file& file);
		std::pair<std::string&, int> POST(nlohmann::json payload, std::vector<basic_curl_mime_file>& files);
		
		static std::shared_ptr<void> _barosavnc_curl_global_init();
		static size_t _barosavnc_curl_write_callback(void* ptr, size_t size, size_t nmemb, void* userdata);
	};

	bool is_connected_to_mullvad();
}

#endif
