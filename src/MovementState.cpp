#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

#include <array>
#include <random>

#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "Intersection.h"
#include "Blending.h"
#include "MovementState.h"

MovementState::MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                             const std::shared_ptr<Window>&             window,
                             const std::shared_ptr<Camera>&             camera,
                             const std::shared_ptr<Shader>&             gameObject3DShader,
                             const std::shared_ptr<GameObject3D>&       table,
                             const std::shared_ptr<GameObject3D>&       teapot)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mGameObject3DShader(gameObject3DShader)
   , mTable(table)
   , mTeapot(teapot)
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/mesh_with_simple_illumination.frag");
   configureLights(mAnimatedMeshShader);

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/mesh_with_simple_illumination.frag");
   configureLights(mStaticMeshShader);

   // Load the diffuse texture of the animated character
   mDiffuseTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/woman/Woman.png");

   // Load the animated character
   cgltf_data* data        = LoadGLTFFile("resources/models/woman/Woman.gltf");
   mSkeleton               = LoadSkeleton(data);
   mAnimatedMeshes         = LoadAnimatedMeshes(data);
   std::vector<Clip> clips = LoadClips(data);
   FreeGLTFFile(data);

   // Rearrange the skeleton
   JointMap jointMap = RearrangeSkeleton(mSkeleton);

   // Rearrange the meshes
   for (unsigned int meshIndex = 0,
        numMeshes = static_cast<unsigned int>(mAnimatedMeshes.size());
        meshIndex < numMeshes;
        ++meshIndex)
   {
      RearrangeMesh(mAnimatedMeshes[meshIndex], jointMap);
   }

   // Optimize the clips, rearrange them and get their names
   mClips.resize(clips.size());
   for (unsigned int clipIndex = 0,
        numClips = static_cast<unsigned int>(clips.size());
        clipIndex < numClips;
        ++clipIndex)
   {
      mClips[clipIndex] = OptimizeClip(clips[clipIndex]);
      RearrangeFastClip(mClips[clipIndex], jointMap);
      mClipNames += (mClips[clipIndex].GetName() + '\0');
   }

   // Configure the VAOs of the animated meshes
   int positionsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("texCoord");
   int weightsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("weights");
   int influencesAttribLocOfAnimatedShader = mAnimatedMeshShader->getAttributeLocation("joints");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].ConfigureVAO(positionsAttribLocOfAnimatedShader,
                                      normalsAttribLocOfAnimatedShader,
                                      texCoordsAttribLocOfAnimatedShader,
                                      weightsAttribLocOfAnimatedShader,
                                      influencesAttribLocOfAnimatedShader);
   }

   // Set the initial clip
   unsigned int numClips = static_cast<unsigned int>(mClips.size());
   for (unsigned int clipIndex = 0; clipIndex < numClips; ++clipIndex)
   {
      if (mClips[clipIndex].GetName() == "Walking")
      {
         mSelectedClip = clipIndex;
         mAnimationData.currentClipIndex = clipIndex;
      }
   }

   // Set the initial skinning mode
   mSelectedSkinningMode = SkinningMode::GPU;
   // Set the initial playback speed
   mSelectedPlaybackSpeed = 1.0f;
   // Set the initial rendering options
   mDisplayMesh = true;
   mDisplayBones = false;
   mDisplayJoints = false;
   mWireframeModeForMesh = false;
   mWireframeModeForJoints = false;
   mPerformDepthTesting = true;

   // Set the initial pose
   mAnimationData.animatedPose = mSkeleton.GetRestPose();

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mAnimationData.animatedPose);

   // --- --- ---

   // Load the ground
   data = LoadGLTFFile("resources/models/ground/terrain.gltf");
   mGroundMeshes = LoadStaticMeshes(data);
   FreeGLTFFile(data);

   int positionsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader   = mStaticMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("texCoord");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGroundMeshes.size());
        i < size;
        ++i)
   {
      mGroundMeshes[i].ConfigureVAO(positionsAttribLocOfStaticShader,
                                    normalsAttribLocOfStaticShader,
                                    texCoordsAttribLocOfStaticShader,
                                    -1,
                                    -1);
   }

   // Load the texture of the ground
   mGroundTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/ground/uv.png");

   // Get the triangles that make up the ground
   mGroundTriangles = GetTrianglesFromMeshes(mGroundMeshes);

   // Compose the motion track, which tells the character where to walk by supplying X and Z coordinates
   mMotionTrack.SetInterpolation(Interpolation::Linear);
   mMotionTrack.SetNumberOfFrames(5);
   // Frame 0
   mMotionTrack.GetFrame(0).mTime     = 0.0f;
   mMotionTrack.GetFrame(0).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(0).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(0).mValue[2] = 1.0f;
   // Frame 1
   mMotionTrack.GetFrame(1).mTime     = 1.0f;
   mMotionTrack.GetFrame(1).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(1).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(1).mValue[2] = 10.0f;
   // Frame 2
   mMotionTrack.GetFrame(2).mTime     = 3.0f;
   mMotionTrack.GetFrame(2).mValue[0] = 22.0f;
   mMotionTrack.GetFrame(2).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(2).mValue[2] = 10.0f;
   // Frame 3
   mMotionTrack.GetFrame(3).mTime     = 4.0f;
   mMotionTrack.GetFrame(3).mValue[0] = 22.0f;
   mMotionTrack.GetFrame(3).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(3).mValue[2] = 2.0f;
   // Frame 4
   mMotionTrack.GetFrame(4).mTime     = 6.0f;
   mMotionTrack.GetFrame(4).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(4).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(4).mValue[2] = 1.0f;

   // Create the IK legs
   mLeftLeg = IKLeg(mSkeleton, "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase");
   mLeftLeg.SetAnkleOffset(0.2f);  // The left ankle is 0.2 units above the ground
   mRightLeg = IKLeg(mSkeleton, "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase");
   mRightLeg.SetAnkleOffset(0.2f); // The right ankle is 0.2 units above the ground

   // Compose the pin track of the left foot, which tells us when the left foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   ScalarTrack leftFootPinTrack;
   leftFootPinTrack.SetInterpolation(Interpolation::Cubic);
   leftFootPinTrack.SetNumberOfFrames(4);
   // Frame 0
   leftFootPinTrack.GetFrame(0).mTime     = 0.0f;
   leftFootPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   leftFootPinTrack.GetFrame(1).mTime     = 0.4f;
   leftFootPinTrack.GetFrame(1).mValue[0] = 1;
   // Frame 2
   leftFootPinTrack.GetFrame(2).mTime     = 0.6f;
   leftFootPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   leftFootPinTrack.GetFrame(3).mTime     = 1.0f;
   leftFootPinTrack.GetFrame(3).mValue[0] = 0;

   // Compose the pin track of the right foot, which tells us when the right foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   ScalarTrack rightFootPinTrack;
   rightFootPinTrack.SetInterpolation(Interpolation::Cubic);
   rightFootPinTrack.SetNumberOfFrames(4);
   // Frame 0
   rightFootPinTrack.GetFrame(0).mTime     = 0.0f;
   rightFootPinTrack.GetFrame(0).mValue[0] = 1;
   // Frame 1
   rightFootPinTrack.GetFrame(1).mTime     = 0.3f;
   rightFootPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   rightFootPinTrack.GetFrame(2).mTime     = 0.7f;
   rightFootPinTrack.GetFrame(2).mValue[0] = 0;
   // Frame 3
   rightFootPinTrack.GetFrame(3).mTime     = 1.0f;
   rightFootPinTrack.GetFrame(3).mValue[0] = 1;

   // Set the pin tracks to the appropriate legs
   mLeftLeg.SetPinTrack(leftFootPinTrack);
   mRightLeg.SetPinTrack(rightFootPinTrack);

   // Initialize the values we use to describe the position of the character
   mModelTransform               = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
   mHeightOfOriginOfYPositionRay = 11.0f;
   mPreviousYPositionOfCharacter = 0.0f;
   mSinkIntoGround               = 0.15f;
   mMotionTrackTime              = 0.0f;
   mMotionTrackPlaybackSpeed     = 0.3f;
   mMotionTrackDuration          = mMotionTrack.GetEndTime() - mMotionTrack.GetStartTime();
   mMotionTrackFutureTimeOffset  = 0.1f;
   mHeightOfHip                  = 2.0f;
   mHeightOfKnees                = 1.0f;
   mDistanceFromAnkleToToe       = 0.3f;

   // Shoot a ray downwards to determine the initial Y position of the character,
   // and sink it into the ground a little so that the IK solver has room to work
   Ray groundRay(glm::vec3(mModelTransform.position.x, mHeightOfOriginOfYPositionRay, mModelTransform.position.z), glm::vec3(0.0f, -1.0f, 0.0f));
   glm::vec3 hitPoint;
   for (unsigned int i = 0,
        numTriangles = static_cast<unsigned int>(mGroundTriangles.size());
        i < numTriangles;
        ++i)
   {
      if (DoesRayIntersectTriangle(groundRay, mGroundTriangles[i], hitPoint))
      {
         mModelTransform.position      = hitPoint;
         mModelTransform.position.y   -= mSinkIntoGround;
         mPreviousYPositionOfCharacter = mModelTransform.position.y;
         break;
      }
   }
}

