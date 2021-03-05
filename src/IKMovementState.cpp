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
#include "IKMovementState.h"

IKMovementState::IKMovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                                 const std::shared_ptr<Window>&             window)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera3(8.0f, 15.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 90.0f, 0.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 500.0f, 0.25f)
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/diffuse_and_scaled_emissive_illumination_same_tex.frag");
   // Sunset color for the skin
   configureLights(mAnimatedMeshShader, glm::vec3(1.0f, 0.252f, 0.039f));

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/diffuse_and_scaled_emissive_illumination_with_scaled_uvs.frag");
   // Neutral color for the terrain
   configureLights(mStaticMeshShader, glm::vec3(0.5f, 0.5f, 0.5f));

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
   for (unsigned int clipIndex = 0,
        numClips = static_cast<unsigned int>(clips.size());
        clipIndex < numClips;
        ++clipIndex)
   {
      FastClip currClip = OptimizeClip(clips[clipIndex]);
      RearrangeFastClip(currClip, jointMap);
      mClips.insert(std::make_pair(currClip.GetName(), currClip));
   }

   mClips["Jump"].SetLooping(false);
   mClips["Jump2"].SetLooping(false);

   // Configure the pin tracks for all the clips
   configurePinTracks();

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
   mGroundDiffuseTexture  = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/ground/grass.png");
   mGroundEmissiveTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/ground/lights.png");

   // Get the triangles that make up the ground
   mGroundTriangles = GetTrianglesFromMeshes(mGroundMeshes);

   // Initialize the values we use to describe the position of the character
   mHeightOfOriginOfYPositionRay = 100.0f;
   mSinkIntoGround               = 0.15f;
   mHeightOfHip                  = 2.0f;
   mHeightOfKnees                = 1.0f;
   mDistanceFromAnkleToToe       = 0.3f;
   mAnkleVerticalOffset          = 0.2f;

   // Create the IK legs
   mLeftLeg = IKLeg(mSkeleton, "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase");
   mLeftLeg.SetAnkleOffset(mAnkleVerticalOffset);  // The left ankle is 0.2 units above the ground
   mRightLeg = IKLeg(mSkeleton, "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase");
   mRightLeg.SetAnkleOffset(mAnkleVerticalOffset); // The right ankle is 0.2 units above the ground

   initializeState();

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mIKCrossFadeController.GetCurrentPose());
}

void IKMovementState::initializeState()
{
   // Set the state values
   mIsWalking = false;
   mIsRunning = false;
   mIsInAir = false;
   mJumpingWhileIdle = false;
   mJumpingWhileWalking = false;
   mJumpingWhileRunning = false;

   // Set the initial clip and initialize the crossfade controller
   mIKCrossFadeController.SetSkeleton(mSkeleton);
   mIKCrossFadeController.Play(&mClips["Idle"], &mLeftFootPinTracks["Idle"], &mRightFootPinTracks["Idle"], false);
   mIKCrossFadeController.Update(0.0f);
   mIKCrossFadeController.GetCurrentPose().GetMatrixPalette(mPosePalette);

   // Set the initial skinning mode
   mSelectedSkinningMode = SkinningMode::GPU;
   // Set the initial playback speed
   mSelectedPlaybackSpeed = 1.0f;
   // Set the initial rendering options
   mDisplayMesh = true;
   mDisplayBones = false;
   mDisplayJoints = false;
   mWireframeModeForCharacter = false;
   mWireframeModeForJoints = false;
   mWireframeModeForTerrain = false;
#ifdef __EMSCRIPTEN__
   mPerformDepthTesting = false;
#else
   mPerformDepthTesting = true;
#endif
   // Set the initial IK options
   mSolveWithConstraints = true;
   mSelectedNumberOfIterations = 15;
   // Set the initial beauty pass options
   mSelectedEmissiveTextureBrightnessScaleFactor = 1.0f;
   mSelectedEmissiveTextureUVScaleFactor = 10.0f;

   // Set the model transform
   mModelTransform = Transform(glm::vec3(81.9238f, 0.0f, -117.334f), Q::normalized(Q::quat(0.0f, -0.130078f, 0.0f, 0.991504f)), glm::vec3(1.0f));
   mPreviousYPositionOfCharacter = 0.0f;

   // Shoot a ray downwards to determine the initial Y position of the character,
   // and sink it into the ground a little so that the IK solver has room to work
   determineYPosition();
   mPreviousYPositionOfCharacter = mModelTransform.position.y;

   // Light blue the sky
   //glClearColor(0.036f, 0.827f, 1.0f, 1.0f);
   // Dark blue for the sky
   glClearColor(0.036f, 0.627f, 1.0f, 1.0f);
}

