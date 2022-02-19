#include <stdinclude.hpp>

#include <minizip/unzip.h>
#include <TlHelp32.h>

extern bool init_hook();
extern void uninit_hook();
extern void start_console();

bool g_dump_entries = false;
bool g_enable_logger = false;
bool g_enable_console = false;
int g_max_fps = -1;
bool g_unlock_size = false;
float g_ui_scale = 1.0f;
float g_aspect_ratio = 16.f / 9.f;
bool g_replace_font = true;
bool g_auto_fullscreen = true;
std::unique_ptr<AutoUpdate::IAutoUpdateService> g_auto_update_service{};

constexpr const char LocalizedDataPath[] = "localized_data";
constexpr const char OldLocalizedDataPath[] = "old_localized_data";
constexpr const char ConfigJson[] = "config.json";

namespace
{
	void create_debug_console()
	{
		AllocConsole();

		// open stdout stream
		auto _ = freopen("CONOUT$", "w+t", stdout);
		_ = freopen("CONOUT$", "w", stderr);
		_ = freopen("CONIN$", "r", stdin);

		SetConsoleTitle("Umamusume - Debug Console");

		// set this to avoid turn japanese texts into question mark
		SetConsoleOutputCP(65001);
		std::locale::global(std::locale(""));

		wprintf(L"\u30a6\u30de\u5a18 Localify Patch Loaded! - By GEEKiDoS\n");
	}

	std::string get_current_version()
	{
		std::ifstream versionStream("version.txt", std::ios_base::ate);
		if (!versionStream.is_open())
		{
			return "";
		}
		const auto length = versionStream.tellg();
		versionStream.seekg(0);
		std::string version(length, 0);
		versionStream.read(version.data(), length);
		return version;
	}

	void write_current_version(const std::string_view& version)
	{
		std::ofstream versionStream("version.txt");
		versionStream.write(version.data(), version.size());
	}

	std::vector<std::string> read_config()
	{
		std::ifstream config_stream { ConfigJson };
		std::vector<std::string> dicts {};

		if (!config_stream.is_open())
			return dicts;

		rapidjson::IStreamWrapper wrapper {config_stream};
		rapidjson::Document document;

		document.ParseStream(wrapper);

		if (!document.HasParseError())
		{
			g_enable_console = document["enableConsole"].GetBool();
			g_enable_logger = document["enableLogger"].GetBool();
			g_dump_entries = document["dumpStaticEntries"].GetBool();
			g_max_fps = document["maxFps"].GetInt();
			g_unlock_size = document["unlockSize"].GetBool();
			g_ui_scale = document["uiScale"].GetFloat();
			g_replace_font = document["replaceFont"].GetBool();
			g_auto_fullscreen = document["autoFullscreen"].GetBool();

			// Looks like not working for now
			// g_aspect_ratio = document["customAspectRatio"].GetFloat();

			auto& dicts_arr = document["dicts"];
			auto len = dicts_arr.Size();

			for (size_t i = 0; i < len; ++i)
			{
				auto dict = dicts_arr[i].GetString();

				dicts.push_back(dict);
			}

			if (document.HasMember("autoUpdate"))
			{
				const auto& autoUpdate = document["autoUpdate"];
				const auto& source = autoUpdate["source"];
				const auto& path = autoUpdate["path"];

				g_auto_update_service = AutoUpdate::CreateAutoUpdateService(source.GetString(), path.GetString());
			}
		}

		config_stream.close();
		return dicts;
	}

	void reload_config()
	{
		std::ifstream config_stream{ "config.json" };
		std::vector<std::string> dicts{};

		rapidjson::IStreamWrapper wrapper{ config_stream };
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
		local::reload_textdb(&dicts);
	}

	template <typename Callable>
	struct SimpleScope
	{
		Callable Cleaner;

		~SimpleScope()
		{
			Cleaner();
		}
	};

	bool decompress_update_file(const char* updateFile, const std::filesystem::path& tmpPath)
	{
		constexpr const char LocalizedDataPrefix[] = "localized_data/";

		const auto zipFile = unzOpen(updateFile);
		if (!zipFile)
		{
			std::printf("Cannot open update file, updating interrupted\n");
			return false;
		}
		SimpleScope zipFileCleaner{ [&]
								   {
									   unzClose(zipFile);
								   } };

		unz_global_info info;
		if (unzGetGlobalInfo(zipFile, &info) != UNZ_OK)
		{
			std::printf("Cannot get update file zip info, updating interrupted\n");
			return false;
		}

		constexpr std::size_t BufferSize = 1024;
		char buffer[BufferSize];

		for (std::size_t i = 0; i < info.number_entry; ++i)
		{
			unz_file_info fileInfo;
			if (unzGetCurrentFileInfo(zipFile, &fileInfo, buffer, BufferSize, nullptr, 0, nullptr, 0) != UNZ_OK)
			{
				std::printf("Cannot get update file entry info, updating interrupted\n");
				return false;
			}

			const std::string_view fileNameView(buffer, fileInfo.size_filename);
			buffer[fileInfo.size_filename] = '\0';
			// 仅更新 LocalizedData 和 config.json，不包含目录
			if ((fileNameView.length() > std::size(LocalizedDataPrefix) && fileNameView.starts_with(LocalizedDataPrefix)) || fileNameView == "config.json")
			{
				if (unzOpenCurrentFile(zipFile) != UNZ_OK)
				{
					std::printf("Cannot open current update file entry, updating interrupted\n");
					return false;
				}
				std::ofstream output(tmpPath / (fileNameView == "config.json" ? fileNameView : fileNameView.substr(std::size(LocalizedDataPrefix) - 1)), std::ios::binary);
				int readSizeOrError;
				// 循环开始时不能继续使用 fileNameView，已被复用于文件内容缓存
				do
				{
					readSizeOrError = unzReadCurrentFile(zipFile, buffer, BufferSize);
					if (readSizeOrError < 0)
					{
						std::printf("Cannot read current update file entry, updating interrupted\n");
						return false;
					}
					output.write(buffer, readSizeOrError);
				} while (readSizeOrError != 0);
			}

			unzCloseCurrentFile(zipFile);

			if (i + 1 != info.number_entry && unzGoToNextFile(zipFile) != UNZ_OK)
			{
				std::printf("Cannot iterate update file entry, updating interrupted\n");
				return false;
			}
		}

		return true;
	}

