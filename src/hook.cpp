#include <stdinclude.hpp>
#include <unordered_set>
#include <ranges>
#include "umagui/umaguiMain.hpp"
#include "umaHook/liveHook.hpp"
#include <set>
#include <Psapi.h>
#include <boost/regex.hpp>

using namespace std;
std::function<void()> g_on_hook_ready;
bool liveFirstPersonEnableRoll = false;
bool raceFollowUmaFirstPersion = false;
bool raceFollowUmaFirstPersionEnableRoll = false;
std::function<void(Il2CppString* title, Il2CppString* content, int buttonCount, int button1Text, int button2Text, int button3Text, int btn_type)> showDialog = nullptr;
bool guiStarting = false;

void _set_u_stat(bool s) {
	if (autoChangeLineBreakMode) {
		MHotkey::set_uma_stat(s);
	}
}

void LoadExtraAssetBundle();

namespace
{
	void path_game_assembly();
	int gallop_get_screenwidth_hook();
	int gallop_get_screenheight_hook();
	void set_start_resolution();

	bool mh_inited = false;

	void dump_bytes(void* pos, std::size_t dumpSize = 0x20)
	{
		printf("Hex dump of %p\n", pos);

		char* memory = reinterpret_cast<char*>(pos);

		for (std::size_t i = 0; i < dumpSize; i++)
		{
			if (i > 0 && i % 16 == 0)
				printf("\n");

			char byte = *(memory++);

			printf("%02hhX ", byte);
		}

		printf("\n\n");
	}

	void* load_library_w_orig = nullptr;
	HMODULE __stdcall load_library_w_hook(const wchar_t* path)
	{
		// GameAssembly.dll code must be loaded and decrypted while loading criware library
		if (path == L"cri_ware_unity.dll"sv)
		{
			path_game_assembly();
			if (g_on_hook_ready)
			{
				g_on_hook_ready();
			}

			MH_DisableHook(LoadLibraryW);
			MH_RemoveHook(LoadLibraryW);

			// use original function beacuse we have unhooked that
			return LoadLibraryW(path);
		}

		return reinterpret_cast<decltype(LoadLibraryW)*>(load_library_w_orig)(path);
	}

	void* populate_with_errors_orig = nullptr;
	bool populate_with_errors_hook(void* _this, Il2CppString* str, TextGenerationSettings_t* settings, void* context)
	{
		return reinterpret_cast<decltype(populate_with_errors_hook)*>(populate_with_errors_orig) (
			_this, local::get_localized_string(str), settings, context
			);
	}

	Il2CppString* (*environment_get_stacktrace)();

	void* localize_jp_get_orig = nullptr;
	Il2CppString* localize_jp_get_hook(int id)
	{
		auto orig_result = reinterpret_cast<decltype(localize_jp_get_hook)*>(localize_jp_get_orig)(id);
		auto result = local::get_localized_string(id);
		return result ? result : orig_result;
	}

	int (*Query_GetInt)(void* _this, int idx);

	struct ColumnIndex
	{
		std::size_t Value;
		std::optional<std::size_t> QueryResult;
	};

	struct BindingParam
	{
		std::size_t Value;
		std::optional<std::size_t> BindedValue;
	};

	using QueryIndex = std::variant<std::monostate, ColumnIndex, BindingParam>;

	struct ILocalizationQuery
	{
		virtual ~ILocalizationQuery() = default;

		virtual void AddColumn(std::size_t index, std::wstring_view column)
		{
		}

		virtual void AddParam(std::size_t index, std::wstring_view param)
		{
		}

		virtual void Bind(std::size_t index, std::size_t value)
		{
		}

		virtual void Step(void* query)
		{
		}

		virtual Il2CppString* GetString(std::size_t index) = 0;
	};

	struct TextDataQuery : ILocalizationQuery
	{
		QueryIndex Category;
		QueryIndex Index;

		QueryIndex Text;

		void AddColumn(std::size_t index, std::wstring_view column) override
		{
			if (column == L"text"sv)
			{
				Text.emplace<ColumnIndex>(index);
			}
		}

		void AddParam(std::size_t index, std::wstring_view param) override
		{
			if (param == L"category"sv)
			{
				Category.emplace<BindingParam>(index);
			}
			else if (param == L"index"sv)
			{
				Index.emplace<BindingParam>(index);
			}
		}

		void Bind(std::size_t index, std::size_t value) override
		{
			if (const auto p = std::get_if<BindingParam>(&Category))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
					return;
				}
			}