void IKMovementState::enter()
{
   // Set the current state
   mSelectedState = 3;
   initializeState();
   resetCamera();
   resetScene();
}

void IKMovementState::processInput(float deltaTime)
{
   deltaTime *= mSelectedPlaybackSpeed;

   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Change the state
   if (mSelectedState != 3)
   {
      switch (mSelectedState)
      {
      case 0:
         mFSM->changeState("viewer");
         break;
      case 1:
         mFSM->changeState("movement");
         break;
      case 2:
         mFSM->changeState("ik");
         break;
      }
   }

#ifndef __EMSCRIPTEN__
   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());
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

   // --- --- ---

   bool movementKeyPressed  = false;
   bool leftShiftKeyPressed = mWindow->keyIsPressed(GLFW_KEY_LEFT_SHIFT);

   float movementSpeed = mCharacterWalkingSpeed;
   float rotationSpeed = mCharacterWalkingRotationSpeed;
   if ((leftShiftKeyPressed && !mJumpingWhileWalking) || mJumpingWhileRunning)
   {
      movementSpeed = mCharacterRunningSpeed;
      rotationSpeed = mCharacterRunningRotationSpeed;
   }

   if (!mJumpingWhileIdle)
   {
      // Move and orient the character
      if (mWindow->keyIsPressed(GLFW_KEY_A))
      {
         float degreesToRotate = rotationSpeed * deltaTime;
         Q::quat rotation = Q::angleAxis(glm::radians(degreesToRotate), glm::vec3(0.0f, 1.0f, 0.0f));
         mModelTransform.rotation = Q::normalized(mModelTransform.rotation * rotation);
         movementKeyPressed = true;
      }

      if (mWindow->keyIsPressed(GLFW_KEY_D))
      {
         float degreesToRotate = rotationSpeed * deltaTime;
         Q::quat rotation = Q::angleAxis(glm::radians(-degreesToRotate), glm::vec3(0.0f, 1.0f, 0.0f));
         mModelTransform.rotation = Q::normalized(mModelTransform.rotation * rotation);
         movementKeyPressed = true;
      }

      if (mWindow->keyIsPressed(GLFW_KEY_W))
      {
         float distToMove = movementSpeed * deltaTime;
         glm::vec3 characterFwd = mModelTransform.rotation * glm::vec3(0.0f, 0.0f, 1.0f);
         mModelTransform.position += characterFwd * distToMove;
         movementKeyPressed = true;
      }

      if (mWindow->keyIsPressed(GLFW_KEY_S))
      {
         float distToMove = movementSpeed * deltaTime;
         glm::vec3 characterFwd = mModelTransform.rotation * glm::vec3(0.0f, 0.0f, 1.0f);
         mModelTransform.position -= characterFwd * distToMove;
         movementKeyPressed = true;
      }
   }

   if (mWindow->keyIsPressed(GLFW_KEY_SPACE) && !mIsInAir)
   {
      mIKCrossFadeController.ClearTargets();

      if (mIsWalking)
      {
         mIKCrossFadeController.FadeTo(&mClips["Jump2"], &mLeftFootPinTracks["Jump2"], &mRightFootPinTracks["Jump2"], 0.1f, true);
         mJumpingWhileWalking = true;
      }
      else if (mIsRunning)
      {
         mIKCrossFadeController.FadeTo(&mClips["Jump2"], &mLeftFootPinTracks["Jump2"], &mRightFootPinTracks["Jump2"], 0.1f, true);
         mJumpingWhileRunning = true;
      }
      else
      {
         mIKCrossFadeController.FadeTo(&mClips["Jump"], &mLeftFootPinTracks["Jump"], &mRightFootPinTracks["Jump"], 0.1f, true);
         mJumpingWhileIdle = true;
      }

      mIsInAir = true;
   }

   if (mIsInAir)
   {
      if (mIKCrossFadeController.IsLocked() && mIKCrossFadeController.IsCurrentClipFinished())
      {
         mIKCrossFadeController.Unlock();

         if (mIsWalking)
         {
            mIKCrossFadeController.FadeTo(&mClips["Walking"], &mLeftFootPinTracks["Walking"], &mRightFootPinTracks["Walking"], 0.15f, false);
            mJumpingWhileWalking = false;
         }
         else if (mIsRunning)
         {
            mIKCrossFadeController.FadeTo(&mClips["Running"], &mLeftFootPinTracks["Running"], &mRightFootPinTracks["Running"], 0.15f, false);
            mJumpingWhileRunning = false;
         }
         else
         {
            mIKCrossFadeController.FadeTo(&mClips["Idle"], &mLeftFootPinTracks["Idle"], &mRightFootPinTracks["Idle"], 0.1f, false);
            mJumpingWhileIdle = false;
         }

         mIsInAir = false;
      }
   }

   if (movementKeyPressed && !mIsInAir)
   {
      if (!mIsWalking && !mIsRunning)
      {
         if (leftShiftKeyPressed)
         {
            mIsRunning = true;
            mIKCrossFadeController.FadeTo(&mClips["Running"], &mLeftFootPinTracks["Running"], &mRightFootPinTracks["Running"], 0.25f, false);
         }
         else
         {
            mIsWalking = true;
            mIKCrossFadeController.FadeTo(&mClips["Walking"], &mLeftFootPinTracks["Walking"], &mRightFootPinTracks["Walking"], 0.25f, false);
         }
      }
      else if (mIsWalking)
      {
         if (leftShiftKeyPressed)
         {
            mIsWalking = false;
            mIsRunning = true;
            mIKCrossFadeController.FadeTo(&mClips["Running"], &mLeftFootPinTracks["Running"], &mRightFootPinTracks["Running"], 0.25f, false);
         }
      }
      else if (mIsRunning)
      {
         if (!leftShiftKeyPressed)
         {
            mIsRunning = false;
            mIsWalking = true;
            mIKCrossFadeController.FadeTo(&mClips["Walking"], &mLeftFootPinTracks["Walking"], &mRightFootPinTracks["Walking"], 0.25f, false);
         }
      }
   }
   else if (!mIsInAir)
   {
      if (mIsWalking || mIsRunning)
      {
         mIsRunning = false;
         mIsWalking = false;
         mIKCrossFadeController.FadeTo(&mClips["Idle"], &mLeftFootPinTracks["Idle"], &mRightFootPinTracks["Idle"], 0.25f, false);
      }
   }
}

