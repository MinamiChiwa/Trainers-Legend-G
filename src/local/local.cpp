#include <stdinclude.hpp>

using namespace std;

namespace local
{
	namespace
	{
		std::mutex db_lock;
		unordered_map<size_t, string> text_db;
		std::vector<size_t> str_list;
	}

	string wide_u8(wstring str)
	{
		string result;
		result.resize(str.length() * 4);

		int len = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.length(), result.data(), result.size(), nullptr, nullptr);

		result.resize(len);

		return result;
	}

	void unlocked_load_textdb(const vector<string>* dicts);

	void reload_textdb(const vector<string>* dicts)
	{
		std::unique_lock lock(db_lock);
		text_db.clear();
		unlocked_load_textdb(dicts);
	}

	void load_textdb(const vector<string>* dicts)
	{
		std::unique_lock lock(db_lock);
		unlocked_load_textdb(dicts);
	}

	void unlocked_load_textdb(const vector<string> *dicts)
	{
		for (auto dict : *dicts)
		{
			if (filesystem::exists(dict))
			{
				std::ifstream dict_stream {dict};

				if (!dict_stream.is_open())
					continue;

				printf("Reading %s...\n", dict.data());

				rapidjson::IStreamWrapper wrapper {dict_stream};
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

		printf("loaded %llu localized entries.\n", text_db.size());
	}

	bool localify_text(size_t hash, string** result)
	{
		std::unique_lock lock(db_lock);
		if (text_db.contains(hash))
		{
			*result = &text_db[hash];
			return true;
		}

		return false;
	}

	Il2CppString* get_localized_string(size_t hash_or_id)
	{
		string* result;

		if (local::localify_text(hash_or_id, &result))
		{
			return il2cpp_string_new(result->data());
		}

		return nullptr;
	}

	Il2CppString* get_localized_string(Il2CppString* str)
	{
		string* result;

		auto hash = std::hash<wstring> {}(str->start_char);

		if (local::localify_text(hash, &result))
		{
			return il2cpp_string_new(result->data());
		}

		if (g_enable_logger && !std::any_of(str_list.begin(), str_list.end(), [hash](size_t hash1) { return hash1 == hash; }))
		{
			str_list.push_back(hash);

			logger::write_entry(hash, str->start_char);
		}

		return str;
	}
}
