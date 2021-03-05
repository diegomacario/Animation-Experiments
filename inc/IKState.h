#ifndef IK_STATE_H
#define IK_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#include "camera.h"
#include "texture.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"
#include "Triangle.h"
#include "IKLeg.h"

class IKState : public State
{
public:

   IKState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
           const std::shared_ptr<Window>&             window,
           const std::shared_ptr<Camera>&             camera);
   ~IKState() = default;

   IKState(const IKState&) = delete;
   IKState& operator=(const IKState&) = delete;

   IKState(IKState&&) = delete;
   IKState& operator=(IKState&&) = delete;

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

   std::shared_ptr<Camera>             mCamera;

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
   bool                      mWireframeModeForTerrain;
   bool                      mPerformDepthTesting;
#endif

   AnimationData             mAnimationData;

   // --- --- ---

   std::vector<AnimatedMesh> mGroundMeshes;
   std::shared_ptr<Texture>  mGroundTexture;
   std::vector<Triangle>     mGroundTriangles;

   VectorTrack               mMotionTrack;
   IKLeg                     mLeftLeg;
   IKLeg                     mRightLeg;
   ScalarTrack               mLeftFootPinTrack;
   ScalarTrack               mRightFootPinTrack;

   Transform                 mModelTransform;
   float                     mHeightOfOriginOfYPositionRay;
   float                     mPreviousYPositionOfCharacter;
   float                     mSinkIntoGround;
   float                     mMotionTrackTime;
   float                     mMotionTrackPlaybackSpeed;
   float                     mMotionTrackDuration;
   float                     mMotionTrackFutureTimeOffset;
   float                     mHeightOfHip;
   float                     mHeightOfKnees;
   float                     mDistanceFromAnkleToToe;
};

#endif
