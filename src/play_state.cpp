#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <array>
#include <random>

#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "play_state.h"

PlayState::PlayState(const std::shared_ptr<FiniteStateMachine>&     finiteStateMachine,
                     const std::shared_ptr<Window>&                 window,
                     const std::shared_ptr<Camera>&                 camera,
                     const std::shared_ptr<Shader>&                 gameObject3DShader,
                     const std::shared_ptr<Shader>&                 lineShader,
                     const std::shared_ptr<GameObject3D>&           table,
                     const std::shared_ptr<GameObject3D>&           teapot)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mGameObject3DShader(gameObject3DShader)
   , mLineShader(lineShader)
   , mTable(table)
   , mTeapot(teapot)
   , mWorldXAxis(glm::vec3(0.0f), glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)) // Red
   , mWorldYAxis(glm::vec3(0.0f), glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)) // Green
   , mWorldZAxis(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)) // Blue
   , mLocalXAxis(glm::vec3(0.0f), glm::vec3(16.0f, 0.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f))
   , mLocalYAxis(glm::vec3(0.0f), glm::vec3(0.0f, 16.0f, 0.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f))
   , mLocalZAxis(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 16.0f), glm::vec3(0.0f), 0.0f, glm::vec3(0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f))
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/mesh_with_simple_illumination.frag");
   mAnimatedMeshShader->use(true);
   mAnimatedMeshShader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   mAnimatedMeshShader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mAnimatedMeshShader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   mAnimatedMeshShader->setUniformFloat("pointLights[0].linearAtt", 0.01f);
   mAnimatedMeshShader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   mAnimatedMeshShader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 2.0f, -10.0f));
   mAnimatedMeshShader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mAnimatedMeshShader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   mAnimatedMeshShader->setUniformFloat("pointLights[1].linearAtt", 0.01f);
   mAnimatedMeshShader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);
   mAnimatedMeshShader->setUniformInt("numPointLightsInScene", 2);
   mAnimatedMeshShader->use(false);

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/mesh_with_simple_illumination.frag");
   mStaticMeshShader->use(true);
   mStaticMeshShader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   mStaticMeshShader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mStaticMeshShader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   mStaticMeshShader->setUniformFloat("pointLights[0].linearAtt", 0.01f);
   mStaticMeshShader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   mStaticMeshShader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 2.0f, -10.0f));
   mStaticMeshShader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mStaticMeshShader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   mStaticMeshShader->setUniformFloat("pointLights[1].linearAtt", 0.01f);
   mStaticMeshShader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);
   mStaticMeshShader->setUniformInt("numPointLightsInScene", 2);
   mStaticMeshShader->use(false);

   mDiffuseTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/woman/Woman.png");

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
         mAnimationData.mCurrentClipIndex = clipIndex;
      }
   }

   // Set the initial skinning mode
   mSelectedSkinningMode = SkinningMode::GPU;
   // Set the initial playback speed
   mSelectedPlaybackSpeed = 1.0f;
   // Set the initial rendering options
   mWireframeMode = false;

   // Set the initial pose
   mAnimationData.mAnimatedPose = mSkeleton.GetRestPose();

   // Set the model transform
   mAnimationData.mModelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
}

void PlayState::enter()
{
   resetCamera();
   resetScene();
}