void IKMovementState::update(float deltaTime)
{
   deltaTime *= mSelectedPlaybackSpeed;

   // --- --- ---

   // Position Sampling and Height Correction
   // **********************************************************************************************************************************************

   // Shoot a ray downwards to determine the initial Y position of the character,
   // and sink it into the ground a little so that the IK solver has room to work
   determineYPosition();

   // --- --- ---

   if (mCurrentSkinningMode != mSelectedSkinningMode)
   {
      if (mCurrentSkinningMode == SkinningMode::GPU)
      {
         switchFromGPUToCPU();
      }
      else if (mCurrentSkinningMode == SkinningMode::CPU)
      {
         switchFromCPUToGPU();
      }

      mCurrentSkinningMode = static_cast<SkinningMode>(mSelectedSkinningMode);
   }

   // Ask the crossfade controller to sample the current clip and fade with the next one if necessary
   mIKCrossFadeController.Update(deltaTime);

   // --- --- ---

   // Ankle Correction
   // **********************************************************************************************************************************************

   FastClip* currClip = mIKCrossFadeController.GetCurrentClip();
   Pose&     currPose = mIKCrossFadeController.GetCurrentPose();

   // The keyframes of the pin tracks are set in normalized time, so they must be sampled with the normalized time
   float leftFootPinTrackValue   = mIKCrossFadeController.GetCurrentLeftFootPinTrackValue();
   float rightFootPinTrackValue  = mIKCrossFadeController.GetCurrentRightFootPinTrackValue();

   // Calculate the world positions of the left and right ankles
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   glm::vec3 worldPosOfLeftAnkle  = combine(mModelTransform, currPose.GetGlobalTransform(mLeftLeg.GetAnkleIndex())).position;
   glm::vec3 worldPosOfRightAnkle = combine(mModelTransform, currPose.GetGlobalTransform(mRightLeg.GetAnkleIndex())).position;

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
   glm::vec3 hitPoint;
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

   mCamera3.processPlayerMovement(mModelTransform.position, mModelTransform.rotation);

   // Interpolate between the current world position of the left ankle and its ground IK target based on the value of the pin track
   // If the pin track says that the foot should be on the ground, then the ground IK target will be favored
   // If the pin track says that the foot should not be on the ground, the the world position of the ankle, which is given by the animation clip, will be favored
   // Note that if we detected that there's ground above the ankle, worldPosOfLeftAnkle and leftAnkleGroundIKTarget will be the same, which means that the foot
   // will be on the ground regardless of what the pin track says
   mLeftAnkleFinalTarget = glm::lerp(worldPosOfLeftAnkle, leftAnkleGroundIKTarget, leftFootPinTrackValue);
   // Do the same for the right ankle
   mRightAnkleFinalTarget = glm::lerp(worldPosOfRightAnkle, rightAnkleGroundIKTarget, rightFootPinTrackValue);

   // Solve the IK chains of the left and right legs so that their end effectors (ankles) are at the positions we interpolated above
   mLeftLeg.Solve(mModelTransform, currPose, mLeftAnkleFinalTarget, mSolveWithConstraints, mSelectedNumberOfIterations);
   mRightLeg.Solve(mModelTransform, currPose, mRightAnkleFinalTarget, mSolveWithConstraints, mSelectedNumberOfIterations);

   // Blend the resulting IK chains into the animated pose
   // Note how the blend factor is equal to 1.0f
   // We want the legs of the animated pose to be equal to the IK chains
   Blend(currPose, mLeftLeg.GetAdjustedPose(), 1.0f, mLeftLeg.GetHipIndex(), currPose);
   Blend(currPose, mRightLeg.GetAdjustedPose(), 1.0f, mRightLeg.GetHipIndex(), currPose);

   // Toe Correction
   // **********************************************************************************************************************************************

   // Calculate the world transforms of the final ankles
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   Transform worldTransfOfLeftAnkle  = combine(mModelTransform, currPose.GetGlobalTransform(mLeftLeg.GetAnkleIndex()));
   Transform worldTransfOfRightAnkle = combine(mModelTransform, currPose.GetGlobalTransform(mRightLeg.GetAnkleIndex()));

   // Calculate the world positions of the toes
   // We do this by combining the model transform of the character (mModelTransform) with the global transforms of the joints
   // Note the parent-child order here, which makes sense
   glm::vec3 worldPosOfLeftToe = combine(mModelTransform, currPose.GetGlobalTransform(mLeftLeg.GetToeIndex())).position;
   glm::vec3 worldPosOfRightToe = combine(mModelTransform, currPose.GetGlobalTransform(mRightLeg.GetToeIndex())).position;

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
   Ray rightToeRay(originOfRightToeRay, glm::vec3(0.0f, -1.0f, 0.0f));

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
   newWorldPosOfLeftToe = glm::lerp(newWorldPosOfLeftToe, leftToeGroundIKTarget, leftFootPinTrackValue);
   // Do the same for the right toes
   newWorldPosOfRightToe = glm::lerp(newWorldPosOfRightToe, rightToeGroundIKTarget, rightFootPinTrackValue);

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
      Transform localTransfOfLeftAnkle = currPose.GetLocalTransform(mLeftLeg.GetAnkleIndex());
      // Multiply newLocalRotOfLeftAnkle by the old local rotation of the left ankle to get:
      // localTransfOfLeftAnkle * newLocalRotOfLeftAnkle = A_old * A_old^-1 * A_new = A_new
      localTransfOfLeftAnkle.rotation = newLocalRotOfLeftAnkle * localTransfOfLeftAnkle.rotation;
      // Update the local transform of the left ankle in the pose
      currPose.SetLocalTransform(mLeftLeg.GetAnkleIndex(), localTransfOfLeftAnkle);
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
      Transform localTransfOfRightAnkle = currPose.GetLocalTransform(mRightLeg.GetAnkleIndex());
      // Multiply newLocalRotOfRightAnkle by the old local rotation of the right ankle to get:
      // localTransfOfRightAnkle * newLocalRotOfRightAnkle = A_old * A_old^-1 * A_new = A_new
      localTransfOfRightAnkle.rotation = newLocalRotOfRightAnkle * localTransfOfRightAnkle.rotation;
      // Update the local transform of the right ankle in the pose
      currPose.SetLocalTransform(mRightLeg.GetAnkleIndex(), localTransfOfRightAnkle);
   }

   // --- --- ---

   // Get the palette of the animated pose
   currPose.GetMatrixPalette(mPosePalette);

   std::vector<glm::mat4>& inverseBindPose = mSkeleton.GetInvBindPose();

   // Generate the skin matrices
   mSkinMatrices.resize(mPosePalette.size());
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mPosePalette.size());
        i < size;
        ++i)
   {
      mSkinMatrices[i] = mPosePalette[i] * inverseBindPose[i];
   }

   // Skin the meshes on the CPU if that's the current skinning mode
   if (mCurrentSkinningMode == SkinningMode::CPU)
   {
      for (unsigned int i = 0, size = (unsigned int)mAnimatedMeshes.size(); i < size; ++i)
      {
         mAnimatedMeshes[i].SkinMeshOnTheCPU(mSkinMatrices);
      }
   }

   // Update the skeleton viewer
   mSkeletonViewer.UpdateBones(currPose, mPosePalette);
}

