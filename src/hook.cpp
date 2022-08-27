#include <stdinclude.hpp>
#include <unordered_set>

using namespace std;
std::function<void()> g_on_hook_ready;

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

	void* query_ctor_orig = nullptr;
	void* query_ctor_hook(void* _this, void* conn, Il2CppString* sql)
	{
		auto ssql = std::wstring_view(sql->start_char);
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
		return reinterpret_cast<decltype(query_ctor_hook)*>(query_ctor_orig)(_this, conn, sql);
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

	void* Query_Step_orig = nullptr;
	bool Query_Step_hook(void* _this)
	{
		const auto result = reinterpret_cast<decltype(Query_Step_hook)*>(Query_Step_orig)(_this);

		if (const auto iter = text_queries.find(_this); iter != text_queries.end())
		{
			iter->second->Step(_this);
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

	bool (*is_virt)() = nullptr;
	int last_height = 0, last_width = 0;

	void* wndproc_orig = nullptr;
	LRESULT wndproc_hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (g_unlock_size) {
			if (uMsg == WM_SIZING)
			{
				RECT* rect = reinterpret_cast<RECT*>(lParam);
				bool isVert = is_virt();

				float ratio = isVert ? 1.f / g_aspect_ratio : g_aspect_ratio;
				float height = rect->bottom - rect->top;
				float width = rect->right - rect->left;

				float new_ratio = width / height;

				// std::cout << std::format("top: {}, bottom: {}, left: {}, right: {}", rect->top, rect->bottom, rect->left, rect->right) << std::endl;
				// std::cout << std::format("w:{} h: {} new_ra: {}, set_ra: {}, af_offset_ra: {}\n", width, height, new_ratio, ratio, ratio + (isVert ? g_unlock_size_offset_vert : g_unlock_size_offset_land)) << std::endl;
				float offset_ratio = ratio + (isVert ? g_unlock_size_offset_vert : g_unlock_size_offset_land);
				if (offset_ratio <= 0) {
					std::cout << std::format("offset <= 0, value: {} - {} = {}", ratio, (isVert ? g_unlock_size_offset_vert : g_unlock_size_offset_land), offset_ratio) << endl;
					offset_ratio = ratio;
				}

				if (new_ratio > ratio && height >= last_height || width < last_width)
					height = width / offset_ratio;
				else if (new_ratio < ratio && width >= last_width || height < last_height)
					width = height * offset_ratio;

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

	void* canvas_scaler_setres_orig;
	void canvas_scaler_setres_hook(void* _this, Vector2_t res)
	{
		Resolution_t r;
		get_resolution_stub(&r);

		// set scale factor to make ui bigger on hi-res screen
		set_scale_factor(_this, max(1.0f, r.width / 1920.f) * g_ui_scale);

		return reinterpret_cast<decltype(canvas_scaler_setres_hook)*>(canvas_scaler_setres_orig)(_this, res);
	}

	struct TransparentStringHash : std::hash<std::wstring>, std::hash<std::wstring_view>
	{
		using is_transparent = void;
	};

	std::unordered_set<std::wstring, TransparentStringHash, std::equal_to<void>> ExtraAssetBundleAssetPaths;
	void* (*AssetBundle_LoadFromFile)(Il2CppString* path);
	void (*AssetBundle_Unload)(void* _this, bool unloadAllLoadedObjects);

	uint32_t ExtraAssetBundleHandle;

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

	void LocalizeStoryTimelineData(void* timelineData)
	{
		const auto storyIdStr = il2cpp_symbols::read_field<Il2CppString*>(timelineData, StoryTimelineDataClass_StoryIdField);
		const auto storyId = static_cast<std::size_t>(_wtoll(storyIdStr->start_char));
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

							il2cpp_symbols::write_field(choiceData, StoryTimelineTextClipDataClass_ChoiceDataClass_TextField, il2cpp_symbols::NewWStr(clip->ChoiceDataList[j]));
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

							il2cpp_symbols::write_field(colorTextInfo, StoryTimelineTextClipDataClass_ColorTextInfoClass_TextField, il2cpp_symbols::NewWStr(clip->ColorTextInfoList[j]));
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

	void* AssetBundle_LoadAsset_orig;
	void* AssetBundle_LoadAsset_hook(void* _this, Il2CppString* name, Il2CppReflectionType* type)
	{
		//const auto stackTrace = environment_get_stacktrace();
		if (ExtraAssetBundleHandle)
		{
			const auto extraAssetBundle = il2cpp_gchandle_get_target(ExtraAssetBundleHandle);
			if (ExtraAssetBundleAssetPaths.contains(std::wstring_view(name->start_char)))
			{
				return reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(extraAssetBundle, name, type);
			}
		}
		const auto cls = il2cpp_class_from_type(type->type);
		if (g_asset_load_log)
		{
			const auto assetCls = static_cast<Il2CppClassHead*>(cls);
			std::wprintf(L"AssetBundle.LoadAsset(this = %p, name = %ls, type = %ls)\n", _this, name->start_char, utility::conversions::to_string_t(assetCls->name).c_str());
		}
		const auto result = reinterpret_cast<decltype(AssetBundle_LoadAsset_hook)*>(AssetBundle_LoadAsset_orig)(_this, name, type);
		if (result)
		{
			if (cls == StoryTimelineDataClass)
			{
				LocalizeStoryTimelineData(result);
			}
			else if (cls == StoryRaceTextAssetClass)
			{
				const auto assetPath = std::filesystem::path(name->start_char).stem();
				const std::wstring_view assetName = assetPath.native();
				constexpr const wchar_t RacePrefix[] = L"storyrace_";
				assert(assetName.starts_with(RacePrefix));
				LocalizeStoryRaceTextAsset(result, static_cast<std::size_t>(_wtoll(assetName.substr(std::size(RacePrefix) - 1).data())));
			}
		}
		return result;
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
		if (std::holds_alternative<UseCustomFont>(g_replace_font) && ExtraAssetBundleHandle)
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
			const auto extraAssetBundle = il2cpp_gchandle_get_target(ExtraAssetBundleHandle);
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
		Resolution_t r;
		r = *get_resolution(&r);
		// MessageBoxA(NULL, std::format("window: {}, {}", width, height).c_str(), "TEST", MB_OK);

		if (width > height) {
			_set_u_stat(false);  // false-横屏
			// std::wprintf(L"to land: %d * %d\n", width, height);
		}
		else {
			_set_u_stat(true);
			// std::wprintf(L"to virt: %d * %d\n", width, height);
		}

		bool need_fullscreen = false;

		if (is_virt() && r.width / static_cast<double>(r.height) == (9.0 / 16.0) && g_auto_fullscreen)
			need_fullscreen = true;
		else if (!is_virt() && r.width / static_cast<double>(r.height) == (16.0 / 9.0) && g_auto_fullscreen)
			need_fullscreen = true;

		return reinterpret_cast<decltype(set_resolution_hook)*>(set_resolution_orig)(
			need_fullscreen ? r.width : width, need_fullscreen ? r.height : height, need_fullscreen
			);
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
	void AlterUpdate_CameraPos_hook(void* _this, Il2CppObject* sheet, int currentFrame, int sheetIndex, bool isUseCameraMotion) {
		// printf("frame: %d, index: %d, motion: %s\n", currentFrame, sheetIndex, isUseCameraMotion ? "true" : "false");
		return reinterpret_cast<decltype(AlterUpdate_CameraPos_hook)*>(AlterUpdate_CameraPos_orig)(
			_this, sheet, g_live_free_camera ? 0 : currentFrame, sheetIndex, isUseCameraMotion);
	}

	void* AlterUpdate_RadialBlur_orig;
	void AlterUpdate_RadialBlur_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_RadialBlur_hook)*>(AlterUpdate_RadialBlur_orig)(
			_this, sheet, 0);

		// printf("AlterUpdate_RadialBlur frame: %d\n", currentFrame);
	}

	void* get_IsEnabledDiffusion_orig;
	bool get_IsEnabledDiffusion_hook(void* _this) {
		// if (g_live_free_camera) return false;  // 唱歌口型问题
		return reinterpret_cast<decltype(get_IsEnabledDiffusion_hook)*>(get_IsEnabledDiffusion_orig)(_this);
	}

	void* LiveTimelineKeyRadialBlurData_get_orig;
	bool LiveTimelineKeyRadialBlurData_get_hook(void* _this) {
		auto data = reinterpret_cast<decltype(LiveTimelineKeyRadialBlurData_get_hook)*>(LiveTimelineKeyRadialBlurData_get_orig)(_this);
		// printf("nbnbnbnb: %d\n", data);
		if (g_live_free_camera)
			return !data;
		else
			return data;
	}

	void* get_IsEnabledBloom_orig;
	bool get_IsEnabledBloom_hook(void* _this) {
		// if (g_live_free_camera) return false;  // 非唱歌期间表情问题
		return reinterpret_cast<decltype(get_IsEnabledBloom_hook)*>(get_IsEnabledBloom_orig)(_this);
	}

	void* AlterUpdate_MultiCameraPosition_orig;
	void AlterUpdate_MultiCameraPosition_hook(void* _this, void* sheet, int currentFrame) {
		// printf("UpdMultiPos: %d\n", currentFrame);
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_MultiCameraPosition_hook)*>(AlterUpdate_MultiCameraPosition_orig)(
			_this, sheet, currentFrame);
	}

	void* AlterUpdate_MultiCameraLookAt_orig;
	void AlterUpdate_MultiCameraLookAt_hook(void* _this, void* sheet, int currentFrame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_MultiCameraLookAt_hook)*>(AlterUpdate_MultiCameraLookAt_orig)(
			_this, sheet, currentFrame);
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
	void alterupdate_eye_camera_pos_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(alterupdate_eye_camera_pos_hook)*>(alterupdate_eye_camera_pos_orig)(
			_this, sheet, currentFrame);
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
	Vector3_t* get_camera_pos_hook(void* _this, Il2CppObject* timelineControl) {
		auto pos = reinterpret_cast<decltype(get_camera_pos_hook)*>(get_camera_pos_orig)(_this, timelineControl);
		if (!g_live_free_camera) {
			return pos;
		}
		// printf("orig_pos: %f, %f, %f\n", pos->x, pos->y, pos->z);
		auto setPos = UmaCamera::getCameraPos();
		pos->x = setPos.x;
		pos->y = setPos.y;
		pos->z = setPos.z;
		// printf("pos: %f, %f, %f\n", pos->x, pos->y, pos->z);
		return pos;
	}

	void* alterupdate_camera_lookat_orig;
	void alterupdate_camera_lookat_hook(void* _this, Il2CppObject* sheet, int currentFrame, Vector3_t* outLookAt) {
		if (!g_live_free_camera) {
			 return reinterpret_cast<decltype(alterupdate_camera_lookat_hook)*>(alterupdate_camera_lookat_orig)(
				_this, sheet, currentFrame, outLookAt);
		}

		auto setLookat = UmaCamera::getCameraLookat();
		outLookAt->x = setLookat.x;
		outLookAt->y = setLookat.y;
		outLookAt->z = setLookat.z;
		// printf("frame: %d, look: %f, %f, %f\n", currentFrame, outLookAt->x, outLookAt->y, outLookAt->z);
	}

	void* AlterUpdate_MonitorCameraLookAt_orig;
	void AlterUpdate_MonitorCameraLookAt_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// printf("AlterUpdate_MonitorCameraLookAt frame: %d\n", currentFrame);
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_MonitorCameraLookAt_hook)*>(AlterUpdate_MonitorCameraLookAt_orig)(
				_this, sheet, currentFrame);
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

	void* LiveTimelineMotionSequence_AlterUpdate_orig;  // 角色动作
	void LiveTimelineMotionSequence_AlterUpdate_hook(void* _this, int currentFrame, float currentTime, float addTime, float targetFps, void* timescaleKeys) {
		// printf("MotionSequenceUpdate frame: %d, currTime: %f, addTime: %f, targetFps: %f\n", currentFrame, currentTime, addTime, targetFps);
		return reinterpret_cast<decltype(LiveTimelineMotionSequence_AlterUpdate_hook)*>(LiveTimelineMotionSequence_AlterUpdate_orig)(
			_this, currentFrame, currentTime, addTime, targetFps, timescaleKeys
			);
	}

	void* AlterUpdate_EyeCameraLookAt_orig;
	void AlterUpdate_EyeCameraLookAt_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		// printf("AlterUpdate_EyeCameraLookAt frame: %d\n", currentFrame);
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_EyeCameraLookAt_hook)*>(AlterUpdate_EyeCameraLookAt_orig)(
				_this, sheet, currentFrame);
		}
	}

	void* live_on_destroy_orig;
	void live_on_destroy_hook(void* _this) {
		printf("Live End!\n");
		UmaCamera::reset_camera();
		return reinterpret_cast<decltype(live_on_destroy_hook)*>(live_on_destroy_orig)(_this);
	}

	void* get_camera_pos2_orig;  // 暂时没用
	Vector3_t* get_camera_pos2_hook(void* _this, Il2CppObject* timelineControl, void* type) {
		auto pos = reinterpret_cast<decltype(get_camera_pos2_hook)*>(get_camera_pos2_orig)(_this, timelineControl, type);
		printf("pos2: %f, %f, %f\n", pos->x, pos->y, pos->z);
		if (!g_live_free_camera) {
			return pos;
		}
		auto setPos = UmaCamera::getCameraPos();
		pos->x = setPos.x;
		pos->y = setPos.y;
		pos->z = setPos.z;
		return pos;
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

	void* AlterUpdate_PostEffect_DOF_orig;
	void AlterUpdate_PostEffect_DOF_hook(void* _this, Il2CppObject* sheet, int currentFrame, Vector3_t* cameraLookAt) {
		if (g_live_free_camera) return;
		return reinterpret_cast<decltype(AlterUpdate_PostEffect_DOF_hook)*>(AlterUpdate_PostEffect_DOF_orig)(_this, sheet, currentFrame, cameraLookAt);
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

	void* AlterUpdate_MultiCamera_orig;
	bool AlterUpdate_MultiCamera_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		if (!g_live_free_camera) {
			return reinterpret_cast<decltype(AlterUpdate_MultiCamera_hook)*>(AlterUpdate_MultiCamera_orig)(_this, sheet, currentFrame);
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
				set_resolution_hook(target_height * 0.5625f, target_height, false);

			il2cpp_thread_detach(tr);
			}).detach();
	}

	void* load_scene_internal_orig = nullptr;
	void* load_scene_internal_hook(Il2CppString* sceneName, int sceneBuildIndex, void* parameters, bool mustCompleteNextFrame)
	{
		wprintf(L"%ls\n", sceneName->start_char);
		return reinterpret_cast<decltype(load_scene_internal_hook)*>(load_scene_internal_orig)(sceneName, sceneBuildIndex, parameters, mustCompleteNextFrame);
	}

	void* request_pack_orig = nullptr;
	int request_pack_hook(
		char* src, char* dst, int srcSize, int dstCapacity)
	{
		// Hook LZ4_compress_default_ext
		int ret = reinterpret_cast<decltype(request_pack_hook)*>(request_pack_orig)(
			src, dst, srcSize, dstCapacity);

		if (!msgFunc::isDMMTokenLoaded)
		{
			string buffer(src, srcSize);
			msgFunc::initDMMToken(msgPrase::praseRequestPack(buffer));
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

#pragma region HOOK_MACRO
#define ADD_HOOK(_name_, _fmt_) \
	auto _name_##_offset = reinterpret_cast<void*>(_name_##_addr); \
	\
	printf(_fmt_, _name_##_offset); \
	dump_bytes(_name_##_offset); \
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

		auto query_ctor_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Query", ".ctor", 2
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

		const auto Query_Step_addr = il2cpp_symbols::get_method_pointer(
			"LibNative.Runtime.dll", "LibNative.Sqlite3",
			"Query", "Step", -1
		);

		auto set_fps_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"Application", "set_targetFrameRate", 1
		);

		auto set_antialiasing_addr = il2cpp_symbols::get_method_pointer(
				"UnityEngine.CoreModule.dll", "UnityEngine",
				"QualitySettings", "set_antiAliasing", 1
			);

		auto graphics_quality_addr = il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"GraphicSettings", "ApplyGraphicsQuality", 2);

		auto set_vsync_count_addr = il2cpp_symbols::get_method_pointer(
			"UnityEngine.CoreModule.dll", "UnityEngine",
			"QualitySettings", "set_vSyncCount", 1
		);

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

		is_virt = reinterpret_cast<bool(*)()>(
			il2cpp_symbols::get_method_pointer(
				"umamusume.dll", "Gallop",
				"StandaloneWindowResize", "get_IsVirt", 0
			));

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

		auto on_populate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"TextCommon", "OnPopulateMesh", 1
		);

		const auto TextCommon_Awake_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"TextCommon", "Awake", 0
		);

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
			il2cpp_symbols::get_method_pointer(
				"UnityEngine.AssetBundleModule.dll", "UnityEngine",
				"AssetBundle", "LoadAsset", 2
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

		auto on_exit_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop",
			"GameSystem", "OnApplicationQuit", 0
		);

		auto AlterUpdate_CameraPos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraPos", 4
		);

		auto AlterUpdate_RadialBlur_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_RadialBlur", 2
		);

		auto get_IsEnabledDiffusion_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyPostEffectBloomDiffusionData", "get_IsEnabledDiffusion", 0
		);

		auto LiveTimelineKeyRadialBlurData_get_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyRadialBlurData", "get_IsEnabledDepthCancelRect", 0
		);

		auto get_IsEnabledBloom_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyPostEffectBloomDiffusionData", "get_IsEnabledBloom", 0
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
			"LiveTimelineControl", "AlterUpdate_EyeCameraPosition", 2
		);

		auto get_camera_pos_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyCameraPositionData", "GetValue", 1
		);

		auto get_camera_pos2_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineKeyCameraPositionData", "GetValue", 2
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

		auto AlterUpdate_PostEffect_DOF_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_PostEffect_DOF", 3
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

		auto AlterUpdate_MultiCamera_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCamera", 2
		);

		auto alterupdate_camera_lookat_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraLookAt", 3
		);

		auto AlterUpdate_CameraSwitcher_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_CameraSwitcher", 2
		);

		auto AlterUpdate_MonitorCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MonitorCameraLookAt", 2
		);

		auto LiveTimelineEasing_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineEasing", "GetValue", 5
		);

		auto LiveTimelineMotionSequence_AlterUpdate_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineMotionSequence", "AlterUpdate", 5
		);

		auto AlterUpdate_EyeCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_EyeCameraLookAt", 2
		);

		auto AlterUpdate_MultiCameraPosition_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCameraPosition", 2
		);

		auto AlterUpdate_MultiCameraLookAt_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "AlterUpdate_MultiCameraLookAt", 2
		);

		auto live_on_destroy_addr = il2cpp_symbols::get_method_pointer(
			"umamusume.dll", "Gallop.Live.Cutt",
			"LiveTimelineControl", "OnDestroy", 0
		);

		auto load_scene_internal_addr = il2cpp_resolve_icall("UnityEngine.SceneManagement.SceneManager::LoadSceneAsyncNameIndexInternal_Injected(System.String,System.Int32,UnityEngine.SceneManagement.LoadSceneParameters&,System.Boolean)");

		const auto GallopUtil_GetModifiedString_addr = il2cpp_symbols::get_method_pointer("umamusume.dll", "Gallop", "GallopUtil", "GetModifiedString", -1);

		if (g_read_request_pack)
		{
			auto libnative_module = GetModuleHandle("libnative.dll");
			auto request_pack_ptr = GetProcAddress(libnative_module, "LZ4_compress_default_ext");
			printf("request pack at %p\n", request_pack_ptr);
			MH_CreateHook(request_pack_ptr, request_pack_hook, &request_pack_orig);
			MH_EnableHook(request_pack_ptr);
		}
