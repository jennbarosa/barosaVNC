#ifndef barosavnc_VNC_H
#define barosavnc_VNC_H
#include "../include/util.hpp"
#include <chrono>
#include <rfb/rfbclient.h>
#include <nlohmann/json.hpp>
#include <string_view>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <fmt/core.h>
#include <vector>
#include <semaphore>
#include <mutex>
#include <format>
#include <thread>
#include <filesystem>
#include <unordered_map>

namespace barosavnc { 
	namespace fs = std::filesystem;
	static std::string_view vnc_global_screenshot_prefix;
	static bool vnc_global_should_screenshot;

	static std::mutex g_pw_mtx;
	static std::unordered_map<rfbClient*, std::string> g_passwords;

	struct filter_ip {
		std::string host;
		int port;
		std::string password;
	};

	struct vnc {
		char *host;
		char *password;
		int port;
		rfbClient* client;
		time_t connection_time;
		bool alive;
		bool client_logging;
		bool should_take_screenshot;
		std::string_view screenshot_prefix;

		// TODO: struct with fields for options where you can mix match
		vnc(const std::string& host = "127.0.0.1", 
			 const std::string& password = "", 
			 const int port = 5900,
	  		 const unsigned int connect_timeout = 5,
	  		 bool client_logging = false,
	  		 bool should_take_screenshot = false,
			 const std::string_view screenshot_prefix = "barosavnc_vnc",
			 int bits_per_sample = 8, 
			 int samples_per_pixel = 3, 
			 int bytes_per_pixel = 4);

		~vnc();

		static void framebuffer_update_callback(rfbClient* client, int x, int y, int w, int h);
		static void handle_text_chat_callback(rfbClient* client, int value, char *text);
	};

	struct filter_job_thread_printer {
		std::atomic<bool> done{false};
    	std::jthread printing_job;
		std::vector<std::string>* buf;
		std::mutex* print_mtx;
		size_t max_lines;

		inline void _print_job_thread() {
			fmt::print("\n\n{}", term::HOME);
			while (!done) {
				{ // lock guard scope
				std::lock_guard lock(*print_mtx);
				fmt::print("{}", term::HOME);
				
				size_t line_count = 0;
				for (auto& line : *buf) {
					if (line_count > this->max_lines) break;
					if (line.empty()) continue;
					if (line.contains("failed") || line.contains("success")) {
						continue;
					}
					fmt::print("{}{}\n", term::CLEAR_LINE, line);
					++line_count;
				}
				std::flush(std::cout);
				} // end lock guard

				std::this_thread::sleep_for(std::chrono::milliseconds{200});
			}
		}

		inline void operator()() {
			assert(buf);
			assert(!buf->empty());
			assert(print_mtx);

			fmt::print("!! {}I will mass connect to VNC servers in 2 seconds ...{}\n", term::YELLOW, term::RESET);
			std::this_thread::sleep_for(std::chrono::seconds{2});
			fmt::print("{}", term::CLEAR_SCREEN);

			this->done = false;
			this->printing_job = std::jthread(&filter_job_thread_printer::_print_job_thread, this);
		}
		
		inline ~filter_job_thread_printer() {
			this->done = true;
		 	if (buf && print_mtx) {
				fmt::print("{}{}", term::CLEAR_SCREEN, term::HOME);
				for (auto& line : *buf) {
					fmt::print("{}{}\n", term::CLEAR_LINE, line);
				}
			}
		}
	};

	struct filter_job {
		size_t idx;
		std::counting_semaphore<>* sem;
		std::vector<std::string>* line_buf;
		std::mutex* line_buf_mtx;
		std::mutex* out_mtx;

		filter_ip* ip;
		filter_job_thread_printer* printer;
		std::vector<std::string>* passwords;
		std::vector<filter_ip>* out;
		size_t ips_count;
	
		unsigned int connect_timeout;
		bool client_logging_enabled;

		inline void worker_update_print_slot(size_t term_line_index, const std::string_view fmt_str, auto&&... args) {
			auto s = std::vformat(std::string(fmt_str),
					std::make_format_args(std::forward<decltype(args)>(args)...));
			std::lock_guard lock{*line_buf_mtx};
			(*line_buf)[idx] = s;
		}

		inline std::pair<bool, std::string> attempt_vnc_login_by_exhausting_passwords_list() {
			bool connected = false;
			bool first = true;
			std::string working_password;

			for (const auto& password : *passwords) {
				const auto im_new_str = first ? "* new scan" : "";
				const auto hostport = std::format("{}:{}", ip->host, ip->port);
				const auto password_str = std::format(" (trying password '{}')", password);

				constexpr size_t HOSTCOL = 30;
				constexpr size_t PWORDCOL = 30;
				constexpr size_t IMNEWCOL = 20;
				const auto msg = std::format(
					"{}{:<{}}{}{:<{}}{}{:<{}}{}",
					term::YELLOW,
					hostport, HOSTCOL,
					term::RESET,
					password_str, PWORDCOL,
					term::GREEN,
					im_new_str, IMNEWCOL,
					term::RESET
				);

				worker_update_print_slot(idx, msg);
				std::flush(std::cout);
				first = false;

				barosavnc::vnc conn{ip->host, password, ip->port, connect_timeout, client_logging_enabled};
				if (conn.alive) {
					working_password = password;
					connected = true;
					break;
				}  // TODO: password rate limiting
			}

			return {connected, working_password};
		}

