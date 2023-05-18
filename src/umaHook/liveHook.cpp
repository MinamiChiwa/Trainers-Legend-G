#include "stdinclude.hpp"
#include "liveData.hpp"

#define CREATE_HOOK(assemblyName, namespaze, klassName, fname, argsCount, _name_) \
	auto _name_##_addr = reinterpret_cast<void*>(il2cpp_symbols::get_method_pointer(assemblyName, namespaze, klassName, fname, argsCount)); \
	const auto _name_##_stat1 = MH_CreateHook(_name_##_addr, _name_##_hook, &_name_##_orig); \
	const auto _name_##_stat2 = MH_EnableHook(_name_##_addr); \
	printf("%s.%s::%s(%d) at %p, (%s, %s)\n", namespaze, klassName, fname, argsCount, _name_##_addr, MH_StatusToString(_name_##_stat1), MH_StatusToString(_name_##_stat2));

#define CAST_FUNC(_name_) reinterpret_cast<decltype(_name_##_hook)*>(_name_##_orig)


namespace UmaLiveHook {
	Il2CppString* (*environment_get_stacktrace)();

	void* DOFUpdateInfoDelegate_orig;
	void DOFUpdateInfoDelegate_hook(void* _this, UmaGUiShowData::PostEffectUpdateInfo_DOF* updateInfo) {

		if (g_live_close_all_blur) {
			updateInfo->IsEnableDOF = false;
		}

		if (g_enable_live_dof_controller && guiStarting && GetShowLiveWnd()) {
			if (LiveData::checkDelegate(LiveDelegateType::OnUpdatePostEffect_DOF, _this)) {
				if (UmaGUiShowData::dofColtrollerFollowGame) {
					CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
				}

				LiveData::PostEffectUpdateInfo_DOF(updateInfo, UmaGUiShowData::dofColtrollerFollowGame).updateData();
			}
		}

		return CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
	}

	bool updatePostFilm = false;

	void* SetupPostFilmUpdateDataInfo_orig;
	void SetupPostFilmUpdateDataInfo_hook(void* _this, UmaGUiShowData::PostFilmUpdateInfo* updateInfo, void* curData, void* nextData, int currentFrame) {
		if (g_enable_live_dof_controller && updatePostFilm && guiStarting && GetShowLiveWnd()) {
			if (UmaGUiShowData::livePostFilmFollowGame[UmaGUiShowData::filmIndex]) {
				CAST_FUNC(SetupPostFilmUpdateDataInfo)(_this, updateInfo, curData, nextData, currentFrame);
			}
			LiveData::PostFilmUpdateInfo(updateInfo, UmaGUiShowData::livePostFilmFollowGame[UmaGUiShowData::filmIndex],
				UmaGUiShowData::filmIndex).updateData();
		}
		return CAST_FUNC(SetupPostFilmUpdateDataInfo)(_this, updateInfo, curData, nextData, currentFrame);
	}

	FieldInfo* OnUpdatePostFilm_field;
	FieldInfo* OnUpdatePostFilm2_field;
	FieldInfo* OnUpdatePostFilm3_field;

	void* AlterUpdate_PostFilm_orig;
	void AlterUpdate_PostFilm_hook(void* _this, void* postFilmKeys, int currentFrame, void* fnUpdatePostFilm) {
		updatePostFilm = true;
		if (il2cpp_symbols::read_field(_this, OnUpdatePostFilm_field) == fnUpdatePostFilm) {
			UmaGUiShowData::filmIndex = 0;
		}
		else if (il2cpp_symbols::read_field(_this, OnUpdatePostFilm2_field) == fnUpdatePostFilm) {
			UmaGUiShowData::filmIndex = 1;
		}
		else if (il2cpp_symbols::read_field(_this, OnUpdatePostFilm3_field) == fnUpdatePostFilm) {
			UmaGUiShowData::filmIndex = 2;
		}
		CAST_FUNC(AlterUpdate_PostFilm)(_this, postFilmKeys, currentFrame, fnUpdatePostFilm);
		updatePostFilm = false;
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

			LiveData::LiveUpdateInfoDelegates.clear();
			LiveData::LiveUpdateInfoDelegates.emplace(LiveDelegateType::OnUpdatePostEffect_DOF, OnUpdatePostEffect_DOF);
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
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "SetupPostFilmUpdateDataInfo", 4,
			SetupPostFilmUpdateDataInfo);
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "AlterUpdate_PostFilm", 3,
			AlterUpdate_PostFilm);

		CREATE_HOOK("umamusume.dll", "Gallop.Live", "StageController", "SetupLiveTimelineControl", 1,
			SetupLiveTimelineControl);

		initUpdateInfoDelegateInvoke();  // HOOK 完成之后再调用此函数
	}
}
