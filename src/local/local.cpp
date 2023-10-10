#include <stdinclude.hpp>
// #include "../../build/strcvtid.hpp"

using namespace std;
using namespace MHotkey;
// using namespace LocalStrCvt;

namespace local
{
	namespace
	{
		std::mutex db_lock;
		unordered_map<size_t, string> text_db;
		std::vector<size_t> str_list;
		
		std::unordered_map<std::size_t, StoryTextData> StoryDict;
		std::unordered_map<std::size_t, RaceTextData> RaceDict;

		TextData TextDataContent;
		CharacterSystemTextData CharacterSystemTextDataContent;
		RaceJikkyoCommentData RaceJikkyoCommentDataContent;
		RaceJikkyoMessageData RaceJikkyoMessageDataContent;
	}

	string wide_u8(wstring str)
	{
		string result;
		result.resize(str.length() * 4);

		int len = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.length(), result.data(), result.size(), nullptr, nullptr);

		result.resize(len);

		return result;
	}

	void unlocked_load_textdb(const vector<string>* dicts, map<size_t, string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict, TextData&& textData, CharacterSystemTextData&& characterSystemTextData, RaceJikkyoCommentData&& raceJikkyoCommentData, RaceJikkyoMessageData&& raceJikkyoMessageData)
	{
		for (auto&& [id, content] : staticDict)
		{
			text_db.emplace(id, std::move(content));
		}

		for (const auto& dict : *dicts)
		{
			if (filesystem::exists(dict))
			{
				std::ifstream dict_stream{ dict };

				if (!dict_stream.is_open())
					continue;

				printf("Reading %s...\n", dict.data());

				rapidjson::IStreamWrapper wrapper{ dict_stream };
				rapidjson::Document document;

				document.ParseStream(wrapper);

				for (auto iter = document.MemberBegin();
					iter != document.MemberEnd(); ++iter)
				{
					auto key = std::stoull(iter->name.GetString());
					auto value = iter->value.GetString();

					text_db.emplace(key, value);
				}

				dict_stream.close();
			}
		}

		StoryDict = std::move(storyDict);
		RaceDict = std::move(raceDict);

		TextDataContent = std::move(textData);
		CharacterSystemTextDataContent = std::move(characterSystemTextData);
		RaceJikkyoCommentDataContent = std::move(raceJikkyoCommentData);
		RaceJikkyoMessageDataContent = std::move(raceJikkyoMessageData);

		printf("loaded %llu localized entries.\n", text_db.size());
		// read_str_config();
	}

	void reload_textdb(const vector<string>* dicts, map<size_t, string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict, TextData&& textData, CharacterSystemTextData&& characterSystemTextData, RaceJikkyoCommentData&& raceJikkyoCommentData, RaceJikkyoMessageData&& raceJikkyoMessageData)
	{
		std::unique_lock lock(db_lock);
		text_db.clear();
		unlocked_load_textdb(dicts, std::move(staticDict), std::move(storyDict), std::move(raceDict), std::move(textData), std::move(characterSystemTextData), std::move(raceJikkyoCommentData), std::move(raceJikkyoMessageData));
	}

	void load_textdb(const vector<string>* dicts, map<size_t, string>&& staticDict, std::unordered_map<std::size_t, StoryTextData>&& storyDict, std::unordered_map<std::size_t, RaceTextData>&& raceDict, TextData&& textData, CharacterSystemTextData&& characterSystemTextData, RaceJikkyoCommentData&& raceJikkyoCommentData, RaceJikkyoMessageData&& raceJikkyoMessageData)
	{
		std::unique_lock lock(db_lock);
		unlocked_load_textdb(dicts, std::move(staticDict), std::move(storyDict), std::move(raceDict), std::move(textData), std::move(characterSystemTextData), std::move(raceJikkyoCommentData), std::move(raceJikkyoMessageData));
	}

	bool localify_text(size_t hash, string** result)
	{
		if (closeTrans.all || closeTrans.staticAndHashTextData) {
			return false;
		}

		std::unique_lock lock(db_lock);
		if (const auto it = text_db.find(hash); it != text_db.end()) {
			*result = &it->second;
			/*
			if (text_db[hash].compare("已解锁上限至最大") == 0) {
				static auto environment_get_stacktrace = 
					reinterpret_cast<Il2CppString* (*)()>(il2cpp_symbols::get_method_pointer("mscorlib.dll", "System", "Environment", "get_StackTrace", 0));

				printf("catchText\n%ls\n\n", environment_get_stacktrace()->start_char);
			}
			*/
			return true;
		}

		return false;
	}

	Il2CppString* get_localized_string(size_t hash_or_id)
	{
		// printf("calling get_localized_string(size_t hash_or_id): %zu\n", hash_or_id);
		string* result;

		if (local::localify_text(hash_or_id, &result))
		{
			return il2cpp_string_new(result->data());
		}

		return nullptr;
	}

	wstring line_break_replace(Il2CppString* text) {
		wstring _ret = L"";
		wstring tmp = text->start_char;
		bool space_flag = false;  // 后接空格
		
		for (int i = 0; i < tmp.length(); i++) {
			if (tmp[i] != L'\n' && tmp[i] != L'\r') {
				_ret += tmp[i];
			}
			else if (space_flag){
				_ret += L'　';
			}

			space_flag = false;
			if (tmp[i] == L'！' || tmp[i] == L'？') {
				space_flag = true;  // 标记空格
			}
		}

		return _ret;
	}

	Il2CppString* get_localized_string(Il2CppString* str)
	{
		if (closeTrans.all || closeTrans.hashTextData) {
			return str;
		}

		string* result;
		wstring result_without_lb = line_break_replace(str);
		
		size_t hash;
		auto hash_with_lb = std::hash<wstring>{}(str->start_char);
		auto hash_without_lb = std::hash<wstring>{}(result_without_lb);
		bool is_without_lb = get_uma_stat();

		if (is_without_lb) {
			hash = hash_without_lb;
		}
		else {
			hash = hash_with_lb;
		}
		

		Il2CppString* t_with_lp = NULL;
		Il2CppString* t_without_lp = NULL;

		if (local::localify_text(hash_without_lb, &result))
		{
			t_without_lp = il2cpp_string_new(result->data());  // "忽略换行符" 模式的文本
		}

		if (local::localify_text(hash_with_lb, &result))
		{
			t_with_lp = il2cpp_string_new(result->data());  // 原文本
		}

		if (t_with_lp != NULL && t_without_lp != NULL) {
			return is_without_lb ? t_without_lp : t_with_lp;
		}
		else if (t_with_lp != NULL || t_without_lp != NULL) {
			return t_with_lp != NULL ? t_with_lp : t_without_lp;
		}


		if (g_enable_logger && !std::any_of(str_list.begin(), str_list.end(), [hash](size_t hash1) { return hash1 == hash; }))
		{

			str_list.push_back(hash);

			logger::write_entry(hash_with_lb, str->start_char);
			if (hash_with_lb != hash_without_lb) {
				logger::write_entry(hash_without_lb, result_without_lb);
			}
		}

		return str;
	}

	StoryTextData const* GetStoryTextData(std::size_t storyId)
	{
		if (closeTrans.all || closeTrans.storyTextData) {
			return nullptr;
		}

		if (const auto iter = StoryDict.find(storyId); iter != StoryDict.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	RaceTextData const* GetRaceTextData(std::size_t raceId)
	{
		if (closeTrans.all || closeTrans.raceTextData) {
			return nullptr;
		}

		if (const auto iter = RaceDict.find(raceId); iter != RaceDict.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	Il2CppString* GetTextData(std::size_t category, std::size_t index)
	{
		if (closeTrans.all || closeTrans.textData || trans_off_textData.contains(category)) {
			return nullptr;
		}
		if (const auto iter = TextDataContent.Data.find(category); iter != TextDataContent.Data.end())
		{
			if (const auto iter2 = iter->second.find(index); iter2 != iter->second.end())
			{
				return il2cpp_string_new_utf16(iter2->second.c_str(), iter2->second.size());
			}
		}

		return nullptr;
	}

	Il2CppString* GetCharacterSystemTextData(std::size_t characterId, std::size_t voiceId)
	{
		if (closeTrans.all || closeTrans.characterSystemTextData) {
			return nullptr;
		}

		if (const auto iter = CharacterSystemTextDataContent.Data.find(characterId); iter != CharacterSystemTextDataContent.Data.end())
		{
			if (const auto iter2 = iter->second.find(voiceId); iter2 != iter->second.end())
			{
				return il2cpp_string_new_utf16(iter2->second.c_str(), iter2->second.size());
			}
		}

		return nullptr;
	}

	Il2CppString* GetRaceJikkyoCommentData(std::size_t id)
	{
		if (closeTrans.all || closeTrans.raceJikkyoCommentData) {
			return nullptr;
		}

		if (const auto iter = RaceJikkyoCommentDataContent.Data.find(id); iter != RaceJikkyoCommentDataContent.Data.end())
		{
			return il2cpp_string_new_utf16(iter->second.c_str(), iter->second.size());
		}

		return nullptr;
	}

	Il2CppString* GetRaceJikkyoMessageData(std::size_t id)
	{
		if (closeTrans.all || closeTrans.raceJikkyoMessageData) {
			return nullptr;
		}

		if (const auto iter = RaceJikkyoMessageDataContent.Data.find(id); iter != RaceJikkyoMessageDataContent.Data.end())
		{
			return il2cpp_string_new_utf16(iter->second.c_str(), iter->second.size());
		}

		return nullptr;
	}
}
