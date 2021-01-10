#include <utility>
#include <iostream>

#include "camera.h"

Camera::Camera(glm::vec3 position,
               glm::vec3 target,
               glm::vec3 worldUp,
               float     fieldOfViewYInDeg,
               float     aspectRatio,
               float     near,
               float     far,
               float     movementSpeed,
               float     mouseSensitivity)
   : mPosition(position)
   , mWorldUp(worldUp)
   , mYawInDeg()
   , mPitchInDeg()
   , mFieldOfViewYInDeg(fieldOfViewYInDeg)
   , mAspectRatio(aspectRatio)
   , mNear(near)
   , mFar(far)
   , mMovementSpeed(movementSpeed)
   , mMouseSensitivity(mouseSensitivity)
   , mIsFree(false)
   , mViewMatrix()
   , mPerspectiveProjectionMatrix()
   , mPerspectiveProjectionViewMatrix()
   , mNeedToUpdateViewMatrix(true)
   , mNeedToUpdatePerspectiveProjectionMatrix(true)
   , mNeedToUpdatePerspectiveProjectionViewMatrix(true)
{
   mCameraZ = glm::normalize(position - target);
   mCameraX = glm::normalize(glm::cross(mWorldUp, mCameraZ));
   mCameraY = glm::normalize(glm::cross(mCameraZ, mCameraX));

   mOrientation = Q::lookRotation(mCameraZ, mCameraY);
}

Camera::Camera(Camera&& rhs) noexcept
   : mPosition(std::exchange(rhs.mPosition, glm::vec3(0.0f)))
   , mWorldUp(std::exchange(rhs.mWorldUp, glm::vec3(0.0f)))
   , mYawInDeg(std::exchange(rhs.mYawInDeg, 0.0f))
   , mPitchInDeg(std::exchange(rhs.mPitchInDeg, 0.0f))
   , mFieldOfViewYInDeg(std::exchange(rhs.mFieldOfViewYInDeg, 0.0f))
   , mAspectRatio(std::exchange(rhs.mAspectRatio, 0.0f))
   , mNear(std::exchange(rhs.mNear, 0.0f))
   , mFar(std::exchange(rhs.mFar, 0.0f))
   , mMovementSpeed(std::exchange(rhs.mMovementSpeed, 0.0f))
   , mMouseSensitivity(std::exchange(rhs.mMouseSensitivity, 0.0f))
   , mIsFree(std::exchange(rhs.mIsFree, false))
   , mViewMatrix(std::exchange(rhs.mViewMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionMatrix(std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionViewMatrix(std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f)))
   , mNeedToUpdateViewMatrix(std::exchange(rhs.mNeedToUpdateViewMatrix, true))
   , mNeedToUpdatePerspectiveProjectionMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true))
   , mNeedToUpdatePerspectiveProjectionViewMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true))
{

}

Camera& Camera::operator=(Camera&& rhs) noexcept
{
   mPosition                                    = std::exchange(rhs.mPosition, glm::vec3(0.0f));
   mWorldUp                                     = std::exchange(rhs.mWorldUp, glm::vec3(0.0f));
   mYawInDeg                                    = std::exchange(rhs.mYawInDeg, 0.0f);
   mPitchInDeg                                  = std::exchange(rhs.mPitchInDeg, 0.0f);
   mFieldOfViewYInDeg                           = std::exchange(rhs.mFieldOfViewYInDeg, 0.0f);
   mAspectRatio                                 = std::exchange(rhs.mAspectRatio, 0.0f);
   mNear                                        = std::exchange(rhs.mNear, 0.0f);
   mFar                                         = std::exchange(rhs.mFar, 0.0f);
   mMovementSpeed                               = std::exchange(rhs.mMovementSpeed, 0.0f);
   mMouseSensitivity                            = std::exchange(rhs.mMouseSensitivity, 0.0f);
   mIsFree                                      = std::exchange(rhs.mIsFree, false);
   mViewMatrix                                  = std::exchange(rhs.mViewMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionMatrix                 = std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionViewMatrix             = std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f));
   mNeedToUpdateViewMatrix                      = std::exchange(rhs.mNeedToUpdateViewMatrix, true);
   mNeedToUpdatePerspectiveProjectionMatrix     = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true);
   mNeedToUpdatePerspectiveProjectionViewMatrix = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true);
   return *this;
}

