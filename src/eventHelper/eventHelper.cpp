#include <stdinclude.hpp>
#include "eventHelper/eventHelper.hpp"

extern std::unordered_set<USHORT> sChineseLangIds;
extern std::unordered_set<USHORT> tChineseLangIds;
extern std::unordered_set<USHORT> japaneseLangIds;


namespace EventHelper {
	std::unordered_map<long, EventInfo> events{};
	std::unordered_set<long> serverExistsEventId{};

	bool isServerMode = false;
	std::wstring systemLang = L"";
	std::wstring serverURL = L"https://umaevent.chinosk6.cn";

	void loadServerData() {
		try {
			serverExistsEventId.clear();
			const auto resp = request_convert::send_post(serverURL, std::format(L"/get_all_events_id?l={}", systemLang), L"");
			if (resp.status_code() != 200) {
				printf("Load event faield: %d\n", resp.status_code());
				return;
			}
			const auto respStr = resp.extract_utf8string().get();
			auto serverIds = nlohmann::json::parse(respStr);
			const bool isSuccess = serverIds["s"];
			if (isSuccess) {
				for (auto& i : serverIds["d"]) {
					const long id = i;
					serverExistsEventId.insert(id);
				}
			}
		}
		catch (std::exception& e) {
			printf("loadServerData error: %s\n", e.what());
		}
	}

	/*
	Thanks:
	https://kamigame.jp/umamusume/page/152540608660042049.html
	https://github.com/UmamusumeResponseAnalyzer/UmamusumeDeserializeDB5
	*/
	void loadData() {
		try {
			events.clear();

			const auto localLang = GetUserDefaultUILanguage();

			std::filesystem::path dataPath = DLL_DIR / "localized_data/eventHelper";
			if (sChineseLangIds.contains(localLang)) {
				systemLang = L"scn";
				dataPath /= "events_scn.json";
			}
			else if (tChineseLangIds.contains(localLang)) {
				systemLang = L"tcn";
				dataPath /= "events_tcn.json";
			}
			else if (japaneseLangIds.contains(localLang)) {
				systemLang = L"jp";
				dataPath /= "events_jp.json";
			}
			else {
				systemLang = L"en";
				dataPath /= "events_en.json";
			}
			if (!std::filesystem::exists(dataPath)) {
				isServerMode = true;
				std::thread([]() {
					loadServerData();
					}).detach();
				return;
			}
			isServerMode = false;

			std::ifstream file(dataPath);
			if (!file.is_open()) {
				printf("Load %ls failed: file not found.\n", dataPath.c_str());
				return;
			}
			std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			auto jsonData = nlohmann::json::parse(fileContent);

			int loadCount = 0;
			for (auto& i : jsonData) {
				const long id = i["Id"];
				const std::string name = i["Name"];
				std::list<EventChoice> choices{};

				for (auto& choice : i["Choices"]) {
					auto& currChoice = choice[0];
					const std::string option = currChoice["Option"];
					const std::string successEffect = currChoice["SuccessEffect"];
					const std::string failedEffect = currChoice["FailedEffect"];

					choices.push_back(EventChoice{
						.Option=option, 
						.SuccessEffect = successEffect,
						.FailedEffect = failedEffect
						});
					loadCount++;
				}

				events.emplace(id, EventInfo{ .Id = id, .Name = name, .Choices = choices });
			}
			printf("%d events loaded.\n", loadCount);

		}
		catch (std::exception& e) {
			printf("Load event info error: %s\n", e.what());
		}
	}

	bool getEventInfoFromServer(const long storyId, EventInfo* eventInfo) {
		try {
			if (!serverExistsEventId.contains(storyId)) {
				return false;
			}
			const auto resp = request_convert::send_post(serverURL, std::format(L"/get_event?l={}&i={}", systemLang, storyId), L"");
			if (resp.status_code() != 200) {
				printf("Get event %ld faield: %d\n", storyId, resp.status_code());
				return false;
			}
			const auto respStr = resp.extract_utf8string().get();
			auto jsonData = nlohmann::json::parse(respStr);
			const bool isSuccess = jsonData["s"];
			if (!isSuccess) {
				return false;
			}

			auto& i = jsonData["d"];

			const long id = i["i"];
			const std::string name = i["n"];
			std::list<EventChoice> choices{};

			for (auto& choice : i["c"]) {
				auto& currChoice = choice[0];
				const std::string option = currChoice["o"];
				const std::string successEffect = currChoice["s"];
				const std::string failedEffect = currChoice["f"];

				choices.push_back(EventChoice{
					.Option = option,
					.SuccessEffect = successEffect,
					.FailedEffect = failedEffect
					});
			}

			const EventInfo getEventInfo{ .Id = id, .Name = name, .Choices = choices };
			events.emplace(id, getEventInfo);
			*eventInfo = getEventInfo;
			return true;
		}
		catch (std::exception& e) {
			printf("getEventInfoFromServer %ld error: %s\n", storyId, e.what());
		}
		return false;
	}

	bool getEventInfo(const long storyId, EventInfo* eventInfo) {
		if (auto iter = events.find(storyId); iter != events.end()) {
			*eventInfo = iter->second;
			return true;
		}
		if (isServerMode) {
			return getEventInfoFromServer(storyId, eventInfo);
		}
		return false;
	}

}
