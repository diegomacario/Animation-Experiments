#ifndef MODEL_VIEWER_STATE_H
#define MODEL_VIEWER_STATE_H

#include "state.h"
#include "finite_state_machine.h"
#include "window.h"
#include "AnimatedMesh.h"
#include "Clip.h"
#include "Shader.h"

class ModelViewerState : public State
{
public:

   ModelViewerState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                    const std::shared_ptr<Window>&             window);
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

   void userInterface();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   struct AnimationData
   {
      AnimationData()
         : currentClipIndex(0)
         , playbackTime(0.0f)
      {

      }

      unsigned int           currentClipIndex;

      float                  playbackTime;
      Pose                   animatedPose;
      std::vector<glm::mat4> animatedPosePalette;
      std::vector<glm::mat4> skinMatrices;
      Transform              modelTransform;
   };

   std::shared_ptr<Shader>   mAnimatedMeshShader;

   Skeleton                  mSkeleton;
   std::vector<AnimatedMesh> mAnimatedMeshes;
   std::vector<FastClip>     mClips;
   std::string               mClipNames;
   int                       mSelectedState;
   int                       mSelectedClip;

   AnimationData             mAnimationData;
};

#endif
