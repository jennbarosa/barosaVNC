#include "../include/wire.hpp"
#include <curl/easy.h>
#include <memory.h>


namespace barosavnc {
	basic_curl::basic_curl(const std::string_view url, bool verbose)
		: url{url}, curl{nullptr}, ok{true}, response{} {

		auto curl_global_initted = _barosavnc_curl_global_init();
		if (!curl_global_initted) { ok = false; return; }

		curl = curl_easy_init();
		if (!curl) { ok = false; return; }
		
		if (verbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_URL, url.data());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &_barosavnc_curl_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err);
	}

	basic_curl::~basic_curl() {
		if (curl) {
			curl_easy_cleanup(curl);
			curl = nullptr;
		}
	}

	std::pair<std::string&, int> basic_curl::GET() {
		response.clear();
		memset(curl_err, 0, sizeof(curl_err));

		if (!ok || !curl) return {response, -1};

		CURLcode rc = curl_easy_perform(curl);
		if (rc != CURLE_OK) {
			if (curl_err[0]) return { response, -1 };
			return { response, -1 };
		}

		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		return { response, static_cast<int>(http_code) };
	}

	std::pair<std::string&, int> basic_curl::POST(nlohmann::json payload) {
		std::vector<basic_curl_mime_file> container{};
		return POST(payload, container);
	}


	std::pair<std::string&, int> basic_curl::POST(nlohmann::json payload, basic_curl_mime_file& file) {
		std::vector<basic_curl_mime_file> container{file};
		return POST(payload, container);
	}


	std::pair<std::string&, int> basic_curl::POST(nlohmann::json payload, std::vector<basic_curl_mime_file>& files) {
	 	response.clear();
		memset(curl_err, 0, sizeof(curl_err));
		if (!ok || !curl) return {response, -1};

		curl_mime* mime = curl_mime_init(curl);
		curl_mimepart* part = nullptr;

		part = curl_mime_addpart(mime);
		curl_mime_name(part, "payload_json");
		curl_mime_type(part, "application/json");

		auto json_str = payload.dump();
		curl_mime_data(part, json_str.c_str(), static_cast<size_t>(json_str.size()));

		for (size_t i = 0; i < files.size(); ++i) {
			const auto& f = files[i];
			part = curl_mime_addpart(mime);
			curl_mime_name(part, f.fieldname.c_str());             // e.g. "files[0]"
			curl_mime_filename(part, f.filename.c_str());           // e.g. "myfile.png"
			curl_mime_type(part, f.content_type.c_str());           // e.g. "image/png"
			curl_mime_data(part, static_cast<const char*>(f.data), f.data_size);
		}

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

		CURLcode rc = curl_easy_perform(curl);
		curl_mime_free(mime);
		if (rc != CURLE_OK) return {response, -1};

		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		return {response, static_cast<int>(http_code)};
	}

	std::shared_ptr<void> basic_curl::_barosavnc_curl_global_init() {
		static std::shared_ptr<void> guard = []() -> std::shared_ptr<void> {
			if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
				return {};
			}
			return std::shared_ptr<void>(new int(69), [](void*) { curl_global_cleanup(); });
		}();
		return guard;
	}

	size_t basic_curl::_barosavnc_curl_write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
		if (!userdata) return 0;
		auto* resp = static_cast<std::string*>(userdata);
		resp->append(static_cast<char*>(ptr), size * nmemb);
		return size * nmemb;
	}
}