			if (const auto p = std::get_if<BindingParam>(&Index))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
				}
			}
		}

		Il2CppString* GetString(std::size_t index) override
		{
			if (index == std::get<ColumnIndex>(Text).Value)
			{
				const auto category = std::get<BindingParam>(Category).BindedValue.value();
				const auto index = std::get<BindingParam>(Index).BindedValue.value();
				return local::GetTextData(category, index);
			}
			return nullptr;
		}
	};

	struct CharacterSystemTextQuery : ILocalizationQuery
	{
		QueryIndex CharacterId;
		QueryIndex VoiceId;

		QueryIndex Text;

		void AddColumn(std::size_t index, std::wstring_view column) override
		{
			if (column == L"text"sv)
			{
				Text.emplace<ColumnIndex>(index);
			}
			else if (column == L"voice_id"sv)
			{
				VoiceId.emplace<ColumnIndex>(index);
			}
		}

		void AddParam(std::size_t index, std::wstring_view param) override
		{
			if (param == L"character_id"sv)
			{
				CharacterId.emplace<BindingParam>(index);
			}
			else if (param == L"voice_id"sv)
			{
				VoiceId.emplace<BindingParam>(index);
			}
		}

		void Bind(std::size_t index, std::size_t value) override
		{
			if (const auto p = std::get_if<BindingParam>(&CharacterId))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
					return;
				}
			}

			if (const auto p = std::get_if<BindingParam>(&VoiceId))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
				}
			}
		}

		void Step(void* query) override
		{
			assert(std::holds_alternative<BindingParam>(CharacterId));
			if (const auto p = std::get_if<ColumnIndex>(&VoiceId))
			{
				const auto voiceId = Query_GetInt(query, p->Value);
				p->QueryResult.emplace(voiceId);
			}
		}

		Il2CppString* GetString(std::size_t index) override
		{
			if (index == std::get<ColumnIndex>(Text).Value)
			{
				const auto characterId = std::get<BindingParam>(CharacterId).BindedValue.value();
				const auto voiceId = [&]
				{
					if (const auto column = std::get_if<ColumnIndex>(&VoiceId))
					{
						return column->QueryResult.value();
					}
					else
					{
						return std::get<BindingParam>(VoiceId).BindedValue.value();
					}
				}();

				return local::GetCharacterSystemTextData(characterId, voiceId);
			}
			return nullptr;
		}
	};

	struct RaceJikkyoCommentQuery : ILocalizationQuery
	{
		QueryIndex Id;

		QueryIndex Message;

		void AddColumn(std::size_t index, std::wstring_view column) override
		{
			if (column == L"message"sv)
			{
				Message.emplace<ColumnIndex>(index);
			}
			else if (column == L"id"sv)
			{
				Id.emplace<ColumnIndex>(index);
			}
		}

		void AddParam(std::size_t index, std::wstring_view param) override
		{
			if (param == L"id"sv)
			{
				Id.emplace<BindingParam>(index);
			}
		}

		void Bind(std::size_t index, std::size_t value) override
		{
			if (const auto p = std::get_if<BindingParam>(&Id))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
				}
			}
		}

		void Step(void* query) override
		{
			if (const auto p = std::get_if<ColumnIndex>(&Id))
			{
				const auto id = Query_GetInt(query, p->Value);
				p->QueryResult.emplace(id);
			}
		}

		Il2CppString* GetString(std::size_t index) override
		{
			if (index == std::get<ColumnIndex>(Message).Value)
			{
				const auto id = [&]
				{
					if (const auto column = std::get_if<ColumnIndex>(&Id))
					{
						return column->QueryResult.value();
					}
					else
					{
						return std::get<BindingParam>(Id).BindedValue.value();
					}
				}();
				return local::GetRaceJikkyoCommentData(id);
			}
			return nullptr;
		}
	};

	struct RaceJikkyoMessageQuery : ILocalizationQuery
	{
		QueryIndex Id;

		QueryIndex Message;

		void AddColumn(std::size_t index, std::wstring_view column) override
		{
			if (column == L"message"sv)
			{
				Message.emplace<ColumnIndex>(index);
			}
			else if (column == L"id"sv)
			{
				Id.emplace<ColumnIndex>(index);
			}
		}

		void AddParam(std::size_t index, std::wstring_view param) override
		{
			if (param == L"id"sv)
			{
				Id.emplace<BindingParam>(index);
			}
		}

		void Bind(std::size_t index, std::size_t value) override
		{
			if (const auto p = std::get_if<BindingParam>(&Id))
			{
				if (index == p->Value)
				{
					p->BindedValue.emplace(value);
				}
			}
		}

		void Step(void* query) override
		{
			if (const auto p = std::get_if<ColumnIndex>(&Id))
			{
				const auto id = Query_GetInt(query, p->Value);
				p->QueryResult.emplace(id);
			}
		}

		Il2CppString* GetString(std::size_t index) override
		{
			if (index == std::get<ColumnIndex>(Message).Value)
			{
				const auto id = [&]
				{
					if (const auto column = std::get_if<ColumnIndex>(&Id))
					{
						return column->QueryResult.value();
					}
					else
					{
						return std::get<BindingParam>(Id).BindedValue.value();
					}
				}();
				return local::GetRaceJikkyoMessageData(id);
			}
			return nullptr;
		}
	};

	std::unordered_map<void*, std::unique_ptr<ILocalizationQuery>> text_queries;

	void* query_setup_orig = nullptr;
	void* query_setup_hook(void* _this, void* conn, Il2CppString* sql)
	{
		static const std::wregex statementPattern(LR"(SELECT (.+?) FROM `(.+?)`(?: WHERE (.+))?;)");
		static const std::wregex columnPattern(LR"(,?`(\w+)`)");
		static const std::wregex whereClausePattern(LR"((?:AND )?`(\w+)=?`)");

		std::wcmatch matches;
		if (std::regex_match(sql->start_char, matches, statementPattern))
		{
			const auto columns = matches[1].str();
			const auto table = matches[2].str();
			const auto whereClause = matches.size() == 4 ? std::optional(matches[3].str()) : std::nullopt;

			std::unique_ptr<ILocalizationQuery> query;

			if (table == L"text_data")
			{
				query = std::make_unique<TextDataQuery>();
			}
			else if (table == L"character_system_text")
			{
				query = std::make_unique<CharacterSystemTextQuery>();
			}
			else if (table == L"race_jikkyo_comment")
			{
				query = std::make_unique<RaceJikkyoCommentQuery>();
			}
			else if (table == L"race_jikkyo_message")
			{
				query = std::make_unique<RaceJikkyoMessageQuery>();
			}
			else
			{
				goto NormalPath;
			}

			auto columnsPtr = columns.c_str();
			std::size_t columnIndex{};
			while (std::regex_search(columnsPtr, matches, columnPattern))
			{
				const auto column = matches[1].str();
				query->AddColumn(columnIndex++, column);

				columnsPtr = matches.suffix().first;
			}

			// 有 WHERE 子句的查询
			if (whereClause)
			{
				auto whereClausePtr = whereClause->c_str();
				std::size_t paramIndex = 1;
				while (std::regex_search(whereClausePtr, matches, whereClausePattern))
				{
					const auto param = matches[1].str();
					query->AddParam(paramIndex++, param);

					whereClausePtr = matches.suffix().first;
				}
			}

			text_queries.emplace(_this, std::move(query));
		}

	NormalPath:
		return reinterpret_cast<decltype(query_setup_hook)*>(query_setup_orig)(_this, conn, sql);
	}

	void* query_dispose_orig = nullptr;
	void query_dispose_hook(void* _this)
	{
		text_queries.erase(_this);

		return reinterpret_cast<decltype(query_dispose_hook)*>(query_dispose_orig)(_this);
	}

	void* query_getstr_orig = nullptr;
	Il2CppString* query_getstr_hook(void* _this, int32_t idx)
	{
		if (const auto iter = text_queries.find(_this); iter != text_queries.end())
		{
			iter->second->Step(_this);
			if (const auto localizedStr = iter->second->GetString(idx))
			{
				return localizedStr;
			}
		}

		return reinterpret_cast<decltype(query_getstr_hook)*>(query_getstr_orig)(_this, idx);
	}

	void* PreparedQuery_BindInt_orig = nullptr;
	void PreparedQuery_BindInt_hook(void* _this, int32_t idx, int32_t value)
	{
		if (const auto iter = text_queries.find(_this); iter != text_queries.end())
		{
			iter->second->Bind(idx, value);
		}

		return reinterpret_cast<decltype(PreparedQuery_BindInt_hook)*>(PreparedQuery_BindInt_orig)(_this, idx, value);
	}

	std::ptrdiff_t Query_stmt_offset;

	void* Plugin_sqlite3_step_orig = nullptr;
	int Plugin_sqlite3_step_hook(void* _this)
	{
		const auto result = reinterpret_cast<decltype(Plugin_sqlite3_step_hook)*>(Plugin_sqlite3_step_orig)(_this);
		// 注意现在直接调用 sqlite3_step，_this 是 Query._stmt
		// TODO: 另一部分 step 直接调用 trampoline，以现在的 hook 实现无法修改，当前在 getstr 中调用 step，这可能使得 step 被重复调用
		const auto query = static_cast<std::byte*>(_this) - Query_stmt_offset;

		if (const auto iter = text_queries.find(query); iter != text_queries.end())
		{
			iter->second->Step(query);
		}

		return result;
	}

	void* on_exit_orig = nullptr;
	void on_exit_hook(void* _this) {
		MHotkey::closeExternalPlugin();
		TerminateProcess(GetCurrentProcess(), 0);
	}

	void* set_fps_orig = nullptr;
	void set_fps_hook(int value)
	{
		return reinterpret_cast<decltype(set_fps_hook)*>(set_fps_orig)(g_max_fps);
	}

	void* set_vsync_count_orig = nullptr;
	void set_vsync_count_hook(int value) {
		// printf("setVsyncCount: %d -> %d\n", value, g_vsync_count);
		return reinterpret_cast<decltype(set_vsync_count_hook)*>(set_vsync_count_orig)(g_vsync_count == -1 ? value : g_vsync_count);
	}

	void* set_antialiasing_orig = nullptr;
	void set_antialiasing_hook(int value) {
		// printf("setAntialiasing: %d -> %d\n", value, g_antialiasing);
		set_vsync_count_hook(1);
		return reinterpret_cast<decltype(set_antialiasing_hook)*>(set_antialiasing_orig)(g_antialiasing == -1 ? value : g_antialiasing);
	}

	void* graphics_quality_orig = nullptr;
	void graphics_quality_hook(Il2CppObject* thisObj, int quality, bool force) {
		// printf("setGraphicsQuality: %d -> %d\n", quality, g_graphics_quality);
		return reinterpret_cast<decltype(graphics_quality_hook)*>(graphics_quality_orig)(thisObj,
			g_graphics_quality == -1 ? quality : g_graphics_quality,
			true);
	}

	void* set_RenderTextureAntiAliasing_orig;
	void set_RenderTextureAntiAliasing_hook(void* _this, int value) {
		return reinterpret_cast<decltype(set_RenderTextureAntiAliasing_hook)*>(set_RenderTextureAntiAliasing_orig)(_this, 
			g_antialiasing == -1 ? value : g_antialiasing);
	}

	void* Get3DAntiAliasingLevel_orig;
	int Get3DAntiAliasingLevel_hook(void* _this, bool allowMSAA) {
		if (g_antialiasing != -1) allowMSAA = true;
		auto data = reinterpret_cast<decltype(Get3DAntiAliasingLevel_hook)*>(Get3DAntiAliasingLevel_orig)(_this, allowMSAA);
		// printf("Get3DAntiAliasingLevel: %d %d\n", allowMSAA, data);
		return data;
	}

	void* KeepAspectRatio_orig;
	void KeepAspectRatio_hook(float w, float h) {
		// printf("KeepAspectRatio_hook: %f, %f\n", w, h);
		// if (g_unlock_size) return;
		return reinterpret_cast<decltype(KeepAspectRatio_hook)*>(KeepAspectRatio_orig)(w, h);
	}

	void* ReshapeAspectRatio_orig;
	void ReshapeAspectRatio_hook() {
		// printf("ReshapeAspectRatio_hook\n");
		if (g_unlock_size) return;
		return reinterpret_cast<decltype(ReshapeAspectRatio_hook)*>(ReshapeAspectRatio_orig)();
	}

	bool setWindowPosOffset(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
		std::shared_ptr<RECT> windowR = std::make_shared<RECT>();
		std::shared_ptr<RECT> clientR = std::make_shared<RECT>();
		GetWindowRect(hWnd, windowR.get());
		GetClientRect(hWnd, clientR.get());

		return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx + windowR->right - windowR->left - clientR->right,
			cy + windowR->bottom - windowR->top - clientR->bottom, uFlags);;
	}

	bool (*is_virt_o)() = nullptr;
	int last_height = 0, last_width = 0;

	bool is_virt() {
		if (g_force_landscape) return false;
		return is_virt_o();
	}

	bool firstTimeScreen = true;
	void set_resolution_hook(int width, int height, bool fullscreen);
	// void recheck_ratio(bool use_cache = false);

	void* InvokeMoveNext_orig;
	void InvokeMoveNext_hook(void* enumerator, void* returnValueAddress) {
		if (g_force_landscape) {
			auto klass = il2cpp_symbols::get_class_from_instance(enumerator);
			std::string_view klassName = static_cast<Il2CppClassHead*>(klass)->name;
			// printf("klassName: %s\n", klassName.data());
			if (klassName.find("ChangeScreenOrientationPortraitAsync") != std::string_view::npos) {
				if (firstTimeScreen) {
					firstTimeScreen = false;
					if ((start_width == -1) || (start_height == -1)) {
						start_height = 800;
						start_width = g_aspect_ratio * start_height;
					}
					const auto isNowVert = start_width < start_height;
					if (!(isNowVert && is_virt())) {
						const auto cw = start_width;
						start_width = start_height;
						start_height = cw;
					}
					set_resolution_hook(start_width, start_height, false);
				}
				return;
			}
		}
		return reinterpret_cast<decltype(InvokeMoveNext_hook)*>(InvokeMoveNext_orig)(enumerator, returnValueAddress);
	}

	void* CoroutineExecutor_klass;
	FieldInfo* CoroutineExecutor_targetCoroutine;
	bool CoroutineExecutor_inited = false;

	void initCoroutineExecutor() {
		if (CoroutineExecutor_inited) return;
		CoroutineExecutor_inited = true;
		CoroutineExecutor_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "CoroutineExecutor");
		CoroutineExecutor_targetCoroutine = il2cpp_class_get_field_from_name(CoroutineExecutor_klass, "_targetCoroutine");
	}

	void* UpdateCoroutine_orig;
	bool UpdateCoroutine_hook(void* _this) {
		if (g_force_landscape) {
			initCoroutineExecutor();
			auto targetCoroutine = il2cpp_symbols::read_field(_this, CoroutineExecutor_targetCoroutine);
			if (targetCoroutine) {
				auto klass = il2cpp_symbols::get_class_from_instance(targetCoroutine);
				std::string_view klassName = static_cast<Il2CppClassHead*>(klass)->name;
				// printf("UpdateCoroutine: %s\n", klassName.data());
				if (klassName.find("ChangeScreenOrientationPortraitAsync") != std::string_view::npos) {
					return true;
				}
			}
		}
		return reinterpret_cast<decltype(UpdateCoroutine_hook)*>(UpdateCoroutine_orig)(_this);
	}

	RECT* updateWindowRatio(HWND hWnd, RECT* modifiedR, WPARAM wParam, bool resize_now) {
		std::shared_ptr<RECT> windowR = std::make_shared<RECT>();
		std::shared_ptr<RECT> clientR = std::make_shared<RECT>();
		GetWindowRect(hWnd, windowR.get());
		GetClientRect(hWnd, clientR.get());

		float add_w = modifiedR->right - modifiedR->left - (windowR->right - windowR->left);
		float add_h = modifiedR->bottom - modifiedR->top - (windowR->bottom - windowR->top);

		if (add_h != 0.f) {
			add_w = add_h * g_aspect_ratio;
		}
		else {
			add_h = add_w / g_aspect_ratio;
		}

		// printf("windowR: left: %ld, right: %ld, top: %ld, bottom: %ld\n", windowR->left, windowR->right, windowR->top, windowR->bottom);
		// printf("modifiR: left: %ld, right: %ld, top: %ld, bottom: %ld\n", modifiedR->left, modifiedR->right, modifiedR->top, modifiedR->bottom);

		int X = windowR->left;
		int Y = windowR->top;
		int cx = clientR->right;
		int cy;

		bool isVert = is_virt();
		if (isVert)
			cy = cx * g_aspect_ratio;
		else
			cy = cx / g_aspect_ratio;

		cx += add_w;
		cy += add_h;

		float new_width = cx + windowR->right - windowR->left - clientR->right;
		float new_height = cy + windowR->bottom - windowR->top - clientR->bottom;

		RECT* newWindowR = new RECT();
		newWindowR->left = X;
		newWindowR->top = Y;
		newWindowR->right = X + new_width;
		newWindowR->bottom = Y + new_height;

		switch (wParam)
		{
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			newWindowR->top -= add_h;
			newWindowR->bottom -= add_h;
			break;
		default:
			break;
		}

		switch (wParam)
		{
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT: {
			newWindowR->left -= add_w;
			newWindowR->right -= add_w;
		}; break;
		default:
			break;
		}

		if (resize_now) {
			SetWindowPos(hWnd, HWND_NOTOPMOST, newWindowR->left, newWindowR->top, 
				newWindowR->right - newWindowR->left, newWindowR->bottom - newWindowR->top, SWP_DEFERERASE);
			// printf("resizeNow: left: %ld, right: %ld, top: %ld, bottom: %ld\n", newWindowR->left, newWindowR->right, newWindowR->top, newWindowR->bottom);
		}
		return newWindowR;
	}

	RECT* land_cache_rect = new RECT();
	RECT* vert_cache_rect = new RECT();
	bool land_cache_rect_cache = false;
	bool vert_cache_rect_cache = false;

	void recheck_ratio(bool use_cache = false) {
		auto window = FindWindowW(L"UnityWndClass", L"umamusume");
		if (window == NULL) {
			printf("window[1]: UnityWndClass - umamusume not found.\n");
			return;
		}

		if (use_cache) {
			bool isVert = is_virt();
			RECT* newWindowR = isVert ? vert_cache_rect : land_cache_rect;
			bool is_cache = isVert ? vert_cache_rect_cache : land_cache_rect_cache;

			if (is_cache) {
				printf("resizeCache: left: %ld, right: %ld, top: %ld, bottom: %ld\n", newWindowR->left, newWindowR->right, newWindowR->top, newWindowR->bottom);
				SetWindowPos(window, HWND_NOTOPMOST, newWindowR->left, newWindowR->top,
					newWindowR->right - newWindowR->left, newWindowR->bottom - newWindowR->top, SWP_DEFERERASE);
				// return;
			}
		}

		std::shared_ptr<RECT> windowR = std::make_shared<RECT>();
		GetWindowRect(window, windowR.get());
		windowR->right += 1;
		updateWindowRatio(window, windowR.get(), WMSZ_RIGHT, true);
	}

	void recheck_ratio_later(int latertimeMills, bool use_cache = false) {
		thread([latertimeMills, use_cache]() {
			Sleep(latertimeMills);
			recheck_ratio(use_cache);
			}).detach();
	}

	bool isLiveStart = false;
	void* wndproc_orig = nullptr;

	bool raceStart = false;

	LRESULT wndproc_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_INPUT)
		{
			if (isLiveStart || (g_race_free_camera && raceStart)) {
				RAWINPUT rawInput;
				UINT size = sizeof(RAWINPUT);
				if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &rawInput, &size, sizeof(RAWINPUTHEADER)) == size)
				{
					
					if (rawInput.header.dwType == RIM_TYPEMOUSE)
					{
						switch (rawInput.data.mouse.ulButtons) {
						case 0: {  // move
							UmaCamera::mouseMove(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY, 3);
						}; break;
						case 4: {  // press
							UmaCamera::mouseMove(0, 0, 1);
						}; break;
						case 8: {  // release
							UmaCamera::mouseMove(0, 0, 2);
						}; break;
						default: break;
						}

						if (rawInput.data.mouse.usButtonFlags == RI_MOUSE_WHEEL) {
							if (rawInput.data.mouse.usButtonData == 120) {
								UmaCamera::mouseMove(0, 1, 4);
							}
							else {
								UmaCamera::mouseMove(0, -1, 4);
							}
						}

					}
				}
			}
		}

		if (g_unlock_size) {
			if (uMsg == WM_SIZING)
			{
				RECT* rect = reinterpret_cast<RECT*>(lParam);

				bool isVert = is_virt();

				if ((!isVert) || (wParam == WMSZ_LEFT) || (wParam == WMSZ_RIGHT)) {  // 可优化, 但是能用了!
					RECT* rect = reinterpret_cast<RECT*>(lParam);
					auto ret = updateWindowRatio(hWnd, rect, wParam, false);
					rect->left = ret->left;
					rect->right = ret->right;
					rect->top = ret->top;
					rect->bottom = ret->bottom;
					delete ret;
					return TRUE;
				}

				float ratio = isVert ? 1.f / g_aspect_ratio : g_aspect_ratio;
				float height = rect->bottom - rect->top;
				float width = rect->right - rect->left;

				float new_ratio = width / height;

				if (new_ratio > ratio && height >= last_height || width < last_width)
					height = width / ratio;
				else if (new_ratio < ratio && width >= last_width || height < last_height)
					width = height * ratio;

				switch (wParam)
				{
				case WMSZ_TOP:
				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
					rect->top = rect->bottom - height;
					break;
				default:
					rect->bottom = rect->top + height;
					break;
				}

				switch (wParam)
				{
				case WMSZ_LEFT:
				case WMSZ_TOPLEFT:
				case WMSZ_BOTTOMLEFT:
					rect->left = rect->right - width;
					break;
				default:
					rect->right = rect->left + width;
					break;
				}

				last_height = height;
				last_width = width;

				return TRUE;
			}

		}
		return reinterpret_cast<decltype(wndproc_hook)*>(wndproc_orig)(hWnd, uMsg, wParam, lParam);
	}

	void* get_virt_size_orig = nullptr;
	Vector3_t* get_virt_size_hook(Vector3_t* pVec3, int width, int height)
	{
		auto size = reinterpret_cast<decltype(get_virt_size_hook)*>(get_virt_size_orig)(pVec3, width, height);
		height = width * g_aspect_ratio;

		size->x = width;
		size->y = height;
		size->z = g_aspect_ratio;

		return size;
	}

	void* get_hori_size_orig = nullptr;
	Vector3_t* get_hori_size_hook(Vector3_t* pVec3, int width, int height)
	{
		auto size = reinterpret_cast<decltype(get_hori_size_hook)*>(get_hori_size_orig)(pVec3, width, height);
		width = height * g_aspect_ratio;

		size->x = width;
		size->y = height;
		size->z = g_aspect_ratio;

		return size;
	}

	Resolution_t* (*get_resolution)(Resolution_t* buffer);

	void get_resolution_stub(Resolution_t* r)
	{
		*r = *get_resolution(r);

		int width = min(r->height, r->width) * g_aspect_ratio;
		if (r->width > r->height)
			r->width = width;
		else
			r->height = width;
	}

	void* gallop_get_screenheight_orig;
	int gallop_get_screenheight_hook()
	{
		Resolution_t res;
		get_resolution_stub(&res);

		int w = max(res.width, res.height), h = min(res.width, res.height);

		return is_virt() ? w : h;
	}

	void* gallop_get_screenwidth_orig;
	int gallop_get_screenwidth_hook()
	{
		Resolution_t res;
		get_resolution_stub(&res);

		int w = max(res.width, res.height), h = min(res.width, res.height);

		return is_virt() ? h : w;
	}

	void (*set_scale_factor)(void*, float);

	void* (*UIManager_GetCanvasScalerList)(void*);

	void* canvas_scaler_setres_orig;
	void canvas_scaler_setres_hook(void* _this, Vector2_t res)
	{
		Resolution_t r;
		get_resolution_stub(&r);

		// set scale factor to make ui bigger on hi-res screen
		set_scale_factor(_this, max(1.0f, r.width / 1920.f) * g_ui_scale);

		return reinterpret_cast<decltype(canvas_scaler_setres_hook)*>(canvas_scaler_setres_orig)(_this, res);
	}

	void* change_resize_ui_for_pc_orig;
	void change_resize_ui_for_pc_hook(void* _this, int width, int height)
	{
		Resolution_t r;
		get_resolution_stub(&r);

		reinterpret_cast<decltype(change_resize_ui_for_pc_hook)*>(change_resize_ui_for_pc_orig)(_this, width, height);

		const auto canvasScalerList = UIManager_GetCanvasScalerList(_this);
		il2cpp_symbols::iterate_IEnumerable(canvasScalerList, [&](void* canvasScaler)
			{
				set_scale_factor(canvasScaler, max(1.0f, r.width / 1920.f) * g_ui_scale);
			});
	}

	struct TransparentStringHash : std::hash<std::wstring>, std::hash<std::wstring_view>
	{
		using is_transparent = void;
	};

	// std::unordered_set<std::wstring, TransparentStringHash, std::equal_to<void>> ExtraAssetBundleAssetPaths;
	typedef std::unordered_set<std::wstring, TransparentStringHash, std::equal_to<void>> AssetPathsType;
	std::map<string, AssetPathsType> CustomAssetBundleAssetPaths;
	void* (*AssetBundle_LoadFromFile)(Il2CppString* path);
	void (*AssetBundle_Unload)(void* _this, bool unloadAllLoadedObjects);

	// uint32_t ExtraAssetBundleHandle;
	unordered_map<string, uint32_t> CustomAssetBundleHandleMap{};

	void* StoryTimelineDataClass;
	FieldInfo* StoryTimelineDataClass_StoryIdField;
	FieldInfo* StoryTimelineDataClass_TitleField;
	FieldInfo* StoryTimelineDataClass_BlockListField;
	void* StoryTimelineTextClipDataClass;
	FieldInfo* StoryTimelineTextClipDataClass_NameField;
	FieldInfo* StoryTimelineTextClipDataClass_TextField;
	FieldInfo* StoryTimelineTextClipDataClass_ChoiceDataList;
	void* StoryTimelineTextClipDataClass_ChoiceDataClass;
	FieldInfo* StoryTimelineTextClipDataClass_ChoiceDataClass_TextField;
	FieldInfo* StoryTimelineTextClipDataClass_ColorTextInfoListField;
	void* StoryTimelineTextClipDataClass_ColorTextInfoClass;
	FieldInfo* StoryTimelineTextClipDataClass_ColorTextInfoClass_TextField;
	void* StoryTimelineBlockDataClass;
	FieldInfo* StoryTimelineBlockDataClass_TextTrackField;
	void* StoryTimelineTextTrackDataClass;
	FieldInfo* StoryTimelineTrackDataClass_ClipListField;
	void* StoryTimelineClipDataClass;

	void* TextRubyDataClass;
	FieldInfo* TextRubyDataClass_DataArray;
	void* RubyBlockDataClass;
	FieldInfo* RubyBlockDataClass_BlockIndex;
	FieldInfo* RubyBlockDataClass_RubyDataList;
	void* RubyDataClass;
	FieldInfo* RubyDataClass_CharX;
	FieldInfo* RubyDataClass_CharY;
	FieldInfo* RubyDataClass_RubyText;

	uint32_t GetBundleHandleByAssetName(wstring assetName) {
		for (const auto& i : CustomAssetBundleAssetPaths) {
			for (const auto& m : i.second) {
				if (std::equal(m.begin(), m.end(), assetName.begin(), assetName.end(),
					[](wchar_t c1, wchar_t c2) {
						return std::tolower(c1, std::locale()) == std::tolower(c2, std::locale());
					})) {
					return CustomAssetBundleHandleMap.at(i.first);
				}
			}
		}
		return NULL;
	}

	uint32_t GetBundleHandleByAssetName(string assetName) {
		return GetBundleHandleByAssetName(utility::conversions::to_string_t(assetName));
	}

	void LocalizeStoryTextRubyData(void* textRuby) {
		const auto textRubyDataArr = il2cpp_symbols::read_field(textRuby, TextRubyDataClass_DataArray);

		il2cpp_symbols::iterate_IEnumerable(textRubyDataArr, [&](void* data) {
			RubyBlockDataClass = il2cpp_symbols::get_class_from_instance(data);
			RubyBlockDataClass_BlockIndex = il2cpp_class_get_field_from_name(RubyBlockDataClass, "BlockIndex");
			RubyBlockDataClass_RubyDataList = il2cpp_class_get_field_from_name(RubyBlockDataClass, "RubyDataList");
			// auto blockIndex = il2cpp_symbols::read_field<int32_t>(data, RubyBlockDataClass_BlockIndex);
			auto rubyDataList = il2cpp_symbols::read_field(data, RubyBlockDataClass_RubyDataList);

			il2cpp_symbols::iterate_list(rubyDataList, [&](int32_t i, void* rubyData) {
				RubyDataClass = il2cpp_symbols::get_class_from_instance(rubyData);
				RubyDataClass_CharX = il2cpp_class_get_field_from_name(RubyDataClass, "CharX");
				RubyDataClass_CharY = il2cpp_class_get_field_from_name(RubyDataClass, "CharY");
				RubyDataClass_RubyText = il2cpp_class_get_field_from_name(RubyDataClass, "RubyText");

				// auto charX = il2cpp_symbols::read_field<float>(rubyData, RubyDataClass_CharX);
				// auto charY = il2cpp_symbols::read_field<float>(rubyData, RubyDataClass_CharY);
				// auto rubyText = il2cpp_symbols::read_field<Il2CppString*>(rubyData, RubyDataClass_RubyText);

				// printf("index: %d, x: %f, y: %f, char: %ls\n", blockIndex, charX, charY, rubyText->start_char);
				il2cpp_symbols::write_field(rubyData, RubyDataClass_RubyText, il2cpp_symbols::NewWStr(L""));
			});
			
		});
	}

	void LocalizeStoryTimelineData(void* timelineData, std::size_t storyId)
	{
		const auto localizedStoryData = local::GetStoryTextData(storyId);
		if (!localizedStoryData)
		{
			return;
		}

		il2cpp_symbols::write_field(timelineData, StoryTimelineDataClass_TitleField, il2cpp_symbols::NewWStr(localizedStoryData->Title));

		const auto blockList = il2cpp_symbols::read_field(timelineData, StoryTimelineDataClass_BlockListField);
		il2cpp_symbols::iterate_list(blockList, [&](int32_t i, void* blockData) {
			if (!StoryTimelineBlockDataClass)
			{
				StoryTimelineBlockDataClass = il2cpp_symbols::get_class_from_instance(blockData);
				StoryTimelineBlockDataClass_TextTrackField = il2cpp_class_get_field_from_name(StoryTimelineBlockDataClass, "TextTrack");
			}
			
			if (i >= localizedStoryData->TextBlockList.size()) {
				printf("[ERROR] Exception occurred while loading story text in TextBlockList. storyId: %llu, block: %d\n", storyId, i);
				printf("The text content may be changed by Cygames, and the localized content may not display properly. Please report to the project maintainer.\n");
				return;
			}
			const auto& clip = localizedStoryData->TextBlockList[i];
			if (!clip)
			{
				return;
			}
			const auto textTrack = il2cpp_symbols::read_field(blockData, StoryTimelineBlockDataClass_TextTrackField);
			if (!textTrack)
			{
				return;
			}
			if (!StoryTimelineTextTrackDataClass)
			{
				StoryTimelineTextTrackDataClass = il2cpp_symbols::get_class_from_instance(textTrack);
				StoryTimelineTrackDataClass_ClipListField = il2cpp_class_get_field_from_name(StoryTimelineTextTrackDataClass, "ClipList");
			}

			const auto clipList = il2cpp_symbols::read_field(textTrack, StoryTimelineTrackDataClass_ClipListField);
			il2cpp_symbols::iterate_list(clipList, [&](int32_t dummy, void* clipData) {
				assert(dummy == 0);
				StoryTimelineClipDataClass = il2cpp_symbols::get_class_from_instance(clipData);
				if (StoryTimelineTextClipDataClass == StoryTimelineClipDataClass)
				{
					il2cpp_symbols::write_field(clipData, StoryTimelineTextClipDataClass_NameField, il2cpp_symbols::NewWStr(clip->Name));
					il2cpp_symbols::write_field(clipData, StoryTimelineTextClipDataClass_TextField, il2cpp_symbols::NewWStr(clip->Text));
					const auto choiceDataList = il2cpp_symbols::read_field(clipData, StoryTimelineTextClipDataClass_ChoiceDataList);
					if (choiceDataList)
					{
						il2cpp_symbols::iterate_list(choiceDataList, [&](int32_t j, void* choiceData) {
							if (!StoryTimelineTextClipDataClass_ChoiceDataClass)
							{
								StoryTimelineTextClipDataClass_ChoiceDataClass = il2cpp_symbols::get_class_from_instance(choiceData);
								StoryTimelineTextClipDataClass_ChoiceDataClass_TextField = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass_ChoiceDataClass, "Text");
							}

							if (j < clip->ChoiceDataList.size()) {
								il2cpp_symbols::write_field(choiceData, StoryTimelineTextClipDataClass_ChoiceDataClass_TextField, il2cpp_symbols::NewWStr(clip->ChoiceDataList[j]));
							}
							else {
								printf("[ERROR] Exception occurred while loading story text in ChoiceDataList. storyId: %llu, block: %d, listIndex: %d\n", storyId, i, j);
								printf("The text content may be changed by Cygames, and the localized content may not display properly. Please report to the project maintainer.\n");
							}
						});
					}
					const auto colorTextInfoList = il2cpp_symbols::read_field(clipData, StoryTimelineTextClipDataClass_ColorTextInfoListField);
					if (colorTextInfoList)
					{
						il2cpp_symbols::iterate_list(colorTextInfoList, [&](int32_t j, void* colorTextInfo) {
							if (!StoryTimelineTextClipDataClass_ColorTextInfoClass)
							{
								StoryTimelineTextClipDataClass_ColorTextInfoClass = il2cpp_symbols::get_class_from_instance(colorTextInfo);
								StoryTimelineTextClipDataClass_ColorTextInfoClass_TextField = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass_ColorTextInfoClass, "Text");
							}

							if (j < clip->ColorTextInfoList.size()) {
								il2cpp_symbols::write_field(colorTextInfo, StoryTimelineTextClipDataClass_ColorTextInfoClass_TextField, il2cpp_symbols::NewWStr(clip->ColorTextInfoList[j]));
							}
							else {
								printf("[ERROR] Exception occurred while loading story text in ChoiceDataList. storyId: %llu, block: %d, listIndex: %d\n", storyId, i, j);
								printf("The text content may be changed by Cygames, and the localized content may not display properly. Please report to the project maintainer.\n");
							}
						});
					}
				}
			});
		});
	}

	void* StoryRaceTextAssetClass;
	TypedField<void*> StoryRaceTextAssetClass_textDataField;
	void* StoryRaceTextAssetKeyClass;
	TypedField<Il2CppString*> StoryRaceTextAssetKeyClass_textField;

	void LocalizeStoryRaceTextAsset(void* raceTextAsset, std::size_t raceId)
	{
		const auto localizedRaceData = local::GetRaceTextData(raceId);
		if (!localizedRaceData)
		{
			return;
		}

		const auto textData = il2cpp_symbols::read_field(raceTextAsset, StoryRaceTextAssetClass_textDataField);
		il2cpp_symbols::iterate_IEnumerable(textData, [&, i = 0](void* key) mutable
			{
				if (!StoryRaceTextAssetKeyClass)
				{
					StoryRaceTextAssetKeyClass = il2cpp_symbols::get_class_from_instance(key);
					StoryRaceTextAssetKeyClass_textField = { il2cpp_class_get_field_from_name(StoryRaceTextAssetKeyClass, "text") };
				}
				const auto text = il2cpp_symbols::read_field(key, StoryRaceTextAssetKeyClass_textField);
				il2cpp_symbols::write_field(key, StoryRaceTextAssetKeyClass_textField, il2cpp_symbols::NewWStr(localizedRaceData->textData[i++]));
			});
	}

	void LocalizeAssets(void* result, Il2CppString* name) {
		const auto cls = il2cpp_symbols::get_class_from_instance(result);
		if (cls == StoryTimelineDataClass)
		{
			const auto assetPath = std::filesystem::path(name->start_char).stem();
			const std::wstring_view assetName = assetPath.native();
			constexpr const wchar_t StoryTimelinePrefix[] = L"storytimeline_";
			constexpr const wchar_t HomeTimelinePrefix[] = L"hometimeline_";
			const auto storyId = assetName.starts_with(StoryTimelinePrefix) ? static_cast<std::size_t>(_wtoll(assetName.substr(std::size(StoryTimelinePrefix) - 1).data())) :
				assetName.starts_with(HomeTimelinePrefix) ? static_cast<std::size_t>(std::stoull([&] {
				auto range = assetName | std::ranges::views::drop(std::size(HomeTimelinePrefix) - 1) | std::ranges::views::filter([](wchar_t ch) { return ch != L'_'; });
				return std::wstring(std::ranges::begin(range), std::ranges::end(range));
					}())) : -1;
			if (storyId != -1)
			{
				LocalizeStoryTimelineData(result, storyId);
			}
		}
		else if (cls == StoryRaceTextAssetClass)
		{
			const auto assetPath = std::filesystem::path(name->start_char).stem();
			const std::wstring_view assetName = assetPath.native();
			constexpr const wchar_t RacePrefix[] = L"storyrace_";
			assert(assetName.starts_with(RacePrefix));
			LocalizeStoryRaceTextAsset(result, static_cast<std::size_t>(_wtoll(assetName.substr(std::size(RacePrefix) - 1).data())));
		}
		else if (cls == TextRubyDataClass) {
			LocalizeStoryTextRubyData(result);
		}
	}

	void* AssetBundle_LoadAsset_orig;
	void* AssetBundle_LoadAsset_hook(void* _this, Il2CppString* name, Il2CppReflectionType* type)
	{
		UmaDatabase::setBundleHandleTargetCache(name->start_char, _this);
		//const auto stackTrace = environment_get_stacktrace();
		const auto& bundleHandle = GetBundleHandleByAssetName(name->start_char);
		if (bundleHandle)
		{
			const auto extraAssetBundle = il2cpp_gchandle_get_target(bundleHandle);
			return reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(extraAssetBundle, name, type);
		}

		if (g_enable_replaceBuiltInAssets) {
			auto newAsseetData = UmaDatabase::origPathToNewPath(name->start_char);
			if (!newAsseetData.first.empty()) {
				const auto newAssetBundlePath = std::string(newAsseetData.second.begin(), newAsseetData.second.end());
				const auto newBundleFilePath = UmaDatabase::bundleNameToPath(newAssetBundlePath);
				auto bundleFile = AssetBundle_LoadFromFile(
					il2cpp_string_new(newBundleFilePath.c_str())
				);
				if (!bundleFile) {
					auto reGet = UmaDatabase::getBundleHandleTargetCache(newAsseetData.first);
					if (reGet != nullptr) {
						bundleFile = reGet;
						printf("Load failed but hit cache: %ls\n", newAsseetData.first.c_str());
					}
				}
				if (bundleFile) {
					auto bundleHandle = il2cpp_gchandle_new(bundleFile, false);
					const auto bundle = il2cpp_gchandle_get_target(bundleHandle);
					auto newFilePath = std::string(newAsseetData.first.begin(), newAsseetData.first.end());

					printf("Redirect asset: %ls To: %s At: %s\n", name->start_char, newFilePath.c_str(), newBundleFilePath.c_str());

					auto retData = reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(bundle, il2cpp_string_new(newFilePath.c_str()), type);
					AssetBundle_Unload(bundle, false);
					return retData;
				}
				else {
					printf("Load built-in asset failed: %s\n", newBundleFilePath.c_str());
				}
			}
		}


		void* result = reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(_this, name, type);
		
		if (g_asset_load_log)
		{
			const auto cls = il2cpp_symbols::get_class_from_instance(result);
			const auto assetCls = static_cast<Il2CppClassHead*>(cls);
			std::wprintf(L"AssetBundle.LoadAsset(this = %p, name = %ls, type = %ls)\n", _this, name->start_char, utility::conversions::to_string_t(assetCls->name).c_str());
		}

		if (result)
		{
			LocalizeAssets(result, name);
		}
		return result;
	}

	std::unordered_map<void*, Il2CppString*> loadHistory{};

	void* AssetBundle_LoadAssetAsync_orig;
	void* AssetBundle_LoadAssetAsync_hook(void* _this, Il2CppString* name, Il2CppReflectionType* type) {
		auto ret = reinterpret_cast<decltype(AssetBundle_LoadAssetAsync_hook)*>(AssetBundle_LoadAssetAsync_orig)(
			_this, name, type
			);
		loadHistory.emplace(ret, name);
		return ret;
	}

	void* AssetBundleRequest_GetResult_orig;
	void* AssetBundleRequest_GetResult_hook(void* _this) {
		void* result = reinterpret_cast<decltype(AssetBundleRequest_GetResult_hook)*>(AssetBundleRequest_GetResult_orig)(_this);

		if (const auto iter = loadHistory.find(_this); iter != loadHistory.end())
		{
			const auto name = iter->second;
			loadHistory.erase(iter);
				
			const auto cls = il2cpp_symbols::get_class_from_instance(result);

			const auto& bundleHandle = GetBundleHandleByAssetName(name->start_char);
			if (bundleHandle)
			{
				// const auto extraAssetBundle = il2cpp_gchandle_get_target(bundleHandle);
				printf("async_extra: %ls\n", name->start_char);  // TODO 加载报错
				// return reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(extraAssetBundle, name,
				//	static_cast<Il2CppReflectionType*>(il2cpp_class_get_type(cls)));
				
			}

			if (g_asset_load_log) {
				const auto assetCls = static_cast<Il2CppClassHead*>(cls);
				printf("AssetBundleRequest.GetResult at: %p, type = %ls, name: %ls\n", _this, utility::conversions::to_string_t(assetCls->name).c_str(),
					name->start_char);
			}

			LocalizeAssets(result, name);
		}
		
		return result;
	}

	void* AssetBundleRequest_get_allAssets_orig;
	void* AssetBundleRequest_get_allAssets_hook(void* _this) {
		void* ret = reinterpret_cast<decltype(AssetBundleRequest_get_allAssets_hook)*>(AssetBundleRequest_get_allAssets_orig)(_this);

		il2cpp_symbols::iterate_IEnumerable(ret, [&](void* data) {
			printf("get_allAssets at: %p\n", data);
			});

		return ret;
	}


	void* AssetLoader_LoadAssetHandle_orig;
	void* AssetLoader_LoadAssetHandle_hook(void* _this, Il2CppString* path, bool isLowerCase) {
		auto ret = reinterpret_cast<decltype(AssetLoader_LoadAssetHandle_hook)*>(AssetLoader_LoadAssetHandle_orig)(_this, path, isLowerCase);
		return ret;
	}

	Il2CppReflectionType* Font_Type;

	void (*text_assign_font)(void*);
	void (*Text_set_horizontalOverflow)(void* _this, int value);
	void (*Text_set_verticalOverflow)(void* _this, int value);
	void* (*Text_get_font)(void*);
	void (*Text_set_font)(void*, void*);
	int (*text_get_size)(void*);
	void* text_set_size_orig;
	void text_set_size_hook(void* _this, int size)
	{
		return reinterpret_cast<decltype(text_set_size_hook)*>(text_set_size_orig)(_this, size + g_custom_font_size_offset);
	}
	float (*text_get_linespacing)(void*);
	void (*text_set_style)(void*, int);
	void (*text_set_linespacing)(void*, float);
	Il2CppString* (*TextCommon_get_text)(void*);

	uint32_t ReplaceFontHandle;

	bool (*Object_IsNativeObjectAlive)(void*);

	void* TextCommon_Awake_orig;
	void TextCommon_Awake_hook(void* _this)
	{
		reinterpret_cast<decltype(TextCommon_Awake_hook)*>(TextCommon_Awake_orig)(_this);

		void* replaceFont{};
		const auto& bundleHandle = GetBundleHandleByAssetName(std::get<UseCustomFont>(g_replace_font).FontPath);
		if (std::holds_alternative<UseCustomFont>(g_replace_font) && bundleHandle)
		{
			if (ReplaceFontHandle)
			{
				replaceFont = il2cpp_gchandle_get_target(ReplaceFontHandle);
				// 加载场景时会被 Resources.UnloadUnusedAssets 干掉，且不受 DontDestroyOnLoad 影响，暂且判断是否存活，并在必要的时候重新加载
				// TODO: 考虑挂载到 GameObject 上
				// AssetBundle 不会被干掉
				if (Object_IsNativeObjectAlive(replaceFont))
				{
					goto FontAlive;
				}
				else
				{
					il2cpp_gchandle_free(std::exchange(ReplaceFontHandle, 0));
				}
			}

			const auto extraAssetBundle = il2cpp_gchandle_get_target(bundleHandle);
			replaceFont = reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(extraAssetBundle, il2cpp_string_new(std::get<UseCustomFont>(g_replace_font).FontPath.c_str()), Font_Type);
			if (replaceFont)
			{
				ReplaceFontHandle = il2cpp_gchandle_new(replaceFont, false);
			}
			else
			{
				std::wprintf(L"Cannot load asset font\n");
			}
		}

	FontAlive:
		if (replaceFont)
		{
			Text_set_font(_this, replaceFont);
		}
		else
		{
			text_assign_font(_this);
		}

		// auto getText = TextCommon_get_text(_this);
		// logger::write_test_log(getText->start_char);
		// wprintf(L"inGetText: %ls\n", getText->start_char);

		Text_set_horizontalOverflow(_this, 1);
		Text_set_verticalOverflow(_this, 1);
		text_set_style(_this, g_custom_font_style);
		reinterpret_cast<decltype(text_set_size_hook)*>(text_set_size_orig)(_this, text_get_size(_this) + g_custom_font_size_offset);
		text_set_linespacing(_this, g_custom_font_linespacing);
	}

	void* set_resolution_orig;
	void set_resolution_hook(int width, int height, bool fullscreen)
	{
		if (g_force_landscape) {
			if (width < height) {
				const int thd = width;
				width = height;
				height = thd;
			}
		}

		Resolution_t r;
		r = *get_resolution(&r);
		// MessageBoxA(NULL, std::format("window: {}, {}", width, height).c_str(), "TEST", MB_OK);
		auto hWnd = FindWindowW(L"UnityWndClass", L"umamusume");
		if (hWnd != NULL) {
			RECT* now_rect = new RECT();
			if (GetWindowRect(hWnd, now_rect)) {
				if ((now_rect->right - now_rect->left) < (now_rect->bottom - now_rect->top)) {
					vert_cache_rect = now_rect;
					vert_cache_rect_cache = true;
				}
				else {
					land_cache_rect = now_rect;
					land_cache_rect_cache = true;
				}
			}
		}
		else {
			printf("window[2]: UnityWndClass - umamusume not found.\n");
		}

		if (width > height) {
			_set_u_stat(false);  // false-横屏
			// std::wprintf(L"to land: %d * %d\n", width, height);
		}
		else {
			_set_u_stat(true);
			// std::wprintf(L"to virt: %d * %d\n", width, height);
		}

		bool need_fullscreen = false;
		auto screen_ratio = r.width / static_cast<double>(r.height);

		if (g_auto_fullscreen)
		{
			float aspect_ratio = g_aspect_ratio;

			if (!is_virt() && r.width > r.height)
				aspect_ratio = static_cast<float>(r.width) / r.height;
			else if (is_virt() && r.width < r.height)
				aspect_ratio = static_cast<float>(r.height) / r.width;
			else
				aspect_ratio = orig_aspect_ratio;

			g_aspect_ratio = aspect_ratio;
		}

		if (is_virt() && abs(screen_ratio - (1.f / g_aspect_ratio)) <= 0.001 && g_auto_fullscreen)
			need_fullscreen = true;
		else if (!is_virt() && abs(screen_ratio - g_aspect_ratio) <= 0.001 && g_auto_fullscreen)
			need_fullscreen = true;

		reinterpret_cast<decltype(set_resolution_hook)*>(set_resolution_orig)(
			need_fullscreen ? r.width : width, need_fullscreen ? r.height : height, need_fullscreen
			);
		if(!need_fullscreen) recheck_ratio_later(150, true);
	}

	void* set_shadows_orig;
	void set_shadows_hook(void* _this, int value) {
		// printf("set_shadows: %d\n", value);  暂时没用
		return reinterpret_cast<decltype(set_shadows_hook)*>(set_shadows_orig)(_this, value);
	}

	void* set_shadow_resolution_orig;
	void set_shadow_resolution_hook(void* _this, int value) {
		// printf("shadowResolution: %d\n", value);  暂时没用
		return reinterpret_cast<decltype(set_shadow_resolution_hook)*>(set_shadow_resolution_orig)(_this, value);
	}

	void* AlterUpdate_CameraPos_orig;
	void AlterUpdate_CameraPos_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime, int sheetIndex, bool isUseCameraMotion) {
		// printf("frame: %d, index: %d, motion: %s\n", currentFrame, sheetIndex, isUseCameraMotion ? "true" : "false");
		reinterpret_cast<decltype(AlterUpdate_CameraPos_hook)*>(AlterUpdate_CameraPos_orig)(
			_this, sheet, g_live_free_camera ? 0 : currentFrame, currentTime, sheetIndex, isUseCameraMotion);
	}

	void* AlterUpdate_RadialBlur_orig;
	void AlterUpdate_RadialBlur_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_RadialBlur_hook)*>(AlterUpdate_RadialBlur_orig)(
			_this, sheet, 0);

		// printf("AlterUpdate_RadialBlur frame: %d\n", currentFrame);
	}

	void* AlterUpdate_MultiCameraPosition_orig;
	void AlterUpdate_MultiCameraPosition_hook(void* _this, void* sheet, int currentFrame, float currentTime) {
		// printf("UpdMultiPos: %d\n", currentFrame);
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_MultiCameraPosition_hook)*>(AlterUpdate_MultiCameraPosition_orig)(
			_this, sheet, currentFrame, currentTime);
	}

	void* AlterUpdate_MultiCameraLookAt_orig;
	void AlterUpdate_MultiCameraLookAt_hook(void* _this, void* sheet, int currentFrame, float currentTime) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_MultiCameraLookAt_hook)*>(AlterUpdate_MultiCameraLookAt_orig)(
			_this, sheet, currentFrame, currentTime);
	}

	void* AlterUpdate_MultiCameraRadialBlur_orig;
	void AlterUpdate_MultiCameraRadialBlur_hook(void* _this, void* sheet, int frame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_MultiCameraRadialBlur_hook)*>(AlterUpdate_MultiCameraRadialBlur_orig)(
			_this, sheet, frame);

		// printf("AlterUpdate_MultiCameraRadialBlur frame: %d\n", frame);
		// return reinterpret_cast<decltype(AlterUpdate_MultiCameraRadialBlur_hook)*>(AlterUpdate_MultiCameraRadialBlur_orig)(_this, sheet, frame);
	}

	void* SetupRadialBlurInfo_orig;
	void SetupRadialBlurInfo_hook(void* _this, void* updateInfo, void* curData, void* nextData, int currentFrame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(SetupRadialBlurInfo_hook)*>(SetupRadialBlurInfo_orig)(
			_this, updateInfo, curData, nextData, currentFrame);
		// printf("SetupRadialBlurInfo frame: %d\n", currentFrame);
	}

	void* alterupdate_eye_camera_pos_orig;
	void alterupdate_eye_camera_pos_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(alterupdate_eye_camera_pos_hook)*>(alterupdate_eye_camera_pos_orig)(
			_this, sheet, currentFrame, currentTime);
	}

	void* AlterUpdate_FormationOffset_orig;
	void AlterUpdate_FormationOffset_hook(void* _this, void* sheet, int currentFrame, void* characterObjectList, bool changeVisibility) {
		// printf("offset frame: %d\n", currentFrame);  控制角色
		return reinterpret_cast<decltype(AlterUpdate_FormationOffset_hook)*>(AlterUpdate_FormationOffset_orig)(_this, sheet, currentFrame, characterObjectList, g_live_force_changeVisibility_false ? false : changeVisibility);
	}

	void* AlterUpdate_PostEffect_BloomDiffusion_orig;
	void AlterUpdate_PostEffect_BloomDiffusion_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_PostEffect_BloomDiffusion_hook)*>(AlterUpdate_PostEffect_BloomDiffusion_orig)(
			_this, sheet, currentFrame);
	}

	void* get_camera_pos_orig;
	Vector3_t* get_camera_pos_hook(void* retstr, void* _this, Il2CppObject* timelineControl) {
		const bool isFollowUma = (UmaCamera::GetLiveCameraType() == LiveCamera_FOLLOW_UMA) && g_live_free_camera;
		if (isFollowUma) {
			static auto klass = il2cpp_symbols::get_class_from_instance(_this);
			static FieldInfo* setType_field = il2cpp_class_get_field_from_name(klass, "setType");
			if (il2cpp_symbols::read_field<int>(_this, setType_field) != 1) {
				il2cpp_symbols::write_field(_this, setType_field, 1);
			}
		}

		auto pos = reinterpret_cast<decltype(get_camera_pos_hook)*>(get_camera_pos_orig)(retstr, _this, timelineControl);
		if (!g_live_free_camera) {
			return pos;
		}
		// printf("orig_pos: %f, %f, %f\n", pos->x, pos->y, pos->z);
		auto setPos = UmaCamera::getCameraPos();
		UmaCamera::setUmaCameraType(CAMERA_LIVE);
		pos->x = setPos.x;
		pos->y = setPos.y;
		pos->z = setPos.z;
		// printf("pos: %f, %f, %f\n", pos->x, pos->y, pos->z);
		return pos;
	}

	bool getCharaPos = false;

	void* Unity_get_fieldOfView_orig;
	float Unity_get_fieldOfView_hook(void* _this) {
		if (g_set_live_fov_as_global || (isLiveStart && g_live_free_camera)) {
			return UmaCamera::getLiveCamFov();
		}

		if (g_race_free_camera && raceStart) {
			return UmaCamera::getRaceCamFov();
		}

		const auto ret = reinterpret_cast<decltype(Unity_get_fieldOfView_hook)*>(Unity_get_fieldOfView_orig)(_this);
		// printf("get_fov: %f\n", ret);
		return ret;
	}

	void* cacheCont = nullptr;
	bool changeIn = false;
	bool updateRaceCame = false;
	bool updateLiveCameraPos = false;

	void* Unity_set_fieldOfView_orig;
	void Unity_set_fieldOfView_hook(void* _this, float single) {
		return reinterpret_cast<decltype(Unity_set_fieldOfView_hook)*>(Unity_set_fieldOfView_orig)(_this, single);
	}

	void* ChangeCameraWithImmediate_orig;
	void ChangeCameraWithImmediate_hook(void* _this, int nextPos) {
		cacheCont = _this;
		changeIn = true;
		reinterpret_cast<decltype(ChangeCameraWithImmediate_hook)*>(ChangeCameraWithImmediate_orig)(_this, nextPos);
		changeIn = false;
	}

	void race_get_CameraPosition(void* _this, Vector3_t* data);
	void updateTargetPos(Vector3_t* data, Quaternion_t* quat);

	void* Unity_get_pos_injected_orig;
	void Unity_get_pos_injected_hook(void* _this, Vector3_t* data) {
		reinterpret_cast<decltype(Unity_get_pos_injected_hook)*>(Unity_get_pos_injected_orig)(_this, data);
	}

	void* Unity_set_localpos_injected_orig;
	void Unity_set_localpos_injected_hook(void* _this, Vector3_t* ret) {
		if (updateRaceCame) {
			auto pos = UmaCamera::getCameraPos();
			ret->x = pos.x;
			ret->y = pos.y;
			ret->z = pos.z;
			// printf("set_position_Injected_c at: %p (%f, %f, %f)\n", _this, ret->x, ret->y, ret->z);
		}
		return reinterpret_cast<decltype(Unity_set_localpos_injected_hook)*>(Unity_set_localpos_injected_orig)(_this, ret);
	}

	void* Unity_set_rotation_Injected_orig;
	void Unity_set_rotation_Injected_hook(void* _this, Quaternion_t* value) {
		return reinterpret_cast<decltype(Unity_set_rotation_Injected_hook)*>(Unity_set_rotation_Injected_orig)(_this, value);
	}

	Quaternion_t* liveCacheTransform = nullptr;
	Quaternion_t* raceCacheTransform = nullptr;
	void* liveLookCameraCachePtr = nullptr;

	void* Unity_set_pos_injected_orig;
	void Unity_set_pos_injected_hook(void* _this, Vector3_t* ret) {
		if (g_home_free_camera && cacheCont == _this) {
			// printf("set_position_Injected: %f, %f, %f\n", ret->x, ret->y, ret->z);
			auto cam = UmaCamera::getHomeCameraPos();
			ret->x = cam.x;
			ret->y = cam.y;
			ret->z = cam.z;
		}
		if (changeIn) {
			cacheCont = _this;
			// printf("set_position_Injected_c at: %p (%f, %f, %f)\n", _this, ret->x, ret->y, ret->z);
		}
		if (updateRaceCame) {
			UmaCamera::setUmaCameraType(CAMERA_RACE);

			auto pos = UmaCamera::getCameraPos();
			ret->x = pos.x;
			ret->y = pos.y;
			ret->z = pos.z;
			race_get_CameraPosition(_this, ret);
			// printf("set_position_Injected_c at: %p (%f, %f, %f)\n", _this, ret->x, ret->y, ret->z);
		}

		return reinterpret_cast<decltype(Unity_set_pos_injected_hook)*>(Unity_set_pos_injected_orig)(_this, ret);
	}

	void* Unity_LookAt_Injected_orig;
	void Unity_LookAt_Injected_hook(void* _this, Vector3_t* worldPosition, Vector3_t* worldUp) {
		if (updateRaceCame) {
			if (g_race_free_camera && g_race_freecam_follow_umamusume && raceFollowUmaFirstPersion) {
				if (raceCacheTransform != nullptr) {
					Unity_set_rotation_Injected_hook(_this, raceCacheTransform);
					return;
				}
			}
			else {
				auto pos = UmaCamera::getCameraLookat();
				worldPosition->x = pos.x;
				worldPosition->y = pos.y;
				worldPosition->z = pos.z;
			}
		}

		if (updateLiveCameraPos && liveFirstPersonEnableRoll) {
			if (liveCacheTransform != nullptr) {
				if (g_live_free_camera && (UmaCamera::GetLiveCameraType() == LiveCamera_FIRST_PERSION)) {
					// Unity_set_rotation_Injected_hook(_this, liveCacheTransform);
					liveLookCameraCachePtr = _this;
				}
			}
		}

		return reinterpret_cast<decltype(Unity_LookAt_Injected_hook)*>(Unity_LookAt_Injected_orig)(_this, worldPosition, worldUp);
	}

	void* Unity_set_localRotation_Injected_orig;
	void Unity_set_localRotation_Injected_hook(void* _this, Quaternion_t* value) {
		if (updateRaceCame) {
			return;
		}

		return reinterpret_cast<decltype(Unity_set_localRotation_Injected_hook)*>(Unity_set_localRotation_Injected_orig)(_this, value);
	}

	void* Unity_set_nearClipPlane_orig;
	void Unity_set_nearClipPlane_hook(void* _this, float single) {
		if ((g_live_free_camera && isLiveStart && UmaCamera::GetLiveCameraType() == LiveCamera_FIRST_PERSION) ||
			(g_race_free_camera && raceStart)) {
			single = 0.001f;
		}
		return reinterpret_cast<decltype(Unity_set_nearClipPlane_hook)*>(Unity_set_nearClipPlane_orig)(_this, single);
	}

	void* Unity_get_nearClipPlane_orig;
	float Unity_get_nearClipPlane_hook(void* _this) {
		auto ret = reinterpret_cast<decltype(Unity_get_nearClipPlane_hook)*>(Unity_get_nearClipPlane_orig)(_this);
		if (updateRaceCame || (g_race_free_camera && raceStart)) {
			ret = 0.001f;
		}
		return ret;
	}

	void* Unity_set_cullingMask_orig;
	void Unity_set_cullingMask_hook(void* _this, int value) {
		// if (updateRaceCame) printf("set_cullingMask: %d\n", value);
		reinterpret_cast<decltype(Unity_set_cullingMask_hook)*>(Unity_set_cullingMask_orig)(_this, value);
	}

	void* Unity_get_rotation_Injected_orig;
	void Unity_get_rotation_Injected_hook(void* _this, Quaternion_t* ret) {
		reinterpret_cast<decltype(Unity_get_rotation_Injected_hook)*>(Unity_get_rotation_Injected_orig)(_this, ret);
	}
	void* Unity_get_local_rotation_Injected_orig;
	void Unity_get_local_rotation_Injected_hook(void* _this, Quaternion_t* ret) {
		reinterpret_cast<decltype(Unity_get_local_rotation_Injected_hook)*>(Unity_get_local_rotation_Injected_orig)(_this, ret);
	}


	void* Unity_get_farClipPlane_orig;
	float Unity_get_farClipPlane_hook(void* _this) {
		auto ret = reinterpret_cast<decltype(Unity_get_farClipPlane_hook)*>(Unity_get_farClipPlane_orig)(_this);
		if (updateRaceCame || (g_race_free_camera && raceStart)) {
			ret = 2500.0f;
		}
		return ret;
	}

	void* Unity_set_farClipPlane_orig;
	void Unity_set_farClipPlane_hook(void* _this, float value) {
		if ((g_live_free_camera && isLiveStart) || (g_race_free_camera && raceStart)) {
			value = 2500.0f;
		}
		reinterpret_cast<decltype(Unity_set_farClipPlane_hook)*>(Unity_set_farClipPlane_orig)(_this, value);
	}


	void* HomeClampAngle_orig;
	float HomeClampAngle_hook(float value, float min, float max) {
		auto ret = reinterpret_cast<decltype(HomeClampAngle_hook)*>(HomeClampAngle_orig)(
			value, g_home_free_camera ? -180 : min, g_home_free_camera ? 180 : max
			);
		UmaCamera::setHomeCameraAngle(-ret);
		return ret;
	}

	void* MasterCharaType_Get_orig;
	void* MasterCharaType_Get_hook(void* _this, int charaId, int targetScene, int targetCut, int targetType) {
		// printf("MasterCharaType_Get, charaId: %d, targetScene: %d, targetCut: %d, targetType: %d\n", charaId, targetScene, targetCut, targetType);
		if (targetScene + targetCut + targetType == 0) {
			if (g_home_walk_chara_id > 0) {
				charaId = g_home_walk_chara_id;
			}
		}
		return reinterpret_cast<decltype(MasterCharaType_Get_hook)*>(MasterCharaType_Get_orig)(_this, charaId, targetScene, targetCut, targetType);;
	}

	void* CreateMiniCharacter_orig;
	void CreateMiniCharacter_hook(void* _this) {
		if (g_bypass_live_205) return;
		return reinterpret_cast<decltype(CreateMiniCharacter_hook)*>(CreateMiniCharacter_orig)(_this);

	}

	void* FinishDragFreeCamera_orig;
	void FinishDragFreeCamera_hook(void* _this) {
		if (g_home_free_camera) return;
		return reinterpret_cast<decltype(FinishDragFreeCamera_hook)*>(FinishDragFreeCamera_orig)(_this);
	}

	void* UpdateEnvironemntStageFovShift_orig;
	void UpdateEnvironemntStageFovShift_hook(void* _this, void* updateInfo) {
		return reinterpret_cast<decltype(UpdateEnvironemntStageFovShift_hook)*>(UpdateEnvironemntStageFovShift_orig)(
			_this, updateInfo);
	}

	void* alterupdate_camera_lookat_orig;
	void alterupdate_camera_lookat_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime, Vector3_t* outLookAt) {
		isLiveStart = true;
		UmaCamera::setLiveStart(true);
		if (!g_live_free_camera) {
			 return reinterpret_cast<decltype(alterupdate_camera_lookat_hook)*>(alterupdate_camera_lookat_orig)(
				_this, sheet, currentFrame, currentTime, outLookAt);
		}

		if (g_live_free_camera && (UmaCamera::GetLiveCameraType() == LiveCamera_FIRST_PERSION) && liveFirstPersonEnableRoll) return;

		auto setLookat = UmaCamera::getCameraLookat();
		UmaCamera::setUmaCameraType(CAMERA_LIVE);
		outLookAt->x = setLookat.x;
		outLookAt->y = setLookat.y;
		outLookAt->z = setLookat.z;
		// printf("frame: %d, look: %f, %f, %f\n", currentFrame, outLookAt->x, outLookAt->y, outLookAt->z);
	}

	void* AlterUpdate_MonitorCameraLookAt_orig;
	void AlterUpdate_MonitorCameraLookAt_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime) {
		// printf("AlterUpdate_MonitorCameraLookAt frame: %d\n", currentFrame);
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_MonitorCameraLookAt_hook)*>(AlterUpdate_MonitorCameraLookAt_orig)(
				_this, sheet, currentFrame, currentTime);
		}
	}

	void* AlterUpdate_CameraSwitcher_orig;
	void AlterUpdate_CameraSwitcher_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// printf("AlterUpdate_CameraSwitcher frame: %d\n", currentFrame);
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_CameraSwitcher_hook)*>(AlterUpdate_CameraSwitcher_orig)(
				_this, sheet, currentFrame);
		}
	}

	void* LiveTimelineEasing_orig;
	float LiveTimelineEasing_hook(void* type, float t, float b, float c, float d) {
		auto data = reinterpret_cast<decltype(LiveTimelineEasing_hook)*>(LiveTimelineEasing_orig)(
			type, t, b, c, d);
		if (!g_live_free_camera) {
			return data;
		}
		// printf("LiveTimelineEasing: %f\n", data);
		return 0;
	}

	void* AlterUpdate_EyeCameraLookAt_orig;
	void AlterUpdate_EyeCameraLookAt_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime) {
		// printf("AlterUpdate_EyeCameraLookAt frame: %d\n", currentFrame);
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_EyeCameraLookAt_hook)*>(AlterUpdate_EyeCameraLookAt_orig)(
				_this, sheet, currentFrame, currentTime);
		}
	}

	std::unordered_map<int, std::set<void*>> liveDisabledObj{};
	 
	void* live_on_destroy_orig;
	void live_on_destroy_hook(void* _this) {
		printf("Live End!\n");
		isLiveStart = false;
		liveLookCameraCachePtr = nullptr;
		UmaCamera::setLiveStart(g_set_live_fov_as_global);
		UmaCamera::reset_camera();
		liveDisabledObj.clear();
		return reinterpret_cast<decltype(live_on_destroy_hook)*>(live_on_destroy_orig)(_this);
	}

	
	void startUmaGui();

	void* Live_OnClickSettingButton_orig;
	void Live_OnClickSettingButton_hook(void* _this) {
		reinterpret_cast<decltype(Live_OnClickSettingButton_hook)*>(Live_OnClickSettingButton_orig)(_this);
		if (g_enable_live_dof_controller) {
			SetShowLiveWnd(true);
			if (getUmaGuiDone()) {
				startUmaGui();
			}
		}
	}

	void* get_camera_pos2_orig;  // 暂时没用
	Vector3_t* get_camera_pos2_hook(void* _this, Il2CppObject* timelineControl, int type) {
		auto pos = reinterpret_cast<decltype(get_camera_pos2_hook)*>(get_camera_pos2_orig)(_this, timelineControl, type);
		printf("pos2: %f, %f, %f\n", pos->x, pos->y, pos->z);
		if (!g_live_free_camera) {
			return pos;
		}
		auto setPos = UmaCamera::getCameraPos();
		UmaCamera::setUmaCameraType(CAMERA_LIVE);
		pos->x = setPos.x;
		pos->y = setPos.y;
		pos->z = setPos.z;
		return pos;
	}

	void* GetCharacterWorldPos_orig;
	Vector3_t* GetCharacterWorldPos_hook(void* retstr, void* timelineControl, int posFlag, int charaParts, Vector3_t* charaPos, Vector3_t* offset) {
		const bool isFollowUma = (UmaCamera::GetLiveCameraType() == LiveCamera_FOLLOW_UMA) && g_live_free_camera;

		if (isFollowUma) {
			charaParts = UmaCamera::GetLiveCameraCharaParts();
			posFlag = UmaCamera::GetLiveCharaPositionFlag();
			offset->x = 0;
			offset->y = 0;
			offset->z = 0;
			charaPos->x = 0;
			charaPos->y = 0;
			charaPos->z = 0;
		}

		auto ret = reinterpret_cast<decltype(GetCharacterWorldPos_hook)*>(GetCharacterWorldPos_orig)(retstr, timelineControl, posFlag, charaParts, charaPos, offset);
		
		if (isFollowUma) {
			// printf("GetCharacterWorldPos: %d (%f, %f, %f)\n", posFlag, ret->x, ret->y, ret->z);
			UmaCamera::SetCameraLookat(ret->x, ret->y, ret->z);
			// UmaCamera::SetCameraPos(ret->x, ret->y, ret->z + 2.0f, true);
			UmaCamera::updateFollowCameraPosByLookatAndOffset();
		}

		return ret;
	}

	void* (*SetActive)(void*, bool);
	void initRaceView();

	void emplaceIntoDisabledObj(std::unordered_map<int, std::set<void*>>& disabledObj, int index, void* part) {
		if (auto iter = disabledObj.find(index); iter != disabledObj.end()) {
			iter->second.emplace(part);
		}
		else {
			disabledObj.emplace(index, std::set{ part });
		}
	}

	void restoreDisableObj(std::unordered_map<int, std::set<void*>>& disabledObj, int currentIndex, bool forceAll) {
		initRaceView();
		std::set<int> restoredIndex{};

		for (auto& i : disabledObj) {
			if ((i.first == currentIndex) && !forceAll) continue;
			for (auto& obj : i.second) {
				if (Object_IsNativeObjectAlive(obj)) {
					SetActive(obj, true);
				}
			}
			restoredIndex.emplace(i.first);
		}
		for (auto& i : restoredIndex) {
			disabledObj.erase(i);
		}
	}

	template<typename T, typename TF>
	void convertPtrType(T* cvtTarget, TF func_ptr) {
		*cvtTarget = reinterpret_cast<T>(func_ptr);
	}

	void*(*GetCharacterObjectFromPositionId)(void*, int);
	void*(*get_CharacterObjectList)(void*);
	void*(*get_LiveModelControllerArray)(void*);
	void*(*get_ClothAsset)(void*);
	void*(*ClothAsset_get_BustClothAsset)(void*);
	void*(*get_HeadTransform)(void*);
	void(*getTransformPosition)(void*, Vector3_t*);
	void(*getTransformRotation)(void*, Quaternion_t*);
	void(*getTransformLocalRotation)(void*, Quaternion_t*);
	void* (*get_OwnerObject)(void*);
	void* (*get_BodyObject)(void*);
	void* (*GameObject_Find)(Il2CppString*);
	Il2CppString* (*GameObject_get_tag)(void*);
	void* (*GameObject_FindGameObjectsWithTag)(Il2CppString*);
	void* (*Transform_GetParent)(void* _this);
	void* (*get_ObjectTransform)(void*);
	Il2CppString* (*get_ObjectName)(void*);
	int (*get_TransformChildCount)(void*);
	void* (*get_TransformChild)(void*, int);
	void* (*get_TransformGameObject)(void*);
	Vector3_t*(*get_liveCharaHeadPosition)(void*);
	Quaternion_t*(*Quaternion_get_identity)();

	void* CharacterObject_klass;
	void* LiveModelController_klass;
	void* Quaternion_klass;
	void* Vector3_klass;
	FieldInfo* CharaObject_liveModelControllerArray;
	FieldInfo* CharaObject_activeModelIndex;
	FieldInfo* CharaObject_liveCharaInitialPosition;
	FieldInfo* LiveModelController_HeadTransform;
	bool isInitLiveChara = false;

	void initLiveChara() {
		if (isInitLiveChara) return;
		isInitLiveChara = true;
		convertPtrType(&GetCharacterObjectFromPositionId, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.Live", "Director", "GetCharacterObjectFromPositionId", 1));
		convertPtrType(&get_CharacterObjectList, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.Live", "Director", "get_CharacterObjectList", 0));
		convertPtrType(&getTransformPosition, il2cpp_resolve_icall("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&getTransformRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&getTransformLocalRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_localRotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&get_LiveModelControllerArray, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.Live", "CharacterObject", "get_LiveModelControllerArray", 0));
		convertPtrType(&get_ClothAsset, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "ModelController", "get_ClothAsset", 0));
		convertPtrType(&ClothAsset_get_BustClothAsset, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "ClothAsset", "get_BustClothAsset", 0));
		convertPtrType(&get_liveCharaHeadPosition, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.Live", "CharacterObject", "get_liveCharaHeadPosition", 0));
		convertPtrType(&get_HeadTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "LiveModelController", "get_HeadTransform", 0));
		convertPtrType(&get_OwnerObject, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "ModelController", "get_OwnerObject", 0));
		convertPtrType(&get_BodyObject, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "ModelController", "get_BodyObject", 0));
		
		convertPtrType(&get_ObjectTransform, il2cpp_resolve_icall("UnityEngine.GameObject::get_transform()"));
		convertPtrType(&get_TransformChildCount, il2cpp_resolve_icall("UnityEngine.Transform::get_childCount()"));
		convertPtrType(&get_TransformChild, il2cpp_resolve_icall("UnityEngine.Transform::GetChild(System.Int32)"));
		convertPtrType(&get_TransformGameObject, il2cpp_resolve_icall("UnityEngine.Component::get_gameObject()"));
		convertPtrType(&get_ObjectName, il2cpp_resolve_icall("UnityEngine.Object::GetName(UnityEngine.Object)"));
		convertPtrType(&SetActive, il2cpp_resolve_icall("UnityEngine.GameObject::SetActive(System.Boolean)"));
		convertPtrType(&GameObject_Find, il2cpp_resolve_icall("UnityEngine.GameObject::Find(System.String)"));
		convertPtrType(&GameObject_get_tag, il2cpp_resolve_icall("UnityEngine.GameObject::get_tag()"));
		convertPtrType(&GameObject_FindGameObjectsWithTag, il2cpp_resolve_icall("UnityEngine.GameObject::FindGameObjectsWithTag(System.String)"));
		convertPtrType(&Transform_GetParent, il2cpp_resolve_icall("UnityEngine.Transform::GetParent()"));
		CharacterObject_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live", "CharacterObject");
		CharaObject_liveModelControllerArray = il2cpp_class_get_field_from_name(CharacterObject_klass, "_liveModelControllerArray");
		CharaObject_activeModelIndex = il2cpp_class_get_field_from_name(CharacterObject_klass, "_activeModelIndex");
		CharaObject_liveCharaInitialPosition = il2cpp_class_get_field_from_name(CharacterObject_klass, "_liveCharaInitialPosition");
		LiveModelController_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "LiveModelController");
		LiveModelController_HeadTransform = il2cpp_class_get_field_from_name(LiveModelController_klass, "<HeadTransform>k__BackingField");
		Quaternion_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Quaternion");
		Vector3_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Vector3");
	}

	/*
	void printAllParents(void* gameObj) {
		auto objName = get_ObjectName(gameObj);
		auto objTransform = get_ObjectTransform(gameObj);  // UnityEngine.Transform
		printf("Target %ls (%p):\n", objName->start_char, gameObj);

		auto parent = Transform_GetParent(objTransform);  // UnityEngine.Transform
		if (parent == objTransform) {
			return;
		}
		else {
			auto parentObj = get_TransformGameObject(parent);  // UnityEngine.GameObject
			printAllParents(parentObj);
		}
	}

	void* findChildGameObject(void* gameObject, std::wstring name) {
		if (!gameObject) return NULL;

		auto objTransform = get_ObjectTransform(gameObject);  // UnityEngine.Transform
		auto childTransformCount = get_TransformChildCount(objTransform);
		for (int i = 0; i < childTransformCount; i++) {
			auto child = get_TransformChild(objTransform, i);  // UnityEngine.Transform
			auto gameObj = get_TransformGameObject(child);  // UnityEngine.GameObject
			if (gameObj) {
				std::wstring_view childObjName = get_ObjectName(gameObj)->start_char;
				if (childObjName.compare(name) == 0) {
					return gameObj;
				}
			}
		}
		return NULL;
	}
	*/

	void hideHead(std::unordered_map<int, std::set<void*>>& disabledObj, void* modelController, int currentFlag) {
		initLiveChara();

		auto ownerObj = get_OwnerObject(modelController);  // UnityEngine.GameObject
		auto objTransform = get_ObjectTransform(ownerObj);  // UnityEngine.Transform
		auto childTransformCount = get_TransformChildCount(objTransform);
		for (int i = 0; i < childTransformCount; i++) {
			auto child = get_TransformChild(objTransform, i);  // UnityEngine.Transform
			auto gameObj = get_TransformGameObject(child);  // UnityEngine.GameObject
			if (gameObj) {
				std::wstring_view objName = get_ObjectName(gameObj)->start_char;
				if (objName.compare(L"M_Hair") == 0) {
					emplaceIntoDisabledObj(disabledObj, currentFlag, gameObj);
					SetActive(gameObj, false);
				}
				else if (objName.compare(L"M_Face") == 0) {
					emplaceIntoDisabledObj(disabledObj, currentFlag, gameObj);
					SetActive(gameObj, false);
				}
			}
		}
	}

	void* Director_AlterUpdate_orig;
	void Director_AlterUpdate_hook(void* _this) {
		const bool isFirstPersion = (UmaCamera::GetLiveCameraType() == LiveCamera_FIRST_PERSION) && g_live_free_camera;
		reinterpret_cast<decltype(Director_AlterUpdate_hook)*>(Director_AlterUpdate_orig)(_this);

		if (!liveDisabledObj.empty()) {
			restoreDisableObj(liveDisabledObj, 0, true);
		}

		if (!isFirstPersion) return;

		initLiveChara();
		int objCount = 0;
		il2cpp_symbols::iterate_list(get_CharacterObjectList(_this), [&](int32_t i, void* charaObj) {
			objCount++;
			});

		auto currentFlag = UmaCamera::GetLiveCharaPositionIndex();
		if (currentFlag >= objCount) {
			currentFlag = objCount - 1;
		}

		auto charaObject = GetCharacterObjectFromPositionId(_this, currentFlag);  // Gallop.Live.CharacterObject
		auto liveModelControllerArr = reinterpret_cast<void*>(static_cast<std::byte*>(charaObject) + CharaObject_liveModelControllerArray->offset);  // LiveModelController[]
		auto liveModelControllerArray = get_LiveModelControllerArray(charaObject);  // LiveModelController[]
		auto activeModelIndex = il2cpp_symbols::read_field<int>(charaObject, CharaObject_activeModelIndex);

		auto currIndex = 0;
		il2cpp_symbols::iterate_IEnumerable(liveModelControllerArray, [&](void* modelController) {  // Gallop.LiveModelController
			if (!modelController) return;

			// auto headTransform = reinterpret_cast<void**>(static_cast<unsigned char*>(modelController) + LiveModelController_HeadTransform->offset);  // UnityEngine.Transform
			auto headTransform = get_HeadTransform(modelController); // UnityEngine.Transform
			
			if (currIndex == 0) {
				Quaternion_t* rot = reinterpret_cast<Quaternion_t*>(il2cpp_object_new(Quaternion_klass));
				Vector3_t* pos = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));

				liveCacheTransform = rot;
				getTransformPosition(headTransform, pos);
				getTransformRotation(headTransform, rot);
				UmaCamera::updatePosAndLookatByRotation(*pos, *rot);

				// 隐藏头部
				hideHead(liveDisabledObj, modelController, currentFlag);
			}
			currIndex++;
			});
		restoreDisableObj(liveDisabledObj, currentFlag, false);

	}

	bool isCySpringInit = false;
	void* CySpringParamDataElement_klass;
	FieldInfo* CySpringParamDataElement_boneName;
	FieldInfo* CySpringParamDataElement_stiffnessForce;
	FieldInfo* CySpringParamDataElement_dragForce;
	FieldInfo* CySpringParamDataElement_gravity;
	FieldInfo* CySpringParamDataElement_childElements;
	FieldInfo* CySpringParamDataElement_verticalWindRateSlow;
	FieldInfo* CySpringParamDataElement_collisionRadius;
	FieldInfo* CySpringParamDataElement_needEnvCollision;
	FieldInfo* CySpringParamDataElement_horizontalWindRateSlow;
	FieldInfo* CySpringParamDataElement_verticalWindRateFast;
	FieldInfo* CySpringParamDataElement_horizontalWindRateFast;
	FieldInfo* CySpringParamDataElement_isLimit;
	FieldInfo* CySpringParamDataElement_MoveSpringApplyRate;
	void* CySpringParamDataChildElement_klass;
	FieldInfo* CySpringParamDataChildElement_boneName;
	FieldInfo* CySpringParamDataChildElement_stiffnessForce;
	FieldInfo* CySpringParamDataChildElement_dragForce;
	FieldInfo* CySpringParamDataChildElement_gravity;
	FieldInfo* CySpringParamDataChildElement_verticalWindRateSlow;
	FieldInfo* CySpringParamDataChildElement_collisionRadius;
	FieldInfo* CySpringParamDataChildElement_needEnvCollision;
	FieldInfo* CySpringParamDataChildElement_horizontalWindRateSlow;
	FieldInfo* CySpringParamDataChildElement_verticalWindRateFast;
	FieldInfo* CySpringParamDataChildElement_horizontalWindRateFast;
	FieldInfo* CySpringParamDataChildElement_isLimit;
	FieldInfo* CySpringParamDataChildElement_MoveSpringApplyRate;

	int getListCount(void* list);

	void initCySpring() {
		if (isCySpringInit) return;
		isCySpringInit = true;

		CySpringParamDataElement_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "CySpringParamDataElement");
		CySpringParamDataElement_boneName = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_boneName");
		CySpringParamDataElement_stiffnessForce = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_stiffnessForce");
		CySpringParamDataElement_dragForce = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_dragForce");
		CySpringParamDataElement_gravity = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_gravity");
		CySpringParamDataElement_childElements = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_childElements");
		
		CySpringParamDataElement_verticalWindRateSlow = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_verticalWindRateSlow");
		CySpringParamDataElement_collisionRadius = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_collisionRadius");
		CySpringParamDataElement_needEnvCollision = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_needEnvCollision");
		CySpringParamDataElement_horizontalWindRateSlow = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_horizontalWindRateSlow");
		CySpringParamDataElement_verticalWindRateFast = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_verticalWindRateFast");
		CySpringParamDataElement_horizontalWindRateFast = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_horizontalWindRateFast");
		CySpringParamDataElement_isLimit = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_isLimit");
		CySpringParamDataElement_MoveSpringApplyRate = il2cpp_class_get_field_from_name(CySpringParamDataElement_klass, "_MoveSpringApplyRate");

		CySpringParamDataChildElement_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "CySpringParamDataChildElement");
		CySpringParamDataChildElement_boneName = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_boneName");
		CySpringParamDataChildElement_stiffnessForce = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_stiffnessForce");
		CySpringParamDataChildElement_dragForce = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_dragForce");
		CySpringParamDataChildElement_gravity = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_gravity");
		
		CySpringParamDataChildElement_verticalWindRateSlow = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_verticalWindRateSlow");
		CySpringParamDataChildElement_collisionRadius = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_collisionRadius");
		CySpringParamDataChildElement_needEnvCollision = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_needEnvCollision");
		CySpringParamDataChildElement_horizontalWindRateSlow = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_horizontalWindRateSlow");
		CySpringParamDataChildElement_verticalWindRateFast = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_verticalWindRateFast");
		CySpringParamDataChildElement_horizontalWindRateFast = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_horizontalWindRateFast");
		CySpringParamDataChildElement_isLimit = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_isLimit");
		CySpringParamDataChildElement_MoveSpringApplyRate = il2cpp_class_get_field_from_name(CySpringParamDataChildElement_klass, "_MoveSpringApplyRate");

	}

	void* CySpring_CreateBone_orig;
	void CySpring_CreateBone_hook(void* _this, void* boneList, void* element, float legacyScale, float windPhaseShift, bool isAdd) {

		if (UmaGUiShowData::isEnableUmaBone) {
			initCySpring();

			auto boneNameStr = il2cpp_symbols::read_field<Il2CppString*>(element, CySpringParamDataElement_boneName);
			std::wstring boneName = boneNameStr->start_char;

			for (const auto& i : UmaGUiShowData::umaBoneData) {
				if (!i.enabled) continue;
				const boost::wregex expr(utility::conversions::to_string_t(i.reg), boost::regex_constants::icase);

				if (boost::regex_search(boneName, expr)) {

					auto stiffnessForce = il2cpp_symbols::read_field<float>(element, CySpringParamDataElement_stiffnessForce);
					auto dragForce = il2cpp_symbols::read_field<float>(element, CySpringParamDataElement_dragForce);
					auto gravity = il2cpp_symbols::read_field<float>(element, CySpringParamDataElement_gravity);

					if (i.replace_stiffnessForce) il2cpp_symbols::write_field(element, CySpringParamDataElement_stiffnessForce, i.stiffnessForce);
					if (i.replace_dragForce) il2cpp_symbols::write_field(element, CySpringParamDataElement_dragForce, i.dragForce);
					if (i.replace_gravity) il2cpp_symbols::write_field(element, CySpringParamDataElement_gravity, i.gravity);
					if (i.replace_verticalWindRateSlow) il2cpp_symbols::write_field(element, CySpringParamDataElement_verticalWindRateSlow, i.verticalWindRateSlow);
					if (i.replace_collisionRadius) il2cpp_symbols::write_field(element, CySpringParamDataElement_collisionRadius, i.collisionRadius);
					if (i.replace_needEnvCollision) il2cpp_symbols::write_field(element, CySpringParamDataElement_needEnvCollision, i.needEnvCollision);
					if (i.replace_horizontalWindRateSlow) il2cpp_symbols::write_field(element, CySpringParamDataElement_horizontalWindRateSlow, i.horizontalWindRateSlow);
					if (i.replace_verticalWindRateFast) il2cpp_symbols::write_field(element, CySpringParamDataElement_verticalWindRateFast, i.verticalWindRateFast);
					if (i.replace_horizontalWindRateFast) il2cpp_symbols::write_field(element, CySpringParamDataElement_horizontalWindRateFast, i.horizontalWindRateFast);
					if (i.replace_isLimit) il2cpp_symbols::write_field(element, CySpringParamDataElement_isLimit, i.isLimit);
					if (i.replace_MoveSpringApplyRate) il2cpp_symbols::write_field(element, CySpringParamDataElement_MoveSpringApplyRate, i.MoveSpringApplyRate);

					auto childElements = il2cpp_symbols::read_field(element, CySpringParamDataElement_childElements);

					printf("%ls: orig stiffnessForce=%f, dragForce=%f, gravity=%f, childCount=%d\n", boneName.data(), stiffnessForce, dragForce, gravity, getListCount(childElements));

					il2cpp_symbols::iterate_list(childElements, [&](size_t index, void* childElement) {
						auto boneNameStr = il2cpp_symbols::read_field<Il2CppString*>(childElement, CySpringParamDataChildElement_boneName);
						std::wstring boneName = boneNameStr->start_char;
						if (boost::regex_search(boneName, expr)) {
							/*
							auto stiffnessForce = il2cpp_symbols::read_field<float>(childElement, CySpringParamDataChildElement_stiffnessForce);
							auto dragForce = il2cpp_symbols::read_field<float>(childElement, CySpringParamDataChildElement_dragForce);
							auto gravity = il2cpp_symbols::read_field<float>(childElement, CySpringParamDataChildElement_gravity);
							printf("child - %ls: orig stiffnessForce=%f, dragForce=%f, gravity=%f\n", boneName.data(), stiffnessForce, dragForce, gravity);
							*/

							if (i.replace_stiffnessForce) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_stiffnessForce, i.stiffnessForce);
							if (i.replace_dragForce) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_dragForce, i.dragForce);
							if (i.replace_gravity) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_gravity, i.gravity);
							if (i.replace_verticalWindRateSlow) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_verticalWindRateSlow, i.verticalWindRateSlow);
							if (i.replace_collisionRadius) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_collisionRadius, i.collisionRadius);
							if (i.replace_needEnvCollision) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_needEnvCollision, i.needEnvCollision);
							if (i.replace_horizontalWindRateSlow) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_horizontalWindRateSlow, i.horizontalWindRateSlow);
							if (i.replace_verticalWindRateFast) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_verticalWindRateFast, i.verticalWindRateFast);
							if (i.replace_horizontalWindRateFast) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_horizontalWindRateFast, i.horizontalWindRateFast);
							if (i.replace_isLimit) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_isLimit, i.isLimit);
							if (i.replace_MoveSpringApplyRate) il2cpp_symbols::write_field(childElement, CySpringParamDataChildElement_MoveSpringApplyRate, i.MoveSpringApplyRate);
						}
						});
				}
			}
		}

		reinterpret_cast<decltype(CySpring_CreateBone_hook)*>(CySpring_CreateBone_orig)(_this, boneList, element, legacyScale, windPhaseShift, isAdd);
	}

	void* CySpringController_UpdateSpringThread_orig;
	void CySpringController_UpdateSpringThread_hook(void* _this) {
		if (g_enable_better60fps) {
			static auto klass = il2cpp_symbols::get_class_from_instance(_this);
			static auto set_UpdateMode = il2cpp_class_get_method_from_name(klass, "set_UpdateMode", 1);
			if (set_UpdateMode) {
				reinterpret_cast<void(*)(void*, int)>(set_UpdateMode->methodPointer)(_this, 1);
			}
			else {
				static FieldInfo* updateModeField = il2cpp_class_get_field_from_name(klass, "<UpdateMode>k__BackingField");
				il2cpp_symbols::write_field(_this, updateModeField, 1);
			}
		}
		reinterpret_cast<decltype(CySpringController_UpdateSpringThread_hook)*>(CySpringController_UpdateSpringThread_orig)(_this);
	}


	int(*HorseData_get_GateNo)(void*);
	float(*get_RunMotionRate)(void*);
	int(*get_RaceMotion)(void*);
	int(*get_SingleModeTeamRank)(void*);
	Il2CppString*(*get_TrainerName)(void*);
	void(*InitTrainerName)(void*);
	Il2CppString*(*get_charaName)(void*);
	float(*get_RaceBaseSpeed)(void*);
	float(*get_MinSpeed)(void*);
	float(*get_StartDashSpeedThreshold)(void*);
	bool(*get_IsOverRun)(void*);
	float(*GetHp)(void*);
	float(*GetMaxHp)(void*);
	float(*GetHpPer)(void*);
	int(*get_NearHorseCount)(void*);
	float(*get_CongestionTime)(void*);
	int(*get_RawSpeed)(void*);
	int(*get_BaseSpeed)(void*);
	float(*get_Speed)(void*);
	int(*get_RawStamina)(void*);
	float(*get_BaseStamina)(void*);
	float(*get_Stamina)(void*);
	int(*get_RawPow)(void*);
	float(*get_BasePow)(void*);
	float(*get_Pow)(void*);
	int(*get_RawGuts)(void*);
	float(*get_BaseGuts)(void*);
	float(*get_Guts)(void*);
	int(*get_RawWiz)(void*);
	float(*get_BaseWiz)(void*);
	float(*get_Wiz)(void*);
	float(*get_DeltaTime)();
	bool(*get_IsStartDash)(void*);
	int(*GetActivateSkillCount)(void*);
	int(*get_FinishOrder)(void*);
	// int(*CalcChallengeMatchPointTotal)(void*);
	float(*get_MoveDistance)(void*);

	bool(*get_IsLastSpurt)(void*);
	float(*get_LastSpurtStartDistance)(void*);

	void* HorseRaceInfo_klass;
	FieldInfo* HorseRaceInfo_baseSpeedAdjusted;
	FieldInfo* HorseRaceInfo_baseStaminaAdjusted;
	FieldInfo* HorseRaceInfo_basePowAdjusted;
	FieldInfo* HorseRaceInfo_baseGutsAdjusted;
	FieldInfo* HorseRaceInfo_baseWizAdjusted;
	FieldInfo* HorseRaceInfo_lastSpeed;
	FieldInfo* HorseRaceInfo_distance;
	FieldInfo* HorseRaceInfo_position;
	FieldInfo* HorseRaceInfo_rotationOnLane;

	bool initHorseDataFuncInited = false;

	void initHorseDataFunc() {
		if (initHorseDataFuncInited) return;
		initHorseDataFuncInited = true;

		convertPtrType(&HorseData_get_GateNo, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseData", "get_GateNo", 0));
		convertPtrType(&get_SingleModeTeamRank, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseData", "get_SingleModeTeamRank", 0));
		convertPtrType(&get_TrainerName, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseData", "get_TrainerName", 0));
		convertPtrType(&InitTrainerName, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseData", "InitTrainerName", 0));
		convertPtrType(&get_charaName, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseData", "get_charaName", 0));
		convertPtrType(&get_RunMotionRate, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "get_RunMotionRate", 0));
		convertPtrType(&get_RaceMotion, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "get_RaceMotion", 0));
		convertPtrType(&get_IsLastSpurt, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "get_IsLastSpurt", 0));
		convertPtrType(&get_LastSpurtStartDistance, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "get_LastSpurtStartDistance", 0));
		convertPtrType(&get_FinishOrder, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "get_FinishOrder", 0));
		// convertPtrType(&CalcChallengeMatchPointTotal, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfoReplay", "CalcChallengeMatchPointTotal", 0));

		convertPtrType(&get_RaceBaseSpeed, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RaceBaseSpeed", 0));
		convertPtrType(&get_MinSpeed, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_MinSpeed", 0));
		convertPtrType(&get_StartDashSpeedThreshold, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_StartDashSpeedThreshold", 0));
		convertPtrType(&get_IsOverRun, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_IsOverRun", 0));
		convertPtrType(&GetHp, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "GetHp", 0));
		convertPtrType(&GetMaxHp, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "GetMaxHp", 0));
		convertPtrType(&GetHpPer, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "GetHpPer", 0));
		convertPtrType(&get_NearHorseCount, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_NearHorseCount", 0));
		convertPtrType(&get_CongestionTime, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_CongestionTime", 0));
		convertPtrType(&get_RawSpeed, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RawSpeed", 0));
		convertPtrType(&get_BaseSpeed, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_BaseSpeed", 0));
		convertPtrType(&get_Speed, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_Speed", 0));
		convertPtrType(&get_RawStamina, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RawStamina", 0));
		convertPtrType(&get_BaseStamina, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_BaseStamina", 0));
		convertPtrType(&get_Stamina, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_Stamina", 0));
		convertPtrType(&get_RawPow, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RawPow", 0));
		convertPtrType(&get_BasePow, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_BasePow", 0));
		convertPtrType(&get_Pow, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_Pow", 0));
		convertPtrType(&get_RawGuts, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RawGuts", 0));
		convertPtrType(&get_BaseGuts, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_BaseGuts", 0));
		convertPtrType(&get_Guts, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_Guts", 0));
		convertPtrType(&get_RawWiz, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_RawWiz", 0));
		convertPtrType(&get_BaseWiz, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_BaseWiz", 0));
		convertPtrType(&get_Wiz, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_Wiz", 0));
		convertPtrType(&get_IsStartDash, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_IsStartDash", 0));
		convertPtrType(&GetActivateSkillCount, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "GetActivateSkillCount", 0));
		convertPtrType(&get_MoveDistance, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "HorseRaceInfo", "get_MoveDistance", 0));
		convertPtrType(&get_DeltaTime, il2cpp_resolve_icall("UnityEngine.Time::get_unscaledDeltaTime()"));

		HorseRaceInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "HorseRaceInfo");
		HorseRaceInfo_baseSpeedAdjusted = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_baseSpeedAdjusted");
		HorseRaceInfo_baseStaminaAdjusted = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_baseStaminaAdjusted");
		HorseRaceInfo_basePowAdjusted = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_basePowAdjusted");
		HorseRaceInfo_baseGutsAdjusted = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_baseGutsAdjusted");
		HorseRaceInfo_baseWizAdjusted = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_baseWizAdjusted");
		HorseRaceInfo_lastSpeed = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_lastSpeed");
		HorseRaceInfo_distance = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_distance");
		HorseRaceInfo_position = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_position");
		HorseRaceInfo_rotationOnLane = il2cpp_class_get_field_from_name(HorseRaceInfo_klass, "_rotationOnLane");  // _rotationFacingFront
	}

	void* get_RunMotionSpeed_orig;
	float get_RunMotionSpeed_hook(void* _this) {
		raceStart = true;
		auto ret = reinterpret_cast<decltype(get_RunMotionSpeed_hook)*>(get_RunMotionSpeed_orig)(_this);
		if (enableRaceInfoTab || (g_race_free_camera && g_race_freecam_follow_umamusume)) {
			initHorseDataFunc();
			if (const auto iter = umaRaceData.find(_this); iter != umaRaceData.end())
			{
				bool isFinished = get_IsOverRun(_this);
				iter->second.UpdateMotionData(ret, get_RunMotionRate(_this), get_RaceBaseSpeed(_this), get_MinSpeed(_this),
					get_StartDashSpeedThreshold(_this), isFinished, GetHp(_this), GetMaxHp(_this), GetHpPer(_this),
					get_NearHorseCount(_this), get_CongestionTime(_this), get_RawSpeed(_this),
					il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_baseSpeedAdjusted),
					get_Speed(_this),
					get_RawStamina(_this), il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_baseStaminaAdjusted),
					get_Stamina(_this), get_RawPow(_this),
					il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_basePowAdjusted),
					get_Pow(_this), get_RawGuts(_this),
					il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_baseGutsAdjusted),
					get_Guts(_this), get_RawWiz(_this),
					il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_baseWizAdjusted),
					get_Wiz(_this), get_IsStartDash(_this), GetActivateSkillCount(_this),
					il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_lastSpeed),
					get_MoveDistance(_this), il2cpp_symbols::read_field<float>(_this, HorseRaceInfo_distance),
					get_DeltaTime(), get_LastSpurtStartDistance(_this), get_IsLastSpurt(_this));
				if (isFinished) iter->second.setFinallyRank(get_FinishOrder(_this) + 1);

				if (g_race_free_camera && g_race_freecam_follow_umamusume) {
					if (iter->second.gateNo == g_race_freecam_follow_umamusume_index + 1) {
						const auto fieldPtr = static_cast<std::byte*>(_this) + HorseRaceInfo_position->offset;
						auto cameraPos = reinterpret_cast<Vector3_t*>(fieldPtr);

						const auto qFieldPtr = static_cast<std::byte*>(_this) + HorseRaceInfo_rotationOnLane->offset;
						auto cameraRot = reinterpret_cast<Quaternion_t*>(qFieldPtr);
						// printf("cameraRot: w: %f, x: %f, y: %f, z: %f\n", cameraRot->w, cameraRot->x, cameraRot->y, cameraRot->z);
						
						updateTargetPos(cameraPos, cameraRot);
					}
				}
			}
		}

		return ret;
	}

	void startUmaGui() {
		if (guiStarting) return;
		guiStarting = true;
		thread([]() {
			printf("GUI START\n");
			guimain();
			guiStarting = false;
			printf("GUI END\n");
			SetShowLiveWnd(false);
			}).detach();
	}

	void* HorseRaceInfoReplay_ctor_orig;
	void HorseRaceInfoReplay_ctor_hook(void* _this, void* data, void* reader) {
		// data: Gallop.HorseData, reader: Gallop.RaceSimulateReader
		reinterpret_cast<decltype(HorseRaceInfoReplay_ctor_hook)*>(HorseRaceInfoReplay_ctor_orig)(_this, data, reader);

		if (enableRaceInfoTab || (g_race_free_camera && g_race_freecam_follow_umamusume)) {
			initHorseDataFunc();
			SetShowRaceWnd(true);
			if (enableRaceInfoTab && getUmaGuiDone()) {
				startUmaGui();
			}
			InitTrainerName(data);
			auto tName = get_TrainerName(data);
			auto umadata = UmaGUiShowData::UmaRaceMotionData(
				HorseData_get_GateNo(data), get_charaName(data)->start_char, tName ? tName->start_char : L""
			);
			umaRaceData.emplace(_this, umadata);
		}

	}

	void* (*GetSkill)(void*, int);
	int (*GetSkillLevel)(void*);
	void* (*GetSkilDetails)(void*);
	Il2CppString* (*get_SkillEffectName)(void*);
	void* (*getSkill_Abilities)(void*);
	void* (*get_SkillMaster)(void*);
	void (*SkillActivate)(void*);
	float (*get_DefaultCoolDownTime)(void*);
	void* skillManager_klass;
	FieldInfo* skillManager_ownerInfo;
	bool isSkillManaInited = false;

	void initSkillManager() {
		if (isSkillManaInited)return;
		isSkillManaInited = true;

		skillManager_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "SkillManager");
		skillManager_ownerInfo = il2cpp_class_get_field_from_name(skillManager_klass, "_ownerInfo");
		convertPtrType(&GetSkill, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillManager", "GetSkill", 1));
		convertPtrType(&GetSkillLevel, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillBase", "get_Level", 0));
		convertPtrType(&GetSkilDetails, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillBase", "get_Details", 0));
		convertPtrType(&get_SkillMaster, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillBase", "get_SkillMaster", 0));
		convertPtrType(&getSkill_Abilities, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillDetail", "get_Abilities", 0));
		convertPtrType(&get_SkillEffectName, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillDetail", "get_SkillEffectName", 0));
		convertPtrType(&SkillActivate, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillDetail", "Activate", 0));
		convertPtrType(&get_DefaultCoolDownTime, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "SkillDetail", "get_DefaultCoolDownTime", 0));
	}

	void* AddUsedSkillId_orig;
	void AddUsedSkillId_hook(void* _this, int skillId) {
		reinterpret_cast<decltype(AddUsedSkillId_hook)*>(AddUsedSkillId_orig)(_this, skillId);

		if (enableRaceInfoTab) {
			initSkillManager();
			// Gallop.IHorseRaceInfo
			void* raceInfo = il2cpp_symbols::read_field(_this, skillManager_ownerInfo);

			if (const auto iter = umaRaceData.find(raceInfo); iter != umaRaceData.end()) {
				auto& horseName = iter->second.charaName;
				auto realTName = iter->second.trainerName.empty() ? "" : std::format(" ({})", iter->second.trainerName);
				auto gateNo = iter->second.gateNo;
				auto skillBaseDetails = GetSkill(_this, skillId);  // Gallop.SkillBase
				auto skillLevel = GetSkillLevel(skillBaseDetails);
				auto skilDetails = GetSkilDetails(skillBaseDetails);  // List`1<class Gallop.ISkillDetail>
				auto skillMaster = get_SkillMaster(skillBaseDetails);  // SkillData

				Il2CppString* (*get_SkillName)(void*);
				auto skillData_klass = il2cpp_symbols::get_class_from_instance(skillMaster);
				auto skillData_get_Name = il2cpp_class_get_method_from_name(skillData_klass, "get_Name", 0);
				FieldInfo* skillRarity_field = il2cpp_class_get_field_from_name(skillData_klass, "Rarity");
				FieldInfo* skillGradeValue_field = il2cpp_class_get_field_from_name(skillData_klass, "GradeValue");

				convertPtrType(&get_SkillName, skillData_get_Name->methodPointer);
				const auto skillRarity = il2cpp_symbols::read_field<int>(skillMaster, skillRarity_field);
				const auto skillGradeValue = il2cpp_symbols::read_field<int>(skillMaster, skillGradeValue_field);

				UmaGUiShowData::SkillEventData skillData(skillLevel, horseName, realTName, gateNo, skillRarity, skillGradeValue);
				auto skillName = get_SkillName(skillMaster);
				if (skillName) {
					skillData.SetSkillName(utility::conversions::to_utf8string(skillName->start_char));
				}

				il2cpp_symbols::iterate_list(skilDetails, [&](int32_t index, void* skillDetail) {
					SkillActivate(skillDetail);

					auto abilities = getSkill_Abilities(skillDetail);  // List`1<class Gallop.ISkillAbility>
					auto defCooldownTime = get_DefaultCoolDownTime(skillDetail);
					skillData.updateCoolDownTime(defCooldownTime);

					il2cpp_symbols::iterate_list(abilities, [&](int32_t cIndex, void* skillAbility) {
						int (*get_AbilityType)(void*);
						float (*get_AbilityValueOnActivate)(void*);
						void* (*get_AbilityTargets)(void*);

						auto klass = il2cpp_symbols::get_class_from_instance(skillAbility);
						convertPtrType(&get_AbilityType, il2cpp_class_get_method_from_name(klass, "get_AbilityType", 0)->methodPointer);
						convertPtrType(&get_AbilityValueOnActivate, il2cpp_class_get_method_from_name(klass, "get_AbilityValueOnActivate", 0)->methodPointer);
						convertPtrType(&get_AbilityTargets, il2cpp_class_get_method_from_name(klass, "get_AbilityTargets", 0)->methodPointer);

						const auto abilityType = get_AbilityType(skillAbility);
						const auto effValue = get_AbilityValueOnActivate(skillAbility);
						const auto abilityTargets = get_AbilityTargets(skillAbility);  // List`1<class Gallop.IHorseRaceInfo>

						UmaGUiShowData::SkillAbility mSkillability(abilityType, effValue);

						il2cpp_symbols::iterate_list(abilityTargets, [&](int32_t aIndex, void* fRaceInfo) {
							if (const auto iter = umaRaceData.find(fRaceInfo); iter != umaRaceData.end())
							{
								mSkillability.addTargets(iter->second);
								// printf("Targets: %d\n", iter->second.gateNo);
							}
							});

						// printf("abilityType: 0x%x, effValue: %f\n", abilityType, effValue);
						skillData.addSkillAbilities(mSkillability);
						});

					});

				umaUsedSkillList.emplace_back(skillData);
			}
		}
		
	}

	void* AlterUpdate_CameraLayer_orig;
	void AlterUpdate_CameraLayer_hook(void* _this, void* sheet, int currentFrame, Vector3_t* offsetMaxPosition, Vector3_t* offsetMinPosition) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_CameraLayer_hook)*>(AlterUpdate_CameraLayer_orig)(
			_this, sheet, currentFrame, offsetMaxPosition, offsetMinPosition
		);
	}

	void* AlterUpdate_TiltShift_orig;
	void AlterUpdate_TiltShift_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_TiltShift_hook)*>(AlterUpdate_TiltShift_orig)(_this, sheet, currentFrame);
		// return true;
	}

	void* AlterUpdate_CameraMotion_orig;
	bool AlterUpdate_CameraMotion_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// if (g_live_free_camera) return true;
		// printf("frame_motion: %d\n", currentFrame);
		return reinterpret_cast<decltype(AlterUpdate_CameraMotion_hook)*>(AlterUpdate_CameraMotion_orig)(_this, sheet, currentFrame);
		// return true;
	}

	void* AlterLateUpdate_CameraMotion_orig;
	bool AlterLateUpdate_CameraMotion_hook(void* _this, Il2CppObject* sheet, int currentFrame) {  // main
		// printf("frame_late_motion: %d\n", currentFrame);
		return reinterpret_cast<decltype(AlterLateUpdate_CameraMotion_hook)*>(AlterLateUpdate_CameraMotion_orig)(_this, sheet, currentFrame);
		// return true;
	}

	void* AlterUpdate_CameraFov_orig;
	bool AlterUpdate_CameraFov_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_CameraFov_hook)*>(AlterUpdate_CameraFov_orig)(_this, sheet, currentFrame);
		}
		// printf("updateFOV: %d\n", currentFrame);
		return true;
	}

	void* AlterUpdate_CameraRoll_orig;
	bool AlterUpdate_CameraRoll_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_CameraRoll_hook)*>(AlterUpdate_CameraRoll_orig)(_this, sheet, currentFrame);
		}
		// printf("updateRoll: %d\n", currentFrame);
		return true;
	}

	void* LiveTimelineControl_AlterLateUpdate_orig;
	void LiveTimelineControl_AlterLateUpdate_hook(void* _this) {
		updateLiveCameraPos = true;
		reinterpret_cast<decltype(LiveTimelineControl_AlterLateUpdate_hook)*>(LiveTimelineControl_AlterLateUpdate_orig)(_this);
		updateLiveCameraPos = false;
		if (g_live_free_camera && liveFirstPersonEnableRoll && (UmaCamera::GetLiveCameraType() == LiveCamera_FIRST_PERSION)) {
			if ((liveLookCameraCachePtr != nullptr) && (liveCacheTransform != nullptr)) {
				Unity_set_rotation_Injected_hook(liveLookCameraCachePtr, liveCacheTransform);
			}
		}
	}

	void* AlterUpdate_MultiCamera_orig;
	bool AlterUpdate_MultiCamera_hook(void* _this, Il2CppObject* sheet, int currentFrame, float currentTime) {
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_MultiCamera_hook)*>(AlterUpdate_MultiCamera_orig)(
				_this, sheet, currentFrame, currentTime);
		}
		// printf("updateMultiCamera: %d\n", currentFrame);
		return true;
	}

	void* GallopUtil_GetModifiedString_orig;
	Il2CppString* GallopUtil_GetModifiedString_hook(Il2CppString* text, void* input, bool allowNewLine)
	{
		return text;
	}

	void adjust_size()
	{
		thread([]() {
			auto tr = il2cpp_thread_attach(il2cpp_domain_get());

			Resolution_t r;
			get_resolution_stub(&r);

			auto target_height = r.height - 100;
			if (start_width == -1 && start_height == -1)
				set_resolution_hook(target_height * g_aspect_ratio, target_height, false);

			il2cpp_thread_detach(tr);
			}).detach();
	}

	void* load_scene_internal_orig = nullptr;
	void* load_scene_internal_hook(Il2CppString* sceneName, int sceneBuildIndex, void* parameters, bool mustCompleteNextFrame)
	{
		wprintf(L"%ls\n", sceneName->start_char);
		return reinterpret_cast<decltype(load_scene_internal_hook)*>(load_scene_internal_orig)(sceneName, sceneBuildIndex, parameters, mustCompleteNextFrame);
	}

	Vector3_t targetPosLastCache{};
	Vector3_t targetPosCache{};
	Quaternion_t currentQuat{};
	void race_get_CameraPosition(void* _this, Vector3_t* data) {
		UmaCamera::setUmaCameraType(CAMERA_RACE);

		if (g_race_freecam_follow_umamusume && !raceFollowUmaFirstPersion) {
			UmaCamera::updateFollowUmaPos(targetPosLastCache, targetPosCache, currentQuat, data);
			return;
		}
		/*
		auto pos = UmaCamera::getCameraPos();
		data->x = pos.x;
		data->y = pos.y;
		data->z = pos.z;
		*/
	}

	void* RaceCameraManager_AlterLateUpdate_orig;
	void RaceCameraManager_AlterLateUpdate_hook(void* _this) {
		updateRaceCame = true && g_race_free_camera;
		reinterpret_cast<decltype(RaceCameraManager_AlterLateUpdate_hook)*>(RaceCameraManager_AlterLateUpdate_orig)(_this);
		updateRaceCame = false;
	}

	void* race_ChangeCameraMode_orig;
	void race_ChangeCameraMode_hook(void* _this, int mode, bool isSkip) {
		// printf("ChangeCameraMode: %d, %d\n", mode, isSkip);
		if (g_race_free_camera) return;
		return reinterpret_cast<decltype(race_ChangeCameraMode_hook)*>(race_ChangeCameraMode_orig)(_this, 0, true);
	}

	void* race_get_CameraFov_orig;
	float race_get_CameraFov_hook(void* _this) {
		if (!g_race_free_camera) return reinterpret_cast<decltype(race_get_CameraFov_hook)*>(race_get_CameraFov_orig)(_this);
		return UmaCamera::getRaceCamFov();
	}
	
	void* race_PlayEventCamera_orig;
	bool race_PlayEventCamera_hook(void* _this, int p1, int p2, int p3, bool p4, bool p5) {
		if (g_race_free_camera) return false;

		return reinterpret_cast<decltype(race_PlayEventCamera_hook)*>(race_PlayEventCamera_orig)(_this, p1, p2, p3, p4, p5);
	}

	void* race_UpdateCameraDistanceBlendRate_orig;
	void race_UpdateCameraDistanceBlendRate_hook(void* _this, void* p1, void* p2, void* p3) {
		if (g_race_free_camera) return;
		reinterpret_cast<decltype(race_UpdateCameraDistanceBlendRate_hook)*>(race_UpdateCameraDistanceBlendRate_orig)(_this, p1, p2, p3);
		// printf("UpdateCameraDistanceBlendRate\n");
	}

	void* (*RaceViewBase_GetModelController)(void*, int);
	void* (*Race_get_HeadTransform)(void*);
	void* (*GetPrefabAttachTransform)(void*, int);
	void* (*get_HeadObject)(void*);
	void* (*GetCancelTransform)(void*, int);
	void (*getTransformLocalPosition)(void*, Vector3_t*);
	bool raceViewInited = false;

	void initRaceView() {
		if (raceViewInited) return;
		raceViewInited = true;

		Quaternion_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Quaternion");
		Vector3_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Vector3");

		convertPtrType(&RaceViewBase_GetModelController, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "RaceViewBase", "GetModelController", 1));
		convertPtrType(&Race_get_HeadTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "RaceModelController", "get_HeadTransform", 0));
		convertPtrType(&GetPrefabAttachTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "RaceModelController", "GetPrefabAttachTransform", 1));
		convertPtrType(&get_HeadObject, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "RaceModelController", "get_HeadObject", 0));
		convertPtrType(&get_OwnerObject, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "ModelController", "get_OwnerObject", 0));
		convertPtrType(&GetCancelTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "RaceModelController", "GetCancelTransform", 1));
		convertPtrType(&getTransformPosition, il2cpp_resolve_icall("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&getTransformRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&getTransformLocalRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_localRotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&getTransformLocalPosition, il2cpp_resolve_icall("UnityEngine.Transform::get_localPosition_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&SetActive, il2cpp_resolve_icall("UnityEngine.GameObject::SetActive(System.Boolean)"));
		convertPtrType(&get_ObjectTransform, il2cpp_resolve_icall("UnityEngine.GameObject::get_transform()"));
		convertPtrType(&get_TransformChildCount, il2cpp_resolve_icall("UnityEngine.Transform::get_childCount()"));
		convertPtrType(&get_TransformChild, il2cpp_resolve_icall("UnityEngine.Transform::GetChild(System.Int32)"));
		convertPtrType(&get_TransformGameObject, il2cpp_resolve_icall("UnityEngine.Component::get_gameObject()"));
		convertPtrType(&get_ObjectName, il2cpp_resolve_icall("UnityEngine.Object::GetName(UnityEngine.Object)"));
	}

	std::unordered_map<int, std::set<void*>> raceDisabledObj{};

	void* RaceViewBase_LateUpdateView_orig;
	void RaceViewBase_LateUpdateView_hook(void* _this) {
		int currentIndex = g_race_freecam_follow_umamusume_index;

		if (g_race_free_camera && g_race_freecam_follow_umamusume && raceFollowUmaFirstPersion) {
			initRaceView();
			auto modelController = RaceViewBase_GetModelController(_this, currentIndex);
			if (modelController) {
				// auto headTransform = Race_get_HeadTransform(modelController);  // 并不能获取完整的Head Transform
				// auto headTransform = GetPrefabAttachTransform(modelController, 0x6);
				auto eyeLTransform = GetPrefabAttachTransform(modelController, 0x7);
				auto eyeRTransform = GetPrefabAttachTransform(modelController, 0x8);
				// auto headTransform = GetCancelTransform(modelController, 0x3);
				// 
				// auto headObject = get_HeadObject(modelController);  // UnityEngine.GameObject
				// SetActive(headObject, false);

				hideHead(raceDisabledObj, modelController, currentIndex);

				Quaternion_t* rot = reinterpret_cast<Quaternion_t*>(il2cpp_object_new(Quaternion_klass));
				Vector3_t* pos = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));
				Quaternion_t* rot2 = reinterpret_cast<Quaternion_t*>(il2cpp_object_new(Quaternion_klass));
				Vector3_t* pos2 = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));

				getTransformPosition(eyeLTransform, pos);
				getTransformRotation(eyeLTransform, rot);
				getTransformPosition(eyeRTransform, pos2);
				getTransformRotation(eyeRTransform, rot2);
				pos->x = (pos2->x + pos->x) / 2;
				pos->y = (pos2->y + pos->y) / 2;
				pos->z = (pos2->z + pos->z) / 2;
				auto newSRot = UmaCamera::slerpTwo(*rot, *rot2, 0.5f);
				rot->w = newSRot.w;
				rot->x = newSRot.x;
				rot->y = newSRot.y;
				rot->z = newSRot.z;

				Quaternion_t newRot;
				if (raceFollowUmaFirstPersionEnableRoll) {
					newRot = UmaCamera::updatePosAndLookatByRotation(*pos, *rot);
				}
				else {
					newRot = UmaCamera::updatePosAndLookatByRotationStable(*pos, *rot, currentQuat);
				}

				rot->w = newRot.w;
				rot->x = newRot.x;
				rot->y = newRot.y;
				rot->z = newRot.z;
				raceCacheTransform = rot;

				// UmaCamera::SetCameraPos(pos->x, pos->y, pos->z);
			}
		}
		restoreDisableObj(raceDisabledObj, currentIndex, !raceFollowUmaFirstPersion);
		reinterpret_cast<decltype(RaceViewBase_LateUpdateView_hook)*>(RaceViewBase_LateUpdateView_orig)(_this);
	}

	void* race_get_CameraShakeTargetOffset_orig;
	Vector3_t* race_get_CameraShakeTargetOffset_hook(void* _this) {
		auto data = reinterpret_cast<decltype(race_get_CameraShakeTargetOffset_hook)*>(race_get_CameraShakeTargetOffset_orig)(_this);
		// printf("shake: %d, %d, %d\n", data->x, data->y, data->z);
		if (!g_race_free_camera) return data;
		data->x = 0;
		data->y = 0;
		data->z = 0;
		return data;
	}

	void updateTargetPos(Vector3_t* data, Quaternion_t* quat) {
		auto y = data->y + 1;
		currentQuat = Quaternion_t(*quat);
		if (g_race_freecam_follow_umamusume) {
			if ((targetPosCache.x != data->x) || (targetPosCache.z != data->z)) {
				targetPosLastCache = Vector3_t{ targetPosCache.x, targetPosCache.y, targetPosCache.z };
			}
			targetPosCache = Vector3_t{ data->x, y, data->z };
		}
	}

	void* race_GetTargetRotation_orig;
	Quaternion_t* race_GetTargetRotation_hook(void* _this, int targetIndex, bool isClipTargetTransOnGoal) {
		auto data = reinterpret_cast<decltype(race_GetTargetRotation_hook)*>(race_GetTargetRotation_orig)(_this, targetIndex, isClipTargetTransOnGoal);
		// printf("rot: %f, %f, %f, %f\n", data->w, data->x, data->y, data->z);
		return data;
		if ((!g_race_free_camera) || (!g_race_freecam_follow_umamusume)) return data;
		data->w = 0;
		data->x = 0;
		data->y = 0;
		data->z = 0;
		return data;
	}

	void* race_OnDestroy_orig;
	void race_OnDestroy_hook(void* _this) {
		reinterpret_cast<decltype(race_OnDestroy_hook)*>(race_OnDestroy_orig)(_this);
		printf("Race End!\n");
		raceStart = false;
		umaRaceData.clear();
		umaUsedSkillList.clear();
		raceDisabledObj.clear();
		// updateRaceGUIData(umaRaceData);
		SetShowRaceWnd(false);
		if (!raceInfoTabAttachToGame) {
			if (closeWhenRaceEnd) {
				SetShowRaceWnd(false);
				SetGuiDone(true);
			}
		}

		UmaCamera::reset_camera();
		targetPosLastCache = Vector3_t{};
		targetPosCache = Vector3_t{};
	}

	enum class UmaControllerType {
		Default = 0x0,
		Race = 0x1,
		Training = 0x2,
		EventTimeline = 0x3,
		Live = 0x4,
		LiveTheater = 0x5,
		HomeStand = 0x6,
		HomeTalk = 0x7,
		HomeWalk = 0x8,
		CutIn = 0x9,
		TrainingTop = 0xa,
		SingleRace = 0xb,
		Simple = 0xc,
		Mini = 0xd,
		Paddock = 0xe,
		Champions = 0xf,
		ORIG = 0x1919810
	};

	std::unordered_set<UmaControllerType> otherReplaceTypes {
		UmaControllerType::Training, 
		UmaControllerType::EventTimeline, 
		UmaControllerType::Live, 
		UmaControllerType::LiveTheater,
		UmaControllerType::HomeStand,
		UmaControllerType::TrainingTop,
		UmaControllerType::Simple,
		UmaControllerType::Champions,
		UmaControllerType::SingleRace,
		UmaControllerType::CutIn,
		UmaControllerType::Race,
		UmaControllerType::Paddock,
		UmaControllerType::ORIG
	};
	bool enableLoadCharLog = false;

	bool replaceCharController(int *charaId, int *dressId, int* headId, UmaControllerType controllerType) {
		bool replaceDress = true;
		if ((*dressId < 100000) && !g_global_char_replace_Universal) {
			replaceDress = false;
		}

		if (g_enable_home_char_replace && (controllerType == UmaControllerType::HomeStand)) {  // HomeStand
			if (*charaId == 9001) {  // Can't replace this at home now.
				return false;
			}
			if (g_home_char_replace.contains(*charaId)) {
				auto* replaceChar = &g_home_char_replace.at(*charaId);
				*charaId = replaceChar->first;
				*dressId = replaceChar->second;
				*headId = UmaDatabase::get_head_id_from_dress_id(*dressId);
				return true;
			}
		}

		if (g_enable_global_char_replace && (controllerType == UmaControllerType::Mini)) {  // mini
			if (g_global_mini_char_replace.contains(*charaId)) {
				auto* replaceChar = &g_global_mini_char_replace.at(*charaId);
				if (UmaDatabase::get_dress_have_mini(replaceChar->second)) {
					*charaId = replaceChar->first;
					if (replaceDress) *dressId = replaceChar->second;
					*headId = UmaDatabase::get_head_id_from_dress_id(*dressId);
					return true;
				}
				else {
					printf("dressId: %d does not have mini character!\n", replaceChar->second);
					return false;
				}
			}
			if (!UmaDatabase::get_dress_have_mini(*dressId)) {
				printf("dressId: %d does not have mini character! Replace to 2.\n", *dressId);
				*dressId = 2;
				return true;
			}
		}

		if (g_enable_global_char_replace && otherReplaceTypes.contains(controllerType)) {
			if ((*charaId == 9001) && (controllerType == UmaControllerType::HomeStand)) {  // Can't replace this at home now.
				return false;
			}
			if (g_global_char_replace.contains(*charaId)) {
				auto* replaceChar = &g_global_char_replace.at(*charaId);
				*charaId = replaceChar->first;
				if (replaceDress) *dressId = replaceChar->second;
				*headId = UmaDatabase::get_head_id_from_dress_id(*dressId);
				return true;
			}
		}
		return false;
	}

	bool replaceCharController(int *cardId, int *charaId, int *dressId, int* headId, UmaControllerType controllerType) {
		if (otherReplaceTypes.contains(controllerType)) {
			if (replaceCharController(charaId, dressId, headId, controllerType)) {
				if (*cardId >= 1000) {
					if ((*cardId / 100) != *charaId) {
						*cardId = *charaId * 100 + 1;
					}
				}
				return true;
			}
		}
		return false;
	}

	void* (*PushDialog)(void*);  // arg: class Data data  return: class Gallop.DialogCommon
	void* (*SetSimpleNoButtonMessage)(void*, Il2CppString*, Il2CppString*);  // DialogCommon.Data
	void* (*PushErrorCommon)(Il2CppString*, Il2CppString*, void* onCloseButtonPushed, int popupType);
	void* (*SetSimpleOneButtonMessage)(void*, Il2CppString*, Il2CppString*, void* onClickCenterButton, int closeTextId, int dialogFormType);
	void* (*SetSimpleTwoButtonMessage)(void*, Il2CppString*, Il2CppString*, void* onRight, int leftTextId, int rightTextId, void* onLeft, int dialogFormType);
	void* (*SetSimpleThreeButtonMessage)(void*, Il2CppString*, Il2CppString*, void* onRight, int rightTextId, void* onCenter, int centerTextId, void* onLeft, int leftTextId, int dialogFormType);
	void* (*MainThreadDispatcher_get_Instance)();
	void* (*MainThreadDispatcher_EnqueEvent)(void*, void*);
	void (*Data_ctor)(void*);
	void (*WebViewManager_open)(void*, Il2CppString* url, void* dialogData, void* onLoadedCallback, void* onErrorCallback, bool isDirect);
	void (*WebViewManager_ctor)(void*);
	void (*WebViewManager_OpenWebView)(void*, Il2CppString* url, void* onLoadedCallback);
	void* WebViewManager_klass;
	void* Data_klass;
	bool dialogInited = false;

	std::wstring nextLocal1675 = L"";
	int nextLocal1675Btn1 = -1;
	int nextLocal1675Btn2 = -1;
	bool WebViewManager_opening = false;

	void* SetSimpleTwoButtonMessagef_orig;
	void* SetSimpleTwoButtonMessagef_hook(void* _this, Il2CppString* title, Il2CppString* content, void* onRight, int leftTextId, int rightTextId, void* onLeft, int dialogFormType) {
		if (WebViewManager_opening) {
			if (!nextLocal1675.empty()) {
				content = il2cpp_symbols::NewWStr(nextLocal1675);
				nextLocal1675 = L"";
			}
			leftTextId = nextLocal1675Btn1 == -1 ? leftTextId : nextLocal1675Btn1;
			rightTextId = nextLocal1675Btn2 == -1 ? rightTextId : nextLocal1675Btn2;
		}
		return reinterpret_cast<decltype(SetSimpleTwoButtonMessagef_hook)*>(SetSimpleTwoButtonMessagef_orig)(_this, title, content, onRight, leftTextId, rightTextId, onLeft, dialogFormType);
	}

	void initDialog() {
		if (dialogInited) return;
		dialogInited = true;
		convertPtrType(&PushDialog, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "DialogManager", "PushDialog", 1));
		convertPtrType(&PushErrorCommon, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "DialogManager", "PushErrorCommon", 4));
		convertPtrType(&MainThreadDispatcher_get_Instance, il2cpp_symbols::get_method_pointer("Cute.Core.Assembly.dll", "Cute.Core", "MainThreadEventDispatcher", "get_Instance", 0));
		convertPtrType(&MainThreadDispatcher_EnqueEvent, il2cpp_symbols::get_method_pointer("Cute.Core.Assembly.dll", "Cute.Core", "MainThreadEventDispatcher", "EnqueEvent", 1));
		convertPtrType(&WebViewManager_open, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "WebViewManager", "Open", 5));
		convertPtrType(&WebViewManager_ctor, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "WebViewManager", ".ctor", 0));
		convertPtrType(&WebViewManager_OpenWebView, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "WebViewManager", "OpenWebView", 2));
		WebViewManager_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "WebViewManager");
		
		void* DialogCommon_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "DialogCommon");
		Data_klass = il2cpp_symbols::find_nested_class_from_name(DialogCommon_klass, "Data");
		convertPtrType(&SetSimpleNoButtonMessage, il2cpp_class_get_method_from_name(Data_klass, "SetSimpleNoButtonMessage", 2)->methodPointer);
		convertPtrType(&SetSimpleOneButtonMessage, il2cpp_class_get_method_from_name(Data_klass, "SetSimpleOneButtonMessage", 5)->methodPointer);
		convertPtrType(&SetSimpleTwoButtonMessage, il2cpp_class_get_method_from_name(Data_klass, "SetSimpleTwoButtonMessage", 7)->methodPointer);
		convertPtrType(&SetSimpleThreeButtonMessage, il2cpp_class_get_method_from_name(Data_klass, "SetSimpleThreeButtonMessage", 9)->methodPointer);
		convertPtrType(&Data_ctor, il2cpp_class_get_method_from_name(Data_klass, ".ctor", 0)->methodPointer);
	
	}


	void showDialogf(Il2CppString* title, Il2CppString* content, int buttonCount, int button1Text, int button2Text, int button3Text, int btn_type = -1) {
		initDialog();
		
		static auto func = [](Il2CppString* title, Il2CppString* content, int buttonCount, int button1Text, int button2Text, int button3Text, int btn_type = -1) {
			auto data_instance = il2cpp_object_new(Data_klass);
			Data_ctor(data_instance);

			std::wstring s = content->start_char;
			std::wregex pattern(L"<tlgURL>(.*?)</tlgURL>");
			std::wsmatch result;
			if (std::regex_search(s, result, pattern)) {
				std::wstring url = result[1].str();
				s = std::regex_replace(s, pattern, L"");
				content = il2cpp_symbols::NewWStr(s);
				nextLocal1675 = s;
				nextLocal1675Btn1 = button1Text;
				nextLocal1675Btn2 = button2Text;

				SetSimpleTwoButtonMessage(data_instance, title, content, nullptr, button1Text, button2Text, nullptr, 0x2);
				auto webview_instance = il2cpp_object_new(WebViewManager_klass);
				WebViewManager_ctor(webview_instance);
				WebViewManager_opening = true;
				WebViewManager_open(webview_instance, il2cpp_symbols::NewWStr(url), data_instance, nullptr, nullptr, false);
				WebViewManager_opening = false;
				// WebViewManager_OpenWebView(webview_instance, il2cpp_string_new("https://umamusume.jp"), nullptr);
				return;
			}

			switch (buttonCount) {
			case 0: {
				SetSimpleNoButtonMessage(data_instance, title, content);
			}; break;
			case 1: {
				SetSimpleOneButtonMessage(data_instance, title, content, nullptr, button1Text, btn_type == -1 ? 0x1 : btn_type);
			}; break;
			case 2: {
				SetSimpleTwoButtonMessage(data_instance, title, content, nullptr, button1Text, button2Text, nullptr, btn_type == -1 ? 0x2 : btn_type);
			}; break;
			case 3: {
				SetSimpleThreeButtonMessage(data_instance, title, content, nullptr, button1Text, nullptr, button2Text, nullptr, button3Text, btn_type == -1 ? 0x3 : btn_type);
			}; break;
			default: {
				PushErrorCommon(title, content, NULL, btn_type == -1 ? 0 : btn_type);
			}; break;
			}
			// auto method = il2cpp_class_get_method_from_name(Data_klass, ".ctor", 0);
			// il2cpp_runtime_invoke(method, data_instance, NULL, NULL);
			PushDialog(data_instance);
		};

		SchedulingFuncs<void, Il2CppString*, Il2CppString*, int, int, int, int, int>::addToDispatcher(func, title, content, buttonCount, button1Text, button2Text, button3Text, btn_type);

		/*
		auto call_func = []() {
			auto data_instance = il2cpp_object_new(Data_klass);
			Data_ctor(data_instance);
			// auto method = il2cpp_class_get_method_from_name(Data_klass, ".ctor", 0);
			// il2cpp_runtime_invoke(method, data_instance, NULL, NULL);
			auto data = SetSimpleNoButtonMessage(data_instance, il2cpp_string_new("你好"), il2cpp_string_new("哈哈哈"));
			PushDialog(data_instance);
		};
		
		auto inst = MainThreadDispatcher_get_Instance();
		printf("inst at %p\n", inst);

		void* params[2]{ nullptr, &call_func };
		auto Action_ctor_method = il2cpp_symbols::get_method("mscorlib.dll", "System", "Action", ".ctor", 2);
		auto aKlass = il2cpp_symbols::get_class("mscorlib.dll", "System", "Action");
		auto actionInstance = il2cpp_object_new(aKlass);
		il2cpp_runtime_invoke(Action_ctor_method, actionInstance, params, NULL);
		MainThreadDispatcher_EnqueEvent(inst, actionInstance);
		*/
	}

	void* UpdateDispatcher_orig;
	void UpdateDispatcher_hook(void* _this) {
		SchedulingFuncs<void, Il2CppString*, Il2CppString*, int, int, int, int, int>::callAllFunctions();
		return reinterpret_cast<decltype(UpdateDispatcher_hook)*>(UpdateDispatcher_orig)(_this);
	}

	/*
	void* TimelineCharaBuildInfo_klass;
	FieldInfo* TimelineCharaBuildInfo_CharaId;
	FieldInfo* TimelineCharaBuildInfo_CardId;
	FieldInfo* TimelineCharaBuildInfo_DressId;
	FieldInfo* TimelineCharaBuildInfo_DressColorId;
	FieldInfo* TimelineCharaBuildInfo_HeadId;
	FieldInfo* TimelineCharaBuildInfo_MobId;
	FieldInfo* TimelineCharaBuildInfo_ZekkenNumber;
	FieldInfo* TimelineCharaBuildInfo_TrackIndex;

	void initTimelineCharaBuildInfo() {
		auto TimelineCharacterController_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "TimelineCharacterController");
		TimelineCharaBuildInfo_klass = il2cpp_symbols::find_nested_class_from_name(TimelineCharacterController_klass, "TimelineCharaBuildInfo");
		TimelineCharaBuildInfo_CharaId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "CharaId");
		TimelineCharaBuildInfo_CardId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "CardId");
		TimelineCharaBuildInfo_DressId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "DressId");
		TimelineCharaBuildInfo_DressColorId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "DressColorId");
		TimelineCharaBuildInfo_HeadId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "HeadId");
		TimelineCharaBuildInfo_MobId = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "MobId");
		TimelineCharaBuildInfo_ZekkenNumber = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "ZekkenNumber");
		TimelineCharaBuildInfo_TrackIndex = il2cpp_class_get_field_from_name(TimelineCharaBuildInfo_klass, "TrackIndex");
	}

	void* StorySceneController_LoadCharacter_orig;
	void StorySceneController_LoadCharacter_hook(void* _this, void* info, bool isReload) {
		auto charaId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_CharaId);
		auto CardId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_CardId);
		auto DressId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_DressId);
		auto DressColorId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_DressColorId);
		auto HeadId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_HeadId);
		auto MobId = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_MobId);
		auto ZekkenNumber = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_ZekkenNumber);
		auto TrackIndex = il2cpp_symbols::read_field<int>(info, TimelineCharaBuildInfo_TrackIndex);

		
		printf("StorySceneController_LoadCharacter CardId: %d charaId: %d DressId: %d DressColorId: %d HeadId: %d MobId: %d ZekkenNumber: %d TrackIndex: %d\n",
			CardId, charaId, DressId, DressColorId, HeadId, MobId, ZekkenNumber, TrackIndex);
		
		if (replaceCharController(&CardId, &charaId, &DressId, 0x1919810)) {
			il2cpp_symbols::write_field(info, TimelineCharaBuildInfo_CardId, CardId);
			il2cpp_symbols::write_field(info, TimelineCharaBuildInfo_CharaId, charaId);
			il2cpp_symbols::write_field(info, TimelineCharaBuildInfo_DressId, DressId);
		}
		

		return reinterpret_cast<decltype(StorySceneController_LoadCharacter_hook)*>(StorySceneController_LoadCharacter_orig)(_this, info, isReload);
	}
	*/

	void* StoryCharacter3D_LoadModel_orig;
	void StoryCharacter3D_LoadModel_hook(int charaId, int cardId, int clothId, int zekkenNumber, int headId, bool isWet, 
		bool isDirt, int mobId, int dressColorId, Il2CppString* zekkenName, int zekkenFontStyle, int color, int fontColor, 
		int suitColor, bool isUseDressDataHeadModelSubId, bool useCircleShadow) {

		if (enableLoadCharLog) printf("StoryCharacter3D_LoadModel CardId: %d charaId: %d DressId: %d DressColorId: %d HeadId: %d MobId: %d ZekkenNumber: %d\n",
			cardId, charaId, clothId, dressColorId, headId, mobId, zekkenNumber);

		 replaceCharController(&cardId, &charaId, &clothId, &headId, UmaControllerType::ORIG);

		return reinterpret_cast<decltype(StoryCharacter3D_LoadModel_hook)*>(StoryCharacter3D_LoadModel_orig)(
			charaId, cardId, clothId, zekkenNumber, headId, isWet,
			isDirt, mobId, dressColorId, zekkenName, zekkenFontStyle, color, fontColor,
			suitColor, isUseDressDataHeadModelSubId, useCircleShadow);
	}

	void* SingleModeSceneController_CreateModel_orig;
	void* SingleModeSceneController_CreateModel_hook(void* _this, int cardId, int dressId, bool addVoiceCue) {
		if (enableLoadCharLog) printf("SingleModeSceneController_CreateModel cardId: %d, dressId: %d\n", cardId, dressId);
		return reinterpret_cast<decltype(SingleModeSceneController_CreateModel_hook)*>(SingleModeSceneController_CreateModel_orig)(
			_this, cardId, dressId, addVoiceCue);
	}

	void* CharacterBuildInfo_ctor_0_orig;
	void CharacterBuildInfo_ctor_0_hook(void* _this, int charaId, int dressId, int controllerType, int headId,
		int zekken, int mobId, int backDancerColorId, bool isUseDressDataHeadModelSubId, int audienceId,
		int motionDressId, bool isEnableModelCache)
	{
		if (enableLoadCharLog) printf("CharacterBuildInfo_ctor_0 charaId: %d, dressId: %d, headId: %d, controllerType: 0x%x\n", charaId, dressId, headId, controllerType);
		replaceCharController(&charaId, &dressId, &headId, (UmaControllerType)controllerType);
		return reinterpret_cast<decltype(CharacterBuildInfo_ctor_0_hook)*>(CharacterBuildInfo_ctor_0_orig)(_this, charaId, dressId, controllerType, headId, zekken, mobId, backDancerColorId, isUseDressDataHeadModelSubId, audienceId, motionDressId, isEnableModelCache);
	}

	void* CharacterBuildInfo_ctor_1_orig;
	void CharacterBuildInfo_ctor_1_hook(void* _this, int cardId, int charaId, int dressId, int controllerType,
		int headId, int zekken, int mobId, int backDancerColorId, int overrideClothCategory,
		bool isUseDressDataHeadModelSubId, int audienceId, int motionDressId, bool isEnableModelCache)
	{
		if (enableLoadCharLog) printf("CharacterBuildInfo_ctor_1 cardId: %d, charaId: %d, dressId: %d, headId: %d, audienceId: %d, motionDressId: %d, controllerType: 0x%x\n", cardId, charaId, dressId, headId, audienceId, motionDressId, controllerType);
		replaceCharController(&charaId, &dressId, &headId, (UmaControllerType)controllerType);
		return reinterpret_cast<decltype(CharacterBuildInfo_ctor_1_hook)*>(CharacterBuildInfo_ctor_1_orig)(_this, cardId, charaId, dressId, controllerType, headId, zekken, mobId, backDancerColorId, overrideClothCategory, isUseDressDataHeadModelSubId, audienceId, motionDressId, isEnableModelCache);
	}

	void* CharacterBuildInfo_Rebuild_orig;
	void CharacterBuildInfo_Rebuild_hook(void* _this) {
		static void* this_klass = il2cpp_symbols::get_class_from_instance(_this);
		static FieldInfo* cardIdField = il2cpp_class_get_field_from_name(this_klass, "_cardId");
		static FieldInfo* charaIdField = il2cpp_class_get_field_from_name(this_klass, "_charaId");
		static FieldInfo* dressIdField = il2cpp_class_get_field_from_name(this_klass, "_dressId");
		static FieldInfo* controllerTypeField = il2cpp_class_get_field_from_name(this_klass, "_controllerType");
		static FieldInfo* headModelSubIdField = il2cpp_class_get_field_from_name(this_klass, "_headModelSubId");
		static FieldInfo* motionDressIdField = il2cpp_class_get_field_from_name(this_klass, "_motionDressId");
		/*
		static FieldInfo* zekkenField = il2cpp_class_get_field_from_name(this_klass, "_zekken");
		static FieldInfo* mobIdField = il2cpp_class_get_field_from_name(this_klass, "_mobId");
		static FieldInfo* audienceIdField = il2cpp_class_get_field_from_name(this_klass, "_audienceId");
		static FieldInfo* isEnableModelCacheField = il2cpp_class_get_field_from_name(this_klass, "_isEnableModelCache");
		FieldInfo* overrideClothCategoryField = il2cpp_class_get_field_from_name(this_klass, "_overrideClothCategory");
		FieldInfo* backDancerColorIdField = il2cpp_class_get_field_from_name(this_klass, "_backDancerColorId");
		FieldInfo* isUseDressDataHeadModelSubIdField = il2cpp_class_get_field_from_name(this_klass, "_isUseDressDataHeadModelSubId");
		FieldInfo* isEnableModelCacheField = il2cpp_class_get_field_from_name(this_klass, "_isEnableModelCache");
		*/

		auto charaId = il2cpp_symbols::read_field<int>(_this, charaIdField);
		auto cardId = il2cpp_symbols::read_field<int>(_this, cardIdField);
		auto dressId = il2cpp_symbols::read_field<int>(_this, dressIdField);
		auto controllerType = il2cpp_symbols::read_field<int>(_this, controllerTypeField);
		auto headModelSub = il2cpp_symbols::read_field<int>(_this, headModelSubIdField);

		/*
		auto zekken = il2cpp_symbols::read_field<int>(_this, zekkenField);
		auto mobId = il2cpp_symbols::read_field<int>(_this, mobIdField);
		auto audienceId = il2cpp_symbols::read_field<int>(_this, audienceIdField);
		auto isEnableModelCache = il2cpp_symbols::read_field<bool>(_this, isEnableModelCacheField);
		auto overrideClothCategory = il2cpp_symbols::read_field<int>(_this, overrideClothCategoryField);
		auto backDancerColorId = il2cpp_symbols::read_field<int>(_this, backDancerColorIdField);
		auto isUseDressDataHeadModelSubId = il2cpp_symbols::read_field<bool>(_this, isUseDressDataHeadModelSubIdField);
		auto motionDressId = il2cpp_symbols::read_field<int>(_this, motionDressIdField);
		auto isEnableModelCache = il2cpp_symbols::read_field<bool>(_this, isEnableModelCacheField);
		*/

		// printf("ReBuild charaId: %d (%d), card: %d, head: %d, controllerType: 0x%x, zekken: %d, mobId: %d, audienceId: %d, isEnableModelCache: %d\n", charaId,
		//	dressId, cardId, headModelSub, controllerType, zekken, mobId, audienceId, isEnableModelCache);
		if (replaceCharController(&charaId, &dressId, &headModelSub, (UmaControllerType)controllerType)) {
			il2cpp_symbols::write_field(_this, charaIdField, charaId);
			il2cpp_symbols::write_field(_this, dressIdField, dressId);
			il2cpp_symbols::write_field(_this, headModelSubIdField, headModelSub);
			il2cpp_symbols::write_field(_this, motionDressIdField, dressId);
			il2cpp_symbols::write_field(_this, cardIdField, -1);
		}

		reinterpret_cast<decltype(CharacterBuildInfo_Rebuild_hook)*>(CharacterBuildInfo_Rebuild_orig)(_this);
	}

	void* GetRaceDressId_orig;
	int GetRaceDressId_hook(void* _this, bool isApplyDressChange) {
		auto ret = reinterpret_cast<decltype(GetRaceDressId_hook)*>(GetRaceDressId_orig)(_this, false);
		// printf("GetRaceDressId: %d, applyChange: %d\n", ret, isApplyDressChange);
		if (g_enable_global_char_replace) {
			if ((ret > 100000) && (ret <= 999999)) {
				int charaId;
				if (ret / 10000 == 90) {
					charaId = ret % 10000;
				}
				else {
					charaId = ret / 100;
				}
				int newDressId = ret;
				int newHeadId = 0;
				if (replaceCharController(&charaId, &newDressId, &newHeadId, UmaControllerType::ORIG)) {
					return newDressId;
				}
			}
		}
		return ret;
	}
	
	/*
	void* CharacterBg_Setup_orig;
	void CharacterBg_Setup_hook(void* _this, Il2CppString* envPath, Il2CppString* bgPath, int charaId, int dressId, bool playIdle, bool isVerticalBg, bool isRaceUI, bool skipSetBgDefaultPos, void* externalModel) {
		printf("CharacterBg_Setup: chara: %d, dress: %d\n", charaId, dressId);
		reinterpret_cast<decltype(CharacterBg_Setup_hook)*>(CharacterBg_Setup_orig)(_this, envPath, bgPath, charaId, dressId, playIdle, isVerticalBg, isRaceUI, skipSetBgDefaultPos, externalModel);
		printf("CharacterBg_Setup end\n");
	}
	*/

	void* GotoTitleOnError_orig;
	void GotoTitleOnError_hook(Il2CppString* text) {
		if (g_enable_global_char_replace) {
			// wprintf(L"GotoTitleOnError: %ls\n%ls\n\n", text->start_char, environment_get_stacktrace()->start_char);
			return;
		}
		return reinterpret_cast<decltype(GotoTitleOnError_hook)*>(GotoTitleOnError_orig)(text);
	}

	void* EditableCharacterBuildInfo_ctor_orig;
	void EditableCharacterBuildInfo_ctor_hook(void* _this, int cardId, int charaId, int dressId, int controllerType, int zekken, int mobId, int backDancerColorId, int headId, bool isUseDressDataHeadModelSubId, bool isEnableModelCache) {
		if (enableLoadCharLog) printf("EditableCharacterBuildInfo_ctor cardId: %d, charaId: %d, dressId: %d, headId: %d, controllerType: 0x%x\n", cardId, charaId, dressId, headId, controllerType);
		replaceCharController(&cardId, &charaId, &dressId, &headId, (UmaControllerType)controllerType);
		return reinterpret_cast<decltype(EditableCharacterBuildInfo_ctor_hook)*>(EditableCharacterBuildInfo_ctor_orig)(_this, cardId, charaId, dressId, controllerType, zekken, mobId, backDancerColorId, headId, isUseDressDataHeadModelSubId, isEnableModelCache);
	}

	void* get_ApplicationServerUrl_orig;
	Il2CppString* get_ApplicationServerUrl_hook() {
		const auto url = reinterpret_cast<decltype(get_ApplicationServerUrl_hook)*>(get_ApplicationServerUrl_orig)();
		const string new_url(g_self_server_url.begin(), g_self_server_url.end());
		return g_enable_self_server ? il2cpp_string_new(new_url.c_str()) : url;
	}

	void* (*CutIn_get_MotionCamera)(void* _this);
	void* (*CutIn_Camera_get_targetTransform)(void* _this);
	void* (*CutIn_Camera_get_targetCamera)(void* _this);
	void* (*CutIn_Camera_get_transformObject)(void* _this);
	void (*Internal_LookAt_Injected)(void* _this, Vector3_t* pos, Vector3_t* up);
	void* (*CutIn_GetCharacterModelController)(void* _this, int chraIndex, bool isCheckListCount);
	void* (*CutIn_GetCharacters)(void* _this, bool isCheckListCount);
	void* (*GetHeadTransform)(void* _this);
	void* (*Transform_SetParent)(void* _this, void*, bool);
	void* (*Unity_Transform_get_localScale_Injected)(void* _this, Vector3_t*);
	void* (*Unity_Transform_set_localScale_Injected)(void* _this, Vector3_t*);

	bool is_cutin_inited = false;

	void init_cutin() {
		if (is_cutin_inited) return;
		convertPtrType(&CutIn_get_MotionCamera, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineController", "get_MotionCamera", 0));
		convertPtrType(&CutIn_Camera_get_targetTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineMotionCamera", "get_targetTransform", 0));
		convertPtrType(&CutIn_Camera_get_targetCamera, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineMotionCamera", "get_targetCamera", 0));
		convertPtrType(&CutIn_Camera_get_transformObject, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineMotionCamera", "get_transformObject", 0));
		convertPtrType(&CutIn_GetCharacterModelController, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineController", "GetCharacterModelController", 2));
		convertPtrType(&CutIn_GetCharacters, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineController", "GetCharacters", 1));
		convertPtrType(&GetHeadTransform, il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop",
			"ModelController", "GetHeadTransform", 0));
		convertPtrType(&Internal_LookAt_Injected, il2cpp_resolve_icall("UnityEngine.Transform::Internal_LookAt_Injected(UnityEngine.Vector3&,UnityEngine.Vector3&)"));
		convertPtrType(&getTransformPosition, il2cpp_resolve_icall("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&getTransformRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&getTransformLocalPosition, il2cpp_resolve_icall("UnityEngine.Transform::get_localPosition_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&getTransformLocalRotation, il2cpp_resolve_icall("UnityEngine.Transform::get_localRotation_Injected(UnityEngine.Quaternion&)"));
		convertPtrType(&Transform_GetParent, il2cpp_resolve_icall("UnityEngine.Transform::GetParent()"));
		convertPtrType(&Transform_SetParent, il2cpp_resolve_icall("UnityEngine.Transform::SetParent(UnityEngine.Transform,System.Boolean)"));
		convertPtrType(&Unity_Transform_get_localScale_Injected, il2cpp_resolve_icall("UnityEngine.Transform::get_localScale_Injected(UnityEngine.Vector3&)"));
		convertPtrType(&Unity_Transform_set_localScale_Injected, il2cpp_resolve_icall("UnityEngine.Transform::set_localScale_Injected(UnityEngine.Vector3&)"));

		Quaternion_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Quaternion");
		Vector3_klass = il2cpp_symbols::get_class("UnityEngine.CoreModule.dll", "UnityEngine", "Vector3");

		is_cutin_inited = true;
	}

	int getListCount(void* list) {
		static auto listClass = il2cpp_symbols::get_class_from_instance(list);
		static auto getCountMethod = reinterpret_cast<int32_t(*)(const void*)>(il2cpp_class_get_method_from_name(listClass, "get_Count", 0)->methodPointer);
		return getCountMethod(list);
	}

	std::unordered_map<int, std::set<void*>> cutInDisabledObj{};

	void* CutInTimelineController_AlterLateUpdate_orig;
	bool CutInTimelineController_AlterLateUpdate_hook(void* _this) {
		UmaCamera::setUmaCameraType(CAMERA_CUTIN);
		auto ret = reinterpret_cast<decltype(CutInTimelineController_AlterLateUpdate_hook)*>(CutInTimelineController_AlterLateUpdate_orig)(_this);
		if (!g_cutin_first_persion) {
			return ret;
		}
		init_cutin();

		auto charaIndex = UmaCamera::get_cutIn_target_index();
		auto charas = CutIn_GetCharacters(_this, false);
		auto charaCount = getListCount(charas);
		if (charaIndex >= charaCount) {
			if (charaCount <= 0) return ret;
			charaIndex = 0;
		}

		auto modelController = CutIn_GetCharacterModelController(_this, charaIndex, false);
		if (modelController) {
			auto headTransform = GetHeadTransform(modelController);

			auto headrot = reinterpret_cast<Quaternion_t*>(il2cpp_object_new(Quaternion_klass));
			auto headpos = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));
			getTransformPosition(headTransform, headpos);
			getTransformRotation(headTransform, headrot);

			auto motionCamera = CutIn_get_MotionCamera(_this);  // CutInTimelineMotionCamera
			if (motionCamera) {
				auto camera = CutIn_Camera_get_targetCamera(motionCamera);  // UnityEngine.Camera
				Unity_set_nearClipPlane_hook(camera, 0.001f);
				Unity_set_fieldOfView_hook(camera, UmaCamera::getLiveCamFov());

				auto cameraTransform = CutIn_Camera_get_targetTransform(motionCamera);
				auto headParent = Transform_GetParent(headTransform);
				
				// Transform_SetParent(cameraTransform, headParent, true);
				auto cameraParent = Transform_GetParent(cameraTransform);
				auto cameraheadrot = reinterpret_cast<Quaternion_t*>(il2cpp_object_new(Quaternion_klass));
				auto cameraheadpos = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));
				auto cameralocalscale = reinterpret_cast<Vector3_t*>(il2cpp_object_new(Vector3_klass));
				getTransformPosition(headParent, cameraheadpos);
				getTransformRotation(headParent, cameraheadrot);
				Unity_Transform_get_localScale_Injected(headParent, cameralocalscale);
				Unity_set_rotation_Injected_hook(cameraParent, cameraheadrot);
				Unity_set_pos_injected_hook(cameraParent, cameraheadpos);
				Unity_Transform_set_localScale_Injected(cameraParent, cameralocalscale);

				Unity_set_rotation_Injected_hook(cameraTransform, headrot);
				headpos->y += 0.07;
				Unity_set_pos_injected_hook(cameraTransform, headpos);
				
				hideHead(cutInDisabledObj, modelController, charaIndex);
			}
			restoreDisableObj(cutInDisabledObj, charaIndex, false);
		}
		return ret;
	}

	void* CutInTimelineController_OnDestroy_orig;
	void CutInTimelineController_OnDestroy_hook(void* _this) {
		if (g_cutin_first_persion) {
			cutInDisabledObj.clear();
		}
		return reinterpret_cast<decltype(CutInTimelineController_OnDestroy_hook)*>(CutInTimelineController_OnDestroy_orig)(_this);
	}

	std::string currentTime()
	{
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch());
		return std::to_string(ms.count());
	}

	void writeFile(std::string file_name, char* buffer, int len)
	{
		FILE* fp;
		fopen_s(&fp, file_name.c_str(), "wb");
		if (fp != nullptr)
		{
			fwrite(buffer, 1, len, fp);
			fclose(fp);
		}
	}

	void* request_pack_orig = nullptr;
	int request_pack_hook(
		char* src, char* dst, int srcSize, int dstCapacity)
	{
		// Hook LZ4_compress_default_ext

		if (g_bypass_live_205)
		{
			std::string pack(src, srcSize);
			std::vector<uint8_t> new_buffer;
			if (request_convert::live_bypass_pack(pack, &new_buffer))
			{
				char* new_src = reinterpret_cast<char*>(&new_buffer[0]);
				memset(src + 170, 0, srcSize - 170);
				memcpy(src + 170, new_src, new_buffer.size());
				srcSize = new_buffer.size() + 170;
			}
		}

		int ret = reinterpret_cast<decltype(request_pack_hook)*>(request_pack_orig)(
			src, dst, srcSize, dstCapacity);

		if (g_enable_self_server) {
			try {
				const std::string _pack(src, srcSize);
				const auto pack = request_convert::parse_request_pack(_pack);
				request_convert::send_msgpack_post(g_self_server_url, L"/server/push_last_data", pack);  // 传递解密后数据
			}
			catch (std::exception& e) {
				printf("push request data failed: %s\n", e.what());
			}
		}

		if (g_read_request_pack && g_save_msgpack)
		{
			const auto outPath = std::format("MsgPack/{}Q.msgpack", currentTime());
			writeFile(outPath, src, srcSize);
			printf("Save request to %s\n", outPath.c_str());
		}

		/*
		if (!msgFunc::isDMMTokenLoaded)
		{
			string buffer(src, srcSize);
			msgFunc::initDMMToken(msgPrase::praseRequestPack(buffer));
		}
		*/

		return ret;
	}

	void* response_pack_orig = nullptr;
	int response_pack_hook(
		char* src, char* dst, int compressedSize, int dstCapacity)
	{
		int ret = reinterpret_cast<decltype(response_pack_hook)*>(response_pack_orig)(
			src, dst, compressedSize, dstCapacity);

		if (g_read_request_pack && g_save_msgpack)
		{
			const string outPath = std::format("MsgPack/{}R.msgpack", currentTime());
			writeFile(outPath, dst, ret);
			printf("Save response to %s\n", outPath.c_str());
		}

		try {
			std::string uma_data(dst, ret);

			if (g_enable_self_server) {
				const auto data = request_convert::send_post(g_self_server_url, L"/server/get_last_response", L"");
				string resp_str = data.extract_utf8string().get();
				try {
					vector<uint8_t> new_buffer = nlohmann::json::to_msgpack(nlohmann::json::parse(resp_str));
					char* new_dst = reinterpret_cast<char*>(&new_buffer[0]);

					printf("dstC: %d, old_size: %d, new_size: %llu\n", dstCapacity, ret, new_buffer.size());
					if (new_buffer.size() > dstCapacity)
					{
						throw std::range_error("Out of memory when modifying response pack!");
					}

					memset(dst, 0, dstCapacity);
					memcpy_s(dst, dstCapacity, new_dst, new_buffer.size());
					ret = new_buffer.size();
				}
				catch (exception& e) {
					printf("get exception: %s\n", e.what());
				}

			}

			if (g_enable_response_convert) {
				const auto data = request_convert::send_msgpack_post(g_convert_url, L"/convert/response", uma_data);
				if (data.status_code() == 200) {
					string resp_str = data.extract_utf8string().get();
					vector<uint8_t> new_buffer = nlohmann::json::to_msgpack(nlohmann::json::parse(resp_str));
					const char* new_dst = reinterpret_cast<char*>(&new_buffer[0]);
					if (new_buffer.size() > dstCapacity)
					{
						throw std::range_error("Out of memory when modifying response pack!");
					}
					memset(dst, 0, dstCapacity);
					memcpy(dst, new_dst, new_buffer.size());
					ret = new_buffer.size();
				}
			}

			if (g_bypass_live_205)
			{
				std::vector<uint8_t> new_buffer;
				if (request_convert::live_unlock_dress(uma_data, &new_buffer))
				{
					char* new_dst = reinterpret_cast<char*>(&new_buffer[0]);
					if (new_buffer.size() > dstCapacity)
					{
						throw std::range_error("Out of memory when modifying response pack!");
					}
					memset(dst, 0, dstCapacity);
					memcpy(dst, new_dst, new_buffer.size());
					ret = new_buffer.size();
				}
			}

		}
		catch (exception& e) {
			printf("Error: %s\n", e.what());
		}

		return ret;
	}

	void dump_all_entries()
	{
		// 0 is None
		for (int i = 1;; i++)
		{
			auto* str = reinterpret_cast<decltype(localize_jp_get_hook)*>(localize_jp_get_orig)(i);

			if (str && *str->start_char)
			{
				logger::write_entry(i, str->start_char);
			}
			else
			{
				// check next string, if it's still empty, then we are done!
				auto* nextStr = reinterpret_cast<decltype(localize_jp_get_hook)*>(localize_jp_get_orig)(i + 1);
				if (!(nextStr && *nextStr->start_char))
					break;
			}
		}
	}

	Il2CppString* (*get_app_version_name)();

	void path_game_assembly()
	{
		if (!mh_inited)
			return;

		printf("Trying to patch GameAssembly.dll...\n");

		auto il2cpp_module = GetModuleHandle("GameAssembly.dll");

		// load il2cpp exported functions
		il2cpp_symbols::init(il2cpp_module);

		if (!dumpGameAssemblyPath.empty())
		{
			// 在 hook 之前 dump，避免破坏内容
			std::printf("Trying to dump GameAssembly.dll...\n");
			std::ofstream out(dumpGameAssemblyPath, std::ios_base::binary);
			MODULEINFO info;
			if (out && GetModuleInformation(GetCurrentProcess(), il2cpp_module, &info, sizeof(info)))
			{
				const auto header = static_cast<const IMAGE_DOS_HEADER*>(info.lpBaseOfDll);
				const auto ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS*>(
					static_cast<const std::byte*>(info.lpBaseOfDll) + header->e_lfanew);
				const auto sections = IMAGE_FIRST_SECTION(ntHeader);

				out.write(static_cast<const char*>(info.lpBaseOfDll),
					reinterpret_cast<const char*>(sections) +
					ntHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) -
					static_cast<const char*>(info.lpBaseOfDll));

				for (std::size_t i = 0; i < ntHeader->FileHeader.NumberOfSections; ++i)
				{
					const auto& section = sections[i];

					out.seekp(section.PointerToRawData);
					out.write(static_cast<const char*>(info.lpBaseOfDll) + section.VirtualAddress,
						section.SizeOfRawData);
				}
			}
		}

#pragma region HOOK_MACRO
#define ADD_HOOK(_name_, _fmt_) \
	auto _name_##_offset = reinterpret_cast<void*>(_name_##_addr); \
	\
	printf(_fmt_, _name_##_offset); \
	/*dump_bytes(_name_##_offset); */ \
 	\
	MH_CreateHook(_name_##_offset, _name_##_hook, &_name_##_orig); \
	MH_EnableHook(_name_##_offset); 
#pragma endregion
#pragma region HOOK_ADDRESSES
		auto populate_with_errors_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.TextRenderingModule.dll",
			"UnityEngine", "TextGenerator",
			"PopulateWithErrors", 3
		);

		// have to do this way because there's Get(TextId id) and Get(string id)
		// the string one looks like will not be called by elsewhere
		// 现在已经移除了额外的版本，因此可直接 il2cpp_class_get_method_from_name 获取
		const auto localize_class = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "Localize");
		const auto localize_jp_class = il2cpp_symbols::find_nested_class_from_name(localize_class, "JP");
		auto localize_jp_get_addr = il2cpp_class_get_method_from_name(localize_jp_class, "Get", 1)->methodPointer;

		environment_get_stacktrace = reinterpret_cast<decltype(environment_get_stacktrace)>(il2cpp_symbols::get_method_pointer("mscorlib.dll", "System", "Environment", "get_StackTrace", 0));

		auto query_setup_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Query", "_Setup", 2
		);

		auto query_getstr_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Query", "GetText", 1
		);

		auto query_dispose_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Query", "Dispose", 0
		);

		Query_GetInt = reinterpret_cast<int(*)(void*, int)>(
			il2cpp_symbols::get_method_pointer(
				"LibNative.Runtime.dll", "LibNative.Sqlite3",
				"Query", "GetInt", -1
			)
		);

		const auto PreparedQuery_BindInt_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"PreparedQuery", "BindInt", -1
		);

		const auto Plugin_sqlite3_step_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Plugin", "sqlite3_step", -1
		);

		const auto Query_stmt_field = il2cpp_symbols::get_field("LibNative.Runtime.dll", "LibNative.Sqlite3", "Query", "_stmt");
		Query_stmt_offset = Query_stmt_field->offset;

		auto set_fps_addr = il2cpp_resolve_icall("UnityEngine.Application::set_targetFrameRate(System.Int32)");

		auto set_antialiasing_addr = il2cpp_resolve_icall("UnityEngine.QualitySettings::set_antiAliasing(System.Int32)");

		auto graphics_quality_addr = il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"GraphicSettings", "ApplyGraphicsQuality", 2);

		auto set_vsync_count_addr = il2cpp_resolve_icall("UnityEngine.QualitySettings::set_vSyncCount(System.Int32)");

		auto wndproc_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StandaloneWindowResize", "WndProc", 4
		);

		auto get_virt_size_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StandaloneWindowResize", "getOptimizedWindowSizeVirt", 2
		);

		auto get_hori_size_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StandaloneWindowResize", "getOptimizedWindowSizeHori", 2
		);

		is_virt_o = reinterpret_cast<bool(*)()>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"StandaloneWindowResize", "get_IsVirt", 0
			));

		auto InvokeMoveNext_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"SetupCoroutine", "InvokeMoveNext", 2
		);

		auto UpdateCoroutine_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"CoroutineExecutor", "UpdateCoroutine", 0
		);

		get_resolution = reinterpret_cast<Resolution_t * (*)(Resolution_t*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.CoreModule.dll", "UnityEngine",
				"Screen", "get_currentResolution", 0
			)
			);

		auto gallop_get_screenheight_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"Screen", "get_Height", 0
		);

		auto gallop_get_screenwidth_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"Screen", "get_Width", 0
		);

		auto change_resize_ui_for_pc_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"UIManager", "ChangeResizeUIForPC", 2
		);

		auto canvas_scaler_setres_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.UI.dll", "UnityEngine.UI",
			"CanvasScaler", "set_referenceResolution", 1
		);

		set_scale_factor = reinterpret_cast<void(*)(void*, float)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"CanvasScaler", "set_scaleFactor", 1
			)
			);

		UIManager_GetCanvasScalerList = reinterpret_cast<decltype(UIManager_GetCanvasScalerList)>(il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"UIManager", "GetCanvasScalerList", -1
		));

		auto on_populate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"TextCommon", "OnPopulateMesh", 1
		);

		const auto TextCommon_Awake_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"TextCommon", "Awake", 0
		);

		
		TextRubyDataClass = il2cpp_symbols::get_class("umamusume.dll", "", "TextRubyData");
		TextRubyDataClass_DataArray = il2cpp_class_get_field_from_name(TextRubyDataClass, "DataArray");

		StoryTimelineDataClass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "StoryTimelineData");
		StoryTimelineDataClass_StoryIdField = il2cpp_class_get_field_from_name(StoryTimelineDataClass, "StoryId");
		StoryTimelineDataClass_TitleField = il2cpp_class_get_field_from_name(StoryTimelineDataClass, "Title");
		StoryTimelineDataClass_BlockListField = il2cpp_class_get_field_from_name(StoryTimelineDataClass, "BlockList");

		StoryTimelineTextClipDataClass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "StoryTimelineTextClipData");
		StoryTimelineTextClipDataClass_NameField = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass, "Name");
		StoryTimelineTextClipDataClass_TextField = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass, "Text");
		StoryTimelineTextClipDataClass_ChoiceDataList = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass, "ChoiceDataList");
		StoryTimelineTextClipDataClass_ColorTextInfoListField = il2cpp_class_get_field_from_name(StoryTimelineTextClipDataClass, "ColorTextInfoList");

		StoryRaceTextAssetClass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "StoryRaceTextAsset");
		StoryRaceTextAssetClass_textDataField = { il2cpp_class_get_field_from_name(StoryRaceTextAssetClass, "textData") };

		AssetBundle_LoadFromFile = reinterpret_cast<void* (*)(Il2CppString*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.AssetBundleModule.dll", "UnityEngine",
				"AssetBundle", "LoadFromFile", 1
			)
			);

		AssetBundle_Unload = reinterpret_cast<void(*)(void*, bool)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.AssetBundleModule.dll", "UnityEngine",
				"AssetBundle", "Unload", -1
			)
			);

		const auto AssetBundle_LoadAsset_addr =
			il2cpp_resolve_icall("UnityEngine.AssetBundle::LoadAsset_Internal(System.String,System.Type)");

		const auto AssetBundle_LoadAssetAsync_addr =
			il2cpp_resolve_icall("UnityEngine.AssetBundle::LoadAssetAsync_Internal(System.String,System.Type)");

		const auto AssetBundleRequest_get_allAssets_addr = 
			il2cpp_resolve_icall("UnityEngine.AssetBundleRequest::get_allAssets()");
		const auto AssetBundleRequest_GetResult_addr = 
			il2cpp_resolve_icall("UnityEngine.AssetBundleRequest::GetResult()");
		
		auto AssetLoader_LoadAssetHandle_addr =
			il2cpp_symbols::get_method_pointer(
				"_Cyan.dll", "Cyan.Loader",
				"AssetLoader", "LoadAssetHandle", 2
			);

		Object_IsNativeObjectAlive = reinterpret_cast<bool(*)(void*)>(il2cpp_symbols::get_method_pointer("UnityEngine.CoreModule.dll", "UnityEngine", "Object", "IsNativeObjectAlive", 1));

		const auto FontClass = il2cpp_symbols::get_class("UnityEngine.TextRenderingModule.dll", "UnityEngine", "Font");
		Font_Type = il2cpp_type_get_object(il2cpp_class_get_type(FontClass));

		LoadExtraAssetBundle();

		text_assign_font = reinterpret_cast<void(*)(void*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "AssignDefaultFont", 0
			)
			);

		Text_get_font = reinterpret_cast<void* (*)(void*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "get_font", 0
			)
			);

		Text_set_font = reinterpret_cast<void(*)(void*, void*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "set_font", 1
			)
			);

		Text_set_horizontalOverflow = reinterpret_cast<void(*)(void*, int)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "set_horizontalOverflow", -1
			)
			);

		Text_set_verticalOverflow = reinterpret_cast<void(*)(void*, int)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "set_verticalOverflow", -1
			)
			);

		text_get_size = reinterpret_cast<int(*)(void*)>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"TextCommon", "get_FontSize", 0
			)
			);

		const auto text_set_size_addr = reinterpret_cast<void(*)(void*, int)>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"TextCommon", "set_FontSize", 1
			)
			);

		TextCommon_get_text = reinterpret_cast<Il2CppString * (*)(void*)>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"TextCommon", "get_text", 0
			)
			);

		get_app_version_name = reinterpret_cast<Il2CppString * (*)()>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"DeviceHelper", "GetAppVersionName", 0
			)
			);

		text_get_linespacing = reinterpret_cast<float(*)(void*)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "get_lineSpacing", 0
			)
			);

		text_set_style = reinterpret_cast<void(*)(void*, int)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "set_fontStyle", 1
			)
			);

		text_set_linespacing = reinterpret_cast<void(*)(void*, float)>(
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.UI.dll", "UnityEngine.UI",
				"Text", "set_lineSpacing", 1
			)
			);

		auto set_resolution_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Screen", "SetResolution", 3
		);

		auto set_shadow_resolution_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Light", "set_shadowResolution", 1
		);

		auto set_shadows_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Light", "set_shadows", 1
		);

		auto set_RenderTextureAntiAliasing_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"RenderTexture", "set_antiAliasing", 1
		);

		auto Get3DAntiAliasingLevel_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"GraphicSettings", "Get3DAntiAliasingLevel", 1
		);

		auto KeepAspectRatio_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StandaloneWindowResize", "KeepAspectRatio", 2
		);

		auto ReshapeAspectRatio_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StandaloneWindowResize", "ReshapeAspectRatio", 0
		);

		auto on_exit_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"GameSystem", "OnApplicationQuit", 0
		);

		auto AlterUpdate_CameraPos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraPos", 5
		);

		auto AlterUpdate_RadialBlur_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_RadialBlur", 2
		);

		auto SetupRadialBlurInfo_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "SetupRadialBlurInfo", 4
		);

		auto AlterUpdate_MultiCameraRadialBlur_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCameraRadialBlur", 2
		);

		auto alterupdate_eye_camera_pos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_EyeCameraPosition", 3
		);

		auto get_camera_pos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyCameraPositionData", "GetValue", 1
		);

		auto get_camera_pos2_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyCameraPositionData", "GetValue", 2
		);

		auto GetCharacterWorldPos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyCameraLookAtData", "GetCharacterWorldPos", 5
		);

		auto Director_AlterUpdate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live",
			"Director", "AlterUpdate", 0
		);

		auto CySpring_CreateBone_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"CySpring", "CreateBone", 5
		);

		auto CySpringController_UpdateSpringThread_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"CySpringController", "UpdateSpringThread", 0
		);

		auto get_RunMotionSpeed_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"HorseRaceInfoReplay", "get_RunMotionSpeed", 0
		);

		auto HorseRaceInfoReplay_ctor_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"HorseRaceInfoReplay", ".ctor", 2
		);
		
		auto AddUsedSkillId_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"SkillManager", "AddUsedSkillId", 1
		);

		auto AlterUpdate_FormationOffset_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_FormationOffset", 4
		);

		auto AlterUpdate_PostEffect_BloomDiffusion_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_PostEffect_BloomDiffusion", 2
		);

		auto AlterUpdate_CameraMotion_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraMotion", 2
		);

		auto AlterUpdate_TiltShift_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_TiltShift", 2
		);

		auto AlterUpdate_CameraLayer_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraLayer", 4
		);

		auto AlterLateUpdate_CameraMotion_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterLateUpdate_CameraMotion", 2
		);

		auto AlterUpdate_CameraFov_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraFov", 2
		);

		auto AlterUpdate_CameraRoll_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraRoll", 2
		);

		auto LiveTimelineControl_AlterLateUpdate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterLateUpdate", 0
		);

		auto AlterUpdate_MultiCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCamera", 3
		);

		auto alterupdate_camera_lookat_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraLookAt", 4
		);

		auto UpdateEnvironemntStageFovShift_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live",
			"StageController", "UpdateEnvironemntStageFovShift", 1
		);
		
		auto Unity_get_fieldOfView_addr = il2cpp_resolve_icall("UnityEngine.Camera::get_fieldOfView()");
		auto Unity_set_fieldOfView_addr = il2cpp_resolve_icall("UnityEngine.Camera::set_fieldOfView(System.Single)");
		auto Unity_set_pos_injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::set_position_Injected(UnityEngine.Vector3&)");
		auto Unity_get_pos_injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)");
		auto Unity_set_localpos_injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::set_localPosition_Injected(UnityEngine.Vector3&)");
		auto Unity_LookAt_Injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::Internal_LookAt_Injected(UnityEngine.Vector3&,UnityEngine.Vector3&)");
		auto Unity_set_localRotation_Injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::set_localRotation_Injected(UnityEngine.Quaternion&)");
		auto Unity_set_nearClipPlane_addr = il2cpp_resolve_icall("UnityEngine.Camera::set_nearClipPlane(System.Single)");
		auto Unity_get_nearClipPlane_addr = il2cpp_resolve_icall("UnityEngine.Camera::get_nearClipPlane()");
		auto Unity_get_farClipPlane_addr = il2cpp_resolve_icall("UnityEngine.Camera::get_farClipPlane()");
		auto Unity_set_farClipPlane_addr = il2cpp_resolve_icall("UnityEngine.Camera::set_farClipPlane(System.Single)");
		auto Unity_set_cullingMask_addr = il2cpp_resolve_icall("UnityEngine.Camera::set_cullingMask(System.Int32)");
		auto Unity_get_rotation_Injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)");
		auto Unity_set_rotation_Injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::set_rotation_Injected(UnityEngine.Quaternion&)");
		auto Unity_get_local_rotation_Injected_addr = il2cpp_resolve_icall("UnityEngine.Transform::get_localRotation_Injected(UnityEngine.Quaternion&)");
		
		auto HomeClampAngle_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"HomeCameraSwitcher", "ClampAngle", 3
		);

		auto MasterCharaType_Get_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"MasterCharaType", "Get", 4
		);

		auto CreateMiniCharacter_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"NowLoading", "CreateMiniCharacter", 0
		);

		auto FinishDragFreeCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"HomeCameraSwitcher", "FinishDragFreeCamera", 0
		);

		auto ChangeCameraWithImmediate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"HomeCameraSwitcher", "ChangeCameraWithImmediate", 1
		);

		auto AlterUpdate_CameraSwitcher_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraSwitcher", 2
		);

		auto AlterUpdate_MonitorCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MonitorCameraLookAt", 3
		);

		auto LiveTimelineEasing_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineEasing", "GetValue", 5
		);

		auto AlterUpdate_EyeCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_EyeCameraLookAt", 3
		);

		auto AlterUpdate_MultiCameraPosition_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCameraPosition", 3
		);

		auto AlterUpdate_MultiCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCameraLookAt", 3
		);

		auto live_on_destroy_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "OnDestroy", 0
		);

		auto Live_OnClickSettingButton_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"LiveTheaterViewController", "OnClickSettingButton", 0
		);

		auto RaceCameraManager_AlterLateUpdate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraManager", "AlterLateUpdate", 0
		);

		auto RaceCameraManager_UpdateMultiCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraManager", "UpdateMultiCamera", 0
		);

		auto RaceCourseCameraEvent_ApplyMultiCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCourseCameraEvent", "ApplyMultiCamera", 1
		);

		auto race_ChangeCameraMode_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraManager", "ChangeCameraMode", 2
		);

		auto race_get_CameraFov_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraEventBase", "get_CameraFov", 0
		);

		auto race_PlayEventCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraManager", "PlayEventCamera", 5
		);

		auto race_UpdateCameraDistanceBlendRate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceModelController", "UpdateCameraDistanceBlendRate", 3
		);

		auto RaceViewBase_LateUpdateView_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceViewBase", "LateUpdateView", 0
		);

		auto race_get_CameraShakeTargetOffset_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraEventBase", "get_CameraShakeTargetOffset", 0
		);

		auto race_GetTargetRotation_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceCameraEventBase", "GetTargetRotation", 2
		);

		auto race_OnDestroy_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"RaceEffectManager", "OnDestroy", 0
		);

		auto CharacterBuildInfo_ctor_0_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"CharacterBuildInfo", ".ctor", 11
			);

		auto CharacterBuildInfo_ctor_1_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"CharacterBuildInfo", ".ctor", 13
			);

		auto CharacterBuildInfo_Rebuild_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"CharacterBuildInfo", "Rebuild", 0
			);	

		auto GetRaceDressId_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"WorkSingleModeCharaData", "GetRaceDressId", 1
			);		
		/*
		auto CharacterBg_Setup_addr = il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"CharacterBg", "Setup", 9
			);
		*/

		auto GotoTitleOnError_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"GallopUtil", "GotoTitleOnError", 1
			);

		auto EditableCharacterBuildInfo_ctor_addr =
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"EditableCharacterBuildInfo", ".ctor", 10
			);

		auto get_ApplicationServerUrl_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"GameDefine", "get_ApplicationServerUrl", 0
		);

		auto CutInTimelineController_AlterLateUpdate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineController", "AlterLateUpdate", 0
		);
		auto CutInTimelineController_OnDestroy_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.CutIn.Cutt",
			"CutInTimelineController", "OnDestroy", 0
		);

		auto StorySceneController_LoadCharacter_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StorySceneController", "LoadCharacter", 2
		);

		auto UpdateDispatcher_addr = il2cpp_symbols::get_method_pointer(
			"Cute.Core.Assembly.dll", "Cute.Core",
			"UpdateDispatcher", "Update", 0
		);
		
		void* DialogCommon_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "DialogCommon");
		Data_klass = il2cpp_symbols::find_nested_class_from_name(DialogCommon_klass, "Data");
		auto twoButtonFunc = il2cpp_class_get_method_from_name(Data_klass, "SetSimpleTwoButtonMessage", 7);
		void* SetSimpleTwoButtonMessagef_addr;
		if (twoButtonFunc) {
			SetSimpleTwoButtonMessagef_addr = (void*)twoButtonFunc->methodPointer;
		}
		else {
			printf("\n\nSetSimpleTwoButtonMessage get failed!\n\n");
		}

		auto StoryCharacter3D_LoadModel_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"StoryCharacter3D", "LoadModel", 16
		);

		auto SingleModeSceneController_CreateModel_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"SingleModeSceneController", "CreateModel", 3
		);

		// auto HomeCharacterCreator_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop", "HomeCharacterCreator");
		// auto CreateInfo_klass = il2cpp_symbols::find_nested_class_from_name(HomeCharacterCreator_klass, "CreateInfo");
		// auto CreateInfo_ctor_addr = il2cpp_class_get_method_from_name(CreateInfo_klass, ".ctor", 2)->methodPointer;

		// initTimelineCharaBuildInfo();
		
		auto load_scene_internal_addr = il2cpp_resolve_icall("UnityEngine.SceneManagement.SceneManager::LoadSceneAsyncNameIndexInternal_Injected(System.String,System.Int32,UnityEngine.SceneManagement.LoadSceneParameters&,System.Boolean)");

		const auto GallopUtil_GetModifiedString_addr = il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "GallopUtil", "GetModifiedString", -1);

		if (g_read_request_pack || g_bypass_live_205)
		{
			auto libnative_module = GetModuleHandleW(L"libnative.dll");
			auto response_pack_ptr = GetProcAddress(libnative_module, "LZ4_decompress_safe_ext");
			printf("reponse pack at %p\n", response_pack_ptr);
			MH_CreateHook(response_pack_ptr, response_pack_hook, &response_pack_orig);
			MH_EnableHook(response_pack_ptr);

			auto request_pack_ptr = GetProcAddress(libnative_module, "LZ4_compress_default_ext");
			printf("request pack at %p\n", request_pack_ptr);
			MH_CreateHook(request_pack_ptr, request_pack_hook, &request_pack_orig);
			MH_EnableHook(request_pack_ptr);
			ADD_HOOK(get_ApplicationServerUrl, "get_ApplicationServerUrl at %p\n");
		}
