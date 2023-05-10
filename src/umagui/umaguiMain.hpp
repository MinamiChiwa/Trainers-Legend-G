#pragma once
#include "stdinclude.hpp"

extern std::map<void*, UmaGUiShowData::UmaRaceMotionData> umaRaceData;
extern std::vector<UmaGUiShowData::SkillEventData> umaUsedSkillList;
extern bool closeWhenRaceEnd;

void SetGuiDone(bool isDone);
void SetShowRaceWnd(bool value);
void SetShowLiveWnd(bool value);
bool GetShowLiveWnd();
bool getUmaGuiDone();
void guimain();
void updateRaceGUIData(std::map<void*, UmaGUiShowData::UmaRaceMotionData>& data);
