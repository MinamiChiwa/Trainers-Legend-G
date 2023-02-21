#pragma once
#include "stdinclude.hpp"


namespace PluginLoader {
	bool loadDll(std::wstring dllName);
	bool loadDll(std::string dllName);
	bool freeDll(std::wstring dllName);
	std::unordered_map<std::wstring, HMODULE> getLoadedDll();
}
