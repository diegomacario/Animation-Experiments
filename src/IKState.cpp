#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

#include "resource_manager.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "Intersection.h"
#include "Blending.h"
#include "IKState.h"

#ifdef USE_THIRD_PERSON_CAMERA
IKState::IKState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                 const std::shared_ptr<Window>&             window)
#else
IKState::IKState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                 const std::shared_ptr<Window>&             window,
                 const std::shared_ptr<Camera>&             camera)
#endif
   : mFSM(finiteStateMachine)
   , mWindow(window)
#ifdef USE_THIRD_PERSON_CAMERA
   , mCamera3(12.0f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 40.0f, 2.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 130.0f, 0.25f)
#else
   , mCamera(camera)
#endif
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/diffuse_illumination.frag");
   configureLights(mAnimatedMeshShader);

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/diffuse_illumination.frag");
   configureLights(mStaticMeshShader);

   // Load the diffuse texture of the animated character
   mDiffuseTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/woman/woman.png");

   // Load the animated character
   cgltf_data* data        = LoadGLTFFile("resources/models/woman/woman.gltf");
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

   // Load the ground
   data = LoadGLTFFile("resources/models/ground/catwalk.gltf");
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
   mGroundTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/ground/catwalk_uv.png");

   // Get the triangles that make up the ground
   mGroundTriangles = GetTrianglesFromMeshes(mGroundMeshes);

   // Compose the motion track, which tells the character where to walk by supplying X and Z coordinates
   mMotionTrack.SetInterpolation(Interpolation::Linear);
   mMotionTrack.SetNumberOfFrames(5);
   // Frame 0
   mMotionTrack.GetFrame(0).mTime     = 0.0f;
   mMotionTrack.GetFrame(0).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(0).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(0).mValue[2] = 10.0f;
   // Frame 1
   mMotionTrack.GetFrame(1).mTime     = 4.82f;
   mMotionTrack.GetFrame(1).mValue[0] = 53.0f;
   mMotionTrack.GetFrame(1).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(1).mValue[2] = 10.0f;
   // Frame 2
   mMotionTrack.GetFrame(2).mTime     = 5.57f;
   mMotionTrack.GetFrame(2).mValue[0] = 53.0f;
   mMotionTrack.GetFrame(2).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(2).mValue[2] = 2.0f;
   // Frame 3
   mMotionTrack.GetFrame(3).mTime     = 10.39f;
   mMotionTrack.GetFrame(3).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(3).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(3).mValue[2] = 1.0f;
   // Frame 4
   mMotionTrack.GetFrame(4).mTime     = 11.14f;
   mMotionTrack.GetFrame(4).mValue[0] = 0.0f;
   mMotionTrack.GetFrame(4).mValue[1] = 0.0f;
   mMotionTrack.GetFrame(4).mValue[2] = 10.0f;

   // Create the IK legs
   mLeftLeg = IKLeg(mSkeleton, "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase");
   mLeftLeg.SetAnkleOffset(0.2f);  // The left ankle is 0.2 units above the ground
   mRightLeg = IKLeg(mSkeleton, "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase");
   mRightLeg.SetAnkleOffset(0.2f); // The right ankle is 0.2 units above the ground

   // Compose the pin track of the left foot, which tells us when the left foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   mLeftFootPinTrack.SetInterpolation(Interpolation::Cubic);
   mLeftFootPinTrack.SetNumberOfFrames(4);
   // Frame 0
   mLeftFootPinTrack.GetFrame(0).mTime     = 0.0f;
   mLeftFootPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   mLeftFootPinTrack.GetFrame(1).mTime     = 0.4f;
   mLeftFootPinTrack.GetFrame(1).mValue[0] = 1;
   // Frame 2
   mLeftFootPinTrack.GetFrame(2).mTime     = 0.6f;
   mLeftFootPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   mLeftFootPinTrack.GetFrame(3).mTime     = 1.0f;
   mLeftFootPinTrack.GetFrame(3).mValue[0] = 0;

   // Compose the pin track of the right foot, which tells us when the right foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   mRightFootPinTrack.SetInterpolation(Interpolation::Cubic);
   mRightFootPinTrack.SetNumberOfFrames(4);
   // Frame 0
   mRightFootPinTrack.GetFrame(0).mTime     = 0.0f;
   mRightFootPinTrack.GetFrame(0).mValue[0] = 1;
   // Frame 1
   mRightFootPinTrack.GetFrame(1).mTime     = 0.3f;
   mRightFootPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   mRightFootPinTrack.GetFrame(2).mTime     = 0.7f;
   mRightFootPinTrack.GetFrame(2).mValue[0] = 0;
   // Frame 3
   mRightFootPinTrack.GetFrame(3).mTime     = 1.0f;
   mRightFootPinTrack.GetFrame(3).mValue[0] = 1;

   // Initialize the values we use to describe the position of the character
   mHeightOfOriginOfYPositionRay = 11.0f;
   mSinkIntoGround               = 0.15f;
   mMotionTrackPlaybackSpeed     = 0.3f;
   mMotionTrackDuration          = mMotionTrack.GetEndTime() - mMotionTrack.GetStartTime();
   mMotionTrackFutureTimeOffset  = 0.1f;
   mHeightOfHip                  = 2.0f;
   mHeightOfKnees                = 1.0f;
   mDistanceFromAnkleToToe       = 0.3f;

   initializeState();

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mAnimationData.animatedPose);
}

