#ifndef CROSSFADE_CONTROLLER_QUEUE_H
#define CROSSFADE_CONTROLLER_QUEUE_H

#include <queue>
#include "CrossFadeTarget.h"
#include "Skeleton.h"

template <typename CLIP>
class TCrossFadeControllerQueue
{
public:

   TCrossFadeControllerQueue();
   TCrossFadeControllerQueue(Skeleton& skeleton);

   void SetSkeleton(Skeleton& skeleton);

   void Play(CLIP* clip);
   void FadeTo(CLIP* targetClip, float fadeDuration);
   void Update(float dt);

   CLIP* GetCurrentClip();
   Pose& GetCurrentPose();

private:

   CLIP*                              mCurrentClip;
   float                              mPlaybackTime;
   Skeleton                           mSkeleton;
   Pose                               mCurrentPose;
   bool                               mWasSkeletonSet;

   std::queue<TCrossFadeTarget<CLIP>> mTargets;
};

typedef TCrossFadeControllerQueue<Clip> CrossFadeControllerQueue;
typedef TCrossFadeControllerQueue<FastClip> FastCrossFadeControllerQueue;

#endif
