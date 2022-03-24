#include <stdinclude.hpp>

#include <minizip/unzip.h>
#include <TlHelp32.h>

#include <unordered_set>
#include <charconv>
#include <cassert>

extern bool init_hook();
extern void uninit_hook();
extern void start_console();

std::optional<std::wstring> localize_get(int id);
std::optional<std::wstring> get_game_version_name();

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
std::string g_static_dict_path;

constexpr const char LocalizedDataPath[] = "localized_data";
constexpr const char OldLocalizedDataPath[] = "old_localized_data";
constexpr const char ConfigJson[] = "config.json";
constexpr const char VersionDll[] = "version.dll";
constexpr const char VersionDllTmp[] = "version.dll.tmp";
constexpr const char StaticDictCache[] = "static_cache.json";
constexpr const char StaticDictCachePath[] = "localized_data/static_cache.json";
constexpr const char StaticDictStamp[] = "static_cache.stamp";
constexpr const char StaticDictStampPath[] = "localized_data/static_cache.stamp";

char line_break_hotkey = 'u';
bool autoChangeLineBreakMode = false;
int start_width = -1;
int start_height = -1;

namespace
{
	void create_debug_console()
	{
		AllocConsole();

		// open stdout stream
		auto _ = freopen("CONOUT$", "w+t", stdout);
		_ = freopen("CONOUT$", "w", stderr);
		_ = freopen("CONIN$", "r", stdin);

		SetConsoleTitleW(L"Umamusume - Debug Console - 此插件为免费下载, 若您是付费购买此插件请立刻举报店家! 交流群: 697216935");

		// set this to avoid turn japanese texts into question mark
		SetConsoleOutputCP(65001);
		std::locale::global(std::locale(""));

		wprintf(L"\u30a6\u30de\u5a18 Localify Patch Loaded! - By GEEKiDoS\n");
	}

	std::string get_current_version()
	{
		std::ifstream versionStream("version.txt", std::ios_base::ate | std::ios_base::binary);
		if (!versionStream.is_open())
		{
			return "unknown";
		}
		const auto length = versionStream.tellg();
		versionStream.seekg(0);
		std::string version(length, 0);
		versionStream.read(version.data(), length);
		std::erase_if(version, [locale = std::locale("")](char ch) { return std::isblank(ch, locale); });
		return version;
	}

	void write_current_version(const std::string_view& version)
	{
		std::ofstream versionStream("version.txt", std::ios_base::binary);
		versionStream.write(version.data(), version.size());
	}

	enum class StaticCacheStatus
	{
		UpToDate,
		Outdated,
		Indeterminate,
	};

	std::string get_static_cache_stamp()
	{
		std::ifstream staticCacheStamp(StaticDictStampPath, std::ios_base::ate | std::ios_base::binary);
		if (!staticCacheStamp.is_open())
		{
			return "";
		}
		const auto length = staticCacheStamp.tellg();
		staticCacheStamp.seekg(0);
		std::string stamp(length, 0);
		staticCacheStamp.read(stamp.data(), length);
		return stamp;
	}

	void write_static_cache_stamp(const std::string_view& stamp)
	{
		std::ofstream staticCacheStamp(StaticDictStampPath);
		staticCacheStamp.write(stamp.data(), stamp.size());
	}

	StaticCacheStatus get_static_cache_status()
	{
		const auto currentGameVersion = get_game_version_name();
		if (!currentGameVersion)
		{
			// 无法获取游戏版本，可能获取版本的方式已变化
			return StaticCacheStatus::Indeterminate;
		}

		return get_static_cache_stamp() == utility::conversions::to_utf8string(*currentGameVersion) ? StaticCacheStatus::UpToDate : StaticCacheStatus::Outdated;
	}

	std::map<std::size_t, std::string> build_static_cache(const std::string& staticDictPath)
	{
		std::printf("Building static cache from %s\n", staticDictPath.c_str());

		std::ifstream staticDictFile(staticDictPath);
		std::map<std::size_t, std::string> dict;

		if (!staticDictFile.is_open())
		{
			return dict;
		}

		rapidjson::IStreamWrapper wrapper(staticDictFile);
		rapidjson::Document doc;

		doc.ParseStream(wrapper);

		if (doc.HasParseError())
		{
			return dict;
		}

		bool lastIsEmpty = false;
		for (int id = 1;; ++id)
		{
			const auto originText = localize_get(id);
			if (!originText)
			{
				if (lastIsEmpty)
				{
					break;
				}

				lastIsEmpty = true;
				continue;
			}

			lastIsEmpty = false;

			const auto utf8OriginText = utility::conversions::to_utf8string(*originText);
			if (const auto iter = doc.FindMember(utf8OriginText.c_str()); iter != doc.MemberEnd() && iter->value.IsString())
			{
				dict.emplace(id, iter->value.GetString());
			}
		}

		return dict;
	}

