#include "../include/vnc.hpp"
#include "../include/util.hpp"
#include "../include/webp.hpp"
#include "../include/wire.hpp"
#include "../include/discord.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <cctype>
#include <memory>
#include <print>
#include <rfb/rfbclient.h>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <semaphore>
#include <thread>

namespace barosavnc {
	vnc::vnc(const std::string& host,
			 const std::string& password,
			 const int port,
		  	 const unsigned int connect_timeout,
			 bool client_logging,
		  	 bool should_take_screenshot,
			 const std::string_view screenshot_prefix,
			 int bits_per_sample,
			 int samples_per_pixel,
			 int bytes_per_pixel)
		: 
		host{const_cast<char*>(host.data())},
		password{const_cast<char*>(password.data())},
		port{port}, 
		client{rfbGetClient(bits_per_sample, samples_per_pixel, bytes_per_pixel)},
		connection_time{time(NULL)},
		alive{true},
		should_take_screenshot{should_take_screenshot}
	{
		vnc_global_should_screenshot = should_take_screenshot;
		if (vnc_global_screenshot_prefix.empty())
			vnc_global_screenshot_prefix = screenshot_prefix;

		rfbEnableClientLogging = client_logging;
	
		client->connectTimeout = connect_timeout;
		client->readTimeout = 10;
		client->serverHost = strdup(host.c_str());
		client->serverPort = port;
		client->GotFrameBufferUpdate = this->framebuffer_update_callback;
		client->HandleTextChat = this->handle_text_chat_callback;

		{
            std::lock_guard lock{g_pw_mtx};
            g_passwords[client] = password;
        }
		client->GetPassword = [](rfbClient* c) -> char* {
			std::lock_guard lock{g_pw_mtx};
			auto it = g_passwords.find(c);
			return strdup(it == g_passwords.end() ? "" : it->second.c_str());
		};

		this->alive = rfbInitClient(client, NULL, NULL);
		if (!this->alive) {
			std::lock_guard lock{g_pw_mtx};
            g_passwords.erase(client);
            client = nullptr;
		}
	}

	vnc::~vnc() {
		if (client) rfbClientCleanup(client);
		std::lock_guard lock{g_pw_mtx};
        g_passwords.erase(client);
	}

	void vnc::framebuffer_update_callback(rfbClient* client, int x, int y, int w, int h) {
		static size_t img = 0;
		static size_t expected_pixels = client->width * client->height;
		static size_t pixels_so_far = 0;

		static bool first_update = true;
		if (first_update) {
			fmt::print("now receiving VNC screen pieces ...\n");
			first_update = false;
		}

		if (!vnc_global_should_screenshot) return;

		pixels_so_far += w * h;

		if (pixels_so_far >= expected_pixels) {
			fmt::print("received entire vnc screen... creating webp...\n");

			barosavnc::webp webp{client, 0, 0, client->width, client->height};
			fmt::print("webp encoded image from frame buffer: {} bytes\n", webp.bytes_count);

			const auto path = std::format("{}{}.webp", vnc_global_screenshot_prefix, ++img);
		
			// send webp img capture of frame buffer to discord webhook provided in .env file
			const auto filename = path;
			const auto filetype = "image/webp";
			const auto attachment_str = std::format("attachment://{}", filename);
			const auto description = std::format("VNC server: {}:{}\n", 
										client->serverHost, client->serverPort);
			const auto footer_text = std::format("Resolution: {}x{}", client->width, client->height);

			barosavnc::discord::webhook webhook{
				.username = "barosavnc VNC webhook",
				.avatar_url = "",
				.em = {
					.title = "barosavnc VNC screen capture",
					.description = description,
					.url = "https://google.com/",
					.color = 0x008000,
					.footer_element = {
						.text = footer_text,
						.icon_url = "",
					},
					.image_element = {
						.url = attachment_str,
						.width = client->width,
						.height = client->height,
					},
				},
			};

			barosavnc::basic_curl_mime_file file_attachment{
				.filename = filename,
				.content_type = filetype,
				.data = webp.bytes,
				.data_size = webp.bytes_count
			};

			barosavnc::discord::set_local_attachment(webhook, file_attachment);
			const nlohmann::json payload = webhook;

			barosavnc::basic_curl curl{ barosavnc::env_quick("DISCORD_WEBHOOK_URL") };
			curl.POST(payload, file_attachment);

			vnc_global_should_screenshot = false;
			// TODO: properly know when to stop this
			fmt::print("webhook sent\n");
			exit(0);
		}
	}

	void vnc::handle_text_chat_callback(rfbClient* client, int value, char *text) {
		if (value > 0 && text) {
			fmt::print("New chat message: {}\n", text);
		} else {
			fmt::print("received a malformed chat message RCE of some kind?\n");
		}
	}
}
