#pragma once

namespace logger
{
	void init_logger();
	void close_logger();
	void write_entry(size_t hash, std::wstring text);

	void write_test_log(std::wstring text);
	void write_test_log(std::string text);
}
