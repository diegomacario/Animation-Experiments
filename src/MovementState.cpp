#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <array>
#include <random>

#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "MovementState.h"

MovementState::MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                             const std::shared_ptr<Window>&             window,
                             const std::shared_ptr<Camera>&             camera,
                             const std::shared_ptr<Shader>&             gameObject3DShader,
                             const std::shared_ptr<Model>&              hardwoodFloor)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera3(12.0f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 30.0f, 0.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 130.0f, 0.25f)
   , mGameObject3DShader(gameObject3DShader)
{
   // Create the hardwood floor
   mHardwoodFloor = std::make_unique<GameObject3D>(hardwoodFloor,
                                                   glm::vec3(0.0f),
                                                   0.0f,
                                                   glm::vec3(0.0f, 0.0f, 0.0f),
                                                   0.8f);

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
   mCurrentSkinningMode = SkinningMode::GPU;
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

   // Set the model transform
   mModelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
         if (leftShiftKeyPressed)
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
         if (leftShiftKeyPressed)
         {
            mIsWalking = false;
            mIsRunning = true;
            mCrossFadeController.FadeTo(&mClips["Running"], 0.25f, false);
         }
      }
      else if (mIsRunning)
      {
         if (!leftShiftKeyPressed)
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

   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mGameObject3DShader->use(true);
   mGameObject3DShader->setUniformMat4("projectionView", mCamera3.getPerspectiveProjectionViewMatrix());
   mGameObject3DShader->setUniformVec3("cameraPos", mCamera3.getPosition());

   mHardwoodFloor->render(*mGameObject3DShader);

   mGameObject3DShader->use(false);

   if (mWireframeModeForMesh)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   // Render the animated meshes
   if (mCurrentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mModelTransform));
      mStaticMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mStaticMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
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
   else if (mCurrentSkinningMode == SkinningMode::GPU && mDisplayMesh)
   {
      mAnimatedMeshShader->use(true);
      mAnimatedMeshShader->setUniformMat4("model",            transformToMat4(mModelTransform));
      mAnimatedMeshShader->setUniformMat4("view",             mCamera3.getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera3.getPerspectiveProjectionMatrix());
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mSkinMatrices);
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
      mSkeletonViewer.RenderBones(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix());
   }

   glLineWidth(1.0f);

   if (mWireframeModeForJoints)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   // Render the joints
   if (mDisplayJoints)
   {
      mSkeletonViewer.RenderJoints(mModelTransform, mCamera3.getPerspectiveProjectionViewMatrix(), mPosePalette);
   }

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glEnable(GL_DEPTH_TEST);

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
   shader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   shader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[0].linearAtt", 0.01f);
   shader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   shader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 2.0f, -10.0f));
   shader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   shader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   shader->setUniformFloat("pointLights[1].linearAtt", 0.01f);
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

   ImGui::Combo("State", &mSelectedState, "Model Viewer\0Flat Movement\0Programmed IK Movement\0IK Movement\0");

   ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

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
   mCamera3.reposition(12.0f, 25.0f, mModelTransform.position, mModelTransform.rotation, glm::vec3(0.0f, 3.0f, 0.0f), 0.0f, 30.0f, 0.0f, 90.0f);
}
