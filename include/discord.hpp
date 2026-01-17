#ifndef barosavnc_H
#define barosavnc_H
#include "wire.hpp"
#include "util.hpp"
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace barosavnc::discord {
	struct footer {
		std::string_view text;
		std::string_view icon_url;
		std::string_view proxy_icon_url;
	};

	struct image {
		std::string_view url;
		std::string_view proxy_url;
		int width;
		int height;
	};

	struct thumbnail {
		std::string_view url;
		std::string_view proxy_url;
		int width;
		int height;
	};

	struct video {
		std::string_view url;
		std::string_view proxy_url;
		int width;
		int height;
	};

	struct provider {
		std::string_view name;
		std::string_view url;
	};

	struct author {
		std::string_view name;
		std::string_view url;
		std::string_view icon_url;
		std::string_view proxy_icon_url;
	};

	struct field {
		std::string_view name;
		std::string_view value;
		bool inline_;
	};

	struct embed {
		std::string_view title;
		std::string_view type = "rich";
		std::string_view description;
		std::string_view url;
		std::string_view timestamp_iso8601;
		uint32_t color;
		footer footer_element;
		image image_element;
		thumbnail thumbnail_element;
		video video_element;
		provider embed_provider;
		author user;
	};

	struct attachment {
		int id;
		std::string_view description;
		std::string_view filename;
	};

	struct webhook {
		std::string_view url;
		std::string_view content;
		std::string_view username;
		std::string_view avatar_url;
		embed em;
		attachment file;
		bool tts;
	};

	inline void set_local_attachment(webhook& webhook, basic_curl_mime_file& file) {
		// TODO: support multiple unique mime attachments in webhook
		file.fieldname = "files[0]";
		webhook.file = {
			.id = 0,
			.description = "attachment",
			.filename = file.filename,
		};
	}

	using json = nlohmann::json;
	// to_json implementations for the structs
	inline void to_json(json& j, const footer& f) {
		j = json{{"text", f.text},
			   {"icon_url", f.icon_url},
			   {"proxy_icon_url", f.proxy_icon_url}};
	}

	inline void to_json(json& j, const image& i) {
		j = json{{"url", i.url},
			   {"proxy_url", i.proxy_url},
			   {"width", i.width},
			   {"height", i.height}};
	}

	inline void to_json(json& j, const thumbnail& t) {
		j = json{{"url", t.url},
			   {"proxy_url", t.proxy_url},
			   {"width", t.width},
			   {"height", t.height}};
	}

	inline void to_json(json& j, const video& v) {
		j = json{{"url", v.url},
			   {"proxy_url", v.proxy_url},
			   {"width", v.width},
			   {"height", v.height}};
	}

	inline void to_json(json& j, const provider& p) {
		j = json{{"name", p.name},
			   {"url", p.url}};
	}

	inline void to_json(json& j, const author& a) {
		j = json{{"name", a.name},
			   {"url", a.url},
			   {"icon_url", a.icon_url},
			   {"proxy_icon_url", a.proxy_icon_url}};
	}

	inline void to_json(json& j, const field& f) {
		j = json{{"name", f.name},
			   {"value", f.value},
			   {"inline", f.inline_}};
	}

	inline void to_json(json& j, const attachment& a) {
		j = json{{"id", a.id},
			   {"description", a.description},
			   {"filename", a.filename}};
	}

	inline void to_json(json& j, const embed& e) {
		j = json{
			{"title", e.title},
			{"type", e.type},
			{"description", e.description},
			{"url", e.url},
			{"timestamp", e.timestamp_iso8601},
			{"color", e.color},
			{"footer", e.footer_element},
			{"image", e.image_element},
			{"thumbnail", e.thumbnail_element},
			{"video", e.video_element},
			{"provider", e.embed_provider},
			{"author", e.user},
			{"fields", json::array()}
		};
	}

	inline void to_json(json& j, const webhook& w) {
		j = json{
			{"content",    w.content},
			{"username",   w.username},
			{"avatar_url", w.avatar_url},
			{"tts",        w.tts},
			{"embeds",     json::array()},
			{"attachments",     json::array()}
		};

		j["embeds"].push_back(w.em);
		j["attachments"].push_back(w.file);
	}
}


#endif
