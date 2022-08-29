#pragma once
#include "stdinclude.hpp"

namespace UmaCamera {
	Vector3_t getCameraPos();
	Vector3_t getCameraLookat();
	void loadGlobalData();
	void setMoveStep(float value);
	void setUmaCameraType(int value);
	void updateFollowUmaPos(Vector3_t lastFrame, Vector3_t thisFrame, Vector3_t* setPos);

	void setRaceCamFOV(float value);
	float getRaceCamFov();

	void reset_camera();
	void camera_forward();
	void camera_back();
	void camera_left();
	void camera_right();
	void camera_down();
	void camera_up();
	void initCameraSettings();
}
