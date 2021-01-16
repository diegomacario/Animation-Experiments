#ifndef CAMERA3_H
#define CAMERA3_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "quat.h"

class Camera3
{
public:

   Camera3(float     distanceBetweenPlayerAndCamera,
           float     cameraPitch,
           float     cameraYaw,
           float     fieldOfViewYInDeg,
           float     aspectRatio,
           float     near,
           float     far,
           float     movementSpeed,
           float     mouseSensitivity);
   ~Camera3() = default;

   Camera3(const Camera3&) = default;
   Camera3& operator=(const Camera3&) = default;

   Camera3(Camera3&& rhs) noexcept;
   Camera3& operator=(Camera3&& rhs) noexcept;

   glm::vec3 getPosition(glm::vec3 playerPosition, Q::quat playerOrientation);

   glm::mat4 getViewMatrix(glm::vec3 playerPosition, Q::quat playerOrientation);
   glm::mat4 getPerspectiveProjectionMatrix();
   glm::mat4 getPerspectiveProjectionViewMatrix(glm::vec3 playerPosition, Q::quat playerOrientation);

   void      reposition(float     distanceBetweenPlayerAndCamera,
                        float     cameraPitch,
                        float     cameraYaw,
                        float     fieldOfViewYInDeg);

   enum class MovementDirection
   {
      Forward,
      Backward,
      Left,
      Right
   };

   void      processMouseMovement(float xOffset, float yOffset);
   void      processScrollWheelMovement(float yOffset);

   bool      isFree() const;
   void      setFree(bool free);

private:

   float     mDistanceBetweenPlayerAndCamera;
   float     mCameraPitch;
   float     mCameraYaw;

   glm::vec3 mPosition;
   Q::quat   mOrientation;

   float     mFieldOfViewYInDeg;
   float     mAspectRatio;
   float     mNear;
   float     mFar;

   float     mMouseSensitivity;

   bool      mIsFree;

   glm::mat4 mViewMatrix;
   glm::mat4 mPerspectiveProjectionMatrix;
   glm::mat4 mPerspectiveProjectionViewMatrix;

   bool      mNeedToUpdateViewMatrix;
   bool      mNeedToUpdatePerspectiveProjectionMatrix;
   bool      mNeedToUpdatePerspectiveProjectionViewMatrix;
};

#endif
