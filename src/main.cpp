#include <Windows.h>
#include <locale.h>

#include <thread>

#include <MinHook.h>
#include "il2cpp_symbols.hpp"

extern bool init_hook();
extern void uninit_hook();

void create_debug_console()
{
	AllocConsole();

	// open stdout stream
	auto _ = freopen("CONOUT$", "w+t", stdout);

	// set this to avoid turn japanese texts into question mark
	SetConsoleOutputCP(65001);
	setlocale(LC_ALL, "jpn");
}

int __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#if _DEBUG
		create_debug_console();
#endif

		std::thread init_thread(init_hook);
		init_thread.detach();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		uninit_hook();
	}

	return 1;
}
