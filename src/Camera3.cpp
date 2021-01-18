#include <utility>

#include "Camera3.h"

Camera3::Camera3(float            distanceBetweenPlayerAndCamera,
                 float            cameraPitch,
                 float            fieldOfViewYInDeg,
                 const glm::vec3& playerPosition,
                 const Q::quat&   playerOrientation,
                 float            aspectRatio,
                 float            near,
                 float            far,
                 float            movementSpeed,
                 float            mouseSensitivity)
   : mCameraPosition()
   , mCameraOrientationWRTPlayer()
   , mPlayerPosition(playerPosition + glm::vec3(0.0f, 3.0f, 0.0f))
   , mPlayerOrientation(playerOrientation)
   , mDistanceBetweenPlayerAndCamera(distanceBetweenPlayerAndCamera)
   , mCameraPitch(cameraPitch)
   , mFieldOfViewYInDeg(fieldOfViewYInDeg)
   , mAspectRatio(aspectRatio)
   , mNear(near)
   , mFar(far)
   , mMouseSensitivity(mouseSensitivity)
   , mIsFree(false)
   , mViewMatrix()
   , mPerspectiveProjectionMatrix()
   , mPerspectiveProjectionViewMatrix()
   , mNeedToUpdateViewMatrix(true)
   , mNeedToUpdatePerspectiveProjectionMatrix(true)
   , mNeedToUpdatePerspectiveProjectionViewMatrix(true)
{
   // There are two important things to note here:
   // - In OpenGL, the camera always looks down the -Z axis, which means that the camera's Z axis is the opposite of the view direction
   // - Q::lookRotation calculates a quaternion that rotates from the world's Z axis to a desired direction,
   //   which makes that direction the Z axis of the rotated coordinate frame
   // That's why the orientation of the camera is calculated using the camera's Z axis instead of the view direction
   Q::quat   pitchRotation     = Q::angleAxis(glm::radians(mCameraPitch), glm::vec3(1.0f, 0.0f, 0.0f));
   glm::vec3 playerToCamera    = pitchRotation * glm::vec3(0.0f, 0.0f, -1.0f);
   glm::vec3 cameraZ           = playerToCamera;
   mCameraOrientationWRTPlayer = Q::lookRotation(cameraZ, glm::vec3(0.0f, 1.0f, 0.0f));
}

Camera3::Camera3(Camera3&& rhs) noexcept
   : mCameraPosition(std::exchange(rhs.mCameraPosition, glm::vec3(0.0f)))
   , mCameraOrientationWRTPlayer(std::exchange(rhs.mCameraOrientationWRTPlayer, Q::quat()))
   , mPlayerPosition(std::exchange(rhs.mPlayerPosition, glm::vec3(0.0f)))
   , mPlayerOrientation(std::exchange(rhs.mPlayerOrientation, Q::quat()))
   , mDistanceBetweenPlayerAndCamera(std::exchange(rhs.mDistanceBetweenPlayerAndCamera, 0.0f))
   , mCameraPitch(std::exchange(rhs.mCameraPitch, 0.0f))
   , mFieldOfViewYInDeg(std::exchange(rhs.mFieldOfViewYInDeg, 0.0f))
   , mAspectRatio(std::exchange(rhs.mAspectRatio, 0.0f))
   , mNear(std::exchange(rhs.mNear, 0.0f))
   , mFar(std::exchange(rhs.mFar, 0.0f))
   , mMouseSensitivity(std::exchange(rhs.mMouseSensitivity, 0.0f))
   , mViewMatrix(std::exchange(rhs.mViewMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionMatrix(std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionViewMatrix(std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f)))
   , mNeedToUpdateViewMatrix(std::exchange(rhs.mNeedToUpdateViewMatrix, true))
   , mNeedToUpdatePerspectiveProjectionMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true))
   , mNeedToUpdatePerspectiveProjectionViewMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true))
{

}

