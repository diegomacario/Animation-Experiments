#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "resource_manager.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "MovementState.h"

#ifdef __EMSCRIPTEN__
extern int runKey;
extern std::string runInstruction;
#endif

MovementState::MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                             const std::shared_ptr<Window>&             window)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera3(14.0f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 30.0f, 0.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 130.0f, 0.25f)
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/diffuse_illumination_with_25_lights.frag");
   configureLights(mAnimatedMeshShader);

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/diffuse_illumination_with_25_lights.frag");
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

   // Optimize the clips, rearrange them and store them
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
   data = LoadGLTFFile("resources/models/ground/platform.gltf");
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
   mGroundTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/ground/platform.png");

   initializeState();

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mCrossFadeController.GetCurrentPose());
}

void MovementState::initializeState()
{
   // Set the state values
   mIsWalking = false;
   mIsRunning = false;
   mIsInAir = false;
   mJumpingWhileIdle = false;
   mJumpingWhileWalking = false;
   mJumpingWhileRunning = false;

   // Set the initial clip and initialize the crossfade controller
   mCrossFadeController.SetSkeleton(mSkeleton);
   mCrossFadeController.Play(&mClips["Idle"], false);
   mCrossFadeController.Update(0.0f);
   mCrossFadeController.GetCurrentPose().GetMatrixPalette(mPosePalette);

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
   mPerformDepthTesting = true;
#endif
   mSelectedConstantAttenuation = 1.0f;
   mSelectedLinearAttenuation = 0.0f;
   mSelectedQuadraticAttenuation = 0.009f;

   // Set the model transform
   mModelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
}

void MovementState::enter()
{
   // Set the current state
   mSelectedState = 1;
   initializeState();
   resetCamera();
   resetScene();
}

void MovementState::processInput(float deltaTime)
{
   deltaTime *= mSelectedPlaybackSpeed;

   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE))
   {
      mWindow->setShouldClose(true);
   }

   // Change the state
   if (mSelectedState != 1)
   {
      switch (mSelectedState)
      {
      case 0:
         mFSM->changeState("viewer");
         break;
      case 2:
         mFSM->changeState("ik");
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
   if (mWindow->mouseMoved() &&
       (mWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || mWindow->keyIsPressed(GLFW_KEY_C)))
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
#ifdef __EMSCRIPTEN__
   bool runKeyPressed = mWindow->keyIsPressed(runKey);
#else
   bool runKeyPressed = mWindow->keyIsPressed(GLFW_KEY_LEFT_SHIFT);
#endif

   float movementSpeed = mCharacterWalkingSpeed;
   float rotationSpeed = mCharacterWalkingRotationSpeed;
   if ((runKeyPressed && !mJumpingWhileWalking) || mJumpingWhileRunning)
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

      if (movementKeyPressed)
      {
         mCamera3.processPlayerMovement(mModelTransform.position, mModelTransform.rotation);
      }
   }

   if (mWindow->keyIsPressed(GLFW_KEY_SPACE) && !mIsInAir)
   {
      mCrossFadeController.ClearTargets();

      if (mIsWalking)
      {
         mCrossFadeController.FadeTo(&mClips["Jump2"], 0.1f, true);
         mJumpingWhileWalking = true;
      }
      else if (mIsRunning)
      {
         mCrossFadeController.FadeTo(&mClips["Jump2"], 0.1f, true);
         mJumpingWhileRunning = true;
      }
      else
      {
         mCrossFadeController.FadeTo(&mClips["Jump"], 0.1f, true);
         mJumpingWhileIdle = true;
      }

      mIsInAir = true;
   }

   if (mIsInAir)
   {
      if (mCrossFadeController.IsLocked() && mCrossFadeController.IsCurrentClipFinished())
      {
         mCrossFadeController.Unlock();

         if (mIsWalking)
         {
            mCrossFadeController.FadeTo(&mClips["Walking"], 0.15f, false);
            mJumpingWhileWalking = false;
         }
         else if (mIsRunning)
         {
            mCrossFadeController.FadeTo(&mClips["Running"], 0.15f, false);
            mJumpingWhileRunning = false;
         }
         else
         {
            mCrossFadeController.FadeTo(&mClips["Idle"], 0.1f, false);
            mJumpingWhileIdle = false;
         }

         mIsInAir = false;
      }
   }

   if (movementKeyPressed && !mIsInAir)
   {
      if (!mIsWalking && !mIsRunning)
      {
         if (runKeyPressed)
         {
            mIsRunning = true;
            mCrossFadeController.FadeTo(&mClips["Running"], 0.25f, false);
         }
         else
         {
            mIsWalking = true;
            mCrossFadeController.FadeTo(&mClips["Walking"], 0.25f, false);
         }
      }
      else if (mIsWalking)
      {
         if (runKeyPressed)
         {
            mIsWalking = false;
            mIsRunning = true;
            mCrossFadeController.FadeTo(&mClips["Running"], 0.25f, false);
         }
      }
      else if (mIsRunning)
      {
         if (!runKeyPressed)
         {
            mIsRunning = false;
            mIsWalking = true;
            mCrossFadeController.FadeTo(&mClips["Walking"], 0.25f, false);
         }
      }
   }
   else if (!mIsInAir)
   {
      if (mIsWalking || mIsRunning)
      {
         mIsRunning = false;
         mIsWalking = false;
         mCrossFadeController.FadeTo(&mClips["Idle"], 0.25f, false);
      }
   }
}

