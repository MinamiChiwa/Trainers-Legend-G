#pragma once
#include "../mhotkey.hpp"

namespace local
{
	struct StoryTextBlock
	{
		std::wstring Name;
		std::wstring Text;
		std::vector<std::wstring> ChoiceDataList;
		std::vector<std::wstring> ColorTextInfoList;
	};

	struct StoryTextData
	{
		std::wstring Title;
		std::vector<std::optional<StoryTextBlock>> TextBlockList;
	};

	struct RaceTextData
	{
		std::vector<std::wstring> textData;
	};

	std::string wide_u8(std::wstring str);
	void load_textdb(const std::vector<std::string>*dicts, std::map<std::size_t, std::string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict);
	void reload_textdb(const std::vector<std::string>* dicts, std::map<std::size_t, std::string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict);
	bool localify_text(size_t hash, std::string** result);
	Il2CppString* get_localized_string(size_t hash_or_id);
	Il2CppString* get_localized_string(Il2CppString* str);

	StoryTextData const* GetStoryTextData(std::size_t storyId);
	RaceTextData const* GetRaceTextData(std::size_t raceId);
};
