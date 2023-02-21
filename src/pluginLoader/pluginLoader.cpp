#include "stdinclude.hpp"


namespace PluginLoader {
	namespace {
		std::unordered_map<std::wstring, HMODULE> loadedDll{};
	}

	bool loadDll(std::wstring dllName) {
		if (loadedDll.contains(dllName)) return false;

		const auto dllPtr = LoadLibraryW(dllName.c_str());
		const auto isSuccess = dllPtr != NULL;
		if (isSuccess) loadedDll.emplace(dllName, dllPtr);
		return isSuccess;
	}

	bool loadDll(std::string dllName) {
		return loadDll(utility::conversions::utf8_to_utf16(dllName));
	}

	bool freeDll(std::wstring dllName) {
		if (const auto iter = loadedDll.find(dllName); iter != loadedDll.end()) {
			const auto dllPtr = iter->second;
			FreeLibraryAndExitThread(dllPtr, 0);
			loadedDll.erase(iter);
			return true;
		}
		return false;
	}

	std::unordered_map<std::wstring, HMODULE> getLoadedDll() {
		return loadedDll;
	}

}
