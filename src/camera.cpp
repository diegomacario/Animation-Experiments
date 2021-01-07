#include <utility>

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
   : mModelTransform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f))
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
   glm::vec3 viewDir = glm::normalize(target - position);
   Q::quat   lookRot = Q::lookRotation(viewDir, worldUp);
   mModelTransform   = Transform(position, lookRot, glm::vec3(1.0f));
}

Camera::Camera(Camera&& rhs) noexcept
   : mFieldOfViewYInDeg(std::exchange(rhs.mFieldOfViewYInDeg, 0.0f))
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
   return mModelTransform.position;
}

glm::mat4 Camera::getViewMatrix()
{
   if (mNeedToUpdateViewMatrix)
   {
      glm::vec3 front = glm::normalize(mModelTransform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
      glm::vec3 up    = glm::normalize(mModelTransform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
      mViewMatrix     = glm::lookAt(mModelTransform.position, mModelTransform.position + front, up);

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
   glm::vec3 viewDir = glm::normalize(target - position);
   Q::quat   lookRot = Q::lookRotation(viewDir, worldUp);
   mModelTransform   = Transform(position, lookRot, glm::vec3(1.0f));

   mFieldOfViewYInDeg = fieldOfViewYInDeg;

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processKeyboardInput(MovementDirection direction, float deltaTime)
{
   float velocity = mMovementSpeed * deltaTime;

   glm::vec3 newPosition(0.0f);

   glm::vec3 front = glm::normalize(mModelTransform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
   glm::vec3 right = glm::normalize(mModelTransform.rotation * glm::vec3(1.0f, 0.0f, 0.0f));

   switch (direction)
   {
   case MovementDirection::Forward:
      newPosition += front * velocity;
      break;
   case MovementDirection::Backward:
      newPosition -= front * velocity;
      break;
   case MovementDirection::Left:
      newPosition += right * velocity;
      break;
   case MovementDirection::Right:
      newPosition -= right * velocity;
      break;
   }

   mModelTransform.position += newPosition;

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
   float yawInDeg   = xOffset * mMouseSensitivity;
   float pitchInDeg = yOffset * mMouseSensitivity;

   //glm::vec3 front;
   //front.x = sin(glm::radians(yawInDeg)) * cos(glm::radians(pitchInDeg));
   //front.y = sin(glm::radians(pitchInDeg));
   //front.z = -1.0f * cos(glm::radians(yawInDeg)) * cos(glm::radians(pitchInDeg));
   //front   = glm::normalize(front);

   //glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
   //glm::vec3 up    = glm::normalize(glm::cross(right, front));

   //Q::quat lookRot = Q::lookRotation(front, glm::vec3(0.0f, 1.0f, 0.0f));
   //mModelTransform.rotation = Q::normalized(Q::conjugate(lookRot) * mModelTransform.rotation);

   Q::quat pitchRot = Q::angleAxis(glm::radians(-pitchInDeg), glm::vec3(1.0f, 0.0f, 0.0f));
   Q::quat yawRot = Q::angleAxis(glm::radians(-yawInDeg), glm::vec3(0.0f, 1.0f, 0.0f));

   //Q::quat newRot = Q::normalized(pitchRot * mModelTransform.rotation * yawRot);
   //mModelTransform.rotation = Q::nlerp(mModelTransform.rotation, newRot, 0.5f);

   mModelTransform.rotation = Q::normalized(pitchRot * mModelTransform.rotation * yawRot);

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