void MovementState::enter()
{
   resetCamera();
   resetScene();
}

void MovementState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());

      // In the play state, the following rules are applied to the cursor:
      // - Fullscreen: Cursor is always disabled
      // - Windowed with a free camera: Cursor is disabled
      // - Windowed with a fixed camera: Cursor is enabled
      if (mWindow->isFullScreen())
      {
         // Disable the cursor when fullscreen
         //mWindow->enableCursor(false);
         if (mCamera->isFree())
         {
            // Disable the cursor when fullscreen with a free camera
            mWindow->enableCursor(false);
            // Going from windowed to fullscreen changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
      }
      else if (!mWindow->isFullScreen())
      {
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
            // Going from fullscreen to windowed changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      }
   }

   // Change the number of samples used for anti aliasing
   if (mWindow->keyIsPressed(GLFW_KEY_1) && !mWindow->keyHasBeenProcessed(GLFW_KEY_1))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_1);
      mWindow->setNumberOfSamples(1);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_2) && !mWindow->keyHasBeenProcessed(GLFW_KEY_2))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_2);
      mWindow->setNumberOfSamples(2);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_4) && !mWindow->keyHasBeenProcessed(GLFW_KEY_4))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_4);
      mWindow->setNumberOfSamples(4);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_8) && !mWindow->keyHasBeenProcessed(GLFW_KEY_8))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_8);
      mWindow->setNumberOfSamples(8);
   }

   // Reset the camera
   if (mWindow->keyIsPressed(GLFW_KEY_R)) { resetCamera(); }

   // Make the camera free or fixed
   if (mWindow->keyIsPressed(GLFW_KEY_C) && !mWindow->keyHasBeenProcessed(GLFW_KEY_C))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_C);
      mCamera->setFree(!mCamera->isFree());

      //if (!mWindow->isFullScreen())
      //{
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      //}

      mWindow->resetMouseMoved();
   }

   // Move and orient the camera
   if (mCamera->isFree())
   {
      // Move
      if (mWindow->keyIsPressed(GLFW_KEY_W)) { mCamera->processKeyboardInput(Camera::MovementDirection::Forward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_S)) { mCamera->processKeyboardInput(Camera::MovementDirection::Backward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_A)) { mCamera->processKeyboardInput(Camera::MovementDirection::Left, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_D)) { mCamera->processKeyboardInput(Camera::MovementDirection::Right, deltaTime); }

      // Orient
      if (mWindow->mouseMoved())
      {
         mCamera->processMouseMovement(mWindow->getCursorXOffset(), mWindow->getCursorYOffset());
         mWindow->resetMouseMoved();
      }

      // Zoom
      if (mWindow->scrollWheelMoved())
      {
         mCamera->processScrollWheelMovement(mWindow->getScrollYOffset());
         mWindow->resetScrollWheelMoved();
      }
   }
}

