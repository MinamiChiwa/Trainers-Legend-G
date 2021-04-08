#pragma once

namespace local
{
	std::string wide_u8(std::wstring str);
	std::wstring u8_wide(std::string str);
	void load_textdb();
	bool localify_text(std::wstring ref, std::wstring** result);
};
