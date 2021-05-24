#include <stdinclude.hpp>

namespace
{
	const char* SteamAPI_GetSteamInstallPath()
	{
		static std::string install_path {};
		if (!install_path.empty())
		{
			return install_path.data();
		}

		HKEY reg_key;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\WOW6432Node\\Valve\\Steam", 0, KEY_QUERY_VALUE,
			&reg_key) ==
			ERROR_SUCCESS)
		{
			char path[MAX_PATH] = {0};
			DWORD length = sizeof(path);
			RegQueryValueExA(reg_key, "InstallPath", nullptr, nullptr, reinterpret_cast<BYTE*>(path),
							 &length);
			RegCloseKey(reg_key);

			install_path = path;
		}

		return install_path.data();
	}

	HMODULE steam_overlay_module, steam_client_module;

	void load_client()
	{
		const std::filesystem::path steam_path = SteamAPI_GetSteamInstallPath();
		if (steam_path.empty()) 
			return;

		LoadLibrary((steam_path / "tier0_s64.dll").string().data());
		LoadLibrary((steam_path / "vstdlib_s64.dll").string().data());
		steam_overlay_module = LoadLibrary((steam_path / "gameoverlayrenderer64.dll").string().data());
		steam_client_module = LoadLibrary((steam_path / "steamclient64.dll").string().data());

		if (!steam_client_module)
			return;


	}
}
