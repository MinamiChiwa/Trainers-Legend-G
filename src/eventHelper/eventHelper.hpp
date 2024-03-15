#pragma once
#include <string>
#include <list>


namespace EventHelper {
	struct EventChoice {
		std::string Option;
		std::string SuccessEffect;
		std::string FailedEffect;
	};

	struct EventInfo {
		long Id;
		std::string Name;
		std::list<EventChoice> Choices;
	};

	void loadData();
	bool getEventInfo(const long storyId, EventInfo* eventInfo);

}