#pragma endregion

		// hook UnityEngine.TextGenerator::PopulateWithErrors to modify text
		ADD_HOOK(populate_with_errors, "UnityEngine.TextGenerator::PopulateWithErrors at %p\n");

		// Looks like they store all localized texts that used by code in a dict
		ADD_HOOK(localize_jp_get, "Gallop.Localize.JP.Get(TextId) at %p\n");
		ADD_HOOK(on_exit, "Gallop.GameSystem.onApplicationQuit at %p\n");
		ADD_HOOK(query_ctor, "Query::ctor at %p\n");
		ADD_HOOK(query_getstr, "Query::GetString at %p\n");
		ADD_HOOK(query_dispose, "Query::Dispose at %p\n");
		ADD_HOOK(PreparedQuery_BindInt, "PreparedQuery::BindInt at %p\n");
		ADD_HOOK(Query_Step, "Query::Step at %p\n");
		ADD_HOOK(AlterUpdate_CameraPos, "AlterUpdate_CameraPos at %p\n");
		ADD_HOOK(alterupdate_camera_lookat, "alterupdate_camera_lookat at %p\n");
		ADD_HOOK(AlterUpdate_MonitorCameraLookAt, "AlterUpdate_MonitorCameraLookAt at %p\n");
		ADD_HOOK(LiveTimelineEasing, "LiveTimelineEasing at %p\n");
		ADD_HOOK(LiveTimelineMotionSequence_AlterUpdate, "LiveTimelineMotionSequence_AlterUpdate at %p\n");
		ADD_HOOK(AlterUpdate_EyeCameraLookAt, "AlterUpdate_EyeCameraLookAt at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraPosition, "AlterUpdate_MultiCameraPosition at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraLookAt, "AlterUpdate_MultiCameraLookAt at %p\n");
		ADD_HOOK(live_on_destroy, "live_on_destroy at %p\n");
		ADD_HOOK(get_camera_pos, "get_camera_pos at %p\n");
		ADD_HOOK(get_camera_pos2, "get_camera_pos2 at %p\n");
		ADD_HOOK(AlterUpdate_FormationOffset, "AlterUpdate_FormationOffset at %p\n");
		ADD_HOOK(AlterUpdate_PostEffect_BloomDiffusion, "AlterUpdate_PostEffect_BloomDiffusion at %p\n");
		ADD_HOOK(alterupdate_eye_camera_pos, "alterupdate_eye_camera_pos at %p\n");
		ADD_HOOK(AlterUpdate_CameraMotion, "AlterUpdate_CameraMotion at %p\n");
		ADD_HOOK(AlterUpdate_TiltShift, "AlterUpdate_TiltShift at %p\n");
		ADD_HOOK(AlterUpdate_CameraLayer, "AlterUpdate_CameraLayer at %p\n");
		ADD_HOOK(AlterLateUpdate_CameraMotion, "AlterLateUpdate_CameraMotion at %p\n");
		ADD_HOOK(AlterUpdate_CameraFov, "AlterUpdate_CameraFov at %p\n");
		ADD_HOOK(AlterUpdate_CameraRoll, "AlterUpdate_CameraRoll at %p\n");
		ADD_HOOK(AlterUpdate_CameraSwitcher, "AlterUpdate_CameraSwitcher at %p\n");
		ADD_HOOK(AlterUpdate_RadialBlur, "AlterUpdate_RadialBlur at %p\n");
		ADD_HOOK(get_IsEnabledDiffusion, "get_IsEnabledDiffusion at %p\n");
		ADD_HOOK(LiveTimelineKeyRadialBlurData_get, "LiveTimelineKeyRadialBlurData_get at %p\n");
		ADD_HOOK(get_IsEnabledBloom, "get_IsEnabledBloom at %p\n");
		ADD_HOOK(SetupRadialBlurInfo, "SetupRadialBlurInfo at %p\n");
		ADD_HOOK(AlterUpdate_MultiCameraRadialBlur, "AlterUpdate_MultiCameraRadialBlur at %p\n");
		ADD_HOOK(AlterUpdate_MultiCamera, "AlterUpdate_MultiCamera at %p\n");
		ADD_HOOK(set_shadow_resolution, "set_shadow_resolution at %p\n");
		ADD_HOOK(set_RenderTextureAntiAliasing, "set_RenderTextureAntiAliasing at %p\n");
		ADD_HOOK(set_shadows, "set_shadows at %p\n");

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
		ADD_HOOK(wndproc, "Gallop.StandaloneWindowResize.WndProc at %p \n");

		// remove fixed 1080p render resolution
		ADD_HOOK(gallop_get_screenheight, "Gallop.Screen::get_Height at %p\n");
		ADD_HOOK(gallop_get_screenwidth, "Gallop.Screen::get_Width at %p\n");

		ADD_HOOK(canvas_scaler_setres, "UnityEngine.UI.CanvasScaler::set_referenceResolution at %p\n");
		// }

		ADD_HOOK(set_resolution, "UnityEngine.Screen.SetResolution(int, int, bool) at %p\n");
		ADD_HOOK(GallopUtil_GetModifiedString, "GallopUtil_GetModifiedString at %p\n");
		if (g_auto_fullscreen)
		{
			adjust_size();
		}

		if (g_dump_entries)
			dump_all_entries();
		// start_monitor_thread();
		_set_u_stat(true);

		if (start_width != -1 && start_height != -1) {
			set_resolution_hook(start_width, start_height, false);
		}

	}
}


