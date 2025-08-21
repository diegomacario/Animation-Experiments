#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "resource_manager.h"
#include "shader_loader.h"
#include "GLTFLoader.h"
#include "RearrangeBones.h"
#include "ModelViewerState.h"

ModelViewerState::ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                                   const std::shared_ptr<Window>&             window)
   : mFSM(finiteStateMachine)
   , mWindow(window)
{
   // Initialize the animated mesh shader
   mAnimatedMeshShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/animated_mesh_with_pregenerated_skin_matrices.vert",
                                                                                       "resources/shaders/diffuse_illumination.frag");

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

   initializeState();
}

void ModelViewerState::initializeState()
{
   // Set the initial clip
   unsigned int numClips = static_cast<unsigned int>(mClips.size());
   for (unsigned int clipIndex = 0; clipIndex < numClips; ++clipIndex)
   {
      if (mClips[clipIndex].GetName() == "Walking")
      {
         mSelectedClip = clipIndex;
         mAnimationData.currentClipIndex = clipIndex;
         break;
      }
   }

   // Set the initial pose
   mAnimationData.animatedPose = mSkeleton.GetRestPose();

   // Set the model transform
   mAnimationData.modelTransform = Transform(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(1.0f));
}

void ModelViewerState::enter()
{
   // Set the current state
   mSelectedState = 0;
   initializeState();
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
}

void ModelViewerState::update(float deltaTime)
{
   if (mAnimationData.currentClipIndex != mSelectedClip)
   {
      mAnimationData.currentClipIndex = mSelectedClip;
      mAnimationData.animatedPose     = mSkeleton.GetRestPose();
      mAnimationData.playbackTime     = 0.0f;
   }

   // Sample the clip to get the animated pose
   FastClip& currClip = mClips[mAnimationData.currentClipIndex];
   mAnimationData.playbackTime = currClip.Sample(mAnimationData.animatedPose, mAnimationData.playbackTime + deltaTime);

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
}

void ModelViewerState::render()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   userInterface();

   mWindow->bindMultisampleFramebuffer();
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);

   // Render the animated meshes
   mAnimatedMeshShader->use(true);
   mAnimatedMeshShader->setUniformMat4("model",      transformToMat4(mAnimationData.modelTransform));
    
   glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0.0f, 2.5f, 10.0f), glm::vec3(0.0f, 2.5f, 0.0f),glm::vec3(0.0f, 1.0f, 0.0f));
    
   mAnimatedMeshShader->setUniformMat4("view", viewMatrix);
    
   glm::mat4 perspectiveProjectionMatrix = glm::perspective(glm::radians(45.0f),
                                                            1280.0f / 720.0f,
                                                            0.1f,
                                                            130.0f);
    
   mAnimatedMeshShader->setUniformMat4("projection", perspectiveProjectionMatrix);
   mAnimatedMeshShader->setUniformMat4Array("animated[0]", mAnimationData.skinMatrices);

   // Loop over the meshes and render each one
   for (unsigned int i = 0,
        size = static_cast<unsigned int>(mAnimatedMeshes.size());
        i < size;
        ++i)
   {
      mAnimatedMeshes[i].Render();
   }

   mAnimatedMeshShader->use(false);

   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void ModelViewerState::exit()
{

}

void ModelViewerState::userInterface()
{
   ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Appearing);

   ImGui::Begin("Model Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Combo("Clip", &mSelectedClip, mClipNames.c_str());

   ImGui::End();
}
