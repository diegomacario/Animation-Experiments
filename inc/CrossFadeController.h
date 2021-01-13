#ifndef CROSSFADE_CONTROLLER_H
#define CROSSFADE_CONTROLLER_H

#include <queue>
#include "CrossFadeTarget.h"
#include "Skeleton.h"

template <typename CLIP>
class TCrossFadeController
{
public:

   TCrossFadeController();
   TCrossFadeController(Skeleton& skeleton);

   void SetSkeleton(Skeleton& skeleton);

   void Play(CLIP* clip);
   void FadeTo(CLIP* targetClip, float fadeDuration);
   void Update(float dt);

   CLIP* GetCurrentClip();
   Pose& GetCurrentPose();

private:

   CLIP*                  mCurrentClip;
   float                  mPlaybackTime;
   Skeleton               mSkeleton;
   Pose                   mCurrentPose;
   bool                   mWasSkeletonSet;

   bool                   mFading;
   TCrossFadeTarget<CLIP> mTarget;
};

typedef TCrossFadeController<Clip> CrossFadeController;
typedef TCrossFadeController<FastClip> FastCrossFadeController;

#endif
