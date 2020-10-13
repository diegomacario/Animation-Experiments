#include <glm/gtx/norm.hpp>

#include "quat.h"
#include <cmath>
#include <iostream>

// Note: I will need a normalize function that checks that the length of the vector is not 0

glm::vec3 normalize(const glm::vec3& vec)
{
	float lengthSquared = glm::length2(vec);
	if (lengthSquared == 0.0f) // TODO: Use threshold here
	{
		return vec;
	}

	return vec / glm::sqrt(lengthSquared);
}

quat angleAxis(float angle, const glm::vec3& axis)
{
	glm::vec3 normalizedAxis = normalize(axis);

	float s = sinf(angle * 0.5f);

	return quat(
		normalizedAxis.x * s,
		normalizedAxis.y * s,
		normalizedAxis.z * s,
		cosf(angle * 0.5f)
	);
}

quat fromTo(const glm::vec3& from, const glm::vec3& to) {
	glm::vec3 f = normalize(from);
	glm::vec3 t = normalize(to);

	if (f == t) {
		return quat();
	}
	else if (f == t * -1.0f) {
		glm::vec3 ortho = glm::vec3(1, 0, 0);
		if (fabsf(f.y) < fabsf(f.x)) {
			ortho = glm::vec3(0, 1, 0);
		}
		if (fabsf(f.z) < fabs(f.y) && fabs(f.z) < fabsf(f.x)) {
			ortho = glm::vec3(0, 0, 1);
		}

		glm::vec3 axis = normalize(glm::cross(f, ortho));
		return quat(axis.x, axis.y, axis.z, 0);
	}

	glm::vec3 half = normalize(f + t);
	glm::vec3 axis = glm::cross(f, half);

	return quat(
		axis.x,
		axis.y,
		axis.z,
		dot(f, half)
	);
}

glm::vec3 getAxis(const quat& quat) {
	return normalize(glm::vec3(quat.x, quat.y, quat.z));
}

float getAngle(const quat& quat) {
	return 2.0f * acosf(quat.w);
}

quat operator+(const quat& a, const quat& b) {
	return quat(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z,
		a.w + b.w
	);
}

quat operator-(const quat& a, const quat& b) {
	return quat(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z,
		a.w - b.w
	);
}

quat operator*(const quat& a, float b) {
	return quat(
		a.x * b,
		a.y * b,
		a.z * b,
		a.w * b
	);
}

quat operator-(const quat& q) {
	return quat(
		-q.x,
		-q.y,
		-q.z,
		-q.w
	);
}

bool operator==(const quat& left, const quat& right) {
	return (fabsf(left.x - right.x) <= QUAT_EPSILON && fabsf(left.y - right.y) <= QUAT_EPSILON && fabsf(left.z - right.z) <= QUAT_EPSILON && fabsf(left.w - left.w) <= QUAT_EPSILON);
}

bool operator!=(const quat& a, const quat& b) {
	return !(a == b);
}

bool sameOrientation(const quat& left, const quat& right) {
	return (fabsf(left.x - right.x) <= QUAT_EPSILON && fabsf(left.y - right.y) <= QUAT_EPSILON && fabsf(left.z - right.z) <= QUAT_EPSILON && fabsf(left.w - left.w) <= QUAT_EPSILON)
		|| (fabsf(left.x + right.x) <= QUAT_EPSILON && fabsf(left.y + right.y) <= QUAT_EPSILON && fabsf(left.z + right.z) <= QUAT_EPSILON && fabsf(left.w + left.w) <= QUAT_EPSILON);
}