void MovementState::update(float deltaTime)
{
   deltaTime *= mSelectedPlaybackSpeed;

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
   mCrossFadeController.Update(deltaTime);

   // Get the palette of the pose
   mCrossFadeController.GetCurrentPose().GetMatrixPalette(mPosePalette);

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
   mSkeletonViewer.UpdateBones(mCrossFadeController.GetCurrentPose(), mPosePalette);
}

void MovementState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

#ifndef __EMSCRIPTEN__
   mWindow->bindMultisampleFramebuffer();
#endif
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mStaticMeshShader->use(true);
   mStaticMeshShader->setUniformFloat("constantAtt",  mSelectedConstantAttenuation);
   mStaticMeshShader->setUniformFloat("linearAtt",    mSelectedLinearAttenuation);
   mStaticMeshShader->setUniformFloat("quadraticAtt", mSelectedQuadraticAttenuation);
   mStaticMeshShader->setUniformMat4("model",         glm::mat4(1.0f));
   mStaticMeshShader->setUniformMat4("view",          mCamera3.getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection",    mCamera3.getPerspectiveProjectionMatrix());
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
   if (mWireframeModeForCharacter)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the animated meshes
   if (mCurrentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformFloat("constantAtt",  mSelectedConstantAttenuation);
      mStaticMeshShader->setUniformFloat("linearAtt",    mSelectedLinearAttenuation);
      mStaticMeshShader->setUniformFloat("quadraticAtt", mSelectedQuadraticAttenuation);
      mStaticMeshShader->setUniformMat4("model",         transformToMat4(mModelTransform));
      mStaticMeshShader->setUniformMat4("view",          mCamera3.getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection",    mCamera3.getPerspectiveProjectionMatrix());
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
      mAnimatedMeshShader->setUniformFloat("constantAtt",     mSelectedConstantAttenuation);
      mAnimatedMeshShader->setUniformFloat("linearAtt",       mSelectedLinearAttenuation);
      mAnimatedMeshShader->setUniformFloat("quadraticAtt",    mSelectedQuadraticAttenuation);
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

void MovementState::exit()
{

}

void MovementState::configureLights(const std::shared_ptr<Shader>& shader)
{
   shader->use(true);

   std::string nameOfArrOfLights = "pointLights[";
   std::string lightIndexAsStr;

   // Set up a symmetric square grid of 25 lights
   float xPos = -40.0f;
   float zPos = -40.0f;
   int lightIndex = 0;
   for (int i = 0; i < 5; ++i)
   {
      for (int k = 0; k < 5; ++k)
      {
         lightIndexAsStr = std::to_string(lightIndex);
         shader->setUniformVec3(nameOfArrOfLights  + lightIndexAsStr + "].worldPos", glm::vec3(xPos, 7.0f, zPos));
         shader->setUniformVec3(nameOfArrOfLights  + lightIndexAsStr + "].color", glm::vec3(1.0f, 1.0f, 1.0f));
         zPos += 20.0f;
         ++lightIndex;
      }
      xPos += 20.0f;
      zPos = -40.0f;
   }

   shader->setUniformInt("numPointLightsInScene", 25);
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
   ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Appearing);

   ImGui::Begin("Flat Movement", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("State", &mSelectedState, "Model Viewer\0Flat Movement\0Programmed IK Movement\0IK Movement\0");

   if (ImGui::CollapsingHeader("Description", nullptr))
   {
      ImGui::Text("This state illustrates how one can fade from\n"
                  "one clip to another to create natural\n"
                  "transitions when a character goes from\n"
                  "walking to running, from running to jumping,\n"
                  "from being idle to walking, etc.");
   }

   if (ImGui::CollapsingHeader("Controls", nullptr))
   {
      ImGui::BulletText("Hold the left mouse button and move the mouse\n"
                        "to rotate the camera around the character.\n"
                        "Alternatively, hold the C key and move \n"
                        "the mouse (this is easier on a touchpad).");
      ImGui::BulletText("Use the scroll wheel to zoom in and out.");
      ImGui::BulletText("Press the R key to reset the camera.");
      ImGui::BulletText("Use the WASD keys to move.");
#ifdef __EMSCRIPTEN__
      ImGui::BulletText(runInstruction.c_str());
#else
      ImGui::BulletText("Hold the left Shift key to run.");
#endif
      ImGui::BulletText("Press the spacebar to jump.");
   }

   if (ImGui::CollapsingHeader("Settings", nullptr))
   {
      ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

      ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

      ImGui::Checkbox("Display Skin", &mDisplayMesh);

      ImGui::Checkbox("Display Bones", &mDisplayBones);

      ImGui::Checkbox("Display Joints", &mDisplayJoints);

#ifndef __EMSCRIPTEN__
      ImGui::Checkbox("Wireframe Mode for Skin", &mWireframeModeForCharacter);

      ImGui::Checkbox("Wireframe Mode for Joints", &mWireframeModeForJoints);

      ImGui::Checkbox("Perform Depth Testing", &mPerformDepthTesting);
#endif

      ImGui::SliderFloat("Constant Att.", &mSelectedConstantAttenuation, 0.0f, 50.0f, "%.3f");

      ImGui::SliderFloat("Linear Att.", &mSelectedLinearAttenuation, 0.0f, 1.0f, "%.3f");

      ImGui::SliderFloat("Quadratic Att.", &mSelectedQuadraticAttenuation, 0.0f, 0.1f, "%.3f");
   }

   ImGui::End();
}

void MovementState::resetScene()
{

}

void MovementState::resetCamera()
{
   mCamera3.reposition(14.0f, 25.0f, mModelTransform.position, mModelTransform.rotation, glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 30.0f, 0.0f, 90.0f);
}
