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
constexpr const char VersionDll[] = "version.dll";
constexpr const char VersionDllTmp[] = "version.dll.tmp";

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
			return "unknown";
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

	void merge_config(const std::filesystem::path& newConfig)
	{
		constexpr const char* keepList[] = { "enableConsole", "enableLogger", "dumpStaticEntries", "maxFps", "unlockSize", "uiScale", "replaceFont", "autoFullscreen" };

		std::ifstream newConfigFile(newConfig);
		if (!newConfigFile.is_open())
		{
			std::wprintf(L"Cannot open new config file, considered as corrupted, try overwriting with old version\n");
			std::filesystem::copy_file(ConfigJson, newConfig, std::filesystem::copy_options::overwrite_existing);
			return;
		}

		rapidjson::IStreamWrapper newConfigWrapper(newConfigFile);
		rapidjson::Document newConfigDocument;

		newConfigDocument.ParseStream(newConfigWrapper);
		newConfigFile.close();

		if (newConfigDocument.HasParseError())
		{
			std::wprintf(L"New config is corrupted, try overwriting with old version\n");
			std::filesystem::copy_file(ConfigJson, newConfig, std::filesystem::copy_options::overwrite_existing);
			return;
		}

		std::ifstream configFile(ConfigJson);
		if (!configFile.is_open())
		{
			std::wprintf(L"Cannot open old config, skip merging\n");
			return;
		}

		rapidjson::IStreamWrapper configWrapper(configFile);
		rapidjson::Document configDocument;

		configDocument.ParseStream(configWrapper);
		configFile.close();

		if (!configDocument.HasParseError())
		{
			for (const auto entry : keepList)
			{
				newConfigDocument[entry] = configDocument[entry];
			}
		}

		std::ofstream newConfigFileOutput(newConfig);
		rapidjson::OStreamWrapper newConfigOutputWrapper(newConfigFileOutput);
		rapidjson::Writer<rapidjson::OStreamWrapper> writer(newConfigOutputWrapper);
		newConfigDocument.Accept(writer);
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
			std::wprintf(L"Cannot open update file, updating interrupted\n");
			return false;
		}
		SimpleScope zipFileCleaner{ [&]
								   {
									   unzClose(zipFile);
								   } };

		unz_global_info info;
		if (unzGetGlobalInfo(zipFile, &info) != UNZ_OK)
		{
			std::wprintf(L"Cannot get update file zip info, updating interrupted\n");
			return false;
		}

		constexpr std::size_t BufferSize = 1024;
		char buffer[BufferSize];

		for (std::size_t i = 0; i < info.number_entry; ++i)
		{
			unz_file_info fileInfo;
			if (unzGetCurrentFileInfo(zipFile, &fileInfo, buffer, BufferSize, nullptr, 0, nullptr, 0) != UNZ_OK)
			{
				std::wprintf(L"Cannot get update file entry info, updating interrupted\n");
				return false;
			}

			const std::string_view fileNameView(buffer, fileInfo.size_filename);
			buffer[fileInfo.size_filename] = '\0';
			// 仅更新 LocalizedData config.json 及本体，不包含目录
			if ((fileNameView.length() > std::size(LocalizedDataPrefix) && fileNameView.starts_with(LocalizedDataPrefix)) || fileNameView == "config.json" || fileNameView == "version.dll")
			{
				if (unzOpenCurrentFile(zipFile) != UNZ_OK)
				{
					std::wprintf(L"Cannot open current update file entry, updating interrupted\n");
					return false;
				}
				std::ofstream output(tmpPath / (fileNameView.starts_with(LocalizedDataPrefix) ? fileNameView.substr(std::size(LocalizedDataPrefix) - 1) : fileNameView), std::ios::binary);
				int readSizeOrError;
				// 循环开始时不能继续使用 fileNameView，已被复用于文件内容缓存
				do
				{
					readSizeOrError = unzReadCurrentFile(zipFile, buffer, BufferSize);
					if (readSizeOrError < 0)
					{
						std::wprintf(L"Cannot read current update file entry, updating interrupted\n");
						return false;
					}
					output.write(buffer, readSizeOrError);
				} while (readSizeOrError != 0);
			}

			unzCloseCurrentFile(zipFile);

			if (i + 1 != info.number_entry && unzGoToNextFile(zipFile) != UNZ_OK)
			{
				std::wprintf(L"Cannot iterate update file entry, updating interrupted\n");
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

	bool is_file_content_equal(const std::filesystem::path& a, const std::filesystem::path& b)
	{
		std::ifstream fileA(a, std::ios_base::ate);
		std::ifstream fileB(b, std::ios_base::ate);

		if (!fileA.is_open() || !fileB.is_open())
		{
			return false;
		}

		auto sizeA = static_cast<std::streamsize>(fileA.tellg());
		if (sizeA != fileB.tellg())
		{
			return false;
		}

		fileA.seekg(0);
		fileB.seekg(0);

		constexpr std::streamsize BufferSize = 1024;
		char bufferA[BufferSize];
		char bufferB[BufferSize];

		while (sizeA)
		{
			const auto readSize = std::min(BufferSize, sizeA);
			fileA.read(bufferA, readSize);
			fileB.read(bufferB, readSize);

			if (std::memcmp(bufferA, bufferB, readSize) != 0)
			{
				return false;
			}

			sizeA -= readSize;
		}

		return true;
	}

	void auto_update()
	{
		constexpr const char AutoUpdateTmpPath[] = "UpdateTemp";
		const std::filesystem::path oldLocalizedDataPath = OldLocalizedDataPath;

		if (g_auto_update_service)
		{
			const auto currentVersion = utility::conversions::to_string_t(get_current_version());
			std::wprintf(L"Current version is %s\n", currentVersion.c_str());
			constexpr auto updateTempFile = "update.zip";

			try
			{
				const auto latestRelease = g_auto_update_service->GetLatestRelease().get();
				if (latestRelease)
				{
					if (currentVersion != latestRelease->Version)
					{
						const auto userResponse = MessageBoxW(NULL, std::format(L"当前版本是 {}，检测到新版本 {}，是否更新？\n需注意在更新完成之前加载的文本可能不会被更新\n更新信息：\n{}",
							currentVersion,
							latestRelease->Version,
							latestRelease->Comment).c_str(), L"翻译插件自动更新", MB_YESNO);
						if (userResponse != IDYES)
						{
							return;
						}
					}
					else
					{
						return;
					}

					std::wprintf(L"New version %s downloading...\n", latestRelease->Version.c_str());

					AutoUpdate::DownloadFile(latestRelease->Uri, updateTempFile).get();

					std::wprintf(L"New version %s downloaded! Updating...\n", latestRelease->Version.c_str());

					const std::filesystem::path tmpPath = AutoUpdateTmpPath;

					try
					{
						// 不关闭会占用部分 json 文件导致失败
						killProcessByName("UnityCrashHandler64.exe");

						// 清空并重新生成临时路径
						std::filesystem::remove_all(tmpPath);
						std::filesystem::create_directory(tmpPath);
						if (decompress_update_file(updateTempFile, tmpPath))
						{
							const auto newVersionDllPath = tmpPath / VersionDll;
							const auto doesNewVersionDllExist = std::filesystem::exists(newVersionDllPath);
							const auto shouldUpdateVersionDll = doesNewVersionDllExist && !is_file_content_equal(VersionDll, newVersionDllPath);

							if (shouldUpdateVersionDll)
							{
								const auto userResponse = MessageBoxW(NULL, L"新版本包含对插件本体的更新，此更新需要重启游戏以应用，若不方便此时重启可以放弃更新，是否重启以更新？", L"翻译插件自动更新", MB_YESNO);
								if (userResponse != IDYES)
								{
									std::filesystem::remove_all(tmpPath);
									return;
								}
							}

							const auto newConfigPath = tmpPath / ConfigJson;
							merge_config(newConfigPath);

							// 备份旧文件
							std::filesystem::rename(LocalizedDataPath, oldLocalizedDataPath);
							std::filesystem::rename(ConfigJson, oldLocalizedDataPath / ConfigJson);
							std::filesystem::copy_file(VersionDll, oldLocalizedDataPath / VersionDll, std::filesystem::copy_options::overwrite_existing);

							// 更新文件
							if (shouldUpdateVersionDll)
							{
								std::filesystem::copy_file(newVersionDllPath, VersionDllTmp);
							}
							std::filesystem::rename(newConfigPath, ConfigJson);
							std::filesystem::rename(tmpPath, LocalizedDataPath);

							// 删除备份
							std::filesystem::remove_all(oldLocalizedDataPath);

							reload_config();

							write_current_version(utility::conversions::to_utf8string(latestRelease->Version));
							std::filesystem::remove(updateTempFile);

							std::wprintf(L"New version updating completed!\n");

							if (shouldUpdateVersionDll)
							{
								constexpr char selfUpdateBatchContent[] = R"(
@echo off
setlocal

taskkill /im "umamusume.exe" >NUL

:waitloop

tasklist | find /i "umamusume.exe" >NUL
if %ERRORLEVEL% == 0 timeout /t 1 /nobreak & goto waitloop

move /y version.dll.tmp version.dll
start umamusume.exe
del SelfUpdate.bat
)";
								std::ofstream selfUpdateBatchFile("SelfUpdate.bat");
								if (selfUpdateBatchFile.is_open())
								{
									// 不写出结尾 0
									selfUpdateBatchFile.write(selfUpdateBatchContent, std::size(selfUpdateBatchContent) - 1);
									selfUpdateBatchFile.close();

									wchar_t commandLine[] = L"cmd.exe /c SelfUpdate.bat";
									STARTUPINFOW startupInfo{ .cb = sizeof(STARTUPINFOW) };
									PROCESS_INFORMATION processInfo{};
									if (CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
									{
										CloseHandle(processInfo.hThread);
										CloseHandle(processInfo.hProcess);

										std::wprintf(L"Exiting process for self-updating...\n");
										TerminateProcess(GetCurrentProcess(), 0);
										// 应不可达
									}
									else
									{
										MessageBoxW(NULL, L"无法执行自更新脚本，请手动用游戏根目录下的 version.dll.tmp 覆盖 version.dll", L"翻译插件自动更新", MB_OK);
									}
								}
								else
								{
									MessageBoxW(NULL, L"无法写出自更新脚本，请手动用游戏根目录下的 version.dll.tmp 覆盖 version.dll", L"翻译插件自动更新", MB_OK);
								}
							}
						}
						else
						{
							std::filesystem::remove_all(tmpPath);
							std::wprintf(L"Cannot decompress update file!\n");
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
					std::wprintf(L"You are using the latest version!\n");
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
