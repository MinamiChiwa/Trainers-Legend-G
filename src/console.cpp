#include "stdinclude.hpp"

namespace
{
	Resolution_t* (*get_resolution)(Resolution_t* buffer);
	void (*set_resolution)(int width, int height, bool fullscreen);
	bool (*get_fullscreen)();

	int (*get_height)();
	int (*get_width)();

	int orig_height, orig_width;

	void console_thread()
	{
		std::string line;

		while (true)
		{
			std::cin >> line;

			std::cout << "\n] " << line << "\n";

			if (line == "fullscreen")
			{
				auto attached_thread = il2cpp_thread_attach(il2cpp_domain_get());

				bool fullscreen_state = get_fullscreen();
				int height = 0, width = 0;

				if (fullscreen_state)
				{
					height = orig_height;
					width = orig_width;
				}
				else
				{
					orig_height = get_height();
					orig_width = get_width();

					Resolution_t res;
					res = *get_resolution(&res);

					height = res.height;
					width = res.width;
				}

				std::cout << "Toggle fullscreen to " << !fullscreen_state << "\n";

				set_resolution(width, height, !fullscreen_state);

				il2cpp_thread_detach(attached_thread);
			}
		}
	}
}

void start_console()
{
	get_resolution = reinterpret_cast<Resolution_t * (*)(Resolution_t*)>(
		il2cpp_symbols::get_method_pointer(
		"UnityEngine.CoreModule.dll", "UnityEngine",
		"Screen", "get_currentResolution", 0
		)
	);

	set_resolution = reinterpret_cast<void(*)(int, int, bool)>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Screen", "SetResolution", 3
		)
	);

	get_fullscreen = reinterpret_cast<bool (*)()>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Screen", "get_fullScreen", 0
	));

	get_height = reinterpret_cast<int (*)()>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Screen", "get_height", 0
	));

	get_width = reinterpret_cast<int (*)()>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Screen", "get_width", 0
	));


	std::thread(console_thread).detach();
}