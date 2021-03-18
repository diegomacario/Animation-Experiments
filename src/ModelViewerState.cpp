#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "resource_manager.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "ModelViewerState.h"

#ifdef USE_THIRD_PERSON_CAMERA
ModelViewerState::ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                                   const std::shared_ptr<Window>&             window)
#else
ModelViewerState::ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                                   const std::shared_ptr<Window>&             window,
                                   const std::shared_ptr<Camera>&             camera)
#endif
   : mFSM(finiteStateMachine)
   , mWindow(window)
#ifdef USE_THIRD_PERSON_CAMERA
   , mCamera3(7.5f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 14.0f, 0.0f, 90.0f, 45.0f, 1280.0f / 720.0f, 0.1f, 130.0f, 0.25f)
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

   // Initialize the ground shader
   mGroundShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                 "resources/shaders/ambient_diffuse_illumination.frag");
   configureLights(mGroundShader);

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
   data = LoadGLTFFile("resources/models/table/wooden_floor.gltf");
   mGroundMeshes = LoadStaticMeshes(data);
   FreeGLTFFile(data);

   int positionsAttribLocOfStaticShader = mGroundShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader   = mGroundShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader = mGroundShader->getAttributeLocation("texCoord");
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
   mGroundTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/table/wooden_floor.jpg");

   initializeState();

   // Initialize the bones of the skeleton viewer
   mSkeletonViewer.InitializeBones(mAnimationData.animatedPose);
}

void ModelViewerState::initializeState()
{
   mPause = false;

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
   mPerformDepthTesting = true;
#endif

   // Set the initial pose
   mAnimationData.animatedPose = mSkeleton.GetRestPose();

   // Set the model transform
   mAnimationData.modelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void ModelViewerState::enter()
{
   // Set the current state
   mSelectedState = 0;
   initializeState();
   resetCamera();
   resetScene();
}

void ModelViewerState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Change the state
   if (mSelectedState != 0)
   {
      switch (mSelectedState)
      {
      case 1:
         mFSM->changeState("movement");
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
   if (mWindow->mouseMoved() &&
       (mWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) || mWindow->keyIsPressed(GLFW_KEY_C)))
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

   if (mWindow->keyIsPressed(GLFW_KEY_P) && !mWindow->keyHasBeenProcessed(GLFW_KEY_P))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_P);
      mPause = !mPause;
   }
}

void ModelViewerState::update(float deltaTime)
{
   if (mPause)
   {
      return;
   }

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
   mAnimationData.playbackTime = currClip.Sample(mAnimationData.animatedPose, mAnimationData.playbackTime + (deltaTime * mSelectedPlaybackSpeed));

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

void ModelViewerState::render()
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

   mGroundShader->use(true);

   glm::mat4 modelMatrix(1.0f);
   modelMatrix = glm::scale(modelMatrix, glm::vec3(0.10f));
   mGroundShader->setUniformMat4("model",      modelMatrix);
#ifdef USE_THIRD_PERSON_CAMERA
   mGroundShader->setUniformMat4("view",       mCamera3.getViewMatrix());
   mGroundShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
#else
   mGroundShader->setUniformMat4("view",       mCamera->getViewMatrix());
   mGroundShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
#endif
   mGroundTexture->bind(0, mGroundShader->getUniformLocation("diffuseTex"));

   // Loop over the ground meshes and render each one
   for (unsigned int i = 0,
      size = static_cast<unsigned int>(mGroundMeshes.size());
      i < size;
      ++i)
   {
      mGroundMeshes[i].Render();
   }

   mGroundTexture->unbind(0);
   mGroundShader->use(false);

#ifndef __EMSCRIPTEN__
   if (mWireframeModeForCharacter)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
#endif

   // Render the animated meshes
   if (mAnimationData.currentSkinningMode == SkinningMode::CPU && mDisplayMesh)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mAnimationData.modelTransform));
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
      mAnimatedMeshShader->setUniformMat4("model",      transformToMat4(mAnimationData.modelTransform));