	std::optional<std::map<std::size_t, std::string>> read_static_cache()
	{
		std::map<std::size_t, std::string> staticCache;
		std::ifstream staticCacheFile(StaticDictCachePath);

		if (!staticCacheFile.is_open())
		{
			return {};
		}

		rapidjson::IStreamWrapper wrapper(staticCacheFile);
		rapidjson::Document staticCacheDocument;

		staticCacheDocument.ParseStream(wrapper);
		staticCacheFile.close();
		if (staticCacheDocument.HasParseError())
		{
			return {};
		}

		for (auto iter = staticCacheDocument.MemberBegin(), end = staticCacheDocument.MemberEnd(); iter != end; ++iter)
		{
			const auto name = iter->name.GetString();
			const auto nameEnd = name + std::strlen(name);
			const auto value = iter->value.GetString();

			int id;
			if (const auto [end, ec] = std::from_chars(name, nameEnd, id); ec != std::errc{})
			{
				return {};
			}

			staticCache.emplace(id, value);
		}

		return staticCache;
	}

	bool write_static_cache(const std::map<std::size_t, std::string>& staticCache)
	{
		std::ofstream staticCacheFile(StaticDictCachePath);
		if (!staticCacheFile.is_open())
		{
			return false;
		}

		rapidjson::OStreamWrapper wrapper(staticCacheFile);
		rapidjson::Writer writer(wrapper);

		writer.StartObject();

		// 足够容纳 std::uint64_t 的最大值
		char idBuffer[21];
		for (const auto& [id, content] : staticCache)
		{
			const auto [end, ec] = std::to_chars(std::begin(idBuffer), std::end(idBuffer), id);
			// 不应当出错
			assert(ec == std::errc{});
			*end = 0;

			writer.Key(idBuffer);
			writer.String(content.c_str());
		}

		writer.EndObject();

		return true;
	}

	void dump_static_dict(const std::filesystem::path& outputPath, const std::map<std::size_t, std::string>& currentStaticCache)
	{
		std::ofstream output(outputPath);
		if (!output.is_open())
		{
			return;
		}

		rapidjson::OStreamWrapper wrapper(output);
		rapidjson::Writer writer(wrapper);

		std::unordered_set<std::wstring> set;

		writer.StartObject();

		bool lastIsEmpty = false;
		for (std::size_t id = 1;; ++id)
		{
			const auto originText = localize_get(id);
			if (!originText)
			{
				if (lastIsEmpty)
				{
					break;
				}

				lastIsEmpty = true;
				continue;
			}

			lastIsEmpty = false;

			if (const auto [iter, succeed] = set.emplace(*originText); !succeed)
			{
				// 重复文本，跳过
				continue;
			}

			const auto origin = utility::conversions::to_utf8string(*originText);
			writer.Key(origin.c_str());
			if (const auto iter = currentStaticCache.find(id); iter != currentStaticCache.end())
			{
				writer.String(iter->second.c_str());
			}
			else
			{
				writer.Null();
			}
		}

		writer.EndObject();
	}
}

std::map<std::size_t, std::string> ensure_latest_static_cache(const std::string& staticDictPath)
{
	if (get_static_cache_status() == StaticCacheStatus::UpToDate)
	{
		auto staticCache = read_static_cache();
		if (staticCache)
		{
			std::wprintf(L"Static dict cache is up-to-date, load from cache\n");
			return std::move(*staticCache);
		}
	}

	std::wprintf(L"Static dict cache is outdated or corrupted, building new cache\n");
	auto newStaticCache = build_static_cache(staticDictPath);
	write_static_cache(newStaticCache);
	if (const auto gameVersion = get_game_version_name())
	{
		write_static_cache_stamp(utility::conversions::to_utf8string(*gameVersion));
	}
	return newStaticCache;
}

namespace
{
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
			line_break_hotkey = document["LineBreakHotKey"].GetString()[0];
			autoChangeLineBreakMode = document["autoChangeLineBreakMode"].GetBool();

			if (document.HasMember("resolution_start")) {
				if (document["resolution_start"].IsArray()) {
					auto st = document["resolution_start"].GetArray();
					if (st.Size() == 2) {
						start_width = st[0].GetInt();
						start_height = st[1].GetInt();
					}
				}
			}

			if (document.HasMember("aspect_ratio")) {
				if (document["aspect_ratio"].IsArray()) {
					auto asp = document["aspect_ratio"].GetArray();
					if (asp.Size() == 2) {
						g_aspect_ratio = asp[0].GetFloat() / asp[1].GetFloat();
					}
				}
			}

			MHotkey::start_hotkey(line_break_hotkey);
			std::wprintf(L"换行符切换热键已设置为: ctrl + %c\n", static_cast<wchar_t>(line_break_hotkey));

