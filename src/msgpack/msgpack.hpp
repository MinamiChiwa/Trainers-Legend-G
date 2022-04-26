#pragma once
#include <nlohmann/json.hpp>
#include <exception>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <nlohmann/json.hpp>

namespace msgPrase
{
	nlohmann::json praseRequestPack(const std::string& data);
}

namespace msgFunc
{
	extern bool isDMMTokenLoaded;
	void initDMMToken(nlohmann::json pack);
	void fastReboot();
}