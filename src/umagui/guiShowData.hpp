#pragma once
#include "stdinclude.hpp"

extern bool ignoreNegativeSpeed;

template<typename T>
class UmaEnum {
private:
	std::vector<std::string> names;
	std::vector<T> values;
	int nowIndex = 0;
	int totalSize = -1;

public:
	UmaEnum(std::vector<std::string> names, std::vector<T> values) {
		if (names.size() != values.size()) return;
		assert(names.size() > 0);
		this->totalSize = names.size();
		this->names = names;
		this->values = values;
	}

	std::vector<std::pair<std::string, T>> GetAllKV() {
		std::vector<std::pair<std::string, T>> ret{};
		for (int i = 0; i < names.size(); i++) {
			ret.push_back(std::make_pair(names[i], values[i]));
		}
		return ret;
	}

	std::string GetCurrentName() {
		return names[nowIndex];
	}

	T GetCurrentValue() {
		return values[nowIndex];
	}

	int GetCurrentIndex() {
		return nowIndex;
	}

	std::pair<std::string, T> GetNowValue() {
		return std::make_pair(GetCurrentName(), GetCurrentValue());
	}

	std::pair<std::string, T> Next() {
		int cIndex = nowIndex;
		cIndex++;
		if (cIndex >= names.size()) cIndex = 0;
		nowIndex = cIndex;
		return GetNowValue();
	}

	std::pair<std::string, T> Last() {
		int cIndex = nowIndex;
		cIndex--;
		if (cIndex < 0) cIndex = names.size() - 1;
		nowIndex = cIndex;
		return GetNowValue();
	}

	void SetIndex(int value) {
		if (value >= names.size()) {
			value = names.size() - 1;
		}
		nowIndex = value;
	}

	std::vector<std::string> GetNames() {
		return names;
	}

	char** GetNames_c() {
		char** result = new char* [names.size()];
		for (int i = 0; i < names.size(); i++) {
			result[i] = new char[names[i].length() + 1]();
			strcpy(result[i], names[i].c_str());

		}
		return result;
	}

	std::vector<T> GetValues() {
		return values;
	}

	std::pair<std::string, T> GetValue(int index) {
		if (index >= names.size()) {
			index = names.size() - 1;
		}
		return std::make_pair(names[index], values[index]);
	}

	int GetSize() {
		return totalSize;
	}

};

namespace UmaGUiShowData {

	class UmaRaceMotionData {
	public:
		int gateNo;
		std::string charaName;
		std::string trainerName;
		float speed = 0.0f;
		float rate = 0.0f;
		float RaceBaseSpeed = 0.0f;
		float MinSpeed = 0.0f;
		float StartDashSpeedThreshold = 0.0f;
		bool IsOverRun = false;
		float Hp = 0.0f;
		float MaxHp = 0.0f;
		float HpPer = 0.0f;
		int NearHorseCount = 0;
		float CongestionTime = 0;
		int RawSpeed = 0;
		float BaseSpeed = 0;
		float Speed = 0;
		int RawStamina = 0;
		float BaseStamina = 0;
		float Stamina = 0;
		int RawPow = 0;
		float BasePow = 0;
		float Pow = 0;
		int RawGuts = 0;
		float BaseGuts = 0;
		float Guts = 0;
		int RawWiz = 0;
		float BaseWiz = 0;
		float Wiz = 0;
		int ActivateSkillCount = 0;
		bool IsStartDash = false;
		float lastSpeed = 0;
		float MoveDistance = 0.0f;
		float distance = 0.0f;
		int rank = 0;
		float deltatime;

		float distanceFront = 0.0f;
		float distanceFirst = 0.0f;
		int finallyRank = -1;

		float LastSpurtStartDistance;
		bool isLastSpurt = false;

