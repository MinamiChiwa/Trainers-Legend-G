#pragma once
#include <exception>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <rapidjson/document.h>
#include <vector>
#include <msgpack.hpp>
#include <regex>

namespace msgPrase
{
	std::string_view convertRequestPack(const std::string& data);
	rapidjson::Document praseRequestPack(const std::string& data);
	std::string convert_pack(const std::string_view data);
	msgpack::sbuffer to_pack(const std::string data);
	msgpack::sbuffer to_pack(const std::wstring data);
}

namespace msgFunc
{
	extern bool isDMMTokenLoaded;
	void initDMMToken(rapidjson::Document pack);
	void fastReboot();
}