#pragma endregion

		// hook UnityEngine.TextGenerator::PopulateWithErrors to modify text
		ADD_HOOK(populate_with_errors, "UnityEngine.TextGenerator::PopulateWithErrors at %p\n");

		// Looks like they store all localized texts that used by code in a dict
		ADD_HOOK(localize_jp_get, "Gallop.Localize.JP.Get(TextId) at %p\n");
		ADD_HOOK(on_exit, "Gallop.GameSystem.onApplicationQuit at %p\n");
		ADD_HOOK(query_setup, "Query::_Setup at %p\n");
		ADD_HOOK(query_getstr, "Query::GetString at %p\n");
		ADD_HOOK(query_dispose, "Query::Dispose at %p\n");
		ADD_HOOK(PreparedQuery_BindInt, "PreparedQuery::BindInt at %p\n");
		ADD_HOOK(Plugin_sqlite3_step, "Plugin_sqlite3_step at %p\n");
		ADD_HOOK(AlterUpdate_CameraPos, "AlterUpdate_CameraPos at %p\n");
		ADD_HOOK(alterupdate_camera_lookat, "alterupdate_camera_lookat at %p\n");
		// ADD_HOOK(UpdateEnvironemntStageFovShift, "UpdateEnvironemntStageFovShift at %p\n");
		ADD_HOOK(Unity_get_fieldOfView, "Unity_get_fieldOfView at %p\n");
		ADD_HOOK(Unity_set_fieldOfView, "Unity_set_fieldOfView at %p\n");
		ADD_HOOK(Unity_set_pos_injected, "Unity_set_pos_injected at %p\n");
		ADD_HOOK(Unity_get_local_rotation_Injected, "Unity_get_local_rotation_Injected at %p\n");
		ADD_HOOK(Unity_set_localpos_injected, "Unity_set_localpos_injected at %p\n");
		ADD_HOOK(Unity_set_localRotation_Injected, "Unity_set_localRotation_Injected at %p\n");
		ADD_HOOK(Unity_set_cullingMask, "Unity_set_cullingMask at %p\n");
		ADD_HOOK(Unity_get_rotation_Injected, "Unity_get_rotation_Injected at %p\n");
		ADD_HOOK(Unity_set_rotation_Injected, "Unity_set_rotation_Injected at %p\n");
		ADD_HOOK(Unity_get_pos_injected, "Unity_get_pos_injected at %p\n");
		ADD_HOOK(Unity_LookAt_Injected, "Unity_LookAt_Injected at %p\n");
		ADD_HOOK(Unity_set_nearClipPlane, "Unity_set_nearClipPlane at %p\n");
		ADD_HOOK(Unity_get_nearClipPlane, "Unity_get_nearClipPlane at %p\n");
		ADD_HOOK(Unity_get_farClipPlane, "Unity_get_farClipPlane at %p\n");
		ADD_HOOK(Unity_set_farClipPlane, "Unity_set_farClipPlane at %p\n");
		
		ADD_HOOK(HomeClampAngle, "HomeClampAngle at %p\n");
		ADD_HOOK(MasterCharaType_Get, "MasterCharaType_Get at %p\n");
		ADD_HOOK(CreateMiniCharacter, "CreateMiniCharacter at %p\n");
		ADD_HOOK(FinishDragFreeCamera, "FinishDragFreeCamera at %p\n");
		ADD_HOOK(ChangeCameraWithImmediate, "ChangeCameraWithImmediate at %p\n");
		ADD_HOOK(AlterUpdate_MonitorCameraLookAt, "AlterUpdate_MonitorCameraLookAt at %p\n");
		ADD_HOOK(LiveTimelineEasing, "LiveTimelineEasing at %p\n");
		ADD_HOOK(AlterUpdate_EyeCameraLookAt, "AlterUpdate_EyeCameraLookAt at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraPosition, "AlterUpdate_MultiCameraPosition at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraLookAt, "AlterUpdate_MultiCameraLookAt at %p\n");
		ADD_HOOK(live_on_destroy, "live_on_destroy at %p\n");
		ADD_HOOK(Live_OnClickSettingButton, "Live_OnClickSettingButton at %p\n");
		ADD_HOOK(get_camera_pos, "get_camera_pos at %p\n");
		ADD_HOOK(get_camera_pos2, "get_camera_pos2 at %p\n");
		ADD_HOOK(GetCharacterWorldPos, "GetCharacterWorldPos at %p\n");
		ADD_HOOK(Director_AlterUpdate, "Director_AlterUpdate at %p\n");
		ADD_HOOK(CySpring_CreateBone, "CySpring_CreateBone at %p\n");
		ADD_HOOK(CySpringController_UpdateSpringThread, "CySpringController_UpdateSpringThread at %p\n");
		ADD_HOOK(get_RunMotionSpeed, "get_RunMotionSpeed at %p\n");
		ADD_HOOK(HorseRaceInfoReplay_ctor, "HorseRaceInfoReplay_ctor at %p\n");
		ADD_HOOK(AddUsedSkillId, "AddUsedSkillId at %p\n");
		ADD_HOOK(AlterUpdate_FormationOffset, "AlterUpdate_FormationOffset at %p\n");
		ADD_HOOK(AlterUpdate_PostEffect_BloomDiffusion, "AlterUpdate_PostEffect_BloomDiffusion at %p\n");
		ADD_HOOK(alterupdate_eye_camera_pos, "alterupdate_eye_camera_pos at %p\n");
		ADD_HOOK(AlterUpdate_CameraMotion, "AlterUpdate_CameraMotion at %p\n");
		ADD_HOOK(AlterUpdate_TiltShift, "AlterUpdate_TiltShift at %p\n");
		ADD_HOOK(AlterUpdate_CameraLayer, "AlterUpdate_CameraLayer at %p\n");
		ADD_HOOK(AlterLateUpdate_CameraMotion, "AlterLateUpdate_CameraMotion at %p\n");
		ADD_HOOK(AlterUpdate_CameraFov, "AlterUpdate_CameraFov at %p\n");
		ADD_HOOK(AlterUpdate_CameraRoll, "AlterUpdate_CameraRoll at %p\n");
		ADD_HOOK(LiveTimelineControl_AlterLateUpdate, "LiveTimelineControl_AlterLateUpdate at %p\n");
		ADD_HOOK(AlterUpdate_CameraSwitcher, "AlterUpdate_CameraSwitcher at %p\n");
		ADD_HOOK(AlterUpdate_RadialBlur, "AlterUpdate_RadialBlur at %p\n");
		ADD_HOOK(SetupRadialBlurInfo, "SetupRadialBlurInfo at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraRadialBlur, "AlterUpdate_MultiCameraRadialBlur at %p\n");
		ADD_HOOK(AlterUpdate_MultiCamera, "AlterUpdate_MultiCamera at %p\n");
		ADD_HOOK(set_shadow_resolution, "set_shadow_resolution at %p\n");
		ADD_HOOK(set_RenderTextureAntiAliasing, "set_RenderTextureAntiAliasing at %p\n");
		ADD_HOOK(Get3DAntiAliasingLevel, "Get3DAntiAliasingLevel at %p\n");
		ADD_HOOK(KeepAspectRatio, "KeepAspectRatio at %p\n");
		ADD_HOOK(ReshapeAspectRatio, "ReshapeAspectRatio at %p\n");
		ADD_HOOK(set_shadows, "set_shadows at %p\n");
		ADD_HOOK(RaceCameraManager_AlterLateUpdate, "RaceCameraManager_AlterLateUpdate at %p\n");
		// ADD_HOOK(RaceCameraManager_UpdateMultiCamera, "RaceCameraManager_UpdateMultiCamera at %p\n");
		// ADD_HOOK(RaceCourseCameraEvent_ApplyMultiCamera, "RaceCourseCameraEvent_ApplyMultiCamera at %p\n");
		ADD_HOOK(race_ChangeCameraMode, "race_ChangeCameraMode at %p\n");
		ADD_HOOK(race_get_CameraFov, "race_get_CameraFov at %p\n");
		ADD_HOOK(race_PlayEventCamera, "race_PlayEventCamera at %p\n");
		ADD_HOOK(race_UpdateCameraDistanceBlendRate, "race_UpdateCameraDistanceBlendRate at %p\n");
		ADD_HOOK(RaceViewBase_LateUpdateView, "RaceViewBase_LateUpdateView at %p\n");
		ADD_HOOK(race_get_CameraShakeTargetOffset, "get_CameraShakePositionOffset at %p\n");
		ADD_HOOK(race_GetTargetRotation, "GetTargetRotation at %p\n");
		ADD_HOOK(race_OnDestroy, "race_OnDestroy at %p\n");
		ADD_HOOK(AssetLoader_LoadAssetHandle, "AssetLoader_LoadAssetHandle at %p\n");
		// ADD_HOOK(CharacterBuildInfo_ctor_0, "CharacterBuildInfo_ctor_0 at %p\n");
		// ADD_HOOK(CharacterBuildInfo_ctor_1, "CharacterBuildInfo_ctor_1 at %p\n");
		// ADD_HOOK(EditableCharacterBuildInfo_ctor, "EditableCharacterBuildInfo_ctor at %p\n");
		ADD_HOOK(CharacterBuildInfo_Rebuild, "CharacterBuildInfo_Rebuild at %p\n");  // 上面三个改成 Rebuild
		ADD_HOOK(GetRaceDressId, "GetRaceDressId at %p\n");
		// ADD_HOOK(CharacterBg_Setup, "CharacterBg_Setup at %p\n");
		ADD_HOOK(GotoTitleOnError, "GotoTitleOnError at %p\n");
		// ADD_HOOK(StorySceneController_LoadCharacter, "StorySceneController_LoadCharacter at %p\n");
		ADD_HOOK(UpdateDispatcher, "UpdateDispatcher at %p\n");
		ADD_HOOK(SetSimpleTwoButtonMessagef, "SetSimpleTwoButtonMessagef at %p\n");
		ADD_HOOK(StoryCharacter3D_LoadModel, "StoryCharacter3D_LoadModel at %p\n");
		ADD_HOOK(SingleModeSceneController_CreateModel, "SingleModeSceneController_CreateModel at %p\n");

		//ADD_HOOK(camera_reset, "UnityEngine.Camera.Reset() at %p\n");

		// ADD_HOOK(load_scene_internal, "SceneManager::LoadSceneAsyncNameIndexInternal at %p\n");

		if (!std::holds_alternative<UseOriginalFont>(g_replace_font))
		{
			ADD_HOOK(TextCommon_Awake, "Gallop.TextCommon::Awake at %p\n");
			ADD_HOOK(text_set_size, "Text.set_size at %p\n");
			if (!g_replace_assets)
			{
				AssetBundle_LoadAsset_orig = reinterpret_cast<void*>(AssetBundle_LoadAsset_addr);
			}
		}

		if (g_replace_assets)
		{
			ADD_HOOK(AssetBundle_LoadAsset, "AssetBundle.LoadAsset at %p\n");
			ADD_HOOK(AssetBundle_LoadAssetAsync, "AssetBundle.LoadAsset at %p\n");
			ADD_HOOK(AssetBundleRequest_get_allAssets, "AssetBundleRequest_get_allAssets at %p\n");
			ADD_HOOK(AssetBundleRequest_GetResult, "AssetBundleRequest_GetResult at %p\n");
		}

		if (g_max_fps > -1)
		{
			// break 30-40fps limit
			ADD_HOOK(set_fps, "UnityEngine.Application.set_targetFrameRate at %p \n");
		}

		ADD_HOOK(set_antialiasing, "UnityEngine.CoreModule.QualitySettings.set_antiAliasing at %p\n");
		ADD_HOOK(graphics_quality, "Gallop.GraphicSettings.ApplyGraphicsQuality at %p\n");
		ADD_HOOK(set_vsync_count, "UnityEngine.CoreModule.QualitySettings.set_vSyncCount at %p\n");
		set_vsync_count_hook(1);
	
		// if (g_unlock_size)
		// {
		// break 1080p size limit
		ADD_HOOK(get_virt_size, "Gallop.StandaloneWindowResize.getOptimizedWindowSizeVirt at %p \n");
		ADD_HOOK(get_hori_size, "Gallop.StandaloneWindowResize.getOptimizedWindowSizeHori at %p \n");
		ADD_HOOK(InvokeMoveNext, "InvokeMoveNext at %p \n");
		ADD_HOOK(UpdateCoroutine, "UpdateCoroutine at %p \n");
		ADD_HOOK(wndproc, "Gallop.StandaloneWindowResize.WndProc at %p \n");

		// remove fixed 1080p render resolution
		ADD_HOOK(gallop_get_screenheight, "Gallop.Screen::get_Height at %p\n");
		ADD_HOOK(gallop_get_screenwidth, "Gallop.Screen::get_Width at %p\n");

		ADD_HOOK(canvas_scaler_setres, "UnityEngine.UI.CanvasScaler::set_referenceResolution at %p\n");
		ADD_HOOK(change_resize_ui_for_pc, "change_resize_ui_for_pc at %p\n");
		// }

		ADD_HOOK(set_resolution, "UnityEngine.Screen.SetResolution(int, int, bool) at %p\n");
		ADD_HOOK(GallopUtil_GetModifiedString, "GallopUtil_GetModifiedString at %p\n");
		ADD_HOOK(CutInTimelineController_AlterLateUpdate, "CutInTimelineController_AlterLateUpdate at %p\n");
		ADD_HOOK(CutInTimelineController_OnDestroy, "CutInTimelineController_OnDestroy at %p\n");
		UmaLiveHook::regHookMain();
		if (g_auto_fullscreen)
		{
			adjust_size();
		}

		if (g_dump_entries)
			dump_all_entries();
		// start_monitor_thread();
		_set_u_stat(true);
		set_start_resolution();
		showDialog = showDialogf;
		request_convert::startUpdateNotice();
	}

	void set_start_resolution() {
		thread([]() {
			while (!g_load_finished) {
				Sleep(100);
			}
			printf("LOAD FINISHED!\n");

			int new_h, new_w;
			if (start_width > 0 && start_height > 0) {
				new_h = start_height;
				new_w = start_width;
			}
			else {
				Resolution_t r;
				r = *get_resolution(&r);
				new_h = r.height - 150;
				new_w = new_h / g_aspect_ratio;
			}

			auto window = FindWindowW(L"UnityWndClass", L"umamusume");
			if (window == NULL) return;
			std::shared_ptr<RECT> windowR = std::make_shared<RECT>();

			GetWindowRect(window, windowR.get());
			setWindowPosOffset(window, HWND_NOTOPMOST, windowR->left, windowR->top, new_w, new_h, SWP_DEFERERASE);
			}).detach();
	}
}

