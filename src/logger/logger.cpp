#include <stdinclude.hpp>

using namespace std;

namespace logger
{
	namespace
	{
		fstream log_file;

		bool enabled = false;
		bool request_exit = false;
		bool has_change = false;

		// copy-pasted from https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
		void replaceAll(std::string& str, const std::string& from, const std::string& to)
		{
			if (from.empty())
				return;
			size_t start_pos = 0;
			while ((start_pos = str.find(from, start_pos)) != std::string::npos)
			{
				str.replace(start_pos, from.length(), to);
				start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
			}
		}
	}

	void init_logger()
	{
		// only output if file exists so regular user will not see it.
		if (g_enable_logger)
		{
			enabled = true;
			log_file.open("dump.txt", ios::app | ios::out);

			thread t([]() {
				while (!request_exit)
				{
					this_thread::sleep_for(1s);

					if (has_change)
					{
						log_file.flush();
						has_change = false;
					}
				}

				log_file.close();
			});

			t.detach();
		}
	}

	void close_logger()
	{
		request_exit = true;
	}

	void write_entry(size_t hash, wstring text)
	{
		if (!enabled)
			return;

		auto u8str = local::wide_u8(text);
		replaceAll(u8str, "\n", "\\n");
		replaceAll(u8str, "\"", "\\\"");

		log_file << "\"" << hash << "\": \"" << u8str << "\",\n";

		has_change = true;
	}
}
