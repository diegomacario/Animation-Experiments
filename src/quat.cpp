#include <glm/gtx/norm.hpp>
#include <cstdio>

#include "quat.h"

Q::quat Q::angleAxis(float angle, const glm::vec3& axis)
{
   glm::vec3 nAxis = normalizeWithZeroLengthCheck(axis);

   float halfSin = glm::sin(angle * 0.5f);

   return Q::quat(nAxis.x * halfSin,
                  nAxis.y * halfSin,
                  nAxis.z * halfSin,
                  glm::cos(angle * 0.5f));
}

float Q::getAngle(const Q::quat& q)
{
   return 2.0f * glm::acos(q.w);
}

glm::vec3 Q::getAxis(const Q::quat& q)
{
   return normalizeWithZeroLengthCheck(glm::vec3(q.x, q.y, q.z));
}

Q::quat Q::fromTo(const glm::vec3& from, const glm::vec3& to)
{
   glm::vec3 nFrom = normalizeWithZeroLengthCheck(from);
   glm::vec3 nTo   = normalizeWithZeroLengthCheck(to);

   // TODO: Check all == comparisons in project and create VEC3_EPSILON
   if (glm::length2(nFrom - nTo) < QUAT_EPSILON) // nFrom == nTo
   {
      // If from and to are the same, then we return a unit quaternion
      return Q::quat();
   }
   else if (glm::length2(nFrom - (nTo * -1.0f)) < QUAT_EPSILON) // nFrom == -nTo
   {
      // If from and to point in opposite directions, then there is no unique half vector between the two
      // In that case, we use the most orthogonal basis vector with respect to from to find the axis of rotation

      // The smallest absolute component of the from vector tells us which basis vector is more orthogonal
      glm::vec3 mostOrthogBasis = glm::vec3(1, 0, 0);
      if (glm::abs(nFrom.y) < glm::abs(nFrom.x))
      {
         mostOrthogBasis = glm::vec3(0, 1, 0);
      }
      if (glm::abs(nFrom.z) < glm::abs(nFrom.y) && glm::abs(nFrom.z) < glm::abs(nFrom.x))
      {
         mostOrthogBasis = glm::vec3(0, 0, 1);
      }

      // The axis of rotation can be found by taking the cross product of the from vector and its most orthogonal basis vector
      glm::vec3 axisOfRot = normalizeWithZeroLengthCheck(glm::cross(nFrom, mostOrthogBasis));

      // Remember that a quaternion is constructed in this way:
      //
      // quat(axisOfRot.x * halfSin,
      //      axisOfRot.y * halfSin,
      //      axisOfRot.z * halfSin,
      //      glm::cos(angle * 0.5f));
      //
      // Since the angle between the from and to vectors is 180 degrees, the half angle is 90 degrees
      // This means that the half sines evaluate to 1 and that the half cosine evaluates to 0, leaving our quaternion looking as it does below:
      return Q::quat(axisOfRot.x, axisOfRot.y, axisOfRot.z, 0);
   }

   // If from and to are not the same and if they don't point in opposite directions,
   // then we find the half vector between the two and we use it to construct a quaternion that rotates half of the desired angle
   // Why half? Because when we use the formula q * v * q^-1 to rotate a vector, the two multiplications will make us rotate the desired angle
   glm::vec3 halfVecBetweenFromAndTo = normalizeWithZeroLengthCheck(nFrom + nTo);

   // The cross product of unit vectors is equal to a vector orthogonal to both times the sine of the angle between them
   glm::vec3 axisOfRot = glm::cross(nFrom, halfVecBetweenFromAndTo);

   return Q::quat(axisOfRot.x, // The multiplication by the sine is already encoded in the axis of rotation
                  axisOfRot.y,
                  axisOfRot.z,
                  glm::dot(nFrom, halfVecBetweenFromAndTo)); // The dot product of unit vectors is equal to the cosine of the angle between them
}

Q::quat Q::operator+(const Q::quat& a, const Q::quat& b)
{
   return Q::quat(a.x + b.x,
                  a.y + b.y,
                  a.z + b.z,
                  a.w + b.w);
}

Q::quat Q::operator-(const Q::quat& a, const Q::quat& b)
{
   return Q::quat(a.x - b.x,
                  a.y - b.y,
                  a.z - b.z,
                  a.w - b.w);
}