Camera3& Camera3::operator=(Camera3&& rhs) noexcept
{
   mCameraPosition                              = std::exchange(rhs.mCameraPosition, glm::vec3(0.0f));
   mCameraOrientationWRTPlayer                  = std::exchange(rhs.mCameraOrientationWRTPlayer, Q::quat());
   mPlayerPosition                              = std::exchange(rhs.mPlayerPosition, glm::vec3(0.0f));
   mPlayerOrientation                           = std::exchange(rhs.mPlayerOrientation, Q::quat());
   mDistanceBetweenPlayerAndCamera              = std::exchange(rhs.mDistanceBetweenPlayerAndCamera, 0.0f);
   mCameraPitch                                 = std::exchange(rhs.mCameraPitch, 0.0f);
   mFieldOfViewYInDeg                           = std::exchange(rhs.mFieldOfViewYInDeg, 0.0f);
   mAspectRatio                                 = std::exchange(rhs.mAspectRatio, 0.0f);
   mNear                                        = std::exchange(rhs.mNear, 0.0f);
   mFar                                         = std::exchange(rhs.mFar, 0.0f);
   mMouseSensitivity                            = std::exchange(rhs.mMouseSensitivity, 0.0f);
   mViewMatrix                                  = std::exchange(rhs.mViewMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionMatrix                 = std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionViewMatrix             = std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f));
   mNeedToUpdateViewMatrix                      = std::exchange(rhs.mNeedToUpdateViewMatrix, true);
   mNeedToUpdatePerspectiveProjectionMatrix     = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true);
   mNeedToUpdatePerspectiveProjectionViewMatrix = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true);
   return *this;
}

glm::vec3 Camera3::getPosition()
{
   return mCameraPosition;
}

glm::mat4 Camera3::getViewMatrix()
{
   if (mNeedToUpdateViewMatrix)
   {
      /*
         The camera's model matrix can be calculated as follows (first rotate, then translate):

            cameraModelMat = cameraTranslation * cameraRotation

         The view matrix is equal to the inverse of the camera's model matrix
         Think about it this way:
         - We know the translation and rotation necessary to place and orient the camera
         - If we apply the opposite translation and rotation to the world, it's as if we were looking at it through the camera

         So with that it mind, we can calculate the view matrix as follows:

            viewMat = cameraModelMat^-1 = (cameraTranslation * cameraRotation)^-1 = cameraRotation^-1 * cameraTranslation^-1

         Which looks like this in code:

            glm::mat4 inverseCameraRotation    = Q::quatToMat4(Q::conjugate(mOrientation));
            glm::mat4 inverseCameraTranslation = glm::translate(glm::mat4(1.0), -mPosition);

            mViewMatrix = inverseCameraRotation * inverseCameraTranslation;

         The implementation below is simply an optimized version of the above
      */

      //mViewMatrix       = Q::quatToMat4(Q::conjugate(mOrientation));
      //// Dot product of X axis with negated position
      //mViewMatrix[3][0] = -(mViewMatrix[0][0] * mPosition.x + mViewMatrix[1][0] * mPosition.y + mViewMatrix[2][0] * mPosition.z);
      //// Dot product of Y axis with negated position
      //mViewMatrix[3][1] = -(mViewMatrix[0][1] * mPosition.x + mViewMatrix[1][1] * mPosition.y + mViewMatrix[2][1] * mPosition.z);
      //// Dot product of Z axis with negated position
      //mViewMatrix[3][2] = -(mViewMatrix[0][2] * mPosition.x + mViewMatrix[1][2] * mPosition.y + mViewMatrix[2][2] * mPosition.z);

      Q::quat globalOrientation = mCameraOrientationWRTPlayer * mPlayerOrientation;

      glm::vec3 cameraZ = globalOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
      mCameraPosition   = mPlayerPosition + (cameraZ * mDistanceBetweenPlayerAndCamera);

      glm::mat4 inverseCameraRotation    = Q::quatToMat4(Q::conjugate(globalOrientation));
      glm::mat4 inverseCameraTranslation = glm::translate(glm::mat4(1.0), -mCameraPosition);

      mViewMatrix = inverseCameraRotation * inverseCameraTranslation;

      mNeedToUpdateViewMatrix = false;
   }

   return mViewMatrix;
}

glm::mat4 Camera3::getPerspectiveProjectionMatrix()
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

glm::mat4 Camera3::getPerspectiveProjectionViewMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionViewMatrix)
   {
      mPerspectiveProjectionViewMatrix = getPerspectiveProjectionMatrix() * getViewMatrix();
      mNeedToUpdatePerspectiveProjectionViewMatrix = false;
   }

   return mPerspectiveProjectionViewMatrix;
}