glm::vec3 Camera::getPosition()
{
   return mPosition;
}

glm::mat4 Camera::getViewMatrix()
{
   if (mNeedToUpdateViewMatrix)
   {
      Q::quat   reverseOrient = Q::conjugate(mOrientation);
      glm::mat4 rot           = Q::quatToMat4(reverseOrient);
      glm::mat4 translation   = glm::translate(glm::mat4(1.0), -mPosition);

      mViewMatrix = rot * translation;

      mNeedToUpdateViewMatrix = false;
   }

   return mViewMatrix;
}

glm::mat4 Camera::getPerspectiveProjectionMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionMatrix)
   {
      mPerspectiveProjectionMatrix = glm::perspective(glm::radians(mFieldOfViewYInDeg),
                                                      mAspectRatio,
                                                      mNear,
                                                      mFar);
      mNeedToUpdatePerspectiveProjectionMatrix = false;
   }

   return mPerspectiveProjectionMatrix;
}

glm::mat4 Camera::getPerspectiveProjectionViewMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionViewMatrix)
   {
      mPerspectiveProjectionViewMatrix = getPerspectiveProjectionMatrix() * getViewMatrix();
      mNeedToUpdatePerspectiveProjectionViewMatrix = false;
   }

   return mPerspectiveProjectionViewMatrix;
}

void Camera::reposition(const glm::vec3& position,
                        const glm::vec3& target,
                        const glm::vec3& worldUp,
                        float            fieldOfViewYInDeg)
{
   mPosition          = position;
   mWorldUp           = worldUp;
   mFieldOfViewYInDeg = fieldOfViewYInDeg;

   mCameraZ = glm::normalize(position - target);

   mOrientation = Q::lookRotation(mCameraZ, mWorldUp);

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processKeyboardInput(MovementDirection direction, float deltaTime)
{
   float velocity = mMovementSpeed * deltaTime;

   glm::vec3 front = mOrientation * glm::vec3(0, 0, -1);
   glm::vec3 right = glm::normalize(glm::cross(front, mWorldUp));

   switch (direction)
   {
   case MovementDirection::Forward:
      mPosition += front * velocity;
      break;
   case MovementDirection::Backward:
      mPosition -= front * velocity;
      break;
   case MovementDirection::Left:
      mPosition -= right * velocity;
      break;
   case MovementDirection::Right:
      mPosition += right * velocity;
      break;
   }

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
   xOffset *= mMouseSensitivity;
   yOffset *= mMouseSensitivity;

   mYawInDeg   = -xOffset;
   mPitchInDeg = -yOffset;

   updateCoordinateFrame();

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processScrollWheelMovement(float yOffset)
{
   // The larger the FOV, the smaller things appear on the screen
   // The smaller the FOV, the larger things appear on the screen
   if (mFieldOfViewYInDeg >= 1.0f && mFieldOfViewYInDeg <= 45.0f)
   {
      mFieldOfViewYInDeg -= yOffset;
   }
   else if (mFieldOfViewYInDeg < 1.0f)
   {
      mFieldOfViewYInDeg = 1.0f;
   }
   else if (mFieldOfViewYInDeg > 45.0f)
   {
      mFieldOfViewYInDeg = 45.0f;
   }

   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

bool Camera::isFree() const
{
   return mIsFree;
}

void Camera::setFree(bool free)
{
   mIsFree = free;
}

void Camera::updateCoordinateFrame()
{
   Q::quat pitchRotation = Q::angleAxis(glm::radians(-mPitchInDeg), mCameraX);
   Q::quat yawRotation   = Q::angleAxis(glm::radians(mYawInDeg), mCameraY);

   mCameraZ = pitchRotation * yawRotation * mCameraZ;
   mCameraX = glm::normalize(glm::cross(mWorldUp, mCameraZ));
   mCameraY = glm::normalize(glm::cross(mCameraZ, mCameraX));

   mOrientation = Q::lookRotation(mCameraZ, mCameraY);
}