		UmaRaceMotionData(int gateNo, std::wstring charaName, std::wstring trainerName);
		void UpdateMotionData(float speed, float rate, float RaceBaseSpeed, float MinSpeed, float StartDashSpeedThreshold,
			bool IsOverRun, float Hp, float MaxHp, float HpPer, int NearHorseCount, float CongestionTime, int RawSpeed,
			float BaseSpeed, float Speed, int RawStamina, float BaseStamina, float Stamina, int RawPow, float BasePow, float Pow,
			int RawGuts, float BaseGuts, float Guts, int RawWiz, float BaseWiz, float Wiz, bool IsStartDash, int ActivateSkillCount,
			float lastSpeed, float MoveDistance, float distance, float deltatime, float LastSpurtStartDistance, bool isLastSpurt);

		void setRank(int rank);
		void setFrontAndFirstDist(float distanceFront, float distanceFirst);
		void setFinallyRank(int rank);

	};

	class SkillAbility {
	public:
		int abilityType;
		float effValue;
		std::vector<UmaRaceMotionData> targets;

		SkillAbility(int abilityType, float effValue);
		void addTargets(UmaRaceMotionData target);

	};

	class SkillEventData {
	public:
		int skillLevel;
		int rarity;
		int gradeValue;
		float cooldownTime = 0.0f;
		std::string skillName = "";
		std::vector<SkillAbility> skillAbilities;

		std::string horseName;
		std::string tName;
		int currGateNo;

		SkillEventData(int skillLevel, std::string horseName, std::string tName, int currGateNo, int rarity, int gradeValue);

		void updateCoolDownTime(float cooldownTime);
		void addSkillAbilities(SkillAbility skillAbility);
		void SetSkillName(std::string skillName);
	};

	enum class DofQuality {
		OnlyBackground = 0x1,
		BackgroundAndForeground = 0x5
	};

	enum class DofBlur {
		Horizon = 0x0,
		Mixed = 0x1,
		Disc = 0x2,
		BallBlur = 0x3,
	};

	struct PostEffectUpdateInfo_DOF {
		bool IsEnableDOF;
		float forcalSize;
		float blurSpread;
		// Vector3_t* forcalPosition;
		DofQuality dofQuality;
		DofBlur dofBlurType;
		float dofForegroundSize;
		float dofFocalPoint;
		float dofSoomthness;
		bool isUseFocalPoint;
		float BallBlurCurveFactor;
		float BallBlurBrightnessThreshhold;
		float BallBlurBrightnessIntensity;
		float BallBlurSpread;
		bool IsPointBallBlur;
	};

	enum class PostFilmMode {
		None = 0x0,
		Lerp = 0x1,
		Add = 0x2,
		Mul = 0x3,
		VignetteLerp = 0x4,
		VignetteAdd = 0x5,
		VignetteMul = 0x6,
		Monochrome = 0x7,
		ScreenBlend = 0x8,
		VignetteScreenBlend = 0x9
	};

	enum class PostColorType {
		ColorAll = 0x0,
		Color2TopBottom = 0x1,
		Color2LeftRight = 0x2,
		Color4 = 0x3
	};

	enum class LayerMode {
		Color = 0x0,
		UVMovie = 0x1
	};

	enum class ColorBlend {
		None = 0x0,
		Lerp = 0x1,
		Additive = 0x2,
		Multiply = 0x3,
	};

	struct PostFilmUpdateInfo {
		PostFilmMode filmMode;
		PostColorType colorType;
		float filmPower;
		float depthPower;
		float DepthClip;
		LayerMode layerMode;
		ColorBlend colorBlend;
		bool inverseVignette;
		float colorBlendFactor;
		int movieResId;
		int movieFrameOffset;
		float movieTime;
		bool movieReverse;
		float RollAngle;
	};

	struct LightProjectionUpdateInfo {
		bool isEnable;
		int TextureId;
		float colorPower;
		bool orthographic;
		float orthographicSize;
		float nearClipPlane;
		float farClipPlane;
		float fieldOfView;
		int nodeHash;
		int CharacterAttachPosition;
		bool IsCharacterAttach;
		bool UseMonitorMovie;
		int AnimationTextureId;
		int AnimationDivX;
		int AnimationDivY;
		int AnimationMaxCut;
		float AnimationTime;
		float ProgressTime;
		// 不可直接从 UpdateInfo 访问
		Vector3_t position;
		Quaternion_t rotation;
		Vector3_t scale;
		Color_t color;
		Vector2_t AnimationScaleUV;
		Vector2_t AnimationOffsetUV;
	};

