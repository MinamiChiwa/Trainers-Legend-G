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
#include <unordered_set>
#include <map>
#include <thread>
#include <variant>

#include <exception>
#include <vector>
#include <regex>

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
#include "requestConvert/request_conv.hpp"

#include "camera/camera.hpp"
#include "umadb/umadb.hpp"
#include <nlohmann/json.hpp>

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

enum CameraType {
	CAMERA_LIVE = 0,
	CAMERA_RACE = 1
};

struct CloseTrans {
	bool all = false;
	bool textData = false;
	bool storyTextData = false;
	bool raceTextData = false;
	bool characterSystemTextData = false;
	bool raceJikkyoCommentData = false;
	bool raceJikkyoMessageData = false;
	bool staticAndHashTextData = false;
	bool hashTextData = false;
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
extern bool openExternalPluginOnLoad;
extern int http_start_port;
extern int g_antialiasing;
extern int g_graphics_quality;
extern int g_vsync_count;
extern bool g_live_free_camera;
extern bool g_live_force_changeVisibility_false;
extern bool g_live_close_all_blur;
extern float g_live_move_step;
extern bool g_set_live_fov_as_global;
extern float g_race_move_step;
extern bool g_race_free_camera;
extern bool g_race_freecam_lookat_umamusume;
extern bool g_race_freecam_follow_umamusume;
extern float g_race_freecam_follow_umamusume_distance;
extern Vector3_t g_race_freecam_follow_umamusume_offset;
extern int g_race_freecam_follow_umamusume_index;
extern std::map<std::string, std::string> g_replaceBuiltInAssets;
extern bool g_enable_replaceBuiltInAssets;
extern bool g_enable_home_char_replace;
extern std::unordered_map<int, std::pair<int, int>> g_home_char_replace;
extern bool g_enable_global_char_replace;
extern std::unordered_map<int, std::pair<int, int>> g_global_char_replace;
extern std::unordered_map<int, std::pair<int, int>> g_global_mini_char_replace;
extern bool g_save_msgpack;
extern bool g_enable_response_convert;
extern std::wstring g_convert_url;
extern bool g_enable_self_server;
extern std::wstring g_self_server_url;
extern std::unordered_set<std::size_t> trans_off_textData;
extern CloseTrans closeTrans;
extern bool g_bypass_live_205;
extern bool g_load_finished;
