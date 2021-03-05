#ifndef MOVEMENT_STATE_H
#define MOVEMENT_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#include "texture.h"
#include "AnimatedMesh.h"
#include "SkeletonViewer.h"
#include "Clip.h"
#include "CrossFadeControllerMultiple.h"
#include "Camera3.h"

class MovementState : public State
{
public:

   MovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                 const std::shared_ptr<Window>&             window);
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

   std::vector<AnimatedMesh>           mGroundMeshes;
   std::shared_ptr<Texture>            mGroundTexture;

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
   float                     mSelectedConstantAttenuation;
   float                     mSelectedLinearAttenuation;
   float                     mSelectedQuadraticAttenuation;
   bool                      mDisplayMesh;
   bool                      mDisplayBones;
   bool                      mDisplayJoints;
#ifndef __EMSCRIPTEN__
   bool                      mWireframeModeForCharacter;
   bool                      mWireframeModeForJoints;
   bool                      mPerformDepthTesting;
#endif

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
