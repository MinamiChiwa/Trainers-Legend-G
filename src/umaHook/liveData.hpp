#pragma once

#include "stdinclude.hpp"

enum class LiveDelegateType {
	UNSET,
	OnUpdatePostEffect_DOF,
	OnUpdatePostFilm,
	OnUpdatePostFilm2,
	OnUpdatePostFilm3,
	OnUpdateLightProjection,
	OnUpdateRadialBlur,
	OnUpdateExposure,
	OnUpdateVortex,
	OnUpdateCharaFootLight,
	OnUpdateGlobalLight
};

namespace LiveData {
	namespace {
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

	std::unordered_map<LiveDelegateType, void*> LiveUpdateInfoDelegates{};
	std::unordered_map<void*, LiveDelegateType> LiveUpdateInfoDelegatesToType{};
	std::unordered_map<LiveDelegateType, std::pair<void*, std::pair<const char*, const char*>>> LiveUpdateInfoDelegatesInvoke{};

	void LiveUpdateInfoDelegates_emplace(LiveDelegateType type, void* ptr) {
		LiveUpdateInfoDelegates.emplace(type, ptr);
		LiveUpdateInfoDelegatesToType.emplace(ptr, type);
	}

	void LiveUpdateInfoDelegates_clear() {
		LiveUpdateInfoDelegates.clear();
		LiveUpdateInfoDelegatesToType.clear();
	}

	void* getDelegate(LiveDelegateType type) {
		if (const auto iter = LiveUpdateInfoDelegates.find(type); iter != LiveUpdateInfoDelegates.end()) {
			return iter->second;
		}
		return NULL;
	}

	LiveDelegateType getDelegateType(void* ptr) {
		if (const auto iter = LiveUpdateInfoDelegatesToType.find(ptr); iter != LiveUpdateInfoDelegatesToType.end()) {
			return iter->second;
		}
		return LiveDelegateType::UNSET;
	}

	bool checkDelegate(LiveDelegateType type, void* ptr) {
		return getDelegate(type) == ptr;
	}

	template <class T>
	class ILiveUpdateInfo {
	public:
		virtual void updateData() = 0;

		LiveDelegateType myType = LiveDelegateType::UNSET;

		void generateUpdateInfo(const char* namespaze, const char* klassName) {
			const auto klass = il2cpp_symbols::get_class("umamusume.dll", namespaze, klassName);
			updateInfo = reinterpret_cast<T*>(il2cpp_object_new(klass));
		}

		void generateUpdateInfo() {
			if (const auto iter = LiveUpdateInfoDelegatesInvoke.find(myType); iter != LiveUpdateInfoDelegatesInvoke.end()) {
				generateUpdateInfo(iter->second.second.first, iter->second.second.second);
			}
		}

		void updateGame(bool reGenerateUpdateInfo = false) {
			auto gameDelegate = getDelegate(myType);
			if (gameDelegate != NULL) {
				if (const auto iter = LiveUpdateInfoDelegatesInvoke.find(myType); iter != LiveUpdateInfoDelegatesInvoke.end()) {
					auto invokeMtd = iter->second.first;
					if (reGenerateUpdateInfo || (!updateInfo)) {
						generateUpdateInfo(iter->second.second.first, iter->second.second.second);
					}

					if (invokeMtd) {
						updateData();
						reinterpret_cast<void (*)(void*, void*)>(invokeMtd)(gameDelegate, updateInfo);
					}
				}
				else {
					printf("UpdateInfoDelegates_Invoke: %d not found!\n", myType);
				}
			}
		}

	protected:
		T* localData;
		T* updateInfo;
		bool condition;
	};

