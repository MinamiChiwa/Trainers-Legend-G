#include "stdinclude.hpp"

#define KEY_W  87
#define KEY_S  83
#define KEY_A  65
#define KEY_D  68
#define KEY_R  82
#define KEY_UP  38
#define KEY_DOWN  40
#define KEY_LEFT  37
#define KEY_RIGHT  39
#define KEY_CTRL  162
#define KEY_SHIFT  160
#define KEY_ALT  164
#define KEY_SPACE  32


namespace UmaCamera {
	namespace {
		float moveStep = 0.1;
		float look_radius = 0.5;  // 转向半径
		float moveAngel = 3.5;  // 转向角度

		float horizontalAngle = 0;  // 水平方向角度
		float verticalAngle = 0;  // 垂直方向角度

		int smoothLevel = 5;
		unsigned long sleepTime = 2;
		Vector3_t cameraPos{ 0.093706, 0.467159, 9.588791 };
		Vector3_t cameraLookAt{ cameraPos.x, cameraPos.y, cameraPos.z - look_radius };
	}

	void setMoveStep(float value) {
		moveStep = value;
	}

	Vector3_t getCameraPos() {
		return cameraPos;
	}

	Vector3_t getCameraLookat() {
		return cameraLookAt;
	}

	void reset_camera() {
		cameraPos = Vector3_t{ 0.093706, 0.467159, 9.588791 };
		cameraLookAt = Vector3_t{ cameraPos.x, cameraPos.y, cameraPos.z - look_radius };
		horizontalAngle = 0;
		verticalAngle = 0;
	}

	void set_lon_move(float degree) {  // 前后移动
		auto radian = degree * 3.14159 / 180;
		auto f_step = cos(radian) * moveStep / smoothLevel;  // ↑↓
		auto l_step = sin(radian) * moveStep / smoothLevel;  // ←→

		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.z -= f_step;
			cameraLookAt.z -= f_step;
			cameraPos.x += l_step;
			cameraLookAt.x += l_step;
			Sleep(sleepTime);
		}
	}

	void camera_forward() {  // 向前
		set_lon_move(verticalAngle);
	}

	void camera_back() {  // 后退
		set_lon_move(verticalAngle + 180);
	}

	void camera_left() {  // 向左
		set_lon_move(verticalAngle + 90);
	}

	void camera_right() {  // 向右
		set_lon_move(verticalAngle - 90);
	}

	void camera_down() {  // 向下
		auto preStep = moveStep / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.y -= preStep;
			cameraLookAt.y -= preStep;
			Sleep(sleepTime);
		}
	}
	
	void camera_up() {  // 向上
		auto preStep = moveStep / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.y += preStep;
			cameraLookAt.y += preStep;
			Sleep(sleepTime);
		}
	}

	void setVertLook(float angle1, float angle2) {  // 上+
		auto radian = angle1 * 3.14159 / 180;
		auto radian2 = ((double)angle2 - 90) * 3.14159 / 180;  // 日

		auto stepX1 = look_radius * sin(radian2) * cos(radian) / smoothLevel;
		auto stepX2 = look_radius * sin(radian2) * sin(radian) / smoothLevel;
		auto stepX3 = look_radius * cos(radian2) / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraLookAt.z = cameraPos.z + stepX1;
			cameraLookAt.y = cameraPos.y + stepX3;
			cameraLookAt.x = cameraPos.x - stepX2;
			Sleep(sleepTime);
		}

	}

	void setHoriLook(float degree) {  // 左+
		auto radian = degree * 3.14159 / 180;
		auto stepBt = cos(radian) * look_radius / smoothLevel;
		auto stepHi = sin(radian) * look_radius / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraLookAt.x = cameraPos.x + stepHi;
			cameraLookAt.z = cameraPos.z - stepBt;
			Sleep(sleepTime);
		}
	}

	void cameraLookat_up() {
		horizontalAngle += moveAngel;
		if (horizontalAngle >= 90) horizontalAngle = 89.99;
		setVertLook(verticalAngle, horizontalAngle);
	}

	void cameraLookat_down() {
		horizontalAngle -= moveAngel;
		if (horizontalAngle <= -90) horizontalAngle = -89.99;
		setVertLook(verticalAngle, horizontalAngle);
	}

	void cameraLookat_left() {
		verticalAngle += moveAngel;
		if (verticalAngle >= 360) verticalAngle = 0;
		setHoriLook(verticalAngle);
	}

	void cameraLookat_right() {
		verticalAngle -= moveAngel;
		if (verticalAngle <= -360) verticalAngle = 0;
		setHoriLook(verticalAngle);
	}

	void on_keyboard_down(int key, DWORD shift, DWORD ctrl, DWORD alt, DWORD space, DWORD up, DWORD down, DWORD left, DWORD right) {
		bool ctrl_down = (key == KEY_CTRL) || (ctrl != 0);
		bool space_down = (key == KEY_SPACE) || (space != 0);
		bool up_down = (key == KEY_UP) || (up != 0);
		bool down_down = (key == KEY_DOWN) || (down != 0);
		bool left_down = (key == KEY_LEFT) || (left != 0);
		bool right_down = (key == KEY_RIGHT) || (right != 0);

		switch (key) {
			case KEY_W: 
				camera_forward(); break;
			case KEY_S:
				camera_back(); break;
			case KEY_A:
				camera_left(); break;
			case KEY_D:
				camera_right(); break;
			case KEY_R:
				reset_camera(); break;
			default:
				break;
		}

		if (ctrl_down) camera_down();
		if (space_down) camera_up();
		if (up_down) cameraLookat_up();
		if (down_down) cameraLookat_down();
		if (left_down) cameraLookat_left();
		if (right_down) cameraLookat_right();

	}

	void initCameraSettings() {
		reset_camera();
		MHotkey::SetKeyCallBack(on_keyboard_down);
	}
}
