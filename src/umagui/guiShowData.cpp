#include "stdinclude.hpp"
#include "umagui/guiShowData.hpp"



UmaGUiShowData::UmaRaceMotionData::UmaRaceMotionData(int gateNo, std::wstring charaName, std::wstring trainerName) {
	this->gateNo = gateNo;
	this->charaName = utility::conversions::to_utf8string(charaName);
	this->trainerName = utility::conversions::to_utf8string(trainerName);
}

void UmaGUiShowData::UmaRaceMotionData::UpdateMotionData(float speed, float rate, float RaceBaseSpeed, float MinSpeed, float StartDashSpeedThreshold,
	bool IsOverRun, float Hp, float MaxHp, float HpPer, int NearHorseCount, float CongestionTime, int RawSpeed,
	float BaseSpeed, float Speed, int RawStamina, float BaseStamina, float Stamina, int RawPow, float BasePow, float Pow,
	int RawGuts, float BaseGuts, float Guts, int RawWiz, float BaseWiz, float Wiz, bool IsStartDash, int ActivateSkillCount,
	float lastSpeed, float MoveDistance, float distance, float deltatime)
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
	this->MoveDistance = MoveDistance;
	this->deltatime = deltatime;
	if (!IsOverRun) {
		this->distance = distance;
	}
}

void UmaGUiShowData::UmaRaceMotionData::setRank(int rank) {
	this->rank = rank;
}
