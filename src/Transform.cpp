#include <glm/gtx/compatibility.hpp>
#include <cstdio>

#include "Transform.h"

bool operator==(const Transform& a, const Transform& b)
{
   return (a.position == b.position) &&
          (a.rotation == b.rotation) &&
          (a.scale == b.scale);
}

bool operator!=(const Transform& a, const Transform& b)
{
   return !(a == b);
}

Transform combine(const Transform& parent, const Transform& child)
{
   Transform result;

   result.scale = parent.scale * child.scale;

   // NOTE: Reversed because q * p is implemented as p * q
   result.rotation = child.rotation * parent.rotation;

   // Bring the child's position into the parent's space before adding their positions
   // First scale, then rotate and finally translate
   result.position = parent.rotation * (parent.scale * child.position);
   result.position = parent.position + result.position;

   return result;
}

Transform inverse(const Transform& t)
{
   Transform result;

   result.scale.x = glm::abs(t.scale.x) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.x);
   result.scale.y = glm::abs(t.scale.y) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.y);
   result.scale.z = glm::abs(t.scale.z) < TRANSFORM_EPSILON ? 0.0f : (1.0f / t.scale.z);

   result.rotation = Q::inverse(t.rotation);

   // The inverse position needs to be scaled and rotated to bring it into the new space
   glm::vec3 inversePos = -t.position;
   result.position = result.rotation * (result.scale * inversePos);

   return result;
}

Transform mix(const Transform& a, const Transform& b, float t)
{
   // Quaternion neighborhood check
   Q::quat bRotation = b.rotation;
   if (Q::dot(a.rotation, b.rotation) < 0.0f)
   {
      bRotation = -bRotation;
   }

   return Transform(glm::lerp(a.position, b.position, t),
                    Q::nlerp(a.rotation, bRotation, t),
                    glm::lerp(a.scale, b.scale, t));
}

glm::mat4 transformToMat4(const Transform& t)
{
   // Calculate the rotation basis of the transform
   glm::vec3 right = t.rotation * glm::vec3(1, 0, 0);
   glm::vec3 up    = t.rotation * glm::vec3(0, 1, 0);
   glm::vec3 fwd   = t.rotation * glm::vec3(0, 0, 1);

   // Scale the rotation basis
   right *= t.scale.x;
   up    *= t.scale.y;
   fwd   *= t.scale.z;

   // Compose the transformation matrix
   return glm::mat4(right.x,      right.y,      right.z,      0,  // Scaled X basis
                    up.x,         up.y,         up.z,         0,  // Scaled Y basis
                    fwd.x,        fwd.y,        fwd.z,        0,  // Scaled Z basis
                    t.position.x, t.position.y, t.position.z, 1); // Position
}

Transform mat4ToTransform(const glm::mat4& m)
{
   Transform result;

   // Extract the position from the matrix
   result.position = glm::vec3(m[3][0], m[3][1], m[3][2]);

   // Extract the rotation from the matrix
   // We can do this even if the matrix contains scale information
   // by normalizing the rotation bases, which is what Q::mat4ToQuat does
   result.rotation = Q::mat4ToQuat(m);

   // This matrix contains the scale and rotation information
   glm::mat4 scaleAndRotMat(m[0][0], m[0][1], m[0][2], 0, // Scaled X basis
                            m[1][0], m[1][1], m[1][2], 0, // Scaled Y basis
                            m[2][0], m[2][1], m[2][2], 0, // Scaled Z basis
                            0,       0,       0,       1);

   // This matrix only contains the inverse of the rotation information
   glm::mat4 inverseRotMat = Q::quatToMat4(Q::inverse(result.rotation));

   // By multiplying the previous two matrices we end up with a matrix that contains scale and skew information
   glm::mat4 scaleAndSkewMat = scaleAndRotMat * inverseRotMat;

   // To extract the scale information from the scale and skew matrix, we will simply take its diagonal
   // This isn't perfect though, so the scale we are extracting here should be considered to be a "lossy" scale
   // It's possible to get an accurate scale using matrix decomposition, but that's an expensive operation
   result.scale = glm::vec3(scaleAndSkewMat[0][0], scaleAndSkewMat[1][1], scaleAndSkewMat[2][2]);

   return result;
}

