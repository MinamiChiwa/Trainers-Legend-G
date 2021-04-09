#pragma once

namespace local
{
	std::string wide_u8(std::wstring str);
	std::wstring u8_wide(std::string str);
	void load_textdb();
	bool localify_text(size_t hash, std::wstring** result);
	Il2CppString* get_localized_string(size_t hash_or_id);
	Il2CppString* get_localized_string(Il2CppString* str);
};
