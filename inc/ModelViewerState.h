#ifndef MODEL_VIEWER_STATE_H
#define MODEL_VIEWER_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#ifdef USE_THIRD_PERSON_CAMERA
#include "Camera3.h"
#else
#include "camera.h"
#endif
#include "texture.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"

class ModelViewerState : public State
{
public:

#ifdef USE_THIRD_PERSON_CAMERA
   ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                    const std::shared_ptr<Window>&             window);
#else
   ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                    const std::shared_ptr<Window>&             window,
                    const std::shared_ptr<Camera>&             camera);
#endif
   ~ModelViewerState() = default;

   ModelViewerState(const ModelViewerState&) = delete;
   ModelViewerState& operator=(const ModelViewerState&) = delete;

   ModelViewerState(ModelViewerState&&) = delete;
   ModelViewerState& operator=(ModelViewerState&&) = delete;

   void initializeState();

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void configureLights(const std::shared_ptr<Shader>& shader);

   void switchFromGPUToCPU();
   void switchFromCPUToGPU();

   void userInterface();

   void resetScene();

   void resetCamera();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

#ifdef USE_THIRD_PERSON_CAMERA
   Camera3                             mCamera3;
#else
   std::shared_ptr<Camera>             mCamera;
#endif

   std::vector<AnimatedMesh>           mGroundMeshes;
   std::shared_ptr<Texture>            mGroundTexture;
   std::shared_ptr<Shader>             mGroundShader;

   enum SkinningMode : int
   {
      GPU = 0,
      CPU = 1,
   };

   struct AnimationData
   {
      AnimationData()
         : currentClipIndex(0)
         , currentSkinningMode(SkinningMode::GPU)
         , playbackTime(0.0f)
      {

      }

      unsigned int           currentClipIndex;
      SkinningMode           currentSkinningMode;

      float                  playbackTime;
      Pose                   animatedPose;
      std::vector<glm::mat4> animatedPosePalette;
      std::vector<glm::mat4> skinMatrices;
      Transform              modelTransform;
   };

   std::shared_ptr<Shader>   mAnimatedMeshShader;
   std::shared_ptr<Shader>   mStaticMeshShader;
   std::shared_ptr<Texture>  mDiffuseTexture;

   Skeleton                  mSkeleton;
   std::vector<AnimatedMesh> mAnimatedMeshes;
   SkeletonViewer            mSkeletonViewer;
   std::vector<FastClip>     mClips;
   std::string               mClipNames;
   int                       mSelectedState;
   int                       mSelectedClip;
   int                       mSelectedSkinningMode;
   float                     mSelectedPlaybackSpeed;
   bool                      mDisplayMesh;
   bool                      mDisplayBones;
   bool                      mDisplayJoints;
#ifndef __EMSCRIPTEN__
   bool                      mWireframeModeForCharacter;
   bool                      mWireframeModeForJoints;
   bool                      mPerformDepthTesting;
#endif

   AnimationData             mAnimationData;

   bool                      mPause = false;
};

#endif
