#ifndef CROSSFADE_CONTROLLER_SINGLE_H
#define CROSSFADE_CONTROLLER_SINGLE_H

#include "CrossFadeTarget.h"
#include "Skeleton.h"

template <typename CLIP>
class TCrossFadeControllerSingle
{
public:

   TCrossFadeControllerSingle();
   TCrossFadeControllerSingle(Skeleton& skeleton);

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

typedef TCrossFadeControllerSingle<Clip> CrossFadeControllerSingle;
typedef TCrossFadeControllerSingle<FastClip> FastCrossFadeControllerSingle;

#endif
