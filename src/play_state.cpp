#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <array>
#include <random>

#include "shader_loader.h"
#include "texture_loader.h"
#include "GLTFLoader.h"
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
                                                                                       "resources/shaders/diffuse.frag");

   // Initialize the static mesh shader
   mStaticMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/static_mesh.vert",
                                                                                     "resources/shaders/diffuse.frag");

   mDiffuseTexture = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/woman/Woman.png");

   cgltf_data* data   = LoadGLTFFile("resources/models/woman/Woman.gltf");
   mSkeleton          = LoadSkeleton(data);
   mCPUAnimatedMeshes = LoadAnimatedMeshes(data);
   mGPUAnimatedMeshes = LoadAnimatedMeshes(data);
   mClips             = LoadClips(data);
   FreeGLTFFile(data);

   // Configure the VAOs of the CPU animated meshes
   int positionsAttribLocOfStaticShader  = mStaticMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfStaticShader    = mStaticMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfStaticShader  = mStaticMeshShader->getAttributeLocation("texCoord");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mCPUAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mCPUAnimatedMeshes[i].ConfigureVAO(positionsAttribLocOfStaticShader,
                                         normalsAttribLocOfStaticShader,
                                         texCoordsAttribLocOfStaticShader,
                                         -1,
                                         -1);
   }

   // Configure the VAOs of the GPU animated meshes
   int positionsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("position");
   int normalsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("normal");
   int texCoordsAttribLocOfAnimatedShader  = mAnimatedMeshShader->getAttributeLocation("texCoord");
   int weightsAttribLocOfAnimatedShader    = mAnimatedMeshShader->getAttributeLocation("weights");
   int influencesAttribLocOfAnimatedShader = mAnimatedMeshShader->getAttributeLocation("joints");
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGPUAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mGPUAnimatedMeshes[i].ConfigureVAO(positionsAttribLocOfAnimatedShader,
                                         normalsAttribLocOfAnimatedShader,
                                         texCoordsAttribLocOfAnimatedShader,
                                         weightsAttribLocOfAnimatedShader,
                                         influencesAttribLocOfAnimatedShader);
   }

   // Set the initial clips
   unsigned int numClips = static_cast<unsigned int>(mClips.size());
   for (unsigned int clipIndex = 0; clipIndex < numClips; ++clipIndex)
   {
      if (mClips[clipIndex].GetName() == "Walking")
      {
         mCPUAnimationData.mClip = clipIndex;
      }
      else if (mClips[clipIndex].GetName() == "Running")
      {
         mGPUAnimationData.mClip = clipIndex;
      }
   }

   // Set the initial poses
   mCPUAnimationData.mAnimatedPose = mSkeleton.GetRestPose();
   mGPUAnimationData.mAnimatedPose = mSkeleton.GetRestPose();

   // Set the model transforms
   mCPUAnimationData.mModelTransform = Transform(glm::vec3(-2.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
   mGPUAnimationData.mModelTransform = Transform(glm::vec3(2.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
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
   Clip& currCPUClip = mClips[mCPUAnimationData.mClip];
   Clip& currGPUClip = mClips[mGPUAnimationData.mClip];

   // Sample the CPU and GPU clips to get animated poses
   mCPUAnimationData.mPlayTime = currCPUClip.Sample(mCPUAnimationData.mAnimatedPose, mCPUAnimationData.mPlayTime + deltaTime);
   mGPUAnimationData.mPlayTime = currGPUClip.Sample(mGPUAnimationData.mAnimatedPose, mGPUAnimationData.mPlayTime + deltaTime);

   // Get the palettes of the CPU and GPU animated poses
   mCPUAnimationData.mAnimatedPose.GetMatrixPalette(mCPUAnimationData.mAnimatedPosePalette);
   mGPUAnimationData.mAnimatedPose.GetMatrixPalette(mGPUAnimationData.mAnimatedPosePalette);

   std::vector<glm::mat4>& inverseBindPose = mSkeleton.GetInvBindPose();

   // Generate the skin matrices of the CPU meshes
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mCPUAnimationData.mAnimatedPosePalette.size());
        i < size;
        ++i)
   {
      // We store the skin matrices in the same vectors that we use to store the pose palettes
      mCPUAnimationData.mAnimatedPosePalette[i] = mCPUAnimationData.mAnimatedPosePalette[i] * inverseBindPose[i];
   }

   // Generate the skin matrices of the GPU meshes
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGPUAnimationData.mAnimatedPosePalette.size());
        i < size;
        ++i)
   {
      // We store the skin matrices in the same vectors that we use to store the pose palettes
      mGPUAnimationData.mAnimatedPosePalette[i] = mGPUAnimationData.mAnimatedPosePalette[i] * inverseBindPose[i];
   }

   // Skin the CPU animated meshes
   for (unsigned int i = 0, size = (unsigned int)mCPUAnimatedMeshes.size(); i < size; ++i)
   {
      mCPUAnimatedMeshes[i].SkinMeshOnTheCPU(mCPUAnimationData.mAnimatedPosePalette);
   }
}

void PlayState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   {
      ImGui::Begin("Animation Controller"); // Create a window called "Animation Controller"

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

      ImGui::End();
   }

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

   mGameObject3DShader->use(false);

   // Disable face culling so that we render the inside of the teapot
   //glDisable(GL_CULL_FACE);
   //mTeapot->render(*mGameObject3DShader);
   //glEnable(GL_CULL_FACE);

   // Render the CPU mesh
   // -------------------------------------------------------------------------

   mStaticMeshShader->use(true);
   mStaticMeshShader->setUniformMat4("model",      transformToMat4(mCPUAnimationData.mModelTransform));
   mStaticMeshShader->setUniformMat4("view",       mCamera->getViewMatrix());
   mStaticMeshShader->setUniformMat4("projection", mCamera->getPerspectiveProjectionMatrix());
   mStaticMeshShader->setUniformVec3("light",      glm::vec3(1, 1, 1));
   mDiffuseTexture->bind(0, mStaticMeshShader->getUniformLocation("tex0"));

   // Loop over the meshes and render each one
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mCPUAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mCPUAnimatedMeshes[i].Render();
   }

   mDiffuseTexture->unbind(0);
   mStaticMeshShader->use(false);

   // Render the GPU mesh
   // -------------------------------------------------------------------------

   mAnimatedMeshShader->use(true);
   mAnimatedMeshShader->setUniformMat4("model",            transformToMat4(mGPUAnimationData.mModelTransform));
   mAnimatedMeshShader->setUniformMat4("view",             mCamera->getViewMatrix());
   mAnimatedMeshShader->setUniformMat4("projection",       mCamera->getPerspectiveProjectionMatrix());
   mAnimatedMeshShader->setUniformMat4Array("animated[0]", mGPUAnimationData.mAnimatedPosePalette);
   mAnimatedMeshShader->setUniformVec3("light",            glm::vec3(1, 1, 1));
   mDiffuseTexture->bind(0, mAnimatedMeshShader->getUniformLocation("tex0"));

   // Loop over the meshes and render each one
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mGPUAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mGPUAnimatedMeshes[i].Render();
   }

   mDiffuseTexture->unbind(0);
   mAnimatedMeshShader->use(false);

   // -------------------------------------------------------------------------

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void PlayState::exit()
{

}

void PlayState::resetScene()
{

}

void PlayState::resetCamera()
{
   mCamera->reposition(glm::vec3(30.0f, 30.0f, 30.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       -45.0f,
                       -30.0f,
                       45.0f);
}