	void* PostEffectUpdateInfo_DOF_klass;
	FieldInfo* PostEffectUpdateInfo_DOF_forcalPosition;
	FieldInfo* PostEffectUpdateInfo_DOF_dofBlurType;
	FieldInfo* PostEffectUpdateInfo_DOF_dofQuality;
	void* PostFilmUpdateInfo_klass;
	FieldInfo* PostFilmUpdateInfo_filmOffsetParam;
	FieldInfo* PostFilmUpdateInfo_layerMode;
	FieldInfo* PostFilmUpdateInfo_colorBlend;
	FieldInfo* PostFilmUpdateInfo_filmOptionParam;
	FieldInfo* PostFilmUpdateInfo_color0;
	FieldInfo* PostFilmUpdateInfo_color1;
	FieldInfo* PostFilmUpdateInfo_color2;
	FieldInfo* PostFilmUpdateInfo_color3;
	FieldInfo* PostFilmUpdateInfo_FilmScale;
	void* LightProjectionUpdateInfo_klass;
	FieldInfo* LightProjectionUpdateInfo_position;
	FieldInfo* LightProjectionUpdateInfo_rotation;
	FieldInfo* LightProjectionUpdateInfo_scale;
	FieldInfo* LightProjectionUpdateInfo_color;
	FieldInfo* LightProjectionUpdateInfo_AnimationScaleUV;
	FieldInfo* LightProjectionUpdateInfo_AnimationOffsetUV;
	void* RadialBlurUpdateInfo_klass;
	FieldInfo* RadialBlurUpdateInfo_radialBlurOffset;
	FieldInfo* RadialBlurUpdateInfo_moveBlurType;
	FieldInfo* RadialBlurUpdateInfo_radialBlurEllipseDir;
	FieldInfo* RadialBlurUpdateInfo_depthCancelRect;
	void* ExposureUpdateInfo_klass;
	FieldInfo* ExposureUpdateInfo_ExposureParameter;
	void* VortexUpdateInfo_klass;
	FieldInfo* VortexUpdateInfo_Area;
	void* CharaFootLightUpdateInfo_klass;
	FieldInfo* CharaFootLightUpdateInfo_lightColor;
	void* GlobalLightUpdateInfo_klass;
	FieldInfo* GlobalLightUpdateInfo_lightRotation;
	FieldInfo* GlobalLightUpdateInfo_rimColor;
	FieldInfo* GlobalLightUpdateInfo_rimColor2;
	FieldInfo* GlobalLightUpdateInfo_flags;

	static bool isffinit = false;
	void init_LiveFieldData() {
		if (isffinit) return;
		PostEffectUpdateInfo_DOF_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "PostEffectUpdateInfo_DOF");
		PostEffectUpdateInfo_DOF_forcalPosition = il2cpp_class_get_field_from_name(PostEffectUpdateInfo_DOF_klass, "forcalPosition");
		PostEffectUpdateInfo_DOF_dofBlurType = il2cpp_class_get_field_from_name(PostEffectUpdateInfo_DOF_klass, "dofBlurType");
		PostEffectUpdateInfo_DOF_dofQuality = il2cpp_class_get_field_from_name(PostEffectUpdateInfo_DOF_klass, "dofQuality");
		
		PostFilmUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "PostFilmUpdateInfo");
		PostFilmUpdateInfo_filmOffsetParam = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "filmOffsetParam");
		PostFilmUpdateInfo_layerMode = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "layerMode");
		PostFilmUpdateInfo_colorBlend = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "colorBlend");
		PostFilmUpdateInfo_filmOptionParam = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "filmOptionParam");
		PostFilmUpdateInfo_color0 = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "color0");
		PostFilmUpdateInfo_color1 = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "color1");
		PostFilmUpdateInfo_color2 = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "color2");
		PostFilmUpdateInfo_color3 = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "color3");
		PostFilmUpdateInfo_FilmScale = il2cpp_class_get_field_from_name(PostFilmUpdateInfo_klass, "FilmScale");

		LightProjectionUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "LightProjectionUpdateInfo");
		LightProjectionUpdateInfo_position = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "position");
		LightProjectionUpdateInfo_rotation = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "rotation");
		LightProjectionUpdateInfo_scale = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "scale");
		LightProjectionUpdateInfo_color = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "color");
		LightProjectionUpdateInfo_AnimationScaleUV = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "AnimationScaleUV");
		LightProjectionUpdateInfo_AnimationOffsetUV = il2cpp_class_get_field_from_name(LightProjectionUpdateInfo_klass, "AnimationOffsetUV");
		
		RadialBlurUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "RadialBlurUpdateInfo");
		RadialBlurUpdateInfo_radialBlurOffset = il2cpp_class_get_field_from_name(RadialBlurUpdateInfo_klass, "radialBlurOffset");
		RadialBlurUpdateInfo_moveBlurType = il2cpp_class_get_field_from_name(RadialBlurUpdateInfo_klass, "moveBlurType");
		RadialBlurUpdateInfo_radialBlurEllipseDir = il2cpp_class_get_field_from_name(RadialBlurUpdateInfo_klass, "radialBlurEllipseDir");
		RadialBlurUpdateInfo_depthCancelRect = il2cpp_class_get_field_from_name(RadialBlurUpdateInfo_klass, "depthCancelRect");
		ExposureUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "ExposureUpdateInfo");
		ExposureUpdateInfo_ExposureParameter = il2cpp_class_get_field_from_name(ExposureUpdateInfo_klass, "ExposureParameter");
		VortexUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "VortexUpdateInfo");
		VortexUpdateInfo_Area = il2cpp_class_get_field_from_name(VortexUpdateInfo_klass, "Area");

		CharaFootLightUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "CharaFootLightUpdateInfo");
		CharaFootLightUpdateInfo_lightColor = il2cpp_class_get_field_from_name(CharaFootLightUpdateInfo_klass, "lightColor");
		
		GlobalLightUpdateInfo_klass = il2cpp_symbols::get_class("umamusume.dll", "Gallop.Live.Cutt", "GlobalLightUpdateInfo");
		GlobalLightUpdateInfo_lightRotation = il2cpp_class_get_field_from_name(GlobalLightUpdateInfo_klass, "lightRotation");
		GlobalLightUpdateInfo_rimColor = il2cpp_class_get_field_from_name(GlobalLightUpdateInfo_klass, "rimColor");
		GlobalLightUpdateInfo_rimColor2 = il2cpp_class_get_field_from_name(GlobalLightUpdateInfo_klass, "rimColor2");
		GlobalLightUpdateInfo_flags = il2cpp_class_get_field_from_name(GlobalLightUpdateInfo_klass, "flags");
		isffinit = true;
	}

	class PostEffectUpdateInfo_DOF : public ILiveUpdateInfo<UmaGUiShowData::PostEffectUpdateInfo_DOF> {
	public:
		PostEffectUpdateInfo_DOF(UmaGUiShowData::PostEffectUpdateInfo_DOF* updateInfo, bool condition) {
			localData = &UmaGUiShowData::postEffectUpdateInfo_DOF;
			this->updateInfo = updateInfo;
			this->condition = condition;
			myType = LiveDelegateType::OnUpdatePostEffect_DOF;
		}

		void updateData() {
			init_LiveFieldData();
			if (!updateInfo) generateUpdateInfo();
			changeValueByType(&localData->dofQuality, &updateInfo->dofQuality, condition);
			auto cDt_t = reinterpret_cast<CsEnum_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostEffectUpdateInfo_DOF_dofBlurType->offset
				);
			changeValueByType(reinterpret_cast<int*>(&localData->dofBlurType), &cDt_t->value__, condition);

			changeValueByType(&localData->IsEnableDOF, &updateInfo->IsEnableDOF, condition);
			changeValueByType(&localData->forcalSize, &updateInfo->forcalSize, condition);
			changeValueByType(&localData->blurSpread, &updateInfo->blurSpread, condition);
			
			changeValueByType(&localData->dofForegroundSize, &updateInfo->dofForegroundSize, condition);
			changeValueByType(&localData->dofFocalPoint, &updateInfo->dofFocalPoint, condition);
			changeValueByType(&localData->dofSoomthness, &updateInfo->dofSoomthness, condition);
			changeValueByType(&localData->isUseFocalPoint, &updateInfo->isUseFocalPoint, condition);
			changeValueByType(&localData->BallBlurCurveFactor, &updateInfo->BallBlurCurveFactor, condition);
			changeValueByType(&localData->BallBlurBrightnessThreshhold, &updateInfo->BallBlurBrightnessThreshhold, condition);
			changeValueByType(&localData->BallBlurBrightnessIntensity, &updateInfo->BallBlurBrightnessIntensity, condition);
			changeValueByType(&localData->BallBlurSpread, &updateInfo->BallBlurSpread, condition);
			changeValueByType(&localData->IsPointBallBlur, &updateInfo->IsPointBallBlur, condition);

			auto forcalPosition = reinterpret_cast<Vector3_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostEffectUpdateInfo_DOF_forcalPosition->offset
				);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.x, &forcalPosition->x, condition);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.y, &forcalPosition->y, condition);
			changeValueByType(&UmaGUiShowData::liveDOFForcalPosition.z, &forcalPosition->z, condition);

		}
	};

	class PostFilmUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::PostFilmUpdateInfo> {
	protected:
		Vector2_t* _filmOffsetParam;
		Vector2_t* _FilmScale;
		Vector4_t* _filmOptionParam;
		Color_t* _filmcolor0;
		Color_t* _filmcolor1;
		Color_t* _filmcolor2;
		Color_t* _filmcolor3;

	public:
		PostFilmUpdateInfo(UmaGUiShowData::PostFilmUpdateInfo* updateInfo, bool condition, int index) {
			localData = &UmaGUiShowData::postFilmUpdateInfo[index];
			this->updateInfo = updateInfo;
			this->condition = condition;

			_filmOffsetParam = &UmaGUiShowData::filmOffsetParam[index];
			_FilmScale = &UmaGUiShowData::FilmScale[index];
			_filmOptionParam = &UmaGUiShowData::filmOptionParam[index];
			_filmcolor0 = &UmaGUiShowData::filmcolor0[index];
			_filmcolor1 = &UmaGUiShowData::filmcolor1[index];
			_filmcolor2 = &UmaGUiShowData::filmcolor2[index];
			_filmcolor3 = &UmaGUiShowData::filmcolor3[index];
		}

		void updateData() {
			init_LiveFieldData();
			changeValueByType(&localData->filmMode, &updateInfo->filmMode, condition);
			changeValueByType(&localData->colorType, &updateInfo->colorType, condition);
			changeValueByType(&localData->filmPower, &updateInfo->filmPower, condition);
			changeValueByType(&localData->depthPower, &updateInfo->depthPower, condition);
			changeValueByType(&localData->DepthClip, &updateInfo->DepthClip, condition);
			changeValueByType(&localData->inverseVignette, &updateInfo->inverseVignette, condition);
			changeValueByType(&localData->colorBlendFactor, &updateInfo->colorBlendFactor, condition);
			changeValueByType(&localData->movieResId, &updateInfo->movieResId, condition);
			changeValueByType(&localData->movieFrameOffset, &updateInfo->movieFrameOffset, condition);
			changeValueByType(&localData->movieTime, &updateInfo->movieTime, condition);
			changeValueByType(&localData->movieReverse, &updateInfo->movieReverse, condition);
			changeValueByType(&localData->RollAngle, &updateInfo->RollAngle, condition);

			auto layerMode = reinterpret_cast<CsEnum_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_layerMode->offset
				);
			changeValueByType(reinterpret_cast<int*>(&localData->layerMode), &layerMode->value__, condition);
			
			auto colorBlend = reinterpret_cast<CsEnum_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_colorBlend->offset
				);
			changeValueByType(reinterpret_cast<int*>(&localData->colorBlend), &colorBlend->value__, condition);
			
			auto filmOffsetParam = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_filmOffsetParam->offset
				);	
			changeValueByType(&_filmOffsetParam->x, &filmOffsetParam->x, condition);
			changeValueByType(&_filmOffsetParam->y, &filmOffsetParam->y, condition);
			
			auto FilmScale = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_FilmScale->offset
				);	
			changeValueByType(&_FilmScale->x, &FilmScale->x, condition);
			changeValueByType(&_FilmScale->y, &FilmScale->y, condition);

			auto filmOptionParam = reinterpret_cast<Vector4_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_filmOptionParam->offset
				);	
			changeValueByType(&_filmOptionParam->x, &filmOptionParam->x, condition);
			changeValueByType(&_filmOptionParam->y, &filmOptionParam->y, condition);
			changeValueByType(&_filmOptionParam->z, &filmOptionParam->z, condition);
			changeValueByType(&_filmOptionParam->w, &filmOptionParam->w, condition);
			
			auto color0 = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_color0->offset
				);
			changeValueByType(&_filmcolor0->r, &color0->r, condition);
			changeValueByType(&_filmcolor0->g, &color0->g, condition);
			changeValueByType(&_filmcolor0->b, &color0->b, condition);
			changeValueByType(&_filmcolor0->a, &color0->a, condition);
			auto color1 = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_color1->offset
				);
			changeValueByType(&_filmcolor1->r, &color1->r, condition);
			changeValueByType(&_filmcolor1->g, &color1->g, condition);
			changeValueByType(&_filmcolor1->b, &color1->b, condition);
			changeValueByType(&_filmcolor1->a, &color1->a, condition);
			auto color2 = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_color2->offset
				);
			changeValueByType(&_filmcolor2->r, &color2->r, condition);
			changeValueByType(&_filmcolor2->g, &color2->g, condition);
			changeValueByType(&_filmcolor2->b, &color2->b, condition);
			changeValueByType(&_filmcolor2->a, &color2->a, condition);
			auto color3 = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + PostFilmUpdateInfo_color3->offset
				);
			changeValueByType(&_filmcolor3->r, &color3->r, condition);
			changeValueByType(&_filmcolor3->g, &color3->g, condition);
			changeValueByType(&_filmcolor3->b, &color3->b, condition);
			changeValueByType(&_filmcolor3->a, &color3->a, condition);
		}
	};

	class LightProjectionUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::LightProjectionUpdateInfo> {
	public:
		LightProjectionUpdateInfo(UmaGUiShowData::LightProjectionUpdateInfo* updateInfo, bool condition) {
			localData = &UmaGUiShowData::lightProjectionUpdateInfo;
			this->updateInfo = updateInfo;
			this->condition = condition;
		}

		void updateData() {
			init_LiveFieldData();
			changeValueByType(&localData->isEnable, &updateInfo->isEnable, condition);
			changeValueByType(&localData->TextureId, &updateInfo->TextureId, condition);
			changeValueByType(&localData->colorPower, &updateInfo->colorPower, condition);
			changeValueByType(&localData->orthographic, &updateInfo->orthographic, condition);
			changeValueByType(&localData->orthographicSize, &updateInfo->orthographicSize, condition);
			changeValueByType(&localData->nearClipPlane, &updateInfo->nearClipPlane, condition);
			changeValueByType(&localData->farClipPlane, &updateInfo->farClipPlane, condition);
			changeValueByType(&localData->fieldOfView, &updateInfo->fieldOfView, condition);
			changeValueByType(&localData->nodeHash, &updateInfo->nodeHash, condition);
			changeValueByType(&localData->CharacterAttachPosition, &updateInfo->CharacterAttachPosition, condition);
			changeValueByType(&localData->IsCharacterAttach, &updateInfo->IsCharacterAttach, condition);
			changeValueByType(&localData->UseMonitorMovie, &updateInfo->UseMonitorMovie, condition);
			changeValueByType(&localData->AnimationTextureId, &updateInfo->AnimationTextureId, condition);
			changeValueByType(&localData->AnimationDivX, &updateInfo->AnimationDivX, condition);
			changeValueByType(&localData->AnimationDivY, &updateInfo->AnimationDivY, condition);
			changeValueByType(&localData->AnimationMaxCut, &updateInfo->AnimationMaxCut, condition);
			changeValueByType(&localData->AnimationTime, &updateInfo->AnimationTime, condition);
			changeValueByType(&localData->ProgressTime, &updateInfo->ProgressTime, condition);
			auto v_position = reinterpret_cast<Vector3_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_position->offset
				);
			changeValueByType(&localData->position.x, &v_position->x, condition);
			changeValueByType(&localData->position.y, &v_position->y, condition);
			changeValueByType(&localData->position.z, &v_position->z, condition);
			auto v_rotation = reinterpret_cast<Quaternion_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_rotation->offset
				);
			changeValueByType(&localData->rotation.x, &v_rotation->x, condition);
			changeValueByType(&localData->rotation.y, &v_rotation->y, condition);
			changeValueByType(&localData->rotation.z, &v_rotation->z, condition);
			changeValueByType(&localData->rotation.w, &v_rotation->w, condition);
			auto v_scale = reinterpret_cast<Vector3_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_scale->offset
				);
			changeValueByType(&localData->scale.x, &v_scale->x, condition);
			changeValueByType(&localData->scale.y, &v_scale->y, condition);
			changeValueByType(&localData->scale.z, &v_scale->z, condition);
			auto v_color = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_color->offset
				);
			changeValueByType(&localData->color.r, &v_color->r, condition);
			changeValueByType(&localData->color.g, &v_color->g, condition);
			changeValueByType(&localData->color.b, &v_color->b, condition);
			changeValueByType(&localData->color.a, &v_color->a, condition);
			auto v_AnimationScaleUV = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_AnimationScaleUV->offset
				);
			changeValueByType(&localData->AnimationScaleUV.x, &v_AnimationScaleUV->x, condition);
			changeValueByType(&localData->AnimationScaleUV.y, &v_AnimationScaleUV->y, condition);
			auto v_AnimationOffsetUV = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + LightProjectionUpdateInfo_AnimationOffsetUV->offset
				);
			changeValueByType(&localData->AnimationOffsetUV.x, &v_AnimationOffsetUV->x, condition);
			changeValueByType(&localData->AnimationOffsetUV.y, &v_AnimationOffsetUV->y, condition);
		}
	};

	class RadialBlurUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::RadialBlurUpdateInfo> {
	public:
		RadialBlurUpdateInfo(UmaGUiShowData::RadialBlurUpdateInfo* updateInfo, bool condition) {
			localData = &UmaGUiShowData::radialBlurUpdateInfo;
			this->updateInfo = updateInfo;
			this->condition = condition;
		}

		void updateData() {
			init_LiveFieldData();
			changeValueByType(&localData->moveBlurType, &updateInfo->moveBlurType, condition);
			changeValueByType(&localData->radialBlurDownsample, &updateInfo->radialBlurDownsample, condition);
			changeValueByType(&localData->radialBlurStartArea, &updateInfo->radialBlurStartArea, condition);
			changeValueByType(&localData->radialBlurEndArea, &updateInfo->radialBlurEndArea, condition);
			changeValueByType(&localData->radialBlurPower, &updateInfo->radialBlurPower, condition);
			changeValueByType(&localData->radialBlurIteration, &updateInfo->radialBlurIteration, condition);
			changeValueByType(&localData->radialBlurRollEulerAngles, &updateInfo->radialBlurRollEulerAngles, condition);
			changeValueByType(&localData->isEnabledDepth, &updateInfo->isEnabledDepth, condition);
			changeValueByType(&localData->depthPowerFront, &updateInfo->depthPowerFront, condition);
			changeValueByType(&localData->depthPowerBack, &updateInfo->depthPowerBack, condition);
			changeValueByType(&localData->isEnabledDepthCancelRect, &updateInfo->isEnabledDepthCancelRect, condition);
			changeValueByType(&localData->depthCancelBlendLength, &updateInfo->depthCancelBlendLength, condition);
			changeValueByType(&localData->isExpandDepthCancelRect, &updateInfo->isExpandDepthCancelRect, condition);

			auto v_radialBlurOffset = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + RadialBlurUpdateInfo_radialBlurOffset->offset
				);
			changeValueByType(&localData->radialBlurOffset.x, &v_radialBlurOffset->x, condition);
			changeValueByType(&localData->radialBlurOffset.y, &v_radialBlurOffset->y, condition);
			auto v_radialBlurEllipseDir = reinterpret_cast<Vector2_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + RadialBlurUpdateInfo_radialBlurEllipseDir->offset
				);
			changeValueByType(&localData->radialBlurEllipseDir.x, &v_radialBlurEllipseDir->x, condition);
			changeValueByType(&localData->radialBlurEllipseDir.y, &v_radialBlurEllipseDir->y, condition);
			auto v_depthCancelRect = reinterpret_cast<Vector4_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + RadialBlurUpdateInfo_depthCancelRect->offset
				);
			changeValueByType(&localData->depthCancelRect.x, &v_depthCancelRect->x, condition);
			changeValueByType(&localData->depthCancelRect.y, &v_depthCancelRect->y, condition);
			changeValueByType(&localData->depthCancelRect.z, &v_depthCancelRect->z, condition);
			changeValueByType(&localData->depthCancelRect.w, &v_depthCancelRect->w, condition);
		}
	};

	class ExposureUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::ExposureUpdateInfo> {
	public:
		ExposureUpdateInfo(UmaGUiShowData::ExposureUpdateInfo* updateInfo, bool condition) {
			localData = &UmaGUiShowData::exposureUpdateInfo;
			this->updateInfo = updateInfo;
			this->condition = condition;
		}

		void updateData() {
			init_LiveFieldData();
			changeValueByType(&localData->IsEnable, &updateInfo->IsEnable, condition);
			changeValueByType(&localData->DepthMask, &updateInfo->DepthMask, condition);
			auto v_ExposureParameter = reinterpret_cast<Vector4_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + ExposureUpdateInfo_ExposureParameter->offset
				);
			changeValueByType(&localData->ExposureParameter.x, &v_ExposureParameter->x, condition);
			changeValueByType(&localData->ExposureParameter.y, &v_ExposureParameter->y, condition);
			changeValueByType(&localData->ExposureParameter.z, &v_ExposureParameter->z, condition);
			changeValueByType(&localData->ExposureParameter.w, &v_ExposureParameter->w, condition);
		}
	};

	class VortexUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::VortexUpdateInfo> {
	public:
		VortexUpdateInfo(UmaGUiShowData::VortexUpdateInfo* updateInfo, bool condition) {
			localData = &UmaGUiShowData::vortexUpdateInfo;
			this->updateInfo = updateInfo;
			this->condition = condition;
		}

		void updateData() {
			init_LiveFieldData();
			changeValueByType(&localData->IsEnable, &updateInfo->IsEnable, condition);
			changeValueByType(&localData->RotVolume, &updateInfo->RotVolume, condition);
			changeValueByType(&localData->DepthClip, &updateInfo->DepthClip, condition);
			auto v_Area = reinterpret_cast<Vector4_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + VortexUpdateInfo_Area->offset
				);
			changeValueByType(&localData->Area.x, &v_Area->x, condition);
			changeValueByType(&localData->Area.y, &v_Area->y, condition);
			changeValueByType(&localData->Area.z, &v_Area->z, condition);
			changeValueByType(&localData->Area.w, &v_Area->w, condition);

		}
	};

	class CharaFootLightUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::CharaFootLightUpdateInfo> {
	public:
		CharaFootLightUpdateInfo(UmaGUiShowData::CharaFootLightUpdateInfo* updateInfo) {
			auto charaIndex = updateInfo->CharacterIndex;
			if (updateInfo->CharacterIndex >= 36) {
				printf("charaIndex: %d over range.\n", updateInfo->CharacterIndex);
				charaIndex = 35;
			}
			localData = &UmaGUiShowData::charaFootLightUpdateInfo[charaIndex];
			this->updateInfo = updateInfo;
			this->condition = localData->followGame;
		}

		void updateData() {
			init_LiveFieldData();
			localData->CharacterIndex = updateInfo->CharacterIndex;
			changeValueByType(&localData->hightMax, &updateInfo->hightMax, condition);
			auto v_lightColor = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + CharaFootLightUpdateInfo_lightColor->offset
				);
			changeValueByType(&localData->lightColor.r, &v_lightColor->r, condition);
			changeValueByType(&localData->lightColor.g, &v_lightColor->g, condition);
			changeValueByType(&localData->lightColor.b, &v_lightColor->b, condition);
			changeValueByType(&localData->lightColor.a, &v_lightColor->a, condition);
		}

		bool getCondition() {
			return condition;
		}
	};

	class GlobalLightUpdateInfo : public ILiveUpdateInfo<UmaGUiShowData::GlobalLightUpdateInfo> {
	private:
		bool isValid = false;

	public:
		static int cacheFrame;
		static int frameIndex;
		static int currentFrame;

		GlobalLightUpdateInfo(UmaGUiShowData::GlobalLightUpdateInfo* updateInfo) {
			init_LiveFieldData();
			UmaGUiShowData::initGuiGlobalData();
			this->updateInfo = updateInfo;
			auto charaIndex = checkIndex();

			if (const auto iter = UmaGUiShowData::globalLightUpdateInfo.find(charaIndex); iter != UmaGUiShowData::globalLightUpdateInfo.end()) {
				localData = &iter->second;
				isValid = true;
				this->condition = localData->followGame;
			}
			else {
				isValid = false;
			}
		}

		int checkIndex() {
			auto cInd_t = reinterpret_cast<CsEnum_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + GlobalLightUpdateInfo_flags->offset
				);
			auto charaIndex = cInd_t->value__;

			if (currentFrame == cacheFrame) {
				frameIndex++;
			}
			else {
				cacheFrame = currentFrame;
				frameIndex = 0;
			}

			if (charaIndex == 0x0) {
				charaIndex = UmaGUiShowData::liveCharaPositionFlag.GetValue(frameIndex).second;
			}
			return charaIndex;

		}

		void updateData() {
			if (!isValid) return;

			auto cInd_t = reinterpret_cast<CsEnum_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + GlobalLightUpdateInfo_flags->offset
				);
			changeValueByType(&localData->flags, &cInd_t->value__, condition);

			changeValueByType(&localData->globalRimShadowRate, &updateInfo->globalRimShadowRate, condition);
			changeValueByType(&localData->rimStep, &updateInfo->rimStep, condition);
			changeValueByType(&localData->rimFeather, &updateInfo->rimFeather, condition);
			changeValueByType(&localData->rimSpecRate, &updateInfo->rimSpecRate, condition);
			changeValueByType(&localData->RimHorizonOffset, &updateInfo->RimHorizonOffset, condition);
			changeValueByType(&localData->RimVerticalOffset, &updateInfo->RimVerticalOffset, condition);
			changeValueByType(&localData->RimHorizonOffset2, &updateInfo->RimHorizonOffset2, condition);
			changeValueByType(&localData->RimVerticalOffset2, &updateInfo->RimVerticalOffset2, condition);
			changeValueByType(&localData->rimStep2, &updateInfo->rimStep2, condition);
			changeValueByType(&localData->rimFeather2, &updateInfo->rimFeather2, condition);
			changeValueByType(&localData->rimSpecRate2, &updateInfo->rimSpecRate2, condition);
			changeValueByType(&localData->globalRimShadowRate2, &updateInfo->globalRimShadowRate2, condition);
			auto v_lightRotation = reinterpret_cast<Quaternion_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + GlobalLightUpdateInfo_lightRotation->offset
				);
			changeValueByType(&localData->lightRotation.x, &v_lightRotation->x, condition);
			changeValueByType(&localData->lightRotation.y, &v_lightRotation->y, condition);
			changeValueByType(&localData->lightRotation.z, &v_lightRotation->z, condition);
			changeValueByType(&localData->lightRotation.w, &v_lightRotation->w, condition);
			auto v_rimColor = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + GlobalLightUpdateInfo_rimColor->offset
				);
			changeValueByType(&localData->rimColor.r, &v_rimColor->r, condition);
			changeValueByType(&localData->rimColor.g, &v_rimColor->g, condition);
			changeValueByType(&localData->rimColor.b, &v_rimColor->b, condition);
			changeValueByType(&localData->rimColor.a, &v_rimColor->a, condition);
			auto v_rimColor2 = reinterpret_cast<Color_t*>(
				static_cast<std::byte*>(reinterpret_cast<void*>(updateInfo)) + GlobalLightUpdateInfo_rimColor2->offset
				);
			changeValueByType(&localData->rimColor2.r, &v_rimColor2->r, condition);
			changeValueByType(&localData->rimColor2.g, &v_rimColor2->g, condition);
			changeValueByType(&localData->rimColor2.b, &v_rimColor2->b, condition);
			changeValueByType(&localData->rimColor2.a, &v_rimColor2->a, condition);
		}

		bool getCondition() {
			return condition;
		}
	};
	int GlobalLightUpdateInfo::cacheFrame = 0;
	int GlobalLightUpdateInfo::frameIndex = 0;
	int GlobalLightUpdateInfo::currentFrame = 0;

}