// Negation operator
Q::quat Q::operator-(const Q::quat& q)
{
   return Q::quat(-q.x, -q.y, -q.z, -q.w);
}

Q::quat Q::operator*(const Q::quat& q, float f)
{
   return Q::quat(q.x * f,
                  q.y * f,
                  q.z * f,
                  q.w * f);
}

Q::quat Q::operator*(float f, const Q::quat& q)
{
   return Q::quat(f * q.x,
                  f * q.y,
                  f * q.z,
                  f * q.w);
}

// This multiplication operator reverses the order of the arguments it receives
// So if we multiply q * p in the code, we are really multiplying p * q
// The advantages of doing this are the following:
// - Multiplying from the left now causes an object to rotate with respect to its local coordinate axes
// - Multiplying from the right now causes an object to rotate with respect to the world's coordinate axes
// This means that we can apply children rotations from the left: Qch * Qparent
// So the signature of this function could also be written like this:
// Q::quat Q::operator*(const Q::quat& child, const Q::quat& parent)
Q::quat Q::operator*(const Q::quat& q, const Q::quat& p)
{
   // The block of operations we return in this function is simply an optimized version of the commented code below,
   // which implements the general equation of the p * q quaternion product.
   /*
   quat result;
   result.scalar = p.scalar * q.scalar - glm::dot(p.vector, q.vector);
   result.vector = (p.scalar * q.vector) + (q.scalar * p.vector) + glm::cross(p.vector, q.vector);
   return result;
   */

   return Q::quat( p.x * q.w + p.y * q.z - p.z * q.y + p.w * q.x,
                  -p.x * q.z + p.y * q.w + p.z * q.x + p.w * q.y,
                   p.x * q.y - p.y * q.x + p.z * q.w + p.w * q.z,
                  -p.x * q.x - p.y * q.y - p.z * q.z + p.w * q.w);
}

glm::vec3 Q::operator*(const Q::quat& q, const glm::vec3& v)
{
   // The block of operations we return in this function is simply an optimized version of the commented code below,
   // which implements the equation used to rotate a vector using a quaternion.
   // Note that the order of the equation is reversed (q^-1 * v * q instead of q * v *q^-1) because of the way we implemented
   // the multiplication operator
   /*
   quat result = inverse(q) * quat(v.x, v.y, v.z, 0) * q;
   return glm::vec3(result.x, result.y, result.z);
   */

   // Also note that this assumes that the inverse of the quaterion q is its conjugate,
   // which in other words means that it assumes that the quaternion q is normalized, unlike the commented code above
   return q.vector * 2.0f * glm::dot(q.vector, v) +
          v * (q.scalar * q.scalar - glm::dot(q.vector, q.vector)) +
          glm::cross(q.vector, v) * 2.0f * q.scalar;
}

// The multiplication operator below doesn't reverse the order of the arguments it receives
// I'm leaving it here along with its corresponding quaternion-vector multiplication operator as a reference
// The consequences of not reversing the order of the arguments are the following:
// - Multiplying from the left causes an object to rotate with respect to the world's coordinate axes
// - Multiplying from the right causes an object to rotate with respect to its local coordinate axes
// This means that children rotations must be applied from the right: Qparent * Qch
// So the signature of this function could also be written like this:
// Q::quat Q::operator*(const Q::quat& parent, const Q::quat& child)
/*
quat operator*(const quat& q, const quat& p)
{
   quat result;
   result.scalar = q.scalar * p.scalar - glm::dot(q.vector, p.vector);
   result.vector = (q.scalar * p.vector) + (p.scalar * q.vector) + glm::cross(q.vector, p.vector);
   return result;
}

glm::vec3 operator*(const quat& q, const glm::vec3& v)
{
   quat result = q * quat(v.x, v.y, v.z, 0) * inverse(q);
   return glm::vec3(result.x, result.y, result.z);
}
*/

Q::quat Q::operator^(const Q::quat& q, float exponent)
{
   // Raising a quaternion to some power simply means scaling its angle
   // Here we decompose the quaternion into its axis and angle,
   // we scale its angle, and then we put it back together
   float halfAngle = glm::acos(q.scalar);
   glm::vec3 axisOfRot = normalizeWithZeroLengthCheck(q.vector);

   float halfCos = glm::cos(exponent * halfAngle);
   float halfSin = glm::sin(exponent * halfAngle);

   return Q::quat(axisOfRot.x * halfSin,
                  axisOfRot.y * halfSin,
                  axisOfRot.z * halfSin,
                  halfCos);
}

