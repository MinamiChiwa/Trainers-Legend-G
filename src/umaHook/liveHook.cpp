#include "stdinclude.hpp"

#define CREATE_HOOK(assemblyName, namespaze, klassName, fname, argsCount, _name_) \
	auto _name_##_addr = reinterpret_cast<void*>(il2cpp_symbols::get_method_pointer(assemblyName, namespaze, klassName, fname, argsCount)); \
	const auto _name_##_stat1 = MH_CreateHook(_name_##_addr, _name_##_hook, &_name_##_orig); \
	const auto _name_##_stat2 = MH_EnableHook(_name_##_addr); \
	printf("%s.%s::%s(%d) at %p, (%s, %s)\n", namespaze, klassName, fname, argsCount, _name_##_addr, MH_StatusToString(_name_##_stat1), MH_StatusToString(_name_##_stat2));

#define CAST_FUNC(_name_) reinterpret_cast<decltype(_name_##_hook)*>(_name_##_orig)


namespace UmaLiveHook {
	namespace {
		enum class UpdateDOFType {
			MainCamera,
			MuitiCamera,
			Unknown
		};
		UpdateDOFType updateDOFType = UpdateDOFType::Unknown;

		template <typename T>
		void changeValueByType(T* p1, T* p2, bool condition) {
			if (condition) {
				*p1 = *p2;
			}
			else {
				*p2 = *p1;
			}
		}
	}

	void* SetupBloomDiffusionUpdateInfo_orig;
	void SetupBloomDiffusionUpdateInfo_hook(void* _this, void* updateInfo, void* curData, void* nextData, int currentFrame) {
		return CAST_FUNC(SetupBloomDiffusionUpdateInfo)(_this, updateInfo, curData, nextData, currentFrame);
	}

	void* SetupPostFilmUpdateDataInfo_orig;
	void SetupPostFilmUpdateDataInfo_hook(void* _this, void* updateInfo, void* curData, void* nextData, int currentFrame) {
		return CAST_FUNC(SetupPostFilmUpdateDataInfo)(_this, updateInfo, curData, nextData, currentFrame);
	}

	void* PostEffectUpdateInfo_DOF_klass;
	FieldInfo* PostEffectUpdateInfo_DOF_forcalPosition;

	bool isffinit = false;
	void init_LiveTimelineKeyPostEffectDOFData() {
		if (isffinit) return;
		PostEffectUpdateInfo_DOF_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "PostEffectUpdateInfo_DOF");
		PostEffectUpdateInfo_DOF_forcalPosition = il2cpp_class_get_field_from_name(PostEffectUpdateInfo_DOF_klass, "forcalPosition");
		isffinit = true;
	}

	void* SetupDOFUpdateInfo_orig;
	void SetupDOFUpdateInfo_hook(void* _this, UmaGUiShowData::PostEffectUpdateInfo_DOF* updateInfo, void* curData, void* nextData, int currentFrame, Vector3_t* cameraLookAt) {

		if (g_enable_live_dof_controller && guiStarting && GetShowLiveWnd() && (updateDOFType == UpdateDOFType::MainCamera)) {
			init_LiveTimelineKeyPostEffectDOFData();

			auto forcalPosition = reinterpret_cast<Vector3_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostEffectUpdateInfo_DOF_forcalPosition->offset
				);

			if (UmaGUiShowData::dofColtrollerFollowGame) {
				reinterpret_cast<decltype(SetupDOFUpdateInfo_hook)*>(SetupDOFUpdateInfo_orig)(_this, updateInfo, curData, nextData, currentFrame, cameraLookAt);
			}
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.IsEnableDOF, &updateInfo->IsEnableDOF, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.forcalSize, &updateInfo->forcalSize, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.blurSpread, &updateInfo->blurSpread, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.x, &forcalPosition->x, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.y, &forcalPosition->y, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.z, &forcalPosition->z, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.dofQuality, &updateInfo->dofQuality, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.dofBlurType, &updateInfo->dofBlurType, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.dofForegroundSize, &updateInfo->dofForegroundSize, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.dofFocalPoint, &updateInfo->dofFocalPoint, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.dofSoomthness, &updateInfo->dofSoomthness, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.isUseFocalPoint, &updateInfo->isUseFocalPoint, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurCurveFactor, &updateInfo->BallBlurCurveFactor, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessThreshhold, &updateInfo->BallBlurBrightnessThreshhold, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurBrightnessIntensity, &updateInfo->BallBlurBrightnessIntensity, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.BallBlurSpread, &updateInfo->BallBlurSpread, UmaGUiShowData::dofColtrollerFollowGame);
			changeValueByType(&UmaGUiShowData::postEffectUpdateInfo_DOF.IsPointBallBlur, &updateInfo->IsPointBallBlur, UmaGUiShowData::dofColtrollerFollowGame);
			return;
		}

		if (g_live_close_all_blur) return;
		reinterpret_cast<decltype(SetupDOFUpdateInfo_hook)*>(SetupDOFUpdateInfo_orig)(_this, updateInfo, curData, nextData, currentFrame, cameraLookAt);
	}

	void* AlterUpdate_PostEffect_DOF_orig;
	void AlterUpdate_PostEffect_DOF_hook(void* _this, void* sheet, int currentFrame, Vector3_t* cameraLookAt) {
		updateDOFType = UpdateDOFType::MainCamera;
		return reinterpret_cast<decltype(AlterUpdate_PostEffect_DOF_hook)*>(AlterUpdate_PostEffect_DOF_orig)(_this, sheet, currentFrame, cameraLookAt);
	}

	void* AlterUpdate_MultiCameraPostEffect_DOF_orig;
	void AlterUpdate_MultiCameraPostEffect_DOF_hook(void* _this, Il2CppObject* sheet, int currentFrame) {
		updateDOFType = UpdateDOFType::MuitiCamera;
		return CAST_FUNC(AlterUpdate_MultiCameraPostEffect_DOF)(_this, sheet, currentFrame);
	}

	void regHookMain() {
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "SetupBloomDiffusionUpdateInfo", 4, 
			SetupBloomDiffusionUpdateInfo);		
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "SetupPostFilmUpdateDataInfo", 4, 
			SetupPostFilmUpdateDataInfo);
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "SetupDOFUpdateInfo", 5,
			SetupDOFUpdateInfo);
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "AlterUpdate_MultiCameraPostEffect_DOF", 2,
			AlterUpdate_MultiCameraPostEffect_DOF);
		CREATE_HOOK("umamusume.dll", "Gallop.Live.Cutt", "LiveTimelineControl", "AlterUpdate_PostEffect_DOF", 3,
			AlterUpdate_PostEffect_DOF);
	}
}
