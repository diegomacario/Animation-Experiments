#ifndef MODEL_VIEWER_STATE_H
#define MODEL_VIEWER_STATE_H

#include "window.h"
#include "AnimatedMesh.h"
#include "Clip.h"
#include "Shader.h"

class ModelViewerState
{
public:

   ModelViewerState(const std::shared_ptr<Window>& window);
   ~ModelViewerState() = default;

   ModelViewerState(const ModelViewerState&) = delete;
   ModelViewerState& operator=(const ModelViewerState&) = delete;

   ModelViewerState(ModelViewerState&&) = delete;
   ModelViewerState& operator=(ModelViewerState&&) = delete;

   void initializeState();

   void enter();
   void processInput(float deltaTime);
   void update(float deltaTime);
   void render();
   void exit();

private:

   void userInterface();

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
   int                       mSelectedClip;

   AnimationData             mAnimationData;
};

#endif