void PlayState::processInput(float deltaTime)
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
         mWindow->enableCursor(false);
         if (mCamera->isFree())
         {
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

      if (!mWindow->isFullScreen())
      {
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
      }

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

void PlayState::update(float deltaTime)
{
   if (mAnimationData.mCurrentClipIndex != mSelectedClip)
   {
      mAnimationData.mCurrentClipIndex = mSelectedClip;
      mAnimationData.mAnimatedPose     = mSkeleton.GetRestPose();
   }

   if (mAnimationData.mCurrentSkinningMode != mSelectedSkinningMode)
   {
      if (mAnimationData.mCurrentSkinningMode == SkinningMode::GPU)
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
      else if (mAnimationData.mCurrentSkinningMode == SkinningMode::CPU)
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

      mAnimationData.mCurrentSkinningMode = static_cast<SkinningMode>(mSelectedSkinningMode);
   }

   // Sample the clip to get the animated pose
   FastClip& currClip = mClips[mAnimationData.mCurrentClipIndex];
   mAnimationData.mPlaybackTime = currClip.Sample(mAnimationData.mAnimatedPose, mAnimationData.mPlaybackTime + (deltaTime * mSelectedPlaybackSpeed));

   // Get the palette of the animated pose
   mAnimationData.mAnimatedPose.GetMatrixPalette(mAnimationData.mAnimatedPosePalette);

   // Update the skeleton viewer
   mSkeletonViewer.ExtractPointsOfSkeletonFromPose(mAnimationData.mAnimatedPose, mAnimationData.mAnimatedPosePalette);
   int positionsAttribLocOfLineShader = mLineShader->getAttributeLocation("inPos");
   int colorsAttribLocOfLineShader    = mLineShader->getAttributeLocation("inCol");
   // TODO: Not cool to be configuring VAO every frame. Fix this.
   mSkeletonViewer.ConfigureVAO(positionsAttribLocOfLineShader, colorsAttribLocOfLineShader);
   mSkeletonViewer.LoadBuffers();

   std::vector<glm::mat4>& inverseBindPose = mSkeleton.GetInvBindPose();

   // Generate the skin matrices
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimationData.mAnimatedPosePalette.size());
        i < size;
        ++i)
   {
      // We store the skin matrices in the same vector that we use to store the pose palette
      mAnimationData.mAnimatedPosePalette[i] = mAnimationData.mAnimatedPosePalette[i] * inverseBindPose[i];
   }

   // Skin the meshes on the CPU if that's the current skinning mode
   if (mAnimationData.mCurrentSkinningMode == SkinningMode::CPU)
   {
      for (unsigned int i = 0, size = (unsigned int)mAnimatedMeshes.size(); i < size; ++i)
      {
         mAnimatedMeshes[i].SkinMeshOnTheCPU(mAnimationData.mAnimatedPosePalette);
      }
   }
}

void PlayState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   //mLineShader->use(true);
   //mLineShader->setUniformMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());

   //mWorldXAxis.render(*mLineShader);
   //mWorldYAxis.render(*mLineShader);
   //mWorldZAxis.render(*mLineShader);

   //mLocalXAxis.render(*mLineShader);
   //mLocalYAxis.render(*mLineShader);
   //mLocalZAxis.render(*mLineShader);

   mGameObject3DShader->use(true);
   mGameObject3DShader->setUniformMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());
   mGameObject3DShader->setUniformVec3("cameraPos", mCamera->getPosition());

   mTable->render(*mGameObject3DShader);

   // Disable face culling so that we render the inside of the teapot
   //glDisable(GL_CULL_FACE);
   //mTeapot->render(*mGameObject3DShader);
   //glEnable(GL_CULL_FACE);

   mGameObject3DShader->use(false);

   if (mWireframeMode)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }

   // Render the animated meshes
   if (mAnimationData.mCurrentSkinningMode == SkinningMode::CPU)
   {
      mStaticMeshShader->use(true);
      mStaticMeshShader->setUniformMat4("model",      transformToMat4(mAnimationData.mModelTransform));
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
   else if (mAnimationData.mCurrentSkinningMode == SkinningMode::GPU)
   {
      mAnimatedMeshShader->use(true);
      mAnimatedMeshShader->setUniformMat4("model",            transformToMat4(mAnimationData.mModelTransform));
      mAnimatedMeshShader->setUniformMat4("view",             mCamera->getViewMatrix());
      mAnimatedMeshShader->setUniformMat4("projection",       mCamera->getPerspectiveProjectionMatrix());
      mAnimatedMeshShader->setUniformMat4Array("animated[0]", mAnimationData.mAnimatedPosePalette);
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

   glLineWidth(2.0f);

   // Render the skeleton
   mLineShader->use(true);
   mLineShader->setUniformMat4("model", transformToMat4(mAnimationData.mModelTransform));
   mLineShader->setUniformMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());
   mSkeletonViewer.Render();
   mLineShader->use(false);

   glLineWidth(1.0f);

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void PlayState::exit()
{

}

void PlayState::userInterface()
{
   ImGui::Begin("Animation Controller"); // Create a window called "Animation Controller"

   ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

   ImGui::Combo("Clip", &mSelectedClip, mClipNames.c_str());

   ImGui::Combo("Skinning Mode", &mSelectedSkinningMode, "GPU\0CPU\0");

   ImGui::SliderFloat("Playback Speed", &mSelectedPlaybackSpeed, 0.0f, 2.0f, "%.3f");

   ImGui::Checkbox("Wireframe Mode", &mWireframeMode);

   ImGui::End();
}

void PlayState::resetScene()
{

}

void PlayState::resetCamera()
{
   mCamera->reposition(glm::vec3(0.0f, 9.0f, 13.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       0.0f,
                       -30.0f,
                       45.0f);
}