			// Looks like not working for now
			// g_aspect_ratio = document["customAspectRatio"].GetFloat();

			auto& dicts_arr = document["dicts"];
			auto len = dicts_arr.Size();

			for (size_t i = 0; i < len; ++i)
			{
				auto dict = dicts_arr[i].GetString();

				dicts.push_back(dict);
			}

			g_static_dict_path = document["static_dict"].GetString();

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

		rapidjson::IStreamWrapper wrapper{ config_stream };
		rapidjson::Document document;

		document.ParseStream(wrapper);

		if (!document.HasParseError())
		{
			std::vector<std::string> dicts{};

			const std::string staticDictPath = document["static_dict"].GetString();
			g_static_dict_path = staticDictPath;
			auto staticDictCache = ensure_latest_static_cache(staticDictPath);

			auto& dicts_arr = document["dicts"];
			auto len = dicts_arr.Size();

			for (size_t i = 0; i < len; ++i)
			{
				auto dict = dicts_arr[i].GetString();

				dicts.push_back(dict);
			}

			local::reload_textdb(&dicts, std::move(staticDictCache));
		}
	}

	// 返回新的 static dict 路径
	std::string merge_config(const std::filesystem::path& newConfig)
	{
		constexpr const char* keepList[] =
		{
			"enableConsole",
			"enableLogger",
			"dumpStaticEntries",
			"maxFps",
			"unlockSize",
			"uiScale",
			"replaceFont",
			"autoFullscreen",
			"LineBreakHotKey",
			"autoChangeLineBreakMode",
		};

		std::ifstream newConfigFile(newConfig);
		if (!newConfigFile.is_open())
		{
			std::wprintf(L"Cannot open new config file, considered as corrupted, try overwriting with old version\n");
			std::filesystem::copy_file(ConfigJson, newConfig, std::filesystem::copy_options::overwrite_existing);
			return g_static_dict_path;
		}

		rapidjson::IStreamWrapper newConfigWrapper(newConfigFile);
		rapidjson::Document newConfigDocument;

		newConfigDocument.ParseStream(newConfigWrapper);
		newConfigFile.close();

		if (newConfigDocument.HasParseError())
		{
			std::wprintf(L"New config is corrupted, try overwriting with old version\n");
			std::filesystem::copy_file(ConfigJson, newConfig, std::filesystem::copy_options::overwrite_existing);
			return g_static_dict_path;
		}

		std::ifstream configFile(ConfigJson);
		if (!configFile.is_open())
		{
			std::wprintf(L"Cannot open old config, skip merging\n");
			return g_static_dict_path;
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

		return newConfigDocument["static_dict"].GetString();
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
		std::ifstream fileA(a, std::ios_base::ate | std::ios_base::binary);
		std::ifstream fileB(b, std::ios_base::ate | std::ios_base::binary);

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
			std::wprintf(L"Current version is %ls\n", currentVersion.c_str());
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

					std::wprintf(L"New version %ls downloading...\n", latestRelease->Version.c_str());

				RetryDownload:
					AutoUpdate::DownloadFile(latestRelease->Uri, updateTempFile).get();

					std::wprintf(L"New version %ls downloaded! Updating...\n", latestRelease->Version.c_str());

					const std::filesystem::path tmpPath = AutoUpdateTmpPath;

					// 已完成下载解压
					bool prepareUpdateFilesCompleted = false;

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
								const auto userResponse = MessageBoxW(NULL, L"新版本包含对插件本体的更新，此更新需要关闭游戏以应用，若不方便此时重启可以放弃更新，是否重启以更新？\n您需要手动从 DMM 管理程序启动", L"翻译插件自动更新", MB_YESNO);
								if (userResponse != IDYES)
								{
									std::filesystem::remove_all(tmpPath);
									return;
								}
							}

							const auto newConfigPath = tmpPath / ConfigJson;
							std::filesystem::path newStaticDictPath = merge_config(newConfigPath);
							constexpr const wchar_t LocalizedDataPrefix[] = L"localized_data/";
							if (newStaticDictPath.native().starts_with(LocalizedDataPrefix))
							{
								newStaticDictPath = tmpPath / std::basic_string_view<std::filesystem::path::value_type>(newStaticDictPath.native()).substr(std::size(LocalizedDataPrefix) - 1);
							}

							const auto forceInvalidateStaticCache = std::filesystem::exists(newStaticDictPath) && !is_file_content_equal(g_static_dict_path, newStaticDictPath);
							// 若不需要更新，则不复制已有的 cache，将会自动被删除
							if (!forceInvalidateStaticCache && get_static_cache_status() == StaticCacheStatus::UpToDate)
							{
								std::filesystem::copy_file(StaticDictCachePath, tmpPath / StaticDictCache);
								std::filesystem::copy_file(StaticDictStampPath, tmpPath / StaticDictStamp);
							}

							prepareUpdateFilesCompleted = true;

							// 备份旧文件，此处理之后旧的 localized_data 应使用 old_localized_data 引用
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
del SelfUpdate.bat)";
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

										std::wprintf(L"Waiting for terminating...\n");
										WaitForSingleObject(processInfo.hProcess, INFINITE);
										// 应不可达
										CloseHandle(processInfo.hProcess);
										std::exit(0);
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
						std::printf("Exception %s occurred during updating, try force update...\n", e.what());

						if (!prepareUpdateFilesCompleted)
						{
							const auto userResponse = MessageBoxW(NULL, L"无法解压更新文件，更新文件可能已损坏，要重新尝试下载吗？\n也可以尝试从官网直接下载最新安装包覆盖", L"翻译插件自动更新", MB_YESNO);
							if (userResponse == IDYES)
							{
								goto RetryDownload;
							}
							return;
						}
						else
						{
							const auto userResponse = MessageBoxW(NULL, L"无法更新文件，可能文件当前被占用，要强制关闭程序覆盖更新吗？也可取消自动更新", L"翻译插件自动更新", MB_YESNOCANCEL);
							if (userResponse == IDCANCEL)
							{
								return;
							}

							if (userResponse == IDYES)
							{
								constexpr char forceSelfUpdateBatchContent[] = R"(
@echo off
setlocal

taskkill /im "umamusume.exe" >NUL

:waitloop

tasklist | find /i "umamusume.exe" >NUL
if %ERRORLEVEL% == 0 timeout /t 1 /nobreak & goto waitloop

cd .

if not exist old_localized_data (
	move /y localized_data old_localized_data
	if not %ERRORLEVEL% == 0 (
		start cmd /c "echo 文件被占用，无法替换，请手动解压覆盖游戏根目录的 update.zip 文件 && pause && start %cd%"
		exit
	)
	move /y config.json old_localized_data\config.json
	copy /y version.dll old_localized_data\version.dll
)

if exist UpdateTemp (
	move /y UpdateTemp\version.dll version.dll.tmp
	move /y UpdateTemp\config.json config.json
	move /y UpdateTemp localized_data
)
move /y version.dll.tmp version.dll
if not %ERRORLEVEL% == 0 (
	start cmd /c "echo version.dll 文件被占用，无法替换，请手动覆盖 version.dll.tmp 到 version.dll && pause && start %cd%"
	exit
)
rd /s /q old_localized_data
del update.zip
echo %1> version.txt
del SelfUpdate.bat)";

								std::ofstream selfUpdateBatchFile("SelfUpdate.bat");
								if (selfUpdateBatchFile.is_open())
								{
									// 不写出结尾 0
									selfUpdateBatchFile.write(forceSelfUpdateBatchContent, std::size(forceSelfUpdateBatchContent) - 1);
									selfUpdateBatchFile.close();

									std::wstring commandLine = std::format(L"cmd.exe /c SelfUpdate.bat {}", latestRelease->Version);
									STARTUPINFOW startupInfo{ .cb = sizeof(STARTUPINFOW) };
									PROCESS_INFORMATION processInfo{};
									if (CreateProcessW(NULL, commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
									{
										CloseHandle(processInfo.hThread);

										std::wprintf(L"Waiting for terminating...\n");
										WaitForSingleObject(processInfo.hProcess, INFINITE);
										// 应不可达
										CloseHandle(processInfo.hProcess);
										std::exit(0);
									}
								}

								std::wprintf(L"Cannot write or execute self update script, rolling back...\n");
							}
						}

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

extern std::function<void()> g_on_hook_ready;

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

		std::thread init_thread([dicts = std::move(dicts)] {
			logger::init_logger();

			if (g_enable_console)
			{
				start_console();
			}

			init_hook();

			std::mutex mutex;
			std::condition_variable cond;
			std::atomic<bool> hookIsReady(false);
			g_on_hook_ready = [&]
			{
				hookIsReady.store(true, std::memory_order_release);
				cond.notify_one();
			};

			// 依赖检查游戏版本的指针加载，因此在 hook 完成后再加载翻译数据
			std::unique_lock lock(mutex);
			cond.wait(lock, [&] {
				return hookIsReady.load(std::memory_order_acquire);
			});
			if (g_enable_console)
			{
				auto _ = freopen("CONOUT$", "w+t", stdout);
				_ = freopen("CONOUT$", "w", stderr);
				_ = freopen("CONIN$", "r", stdin);
			}

			auto staticDictCache = ensure_latest_static_cache(g_static_dict_path);
			if (g_dump_entries)
			{
				dump_static_dict("static_dump.json", staticDictCache);
			}
			local::load_textdb(&dicts, std::move(staticDictCache));
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
