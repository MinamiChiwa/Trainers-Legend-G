#pragma once

#include <string>

namespace request_convert
{
	bool live_bypass_pack(std::string pack, std::vector<uint8_t>* new_buffer);
}
