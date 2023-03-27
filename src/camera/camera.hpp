#pragma once
#include "stdinclude.hpp"

namespace UmaCamera {
	Vector3_t getCameraPos();
	Vector3_t getHomeCameraPos();
	Vector3_t getCameraLookat();
	void SetCameraPos(float x, float y, float z);
	void SetCameraLookat(float x, float y, float z);
	void updateFollowCameraPosByLookatAndOffset();
	void loadGlobalData();
	void setMoveStep(float value);
	void setUmaCameraType(int value);
	void updateFollowUmaPos(Vector3_t lastFrame, Vector3_t thisFrame, Quaternion_t currQuat, Vector3_t* setPos);
	void setHomeCameraAngle(float value);
	void mouseMove(LONG x, LONG y, int mouseEventType);

	int GetLiveCharaPositionFlag();
	int GetLiveCameraCharaParts();
	int GetLiveCameraType();

	void setRaceCamFOV(float value);
	float getRaceCamFov();
	float getLiveCamFov();
	void setLiveStart(bool value);

	void reset_camera();
	void camera_forward();
	void camera_back();
	void camera_left();
	void camera_right();
	void camera_down();
	void camera_up();
	void initCameraSettings();
}