bool init_hook()
{
	if (mh_inited)
		return false;

	if (MH_Initialize() != MH_OK)
		return false;

	mh_inited = true;

	MH_CreateHook(LoadLibraryW, load_library_w_hook, &load_library_w_orig);
	MH_EnableHook(LoadLibraryW);

	return true;
}

void LoadExtraAssetBundle()
{
	if (g_extra_assetbundle_path.empty())
	{
		return;
	}

	assert(!ExtraAssetBundleHandle && ExtraAssetBundleAssetPaths.empty());

	const auto AssetBundle_GetAllAssetNames = reinterpret_cast<void* (*)(void*)>(
		il2cpp_symbols::get_method_pointer(
			"UnityEngine.AssetBundleModule.dll", "UnityEngine",
			"AssetBundle", "GetAllAssetNames", 0
		)
		);

	const auto extraAssetBundle = AssetBundle_LoadFromFile(il2cpp_string_new(g_extra_assetbundle_path.c_str()));
	if (extraAssetBundle)
	{
		const auto allAssetPaths = AssetBundle_GetAllAssetNames(extraAssetBundle);
		il2cpp_symbols::iterate_IEnumerable<Il2CppString*>(allAssetPaths, [](Il2CppString* path)
			{
				ExtraAssetBundleAssetPaths.emplace(path->start_char);
			});
		ExtraAssetBundleHandle = il2cpp_gchandle_new(extraAssetBundle, false);
	}
	else
	{
		std::wprintf(L"Cannot load asset bundle\n");
	}
}

void UnloadExtraAssetBundle()
{
	if (ExtraAssetBundleHandle)
	{
		ExtraAssetBundleAssetPaths.clear();
		AssetBundle_Unload(il2cpp_gchandle_get_target(ExtraAssetBundleHandle), true);
		il2cpp_gchandle_free(ExtraAssetBundleHandle);
		ExtraAssetBundleHandle = 0;
	}
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
