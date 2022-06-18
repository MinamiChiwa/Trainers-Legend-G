#pragma once

#define NOMINMAX

#include <Windows.h>
#include <shlobj.h>

#include <cinttypes>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <string>
#include <thread>
#include <unordered_map>
#include <map>
#include <thread>
#include <variant>

#include <MinHook.h>

#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "il2cpp/il2cpp_symbols.hpp"
#include "local/local.hpp"
#include "logger/logger.hpp"
#include "auto_update/auto_update.hpp"
#include "msgpack/msgpack.hpp"

extern bool g_dump_entries;
extern bool g_enable_logger;
extern bool g_enable_console;
extern int g_max_fps;
extern bool g_unlock_size;
extern float g_ui_scale;
extern float g_aspect_ratio;
extern bool g_read_request_pack;

extern std::string g_extra_assetbundle_path;

struct UseOriginalFont
{
};

struct UseDefaultFont
{
};

struct UseCustomFont
{
	std::string FontPath;
};

extern std::variant<UseOriginalFont, UseDefaultFont, UseCustomFont> g_replace_font;
extern int g_custom_font_size_offset;
extern int g_custom_font_style;
extern float g_custom_font_linespacing;
extern bool g_replace_assets;
extern bool g_asset_load_log;

extern bool g_auto_fullscreen;
extern std::unique_ptr<AutoUpdate::IAutoUpdateService> g_auto_update_service;
extern std::string g_static_dict_path;
extern bool autoChangeLineBreakMode;
extern int start_width;
extern int start_height;
