#include <stdinclude.hpp>

il2cpp_string_new_utf16_t il2cpp_string_new_utf16;

void init_il2cpp(HMODULE game_module)
{
	il2cpp_string_new_utf16 = reinterpret_cast<il2cpp_string_new_utf16_t>(
		GetProcAddress(game_module,"il2cpp_string_new_utf16")
	);
}
