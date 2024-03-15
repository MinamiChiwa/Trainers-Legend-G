#include <stdinclude.hpp>
#include "eventHelper/eventHelper.hpp"

extern std::unordered_set<USHORT> sChineseLangIds;
extern std::unordered_set<USHORT> tChineseLangIds;
extern std::unordered_set<USHORT> japaneseLangIds;


namespace EventHelper {
	std::unordered_map<long, EventInfo> events{};

	/*
	Thanks:
	https://kamigame.jp/umamusume/page/152540608660042049.html
	https://github.com/UmamusumeResponseAnalyzer/UmamusumeDeserializeDB5
	*/
	void loadData() {
		try {
			events.clear();

			const auto localLang = GetUserDefaultUILanguage();

			std::filesystem::path dataPath = "localized_data/eventHelper";
			if (sChineseLangIds.contains(localLang)) {
				dataPath /= "events_scn.json";
			}
			else if (tChineseLangIds.contains(localLang)) {
				dataPath /= "events_tcn.json";
			}
			else if (japaneseLangIds.contains(localLang)) {
				dataPath /= "events_jp.json";
			}
			else {
				dataPath /= "events_en.json";
			}
			if (!std::filesystem::exists(dataPath)) {
				printf("Event file: %ls not found.\n", dataPath.c_str());
				return;
			}

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

	bool getEventInfo(const long storyId, EventInfo* eventInfo) {
		if (auto iter = events.find(storyId); iter != events.end()) {
			*eventInfo = iter->second;
			return true;
		}
		return false;
	}

}
