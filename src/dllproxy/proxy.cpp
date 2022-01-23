#include <stdinclude.hpp>

extern "C"
{
	void* GetFileVersionInfoA_Original = NULL;
	void* GetFileVersionInfoByHandle_Original = NULL;
	void* GetFileVersionInfoExA_Original = NULL;
	void* GetFileVersionInfoExW_Original = NULL;
	void* GetFileVersionInfoSizeA_Original = NULL;
	void* GetFileVersionInfoSizeExA_Original = NULL;
	void* GetFileVersionInfoSizeExW_Original = NULL;
	void* GetFileVersionInfoSizeW_Original = NULL;
	void* GetFileVersionInfoW_Original = NULL;
	void* VerFindFileA_Original = NULL;
	void* VerFindFileW_Original = NULL;
	void* VerInstallFileA_Original = NULL;
	void* VerInstallFileW_Original = NULL;
	void* VerLanguageNameA_Original = NULL;
	void* VerLanguageNameW_Original = NULL;
	void* VerQueryValueA_Original = NULL;
	void* VerQueryValueW_Original = NULL;
}

using namespace std;

namespace
{
	class version_init
	{
	public:
		version_init()
		{
			std::string dll_path;
			dll_path.resize(MAX_PATH);
			dll_path.resize(GetSystemDirectoryA(dll_path.data(), MAX_PATH));

			dll_path += "\\" + "version.dll"s;

			auto original_dll = LoadLibraryA(dll_path.data());

			GetFileVersionInfoA_Original = GetProcAddress(original_dll, "GetFileVersionInfoA");
			GetFileVersionInfoByHandle_Original = GetProcAddress(original_dll, "GetFileVersionInfoByHandle");
			GetFileVersionInfoExA_Original = GetProcAddress(original_dll, "GetFileVersionInfoExA");
			GetFileVersionInfoExW_Original = GetProcAddress(original_dll, "GetFileVersionInfoExW");
			GetFileVersionInfoSizeA_Original = GetProcAddress(original_dll, "GetFileVersionInfoSizeA");
			GetFileVersionInfoSizeExA_Original = GetProcAddress(original_dll, "GetFileVersionInfoSizeExA");
			GetFileVersionInfoSizeExW_Original = GetProcAddress(original_dll, "GetFileVersionInfoSizeExW");
			GetFileVersionInfoSizeW_Original = GetProcAddress(original_dll, "GetFileVersionInfoSizeW");
			GetFileVersionInfoW_Original = GetProcAddress(original_dll, "GetFileVersionInfoW");
			VerFindFileA_Original = GetProcAddress(original_dll, "VerFindFileA");
			VerFindFileW_Original = GetProcAddress(original_dll, "VerFindFileW");
			VerInstallFileA_Original = GetProcAddress(original_dll, "VerInstallFileA");
			VerInstallFileW_Original = GetProcAddress(original_dll, "VerInstallFileW");
			VerLanguageNameA_Original = GetProcAddress(original_dll, "VerLanguageNameA");
			VerLanguageNameW_Original = GetProcAddress(original_dll, "VerLanguageNameW");
			VerQueryValueA_Original = GetProcAddress(original_dll, "VerQueryValueA");
			VerQueryValueW_Original = GetProcAddress(original_dll, "VerQueryValueW");
		};
	};

	version_init init {};
}
