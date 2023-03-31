#include "stdinclude.hpp"
#include <corecrt_math_defines.h>

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

template<typename T>
class UmaEnum {
private:
	std::vector<std::string> names;
	std::vector<T> values;
	int nowIndex = 0;

public:
	UmaEnum(std::vector<std::string> names, std::vector<T> values) {
		if (names.size() != values.size()) return;
		assert(names.size() > 0);
		this->names = names;
		this->values = values;
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

};

/*
坐标:
  ↑y
x← ↘z
*/
namespace UmaCamera {
	namespace CameraCalc {
		struct Vector3
		{
			float x, y, z;

			Vector3(float _x, float _y, float _z)
			{
				x = _x;
				y = _y;
				z = _z;
			}

			Vector3(Vector3_t& vec) {
				x = vec.x;
				y = vec.y;
				z = vec.z;
			}

			Vector3 operator+(const Vector3& v) const
			{
				return Vector3(x + v.x, y + v.y, z + v.z);
			}

			Vector3 operator-(const Vector3& other) const {
				return Vector3(x - other.x, y - other.y, z - other.z);
			}

			Vector3 operator*(float s) const
			{
				return Vector3(x * s, y * s, z * s);
			}

			Vector3 normalized() const {
				float n = norm();
				return Vector3(x / n, y / n, z / n);
			}

			float norm() const {
				return std::sqrt(x * x + y * y + z * z);
			}

			static Vector3 cross(const Vector3& a, const Vector3& b) {
				return Vector3(a.y * b.z - a.z * b.y,
					a.z * b.x - a.x * b.z,
					a.x * b.y - a.y * b.x);
			}
		};


		struct Quaternion
		{
			float w, x, y, z;

			Quaternion(float _w, float _x, float _y, float _z)
			{
				w = _w;
				x = _x;
				y = _y;
				z = _z;
			}

			Quaternion(Quaternion_t& quat) {
				w = quat.w;
				x = quat.x;
				y = quat.y;
				z = quat.z;
			}

			operator Quaternion_t() const {
				return Quaternion_t{ w,x,y,z };
			}

			Quaternion operator*(const Quaternion& q) const
			{
				float nw = w * q.w - x * q.x - y * q.y - z * q.z;
				float nx = w * q.x + x * q.w + y * q.z - z * q.y;
				float ny = w * q.y - x * q.z + y * q.w + z * q.x;
				float nz = w * q.z + x * q.y - y * q.x + z * q.w;
				return Quaternion(nw, nx, ny, nz);
			}

			Quaternion operator *(const float& rhs) const {
				return Quaternion(w * rhs, x * rhs, y * rhs, z * rhs);
			}

			Quaternion operator +(const Quaternion& rhs) const {
				return Quaternion(w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z);
			}

			Quaternion operator -(const Quaternion& rhs) const {
				return Quaternion(w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z);
			}

			Quaternion operator -() const {
				return Quaternion(-w , -x, -y, -z);
			}

			float norm() const {
				return std::sqrt(w * w + x * x + y * y + z * z);
			}

			Quaternion normalized() const {
				float n = norm();
				return Quaternion(w / n, x / n, y / n, z / n);
			}

			Quaternion Conjugate() const
			{
				return Quaternion(w, -x, -y, -z);
			}

			//  roll, pitch, yaw
			Vector3 ToEuler() {
				Vector3 euler(0, 0, 0);
				// 计算 roll (x-axis rotation)
				double sinr = 2.0 * (w * x + y * z);
				double cosr = 1.0 - 2.0 * (x * x + y * y);
				euler.x = atan2(sinr, cosr);

				// 计算 pitch (y-axis rotation)
				double sinp = 2.0 * (w * y - z * x);
				if (fabs(sinp) >= 1) {
					euler.y = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
				}
				else {
					euler.y = asin(sinp);
				}

				// 计算 yaw (z-axis rotation)
				double siny = 2.0 * (w * z + x * y);
				double cosy = 1.0 - 2.0 * (y * y + z * z);
				euler.z = atan2(siny, cosy);

				return euler;
			}

			static Quaternion FromEuler(Vector3 euler) {
				double cy = cos(euler.z * 0.5);
				double sy = sin(euler.z * 0.5);
				double cp = cos(euler.y * 0.5);
				double sp = sin(euler.y * 0.5);
				double cr = cos(euler.x * 0.5);
				double sr = sin(euler.x * 0.5);

				Quaternion q(0, 0, 0, 0);
				q.w = cr * cp * cy + sr * sp * sy;
				q.x = sr * cp * cy - cr * sp * sy;
				q.y = cr * sp * cy + sr * cp * sy;
				q.z = cr * cp * sy - sr * sp * cy;
				return q;
			}

			static float Dot(const Quaternion& q1, const Quaternion& q2) {
				return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
			}

			// 计算夹角
			static float Acos(const float x) {
				if (x < -1.0f) {
					return M_PI;
				}
				else if (x > 1.0f) {
					return 0.0f;
				}
				else {
					return std::acos(x);
				}
			}

			// Slerp方法
			static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, const float t) {
				Quaternion q3(0, 0, 0, 0);
				float dot = Quaternion::Dot(q1, q2);
				if (dot < 0.0f) {
					dot = -dot;
					q3.w = -q2.w;
					q3.x = -q2.x;
					q3.y = -q2.y;
					q3.z = -q2.z;
				}
				else {
					q3 = q2;
				}
				if (dot < 0.95f) {
					const float angle = 2.0f * Quaternion::Acos(dot);
					const float sinAngle = std::sinf(angle);
					const float sin1 = std::sinf((1.0f - t) * angle) / sinAngle;
					const float sin2 = std::sinf(t * angle) / sinAngle;
					q3.w = (q1.w * sin1 + q3.w * sin2);
					q3.x = (q1.x * sin1 + q3.x * sin2);
					q3.y = (q1.y * sin1 + q3.y * sin2);
					q3.z = (q1.z * sin1 + q3.z * sin2);
				}
				else {
					q3.w = q1.w + t * (q3.w - q1.w);
					q3.x = q1.x + t * (q3.x - q1.x);
					q3.y = q1.y + t * (q3.y - q1.y);
					q3.z = q1.z + t * (q3.z - q1.z);
				}
				return q3;
			}
		};

