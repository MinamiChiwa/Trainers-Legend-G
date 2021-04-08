#include <stdinclude.hpp>

using namespace std;

namespace local
{
	namespace
	{
		unordered_map<size_t, wstring> text_db;

		wstring u8_wide(u8string str)
		{
			wstring result;
			result.resize(str.length());

			MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<char*>(str.data()),
				str.size(), result.data(), result.length());

			return result;
		}
	}

	wstring u8_wide(string str)
	{
		u8string temp;
		temp.resize(str.size());
		str.copy(reinterpret_cast<char*>(temp.data()), str.size());
		return u8_wide(temp);
	}

	string wide_u8(wstring str)
	{
		string result;
		result.resize(str.length() * 4);

		int len = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.length(), result.data(), result.size(), nullptr, nullptr);

		result.resize(len);

		return result;
	}

	void load_textdb()
	{
		auto db = YAML::LoadFile("localized_strings.yaml");

		if (db)
		{
			for (const auto& entry : db)
			{
				auto key = entry.first.as<size_t>();
				auto value = u8_wide(entry.second.as<string>());

				wprintf(L"%llu - %s\n", key, value.data());

				text_db.emplace(key, value);
			}
		}
	}

	bool localify_text(wstring ref, wstring** result)
	{
		auto hash = std::hash<wstring>{}(ref);

		if (text_db.contains(hash))
		{
			*result = &text_db[hash];
			return true;
		}

		return false;
	}
}
