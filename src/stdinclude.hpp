#pragma once

#include <Windows.h>

#include <cinttypes>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <string>
#include <thread>
#include <unordered_map>

#include <MinHook.h>

#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>

#include "il2cpp/il2cpp_symbols.hpp"
#include "local/local.hpp"
#include "logger/logger.hpp"

extern bool g_dump_entries;
extern bool g_enable_logger;
extern bool g_enable_console;
extern bool g_unlock_fps;
extern bool g_unlock_size;
extern float g_aspect_ratio;