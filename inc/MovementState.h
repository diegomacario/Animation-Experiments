#ifndef MOVEMENT_STATE_H
#define MOVEMENT_STATE_H

#include <array>

#include "game.h"
#include "line.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"
#include "CrossFadeControllerMultiple.h"
#include "Camera3.h"

class MovementState : public State
{
public:

   MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                 const std::shared_ptr<Window>&             window,
                 const std::shared_ptr<Camera>&             camera,
                 const std::shared_ptr<Shader>&             gameObject3DShader,
                 const std::shared_ptr<Model>&              hardwoodFloor);
   ~MovementState() = default;

   MovementState(const MovementState&) = delete;
   MovementState& operator=(const MovementState&) = delete;

   MovementState(MovementState&&) = delete;
   MovementState& operator=(MovementState&&) = delete;

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

   Camera3                             mCamera3;

   std::shared_ptr<Shader>             mGameObject3DShader;

   std::unique_ptr<GameObject3D>       mHardwoodFloor;

   enum SkinningMode : int
   {
      GPU = 0,
      CPU = 1,
   };

   std::shared_ptr<Shader>   mAnimatedMeshShader;
   std::shared_ptr<Shader>   mStaticMeshShader;
   std::shared_ptr<Texture>  mDiffuseTexture;

   Skeleton                  mSkeleton;
   std::vector<AnimatedMesh> mAnimatedMeshes;
   SkeletonViewer            mSkeletonViewer;
   SkinningMode              mCurrentSkinningMode;
   int                       mSelectedState;
   int                       mSelectedSkinningMode;
   float                     mSelectedPlaybackSpeed;
   bool                      mDisplayMesh;
   bool                      mDisplayBones;
   bool                      mDisplayJoints;
   bool                      mWireframeModeForMesh;
   bool                      mWireframeModeForJoints;
   bool                      mPerformDepthTesting;

   // --- --- ---

   std::map<std::string, FastClip> mClips;

   FastCrossFadeControllerMultiple mCrossFadeController;
   std::vector<glm::mat4>          mPosePalette;
   std::vector<glm::mat4>          mSkinMatrices;

   Transform                       mModelTransform;
   float                           mCharacterWalkingSpeed = 4.0f;
   float                           mCharacterRunningSpeed = 12.0f;
   float                           mCharacterWalkingRotationSpeed = 100.0f;
   float                           mCharacterRunningRotationSpeed = 200.0f;
   bool                            mIsWalking = false;
   bool                            mIsRunning = false;
   bool                            mIsInAir = false;
   bool                            mJumpingWhileIdle = false;
   bool                            mJumpingWhileWalking = false;
   bool                            mJumpingWhileRunning = false;
};

#endif
