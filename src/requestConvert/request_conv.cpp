#include <stdinclude.hpp>

namespace request_convert
{
	std::string_view parse_request_pack(const std::string& data) {
		const uint32_t offset = *(uint32_t*)data.c_str();
		if (offset != 166)
		{
			std::cout << "Unknown offset detected: " << offset << "!\n";
		}
		const auto v = std::string_view(data);
		return v.substr(4 + offset);
	}

	bool live_bypass_pack(std::string pack, std::vector<uint8_t>* new_buffer)
	{
		try
		{
			auto json_data = nlohmann::json::from_msgpack(parse_request_pack(pack), false);
			if (json_data.contains("live_theater_save_info"))
			{
				printf("Catch live_theater_save_info\n");
				for (int i = 0; i < json_data["live_theater_save_info"]["member_info_array"].size(); i++) {
					json_data["live_theater_save_info"]["member_info_array"][i]["chara_id"] = 0;
					json_data["live_theater_save_info"]["member_info_array"][i]["mob_id"] = 8590 + i;
					json_data["live_theater_save_info"]["member_info_array"][i]["dress_id"] = 7;
				}
				auto new_buf = nlohmann::json::to_msgpack(json_data);
				*new_buffer = new_buf;
				return true;
			}
			return false;
		}
		catch (std::exception& e)
		{
			printf("Exception occurred in live_bypass_pack: %s\n", e.what());
			return false;
		}

	}

}
