#include <stdinclude.hpp>

extern bool init_hook();
extern void uninit_hook();

namespace
{
	void create_debug_console()
	{
		AllocConsole();

		// open stdout stream
		auto _ = freopen("CONOUT$", "w+t", stdout);

		SetConsoleTitle("Umamusume - Debug Console");

		// set this to avoid turn japanese texts into question mark
		SetConsoleOutputCP(65001);
		std::locale::global(std::locale(""));

		printf("¥¦¥ÞÄï Localify Patch Loaded! - By GEEKiDoS\n");
	}
}

int __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// the DMM Launcher set start path to system32 wtf????
		std::string module_name;
		module_name.resize(MAX_PATH);
		module_name.resize(GetModuleFileName(nullptr, module_name.data(), MAX_PATH));

		std::filesystem::path module_path(module_name);

		// check name
		if (module_path.filename() != "umamusume.exe")
			return 1;

		std::filesystem::current_path(
			module_path.parent_path()
		);

		create_debug_console();

		std::thread init_thread([]() {
			logger::init_logger();
			local::load_textdb();
			init_hook();
		});
		init_thread.detach();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		uninit_hook();
		logger::close_logger();
	}

	return 1;
}