void IKMovementState::render()
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
   mStaticMeshShader->setUniformMat4("model", glm::mat4(1.0f));
   mStaticMeshShader->setUniformMat4("view", mCamera3.getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
   mStaticMeshShader->setUniformFloat("emissiveTextureBrightnessScaleFactor", mSelectedEmissiveTextureBrightnessScaleFactor);
   mStaticMeshShader->setUniformFloat("emissiveTextureUVScaleFactor", mSelectedEmissiveTextureUVScaleFactor);
   mGroundDiffuseTexture->bind(0, mStaticMeshShader->getUniformLocation("diffuseTex"));
   mGroundEmissiveTexture->bind(1, mStaticMeshShader->getUniformLocation("emissiveTex"));

   // Loop over the ground meshes and render each one
   for (unsigned int i = 0,
      size = static_cast<unsigned int>(mGroundMeshes.size());
      i < size;
      ++i)
   {
      mGroundMeshes[i].Render();
   }

   mGroundDiffuseTexture->unbind(0);
   mGroundEmissiveTexture->unbind(1);
   mStaticMeshShader->use(false);

#ifndef __EMSCRIPTEN__
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   if (mWireframeModeForCharacter)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the animated meshes
   if (mCurrentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mModelTransform));
      mStaticMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
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
   else if (mCurrentSkinningMode == SkinningMode::GPU && mDisplayMesh)
   {
      mAnimatedMeshShader->use(true);
      mAnimatedMeshShader->setUniformMat4("model",            transformToMat4(mModelTransform));
      mAnimatedMeshShader->setUniformMat4("view",             mCamera3.getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera3.getPerspectiveProjectionMatrix());
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mSkinMatrices);
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