void LoadExtraAssetBundle()
{
	if (g_extra_assetbundle_paths.empty())
	{
		return;
	}
	// CustomAssetBundleHandleMap.clear();
	// CustomAssetBundleAssetPaths.clear();
	// assert(!ExtraAssetBundleHandle && ExtraAssetBundleAssetPaths.empty());

	const auto AssetBundle_GetAllAssetNames = reinterpret_cast<void* (*)(void*)>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.AssetBundleModule.dll", "UnityEngine",
			"AssetBundle", "GetAllAssetNames", 0
		)
		);

	for (const auto& i : g_extra_assetbundle_paths) {
		if (CustomAssetBundleHandleMap.contains(i)) continue;

		const auto extraAssetBundle = AssetBundle_LoadFromFile(il2cpp_string_new(i.c_str()));
		if (extraAssetBundle)
		{
			const auto allAssetPaths = AssetBundle_GetAllAssetNames(extraAssetBundle);
			AssetPathsType assetPath{};
			il2cpp_symbols::iterate_IEnumerable<Il2CppString*>(allAssetPaths, [&assetPath](Il2CppString* path)
				{
					// ExtraAssetBundleAssetPaths.emplace(path->start_char);
					assetPath.emplace(path->start_char);
				});
			CustomAssetBundleAssetPaths.emplace(i, assetPath);
			CustomAssetBundleHandleMap.emplace(i, il2cpp_gchandle_new(extraAssetBundle, false));
		}
		else
		{
			printf("Cannot load asset bundle: %s\n", i.c_str());
		}
	}
}