	// https://stackoverflow.com/questions/7956519/how-to-kill-processes-by-name-win32-api
	void killProcessByName(const char* filename)
	{
		const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
		SimpleScope snapShotCleaner{ [&] {
			CloseHandle(snapShot);
		} };
		PROCESSENTRY32 processEntry{ .dwSize = sizeof(PROCESSENTRY32) };
		auto hRes = Process32First(snapShot, &processEntry);
		while (hRes)
		{
			if (strcmp(processEntry.szExeFile, filename) == 0)
			{
				const auto hProcess = OpenProcess(PROCESS_TERMINATE, 0, processEntry.th32ProcessID);
				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
				}
			}
			hRes = Process32Next(snapShot, &processEntry);
		}
	}

	void auto_update()
	{
		constexpr const char AutoUpdateTmpPath[] = "UpdateTemp";
		const std::filesystem::path oldLocalizedDataPath = OldLocalizedDataPath;

		// 不关闭会占用部分 json 文件导致失败
		killProcessByName("UnityCrashHandler64.exe");

		if (g_auto_update_service)
		{
			const auto currentVersion = get_current_version();
			printf("Current version is %s\n", currentVersion.c_str());
			constexpr auto updateTempFile = "update.zip";

			try
			{
				const auto newVersion = g_auto_update_service->CheckAndUpdate(currentVersion, updateTempFile).get();
				if (newVersion)
				{
					std::printf("New version %s downloaded! Updating...\n", newVersion->c_str());
					const std::filesystem::path tmpPath = AutoUpdateTmpPath;

					try
					{
						// 清空并重新生成临时路径
						std::filesystem::remove_all(tmpPath);
						std::filesystem::create_directory(tmpPath);
						if (decompress_update_file(updateTempFile, tmpPath))
						{
							std::filesystem::rename(LocalizedDataPath, oldLocalizedDataPath);
							std::filesystem::rename(ConfigJson, oldLocalizedDataPath / ConfigJson);
							std::filesystem::rename(tmpPath / ConfigJson, ConfigJson);
							std::filesystem::rename(tmpPath, LocalizedDataPath);
							std::filesystem::remove_all(oldLocalizedDataPath);

							reload_config();

							write_current_version(*newVersion);
							std::filesystem::remove(updateTempFile);

							std::printf("New version updating completed!\n");
						}
						else
						{
							std::filesystem::remove_all(tmpPath);
							std::printf("Cannot decompress update file!\n");
						}
					}
					catch (const std::exception& e)
					{
						std::printf("Exception %s occurred during updating, try rolling back...\n", e.what());

						try
						{
							std::filesystem::remove_all(tmpPath);
							if (std::filesystem::exists(oldLocalizedDataPath))
							{
								std::filesystem::remove_all(LocalizedDataPath);
								std::filesystem::rename(oldLocalizedDataPath / ConfigJson, ConfigJson);
								std::filesystem::rename(oldLocalizedDataPath, LocalizedDataPath);
							}
						}
						catch (const std::exception& e)
						{
							const auto msg = std::format("Another exception {} occurred during rolling back, the program should be unable to continue, please try reopening program or reinstalling patch", e.what());
							std::printf("%s\n", msg.c_str());
							MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);
							std::exit(1);
						}
					}
				}
				else
				{
					std::printf("You are using the latest version!\n");
				}
			}
			catch (const std::exception& e)
			{
				std::printf("Exception %s occurred during checking update, skipping\n", e.what());
			}
		}
	}
}

int __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// the DMM Launcher set start path to system32 wtf????
		std::string module_name;
		module_name.resize(MAX_PATH);
		module_name.resize(GetModuleFileName(nullptr, module_name.data(), MAX_PATH));

		std::filesystem::path module_path(module_name);

		// check name
		if (module_path.filename() != "umamusume.exe")
			return 1;

		std::filesystem::current_path(
			module_path.parent_path()
		);

		const std::filesystem::path oldLocalizedDataPath = OldLocalizedDataPath;
		if (std::filesystem::exists(oldLocalizedDataPath))
		{
			// 上一次升级失败？
			killProcessByName("UnityCrashHandler64.exe");
			std::filesystem::remove_all(LocalizedDataPath);
			std::filesystem::rename(oldLocalizedDataPath / ConfigJson, ConfigJson);
			std::filesystem::rename(oldLocalizedDataPath, LocalizedDataPath);
		}

		auto dicts = read_config();

		if(g_enable_console)
		 	create_debug_console();

		std::thread init_thread([dicts]() {
			logger::init_logger();
			local::load_textdb(&dicts);
			init_hook();

			if (g_enable_console)
				start_console();

			auto_update();
		});
		init_thread.detach();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		uninit_hook();
		logger::close_logger();
	}

	return 1;
}
