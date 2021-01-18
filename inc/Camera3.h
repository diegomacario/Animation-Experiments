#ifndef CAMERA3_H
#define CAMERA3_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "quat.h"

class Camera3
{
public:

   Camera3(float            distanceBetweenPlayerAndCamera,
           float            cameraPitch,
           float            fieldOfViewYInDeg,
           const glm::vec3& playerPosition,
           const Q::quat&   playerOrientation,
           float            aspectRatio,
           float            near,
           float            far,
           float            movementSpeed,
           float            mouseSensitivity);
   ~Camera3() = default;

   Camera3(const Camera3&) = default;
   Camera3& operator=(const Camera3&) = default;

   Camera3(Camera3&& rhs) noexcept;
   Camera3& operator=(Camera3&& rhs) noexcept;

   glm::vec3 getPosition();

   glm::mat4 getViewMatrix();
   glm::mat4 getPerspectiveProjectionMatrix();
   glm::mat4 getPerspectiveProjectionViewMatrix();

   void      reposition(float            distanceBetweenPlayerAndCamera,
                        float            cameraPitch,
                        float            fieldOfViewYInDeg,
                        const glm::vec3& playerPosition,
                        const Q::quat&   playerOrientation);

   enum class MovementDirection
   {
      Forward,
      Backward,
      Left,
      Right
   };

   void      processMouseMovement(float xOffset, float yOffset);
   void      processScrollWheelMovement(float yOffset);
   void      processPlayerMovement(const glm::vec3& playerPosition, const Q::quat& playerOrientation);

   bool      isFree() const;
   void      setFree(bool free);

private:

   glm::vec3 mPosition;
   Q::quat   mLocalOrientation;
   Q::quat   mGlobalOrientation;

   float     mDistanceBetweenPlayerAndCamera;
   float     mCameraPitch;

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
