#ifndef QUAT_H
#define QUAT_H

#include <glm/glm.hpp>

#define QUAT_EPSILON 0.000001f

struct quat {
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			glm::vec3 vector;
			float scalar;
		};
		float v[4];
	};

	inline quat() :
		x(0), y(0), z(0), w(1) { }
	inline quat(float _x, float _y, float _z, float _w) :
		x(_x), y(_y), z(_z), w(_w) {}
};

quat angleAxis(float angle, const glm::vec3& axis);
quat fromTo(const glm::vec3& from, const glm::vec3& to);
glm::vec3 getAxis(const quat& quat);
float getAngle(const quat& quat);
quat operator+(const quat& a, const quat& b);
quat operator-(const quat& a, const quat& b);
quat operator*(const quat& a, float b);
quat operator-(const quat& q);
bool operator==(const quat& left, const quat& right);
bool operator!=(const quat& a, const quat& b);
bool sameOrientation(const quat& left, const quat& right);
float dot(const quat& a, const quat& b);
float lenSq(const quat& q);
float len(const quat& q);
void normalize(quat& q);
quat normalized(const quat& q);
quat conjugate(const quat& q);
quat inverse(const quat& q);
quat operator*(const quat& Q1, const quat& Q2);
quat operator*(const quat& Q1, const quat& Q2);
glm::vec3 operator*(const quat& q, const glm::vec3& v);
quat mix(const quat& from, const quat& to, float t);
quat nlerp(const quat& from, const quat& to, float t);
quat operator^(const quat& q, float f);
quat slerp(const quat& start, const quat& end, float t);
quat lookRotation(const glm::vec3& direction, const glm::vec3& up);
glm::mat4 quatToMat4(const quat& q);
quat mat4ToQuat(const glm::mat4& m);

#endif