void UnloadExtraAssetBundle()
{	
	CustomAssetBundleAssetPaths.clear();
	for (const auto& i : CustomAssetBundleHandleMap) {
		if (i.second)
		{
			AssetBundle_Unload(il2cpp_gchandle_get_target(i.second), true);
			il2cpp_gchandle_free(i.second);
			// i.second = 0;
		}
	}

}

void reloadAssetBundle() {
	// UnloadExtraAssetBundle();
	LoadExtraAssetBundle();
}

bool init_hook()
{
	if (mh_inited)
		return false;

	if (MH_Initialize() != MH_OK)
		return false;

	mh_inited = true;
	onPluginReload.push_back(reloadAssetBundle);

	MH_CreateHook(LoadLibraryW, load_library_w_hook, &load_library_w_orig);
	MH_EnableHook(LoadLibraryW);
	return true;
}

void uninit_hook()
{
	if (!mh_inited)
		return;

	UnloadExtraAssetBundle();

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

std::optional<std::wstring> localize_get(int id)
{
	if (!localize_jp_get_orig)
	{
		return {};
	}

	const auto str = reinterpret_cast<decltype(localize_jp_get_hook)*>(localize_jp_get_orig)(id);
	if (!str || !str->start_char[0])
	{
		return {};
	}

	return str->start_char;
}

std::optional<std::wstring> get_game_version_name()
{
	if (!get_app_version_name)
	{
		return {};
	}

	const auto versionString = get_app_version_name();
	if (!versionString || !versionString->start_char[0])
	{
		return {};
	}

	return versionString->start_char;
}