#ifndef __EMSCRIPTEN__
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

   if (!mPerformDepthTesting)
   {
      glDisable(GL_DEPTH_TEST);
   }

   glLineWidth(2.0f);

   // Render the bones
   if (mDisplayBones)
   {
      mSkeletonViewer.RenderBones(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix());
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
      mSkeletonViewer.RenderJoints(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix(), mPosePalette);
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

void IKMovementState::exit()
{

}

void IKMovementState::configureLights(const std::shared_ptr<Shader>& shader, const glm::vec3& lightColor)
{
   shader->use(true);
   shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 100.0f, 100.0f));
   shader->setUniformVec3("pointLights[0].color", lightColor);
   shader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[0].linearAtt", 0.002f);
   shader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);

   shader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 100.0f, -100.0f));
   shader->setUniformVec3("pointLights[1].color", lightColor);
   shader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[1].linearAtt", 0.002f);
   shader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);

   shader->setUniformInt("numPointLightsInScene", 2);
   shader->use(false);
}

void IKMovementState::switchFromGPUToCPU()
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

void IKMovementState::switchFromCPUToGPU()
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

void IKMovementState::userInterface()
{
   ImGui::Begin("Animation Controller"); // Create a window called "Animation Controller"

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("State", &mSelectedState, "Model Viewer\0Flat Movement\0Programmed IK Movement\0IK Movement\0");

   ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

   ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

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

   ImGui::SliderInt("Iterations", &mSelectedNumberOfIterations, 0, 100);

   ImGui::SliderFloat("Emissivity", &mSelectedEmissiveTextureBrightnessScaleFactor, 0.0f, 1.0f, "%.3f");

   ImGui::SliderFloat("UV Scale", &mSelectedEmissiveTextureUVScaleFactor, 0.0f, 100.0f, "%.3f");

   ImGui::End();
}