	enum class MoveBlurType {
		None = 0x0,
		Circle = 0x1,
		Horizontal = 0x2,
		Vertical = 0x3,
		Ellipse = 0x4,
		Roll = 0x5
	};

	struct RadialBlurUpdateInfo {
		MoveBlurType moveBlurType;
		Vector2_t radialBlurOffset;
		int radialBlurDownsample;
		float radialBlurStartArea;
		float radialBlurEndArea;
		float radialBlurPower;
		int radialBlurIteration;
		Vector2_t radialBlurEllipseDir;
		float radialBlurRollEulerAngles;
		bool isEnabledDepth;
		float depthPowerFront;
		float depthPowerBack;
		bool isEnabledDepthCancelRect;
		Vector4_t depthCancelRect;
		float depthCancelBlendLength;
		bool isExpandDepthCancelRect;
	};

	struct ExposureUpdateInfo {
		bool IsEnable;
		float DepthMask;
		Vector4_t ExposureParameter;
	};

	struct VortexUpdateInfo {
		bool IsEnable;
		float RotVolume;
		float DepthClip;

		Vector4_t Area;
	};

	struct CharaFootLightUpdateInfo {
		int CharacterIndex = 0;
		float hightMax;
		// 不可直接从 UpdateInfo 访问
		Color_t lightColor;
		bool followGame = true;
	};


	extern UmaEnum<int> liveCharaPositionFlag;

	struct GlobalLightUpdateInfo {
		int flags;  // LiveCharaPositionFlag
		float globalRimShadowRate;
		float rimStep;
		float rimFeather;
		float rimSpecRate;
		float RimHorizonOffset;
		float RimVerticalOffset;
		float RimHorizonOffset2;
		float RimVerticalOffset2;
		float rimStep2;
		float rimFeather2;
		float rimSpecRate2;
		float globalRimShadowRate2;

		Quaternion_t lightRotation;
		Color_t rimColor;
		Color_t rimColor2;

		bool followGame = true;
	};


	// PostEffectUpdateInfo_DOF
	extern Vector3_t liveDOFForcalPosition;
	extern PostEffectUpdateInfo_DOF postEffectUpdateInfo_DOF;
	extern bool dofColtrollerFollowGame;


	// VolumeLightUpdateInfo
	extern PostFilmUpdateInfo postFilmUpdateInfo[3];
	extern bool livePostFilmFollowGame[3];

	extern Vector2_t filmOffsetParam[3];
	extern Vector4_t filmOptionParam[3];
	extern Color_t filmcolor0[3];
	extern Color_t filmcolor1[3];
	extern Color_t filmcolor2[3];
	extern Color_t filmcolor3[3];
	extern Vector2_t FilmScale[3];
	extern int filmIndex;

	// LightProjectionUpdateInfo
	extern LightProjectionUpdateInfo lightProjectionUpdateInfo;
	extern bool liveLightProjectionFollowGame;

	// RadialBlurUpdateInfo
	extern RadialBlurUpdateInfo radialBlurUpdateInfo;
	extern bool liveRadialBlurFollowGame;

	// ExposureUpdateInfo
	extern ExposureUpdateInfo exposureUpdateInfo;
	extern bool liveExposureFollowGame;

	// VortexUpdateInfo
	extern VortexUpdateInfo vortexUpdateInfo;
	extern bool liveVortexFollowGame;

	// CharaFootLightUpdateInfo
	extern CharaFootLightUpdateInfo charaFootLightUpdateInfo[36];

	// GlobalLightUpdateInfo
	extern std::unordered_map<int, GlobalLightUpdateInfo> globalLightUpdateInfo;
	extern bool globalLightUpdateInfo_inited;


	void initGuiGlobalData();
}