		inline void operator()() noexcept {
			assert(sem);
			assert(line_buf);
			assert(line_buf_mtx);
			assert(ip);
			assert(out);
			assert(ips_count>0);
			assert(printer);	

			using namespace barosavnc;
			sem->acquire();
			
			auto [connected_successfully, working_password] = attempt_vnc_login_by_exhausting_passwords_list();

			if (!connected_successfully) {
				worker_update_print_slot(idx, "{}{}:{} all passwords failed{}", term::RED, ip->host, ip->port, term::RESET);
			} else {
				const auto password_text = working_password.empty() ? 
													  "no password" : 
													  std::format("password was \"{}\"", working_password);
				worker_update_print_slot(idx, "{}{}:{} connection success {}{}", 
						 term::GREEN, ip->host, ip->port, password_text, term::RESET);

				std::lock_guard lock{*out_mtx};
				out->emplace_back(ip->host, ip->port, working_password);
			}

			sem->release();
		}
	};

	struct vnc_servers_filter {
		fs::path input_file;
		fs::path passwords_file;
		std::string_view format_option;
		unsigned int connect_timeout;
		unsigned int max_threads;
		unsigned int max_job_lines;
		bool client_logging;
		std::vector<filter_ip>& out;
		bool failed = true;

		inline void operator()() {
			if (!fs::exists(input_file)) {
				fmt::print("{}fatal{} -i: input file \"{}\" doesn't exist\n", term::RED, term::RESET, input_file.string());
				return;
			}
		
			if (!fs::exists(passwords_file)) {	
				fmt::print("{}fatal{} -pwlist: passwords file \"{}\" doesn't exist\n", term::RED, term::RESET, passwords_file.string());
				return;
			}
		
			std::vector<filter_ip> ips;
			std::ifstream input_stream{input_file};
			if (!input_stream.is_open()) {
				fmt::print("{}fatal{} unable to open input file \"{}\"\n", term::RED, term::RESET, input_file.string());
				return;
			}

			std::vector<std::string> passwords{""};
			std::ifstream passwords_stream{passwords_file};
			if (!passwords_stream.is_open()) {
				fmt::print("{}fatal{} unable to open passwords file \"{}\"\n", term::RED, term::RESET, passwords_file.string());
				return;
			}

			std::string pw;
			while (std::getline(passwords_stream, pw)) {
				passwords.emplace_back(pw);
			}

			if (format_option == "json") {
				std::string content{std::istreambuf_iterator<char>(input_stream), std::istreambuf_iterator<char>()};
				if (!nlohmann::json::accept(content)) {
					fmt::print("{}fatal{} input file \"{}\" is malformed json\n", term::RED, term::RESET, input_file.string());
					return;
				}

				nlohmann::json json = nlohmann::json::parse(content);
				for (const auto& entry : json) {
					if (!entry.contains("ip") || !entry["ip"].is_string()) {
						const auto entry_str = entry.dump();
						fmt::print("{}\n", entry_str);
						fmt::print("^^^^^^^^^^^^^^^^^\n");
						fmt::print("{}ip field in json object is empty or isn't a string{}\n",
								term::RED, term::RESET);
						return;
					}

					const auto host = entry["ip"].get<std::string>();

					for (const auto& host_entry : entry["ports"]) {
						if (!host_entry.contains("port") || !host_entry["port"].is_number_unsigned()) {
							const auto entry_str = host_entry.dump();
							fmt::print("{}\n", entry_str);
							fmt::print("^^^^^^^^^^^^^^^^^\n");
							fmt::print("{}malformed port in json object{} port for \"{}\" is empty or not valid\n", 
									term::RED, term::RESET, host);
							return;
						}

						int port = host_entry["port"].get<int>();
						ips.emplace_back(host, port);
					}
				}
			} else if (format_option == "lines") {
				std::string ip;
				while(std::getline(input_stream, ip)) {
					barosavnc::str::trim(ip);
					auto tokens = barosavnc::str::regex_split(ip, ":");
					if (tokens.empty()) continue;
					
					const auto host = tokens[0];
					if (!barosavnc::str::is_ipv4_host_regex(host)) {
						continue;
					}

					const auto port_str = tokens.size() == 1 ? "" : tokens[1];
					const auto port = barosavnc::safe_stoi<int>(port_str).value_or(5900);
					
					ips.emplace_back(host, port);
				}
			} else {
				fmt::print("{}fatal{} -fmt: unsupported format. only values for this flag are 'json' and 'lines'\n", term::RED, term::RESET, input_file.string());
				return;
			}

			out.clear();

			//
			// parallelelize
			// 
			const size_t MAX_JOB_LINES = max_job_lines;
			const unsigned int MAX_CONCURRENCY = max_threads;
			std::mutex cout_mtx;
			std::mutex out_mtx;
			std::counting_semaphore<> sem{MAX_CONCURRENCY};

			std::vector<std::jthread> jobs{};
			jobs.reserve(ips.size());

			std::vector<std::string> line_buf{ips.size()};
			std::mutex line_buf_mtx;

			filter_job_thread_printer thread_printer{
				.buf=&line_buf,
				.print_mtx=&line_buf_mtx,
				.max_lines=MAX_JOB_LINES,
			};
			thread_printer();
		
			for (size_t i = 0; i < ips.size(); ++i) {
				auto slot = i;
				auto& ip = ips[i];

				filter_job job{
					.idx=slot,
					.sem=&sem, 
					.line_buf=&line_buf,
					.line_buf_mtx=&line_buf_mtx,
					.out_mtx=&out_mtx,
					.ip=&ip,
					.printer=&thread_printer,
					.passwords=&passwords,
					.out=&out,
					.ips_count = ips.size(),
					.connect_timeout=connect_timeout,
					.client_logging_enabled=client_logging,
				};

				jobs.emplace_back(job);
			}

			jobs.clear();
			failed = false;
		}
	};
}

#endif