glm::vec3 transformPoint(const Transform& t, const glm::vec3& p)
{
   // First scale, then rotate and finally translate
   return t.position + (t.rotation * (t.scale * p));
}

glm::vec3 transformVector(const Transform& t, const glm::vec3& v)
{
   // First scale, then rotate
   // We don't translate because vectors don't have a position
   // Only a magnitude and a direction
   return t.rotation * (t.scale * v);
}

void testTransform()
{
   std::printf("=== Transform Test Suite ===\n\n");
   
   // Test constructors
   std::printf("-- Constructors --\n");
   Transform t1;
   std::printf("Default constructor:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t1.position.x, t1.position.y, t1.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t1.rotation.x, t1.rotation.y, t1.rotation.z, t1.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t1.scale.x, t1.scale.y, t1.scale.z);
   
   glm::vec3 pos(2.0f, 3.0f, 4.0f);
   Q::quat rot = Q::angleAxis(glm::radians(45.0f), glm::vec3(0, 1, 0));
   glm::vec3 scl(1.5f, 2.0f, 0.5f);
   Transform t2(pos, rot, scl);
   std::printf("\nParameterized constructor:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t2.position.x, t2.position.y, t2.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t2.rotation.x, t2.rotation.y, t2.rotation.z, t2.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t2.scale.x, t2.scale.y, t2.scale.z);
   
   // Test equality operators
   std::printf("\n-- Equality Operators --\n");
   Transform t3 = t2;
   std::printf("t2 == t3: %s\n", (t2 == t3) ? "true" : "false");
   std::printf("t1 != t2: %s\n", (t1 != t2) ? "true" : "false");
   
   // Test combine
   std::printf("\n-- Combine --\n");
   Transform parent;
   parent.position = glm::vec3(10.0f, 0.0f, 0.0f);
   parent.rotation = Q::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
   parent.scale = glm::vec3(2.0f, 2.0f, 2.0f);
   
   Transform child;
   child.position = glm::vec3(5.0f, 0.0f, 0.0f);
   child.rotation = Q::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
   child.scale = glm::vec3(0.5f, 0.5f, 0.5f);
   
   Transform combined = combine(parent, child);
   std::printf("Parent transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", parent.position.x, parent.position.y, parent.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", parent.rotation.x, parent.rotation.y, parent.rotation.z, parent.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", parent.scale.x, parent.scale.y, parent.scale.z);
   
   std::printf("Child transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", child.position.x, child.position.y, child.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", child.rotation.x, child.rotation.y, child.rotation.z, child.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", child.scale.x, child.scale.y, child.scale.z);
   
   std::printf("Combined transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", combined.position.x, combined.position.y, combined.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", combined.rotation.x, combined.rotation.y, combined.rotation.z, combined.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", combined.scale.x, combined.scale.y, combined.scale.z);
   
   // Test inverse
   std::printf("\n-- Inverse --\n");
   Transform t4;
   t4.position = glm::vec3(5.0f, 10.0f, 15.0f);
   t4.rotation = Q::angleAxis(glm::radians(30.0f), glm::vec3(1, 0, 0));
   t4.scale = glm::vec3(2.0f, 4.0f, 0.5f);
   
   Transform t4_inv = inverse(t4);
   std::printf("Original transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t4.position.x, t4.position.y, t4.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t4.rotation.x, t4.rotation.y, t4.rotation.z, t4.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t4.scale.x, t4.scale.y, t4.scale.z);
   
   std::printf("Inverse transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t4_inv.position.x, t4_inv.position.y, t4_inv.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t4_inv.rotation.x, t4_inv.rotation.y, t4_inv.rotation.z, t4_inv.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t4_inv.scale.x, t4_inv.scale.y, t4_inv.scale.z);
   
   // Verify that t * t^-1 = identity
   Transform identity_check = combine(t4, t4_inv);
   std::printf("t4 * inverse(t4):\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", identity_check.position.x, identity_check.position.y, identity_check.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", identity_check.rotation.x, identity_check.rotation.y, identity_check.rotation.z, identity_check.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", identity_check.scale.x, identity_check.scale.y, identity_check.scale.z);
   
   // Test mix
   std::printf("\n-- Mix (Interpolation) --\n");
   Transform start;
   start.position = glm::vec3(0.0f, 0.0f, 0.0f);
   start.rotation = Q::angleAxis(0.0f, glm::vec3(0, 1, 0));
   start.scale = glm::vec3(1.0f, 1.0f, 1.0f);
   
   Transform end;
   end.position = glm::vec3(10.0f, 5.0f, 0.0f);
   end.rotation = Q::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));
   end.scale = glm::vec3(2.0f, 2.0f, 2.0f);
   
   Transform mix25 = mix(start, end, 0.25f);
   Transform mix50 = mix(start, end, 0.5f);
   Transform mix75 = mix(start, end, 0.75f);
   
   std::printf("Start transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", start.position.x, start.position.y, start.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", start.rotation.x, start.rotation.y, start.rotation.z, start.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", start.scale.x, start.scale.y, start.scale.z);
   
   std::printf("End transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", end.position.x, end.position.y, end.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", end.rotation.x, end.rotation.y, end.rotation.z, end.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", end.scale.x, end.scale.y, end.scale.z);
   
   std::printf("Mix at t=0.25:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", mix25.position.x, mix25.position.y, mix25.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", mix25.rotation.x, mix25.rotation.y, mix25.rotation.z, mix25.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", mix25.scale.x, mix25.scale.y, mix25.scale.z);
   
   std::printf("Mix at t=0.5:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", mix50.position.x, mix50.position.y, mix50.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", mix50.rotation.x, mix50.rotation.y, mix50.rotation.z, mix50.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", mix50.scale.x, mix50.scale.y, mix50.scale.z);
   
   std::printf("Mix at t=0.75:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", mix75.position.x, mix75.position.y, mix75.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", mix75.rotation.x, mix75.rotation.y, mix75.rotation.z, mix75.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", mix75.scale.x, mix75.scale.y, mix75.scale.z);
   
   // Test matrix conversions
   std::printf("\n-- Matrix Conversions --\n");
   Transform t5;
   t5.position = glm::vec3(3.0f, 4.0f, 5.0f);
   t5.rotation = Q::angleAxis(glm::radians(60.0f), glm::vec3(1, 1, 0));
   t5.scale = glm::vec3(2.0f, 3.0f, 1.0f);
   
   glm::mat4 mat = transformToMat4(t5);
   std::printf("Original transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t5.position.x, t5.position.y, t5.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t5.rotation.x, t5.rotation.y, t5.rotation.z, t5.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t5.scale.x, t5.scale.y, t5.scale.z);
   
   std::printf("transformToMat4:\n");
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
   std::printf("  [%.6f, %.6f, %.6f, %.6f]\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
   
   Transform t6 = mat4ToTransform(mat);
   std::printf("mat4ToTransform (should match original):\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t6.position.x, t6.position.y, t6.position.z);
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", t6.rotation.x, t6.rotation.y, t6.rotation.z, t6.rotation.w);
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t6.scale.x, t6.scale.y, t6.scale.z);
   
   // Test transformPoint
   std::printf("\n-- transformPoint --\n");
   Transform t7;
   t7.position = glm::vec3(10.0f, 0.0f, 0.0f);
   t7.rotation = Q::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
   t7.scale = glm::vec3(2.0f, 2.0f, 2.0f);
   
   glm::vec3 point1(1.0f, 0.0f, 0.0f);
   glm::vec3 point2(0.0f, 1.0f, 0.0f);
   glm::vec3 point3(0.0f, 0.0f, 1.0f);
   
   glm::vec3 transformed1 = transformPoint(t7, point1);
   glm::vec3 transformed2 = transformPoint(t7, point2);
   glm::vec3 transformed3 = transformPoint(t7, point3);
   
   std::printf("Transform:\n");
   std::printf("  position: (%.6f, %.6f, %.6f)\n", t7.position.x, t7.position.y, t7.position.z);
   std::printf("  rotation: 90 deg around Y\n");
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t7.scale.x, t7.scale.y, t7.scale.z);
   
   std::printf("transformPoint((1,0,0)) = (%.6f, %.6f, %.6f)\n", transformed1.x, transformed1.y, transformed1.z);
   std::printf("transformPoint((0,1,0)) = (%.6f, %.6f, %.6f)\n", transformed2.x, transformed2.y, transformed2.z);
   std::printf("transformPoint((0,0,1)) = (%.6f, %.6f, %.6f)\n", transformed3.x, transformed3.y, transformed3.z);
   
   // Test transformVector
   std::printf("\n-- transformVector --\n");
   glm::vec3 vec1(1.0f, 0.0f, 0.0f);
   glm::vec3 vec2(0.0f, 1.0f, 0.0f);
   glm::vec3 vec3(0.0f, 0.0f, 1.0f);
   
   glm::vec3 transformedVec1 = transformVector(t7, vec1);
   glm::vec3 transformedVec2 = transformVector(t7, vec2);
   glm::vec3 transformedVec3 = transformVector(t7, vec3);
   
   std::printf("transformVector((1,0,0)) = (%.6f, %.6f, %.6f)\n", transformedVec1.x, transformedVec1.y, transformedVec1.z);
   std::printf("transformVector((0,1,0)) = (%.6f, %.6f, %.6f)\n", transformedVec2.x, transformedVec2.y, transformedVec2.z);
   std::printf("transformVector((0,0,1)) = (%.6f, %.6f, %.6f)\n", transformedVec3.x, transformedVec3.y, transformedVec3.z);
   
   // Test edge cases
   std::printf("\n-- Edge Cases --\n");
   Transform t8;
   t8.position = glm::vec3(0.0f, 0.0f, 0.0f);
   t8.rotation = Q::quat();
   t8.scale = glm::vec3(0.0f, 1.0f, 2.0f); // One component is zero
   
   Transform t8_inv = inverse(t8);
   std::printf("Transform with zero scale component:\n");
   std::printf("  scale: (%.6f, %.6f, %.6f)\n", t8.scale.x, t8.scale.y, t8.scale.z);
   std::printf("Inverse scale: (%.6f, %.6f, %.6f)\n", t8_inv.scale.x, t8_inv.scale.y, t8_inv.scale.z);
   
   // Test quaternion neighborhood in mix
   std::printf("\n-- Quaternion Neighborhood in Mix --\n");
   Transform qa, qb;
   qa.rotation = Q::angleAxis(glm::radians(10.0f), glm::vec3(0, 1, 0));
   qb.rotation = Q::angleAxis(glm::radians(350.0f), glm::vec3(0, 1, 0)); // Should be treated as -10 degrees
   qa.position = qa.scale = glm::vec3(0.0f, 0.0f, 0.0f);
   qb.position = qb.scale = glm::vec3(1.0f, 1.0f, 1.0f);
   
   Transform mixed = mix(qa, qb, 0.5f);
   std::printf("Mixing 10 deg and 350 deg rotations at t=0.5:\n");
   std::printf("  rotation: (%.6f, %.6f, %.6f, %.6f)\n", mixed.rotation.x, mixed.rotation.y, mixed.rotation.z, mixed.rotation.w);
   float angle = Q::getAngle(mixed.rotation);
   std::printf("  angle: %.6f radians (%.6f degrees)\n", angle, glm::degrees(angle));
   
   std::printf("\n=== End of Transform Test Suite ===\n");
}
