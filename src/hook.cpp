#include <vector>
#include <string>

#include <MinHook.h>
#include "il2cpp_symbols.hpp"

using namespace std;

namespace
{
	void path_game_assembly();

	bool mh_inited = false;
	vector<void*> enabled_hooks;

	void dump_bytes(void* pos)
	{
		printf("Hex dump of %p\n", pos);

		char* memory = reinterpret_cast<char*>(pos);

		for (int i = 0; i < 0x20; i++) {
			if (i > 0 && i % 16 == 0)
				printf("\n");

			char byte = *(memory++);

			printf("%02hhX ", byte);
		}

		printf("\n");
	}

	void* load_library_w_orig = nullptr;
	HMODULE __stdcall load_library_w_hook(const wchar_t* path)
	{
		// GameAssembly.dll code must be loaded and decrypted while loading criware library
		if (path == L"cri_ware_unity.dll"s)
		{
			path_game_assembly();

			MH_DisableHook(LoadLibraryW);
			MH_RemoveHook(LoadLibraryW);

			// use original function beacuse we have unhooked that
			return LoadLibraryW(path);
		}

		return reinterpret_cast<decltype(LoadLibraryW)*>(load_library_w_orig)(path);
	}

	void* populate_with_errors_orig = nullptr;
	bool __fastcall populate_with_errors_hook(void* _this, Il2CppString* str, void* settings, void* context)
	{
		auto text = L"รþมหรþมห";
		auto test = il2cpp_string_new_utf16(text, lstrlenW(text));

		return reinterpret_cast<decltype(populate_with_errors_hook)*>(populate_with_errors_orig)(
			_this, test, settings, context
			);
	}

	void path_game_assembly()
	{
		if (!mh_inited)
			return;

		printf("Trying to patch GameAssembly.dll...\n");

		auto il2cpp_module = GetModuleHandle("GameAssembly.dll");

		// load il2cpp exported functions
		init_il2cpp(il2cpp_module);

		// hook UnityEngine.TextGenerator::PopulateWithErrors to modify text
		// address is +1D9C9C0 for now, maybe call il2cpp function to get that later
		auto populate_offset = reinterpret_cast<void*>(
			0x1D9C9C0 + reinterpret_cast<uintptr_t>(il2cpp_module)
		);

		printf("UnityEngine.TextGenerator::PopulateWithErrors at %p\n", populate_offset);
		dump_bytes(populate_offset);

		MH_CreateHook(populate_offset, populate_with_errors_hook, &populate_with_errors_orig);
		MH_EnableHook(populate_offset);

		enabled_hooks.push_back(populate_offset);
	}
}

bool init_hook()
{
	if (mh_inited)
		return false;

	if (MH_Initialize() != MH_OK)
		return false;

	mh_inited = true;

	MH_CreateHook(LoadLibraryW, load_library_w_hook, &load_library_w_orig);
	MH_EnableHook(LoadLibraryW);

	return true;
}

void uninit_hook()
{
	if (!mh_inited)
		return;

	for (auto hook : enabled_hooks)
		MH_DisableHook(hook);

	enabled_hooks.clear();

	MH_Uninitialize();
}
