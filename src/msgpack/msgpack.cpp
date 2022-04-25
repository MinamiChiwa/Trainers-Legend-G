#include "msgpack.hpp"

namespace msgPrase
{
	const std::vector<int> header_regions = { 4, 56, 72, 88, 104, 120 };
	nlohmann::json praseRequestPack(const std::string& data)
	{
		try
		{
			const uint32_t offset = *(uint32_t*)data.c_str();
			if (offset != 166)
			{
				std::cout << "Unknown offset detected: " << offset << "!\n";
			}

			const auto v = std::string_view(data);

			return nlohmann::json::from_msgpack(v.substr(4 + offset));
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
	void initDMMToken(nlohmann::json pack)
	{
		if (not isDMMTokenLoaded)
		{
			try
			{
				DMMViewerID = pack["dmm_viewer_id"];
				DMMOnetimeToken = pack["dmm_onetime_token"];
				printf("Successfully read DMM Token. Fast Reboot is available.\n");
				isDMMTokenLoaded = true;
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