bool Q::operator==(const Q::quat& a, const Q::quat& b)
{
   return (glm::abs(a.x - b.x) <= QUAT_EPSILON &&
           glm::abs(a.y - b.y) <= QUAT_EPSILON &&
           glm::abs(a.z - b.z) <= QUAT_EPSILON &&
           glm::abs(a.w - b.w) <= QUAT_EPSILON);
}

bool Q::operator!=(const Q::quat& a, const Q::quat& b)
{
   return !(a == b);
}

bool Q::sameOrientation(const Q::quat& a, const Q::quat& b)
{
   // A quaternion and its inverse represent the same rotation
   // They simply take different routes to get there
   // That's why we consider a quaternion and its inverse to have the same orientation
   return (glm::abs(a.x - b.x) <= QUAT_EPSILON &&
           glm::abs(a.y - b.y) <= QUAT_EPSILON &&
           glm::abs(a.z - b.z) <= QUAT_EPSILON &&
           glm::abs(a.w - b.w) <= QUAT_EPSILON) ||
          (glm::abs(a.x + b.x) <= QUAT_EPSILON &&
           glm::abs(a.y + b.y) <= QUAT_EPSILON &&
           glm::abs(a.z + b.z) <= QUAT_EPSILON &&
           glm::abs(a.w + b.w) <= QUAT_EPSILON);
}