float dot(const quat& a, const quat& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float lenSq(const quat& q) {
	return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

float len(const quat& q) {
	float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (lenSq < QUAT_EPSILON) {
		return 0.0f;
	}
	return sqrtf(lenSq);
}

void normalize(quat& q) {
	float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (lenSq < QUAT_EPSILON) {
		return;
	}
	float i_len = 1.0f / sqrtf(lenSq);

	q.x *= i_len;
	q.y *= i_len;
	q.z *= i_len;
	q.w *= i_len;
}

quat normalized(const quat& q) {
	float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (lenSq < QUAT_EPSILON) {
		return quat();
	}
	float i_len = 1.0f / sqrtf(lenSq);

	return quat(
		q.x * i_len,
		q.y * i_len,
		q.z * i_len,
		q.w * i_len
	);
}

quat conjugate(const quat& q) {
	return quat(
		-q.x,
		-q.y,
		-q.z,
		q.w
	);
}

quat inverse(const quat& q) {
	float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (lenSq < QUAT_EPSILON) {
		return quat();
	}
	float recip = 1.0f / lenSq;

	// conjugate / norm
	return quat(
		-q.x * recip,
		-q.y * recip,
		-q.z * recip,
		q.w * recip
	);
}

// By Gabor
/*
quat operator*(const quat& Q1, const quat& Q2) {
	return quat(
		Q2.x * Q1.w + Q2.y * Q1.z - Q2.z * Q1.y + Q2.w * Q1.x,
		-Q2.x * Q1.z + Q2.y * Q1.w + Q2.z * Q1.x + Q2.w * Q1.y,
		Q2.x * Q1.y - Q2.y * Q1.x + Q2.z * Q1.w + Q2.w * Q1.z,
		-Q2.x * Q1.x - Q2.y * Q1.y - Q2.z * Q1.z + Q2.w * Q1.w
	);
}
*/

// By Gabor
quat operator*(const quat& Q, const quat& P)
{
	quat result;
	result.scalar = P.scalar * Q.scalar - dot(P.vector, Q.vector);
	result.vector = (Q.vector * P.scalar) + (P.vector * Q.scalar) + cross(P.vector, Q.vector);
	return result;
}

// By Gabor
/*
glm::vec3 operator*(const quat& q, const glm::vec3& v) {
	return    q.vector * 2.0f * dot(q.vector, v) +
		v * (q.scalar * q.scalar - dot(q.vector, q.vector)) +
		cross(q.vector, v) * 2.0f * q.scalar;
}
*/

// By Gabor
glm::vec3 operator*(const quat& q, const glm::vec3& v)
{
	quat result = inverse(q) * (quat(v.x, v.y, v.z, 0) * q);
	return glm::vec3(result.x, result.y, result.z);
}

// By 3DGEP
/*
quat operator*(const quat& Q, const quat& P)
{
	quat result;
	result.scalar = Q.scalar * P.scalar - dot(Q.vector, P.vector);
	result.vector = (Q.scalar * P.vector) + (P.scalar * Q.vector) + cross(Q.vector, P.vector);
	return result;
}
*/

// By 3DGEP
/*
glm::vec3 operator*(const quat& q, const glm::vec3& v)
{
	quat result = q * quat(v.x, v.y, v.z, 0) * inverse(q);
	return glm::vec3(result.x, result.y, result.z);
}
*/

quat mix(const quat& from, const quat& to, float t) {
	return from * (1.0f - t) + to * t;
}

quat nlerp(const quat& from, const quat& to, float t) {
	return normalized(from + (to - from) * t);
}

quat operator^(const quat& q, float f) {
	float angle = 2.0f * acosf(q.scalar);
	glm::vec3 axis = normalize(q.vector);

	float halfCos = cosf(f * angle * 0.5f);
	float halfSin = sinf(f * angle * 0.5f);

	return quat(
		axis.x * halfSin,
		axis.y * halfSin,
		axis.z * halfSin,
		halfCos
	);
}

quat slerp(const quat& start, const quat& end, float t) {
	if (fabsf(dot(start, end)) > 1.0f - QUAT_EPSILON) {
		return nlerp(start, end, t);
	}

	// TODO: In Gabor's written description this is (end * inverse(start))
	//return normalized(((inverse(start) * end) ^ t) * start);
   return normalized(start * ((inverse(start) * end) ^ t));
}

// TODO: Will play with this
// This is still mysterious to me
quat lookRotation(const glm::vec3& direction, const glm::vec3& up) {
	// Find orthonormal basis vectors
	glm::vec3 f = normalize(direction);
	glm::vec3 u = normalize(up);
	glm::vec3 r = cross(u, f);
	u = cross(f, r);

	// From world forward to object forward
	quat f2d = fromTo(glm::vec3(0, 0, 1), f);

	// what direction is the new object up?
	glm::vec3 objectUp = f2d * glm::vec3(0, 1, 0);
	// From object up to desired up
	quat u2u = fromTo(objectUp, u);

	// TODO: The comment below is weird, isn't it? If we want to rotate to fwd first and then twist to correct up, should we be doing this? u2u * f2d
	// Rotate to forward direction first, then twist to correct up
	quat result = f2d * u2u;
	// Don’t forget to normalize the result
	return normalized(result);
}

// TODO: Need to make sure this plays well with GLM
glm::mat4 quatToMat4(const quat& q) {
	glm::vec3 r = q * glm::vec3(1, 0, 0);
	glm::vec3 u = q * glm::vec3(0, 1, 0);
	glm::vec3 f = q * glm::vec3(0, 0, 1);

	return glm::mat4(
		r.x, r.y, r.z, 0,
		u.x, u.y, u.z, 0,
		f.x, f.y, f.z, 0,
		0, 0, 0, 1
	);
}

// TODO: Need to make sure this plays well with GLM
// How do I access up and forward in glm?
quat mat4ToQuat(const glm::mat4& m) {
	glm::vec3 up = normalize(glm::vec3(m[1].x, m[1].y, m[1].z));
	glm::vec3 forward = normalize(glm::vec3(m[2].x, m[2].y, m[2].z));
	glm::vec3 right = cross(up, forward);
	up = glm::cross(forward, right);

	return lookRotation(forward, up);
}