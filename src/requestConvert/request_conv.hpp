#pragma once

#include <string>

namespace request_convert
{
	std::string_view parse_request_pack(const std::string& data);
	bool live_bypass_pack(std::string pack, std::vector<uint8_t>* new_buffer);
}
