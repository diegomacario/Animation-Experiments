#ifndef CROSSFADE_CONTROLLER_H
#define CROSSFADE_CONTROLLER_H

#include <queue>
#include "CrossFadeTarget.h"
#include "Skeleton.h"

class CrossFadeController
{
public:

   CrossFadeController();
   CrossFadeController(Skeleton& skeleton);

   void SetSkeleton(Skeleton& skeleton);

   void Play(Clip* clip);
   void FadeTo(Clip* targetClip, float fadeDuration);
   void Update(float dt);

   Clip* GetcurrentClip();
   Pose& GetCurrentPose();

protected:

   Clip*                        mCurrentClip;
   float                        mPlaybackTime;
   Skeleton                     mSkeleton;
   Pose                         mCurrentPose;
   bool                         mWasSkeletonSet;
   std::queue<CrossFadeTarget>  mTargets;
};

#endif
