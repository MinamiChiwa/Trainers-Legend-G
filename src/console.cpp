#include "stdinclude.hpp"

std::map<std::size_t, std::string> ensure_latest_static_cache(const std::string& staticDictPath);

namespace
{
	void console_thread()
	{
		std::string line;

		while (true)
		{
			std::cin >> line;

			std::cout << "\n] " << line << "\n";

			if (line == "reload")
			{
				std::ifstream config_stream {"config.json"};
				std::vector<std::string> dicts {};

				rapidjson::IStreamWrapper wrapper {config_stream};
				rapidjson::Document document;

				document.ParseStream(wrapper);

				if (!document.HasParseError())
				{
					auto& dicts_arr = document["dicts"];
					auto len = dicts_arr.Size();

					for (size_t i = 0; i < len; ++i)
					{
						auto dict = dicts_arr[i].GetString();

						dicts.push_back(dict);
					}
				}

				config_stream.close();
				local::reload_textdb(&dicts, ensure_latest_static_cache(g_static_dict_path));
			}
		}
	}
}

void start_console()
{
#ifdef _DEBUG
	std::thread(console_thread).detach();
#endif
}