void MovementState::update(float deltaTime)
{
   deltaTime *= mSelectedPlaybackSpeed;

   // --- --- ---

   // Position Sampling and Height Correction
   // **********************************************************************************************************************************************

   // mMotionTrackTime is the time we use to sample the motion track
   // mMotionTrackPlaybackSpeed is used to adjust the playback of the motion track
   // so that the speed at which the character moves matches its animation
   mMotionTrackTime += deltaTime * mMotionTrackPlaybackSpeed;
   // Loop mMotionTrackTime so that it doesn't become infinitely large
   if (mMotionTrackTime > 6.0f)
   {
      mMotionTrackTime -= 6.0f;
   }

   // Sample the motion track to get the X and Z world space position values of the character
   // at the current time and a little in the future
   glm::vec3 currWorldPosOfCharacter = mMotionTrack.Sample(mMotionTrackTime, true);
   glm::vec3 nextWorldPosOfCharacter = mMotionTrack.Sample(mMotionTrackTime + mMotionTrackFutureTimeOffset, true);
   // Since the motion track only gives us X and Z values, we reuse the Y value from the previous frame as the default one
   currWorldPosOfCharacter.y         = mModelTransform.position.y;
   nextWorldPosOfCharacter.y         = mModelTransform.position.y;
   mModelTransform.position          = currWorldPosOfCharacter;

   // Shoot a ray downwards to determine the Y position of the character,
   // and sink it into the ground a little so that the IK solver has room to work
   Ray groundRay(glm::vec3(mModelTransform.position.x, mHeightOfOriginOfYPositionRay, mModelTransform.position.z), glm::vec3(0.0f, -1.0f, 0.0f));
   glm::vec3 hitPoint;
   for (unsigned int i = 0,
        numTriangles = static_cast<unsigned int>(mGroundTriangles.size());
        i < numTriangles;
        ++i)
   {
      if (DoesRayIntersectTriangle(groundRay, mGroundTriangles[i], hitPoint))
      {
         mModelTransform.position = hitPoint;
         mModelTransform.position.y -= mSinkIntoGround;
         break;
      }
   }

   // Calculate the new forward direction of the character in world space
   glm::vec3 newFwdDirOfCharacter = glm::normalize(nextWorldPosOfCharacter - currWorldPosOfCharacter);
   // Calculate the quaternion that rotates from the world forward direction (0.0f, 0.0f, 1.0f)
   // to the new forward direction of the character in world space
   Q::quat rotFromWorldFwdToNewFwdDirOfCharacter = Q::lookRotation(newFwdDirOfCharacter, glm::vec3(0, 1, 0));
   // We want to interpolate between the current forward direction of the character and the new one by a small factor
   // Before doing that, however, we must perform a neighborhood check
   if (Q::dot(mModelTransform.rotation, rotFromWorldFwdToNewFwdDirOfCharacter) < 0.0f)
   {
      rotFromWorldFwdToNewFwdDirOfCharacter = rotFromWorldFwdToNewFwdDirOfCharacter * -1.0f;
   }
   // Interpolate between the rotations that represent the old forward direction and the new one
   mModelTransform.rotation = Q::nlerp(mModelTransform.rotation, rotFromWorldFwdToNewFwdDirOfCharacter, deltaTime * 10.0f);

   // --- --- ---

   if (mAnimationData.currentClipIndex != mSelectedClip)
   {
      mAnimationData.currentClipIndex = mSelectedClip;
      mAnimationData.animatedPose     = mSkeleton.GetRestPose();
      mAnimationData.playbackTime     = 0.0f;
   }

   if (mAnimationData.currentSkinningMode != mSelectedSkinningMode)
   {
      if (mAnimationData.currentSkinningMode == SkinningMode::GPU)
      {
         switchFromGPUToCPU();
      }
      else if (mAnimationData.currentSkinningMode == SkinningMode::CPU)
      {
         switchFromCPUToGPU();
      }

      mAnimationData.currentSkinningMode = static_cast<SkinningMode>(mSelectedSkinningMode);
   }

   // Sample the clip to get the animated pose
   FastClip& currClip = mClips[mAnimationData.currentClipIndex];
   mAnimationData.playbackTime = currClip.Sample(mAnimationData.animatedPose, mAnimationData.playbackTime + deltaTime);

   // --- --- ---

   // Ankle Correction
   // **********************************************************************************************************************************************

   // The keyframes of the pin tracks are set in normalized time, so they must be sampled with the normalized time
   float normalizedPlaybackTime = (mAnimationData.playbackTime - currClip.GetStartTime()) / currClip.GetDuration();
   float leftLegPinTrackValue   = mLeftLeg.GetPinTrack().Sample(normalizedPlaybackTime, true);
   float rightLegPinTrackValue  = mRightLeg.GetPinTrack().Sample(normalizedPlaybackTime, true);

   // Calculate the world positions of the left and right ankles
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   glm::vec3 worldPosOfLeftAnkle  = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mLeftLeg.GetAnkleIndex())).position;
   glm::vec3 worldPosOfRightAnkle = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mRightLeg.GetAnkleIndex())).position;

   // Construct rays for the left and right ankles
   // These shoot down from the height of the hip
   Ray leftAnkleRay(worldPosOfLeftAnkle + glm::vec3(0.0f, mHeightOfHip, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
   Ray rightAnkleRay(worldPosOfRightAnkle + glm::vec3(0.0f, mHeightOfHip, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

   // If the rays don't hit anything, we use the positions of the ankles as default values
   glm::vec3 leftAnkleGroundIKTarget  = worldPosOfLeftAnkle;
   glm::vec3 rightAnkleGroundIKTarget = worldPosOfRightAnkle;

   // Here we do the equivalent of the following:
   // - Shoot a ray downwards from the height of the hip to the ankle
   // - Shoot a ray downwards from the height of the hip through the ankle to infinity
   // The first ray tells us if there's ground above the ankle
   // If there is, that becomes the new position of the ankle
   // The second ray tells us if there's ground above or below the ankle
   // If there is, that becomes the new target of the IK chain
   for (unsigned int i = 0,
        numTriangles = static_cast<unsigned int>(mGroundTriangles.size());
        i < numTriangles;
        ++i)
   {
      if (DoesRayIntersectTriangle(leftAnkleRay, mGroundTriangles[i], hitPoint))
      {
         // Is the hit point between the ankle and the hip?
         // In other words, is it above the ankle?
         // TODO: Is there a better way to check if the hit point is above the ankle?
         if (glm::length2(hitPoint - leftAnkleRay.origin) < mHeightOfHip * mHeightOfHip)
         {
            // If it is, we update the position of the ankle to be on the ground
            // We do this because if there's ground above the ankle, the foot should be on the ground regardless of what the pin track says
            // In other words, here we override the animation to avoid having the foot be below ground
            worldPosOfLeftAnkle = hitPoint;
         }

         leftAnkleGroundIKTarget = hitPoint;
      }

      if (DoesRayIntersectTriangle(rightAnkleRay, mGroundTriangles[i], hitPoint))
      {
         // Is the hit point between the ankle and the hip?
         // In other words, is it above the ankle?
         // TODO: Is there a better way to check if the hit point is above the ankle?
         if (glm::length2(hitPoint - rightAnkleRay.origin) < mHeightOfHip * mHeightOfHip)
         {
            // If it is, we update the position of the ankle to be on the ground
            // We do this because if there's ground above the ankle, the foot should be on the ground regardless of what the pin track says
            // In other words, here we override the animation to avoid having the foot be below ground
            worldPosOfRightAnkle = hitPoint;
         }

         rightAnkleGroundIKTarget = hitPoint;
      }
   }

   // Create a vector with the current X and Z position values of the character and the old Y value
   glm::vec3 currPosOfCharacterWithPreviousHeight = mModelTransform.position;
   currPosOfCharacterWithPreviousHeight.y         = mPreviousYPositionOfCharacter;
   // Interpolate between the old height and the new height to minimize jumpiness when the height of the ground changes abruptly
   mModelTransform.position = glm::lerp(currPosOfCharacterWithPreviousHeight, mModelTransform.position, deltaTime * 10.0f);
   // Update mPreviousYPositionOfCharacter
   mPreviousYPositionOfCharacter = mModelTransform.position.y;

   // Interpolate between the current world position of the left ankle and its ground IK target based on the value of the pin track
   // If the pin track says that the foot should be on the ground, then the ground IK target will be favored
   // If the pin track says that the foot should not be on the ground, the the world position of the ankle, which is given by the animation clip, will be favored
   // Note that if we detected that there's ground above the ankle, worldPosOfLeftAnkle and leftAnkleGroundIKTarget will be the same, which means that the foot
   // will be on the ground regardless of what the pin track says
   worldPosOfLeftAnkle  = glm::lerp(worldPosOfLeftAnkle, leftAnkleGroundIKTarget, leftLegPinTrackValue);
   // Do the same for the right ankle
   worldPosOfRightAnkle = glm::lerp(worldPosOfRightAnkle, rightAnkleGroundIKTarget, rightLegPinTrackValue);

   // Solve the IK chains of the left and right legs so that their end effectors (ankles) are at the positions we interpolated above
   mLeftLeg.Solve(mModelTransform, mAnimationData.animatedPose, worldPosOfLeftAnkle);
   mRightLeg.Solve(mModelTransform, mAnimationData.animatedPose, worldPosOfRightAnkle);

   // Blend the resulting IK chains into the animated pose
   // Note how the blend factor is equal to 1.0f
   // We want the legs of the animated pose to be equal to the IK chains
   Blend(mAnimationData.animatedPose, mLeftLeg.GetAdjustedPose(), 1.0f, mLeftLeg.GetHipIndex(), mAnimationData.animatedPose);
   Blend(mAnimationData.animatedPose, mRightLeg.GetAdjustedPose(), 1.0f, mRightLeg.GetHipIndex(), mAnimationData.animatedPose);

   // Toe Correction
   // **********************************************************************************************************************************************

   // Calculate the world transforms of the final ankles
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   Transform worldTransfOfLeftAnkle  = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mLeftLeg.GetAnkleIndex()));
   Transform worldTransfOfRightAnkle = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mRightLeg.GetAnkleIndex()));

   // Calculate the world positions of the toes
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   glm::vec3 worldPosOfLeftToe = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mLeftLeg.GetToeIndex())).position;
   glm::vec3 worldPosOfRightToe = combine(mModelTransform, mAnimationData.animatedPose.GetGlobalTransform(mRightLeg.GetToeIndex())).position;

   // World forward direction of character
   glm::vec3 worldFwdDirOfCharacter = mModelTransform.rotation * glm::vec3(0.0f, 0.0f, 1.0f);

   // Construct rays for the left and right toes
   // These are composed a follows:
   // - Start at position of the ankle
   // - Move the origin to the height of the toe
   // - Move the origin up by the distance between the toe and the knee
   // - Move the origin forward by the distance between the ankle and the toe
   // By doing this we create a ray that shoots down from the knee in front of the ankle
   glm::vec3 originOfLeftToeRay = worldTransfOfLeftAnkle.position;
   originOfLeftToeRay.y         = worldPosOfLeftToe.y + mHeightOfKnees;
   originOfLeftToeRay          += worldFwdDirOfCharacter * mDistanceFromAnkleToToe;
   Ray leftToeRay(originOfLeftToeRay, glm::vec3(0.0f, -1.0f, 0.0f));

   glm::vec3 originOfRightToeRay = worldTransfOfRightAnkle.position;
   originOfRightToeRay.y         = worldPosOfRightToe.y + mHeightOfKnees;
   originOfRightToeRay          += worldFwdDirOfCharacter * mDistanceFromAnkleToToe;
   Ray rightToeRay = Ray(originOfRightToeRay, glm::vec3(0.0f, -1.0f, 0.0f));

   // If the rays don't hit anything, we use the positions of the toes as default values
   glm::vec3 leftToeGroundIKTarget  = worldPosOfLeftToe;
   glm::vec3 rightToeGroundIKTarget = worldPosOfRightToe;
   // We use these if we hit anything above the toes
   glm::vec3 newWorldPosOfLeftToe  = worldPosOfLeftToe;
   glm::vec3 newWorldPosOfRightToe = worldPosOfRightToe;

   // Here we do the equivalent of the following:
   // - Shoot a ray downwards from the height of the knees to the toe
   // - Shoot a ray downwards from the height of the knees through the toe to infinity
   // The first ray tells us if there's ground above the toe
   // If there is, that becomes the new position of the toe
   // The second ray tells us if there's ground above or below the toe
   // If there is, that becomes the new target of the IK chain
   for (unsigned int i = 0,
        numTriangles = static_cast<unsigned int>(mGroundTriangles.size());
        i < numTriangles;
        ++i)
   {
      if (DoesRayIntersectTriangle(leftToeRay, mGroundTriangles[i], hitPoint))
      {
         // Is the hit point between the toe and the knees?
         // In other words, is it above the toe?
         // TODO: Is there a better way to check if the hit point is above the toe?
         if (glm::length2(hitPoint - leftToeRay.origin) < mHeightOfKnees * mHeightOfKnees)
         {
            // If it is, we update the position of the toe to be on the ground
            // We do this because if there's ground above the toe, the toe should be on the ground regardless of what the pin track says
            // In other words, here we override the animation to avoid having the toe be below ground
            newWorldPosOfLeftToe = hitPoint;
         }

         leftToeGroundIKTarget = hitPoint;
      }

      if (DoesRayIntersectTriangle(rightToeRay, mGroundTriangles[i], hitPoint))
      {
         // Is the hit point between the toe and the knees?
         // In other words, is it above the toe?
         // TODO: Is there a better way to check if the hit point is above the toe?
         if (glm::length2(hitPoint - rightToeRay.origin) < mHeightOfKnees * mHeightOfKnees)
         {
            // If it is, we update the position of the toe to be on the ground
            // We do this because if there's ground above the toe, the toe should be on the ground regardless of what the pin track says
            // In other words, here we override the animation to avoid having the toe be below ground
            newWorldPosOfRightToe = hitPoint;
         }

         rightToeGroundIKTarget = hitPoint;
      }
   }

   // Interpolate between the new world position of the left toe and its ground IK target based on the value of the pin track
   // If the pin track says that the toe should be on the ground, then the ground IK target will be favored
   // If the pin track says that the toe should not be on the ground, the the world position of the toe, which is given by the animation clip, will be favored
   // Note that if we detected that there's ground above the toe, newWorldPosOfLeftToe and leftToeGroundIKTarget will be the same, which means that the toe
   // will be on the ground regardless of what the pin track says
   newWorldPosOfLeftToe = glm::lerp(newWorldPosOfLeftToe, leftToeGroundIKTarget, leftLegPinTrackValue);
   // Do the same for the right toes
   newWorldPosOfRightToe = glm::lerp(newWorldPosOfRightToe, rightToeGroundIKTarget, rightLegPinTrackValue);

   // Rotate the left ankle if necessary
   glm::vec3 leftAnkleToCurrToe = worldPosOfLeftToe - worldTransfOfLeftAnkle.position;
   glm::vec3 leftAnkleToNewToe  = newWorldPosOfLeftToe - worldTransfOfLeftAnkle.position;
   // TODO: Use constant
   if (glm::dot(leftAnkleToCurrToe, leftAnkleToNewToe) > 0.00001f)
   {
      // Construct a world rotation that goes from the current toe to the new one
      Q::quat worldRotFromCurrToeToNewToe = Q::fromTo(leftAnkleToCurrToe, leftAnkleToNewToe);

      // Apply the toe-to-toe world rotation to the world rotation of the ankle
      Q::quat newWorldRotOfLeftAnkle = worldTransfOfLeftAnkle.rotation * worldRotFromCurrToeToNewToe;

      // Multiply the new world rotation of the ankle by the inverse of its old world rotation to get:
      // worldTransfOfLeftAnkle.rotation^-1 * newWorldRotOfAnkle = (D * C * B * A_old)^-1 * (D * C * B * A_new) = (A_old^-1 * B^-1 * C^-1 * D^-1) * (D * C * B * A_new) = A_old^-1 * A_new
      Q::quat newLocalRotOfLeftAnkle = newWorldRotOfLeftAnkle * inverse(worldTransfOfLeftAnkle.rotation);

      // Get the local transform of the left ankle
      Transform localTransfOfLeftAnkle = mAnimationData.animatedPose.GetLocalTransform(mLeftLeg.GetAnkleIndex());
      // Multiply newLocalRotOfLeftAnkle by the old local rotation of the left ankle to get:
      // localTransfOfLeftAnkle * newLocalRotOfLeftAnkle = A_old * A_old^-1 * A_new = A_new
      localTransfOfLeftAnkle.rotation = newLocalRotOfLeftAnkle * localTransfOfLeftAnkle.rotation;
      // Update the local transform of the left ankle in the pose
      mAnimationData.animatedPose.SetLocalTransform(mLeftLeg.GetAnkleIndex(), localTransfOfLeftAnkle);
   }

   // Rotate the right ankle if necessary
   glm::vec3 rightAnkleToCurrToe = worldPosOfRightToe - worldTransfOfRightAnkle.position;
   glm::vec3 rightAnkleToNewToe  = newWorldPosOfRightToe - worldTransfOfRightAnkle.position;
   // TODO: Use constant
   if (glm::dot(rightAnkleToCurrToe, rightAnkleToNewToe) > 0.00001f)
   {
      // Construct a world rotation that goes from the current toe to the new one
      Q::quat worldRotFromCurrToeToNewToe = Q::fromTo(rightAnkleToCurrToe, rightAnkleToNewToe);

      // Apply the toe-to-toe world rotation to the world rotation of the ankle
      Q::quat newWorldRotOfRightAnkle = worldTransfOfRightAnkle.rotation * worldRotFromCurrToeToNewToe;

      // Multiply the new world rotation of the ankle by the inverse of its old world rotation to get:
      // worldTransfOfRightAnkle.rotation^-1 * newWorldRotOfAnkle = (D * C * B * A_old)^-1 * (D * C * B * A_new) = (A_old^-1 * B^-1 * C^-1 * D^-1) * (D * C * B * A_new) = A_old^-1 * A_new
      Q::quat newLocalRotOfRightAnkle = newWorldRotOfRightAnkle * inverse(worldTransfOfRightAnkle.rotation);

      // Get the local transform of the right ankle
      Transform localTransfOfRightAnkle = mAnimationData.animatedPose.GetLocalTransform(mRightLeg.GetAnkleIndex());
      // Multiply newLocalRotOfRightAnkle by the old local rotation of the right ankle to get:
      // localTransfOfRightAnkle * newLocalRotOfRightAnkle = A_old * A_old^-1 * A_new = A_new
      localTransfOfRightAnkle.rotation = newLocalRotOfRightAnkle * localTransfOfRightAnkle.rotation;
      // Update the local transform of the right ankle in the pose
      mAnimationData.animatedPose.SetLocalTransform(mRightLeg.GetAnkleIndex(), localTransfOfRightAnkle);
   }

   // --- --- ---

   // Get the palette of the animated pose
   mAnimationData.animatedPose.GetMatrixPalette(mAnimationData.animatedPosePalette);

   std::vector<glm::mat4>& inverseBindPose = mSkeleton.GetInvBindPose();

   // Generate the skin matrices
   mAnimationData.skinMatrices.resize(mAnimationData.animatedPosePalette.size());
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimationData.animatedPosePalette.size());
        i < size;
        ++i)
   {
      mAnimationData.skinMatrices[i] = mAnimationData.animatedPosePalette[i] * inverseBindPose[i];
   }

   // Skin the meshes on the CPU if that's the current skinning mode
   if (mAnimationData.currentSkinningMode == SkinningMode::CPU)
   {
      for (unsigned int i = 0, size = (unsigned int)mAnimatedMeshes.size(); i < size; ++i)
      {
         mAnimatedMeshes[i].SkinMeshOnTheCPU(mAnimationData.skinMatrices);
      }
   }

   // Update the skeleton viewer
   mSkeletonViewer.UpdateBones(mAnimationData.animatedPose, mAnimationData.animatedPosePalette);
}

