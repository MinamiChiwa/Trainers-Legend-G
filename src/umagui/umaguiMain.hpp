#pragma once
#include "stdinclude.hpp"

extern std::map<void*, UmaGUiShowData::UmaRaceMotionData> umaRaceData;
extern std::vector<UmaGUiShowData::SkillEventData> umaUsedSkillList;
extern bool closeWhenRaceEnd;

void SetGuiDone(bool isDone);
void SetShowRaceWnd(bool value);
bool getUmaGuiDone();
void guimain();
void updateRaceGUIData(std::map<void*, UmaGUiShowData::UmaRaceMotionData>& data);
