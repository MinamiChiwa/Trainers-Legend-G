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
		std::optional<std::vector<StoryTextBlock>> Siblings;
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

	struct TextData
	{
		// { category: { index: text } }
		std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::wstring>> Data;
	};

	struct CharacterSystemTextData
	{
		// { character_id: { voice_id: text } }
		std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::wstring>> Data;
	};
	
	struct RaceJikkyoCommentData
	{
		// { id: text }
		std::unordered_map<std::size_t, std::wstring> Data;
	};

	struct RaceJikkyoMessageData
	{
		// { id: text }
		std::unordered_map<std::size_t, std::wstring> Data;
	};

	std::string wide_u8(std::wstring str);
	void load_textdb(const std::vector<std::string>*dicts, std::map<std::size_t, std::string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict, TextData&& textData, CharacterSystemTextData&& characterSystemTextData, RaceJikkyoCommentData&& raceJikkyoCommentData, RaceJikkyoMessageData&& raceJikkyoMessageData);
	void reload_textdb(const std::vector<std::string>* dicts, std::map<std::size_t, std::string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict, TextData&& textData, CharacterSystemTextData&& characterSystemTextData, RaceJikkyoCommentData&& raceJikkyoCommentData, RaceJikkyoMessageData&& raceJikkyoMessageData);
	bool localify_text(size_t hash, std::string** result);
	Il2CppString* get_localized_string(size_t hash_or_id);
	Il2CppString* get_localized_string(Il2CppString* str);

	StoryTextData const* GetStoryTextData(std::size_t storyId);
	RaceTextData const* GetRaceTextData(std::size_t raceId);

	Il2CppString* GetTextData(std::size_t category, std::size_t index);
	Il2CppString* GetCharacterSystemTextData(std::size_t characterId, std::size_t voiceId);
	Il2CppString* GetRaceJikkyoCommentData(std::size_t id);
	Il2CppString* GetRaceJikkyoMessageData(std::size_t id);

}
