#include <stdinclude.hpp>
#include <msgpack.hpp>

bool isNoticeUrlRight = true;
bool noticeThreadStart = false;
int pingFailedCount = 0;
bool pinged = false;
std::unordered_set<int> globalReadedNotices{};

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

	std::string Get_autoupdateUrl() {
		std::regex pattern("(http|https)://[^/]+");
		std::string autoupdateUrl = "";
		std::smatch match;
		if (std::regex_search(g_autoupdateUrl, match, pattern)) {
			autoupdateUrl = match[0];
		}
		return autoupdateUrl;
	}

	void check_and_upload_gacha_history(const std::string& pack) {
		static auto get_UserName = reinterpret_cast<Il2CppString * (*)()>(il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop", "GallopUtil", "GetUserName", 0));
		static auto get_dmmViewerId = reinterpret_cast<Il2CppString * (*)()>(il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop", "Certification", "get_dmmViewerId", 0));

		std::thread([pack]() {
			bool isHistory = false;
			try {
				auto jsonPack = nlohmann::json::from_msgpack(pack);
				if (jsonPack.contains("data")) {
					isHistory = true;
					auto& data = jsonPack["data"];
					if (data.contains("gacha_exec_history_array") && data.contains("gacha_reward_history_array")) {
						auto userName = get_UserName();
						auto dmmViewerId = get_dmmViewerId();
						jsonPack["user_name"] = utility::conversions::to_utf8string(userName->start_char);
						jsonPack["dmm_viewer_id"] = utility::conversions::to_utf8string(dmmViewerId->start_char);
						auto resp = send_post(g_upload_gacha_history_endpoint, "/api/upload/usergacha", jsonPack.dump());
						if (resp.status_code() == web::http::status_codes::OK) {
							const auto userId = resp.extract_utf8string().get();
							printf("Upload gacha history success. Your id is: %s\nGo to %ls?uid=%s to view.\n", userId.c_str(), g_upload_gacha_history_endpoint.c_str(), userId.c_str());
						}
					}
				}
			}
			catch (nlohmann::json::exception& ex) {
				if (ex.id != 113) printf("ParseGHError: %s\n", ex.what());
			}
			catch (std::exception& ex) {
				if (isHistory) printf("Upload gacha history failed: %s\n", ex.what());
			}
			}).detach();
	}

	void updateNotice() {
		try
		{
			if (!isNoticeUrlRight) return;
			if (showDialog == nullptr) return;

			static std::string autoupdateUrl = Get_autoupdateUrl();
			if (autoupdateUrl.empty()) return;
			if (!pinged) {
				auto pingResult = send_post(utility::conversions::to_string_t(autoupdateUrl).c_str(), L"/api/notice/ping", L"");
				if (pingResult.status_code() != 200) {
					pingFailedCount++;
					if (pingFailedCount > 3) {
						isNoticeUrlRight = false;
					}
					return;
				}
				pingFailedCount = 0;
				pinged = true;
			}

			std::vector<int> readedNotices{};
			std::string versionStr = "";
			nlohmann::json externalPluginData;
			if (std::filesystem::exists("epconf.json")) {
				std::ifstream ifs("epconf.json");
				std::string content((std::istreambuf_iterator<char>(ifs)),
					std::istreambuf_iterator<char>());
				externalPluginData = nlohmann::json::parse(content);
				if (externalPluginData.contains("more_settings_data")) {
					if (externalPluginData["more_settings_data"].contains("readedNotices")) {
						readedNotices = externalPluginData["more_settings_data"]["readedNotices"].get<std::vector<int>>();
					}
					else {
						externalPluginData["more_settings_data"].emplace("readedNotices", nlohmann::json::array());
					}
				}
				else {
					externalPluginData.emplace("more_settings_data", nlohmann::json::object());
					externalPluginData["more_settings_data"].emplace("readedNotices", nlohmann::json::array());
				}
			}
			if (std::filesystem::exists("version.txt")) {
				std::ifstream ifs("version.txt");
				std::string content((std::istreambuf_iterator<char>(ifs)),
					std::istreambuf_iterator<char>());
				versionStr = content;
			}


			nlohmann::json postJson;
			postJson["is_first"] = readedNotices.empty();
			postJson["readed"] = readedNotices;
			postJson["area"] = GetUserDefaultUILanguage();
			postJson["version"] = versionStr.c_str();


			auto noticeResp = send_post(utility::conversions::to_string_t(autoupdateUrl).c_str(), 
										L"/api/notice/get", 
										utility::conversions::to_string_t(postJson.dump()));
			std::string resp_str = noticeResp.extract_utf8string().get();
			nlohmann::json getNotice = nlohmann::json::parse(resp_str);
			if (getNotice.contains("msg")) {
				if (getNotice["msg"].empty()) {
					return;
				}

				for (auto& i : getNotice["msg"]) {
					std::string title = i["title"];
					std::string content = i["content"];
					int btn1Text = i["btn1Text"];
					int btn2Text = i["btn2Text"];
					int btn3Text = i["btn3Text"];
					int btnCount = i["btnCount"];
					int btnType = i["btnType"];

					int noticeId = i["noticeid"];
					readedNotices.push_back(noticeId);

					if (globalReadedNotices.contains(noticeId)) {
						printf("WARNING: Readed notice: %d, ignore.\n", noticeId);
					}
					else {
						showDialog(il2cpp_string_new(title.c_str()), il2cpp_string_new(content.c_str()),
							btnCount, btn1Text, btn2Text, btn3Text, btnType);
						globalReadedNotices.emplace(noticeId);
					}
				}
			}

			externalPluginData["more_settings_data"]["readedNotices"] = readedNotices;
			std::string output = externalPluginData.dump(4);
			std::ofstream ofs("epconf.json");
			ofs << output;
			ofs.close();
		}
		catch (std::exception& e)
		{
			pinged = false;
			printf("Exception occurred while getting notice: %s\n", e.what());
		}
	}

	void startUpdateNotice() {
		if (noticeThreadStart) return;
		std::thread([]() {
			noticeThreadStart = true;
			Sleep(20 * 1000);
			while (true) {
				if (!noticeThreadStart) break;
				updateNotice();
				Sleep(180 * 1000);
			}
			}).detach();
	}

	void stopUpdateNotice() {
		noticeThreadStart = false;
	}
}