void Camera3::reposition(float            distanceBetweenPlayerAndCamera,
                         float            cameraPitch,
                         float            fieldOfViewYInDeg,
                         const glm::vec3& playerPosition,
                         const Q::quat&   playerOrientation)
{
   mDistanceBetweenPlayerAndCamera = distanceBetweenPlayerAndCamera;
   mCameraPitch                    = cameraPitch;
   mFieldOfViewYInDeg              = fieldOfViewYInDeg;

   // There are two important things to note here:
   // - In OpenGL, the camera always looks down the -Z axis, which means that the camera's Z axis is the opposite of the view direction
   // - Q::lookRotation calculates a quaternion that rotates from the world's Z axis to a desired direction,
   //   which makes that direction the Z axis of the rotated coordinate frame
   // That's why the orientation of the camera is calculated using the camera's Z axis instead of the view direction
   glm::vec3 playerToCamera    = Q::angleAxis(glm::radians(mCameraPitch), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec3(0.0f, 0.0f, -1.0f);
   glm::vec3 cameraZ           = playerToCamera;
   mCameraOrientationWRTPlayer = Q::lookRotation(cameraZ, glm::vec3(0.0f, 1.0f, 0.0f));
   mPlayerPosition             = playerPosition + glm::vec3(0.0f, 3.0f, 0.0f);
   mPlayerOrientation          = playerOrientation;

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::processMouseMovement(float xOffset, float yOffset)
{
   // The xOffset corresponds to a change in the yaw of the view direction
   // If the xOffset is positive, the view direction is moving right
   // If the xOffset is negative, the view direction is moving left

   // The yOffset corresponds to a change in the pitch of the view direction
   // If the yOffset is positive, the view direction is moving up
   // If the yOffset is negative, the view direction is moving down

   // Since the camera's orientation is calculated using the camera's Z axis, which is the opposite the view direction,
   // we need to negate the xOffset and yOffset so that they correspond to changes in the yaw/pitch of the camera's Z axis instead of the view direction

   // Here are some examples to illustrate that idea:
   // - If the xOffset is equal to 1.0f, the view direction is moving right by that amount while the camera's Z axis is moving left by the same amount
   //   Since the camera's Z vector is moving left, its xOffset is equal to -1.0f
   // - If the yOffset is equal to -1.0f, the view direction is moving down by that amount while the camera's Z axis is moving up by the same amount
   //   Since the camera's Z vector is moving up, its yOffset is equal to 1.0f

   float yawChangeOfCameraZInDeg   = -xOffset * mMouseSensitivity;
   float pitchChangeOfCameraZInDeg = -yOffset * mMouseSensitivity;

   if (mCameraPitch + pitchChangeOfCameraZInDeg > 90.0f)
   {
      pitchChangeOfCameraZInDeg = 90.0f - mCameraPitch;
      mCameraPitch = 90.0f;
   }
   //else if (mCameraPitch + pitchChangeOfCameraZInDeg < -90.0f)
   //{
   //   pitchChangeOfCameraZInDeg = -90.0f - mCameraPitch;
   //   mCameraPitch = -90.0f;
   //}
   else if (mCameraPitch + pitchChangeOfCameraZInDeg < 0.0f)
   {
      pitchChangeOfCameraZInDeg = -mCameraPitch;
      mCameraPitch = 0.0f;
   }
   else
   {
      mCameraPitch += pitchChangeOfCameraZInDeg;
   }

   // The yaw is defined as a counterclockwise rotation around the Y axis
   Q::quat yawRot   = Q::angleAxis(glm::radians(yawChangeOfCameraZInDeg),   glm::vec3(0.0f, 1.0f, 0.0f));
   // The pitch is defined as a clockwise rotation around the X axis
   // Since the rotation is clockwise, the angle is negated in the call to Q::angleAxis below
   Q::quat pitchRot = Q::angleAxis(glm::radians(-pitchChangeOfCameraZInDeg), glm::vec3(1.0f, 0.0f, 0.0f));

   // To avoid introducing roll, the yaw is applied globally while the pitch is applied locally
   // In other words, the yaw is applied with respect to the world's Y axis, while the pitch is applied with respect to the camera's X axis
   mCameraOrientationWRTPlayer = Q::normalized(pitchRot * mCameraOrientationWRTPlayer * yawRot);

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::processScrollWheelMovement(float yOffset)
{
   mDistanceBetweenPlayerAndCamera -= yOffset;

   if (mDistanceBetweenPlayerAndCamera < 0.0f)
   {
      mDistanceBetweenPlayerAndCamera = 0.0f;
   }
   else if (mDistanceBetweenPlayerAndCamera > 30.0f)
   {
      mDistanceBetweenPlayerAndCamera = 30.0f;
   }

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera3::processPlayerMovement(const glm::vec3& playerPosition, const Q::quat& playerOrientation)
{
   mPlayerPosition    = playerPosition + glm::vec3(0.0f, 3.0f, 0.0f);
   mPlayerOrientation = playerOrientation;

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

bool Camera3::isFree() const
{
   return mIsFree;
}

void Camera3::setFree(bool free)
{
   mIsFree = free;
}