void IKMovementState::resetScene()
{

}

void IKMovementState::resetCamera()
{
   mCamera3.reposition(8.0f, 15.0f, mModelTransform.position, mModelTransform.rotation, glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 90.0f, 0.0f, 90.0f);
}

void IKMovementState::configurePinTracks()
{
   // Walking pin tracks

   // Compose the pin track of the left foot, which tells us when the left foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   ScalarTrack leftFootWalkingPinTrack;
   leftFootWalkingPinTrack.SetInterpolation(Interpolation::Cubic);
   leftFootWalkingPinTrack.SetNumberOfFrames(4);
   // Frame 0
   leftFootWalkingPinTrack.GetFrame(0).mTime     = 0.0f;
   leftFootWalkingPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   leftFootWalkingPinTrack.GetFrame(1).mTime     = 0.4f;
   leftFootWalkingPinTrack.GetFrame(1).mValue[0] = 1;
   // Frame 2
   leftFootWalkingPinTrack.GetFrame(2).mTime     = 0.6f;
   leftFootWalkingPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   leftFootWalkingPinTrack.GetFrame(3).mTime     = 1.0f;
   leftFootWalkingPinTrack.GetFrame(3).mValue[0] = 0;

   //// Mine walking
   //// Frame 0
   //mLeftFootPinTrack.GetFrame(0).mTime = 0.0f;
   //mLeftFootPinTrack.GetFrame(0).mValue[0] = 0;
   //// Frame 1
   //mLeftFootPinTrack.GetFrame(1).mTime = 0.367f;
   //mLeftFootPinTrack.GetFrame(1).mValue[0] = 1;
   //// Frame 2
   //mLeftFootPinTrack.GetFrame(2).mTime = 0.5f;
   //mLeftFootPinTrack.GetFrame(2).mValue[0] = 1;
   //// Frame 3
   //mLeftFootPinTrack.GetFrame(3).mTime = 1.0f;
   //mLeftFootPinTrack.GetFrame(3).mValue[0] = 0;

   // Compose the pin track of the right foot, which tells us when the right foot is on and off the ground
   // Note that the times of the keyframes that make up this pin track are normalized, which means that
   // it must be sampled with the current time of an animation divided by its duration
   ScalarTrack rightFootWalkingPinTrack;
   rightFootWalkingPinTrack.SetInterpolation(Interpolation::Cubic);
   rightFootWalkingPinTrack.SetNumberOfFrames(4);
   // Frame 0
   rightFootWalkingPinTrack.GetFrame(0).mTime     = 0.0f;
   rightFootWalkingPinTrack.GetFrame(0).mValue[0] = 1;
   // Frame 1
   rightFootWalkingPinTrack.GetFrame(1).mTime     = 0.3f;
   rightFootWalkingPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   rightFootWalkingPinTrack.GetFrame(2).mTime     = 0.7f;
   rightFootWalkingPinTrack.GetFrame(2).mValue[0] = 0;
   // Frame 3
   rightFootWalkingPinTrack.GetFrame(3).mTime     = 1.0f;
   rightFootWalkingPinTrack.GetFrame(3).mValue[0] = 1;

   //// Mine walking
   //// Frame 0
   //mRightFootPinTrack.GetFrame(0).mTime = 0.0f;
   //mRightFootPinTrack.GetFrame(0).mValue[0] = 1;
   //// Frame 1
   //mRightFootPinTrack.GetFrame(1).mTime = 0.346f;
   //mRightFootPinTrack.GetFrame(1).mValue[0] = 0;
   //// Frame 2
   //mRightFootPinTrack.GetFrame(2).mTime = 0.78f;
   //mRightFootPinTrack.GetFrame(2).mValue[0] = 0;
   //// Frame 3
   //mRightFootPinTrack.GetFrame(3).mTime = 1.0f;
   //mRightFootPinTrack.GetFrame(3).mValue[0] = 1;

   mLeftFootPinTracks.insert(std::make_pair("Walking", leftFootWalkingPinTrack));
   mRightFootPinTracks.insert(std::make_pair("Walking", rightFootWalkingPinTrack));

   // Idle pin tracks

   ScalarTrack leftFootIdlePinTrack;
   leftFootIdlePinTrack.SetInterpolation(Interpolation::Constant);
   leftFootIdlePinTrack.SetNumberOfFrames(2);
   // Frame 0
   leftFootIdlePinTrack.GetFrame(0).mTime = 0.0f;
   leftFootIdlePinTrack.GetFrame(0).mValue[0] = 1.0f;
   // Frame 1
   leftFootIdlePinTrack.GetFrame(1).mTime = 1.0f;
   leftFootIdlePinTrack.GetFrame(1).mValue[0] = 1.0f;

   ScalarTrack rightFootIdlePinTrack;
   rightFootIdlePinTrack.SetInterpolation(Interpolation::Constant);
   rightFootIdlePinTrack.SetNumberOfFrames(2);
   // Frame 0
   rightFootIdlePinTrack.GetFrame(0).mTime = 0.0f;
   rightFootIdlePinTrack.GetFrame(0).mValue[0] = 1.0f;
   // Frame 1
   rightFootIdlePinTrack.GetFrame(1).mTime = 1.0f;
   rightFootIdlePinTrack.GetFrame(1).mValue[0] = 1.0f;

   mLeftFootPinTracks.insert(std::make_pair("Idle", leftFootIdlePinTrack));
   mRightFootPinTracks.insert(std::make_pair("Idle", rightFootIdlePinTrack));

   // Running pin tracks

   ScalarTrack leftFootRunningPinTrack;
   leftFootRunningPinTrack.SetInterpolation(Interpolation::Cubic);
   leftFootRunningPinTrack.SetNumberOfFrames(4);
   // Frame 0
   leftFootRunningPinTrack.GetFrame(0).mTime = 0.0f;
   leftFootRunningPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   leftFootRunningPinTrack.GetFrame(1).mTime = 0.38f;
   leftFootRunningPinTrack.GetFrame(1).mValue[0] = 1;
   // Frame 2
   leftFootRunningPinTrack.GetFrame(2).mTime = 0.55f;
   leftFootRunningPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   leftFootRunningPinTrack.GetFrame(3).mTime = 1.0f;
   leftFootRunningPinTrack.GetFrame(3).mValue[0] = 0;

   ScalarTrack rightFootRunningPinTrack;
   rightFootRunningPinTrack.SetInterpolation(Interpolation::Cubic);
   rightFootRunningPinTrack.SetNumberOfFrames(4);
   // Frame 0
   rightFootRunningPinTrack.GetFrame(0).mTime = 0.0f;
   rightFootRunningPinTrack.GetFrame(0).mValue[0] = 1;
   // Frame 1
   rightFootRunningPinTrack.GetFrame(1).mTime = 0.152f;
   rightFootRunningPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   rightFootRunningPinTrack.GetFrame(2).mTime = 0.8f;
   rightFootRunningPinTrack.GetFrame(2).mValue[0] = 0;
   // Frame 3
   rightFootRunningPinTrack.GetFrame(3).mTime = 1.0f;
   rightFootRunningPinTrack.GetFrame(3).mValue[0] = 1;

   mLeftFootPinTracks.insert(std::make_pair("Running", leftFootRunningPinTrack));
   mRightFootPinTracks.insert(std::make_pair("Running", rightFootRunningPinTrack));

   // Jumping statically pin track

   ScalarTrack leftAndRightFeetJumpingStaticallyPinTrack;
   leftAndRightFeetJumpingStaticallyPinTrack.SetInterpolation(Interpolation::Cubic);
   leftAndRightFeetJumpingStaticallyPinTrack.SetNumberOfFrames(4);
   // Frame 0
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(0).mTime = 0.0f;
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(0).mValue[0] = 1;
   // Frame 1
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(1).mTime = 0.297f;
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(2).mTime = 0.545f;
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(2).mValue[0] = 0;
   // Frame 3
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(3).mTime = 1.0f;
   leftAndRightFeetJumpingStaticallyPinTrack.GetFrame(3).mValue[0] = 1;

   mLeftFootPinTracks.insert(std::make_pair("Jump", leftAndRightFeetJumpingStaticallyPinTrack));
   mRightFootPinTracks.insert(std::make_pair("Jump", leftAndRightFeetJumpingStaticallyPinTrack));

   // Jumping dynamically pin track

   ScalarTrack leftFootJumpingDynamicallyPinTrack;
   leftFootJumpingDynamicallyPinTrack.SetInterpolation(Interpolation::Cubic);
   leftFootJumpingDynamicallyPinTrack.SetNumberOfFrames(5);
   // Frame 0
   leftFootJumpingDynamicallyPinTrack.GetFrame(0).mTime = 0.0f;
   leftFootJumpingDynamicallyPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   leftFootJumpingDynamicallyPinTrack.GetFrame(1).mTime = 0.8f;
   leftFootJumpingDynamicallyPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   leftFootJumpingDynamicallyPinTrack.GetFrame(2).mTime = 0.845f;
   leftFootJumpingDynamicallyPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   leftFootJumpingDynamicallyPinTrack.GetFrame(3).mTime = 0.95f;
   leftFootJumpingDynamicallyPinTrack.GetFrame(3).mValue[0] = 1;
   // Frame 4
   leftFootJumpingDynamicallyPinTrack.GetFrame(4).mTime = 1.0f;
   leftFootJumpingDynamicallyPinTrack.GetFrame(4).mValue[0] = 0;

   ScalarTrack rightFootJumpingDynamicallyPinTrack;
   rightFootJumpingDynamicallyPinTrack.SetInterpolation(Interpolation::Cubic);
   rightFootJumpingDynamicallyPinTrack.SetNumberOfFrames(6);
   // Frame 0
   rightFootJumpingDynamicallyPinTrack.GetFrame(0).mTime = 0.0f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(0).mValue[0] = 0;
   // Frame 1
   rightFootJumpingDynamicallyPinTrack.GetFrame(1).mTime = 0.622f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(1).mValue[0] = 0;
   // Frame 2
   rightFootJumpingDynamicallyPinTrack.GetFrame(2).mTime = 0.672f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(2).mValue[0] = 1;
   // Frame 3
   rightFootJumpingDynamicallyPinTrack.GetFrame(3).mTime = 0.773f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(3).mValue[0] = 1;
   // Frame 4
   rightFootJumpingDynamicallyPinTrack.GetFrame(4).mTime = 0.823f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(4).mValue[0] = 0;
   // Frame 5
   rightFootJumpingDynamicallyPinTrack.GetFrame(5).mTime = 1.0f;
   rightFootJumpingDynamicallyPinTrack.GetFrame(5).mValue[0] = 0;

   mLeftFootPinTracks.insert(std::make_pair("Jump2", leftFootJumpingDynamicallyPinTrack));
   mRightFootPinTracks.insert(std::make_pair("Jump2", rightFootJumpingDynamicallyPinTrack));
}

void IKMovementState::determineYPosition()
{
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
         break;
      }
   }
}
