#include "stdinclude.hpp"
#include "umagui/guiShowData.hpp"

bool ignoreNegativeSpeed = false;
Vector3_t UmaGUiShowData::liveDOFForcalPosition { .0f, 1.72f, 1.0f };
UmaGUiShowData::PostEffectUpdateInfo_DOF UmaGUiShowData::postEffectUpdateInfo_DOF {
	false, 5.0f, 1.6f, DofQuality::OnlyBackground, DofBlur::Mixed,
	1.72, 1.0, 0.5, false, 3.25, 0.5, 0.4, -0.67, 0
};
bool UmaGUiShowData::dofColtrollerFollowGame = true;

Vector2_t UmaGUiShowData::filmOffsetParam[3]{};
Vector4_t UmaGUiShowData::filmOptionParam[3]{};
Color_t UmaGUiShowData::filmcolor0[3]{};
Color_t UmaGUiShowData::filmcolor1[3]{};
Color_t UmaGUiShowData::filmcolor2[3]{};
Color_t UmaGUiShowData::filmcolor3[3]{};
Vector2_t UmaGUiShowData::FilmScale[3]{};
UmaGUiShowData::PostFilmUpdateInfo UmaGUiShowData::postFilmUpdateInfo[3]{};
bool UmaGUiShowData::livePostFilmFollowGame[3] = { true , true , true };
int UmaGUiShowData::filmIndex = 0;

UmaGUiShowData::LightProjectionUpdateInfo UmaGUiShowData::lightProjectionUpdateInfo{};
bool UmaGUiShowData::liveLightProjectionFollowGame = true;

UmaGUiShowData::RadialBlurUpdateInfo UmaGUiShowData::radialBlurUpdateInfo{};
bool UmaGUiShowData::liveRadialBlurFollowGame = true;

UmaGUiShowData::ExposureUpdateInfo UmaGUiShowData::exposureUpdateInfo;
bool UmaGUiShowData::liveExposureFollowGame = true;

UmaGUiShowData::VortexUpdateInfo UmaGUiShowData::vortexUpdateInfo;
bool UmaGUiShowData::liveVortexFollowGame = true;

UmaGUiShowData::CharaFootLightUpdateInfo UmaGUiShowData::charaFootLightUpdateInfo[36]{};

std::unordered_map<int, UmaGUiShowData::GlobalLightUpdateInfo> UmaGUiShowData::globalLightUpdateInfo{};
bool UmaGUiShowData::globalLightUpdateInfo_inited = false;

UmaEnum<int> UmaGUiShowData::liveCharaPositionFlag (
	std::vector<std::string>{ "Place01", "Place02", "Place03", "Place04", "Place05", "Place06", "Place07",
	"Place08", "Place09", "Place10", "Place11", "Place12", "Place13", "Place14", "Place15", "Place16",
	"Place17", "Place18", "Center", "Left", "Right", "Side", "Back", "Other", "All", "Default"
},
std::vector<int32_t>{ 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000,
0x4000, 0x8000, 0x10000, 0x20000, 0x1, 0x2, 0x4, 0x6, 0x3fff8, 0x3fffe, 0x3ffff, 0x0
}
);

UmaGUiShowData::UmaRaceMotionData::UmaRaceMotionData(int gateNo, std::wstring charaName, std::wstring trainerName) {
	this->gateNo = gateNo;
	this->charaName = utility::conversions::to_utf8string(charaName);
	this->trainerName = utility::conversions::to_utf8string(trainerName);
}

