#include "stdinclude.hpp"
#include "liveData.hpp"

#define CREATE_HOOK(assemblyName, namespaze, klassName, fname, argsCount, _name_) \
	auto _name_##_addr = reinterpret_cast<void*>(il2cpp_symbols::get_method_pointer(assemblyName, namespaze, klassName, fname, argsCount)); \
	const auto _name_##_stat1 = MH_CreateHook(_name_##_addr, _name_##_hook, &_name_##_orig); \
	const auto _name_##_stat2 = MH_EnableHook(_name_##_addr); \
	printf("%s.%s::%s(%d) at %p, (%s, %s)\n", namespaze, klassName, fname, argsCount, _name_##_addr, MH_StatusToString(_name_##_stat1), MH_StatusToString(_name_##_stat2));

#define CAST_FUNC(_name_) reinterpret_cast<decltype(_name_##_hook)*>(_name_##_orig)


namespace UmaLiveHook {
	namespace {
		FieldInfo* OnUpdatePostFilm_field;
		FieldInfo* OnUpdatePostFilm2_field;
		FieldInfo* OnUpdatePostFilm3_field;
	}

	Il2CppString* (*environment_get_stacktrace)();

	void* DOFUpdateInfoDelegate_orig;
	void DOFUpdateInfoDelegate_hook(void* _this, void* updateInfo) {

		if (g_live_close_all_blur) {
			if (LiveData::checkDelegate(LiveDelegateType::OnUpdatePostEffect_DOF, _this)) {
				static_cast<UmaGUiShowData::PostEffectUpdateInfo_DOF*>(updateInfo)->IsEnableDOF = false;
			}
		}

		if (g_enable_live_dof_controller && guiStarting && GetShowLiveWnd()) {
			switch (LiveData::getDelegateType(_this)) {
			case LiveDelegateType::OnUpdatePostEffect_DOF: {
				if (g_live_close_all_blur) {
					static_cast<UmaGUiShowData::PostEffectUpdateInfo_DOF*>(updateInfo)->IsEnableDOF = false;
				}
				if (UmaGUiShowData::dofColtrollerFollowGame) {
					CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
				}

				LiveData::PostEffectUpdateInfo_DOF(static_cast<UmaGUiShowData::PostEffectUpdateInfo_DOF*>(updateInfo),
					UmaGUiShowData::dofColtrollerFollowGame).updateData();
			}; break;

			case LiveDelegateType::OnUpdatePostFilm1: UmaGUiShowData::filmIndex = 0; goto UpdatePostFilm;
			case LiveDelegateType::OnUpdatePostFilm2: UmaGUiShowData::filmIndex = 1; goto UpdatePostFilm;
			case LiveDelegateType::OnUpdatePostFilm3: {
				UmaGUiShowData::filmIndex = 2;
				UpdatePostFilm:
				if (UmaGUiShowData::livePostFilmFollowGame[UmaGUiShowData::filmIndex]) {
					CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
				}
				LiveData::PostFilmUpdateInfo(static_cast<UmaGUiShowData::PostFilmUpdateInfo*>(updateInfo),
					UmaGUiShowData::livePostFilmFollowGame[UmaGUiShowData::filmIndex],
					UmaGUiShowData::filmIndex).updateData();
			}; break;
			default: break;
			}
		}
		return CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
	}

	void* SetupLiveTimelineControl_orig;
	void SetupLiveTimelineControl_hook(void* _this, void* liveTimelineControl) {
		CAST_FUNC(SetupLiveTimelineControl)(_this, liveTimelineControl);
		
		if (g_enable_live_dof_controller) {
			auto liveTimelineControl_klass = il2cpp_symbols::get_class_from_instance(liveTimelineControl);
			FieldInfo* OnUpdatePostEffect_DOF_field = il2cpp_class_get_field_from_name(liveTimelineControl_klass, "OnUpdatePostEffect_DOF");
			auto OnUpdatePostEffect_DOF = il2cpp_symbols::read_field(liveTimelineControl, OnUpdatePostEffect_DOF_field);

			OnUpdatePostFilm_field = il2cpp_class_get_field_from_name(liveTimelineControl_klass, "OnUpdatePostFilm");
			OnUpdatePostFilm2_field = il2cpp_class_get_field_from_name(liveTimelineControl_klass, "OnUpdatePostFilm2");
			OnUpdatePostFilm3_field = il2cpp_class_get_field_from_name(liveTimelineControl_klass, "OnUpdatePostFilm3");

			LiveData::LiveUpdateInfoDelegates_clear();
			LiveData::LiveUpdateInfoDelegates_emplace(LiveDelegateType::OnUpdatePostEffect_DOF, OnUpdatePostEffect_DOF);
			LiveData::LiveUpdateInfoDelegates_emplace(LiveDelegateType::OnUpdatePostFilm1, il2cpp_symbols::read_field(liveTimelineControl, OnUpdatePostFilm_field));
			LiveData::LiveUpdateInfoDelegates_emplace(LiveDelegateType::OnUpdatePostFilm2, il2cpp_symbols::read_field(liveTimelineControl, OnUpdatePostFilm2_field));
			LiveData::LiveUpdateInfoDelegates_emplace(LiveDelegateType::OnUpdatePostFilm3, il2cpp_symbols::read_field(liveTimelineControl, OnUpdatePostFilm3_field));
		}
	}

	void initUpdateInfoDelegateInvoke() {
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdatePostEffect_DOF,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "PostEffectUpdateInfo_DOF")));
	}

	void regHookMain() {
		environment_get_stacktrace = reinterpret_cast<decltype(environment_get_stacktrace)>(il2cpp_symbols::get_method_pointer("mscorlib.dll", "System", "Environment", "get_StackTrace", 0));

		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "DOFUpdateInfoDelegate", "Invoke", 1,
			DOFUpdateInfoDelegate);

		CREATE_HOOK("umamusume.dll", "Gallop.Live", "StageController", "SetupLiveTimelineControl", 1,
			SetupLiveTimelineControl);

		initUpdateInfoDelegateInvoke();  // HOOK 完成之后再调用此函数
	}
}
