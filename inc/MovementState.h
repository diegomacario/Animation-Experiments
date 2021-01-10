#ifndef MOVEMENT_STATE_H
#define MOVEMENT_STATE_H

#include <array>

#include "game.h"
#include "line.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"
#include "Triangle.h"
#include "IKLeg.h"

class MovementState : public State
{
public:

   MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                 const std::shared_ptr<Window>&             window,
                 const std::shared_ptr<Camera>&             camera,
                 const std::shared_ptr<Shader>&             gameObject3DShader,
                 const std::shared_ptr<GameObject3D>&       table,
                 const std::shared_ptr<GameObject3D>&       teapot);
   ~MovementState() = default;

   MovementState(const MovementState&) = delete;
   MovementState& operator=(const MovementState&) = delete;

   MovementState(MovementState&&) = delete;
   MovementState& operator=(MovementState&&) = delete;

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

   std::shared_ptr<Shader>             mGameObject3DShader;

   std::shared_ptr<GameObject3D>       mTable;
   std::shared_ptr<GameObject3D>       mTeapot;

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
   int                       mSelectedClip;
   int                       mSelectedSkinningMode;
   float                     mSelectedPlaybackSpeed;
   bool                      mDisplayMesh;
   bool                      mDisplayBones;
   bool                      mDisplayJoints;
   bool                      mWireframeModeForMesh;
   bool                      mWireframeModeForJoints;
   bool                      mPerformDepthTesting;

   AnimationData             mAnimationData;

   // --- --- ---

   std::vector<AnimatedMesh> mGroundMeshes;
   std::shared_ptr<Texture>  mGroundTexture;
   std::vector<Triangle>     mGroundTriangles;

   VectorTrack               mMotionTrack;
   IKLeg                     mLeftLeg;
   IKLeg                     mRightLeg;

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