void MovementState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mGameObject3DShader->use(true);
   mGameObject3DShader->setUniformMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());
   mGameObject3DShader->setUniformVec3("cameraPos", mCamera->getPosition());

   //mTable->render(*mGameObject3DShader);

   // Disable face culling so that we render the inside of the teapot
   //glDisable(GL_CULL_FACE);
   //mTeapot->render(*mGameObject3DShader);
   //glEnable(GL_CULL_FACE);

   mGameObject3DShader->use(false);

   if (mWireframeModeForMesh)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   // Render the animated meshes
   if (mAnimationData.currentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mModelTransform));
      mStaticMeshShader->setUniformMat4("view",       mCamera->getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
      //mStaticMeshShader->setUniformVec3("cameraPos",  mCamera->getPosition());
      mDiffuseTexture->bind(0, mStaticMeshShader->getUniformLocation("diffuseTex"));

      // Loop over the meshes and render each one
      for (unsigned int i = 0,
           size = static_cast<unsigned int>(mAnimatedMeshes.size());
           i < size;
           ++i)
      {
         mAnimatedMeshes[i].Render();
      }

      mDiffuseTexture->unbind(0);
      mStaticMeshShader->use(false);
   }
   else if (mAnimationData.currentSkinningMode == SkinningMode::GPU && mDisplayMesh)
   {
      mAnimatedMeshShader->use(true);
      mAnimatedMeshShader->setUniformMat4("model",            transformToMat4(mModelTransform));
      mAnimatedMeshShader->setUniformMat4("view",             mCamera->getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera->getPerspectiveProjectionMatrix());
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mAnimationData.skinMatrices);
      //mAnimatedMeshShader->setUniformVec3("cameraPos",        mCamera->getPosition());
      mDiffuseTexture->bind(0, mAnimatedMeshShader->getUniformLocation("diffuseTex"));

      // Loop over the meshes and render each one
      for (unsigned int i = 0,
           size = static_cast<unsigned int>(mAnimatedMeshes.size());
           i < size;
           ++i)
      {
         mAnimatedMeshes[i].Render();
      }

      mDiffuseTexture->unbind(0);
      mAnimatedMeshShader->use(false);
   }

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   if (!mPerformDepthTesting)
   {
      glDisable(GL_DEPTH_TEST);
   }

   glLineWidth(2.0f);

   // Render the bones
   if (mDisplayBones)
   {
      mSkeletonViewer.RenderBones(mModelTransform, mCamera->getPerspectiveProjectionViewMatrix());
   }

   glLineWidth(1.0f);

   if (mWireframeModeForJoints)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   // Render the joints
   if (mDisplayJoints)
   {
      mSkeletonViewer.RenderJoints(mModelTransform, mCamera->getPerspectiveProjectionViewMatrix(), mAnimationData.animatedPosePalette);
   }

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glEnable(GL_DEPTH_TEST);

   // --- --- ---

   mStaticMeshShader->use(true);
   mStaticMeshShader->setUniformMat4("model", glm::mat4(1.0f));
   mStaticMeshShader->setUniformMat4("view", mCamera->getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
   mGroundTexture->bind(0, mStaticMeshShader->getUniformLocation("diffuseTex"));

   // Loop over the ground meshes and render each one
   for (unsigned int i = 0,
      size = static_cast<unsigned int>(mGroundMeshes.size());
      i < size;
      ++i)
   {
      mGroundMeshes[i].Render();
   }

   mGroundTexture->unbind(0);
   mStaticMeshShader->use(false);

   // --- --- ---

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void MovementState::exit()
{

}

void MovementState::configureLights(const std::shared_ptr<Shader>& shader)
{
   shader->use(true);
   //shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(-5.0f, 5.0f, -5.0f));
   shader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[0].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   shader->setUniformVec3("pointLights[1].worldPos", glm::vec3(27.0f, 5.0f, 15.5f));
   shader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[1].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);
   shader->setUniformInt("numPointLightsInScene", 2);
   shader->use(false);
}

