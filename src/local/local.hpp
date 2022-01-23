#pragma once

namespace local
{
	std::string wide_u8(std::wstring str);
	void load_textdb(const std::vector<std::string> *dicts);
	void reload_textdb(const std::vector<std::string>* dicts);
	bool localify_text(size_t hash, std::string** result);
	Il2CppString* get_localized_string(size_t hash_or_id);
	Il2CppString* get_localized_string(Il2CppString* str);
};
