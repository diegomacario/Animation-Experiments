#ifndef IK_MOVEMENT_STATE_H
#define IK_MOVEMENT_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#include "texture.h"
#include "AnimatedMesh.h"
#include "SkeletonViewerClipped.h"
#include "Clip.h"
#include "IKLeg.h"
#include "IKCrossFadeController.h"
#include "Camera3.h"
#include "Water.h"
#include "Sky.h"

class IKMovementState : public State
{
public:

   IKMovementState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                   const std::shared_ptr<Window>&             window);
   ~IKMovementState() = default;

   IKMovementState(const IKMovementState&) = delete;
   IKMovementState& operator=(const IKMovementState&) = delete;

   IKMovementState(IKMovementState&&) = delete;
   IKMovementState& operator=(IKMovementState&&) = delete;

   void initializeState();

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void configureLights(const std::shared_ptr<Shader>& shader, const glm::vec3& lightColor);

   void switchFromGPUToCPU();
   void switchFromCPUToGPU();

   void renderScene(const glm::vec2& horizontalClippingPlaneYNormalAndHeight, const glm::mat4& viewMat, const glm::mat4 perspMat, bool renderWater);

   void userInterface();

   void resetScene();

   void resetCamera();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   Camera3                             mCamera3;

   enum SkinningMode : int
   {
      GPU = 0,
      CPU = 1,
   };

   std::shared_ptr<Shader>   mAnimatedCharacterMeshShader;
   std::shared_ptr<Shader>   mStaticCharacterMeshShader;
   std::shared_ptr<Shader>   mStaticMeshShader;
   std::shared_ptr<Texture>  mDiffuseTexture;

   Skeleton                  mSkeleton;
   std::vector<AnimatedMesh> mAnimatedMeshes;
   SkeletonViewerClipped     mSkeletonViewer;
   SkinningMode              mCurrentSkinningMode;
   int                       mSelectedState;
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
   bool                      mSolveWithConstraints;
   int                       mSelectedNumberOfIterations;
   float                     mSelectedEmissiveTextureBrightnessScaleFactor;
   float                     mSelectedEmissiveTextureUVScaleFactor;

   // --- --- ---

   std::map<std::string, FastClip>    mClips;
   std::map<std::string, ScalarTrack> mLeftFootPinTracks;
   std::map<std::string, ScalarTrack> mRightFootPinTracks;

   FastIKCrossFadeController mIKCrossFadeController;
   std::vector<glm::mat4>    mPosePalette;
   std::vector<glm::mat4>    mSkinMatrices;

   Transform                 mModelTransform;
   float                     mCharacterWalkingSpeed = 4.0f;
   float                     mCharacterRunningSpeed = 12.0f;
   float                     mCharacterWalkingRotationSpeed = 100.0f;
   float                     mCharacterRunningRotationSpeed = 200.0f;
   bool                      mIsWalking = false;
   bool                      mIsRunning = false;
   bool                      mIsInAir = false;
   bool                      mJumpingWhileIdle = false;
   bool                      mJumpingWhileWalking = false;
   bool                      mJumpingWhileRunning = false;

   // --- --- ---

   std::vector<AnimatedMesh> mGroundMeshes;
   std::shared_ptr<Texture>  mGroundDiffuseTexture;
   std::shared_ptr<Texture>  mGroundEmissiveTexture;
   std::vector<Triangle>     mGroundTriangles;

   IKLeg                     mLeftLeg;
   IKLeg                     mRightLeg;

   float                     mHeightOfOriginOfYPositionRay;
   float                     mPreviousYPositionOfCharacter;
   float                     mSinkIntoGround;
   float                     mHeightOfHip;
   float                     mHeightOfKnees;
   float                     mDistanceFromAnkleToToe;
   float                     mAnkleVerticalOffset;

   glm::vec3                 mLeftAnkleFinalTarget;
   glm::vec3                 mRightAnkleFinalTarget;

   Water                     mWater;
   Sky                       mSky;

   // --- --- ---

   void configurePinTracks();

   void determineYPosition();

   glm::mat4 calculateReflectionViewMatrix();
};

#endif