void UmaGUiShowData::UmaRaceMotionData::UpdateMotionData(float speed, float rate, float RaceBaseSpeed, float MinSpeed, float StartDashSpeedThreshold,
	bool IsOverRun, float Hp, float MaxHp, float HpPer, int NearHorseCount, float CongestionTime, int RawSpeed,
	float BaseSpeed, float Speed, int RawStamina, float BaseStamina, float Stamina, int RawPow, float BasePow, float Pow,
	int RawGuts, float BaseGuts, float Guts, int RawWiz, float BaseWiz, float Wiz, bool IsStartDash, int ActivateSkillCount,
	float lastSpeed, float MoveDistance, float distance, float deltatime, float LastSpurtStartDistance, bool isLastSpurt)
{
	this->speed = speed;
	this->rate = rate;
	this->RaceBaseSpeed = RaceBaseSpeed;
	this->MinSpeed = MinSpeed;
	this->StartDashSpeedThreshold = StartDashSpeedThreshold;
	this->IsOverRun = IsOverRun;
	this->Hp = Hp;
	this->MaxHp = MaxHp;
	this->HpPer = HpPer;
	this->NearHorseCount = NearHorseCount;
	this->CongestionTime = CongestionTime;
	this->RawSpeed = RawSpeed;
	this->BaseSpeed = BaseSpeed;
	this->Speed = Speed;
	this->RawStamina = RawStamina;
	this->BaseStamina = BaseStamina;
	this->Stamina = Stamina;
	this->RawPow = RawPow;
	this->BasePow = BasePow;
	this->Pow = Pow;
	this->RawGuts = RawGuts;
	this->BaseGuts = BaseGuts;
	this->Guts = Guts;
	this->RawWiz = RawWiz;
	this->BaseWiz = BaseWiz;
	this->Wiz = Wiz;
	this->IsStartDash = IsStartDash;
	this->ActivateSkillCount = ActivateSkillCount;
	this->lastSpeed = lastSpeed;
	if (!(ignoreNegativeSpeed && (MoveDistance < 0))) {
		this->MoveDistance = MoveDistance;
	}
	this->deltatime = deltatime;
	if (!IsOverRun) {
		this->distance = distance;
	}
	this->LastSpurtStartDistance = LastSpurtStartDistance;
	this->isLastSpurt = isLastSpurt;
}

void UmaGUiShowData::UmaRaceMotionData::setRank(int rank) {
	this->rank = rank;
}

void UmaGUiShowData::UmaRaceMotionData::setFrontAndFirstDist(float distanceFront, float distanceFirst) {
	this->distanceFront = distanceFront;
	this->distanceFirst = distanceFirst;
}

void UmaGUiShowData::UmaRaceMotionData::setFinallyRank(int rank) {
	this->finallyRank = rank;
}


UmaGUiShowData::SkillEventData::SkillEventData(int skillLevel, std::string horseName, std::string tName, int currGateNo, 
		int rarity, int gradeValue) {
	this->skillLevel = skillLevel;
	this->horseName = horseName;
	this->tName = tName;
	this->currGateNo = currGateNo;
	this->rarity = rarity;
	this->gradeValue = gradeValue;
}

void UmaGUiShowData::SkillEventData::updateCoolDownTime(float cooldownTime) {
	if (cooldownTime != 0.0f) {
		this->cooldownTime = cooldownTime;
	}
}

void UmaGUiShowData::SkillEventData::addSkillAbilities(SkillAbility skillAbility) {
	this->skillAbilities.push_back(skillAbility);
}

void UmaGUiShowData::SkillEventData::SetSkillName(std::string skillName) {
	this->skillName = skillName;
}


UmaGUiShowData::SkillAbility::SkillAbility(int abilityType, float effValue) {
	this->abilityType = abilityType;
	this->effValue = effValue;
}

void UmaGUiShowData::SkillAbility::addTargets(UmaRaceMotionData target) {
	this->targets.emplace_back(target);
}

void UmaGUiShowData::initGuiGlobalData() {
	if (!UmaGUiShowData::globalLightUpdateInfo_inited) {
		UmaGUiShowData::globalLightUpdateInfo_inited = true;
		for (auto i : UmaGUiShowData::liveCharaPositionFlag.GetValues()) {
			UmaGUiShowData::globalLightUpdateInfo.emplace(i, UmaGUiShowData::GlobalLightUpdateInfo{i});
		}
	}
}