		void SmoothQuaternion(Quaternion& q0, Quaternion& q1, const float threshold) {
			float angle = 2.0f * Quaternion::Acos(Quaternion::Dot(q0, q1));
			int forCount = 0;
			while (angle > threshold) {
				float t = threshold / angle;
				Quaternion q2 = Quaternion::Slerp(q0, q1, t);
				q0 = q2;
				angle = 2.0f * Quaternion::Acos(Quaternion::Dot(q0, q1));
				forCount++;
				if (forCount > 50) break;
			}
		}

		Quaternion LookRotation(const Vector3& forward, const Vector3& upwards = Vector3(0, 1, 0)) {
			Vector3 right = Vector3::cross(upwards, forward).normalized();
			Vector3 up = Vector3::cross(forward, right).normalized();
			float m[4][4] = {
				{right.x, up.x, forward.x, 0},
				{right.y, up.y, forward.y, 0},
				{right.z, up.z, forward.z, 0},
				{0, 0, 0, 1}
			};
			float tr = m[0][0] + m[1][1] + m[2][2];
			float qw, qx, qy, qz;
			if (tr > 0) {
				float S = sqrt(tr + 1.0f) * 2;
				qw = 0.25f * S;
				qx = (m[2][1] - m[1][2]) / S;
				qy = (m[0][2] - m[2][0]) / S;
				qz = (m[1][0] - m[0][1]) / S;
			}
			else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2])) {
				float S = sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2;
				qw = (m[2][1] - m[1][2]) / S;
				qx = 0.25f * S;
				qy = (m[0][1] + m[1][0]) / S;
				qz = (m[0][2] + m[2][0]) / S;
			}
			else if (m[1][1] > m[2][2]) {
				float S = sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2;
				qw = (m[0][2] - m[2][0]) / S;
				qx = (m[0][1] + m[1][0]) / S;
				qy = 0.25f * S;
				qz = (m[1][2] + m[2][1]) / S;
			}
			else {
				float S = sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2;
				qw = (m[1][0] - m[0][1]) / S;
				qx = (m[0][2] + m[2][0]) / S;
				qy = (m[1][2] + m[2][1]) / S;
				qz = 0.25f * S;
			}
			return Quaternion(qx, qy, qz, qw);
		}

		Quaternion RotateQuaternion(const Quaternion& q, float angle_degrees, const Vector3& axis) {
			float angle_radians = angle_degrees * M_PI / 180.0f;
			float half_angle = angle_radians * 0.5f;
			float s = sin(half_angle);
			Vector3 normalized_axis = axis.normalized();
			Quaternion q1(cos(half_angle), normalized_axis.x * s, normalized_axis.y * s, normalized_axis.z * s);
			Quaternion q2 = q * q1;
			return q2;
		}

		Vector3 RotateVector(const Quaternion& q, const Vector3& v)
		{
			Quaternion p(0, v.x, v.y, v.z);
			Quaternion q_inv = q.Conjugate();
			Quaternion rotated = q * p * q_inv;
			return Vector3(rotated.x, rotated.y, rotated.z);
		}

		Vector3 GetFrontPos(const Vector3& pos, const Quaternion& rot, float distance)
		{
			Vector3 v(0, 0, distance);
			Vector3 v_rotated = RotateVector(rot, v);
			Vector3 pos_front = pos + v_rotated;
			return pos_front;
		}

		Vector3_t GetFrontPos(const Vector3_t& pos, const Quaternion_t& rot, float distance) {
			Vector3 vPos(pos.x, pos.y, pos.z);
			Quaternion vQos(rot.w, rot.x, rot.y, rot.z);
			const auto ret = GetFrontPos(vPos, vQos, distance);
			return Vector3_t{ ret.x, ret.y, ret.z };
		}

	}


	namespace {
		int cameraType = CAMERA_LIVE;
		int liveCameraType = LiveCamera_FREE;
		float moveStep = 0.1;
		float look_radius = 5;  // 转向半径
		float moveAngel = 1.5;  // 转向角度

		float horizontalAngle = 0;  // 水平方向角度
		float verticalAngle = 0;  // 垂直方向角度
		float homeCameraAngle = 0; // 主页相机角度

		float raceDefaultFOV = 12;
		float liveDefaultFOV = 60;
		bool isLiveStart = false;

		int smoothLevel = 1;
		unsigned long sleepTime = 0;
		Vector3_t cameraPos{ 0.093706, 0.467159, 9.588791 };
		Vector3_t homeCameraPos{ 0.0, 1.047159, -4.811181 };
		Vector3_t cameraLookAt{ cameraPos.x, cameraPos.y, cameraPos.z - look_radius };
		bool orig_lookat_target = g_race_freecam_lookat_umamusume;
		float orig_g_race_freecam_follow_umamusume_distance = g_race_freecam_follow_umamusume_distance;
		Vector3_t orig_g_race_freecam_follow_umamusume_offset{
			g_race_freecam_follow_umamusume_offset.x,
			g_race_freecam_follow_umamusume_offset.y,
			g_race_freecam_follow_umamusume_offset.z
		};
		Vector3_t cacheLastRacePos{};
		Vector3_t liveFollowCameraOffset{ 0, 0, -2 };
		Vector3_t liveFollowCameraLookatOffset{ 0, 0, 0 };
		Vector2_t raceFollowLookatOffset{ 0, 0 };
		Vector3_t liveFirstPersonOffset{ 0, 0.075, 0.015 };
		Vector3_t raceFirstPersonLookAtOffset{ 0, 0, 0 };
		bool lookAtUmaReverse = false;

		bool mouseLockThreadStart = false;
		bool lookVertRunning, lookHoriRunning = false;
		bool rMousePressFlg = false;

		typedef struct Point
		{
			DOUBLE x, y;
			Point()
			{
				x = 0;
				y = 0;
			}
			Point(DOUBLE xx, DOUBLE yy)
			{
				x = xx;
				y = yy;
			}
		}SDPoint;

		enum LonMoveHState {
			LonMoveLeftAndRight,
			LonMoveForward,
			LonMoveBack
		};

		UmaEnum liveCharaPositionFlag(
			std::vector<std::string>{ "Place01", "Place02", "Place03", "Place04", "Place05", "Place06", "Place07", 
				"Place08", "Place09", "Place10", "Place11", "Place12", "Place13", "Place14", "Place15", "Place16", 
				"Place17", "Place18", "Center", "Left", "Right", "Side", "Back", "Other", "All"
			},
			std::vector<int32_t>{ 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000,
				0x4000, 0x8000, 0x10000, 0x20000, 0x1, 0x2, 0x4, 0x6, 0x3fff8, 0x3fffe, 0x3ffff
			}
		);

		UmaEnum liveCameraCharaParts(
			std::vector<std::string>{ "Face", "Waist", "LeftHandWrist", "RightHandAttach", "Chest", "Foot", "InitFaceHeight",
				"InitWaistHeight", "InitChestHeight", "RightHandWrist", "LeftHandAttach", "ConstFaceHeight", "ConstChestHeight",
				"ConstWaistHeight", "ConstFootHeight", "Position", "PositionWithoutOffset", "InitialHeightFace", 
				"InitialHeightChest", "InitialHeightWaist", "Max"
			},
			std::vector<int32_t>{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 
				0x12, 0x13, 0x14
			}
		);

	}

	void loadGlobalData() {
		orig_g_race_freecam_follow_umamusume_distance = g_race_freecam_follow_umamusume_distance;
		orig_lookat_target = g_race_freecam_lookat_umamusume;
		orig_g_race_freecam_follow_umamusume_offset = Vector3_t{
			g_race_freecam_follow_umamusume_offset.x,
			g_race_freecam_follow_umamusume_offset.y,
			g_race_freecam_follow_umamusume_offset.z
		};
	}

	BOOL ExPandLine(SDPoint pt1, SDPoint pt2, DOUBLE nLen, SDPoint& OutPt)
	{
		if (pt1.x - pt2.x == 0)
		{
			OutPt.x = pt1.x;
			if (pt1.y - pt2.y > 0)
			{
				OutPt.y = pt2.y - nLen;
			}
			else
			{
				OutPt.y = pt2.y + nLen;
			}
		}
		else if (pt1.y - pt2.y == 0)
		{
			OutPt.y = pt1.y;
			if (pt1.x - pt2.x > 0)
			{
				OutPt.x = pt2.x - nLen;
			}
			else
			{
				OutPt.x = pt2.x + nLen;
			}
		}
		else
		{
			DOUBLE k = 0.0;
			DOUBLE b = 0.0;
			k = (pt1.y - pt2.y) / (pt1.x - pt2.x);
			b = pt1.y - k * pt1.x;
			DOUBLE zoom = 0.0;
			zoom = nLen / sqrt((pt2.x - pt1.x) * (pt2.x - pt1.x) + (pt2.y - pt1.y) * (pt2.y - pt1.y));

			if (k > 0)
			{
				if (pt1.x - pt2.x > 0)
				{
					OutPt.x = pt2.x - zoom * (pt1.x - pt2.x);
					OutPt.y = k * OutPt.x + b;
				}
				else
				{
					OutPt.x = pt2.x + zoom * (pt2.x - pt1.x);
					OutPt.y = k * OutPt.x + b;
				}
			}
			else
			{
				if (pt1.x - pt2.x > 0)
				{
					OutPt.x = pt2.x - zoom * (pt1.x - pt2.x);
					OutPt.y = k * OutPt.x + b;
				}
				else
				{
					OutPt.x = pt2.x + zoom * (pt2.x - pt1.x);
					OutPt.y = k * OutPt.x + b;
				}
			}
		}
		return TRUE;
	}

	void SetCameraPos(float x, float y, float z) {
		cameraPos.x = x;
		cameraPos.y = y;
		cameraPos.z = z;
	}

	void updateFollowCameraPosByLookatAndOffset() {
		auto nowAngel = liveFollowCameraOffset.x * M_PI / 180;

		cameraPos.x = cameraLookAt.x - sin(nowAngel) * liveFollowCameraOffset.z;
		cameraPos.z = cameraLookAt.z - cos(nowAngel) * liveFollowCameraOffset.z;
		cameraPos.y = cameraLookAt.y + liveFollowCameraOffset.y;

	}

	void SetCameraLookat(float x, float y, float z) {
		if (isLiveStart && (liveCameraType == LiveCamera_FOLLOW_UMA)) {
			x += liveFollowCameraLookatOffset.x;
			y += liveFollowCameraLookatOffset.y;
			z += liveFollowCameraLookatOffset.z;
		}
		cameraLookAt.x = x;
		cameraLookAt.y = y;
		cameraLookAt.z = z;

	}

	int GetLiveCameraType() {
		return liveCameraType;
	}

	void setMoveStep(float value) {
		moveStep = value;
	}

	void setRaceCamFOV(float value) {
		raceDefaultFOV = value;
	}

	float getRaceCamFov() {
		return raceDefaultFOV;
	}

	float getLiveCamFov() {
		return liveDefaultFOV;
	}

	void setLiveStart(bool value) {
		isLiveStart = value;
	}

	void setHomeCameraAngle(float value) {
		homeCameraAngle = value;
	}

	void reset_camera() {
		if (cameraType == CAMERA_RACE) {
			cameraPos = Vector3_t{ -51.72, 7.91, 108.57 };
		}
		else {
			cameraPos = Vector3_t{ 0.093706, 0.467159, 9.588791 };
		}
		cameraLookAt = Vector3_t{ cameraPos.x, cameraPos.y, cameraPos.z - look_radius };
		homeCameraPos = Vector3_t{ 0.0, 1.047159, -4.811181 };
		horizontalAngle = 0;
		verticalAngle = 0;
		g_race_freecam_follow_umamusume_offset = Vector3_t{
			orig_g_race_freecam_follow_umamusume_offset.x,
			orig_g_race_freecam_follow_umamusume_offset.y,
			orig_g_race_freecam_follow_umamusume_offset.z
		};
		orig_g_race_freecam_follow_umamusume_distance = g_race_freecam_follow_umamusume_distance;
		g_race_freecam_follow_umamusume = -1;
		liveDefaultFOV = 60;
		liveFollowCameraOffset = Vector3_t{ 0,0,-2 };
		liveFollowCameraLookatOffset = Vector3_t{ 0,0,0 };
		raceFollowLookatOffset = Vector2_t{ 0,0 };
		
		liveFirstPersonOffset = Vector3_t{ 0, 0.075, 0.015 };
		raceFirstPersonLookAtOffset = Vector3_t{ 0, 0, 0 };
	}

	void setUmaCameraType(int value) {
		if (cameraType == value) return;

		if (cameraType == CAMERA_LIVE && value == CAMERA_RACE) {
			moveStep = g_race_move_step;
			moveAngel /= 2;
		}
		else if (cameraType == CAMERA_RACE && value == CAMERA_LIVE) {
			moveStep = g_live_move_step;
			moveAngel *= 2;
		}
		cameraType = value;
		reset_camera();
	}

	Vector3_t getCameraPos() {
		return cameraPos;
	}

	Vector3_t getHomeCameraPos() {
		return homeCameraPos;
	}

	Vector3_t getCameraLookat() {
		return cameraLookAt;
	}

	void set_lon_move(float vertangle, LonMoveHState moveState=LonMoveLeftAndRight) {  // 前后移动
		auto radian = vertangle * M_PI / 180;
		auto radianH = (double)horizontalAngle * M_PI / 180;
		
		auto f_step = cos(radian) * moveStep * cos(radianH) / smoothLevel;  // ↑↓
		auto l_step = sin(radian) * moveStep * cos(radianH) / smoothLevel;  // ←→
		// auto h_step = tan(radianH) * sqrt(pow(f_step, 2) + pow(l_step, 2));
		auto h_step = sin(radianH) * moveStep / smoothLevel;

		switch (moveState)
		{
			case LonMoveForward: break;
			case LonMoveBack: h_step = -h_step; break;
			default: h_step = 0; break;
		}

		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.z -= f_step;
			cameraLookAt.z -= f_step;
			cameraPos.x += l_step;
			cameraLookAt.x += l_step;
			cameraPos.y += h_step;
			cameraLookAt.y += h_step;
			Sleep(sleepTime);
		}
	}

	void set_homecam_lon_move(float degree) {  // 主页相机前后移动
		auto radian = degree * M_PI / 180;
		auto f_step = cos(radian) * moveStep;  // ↑↓
		auto l_step = sin(radian) * moveStep;  // ←→
		homeCameraPos.z += f_step;
		homeCameraPos.x -= l_step;
	}

	void chechAndUpdateRaceRet(Vector3_t* setPos) {
		float diff_x = setPos->x - cacheLastRacePos.x;
		float diff_z = setPos->z - cacheLastRacePos.z;

		if (abs(diff_x) >= 0.3) {
			//setPos->x -= diff_x / 2;
		}
		if (abs(diff_z) >= 0.3) {
			//setPos->z -= diff_z / 2;
		}

		cacheLastRacePos = Vector3_t{ setPos->x, setPos->y, setPos->z };
	}

	Quaternion_t slerpTwo(Quaternion_t& rot, Quaternion_t& rot2, float t) {
		return CameraCalc::Quaternion::Slerp(rot, rot2, t);
	}

	Quaternion_t updateLookatByRotation(Quaternion_t rot) {
		if (g_race_freecam_follow_umamusume && raceFollowUmaFirstPersion) {
			auto newRot = CameraCalc::RotateQuaternion(rot, raceFirstPersonLookAtOffset.y, CameraCalc::Vector3(1, 0, 0));
			rot = CameraCalc::RotateQuaternion(newRot, raceFirstPersonLookAtOffset.x, CameraCalc::Vector3(0, 1, 0));
		}

		auto frontPos = CameraCalc::GetFrontPos(cameraPos, rot, look_radius);
		cameraLookAt.x = frontPos.x;
		cameraLookAt.y = frontPos.y;
		cameraLookAt.z = frontPos.z;
		return rot;
	}

	Quaternion_t updatePosAndLookatByRotation(Vector3_t pos, Quaternion_t rot) {
		if ((liveCameraType == LiveCamera_FIRST_PERSION) || raceFollowUmaFirstPersion) {
			pos = CameraCalc::GetFrontPos(pos, rot, liveFirstPersonOffset.z);
			pos.y += liveFirstPersonOffset.y;
		}
		cameraPos.x = pos.x;
		cameraPos.y= pos.y;
		cameraPos.z = pos.z;
		return updateLookatByRotation(rot);
	}

	// CameraCalc::Vector3 lastEuler = Vector3(0, 0, 0);
	CameraCalc::Quaternion lastRot = CameraCalc::Quaternion(1, 1, 1, 1);
	Quaternion_t updatePosAndLookatByRotationStable(Vector3_t pos, Quaternion_t rot, Quaternion_t nowRot) {
		auto rotateQuat = CameraCalc::Quaternion(rot);
		CameraCalc::SmoothQuaternion(rotateQuat, lastRot, 0.01f);
		lastRot.w = rotateQuat.w;
		lastRot.x = rotateQuat.x;
		lastRot.y = rotateQuat.y;
		lastRot.z = rotateQuat.z;
		return updatePosAndLookatByRotation(pos, rotateQuat);
		/*
		// 下方为原版
		auto rotateQuat = CameraCalc::Quaternion(rot);
		CameraCalc::SmoothQuaternion(rotateQuat, lastRot, 0.01f);
		lastRot.w = rotateQuat.w;
		lastRot.x = rotateQuat.x;
		lastRot.y = rotateQuat.y;
		lastRot.z = rotateQuat.z;
		// printf("orig: %f, %f, %f, %f; new: %f, %f, %f, %f\n", rot.w, rot.x, rot.y, rot.z, rotateQuat.w, rotateQuat.x, rotateQuat.y, rotateQuat.z);
		// auto rotateQuat = CameraCalc::RotateQuaternion(nowRot, 180, CameraCalc::Vector3(0, 1, 0));
		auto ret = Quaternion_t{ rotateQuat.w, rotateQuat.x, rotateQuat.y, rotateQuat.z };
		return updatePosAndLookatByRotation(pos, ret);
		*/
	}

	void updateFollowUmaPos(Vector3_t lastFrame, Vector3_t thisFrame, Quaternion_t currQuat, Vector3_t* setPos) {

		auto q2 = CameraCalc::RotateQuaternion(CameraCalc::Quaternion(currQuat.w, currQuat.x, currQuat.y, currQuat.z), 
			raceFollowLookatOffset.x,
			CameraCalc::Vector3(0, 1, 0)
		);
		currQuat.w = q2.w;
		currQuat.x = q2.x;
		currQuat.y = q2.y;
		currQuat.z = q2.z;

		auto frontPosOffset = CameraCalc::GetFrontPos(thisFrame, currQuat, g_race_freecam_follow_umamusume_distance < 0 ? -1 : 1);

		auto frontPos = CameraCalc::GetFrontPos(frontPosOffset, currQuat, -g_race_freecam_follow_umamusume_distance);
		setPos->x = frontPos.x;
		setPos->y = frontPos.y + g_race_freecam_follow_umamusume_offset.y;
		setPos->z = frontPos.z;


		SetCameraLookat(frontPosOffset.x, frontPosOffset.y + raceFollowLookatOffset.y, frontPosOffset.z);
		return;
		/*
		SDPoint pt1{thisFrame.x, thisFrame.z};
		SDPoint pt2{lastFrame.x, lastFrame.z};
		SDPoint ptOut{};
		if (lookAtUmaReverse) {
			ExPandLine(pt2, pt1, g_race_freecam_follow_umamusume_distance, ptOut);
		}
		else {
			ExPandLine(pt1, pt2, g_race_freecam_follow_umamusume_distance, ptOut);
		}
		
		setPos->x = ptOut.x + g_race_freecam_follow_umamusume_offset.x;
		setPos->z = ptOut.y + g_race_freecam_follow_umamusume_offset.z;
		// setPos->y = ceil((lastFrame.y + thisFrame.y) / 2) + g_race_freecam_follow_umamusume_offset.y;
		setPos->y = thisFrame.y + g_race_freecam_follow_umamusume_offset.y;
		// printf("calc: %f, %f  last: %f, %f  this: %f, %f\n", setPos->x, setPos->z, pt2.x, pt2.y, pt1.x, pt1.y);
		chechAndUpdateRaceRet(setPos);
		*/
	}

	int raceFollowStat = 0;
	void setReverseLookatUma() {
		liveFirstPersonEnableRoll = !liveFirstPersonEnableRoll;
		g_race_freecam_follow_umamusume_distance = -g_race_freecam_follow_umamusume_distance;
		if (cameraType == CAMERA_LIVE) {
			if (liveFirstPersonEnableRoll) {
				printf("Live first person camera enable roll.\n");
			}
			else {
				printf("Live first person camera disable roll.\n");
			}
		}
		else {
			switch (raceFollowStat) {
			case 0: {
				lookAtUmaReverse = !lookAtUmaReverse;
				raceFollowUmaFirstPersion = false;
				raceFollowUmaFirstPersionEnableRoll = false;
				printf(lookAtUmaReverse ? "Race camera ahead.\n" : "Race camera behind.\n");
				raceFollowStat++;
			}; break;
			case 1: {
				lookAtUmaReverse = !lookAtUmaReverse;
				raceFollowUmaFirstPersion = false;
				raceFollowUmaFirstPersionEnableRoll = false;
				printf(lookAtUmaReverse ? "Race camera ahead.\n" : "Race camera behind.\n");
				raceFollowStat++;
			}; break;
			case 2: {
				raceFollowUmaFirstPersion = true;
				raceFollowUmaFirstPersionEnableRoll = true;
				printf("Race first persion. Enable roll.\n");
				raceFollowStat++;
			}; break;
			
			case 3: {
				raceFollowUmaFirstPersion = true;
				raceFollowUmaFirstPersionEnableRoll = false;
				printf("Race first persion. Disable roll.\n");
				raceFollowStat = 0;
			}; break;
			
			default: {
				raceFollowUmaFirstPersion = false;
				raceFollowStat = 0;
			}; break;
			}
		}

		
	}

	void camera_forward() {  // 向前
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume){
			if (raceFollowUmaFirstPersion) {
				liveFirstPersonOffset.z += 0.005;
			}
			else {
				g_race_freecam_follow_umamusume_offset.z -= moveStep / 2;
				g_race_freecam_follow_umamusume_distance += moveStep / 2;
			}
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraOffset.z -= moveStep / 2;
			return;
		}		
		
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FIRST_PERSION) {
			liveFirstPersonOffset.z += 0.005;
			return;
		}

		set_lon_move(verticalAngle, LonMoveForward);
		set_homecam_lon_move(homeCameraAngle);
	}

	void camera_back() {  // 后退
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (raceFollowUmaFirstPersion) {
				liveFirstPersonOffset.z -= 0.005;
			}
			else {
				g_race_freecam_follow_umamusume_offset.z += moveStep / 2;
				g_race_freecam_follow_umamusume_distance -= moveStep / 2;
			}
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraOffset.z += moveStep / 2;
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FIRST_PERSION) {
			liveFirstPersonOffset.z -= 0.005;
			return;
		}

		set_lon_move(verticalAngle + 180, LonMoveBack);
		set_homecam_lon_move(homeCameraAngle + 180);
	}

	void camera_left() {  // 向左
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			g_race_freecam_follow_umamusume_offset.x += moveStep / 2;
			raceFollowLookatOffset.x -= moveStep * 2;
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraLookatOffset.x += moveStep;
			// if (liveFollowCameraOffset.x > 360) liveFollowCameraOffset.x = 0;
			// liveFollowCameraOffset.x += moveStep * 5;
			return;
		}

		set_lon_move(verticalAngle + 90);
		set_homecam_lon_move(homeCameraAngle + 90);
	}

	void camera_right() {  // 向右
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			g_race_freecam_follow_umamusume_offset.x -= moveStep / 2;
			raceFollowLookatOffset.x += moveStep * 2;
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraLookatOffset.x -= moveStep;
			// if (liveFollowCameraOffset.x < 0) liveFollowCameraOffset.x = 360;
			// liveFollowCameraOffset.x -= moveStep * 5;
			return;
		}

		set_lon_move(verticalAngle - 90);
		set_homecam_lon_move(homeCameraAngle - 90);
	}

	void camera_down() {  // 向下
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (raceFollowUmaFirstPersion) {
				liveFirstPersonOffset.y -= 0.005;
			}
			else {
				g_race_freecam_follow_umamusume_offset.y -= moveStep / 8;
			}
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraLookatOffset.y -= moveStep / 2;
			// liveFollowCameraOffset.y -= moveStep / 2;
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FIRST_PERSION) {
			liveFirstPersonOffset.y -= 0.005;
			return;
		}

		float preStep;
		if (cameraType == CAMERA_RACE) {
			preStep = moveStep / 3 / smoothLevel;
		}
		else {
			preStep = moveStep / smoothLevel;
		}

		homeCameraPos.y -= moveStep;
		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.y -= preStep;
			cameraLookAt.y -= preStep;
			Sleep(sleepTime);
		}
	}
	
	void camera_up() {  // 向上
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (raceFollowUmaFirstPersion) {
				liveFirstPersonOffset.y += 0.005;
			}
			else {
				g_race_freecam_follow_umamusume_offset.y += moveStep / 8;
			}
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraLookatOffset.y += moveStep / 3;
			// liveFollowCameraOffset.y += moveStep / 2;
			return;
		}
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FIRST_PERSION) {
			liveFirstPersonOffset.y += 0.005;
			return;
		}

		float preStep;
		if (cameraType == CAMERA_RACE) {
			preStep = moveStep / 3 / smoothLevel;
		}
		else {
			preStep = moveStep / smoothLevel;
		}

		homeCameraPos.y += moveStep;
		for (int i = 0; i < smoothLevel; i++) {
			cameraPos.y += preStep;
			cameraLookAt.y += preStep;
			Sleep(sleepTime);
		}
	}

	void setVertLook(float vertangle, float horiangle) {  // 上+
		if (lookVertRunning) return;
		lookVertRunning = true;
		auto radian = vertangle * M_PI / 180;
		auto radian2 = ((double)horiangle - 90) * M_PI / 180;  // 日

		auto stepX1 = look_radius * sin(radian2) * cos(radian) / smoothLevel;
		auto stepX2 = look_radius * sin(radian2) * sin(radian) / smoothLevel;
		auto stepX3 = look_radius * cos(radian2) / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraLookAt.z = cameraPos.z + stepX1;
			cameraLookAt.y = cameraPos.y + stepX3;
			cameraLookAt.x = cameraPos.x - stepX2;
			Sleep(sleepTime);
		}
		lookVertRunning = false;
	}

	void setHoriLook(float vertangle) {  // 左+
		if (lookHoriRunning) return;
		lookHoriRunning = true;
		auto radian = vertangle * M_PI / 180;
		auto radian2 = horizontalAngle * M_PI / 180;

		auto stepBt = cos(radian) * look_radius * cos(radian2) / smoothLevel;
		auto stepHi = sin(radian) * look_radius * cos(radian2) / smoothLevel;
		auto stepY = sin(radian2) * look_radius / smoothLevel;

		for (int i = 0; i < smoothLevel; i++) {
			cameraLookAt.x = cameraPos.x + stepHi;
			cameraLookAt.z = cameraPos.z - stepBt;
			cameraLookAt.y = cameraPos.y + stepY;
			Sleep(sleepTime);
		}
		lookHoriRunning = false;
	}

	void cameraLookat_up(float mAngel, bool mouse = false) {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (mouse) return;
			g_race_freecam_follow_umamusume_distance += 0.2;
			return;
		}

		if (cameraType == CAMERA_LIVE && liveCameraType != LiveCamera_FREE) return;

		horizontalAngle += mAngel;
		if (horizontalAngle >= 90) horizontalAngle = 89.99;
		setVertLook(verticalAngle, horizontalAngle);
	}

	void cameraLookat_down(float mAngel, bool mouse = false) {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (mouse) return;
			g_race_freecam_follow_umamusume_distance -= 0.2;
			return;
		}

		if (cameraType == CAMERA_LIVE && liveCameraType != LiveCamera_FREE) return;

		horizontalAngle -= mAngel;
		if (horizontalAngle <= -90) horizontalAngle = -89.99;
		setVertLook(verticalAngle, horizontalAngle);
	}

	void cameraLookat_left(float mAngel) {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			return;
		}

		if (cameraType == CAMERA_LIVE && liveCameraType != LiveCamera_FREE) return;

		verticalAngle += mAngel;
		if (verticalAngle >= 360) verticalAngle = -360;
		setHoriLook(verticalAngle);
	}

	void cameraLookat_right(float mAngel) {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			return;
		}

		if (cameraType == CAMERA_LIVE && liveCameraType != LiveCamera_FREE) return;

		verticalAngle -= mAngel;
		if (verticalAngle <= -360) verticalAngle = 360;
		setHoriLook(verticalAngle);
	}

	void singleLeftClick() {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			if (g_race_freecam_follow_umamusume_index <= -1) return;
			g_race_freecam_follow_umamusume_index--;
			if (g_race_freecam_follow_umamusume_index <= -1) {
				printf("Look at auto\n");
			}
			else {
				printf("Look at No.%d\n", g_race_freecam_follow_umamusume_index + 1);
			}
		}
	}

	void singleRightClick() {
		if ((cameraType == CAMERA_RACE) && g_race_freecam_follow_umamusume) {
			g_race_freecam_follow_umamusume_index++;
			printf("Look at No.%d\n", g_race_freecam_follow_umamusume_index + 1);
		}
	}

	void changeCameraFOV(float value) {
		if (cameraType == CAMERA_RACE) {
			raceDefaultFOV += value;
			printf("Race camera FOV has been changed to %f\n", raceDefaultFOV);
		}
		else {
			if (!isLiveStart) return;
			liveDefaultFOV += value;
			printf("Live camera FOV has been changed to %f\n", liveDefaultFOV);
		}
	}

	void changeFollowTargetState() {
		if (cameraType == CAMERA_RACE) {
			if (g_race_freecam_follow_umamusume) {
				g_race_freecam_follow_umamusume = false;
				g_race_freecam_lookat_umamusume = orig_lookat_target;
				printf("Free Camera!\n");
			}
			else {
				g_race_freecam_follow_umamusume = true;
				g_race_freecam_lookat_umamusume = true;
				printf("Follow Umamusume!\n");
			}
		}
		else if (cameraType == CAMERA_LIVE) {
			if (!isLiveStart) return;

			if (liveCameraType == LiveCamera_FREE) {
				liveCameraType = LiveCamera_FOLLOW_UMA;
				printf("LIVE Follow Umamusume\n");
			}
			else if (liveCameraType == LiveCamera_FOLLOW_UMA) {
				liveCameraType = LiveCamera_FIRST_PERSION;
				printf("LIVE First Persion\n");
			}
			else {
				liveCameraType = LiveCamera_FREE;
				printf("LIVE Free Camera\n");
			}
		}

	}

	int GetLiveCharaPositionFlag() {
		return liveCharaPositionFlag.GetCurrentValue();
	}
	
	int GetLiveCharaPositionIndex() {
		return liveCharaPositionFlag.GetCurrentIndex();
	}

	int GetLiveCameraCharaParts() {
		return liveCameraCharaParts.GetCurrentValue();
	}

	void changeLiveCameraLockChara(int changeIndex) {
		if (cameraType != CAMERA_LIVE) return;
		if ((liveCameraType != LiveCamera_FOLLOW_UMA) && (liveCameraType != LiveCamera_FIRST_PERSION)) return;

		const auto changedData = changeIndex > 0 ? liveCharaPositionFlag.Next() : liveCharaPositionFlag.Last();
		printf("Live look position flag: %s (0x%x)\n", changedData.first.c_str(), changedData.second);
	}

	void changeLiveCameraLockPosition(int changeIndex) {
		if (cameraType != CAMERA_LIVE) return;
		if (liveCameraType != LiveCamera_FOLLOW_UMA) return;

		const auto changedData = changeIndex > 0 ? liveCameraCharaParts.Next() : liveCameraCharaParts.Last();
		printf("Live look chara parts: %s (0x%x)\n", changedData.first.c_str(), changedData.second);
	}

	void changeLiveFollowCameraOffsetX(float value, bool mouse = false) {
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraOffset.x += value * 2;
		}
		if (cameraType == CAMERA_RACE) {
			if (g_race_freecam_follow_umamusume) {
				if (raceFollowUmaFirstPersion) {
					if (mouse)
						raceFirstPersonLookAtOffset.x -= value;
					else
						raceFirstPersonLookAtOffset.x += value;
				}
				else {
					raceFollowLookatOffset.x -= value;
				}
			}
		}
	}	
	
	void changeLiveFollowCameraOffsetY(float value, bool mouse = false) {
		if ((cameraType == CAMERA_LIVE) && liveCameraType == LiveCamera_FOLLOW_UMA) {
			liveFollowCameraOffset.y += value;
		}
		if (cameraType == CAMERA_RACE) {
			if (g_race_freecam_follow_umamusume) {
				if (raceFollowUmaFirstPersion) {
					if (mouse)
						raceFirstPersonLookAtOffset.y -= value * 16;
					else
						raceFirstPersonLookAtOffset.y += value * 16;
				}
				else {
					raceFollowLookatOffset.y += value / 2;
					if (mouse) g_race_freecam_follow_umamusume_offset.y += value / 2;
				}
			}
		}
	}

	void mouseMove(LONG moveX, LONG moveY, int mouseEventType) {
		if (mouseEventType == 1) {  // down
			rMousePressFlg = true;
			int fCount = 0;
			while (ShowCursor(false) >= 0) {
				if (fCount >= 5) break;
				fCount++;
			}
		}
		else if (mouseEventType == 2) {  // up
			rMousePressFlg = false;
			int fCount = 0;
			while (ShowCursor(true) < 0) {
				if (fCount >= 5) break;
				fCount++;
			}
		}
		else if (mouseEventType == 3) {  // move
			std::thread([moveX, moveY]() {
				if (!rMousePressFlg) return;
				if (moveX > 0) {
					cameraLookat_right(moveX * g_free_camera_mouse_speed / 100.0);
					changeLiveFollowCameraOffsetX(moveX * g_free_camera_mouse_speed / 50.0, true);
				}
				else if (moveX < 0) {
					cameraLookat_left(-moveX * g_free_camera_mouse_speed / 100.0);
					changeLiveFollowCameraOffsetX(moveX * g_free_camera_mouse_speed / 50.0, true);
				}
				if (moveY > 0) {
					cameraLookat_down(moveY * g_free_camera_mouse_speed / 100.0, true);
					changeLiveFollowCameraOffsetY(moveY * g_free_camera_mouse_speed / 800.0, true);
				}
				else if (moveY < 0) {
					cameraLookat_up(-moveY * g_free_camera_mouse_speed / 100.0, true);
					changeLiveFollowCameraOffsetY(moveY * g_free_camera_mouse_speed / 800.0, true);
				}
				// printf("move x: %d, y: %d\n", moveX, moveY);
				}).detach();
		}
	}

	struct CameraMoveState {
		bool w = false;
		bool s = false;
		bool a = false;
		bool d = false;
		bool ctrl = false;
		bool space = false;
		bool up = false;
		bool down = false;
		bool left = false;
		bool right = false;
		bool q = false;
		bool e = false;
		bool i = false;
		bool k = false;
		bool j = false;
		bool l = false;
		bool threadRunning = false;
	} cameraMoveState;

	void cameraRawInputThread() {
		std::thread([]() {
			if (cameraMoveState.threadRunning) return;
			cameraMoveState.threadRunning = true;
			while (true) {
				if (cameraMoveState.w) camera_forward();
				if (cameraMoveState.s) camera_back();
				if (cameraMoveState.a) camera_left();
				if (cameraMoveState.d) camera_right();
				if (cameraMoveState.ctrl) camera_down();
				if (cameraMoveState.space) camera_up();
				if (cameraMoveState.up) cameraLookat_up(moveAngel);
				if (cameraMoveState.down) cameraLookat_down(moveAngel);
				if (cameraMoveState.left) cameraLookat_left(moveAngel);
				if (cameraMoveState.right) cameraLookat_right(moveAngel);
				if (cameraMoveState.q) changeCameraFOV(0.5f);
				if (cameraMoveState.e) changeCameraFOV(-0.5f);
				if (cameraMoveState.i) changeLiveFollowCameraOffsetY(moveStep / 3);
				if (cameraMoveState.k) changeLiveFollowCameraOffsetY(-moveStep / 3);
				if (cameraMoveState.j) changeLiveFollowCameraOffsetX(moveStep * 10);
				if (cameraMoveState.l) changeLiveFollowCameraOffsetX(-moveStep * 10);
				Sleep(10);
			}
			}).detach();
	}

	void on_cam_rawinput_keyboard(int message, int key) {
		if (message == WM_KEYDOWN || message == WM_KEYUP) {
			switch (key) {
			case KEY_W:
				cameraMoveState.w = message == WM_KEYDOWN ? true : false; break;
			case KEY_S:
				cameraMoveState.s = message == WM_KEYDOWN ? true : false; break;
			case KEY_A:
				cameraMoveState.a = message == WM_KEYDOWN ? true : false; break;
			case KEY_D:
				cameraMoveState.d = message == WM_KEYDOWN ? true : false; break;
			case KEY_CTRL:
				cameraMoveState.ctrl = message == WM_KEYDOWN ? true : false; break;
			case KEY_SPACE:
				cameraMoveState.space = message == WM_KEYDOWN ? true : false; break;
			case KEY_UP:
				cameraMoveState.up = message == WM_KEYDOWN ? true : false; break;
			case KEY_DOWN:
				cameraMoveState.down = message == WM_KEYDOWN ? true : false; break;
			case KEY_LEFT:
				cameraMoveState.left = message == WM_KEYDOWN ? true : false; break;
			case KEY_RIGHT:
				cameraMoveState.right = message == WM_KEYDOWN ? true : false; break;
			case 'Q':
				cameraMoveState.q = message == WM_KEYDOWN ? true : false; break;
			case 'E':
				cameraMoveState.e = message == WM_KEYDOWN ? true : false; break;
			case 'I':
				cameraMoveState.i = message == WM_KEYDOWN ? true : false; break;
			case 'K':
				cameraMoveState.k = message == WM_KEYDOWN ? true : false; break;
			case 'J':
				cameraMoveState.j = message == WM_KEYDOWN ? true : false; break;
			case 'L':
				cameraMoveState.l = message == WM_KEYDOWN ? true : false; break;
			default: break;
			}
		}
	}

	void on_keyboard_down(int key, DWORD shift, DWORD ctrl, DWORD alt, DWORD space, DWORD up, DWORD down, DWORD left, DWORD right) {
		// bool ctrl_down = (key == KEY_CTRL) || (ctrl != 0);
		// bool space_down = (key == KEY_SPACE) || (space != 0);
		bool up_down = (key == KEY_UP);
		bool down_down = (key == KEY_DOWN);
		bool left_down = (key == KEY_LEFT);
		bool right_down = (key == KEY_RIGHT);

		switch (key) {
			case KEY_R:
				reset_camera(); break;
			case 'F':
				changeFollowTargetState(); break;
			case KEY_LEFT:
				singleLeftClick(); break;
			case KEY_RIGHT:
				singleRightClick(); break;
			case 'V':
				setReverseLookatUma(); break;
			default:
				break;
		}

		// if (ctrl_down) camera_down();
		// if (space_down) camera_up();
		if (up_down) changeLiveCameraLockPosition(1);
		if (down_down) changeLiveCameraLockPosition(-1);
		if (left_down) changeLiveCameraLockChara(-1);
		if (right_down) changeLiveCameraLockChara(1);

	}

	void initCameraSettings() {
		reset_camera();
		cameraRawInputThread();
		MHotkey::setMKeyBoardRawCallBack(on_cam_rawinput_keyboard);
		MHotkey::SetKeyCallBack(on_keyboard_down);
	}
}
