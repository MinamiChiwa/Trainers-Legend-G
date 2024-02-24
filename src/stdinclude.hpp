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
#include "pluginLoader/pluginLoader.hpp"
#include <nlohmann/json.hpp>
#include "umagui/guiShowData.hpp"
#include "umagui/umaguiMain.hpp"

extern bool g_dump_entries;
extern bool g_enable_logger;
extern bool g_enable_console;
extern int g_max_fps;
extern bool g_unlock_size;
extern float g_ui_scale;
extern float g_aspect_ratio;
extern bool g_read_request_pack;

extern float orig_aspect_ratio;

// extern std::string g_extra_assetbundle_path;
extern std::list<std::string> g_extra_assetbundle_paths;

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
	CAMERA_RACE = 1,
	CAMERA_CUTIN = 2
};

enum LiveCameraType {
	LiveCamera_FREE = 0,
	LiveCamera_FOLLOW_UMA = 1,
	LiveCamera_FIRST_PERSON = 2
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

template<typename T, typename... Args>
class SchedulingFuncs {
public:
	struct FunctionInfo {
		T(*function)(Args...);
		std::tuple<Args...> args;
	};

	static std::vector<FunctionInfo> schedulingFuncList;

	static void addToDispatcher(T(*t)(Args...), Args... args) {
		schedulingFuncList.push_back({ t, std::make_tuple(args...) });
	}

	static void callAllFunctions() {
		bool isCall = false;
		for (const auto& funcInfo : schedulingFuncList) {
			std::apply(funcInfo.function, funcInfo.args);
			isCall = true;
		}
		if (isCall) schedulingFuncList.clear();
	}
};

template<typename T, typename... Args>
std::vector<typename SchedulingFuncs<T, Args...>::FunctionInfo> SchedulingFuncs<T, Args...>::schedulingFuncList{};

extern std::variant<UseOriginalFont, UseDefaultFont, UseCustomFont> g_replace_font;
extern int g_custom_font_size_offset;
extern int g_custom_font_style;
extern float g_custom_font_linespacing;
extern bool g_replace_assets;
extern bool g_asset_load_log;

extern bool g_auto_fullscreen;
extern bool g_fullscreen_block_minimization;
extern std::unique_ptr<AutoUpdate::IAutoUpdateService> g_auto_update_service;
extern std::string g_static_dict_path;
extern bool autoChangeLineBreakMode;
extern int start_width;
extern int start_height;
extern bool openExternalPluginOnLoad;
extern int http_start_port;
extern int g_antialiasing;
extern int g_graphics_quality;
extern float g_virtual_resolution_multiple;
extern int g_vsync_count;
extern bool g_live_free_camera;
extern bool g_live_force_changeVisibility_false;
extern bool g_enable_live_follow_uma_smooth;
extern float g_live_follow_uma_smooth_lookat_step;
extern float g_live_follow_uma_smooth_pos_step;
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
extern bool g_global_char_replace_Universal;
extern bool g_save_msgpack;
extern bool g_enable_response_convert;
extern std::wstring g_convert_url;
extern bool g_enable_self_server;
extern std::wstring g_self_server_url;
extern std::unordered_set<std::size_t> trans_off_textData;
extern CloseTrans closeTrans;
extern bool g_bypass_live_205;
extern bool g_load_finished;
extern bool g_home_free_camera;
extern int g_home_walk_chara_id;
extern float g_free_camera_mouse_speed;
extern std::list<std::function<void(void)>> onPluginReload;
extern bool enableRaceInfoTab;
extern bool raceInfoTabAttachToGame;
extern bool liveFirstPersonEnableRoll;
extern bool raceFollowUmaFirstPerson;
extern bool raceFollowUmaFirstPersonEnableRoll;
extern std::string g_autoupdateUrl;
extern std::function<void(Il2CppString* title, Il2CppString* content, int buttonCount, int button1Text, int button2Text, int button3Text, int btn_type)> showDialog;
extern bool g_force_landscape;
extern std::string dumpGameAssemblyPath;
extern bool g_enable_live_dof_controller;
extern bool guiStarting;
extern bool g_cutin_first_person;
extern bool g_enable_cutin_first_person;
extern bool g_enable_better60fps;
extern bool g_enable_custom_PersistentDataPath;
extern std::string g_custom_PersistentDataPath;
extern bool g_upload_gacha_history;
extern std::wstring g_upload_gacha_history_endpoint;
extern bool g_dump_sprite_tex;
