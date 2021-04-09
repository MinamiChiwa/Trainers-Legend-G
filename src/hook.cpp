#include <stdinclude.hpp>

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

		for (int i = 0; i < 0x20; i++)
		{
			if (i > 0 && i % 16 == 0)
				printf("\n");

			char byte = *(memory++);

			printf("%02hhX ", byte);
		}

		printf("\n\n");
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

	std::vector<void*> font_list;

	void* populate_with_errors_orig = nullptr;
	bool __fastcall populate_with_errors_hook(void* _this, Il2CppString* str, TextGenerationSettings_t* settings, void* context)
	{
		if (!std::any_of(font_list.begin(), font_list.end(), [settings](void* font) { return font == settings->font; }))
		{
			font_list.push_back(settings->font);
		}

		return reinterpret_cast<decltype(populate_with_errors_hook)*>(populate_with_errors_orig) (
			_this, local::get_localized_string(str), settings, context
			);
	}

	void* localize_get_orig = nullptr;
	Il2CppString* __fastcall localize_get_hook(int id)
	{
		auto result = local::get_localized_string(id);

		if (result)
			return result;

		return reinterpret_cast<decltype(localize_get_hook)*>(localize_get_orig)(id);
	}

	void dump_all_entries()
	{
		// TextId 0 - 0xA55, 0 is None
		for (int i = 1; i <= 0xA55; ++i)
		{
			auto entry = reinterpret_cast<decltype(localize_get_hook)*>(localize_get_orig)(i);
			logger::write_entry(i, entry->start_char);
		}
	}

	void path_game_assembly()
	{
		if (!mh_inited)
			return;

		printf("Trying to patch GameAssembly.dll...\n");

		auto il2cpp_module = GetModuleHandle("GameAssembly.dll");

		// load il2cpp exported functions
		il2cpp_symbols::init(il2cpp_module);

#pragma region HOOK_MACRO
#define ADD_HOOK(_offset_, _name_, _fmt_) \
	auto _name_##_offset = reinterpret_cast<void*>(_offset_); \
	\
	printf(_fmt_, _name_##_offset); \
	dump_bytes(_name_##_offset); \
	\
	MH_CreateHook(_name_##_offset, _name_##_hook, &_name_##_orig); \
	MH_EnableHook(_name_##_offset); \
	\
	enabled_hooks.push_back(_name_##_offset)
#pragma endregion

		auto populate_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.TextRenderingModule.dll",
			"UnityEngine", "TextGenerator",
			"PopulateWithErrors", 3
		);

		// have to do this way because there's Get(TextId id) and Get(string id)
		// the string one looks like will not be called by elsewhere
		auto localize_get_addr = il2cpp_symbols::find_method("umamusume.dll", "Gallop", "Localize", [](const MethodInfo* method) {
			return method->name == "Get"s && 
				method->parameters->parameter_type->type == IL2CPP_TYPE_VALUETYPE;
		});

		// hook UnityEngine.TextGenerator::PopulateWithErrors to modify text
		ADD_HOOK(populate_addr, populate_with_errors, "UnityEngine.TextGenerator::PopulateWithErrors at %p\n");

		// Looks like they store all localized texts that used by code in a dict
		ADD_HOOK(localize_get_addr, localize_get, "Gallop.Localize.Get(TextId) at %p\n");

		if (strstr(GetCommandLine(), "--dump-entries"))
			dump_all_entries();
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