float Q::dot(const Q::quat& a, const Q::quat& b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float Q::squaredLength(const Q::quat& q)
{
   return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

float Q::length(const Q::quat& q)
{
   float squaredLen = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
   if (squaredLen < QUAT_EPSILON)
   {
      return 0.0f;
   }
   return glm::sqrt(squaredLen);
}

void Q::normalize(Q::quat& q)
{
   float squaredLen = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
   if (squaredLen < QUAT_EPSILON)
   {
      return;
   }
   float invertedLen = 1.0f / glm::sqrt(squaredLen);

   q.x *= invertedLen;
   q.y *= invertedLen;
   q.z *= invertedLen;
   q.w *= invertedLen;
}

Q::quat Q::normalized(const Q::quat& q)
{
   float squaredLen = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
   if (squaredLen < QUAT_EPSILON)
   {
      return Q::quat();
   }
   float invertedLen = 1.0f / glm::sqrt(squaredLen);

   return Q::quat(q.x * invertedLen,
                  q.y * invertedLen,
                  q.z * invertedLen,
                  q.w * invertedLen);
}

Q::quat Q::conjugate(const Q::quat& q)
{
   // Note that the inverse of a unit quaternion is equal to its conjugate
   return Q::quat(-q.x,
                  -q.y,
                  -q.z,
                   q.w);
}

Q::quat Q::inverse(const Q::quat& q)
{
   float squaredLen = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
   if (squaredLen < QUAT_EPSILON)
   {
      return Q::quat();
   }
   float invertedSquaredLen = 1.0f / squaredLen;

   // The inverse of a quaternion is equal to its conjugate divided by its squared length
   return Q::quat(-q.x * invertedSquaredLen,
                  -q.y * invertedSquaredLen,
                  -q.z * invertedSquaredLen,
                   q.w * invertedSquaredLen);
}

// Note: This function assumes that from and to are in the appropriate neighborhood
Q::quat Q::mix(const Q::quat& start, const Q::quat& end, float t)
{
   // This function is similar to a lerp function,
   // with the only difference being that the resulting interpolation traces an arc

   // The formula for linerarly interpolating two things is the following:
   // a + (b - a) * t
   // a + (b * t) - (a * t)
   // a * (1 - t) + (b * t)
   // That last version is how it's implemented here
   // Note that this function doesn't return a normalized quaternion
   return start * (1.0f - t) + end * t;
}

// Note: This function assumes that from and to are in the appropriate neighborhood
Q::quat Q::nlerp(const Q::quat& start, const Q::quat& end, float t)
{
   // nlerp is a fast and good approximation of slerp
   // It's identical to the mix function except that we normalize the result
   return Q::normalized(start + (end - start) * t);
}

// Note: This function assumes that from and to are in the appropriate neighborhood
Q::quat Q::slerp(const Q::quat& start, const Q::quat& end, float t)
{
   // If the start and end quaternions are really close together slerp breaks down,
   // so we fall back on nlerp in that case
   if (glm::abs(dot(start, end)) > 1.0f - QUAT_EPSILON)
   {
      return Q::nlerp(start, end, t);
   }

   // Remember that the formula for linearly interpolating two things is the following:
   // start + (end - start) * t
   // Here is how we adapt that formula for slerp:
   // - The quaternion that rotates from start to end is:
   //   delta = end * start^-1
   // - Raising the delta quaternion to the power of t allows us to scale it without affecting its length
   //   (end * start^-1)^t
   // - Finally, multiplying by the start quaternion gives us our default value for when t is equal to zero:
   //   (end * start^-1)^t * start
   // Notice what happens when t is equal to zero:
   // (end * start^-1)^0 * start = unit * start = start
   // Also notice what happens when t is equal to one
   // (end * start^-1)^1 * start = end * start^-1 * start = end

   // NOTE: Reversed because q * p is implemented as p * q
   return Q::normalized(start * ((Q::inverse(start) * end) ^ t));
}

// lookRotation returns a quaternion that rotates from the world's forward vector (Z) to a desired direction
// This means that the direction that's passed in becomes the forward vector of the rotated basis vectors
Q::quat Q::lookRotation(const glm::vec3& direction, const glm::vec3& upReference)
{
   // Calculate the rotated basis vectors
   glm::vec3 fwd   = normalizeWithZeroLengthCheck(direction);
   glm::vec3 up    = normalizeWithZeroLengthCheck(upReference);
   glm::vec3 right = normalizeWithZeroLengthCheck(glm::cross(up, fwd));
   up = glm::cross(fwd, right);

   // Create a rotation matrix and convert it into a quaternion
   return Q::normalized(Q::mat3ToQuat(glm::mat3(right.x, right.y, right.z,  // Column 0 (X)
                                                up.x,    up.y,    up.z,     // Column 1 (Y)
                                                fwd.x,   fwd.y,   fwd.z))); // Column 2 (Z)
}

// This function is based on the glm::mat3_cast function
glm::mat3 Q::quatToMat3(const Q::quat& q)
{
   glm::mat3 result(1.0f);
   float qxx(q.x * q.x);
   float qyy(q.y * q.y);
   float qzz(q.z * q.z);
   float qxz(q.x * q.z);
   float qxy(q.x * q.y);
   float qyz(q.y * q.z);
   float qwx(q.w * q.x);
   float qwy(q.w * q.y);
   float qwz(q.w * q.z);

   result[0][0] = 1.0f - 2.0f * (qyy + qzz);
   result[0][1] = 2.0f * (qxy + qwz);
   result[0][2] = 2.0f * (qxz - qwy);

   result[1][0] = 2.0f * (qxy - qwz);
   result[1][1] = 1.0f - 2.0f * (qxx + qzz);
   result[1][2] = 2.0f * (qyz + qwx);

   result[2][0] = 2.0f * (qxz + qwy);
   result[2][1] = 2.0f * (qyz - qwx);
   result[2][2] = 1.0f - 2.0f * (qxx + qyy);

   return result;
}

// This function is based on the glm::quat_cast function
Q::quat Q::mat3ToQuat(const glm::mat3& m)
{
   float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
   float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
   float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
   float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

   int biggestIndex = 0;
   float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
   if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
   {
      fourBiggestSquaredMinus1 = fourXSquaredMinus1;
      biggestIndex = 1;
   }
   if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
   {
      fourBiggestSquaredMinus1 = fourYSquaredMinus1;
      biggestIndex = 2;
   }
   if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
   {
      fourBiggestSquaredMinus1 = fourZSquaredMinus1;
      biggestIndex = 3;
   }

   float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
   float mult = 0.25f / biggestVal;

   switch (biggestIndex)
   {
   case 0:
      return Q::quat((m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult, biggestVal);
   case 1:
      return Q::quat(biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] - m[2][1]) * mult);
   case 2:
      return Q::quat((m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult, (m[2][0] - m[0][2]) * mult);
   case 3:
      return Q::quat((m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal, (m[0][1] - m[1][0]) * mult);
   default:
      assert(false);
      return Q::quat(0, 0, 0, 1);
   }
}

glm::mat4 Q::quatToMat4(const Q::quat& q)
{
   // Rotate the world's basis vectors using the quaternion
   glm::vec3 right = q * glm::vec3(1, 0, 0);
   glm::vec3 up    = q * glm::vec3(0, 1, 0);
   glm::vec3 fwd   = q * glm::vec3(0, 0, 1);

   // Compose a rotation matrix using the rotated basis vectors
   return glm::mat4(right.x, right.y, right.z, 0,  // Column 0 (X)
                    up.x,    up.y,    up.z,    0,  // Column 1 (Y)
                    fwd.x,   fwd.y,   fwd.z,   0,  // Column 2 (Z)
                    0,       0,       0,       1); // Column 3
}

Q::quat Q::mat4ToQuat(const glm::mat4& m)
{
   // Get the rotated basis vectors from the matrix
   glm::vec3 up      = normalizeWithZeroLengthCheck(glm::vec3(m[1].x, m[1].y, m[1].z));
   glm::vec3 forward = normalizeWithZeroLengthCheck(glm::vec3(m[2].x, m[2].y, m[2].z));
   glm::vec3 right   = glm::cross(up, forward);
   up = glm::cross(forward, right);

   // Use the lookRotation function to compose a quaternion that rotates from the world's basis vectors to the rotated basis vectors
   return Q::lookRotation(forward, up);
}

// TODO: This function shouldn't be declared in quat.h
// This helper function is identical to glm::normalize, except that it checks if the length of the vector
// we want to normalize is zero, and if it is then it doesn't normalize it
glm::vec3 normalizeWithZeroLengthCheck(const glm::vec3& v)
{
   float squaredLen = glm::length2(v);
   if (squaredLen < QUAT_EPSILON)
   {
      return v;
   }

   return v / glm::sqrt(squaredLen);
}

void testQuaternion()
{
   std::printf("=== Quaternion Test Suite ===\n\n");
   
   // Test constructors
   std::printf("-- Constructors --\n");
   Q::quat q1;
   std::printf("Default constructor: q1 = (%.6f, %.6f, %.6f, %.6f)\n", q1.x, q1.y, q1.z, q1.w);
   
   Q::quat q2(0.5f, 0.5f, 0.5f, 0.5f);
   std::printf("Parameterized constructor: q2 = (%.6f, %.6f, %.6f, %.6f)\n", q2.x, q2.y, q2.z, q2.w);
   
   // Test angleAxis
   std::printf("\n-- angleAxis --\n");
   Q::quat q3 = Q::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
   std::printf("90 deg rotation around Y: q3 = (%.6f, %.6f, %.6f, %.6f)\n", q3.x, q3.y, q3.z, q3.w);
   
   Q::quat q4 = Q::angleAxis(glm::radians(45.0f), glm::vec3(1, 0, 0));
   std::printf("45 deg rotation around X: q4 = (%.6f, %.6f, %.6f, %.6f)\n", q4.x, q4.y, q4.z, q4.w);
   
   // Test getAngle and getAxis
   std::printf("\n-- getAngle and getAxis --\n");
   float angle3 = Q::getAngle(q3);
   glm::vec3 axis3 = Q::getAxis(q3);
   std::printf("Angle from q3: %.6f radians (%.6f degrees)\n", angle3, glm::degrees(angle3));
   std::printf("Axis from q3: (%.6f, %.6f, %.6f)\n", axis3.x, axis3.y, axis3.z);
   
   // Test fromTo
   std::printf("\n-- fromTo --\n");
   glm::vec3 from1(1, 0, 0);
   glm::vec3 to1(0, 1, 0);
   Q::quat q5 = Q::fromTo(from1, to1);
   std::printf("fromTo X to Y: q5 = (%.6f, %.6f, %.6f, %.6f)\n", q5.x, q5.y, q5.z, q5.w);
   
   glm::vec3 from2(1, 0, 0);
   glm::vec3 to2(-1, 0, 0);
   Q::quat q6 = Q::fromTo(from2, to2);
   std::printf("fromTo X to -X: q6 = (%.6f, %.6f, %.6f, %.6f)\n", q6.x, q6.y, q6.z, q6.w);
   
   // Test addition and subtraction
   std::printf("\n-- Addition and Subtraction --\n");
   Q::quat q7 = q2 + q3;
   std::printf("q2 + q3 = (%.6f, %.6f, %.6f, %.6f)\n", q7.x, q7.y, q7.z, q7.w);
   
   Q::quat q8 = q3 - q2;
   std::printf("q3 - q2 = (%.6f, %.6f, %.6f, %.6f)\n", q8.x, q8.y, q8.z, q8.w);
   
   Q::quat q9 = -q3;
   std::printf("-q3 = (%.6f, %.6f, %.6f, %.6f)\n", q9.x, q9.y, q9.z, q9.w);
   
   // Test scalar multiplication
   std::printf("\n-- Scalar Multiplication --\n");
   Q::quat q10 = q3 * 2.0f;
   std::printf("q3 * 2.0 = (%.6f, %.6f, %.6f, %.6f)\n", q10.x, q10.y, q10.z, q10.w);
   
   Q::quat q11 = 0.5f * q3;
   std::printf("0.5 * q3 = (%.6f, %.6f, %.6f, %.6f)\n", q11.x, q11.y, q11.z, q11.w);
   
   // Test quaternion multiplication
   std::printf("\n-- Quaternion Multiplication --\n");
   Q::quat q12 = q3 * q4;
   std::printf("q3 * q4 = (%.6f, %.6f, %.6f, %.6f)\n", q12.x, q12.y, q12.z, q12.w);
   
   // Test vector rotation
   std::printf("\n-- Vector Rotation --\n");
   glm::vec3 v1(1, 0, 0);
   glm::vec3 v2 = q3 * v1;
   std::printf("q3 * (1,0,0) = (%.6f, %.6f, %.6f)\n", v2.x, v2.y, v2.z);
   
   glm::vec3 v3(0, 0, 1);
   glm::vec3 v4 = q4 * v3;
   std::printf("q4 * (0,0,1) = (%.6f, %.6f, %.6f)\n", v4.x, v4.y, v4.z);
   
   // Test power operator
   std::printf("\n-- Power Operator --\n");
   Q::quat q13 = q3 ^ 0.5f;
   std::printf("q3 ^ 0.5 = (%.6f, %.6f, %.6f, %.6f)\n", q13.x, q13.y, q13.z, q13.w);
   
   Q::quat q14 = q3 ^ 2.0f;
   std::printf("q3 ^ 2.0 = (%.6f, %.6f, %.6f, %.6f)\n", q14.x, q14.y, q14.z, q14.w);
   
   // Test equality operators
   std::printf("\n-- Equality Operators --\n");
   Q::quat q15 = q3;
   std::printf("q3 == q15: %s\n", (q3 == q15) ? "true" : "false");
   std::printf("q3 != q4: %s\n", (q3 != q4) ? "true" : "false");
   
   // Test sameOrientation
   std::printf("\n-- sameOrientation --\n");
   Q::quat q16 = -q3;
   std::printf("sameOrientation(q3, q16): %s\n", Q::sameOrientation(q3, q16) ? "true" : "false");
   std::printf("sameOrientation(q3, q4): %s\n", Q::sameOrientation(q3, q4) ? "true" : "false");
   
   // Test dot product
   std::printf("\n-- Dot Product --\n");
   float dot1 = Q::dot(q3, q4);
   std::printf("dot(q3, q4) = %.6f\n", dot1);
   
   float dot2 = Q::dot(q3, q3);
   std::printf("dot(q3, q3) = %.6f\n", dot2);
   
   // Test length functions
   std::printf("\n-- Length Functions --\n");
   float sqLen = Q::squaredLength(q2);
   std::printf("squaredLength(q2) = %.6f\n", sqLen);
   
   float len = Q::length(q2);
   std::printf("length(q2) = %.6f\n", len);
   
   // Test normalization
   std::printf("\n-- Normalization --\n");
   Q::quat q17 = q2;
   Q::normalize(q17);
   std::printf("normalize(q2) = (%.6f, %.6f, %.6f, %.6f)\n", q17.x, q17.y, q17.z, q17.w);
   std::printf("length after normalize = %.6f\n", Q::length(q17));
   
   Q::quat q18 = Q::normalized(q2);
   std::printf("normalized(q2) = (%.6f, %.6f, %.6f, %.6f)\n", q18.x, q18.y, q18.z, q18.w);
   
   // Test conjugate and inverse
   std::printf("\n-- Conjugate and Inverse --\n");
   Q::quat q19 = Q::conjugate(q3);
   std::printf("conjugate(q3) = (%.6f, %.6f, %.6f, %.6f)\n", q19.x, q19.y, q19.z, q19.w);
   
   Q::quat q20 = Q::inverse(q3);
   std::printf("inverse(q3) = (%.6f, %.6f, %.6f, %.6f)\n", q20.x, q20.y, q20.z, q20.w);
   
   // Verify that q * q^-1 = identity
   Q::quat q21 = q3 * q20;
   std::printf("q3 * inverse(q3) = (%.6f, %.6f, %.6f, %.6f)\n", q21.x, q21.y, q21.z, q21.w);
   
   // Test interpolation functions
   std::printf("\n-- Interpolation Functions --\n");
   Q::quat start = Q::angleAxis(0, glm::vec3(0, 1, 0));
   Q::quat end = Q::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
   
   Q::quat q22 = Q::mix(start, end, 0.5f);
   std::printf("mix(start, end, 0.5) = (%.6f, %.6f, %.6f, %.6f)\n", q22.x, q22.y, q22.z, q22.w);
   
   Q::quat q23 = Q::nlerp(start, end, 0.5f);
   std::printf("nlerp(start, end, 0.5) = (%.6f, %.6f, %.6f, %.6f)\n", q23.x, q23.y, q23.z, q23.w);
   
   Q::quat q24 = Q::slerp(start, end, 0.5f);
   std::printf("slerp(start, end, 0.5) = (%.6f, %.6f, %.6f, %.6f)\n", q24.x, q24.y, q24.z, q24.w);
   
   // Test lookRotation
   std::printf("\n-- lookRotation --\n");
   glm::vec3 direction(1, 0, 1);
   glm::vec3 up(0, 1, 0);
   Q::quat q25 = Q::lookRotation(direction, up);
   std::printf("lookRotation((1,0,1), (0,1,0)) = (%.6f, %.6f, %.6f, %.6f)\n", q25.x, q25.y, q25.z, q25.w);
   
   // Test matrix conversions
   std::printf("\n-- Matrix Conversions --\n");
   glm::mat3 mat3 = Q::quatToMat3(q3);
   std::printf("quatToMat3(q3):\n");
   std::printf("  [%.6f, %.6f, %.6f]\n", mat3[0][0], mat3[0][1], mat3[0][2]);
   std::printf("  [%.6f, %.6f, %.6f]\n", mat3[1][0], mat3[1][1], mat3[1][2]);
   std::printf("  [%.6f, %.6f, %.6f]\n", mat3[2][0], mat3[2][1], mat3[2][2]);
   
   Q::quat q26 = Q::mat3ToQuat(mat3);
   std::printf("mat3ToQuat(mat3) = (%.6f, %.6f, %.6f, %.6f)\n", q26.x, q26.y, q26.z, q26.w);
   
   glm::mat4 mat4 = Q::quatToMat4(q3);
   std::printf("\nquatToMat4(q3):\n");
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat4[0][0], mat4[0][1], mat4[0][2], mat4[0][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat4[1][0], mat4[1][1], mat4[1][2], mat4[1][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat4[2][0], mat4[2][1], mat4[2][2], mat4[2][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat4[3][0], mat4[3][1], mat4[3][2], mat4[3][3]);
   
   Q::quat q27 = Q::mat4ToQuat(mat4);
   std::printf("mat4ToQuat(mat4) = (%.6f, %.6f, %.6f, %.6f)\n", q27.x, q27.y, q27.z, q27.w);
   
   // Test edge cases
   std::printf("\n-- Edge Cases --\n");
   Q::quat zeroQuat(0, 0, 0, 0);
   std::printf("Zero quaternion: (%.6f, %.6f, %.6f, %.6f)\n", zeroQuat.x, zeroQuat.y, zeroQuat.z, zeroQuat.w);
   std::printf("length(zeroQuat) = %.6f\n", Q::length(zeroQuat));
   std::printf("normalized(zeroQuat) = (%.6f, %.6f, %.6f, %.6f)\n", 
           Q::normalized(zeroQuat).x, Q::normalized(zeroQuat).y, 
           Q::normalized(zeroQuat).z, Q::normalized(zeroQuat).w);
   
   std::printf("\n=== End of Quaternion Test Suite ===\n");
}