#ifdef USE_THIRD_PERSON_CAMERA
      mAnimatedMeshShader->setUniformMat4("view",       mCamera3.getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection", mCamera3.getPerspectiveProjectionMatrix());
#else
      mAnimatedMeshShader->setUniformMat4("view",       mCamera->getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
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
      mSkeletonViewer.RenderBones(mAnimationData.modelTransform, mCamera3.getPerspectiveProjectionViewMatrix());
#else
      mSkeletonViewer.RenderBones(mAnimationData.modelTransform, mCamera->getPerspectiveProjectionViewMatrix());
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
      mSkeletonViewer.RenderJoints(mAnimationData.modelTransform, mCamera3.getPerspectiveProjectionViewMatrix(), mAnimationData.animatedPosePalette);
#else
      mSkeletonViewer.RenderJoints(mAnimationData.modelTransform, mCamera->getPerspectiveProjectionViewMatrix(), mAnimationData.animatedPosePalette);
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

void ModelViewerState::exit()
{

}

void ModelViewerState::configureLights(const std::shared_ptr<Shader>& shader)
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

void ModelViewerState::switchFromGPUToCPU()
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

void ModelViewerState::switchFromCPUToGPU()
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

void ModelViewerState::userInterface()
{
   ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Appearing);

   ImGui::Begin("Model Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("State", &mSelectedState, "Model Viewer\0Flat Movement\0Programmed IK Movement\0IK Movement\0");

   if (ImGui::CollapsingHeader("Controls", nullptr))
   {
      ImGui::BulletText("Hold the right mouse button and move the mouse\n"
                        "to rotate the camera around the character.\n"
                        "Alternatively, hold the C key and move \n"
                        "the mouse (this is easier on a touchpad).");
      ImGui::BulletText("Use the scroll wheel to zoom in and out.");
      ImGui::BulletText("Press the P key to pause the animation.");
      ImGui::BulletText("Press the R key to reset the camera.");
   }

   if (ImGui::CollapsingHeader("Settings", nullptr))
   {
      ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

      ImGui::Combo("Clip", &mSelectedClip, mClipNames.c_str());

      ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

      float durationOfCurrClip = mClips[mAnimationData.currentClipIndex].GetDuration();
      char progress[32];
      snprintf(progress, 32, "%.3f / %.3f", mAnimationData.playbackTime, durationOfCurrClip);
      ImGui::ProgressBar(mAnimationData.playbackTime / durationOfCurrClip, ImVec2(0.0f, 0.0f), progress);

      //float normalizedPlaybackTime = (mAnimationData.playbackTime - mClips[mAnimationData.currentClipIndex].GetStartTime()) / mClips[mAnimationData.currentClipIndex].GetDuration();
      //char progress[32];
      //snprintf(progress, 32, "%.3f", normalizedPlaybackTime);
      //ImGui::ProgressBar(normalizedPlaybackTime, ImVec2(0.0f, 0.0f), progress);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::Text("Playback Time");

      ImGui::Checkbox("Display Skin", &mDisplayMesh);

      ImGui::Checkbox("Display Bones", &mDisplayBones);

      ImGui::Checkbox("Display Joints", &mDisplayJoints);

#ifndef __EMSCRIPTEN__
      ImGui::Checkbox("Wireframe Mode for Skin", &mWireframeModeForCharacter);

      ImGui::Checkbox("Wireframe Mode for Joints", &mWireframeModeForJoints);

      ImGui::Checkbox("Perform Depth Testing", &mPerformDepthTesting);
#endif
   }

   ImGui::End();
}

void ModelViewerState::resetScene()
{

}

void ModelViewerState::resetCamera()
{
#ifdef USE_THIRD_PERSON_CAMERA
   mCamera3.reposition(7.5f, 25.0f, glm::vec3(0.0f), Q::quat(), glm::vec3(0.0f, 2.5f, 0.0f), 2.0f, 14.0f, 0.0f, 90.0f);
   mCamera3.processMouseMovement(180.0f / 0.25f, 0.0f);
#else
   mCamera->reposition(glm::vec3(0.00179474f, 6.62452f, 8.41094f),
                       glm::vec3(-0.0242029f, 1.95141f, 0.46319f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       45.0f);
#endif
}
