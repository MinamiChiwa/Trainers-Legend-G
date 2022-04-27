#include "msgpack.hpp"


namespace msgPrase
{
	const std::vector<int> header_regions = { 4, 56, 72, 88, 104, 120 };

	std::string convert_pack(const std::string_view data) {
		msgpack::unpacked msg;
		msgpack::unpack(msg, std::string{ data }.c_str(), data.size());
		msgpack::object obj = msg.get();
		std::stringstream ss;
		ss << obj;
		return ss.str();
	}

	rapidjson::Document praseRequestPack(const std::string& data)
	{
		try
		{
			const uint32_t offset = *(uint32_t*)data.c_str();
			if (offset != 166)
			{
				std::cout << "Unknown offset detected: " << offset << "!\n";
			}

			const auto v = std::string_view(data);
			rapidjson::Document document;
			const auto pack_data = convert_pack(v.substr(4 + offset));

			std::regex re("\"viewer_id\":(.*?),\"");
			const auto rep_result = std::regex_replace(pack_data, re, "\"");

			// std::cout << "pack_data:" << pack_data << std::endl;
			// std::cout << "pack_replaced_data:" << rep_result << std::endl;

			document.Parse(rep_result.c_str());
			return document;

		}
		catch (std::exception e)
		{
			printf("Error: %s\n", e.what());
			return NULL;
		}
	}
}

namespace msgFunc
{
	std::string DMMViewerID, DMMOnetimeToken;
	bool isDMMTokenLoaded = false;
	void initDMMToken(rapidjson::Document pack)
	{

		if ((pack == NULL) || pack.HasParseError()) {
			printf("Pack failed. Fast Reboot is not available.\n");
			return;
		}

		if (not isDMMTokenLoaded)
		{
			try
			{
				if (pack.HasMember("dmm_viewer_id") & pack.HasMember("dmm_onetime_token")) {
					DMMViewerID = pack["dmm_viewer_id"].GetString();
					DMMOnetimeToken = pack["dmm_onetime_token"].GetString();
					printf("Successfully read DMM Token. Fast Reboot is available.\n");
					isDMMTokenLoaded = true;
				}
				else {
					printf("Read DMM Token failed. Fast Reboot is not available.\n");
				}

			}
			catch (const char* msg)
			{
				std::cout << msg << std::endl;
				return;
			}
		}
	}
	void fastReboot()
	{
		if (not isDMMTokenLoaded)
		{
			printf("Error: Unable to reboot, can't read DMM Token. Did you enable `readRequestPack`?\n");
			return;
		}
		std::string rebootCMD = R"(
@echo off
setlocal
taskkill /im "umamusume.exe" >NUL

:waitloop
tasklist | find /i "umamusume.exe" >NUL
if %ERRORLEVEL% == 0 goto waitloop

start "" umamusume.exe /viewer_id=)" + DMMViewerID +
" /onetime_token=" + DMMOnetimeToken + "\ndel reboot.bat";
		std::ofstream bat("reboot.bat");
		if (bat.is_open())
		{
			bat.write(rebootCMD.c_str(), rebootCMD.size());
			bat.close();

			try
			{
				ShellExecuteA(NULL, "open", "cmd.exe", "/c reboot.bat", NULL, SW_HIDE);
				exit(0);
			}
			catch (std::exception e)
			{
				printf("Error: %s\n", e.what());
			}
		}
		else
		{
			printf("Cannot make `reboot.bat`. Please try again.\n");
		}
	}
}