void IKState::initializeState()
{
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
#ifndef __EMSCRIPTEN__
   mWireframeModeForCharacter = false;
   mWireframeModeForJoints = false;
   mWireframeModeForTerrain = false;
   mPerformDepthTesting = true;
#endif
   // Set the initial IK options
   mSolveWithConstraints = true;
   mSelectedNumberOfIterations = 15;

   // Set the initial pose
   mAnimationData.animatedPose = mSkeleton.GetRestPose();

   mModelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::lookRotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0)), glm::vec3(1.0f));
   mPreviousYPositionOfCharacter = 0.0f;
   mMotionTrackTime = 0.0f;

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

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void IKState::enter()
{
   // Set the current state
   mSelectedState = 2;
   initializeState();
   resetCamera();
   resetScene();
}

void IKState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Change the state
   if (mSelectedState != 2)
   {
      switch (mSelectedState)
      {
      case 0:
         mFSM->changeState("viewer");
         break;
      case 1:
         mFSM->changeState("movement");
         break;
      case 3:
         mFSM->changeState("ik_movement");
         break;
      }
   }

#ifndef __EMSCRIPTEN__
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
#ifndef USE_THIRD_PERSON_CAMERA
         if (mCamera->isFree())
         {
            // Disable the cursor when fullscreen with a free camera
            mWindow->enableCursor(false);
            // Going from windowed to fullscreen changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
#endif
      }
      else if (!mWindow->isFullScreen())
      {
#ifndef USE_THIRD_PERSON_CAMERA
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
#endif
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
#endif

   // Reset the camera
   if (mWindow->keyIsPressed(GLFW_KEY_R)) { resetCamera(); }

#ifdef USE_THIRD_PERSON_CAMERA
   // Orient the camera
   if (mWindow->mouseMoved() && mWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
   {
      mCamera3.processMouseMovement(mWindow->getCursorXOffset(), mWindow->getCursorYOffset());
      mWindow->resetMouseMoved();
   }

   // Adjust the distance between the player and the camera
   if (mWindow->scrollWheelMoved())
   {
      mCamera3.processScrollWheelMovement(mWindow->getScrollYOffset());
      mWindow->resetScrollWheelMoved();
   }
#else
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
#endif
}

void IKState::update(float deltaTime)
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
   if (mMotionTrackTime > mMotionTrackDuration)
   {
      mMotionTrackTime -= mMotionTrackDuration;
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
   float leftLegPinTrackValue   = mLeftFootPinTrack.Sample(normalizedPlaybackTime, true);
   float rightLegPinTrackValue  = mRightFootPinTrack.Sample(normalizedPlaybackTime, true);

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

#ifdef USE_THIRD_PERSON_CAMERA
   mCamera3.processPlayerMovement(mModelTransform.position, mModelTransform.rotation);
#endif

   // Interpolate between the current world position of the left ankle and its ground IK target based on the value of the pin track
   // If the pin track says that the foot should be on the ground, then the ground IK target will be favored
   // If the pin track says that the foot should not be on the ground, the the world position of the ankle, which is given by the animation clip, will be favored
   // Note that if we detected that there's ground above the ankle, worldPosOfLeftAnkle and leftAnkleGroundIKTarget will be the same, which means that the foot
   // will be on the ground regardless of what the pin track says
   worldPosOfLeftAnkle  = glm::lerp(worldPosOfLeftAnkle, leftAnkleGroundIKTarget, leftLegPinTrackValue);
   // Do the same for the right ankle
   worldPosOfRightAnkle = glm::lerp(worldPosOfRightAnkle, rightAnkleGroundIKTarget, rightLegPinTrackValue);

   // Solve the IK chains of the left and right legs so that their end effectors (ankles) are at the positions we interpolated above
   mLeftLeg.Solve(mModelTransform, mAnimationData.animatedPose, worldPosOfLeftAnkle, mSolveWithConstraints, mSelectedNumberOfIterations);
   mRightLeg.Solve(mModelTransform, mAnimationData.animatedPose, worldPosOfRightAnkle, mSolveWithConstraints, mSelectedNumberOfIterations);

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

void IKState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

#ifdef __EMSCRIPTEN__
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
   mWindow->clearAndBindMultisampleFramebuffer();
#endif

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

#ifndef __EMSCRIPTEN__
   if (mWireframeModeForTerrain)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   mStaticMeshShader->use(true);
   mStaticMeshShader->setUniformMat4("model",      glm::mat4(1.0f));
#ifdef USE_THIRD_PERSON_CAMERA
   mStaticMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
#else
   mStaticMeshShader->setUniformMat4("view",       mCamera->getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
#endif
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

#ifndef __EMSCRIPTEN__
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   if (mWireframeModeForCharacter)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the animated meshes
   if (mAnimationData.currentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mModelTransform));
#ifdef USE_THIRD_PERSON_CAMERA
      mStaticMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
#else
      mStaticMeshShader->setUniformMat4("view",       mCamera->getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
#endif
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
#ifdef USE_THIRD_PERSON_CAMERA
      mAnimatedMeshShader->setUniformMat4("view",             mCamera3.getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera3.getPerspectiveProjectionMatrix());
#else
      mAnimatedMeshShader->setUniformMat4("view",             mCamera->getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera->getPerspectiveProjectionMatrix());
#endif
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mAnimationData.skinMatrices);
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

#ifdef __EMSCRIPTEN__
   glDisable(GL_DEPTH_TEST);
#else
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   if (!mPerformDepthTesting)
   {
      glDisable(GL_DEPTH_TEST);
   }
#endif

   glLineWidth(2.0f);

   // Render the bones
   if (mDisplayBones)
   {
#ifdef USE_THIRD_PERSON_CAMERA
      mSkeletonViewer.RenderBones(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix());
#else
      mSkeletonViewer.RenderBones(mModelTransform, mCamera->getPerspectiveProjectionViewMatrix());
#endif
   }

   glLineWidth(1.0f);

#ifndef __EMSCRIPTEN__
   if (mWireframeModeForJoints)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the joints
   if (mDisplayJoints)
   {
#ifdef USE_THIRD_PERSON_CAMERA
      mSkeletonViewer.RenderJoints(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix(), mAnimationData.animatedPosePalette);
#else
      mSkeletonViewer.RenderJoints(mModelTransform, mCamera->getPerspectiveProjectionViewMatrix(), mAnimationData.animatedPosePalette);
#endif
   }

#ifndef __EMSCRIPTEN__
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
   glEnable(GL_DEPTH_TEST);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef __EMSCRIPTEN__
   mWindow->generateAntiAliasedImage();
#endif

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void IKState::exit()
{

}

void IKState::configureLights(const std::shared_ptr<Shader>& shader)
{
   shader->use(true);

   // To the right of bridge
   shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(28.0f, 20.0f, 20.0f));
   shader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[0].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);

   // Under bridge
   shader->setUniformVec3("pointLights[1].worldPos", glm::vec3(28.0f, 3.0f, 5.0f));
   shader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[1].constantAtt", 3.0f);
   shader->setUniformFloat("pointLights[1].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);

   // To the left of bridge
   shader->setUniformVec3("pointLights[2].worldPos", glm::vec3(28.0f, 3.0f, -15.0f));
   shader->setUniformVec3("pointLights[2].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[2].constantAtt", 5.0f);
   shader->setUniformFloat("pointLights[2].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[2].quadraticAtt", 0.0f);

   // Starting point
   shader->setUniformVec3("pointLights[3].worldPos", glm::vec3(-5.0f, 10.0f, 5.0f));
   shader->setUniformVec3("pointLights[3].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[3].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[3].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[3].quadraticAtt", 0.01f);

   // Far side
   shader->setUniformVec3("pointLights[4].worldPos", glm::vec3(58.0f, 10.0f, 6.0f));
   shader->setUniformVec3("pointLights[4].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[4].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[4].linearAtt", 0.00001f);
   shader->setUniformFloat("pointLights[4].quadraticAtt", 0.01f);

   shader->setUniformInt("numPointLightsInScene", 5);
   shader->use(false);
}

void IKState::switchFromGPUToCPU()
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

void IKState::switchFromCPUToGPU()
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

void IKState::userInterface()
{
   ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Appearing);

   ImGui::Begin("Programmed IK Movement", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("State", &mSelectedState, "Model Viewer\0Flat Movement\0Programmed IK Movement\0IK Movement\0");

   ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

   //ImGui::Combo("Clip", &mSelectedClip, mClipNames.c_str());

   ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

   float durationOfCurrClip = mClips[mAnimationData.currentClipIndex].GetDuration();
   char progress[32];
   snprintf(progress, 32, "%.3f / %.3f", mAnimationData.playbackTime, durationOfCurrClip);
   ImGui::ProgressBar(mAnimationData.playbackTime / durationOfCurrClip, ImVec2(0.0f, 0.0f), progress);

   ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
   ImGui::Text("Playback Time");

   ImGui::Checkbox("Display Skin", &mDisplayMesh);

   ImGui::Checkbox("Display Bones", &mDisplayBones);

   ImGui::Checkbox("Display Joints", &mDisplayJoints);

#ifndef __EMSCRIPTEN__
   ImGui::Checkbox("Wireframe Mode for Skin", &mWireframeModeForCharacter);

   ImGui::Checkbox("Wireframe Mode for Joints", &mWireframeModeForJoints);

   ImGui::Checkbox("Wireframe Mode for Terrain", &mWireframeModeForTerrain);

   ImGui::Checkbox("Perform Depth Testing", &mPerformDepthTesting);
#endif

   ImGui::Checkbox("Solve with Constraints", &mSolveWithConstraints);

   ImGui::SliderInt("IK Iterations", &mSelectedNumberOfIterations, 0, 100);

   ImGui::End();
}

void IKState::resetScene()
{

}

void IKState::resetCamera()
{
#ifdef USE_THIRD_PERSON_CAMERA
   mCamera3.reposition(12.0f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 40.0f, 0.0f, 90.0f);
   mCamera3.processMouseMovement(-90.0f / 0.25f, 0.0f);
#else
   mCamera->reposition(glm::vec3(9.94739f, 12.5202f, 30.2262f),
                       glm::vec3(10.0207f, 8.6164f, 21.0199f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       45.0f);
#endif
}
