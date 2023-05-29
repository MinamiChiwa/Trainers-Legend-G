#include "stdinclude.hpp"
#include "liveData.hpp"

#define CREATE_HOOK(assemblyName, namespaze, klassName, fname, argsCount, _name_) \
	auto _name_##_addr = reinterpret_cast<void*>(il2cpp_symbols::get_method_pointer(assemblyName, namespaze, klassName, fname, argsCount)); \
	const auto _name_##_stat1 = MH_CreateHook(_name_##_addr, _name_##_hook, &_name_##_orig); \
	const auto _name_##_stat2 = MH_EnableHook(_name_##_addr); \
	printf("%s.%s::%s(%d) at %p, (%s, %s)\n", namespaze, klassName, fname, argsCount, _name_##_addr, MH_StatusToString(_name_##_stat1), MH_StatusToString(_name_##_stat2));

#define CAST_FUNC(_name_) reinterpret_cast<decltype(_name_##_hook)*>(_name_##_orig)

#define CHECK_AND_UPDATE_INFO(followGame, updateInfoClass) if (UmaGUiShowData::followGame) {CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);}\
	LiveData::updateInfoClass(static_cast<UmaGUiShowData::updateInfoClass*>(updateInfo),\
	UmaGUiShowData::followGame).updateData()\


namespace UmaLiveHook {
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

			case LiveDelegateType::OnUpdatePostFilm: UmaGUiShowData::filmIndex = 0; goto UpdatePostFilm;
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

			case LiveDelegateType::OnUpdateLightProjection: {
				CHECK_AND_UPDATE_INFO(liveLightProjectionFollowGame, LightProjectionUpdateInfo);
			}; break;
			case LiveDelegateType::OnUpdateRadialBlur: {
				CHECK_AND_UPDATE_INFO(liveRadialBlurFollowGame, RadialBlurUpdateInfo);
			}; break;
			case LiveDelegateType::OnUpdateExposure: {
				CHECK_AND_UPDATE_INFO(liveExposureFollowGame, ExposureUpdateInfo);
			}; break;
			case LiveDelegateType::OnUpdateVortex: {
				CHECK_AND_UPDATE_INFO(liveVortexFollowGame, VortexUpdateInfo);
			}; break;
			case LiveDelegateType::OnUpdateCharaFootLight: {
				LiveData::CharaFootLightUpdateInfo charaFLUpdate(static_cast<UmaGUiShowData::CharaFootLightUpdateInfo*>(updateInfo));
				if (charaFLUpdate.getCondition()) {
					CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
				}
				charaFLUpdate.updateData();
			}; break;
			case LiveDelegateType::OnUpdateGlobalLight: {
				LiveData::GlobalLightUpdateInfo charaGLUpdate(static_cast<UmaGUiShowData::GlobalLightUpdateInfo*>(updateInfo));
				if (charaGLUpdate.getCondition()) {
					CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
				}
				charaGLUpdate.updateData();
			}; break;
			

			default: break;
			}
		}
		return CAST_FUNC(DOFUpdateInfoDelegate)(_this, updateInfo);
	}

	void* AlterUpdate_GlobalLight_orig;
	void AlterUpdate_GlobalLight_hook(void* _this, void* sheet, int currentFrame) {
		LiveData::GlobalLightUpdateInfo::currentFrame = currentFrame;
		return CAST_FUNC(AlterUpdate_GlobalLight)(_this, sheet, currentFrame);
	}

	void* SetupLiveTimelineControl_orig;
	void SetupLiveTimelineControl_hook(void* _this, void* liveTimelineControl) {
		CAST_FUNC(SetupLiveTimelineControl)(_this, liveTimelineControl);

#define EmplaceLiveDelegates(_name_) \
	FieldInfo* _name_##_field = il2cpp_class_get_field_from_name(liveTimelineControl_klass, #_name_); \
	LiveData::LiveUpdateInfoDelegates_emplace(LiveDelegateType::##_name_, il2cpp_symbols::read_field(liveTimelineControl, _name_##_field))

		if (g_enable_live_dof_controller) {
			auto liveTimelineControl_klass = il2cpp_symbols::get_class_from_instance(liveTimelineControl);
			LiveData::LiveUpdateInfoDelegates_clear();
			EmplaceLiveDelegates(OnUpdatePostEffect_DOF);
			EmplaceLiveDelegates(OnUpdatePostFilm);
			EmplaceLiveDelegates(OnUpdatePostFilm2);
			EmplaceLiveDelegates(OnUpdatePostFilm3);
			EmplaceLiveDelegates(OnUpdateLightProjection);
			EmplaceLiveDelegates(OnUpdateRadialBlur);
			EmplaceLiveDelegates(OnUpdateExposure);
			EmplaceLiveDelegates(OnUpdateVortex);
			EmplaceLiveDelegates(OnUpdateCharaFootLight);
			EmplaceLiveDelegates(OnUpdateGlobalLight);
		}
	}

	void initUpdateInfoDelegateInvoke() {
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdatePostEffect_DOF,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "PostEffectUpdateInfo_DOF")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateLightProjection,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "LightProjectionUpdateInfo")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateRadialBlur,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "RadialBlurUpdateInfo")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateExposure,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "ExposureUpdateInfo")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateVortex,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "VortexUpdateInfo")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateCharaFootLight,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "CharaFootLightUpdateInfo")));
		LiveData::LiveUpdateInfoDelegatesInvoke.emplace(LiveDelegateType::OnUpdateGlobalLight,
			std::make_pair(DOFUpdateInfoDelegate_orig, std::make_pair("Gallop.Live.Cutt", "GlobalLightUpdateInfo")));
	}

	void regHookMain() {
		environment_get_stacktrace = reinterpret_cast<decltype(environment_get_stacktrace)>(il2cpp_symbols::get_method_pointer("mscorlib.dll", "System", "Environment", "get_StackTrace", 0));

		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "DOFUpdateInfoDelegate", "Invoke", 1,
			DOFUpdateInfoDelegate);
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "AlterUpdate_GlobalLight", 2,
			AlterUpdate_GlobalLight);

		CREATE_HOOK("umamusume.dll", "Gallop.Live", "StageController", "SetupLiveTimelineControl", 1,
			SetupLiveTimelineControl);

		initUpdateInfoDelegateInvoke();  // HOOK 完成之后再调用此函数
	}
}