void MovementState::switchFromGPUToCPU()
{
   int positionsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("texCoord");
   int weightsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("weights");
   int influencesAttribLocOfAnimatedShader = mAnimatedMeshShader->getAttributeLocation("joints");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].UnconfigureVAO(positionsAttribLocOfAnimatedShader,
                                        normalsAttribLocOfAnimatedShader,
                                        texCoordsAttribLocOfAnimatedShader,
                                        weightsAttribLocOfAnimatedShader,
                                        influencesAttribLocOfAnimatedShader);
   }

   int positionsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader   = mStaticMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("texCoord");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].ConfigureVAO(positionsAttribLocOfStaticShader,
                                      normalsAttribLocOfStaticShader,
                                      texCoordsAttribLocOfStaticShader,
                                      -1,
                                      -1);
   }
}

void MovementState::switchFromCPUToGPU()
{
   int positionsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader   = mStaticMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader = mStaticMeshShader->getAttributeLocation("texCoord");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].UnconfigureVAO(positionsAttribLocOfStaticShader,
                                        normalsAttribLocOfStaticShader,
                                        texCoordsAttribLocOfStaticShader,
                                        -1,
                                        -1);
   }

   int positionsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("texCoord");
   int weightsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("weights");
   int influencesAttribLocOfAnimatedShader = mAnimatedMeshShader->getAttributeLocation("joints");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].ConfigureVAO(positionsAttribLocOfAnimatedShader,
                                      normalsAttribLocOfAnimatedShader,
                                      texCoordsAttribLocOfAnimatedShader,
                                      weightsAttribLocOfAnimatedShader,
                                      influencesAttribLocOfAnimatedShader);
   }

   // TODO: This is inefficient. We just need to load the positions and normals.
   // Load the original positions and normals because the CPU skinning algorithm
   // has been modifying them every frame
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].LoadBuffers();
   }
}

void MovementState::userInterface()
{
   ImGui::Begin("Animation Controller"); // Create a window called "Animation Controller"

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

   ImGui::Combo("Clip", &mSelectedClip, mClipNames.c_str());

   ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

   ImGui::Checkbox("Display Mesh", &mDisplayMesh);

   ImGui::Checkbox("Display Bones", &mDisplayBones);

   ImGui::Checkbox("Display Joints", &mDisplayJoints);

   ImGui::Checkbox("Wireframe Mode for Mesh", &mWireframeModeForMesh);

   ImGui::Checkbox("Wireframe Mode for Joints", &mWireframeModeForJoints);

   ImGui::Checkbox("Perform Depth Testing", &mPerformDepthTesting);

   ImGui::End();
}

void MovementState::resetScene()
{

}

void MovementState::resetCamera()
{
   mCamera->reposition(glm::vec3(0.0f, 7.0f, 9.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       0.0f,
                       -30.0f,
                       45.0f);
}
