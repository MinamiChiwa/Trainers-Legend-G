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
	rapidjson::Document praseRequestPack(const std::string& data);
}

namespace msgFunc
{
	extern bool isDMMTokenLoaded;
	void initDMMToken(rapidjson::Document pack);
	void fastReboot();
}