#include "stdinclude.hpp"
#include "umagui/guiShowData.hpp"

bool ignoreNegativeSpeed = false;


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
