#include <stdinclude.hpp>
#include <msgpack.hpp>


namespace request_convert
{
	web::http::http_response send_post(std::wstring url, std::wstring path, std::wstring data, int timeout) {
		web::http::client::http_client_config cfg;
		cfg.set_timeout(utility::seconds(timeout));
		web::http::client::http_client client(url, cfg);
		return client.request(web::http::methods::POST, path, data).get();
	}

	web::http::http_response send_post(std::wstring url, std::string path, std::string data, int timeout) {
		web::http::client::http_client_config cfg;
		cfg.set_timeout(utility::seconds(30));
		web::http::client::http_client client(url, cfg);
		return client.request(web::http::methods::POST, path, data).get();
	}

	web::http::http_response send_msgpack_post(std::wstring url, std::wstring path, std::string msgpack) {
		auto json_data = nlohmann::json::from_msgpack(msgpack, false);
		std::string resp_data = json_data.dump();
		std::wstring wd(resp_data.begin(), resp_data.end());
		return send_post(url, path, wd);
	}

	web::http::http_response send_msgpack_unparse_post(std::wstring url, std::string path, std::string msgpack) {
		return send_post(url, path, msgpack);
	}

	web::http::http_response send_msgpack_post(std::wstring url, std::wstring path, std::string_view msgpack) {
		auto json_data = nlohmann::json::from_msgpack(msgpack, false);
		std::string resp_data = json_data.dump();
		std::wstring wd(resp_data.begin(), resp_data.end());
		return send_post(url, path, wd);
	}

	std::string_view parse_request_pack(const std::string& data) {
		const uint32_t offset = *(uint32_t*)data.c_str();
		if (offset != 166)
		{
			std::cout << "Unknown offset detected: " << offset << "!\n";
		}
		const auto v = std::string_view(data);
		return v.substr(4 + offset);
	}

	bool live_bypass_pack(const std::string pack, std::vector<uint8_t>* new_buffer)
	{
		try
		{
			auto json_data = nlohmann::json::from_msgpack(parse_request_pack(pack), false);
			if (json_data.contains("live_theater_save_info"))
			{
				printf("Catch live_theater_save_info\n");
				for (int i = 0; i < json_data["live_theater_save_info"]["member_info_array"].size(); i++) {
					json_data["live_theater_save_info"]["member_info_array"][i]["chara_id"] = 0;
					json_data["live_theater_save_info"]["member_info_array"][i]["mob_id"] = 8590 + i;
					json_data["live_theater_save_info"]["member_info_array"][i]["dress_id"] = 7;
				}
				const auto new_buf = nlohmann::json::to_msgpack(json_data);
				*new_buffer = new_buf;
				return true;
			}
		}
		catch (std::exception& e)
		{
			printf("Exception occurred in live_bypass_pack: %s\n", e.what());
		}
		return false;
	}

	bool live_unlock_dress(const std::string pack, std::vector<uint8_t>* new_buffer)
	{
		try
		{
			if (!MHotkey::get_is_plugin_open()) return false;

			const auto data = send_msgpack_unparse_post(std::format(L"http://127.0.0.1:{}", http_start_port + 1),
				"/tools/unlock_live_dress", pack);
			if (data.status_code() == 200) {
				wprintf(L"解锁全部服装...\n此功能可能导致游戏崩溃. 若崩溃, 请使用快速重启, 或者关闭此功能!\n");
				printf("Unlock all dress...\nMay cause the game to crash. If it crashes, use `Fast Reboot` or disable this feature.\n");
				std::string resp_str = data.extract_utf8string().get();
				const std::vector<uint8_t> new_buf = nlohmann::json::to_msgpack(nlohmann::json::parse(resp_str));
				*new_buffer = new_buf;
				return true;
			}
			else if (data.status_code() == 500) {
				printf("Remote server error: %s\n", data.extract_utf8string().get().c_str());
			}
		}
		catch (std::exception& e)
		{
			printf("Exception occurred in live_unlock_dress: %s\n", e.what());
		}
		return false;
	}

}
