#ifndef CROSSFADE_TARGET_H
#define CROSSFADE_TARGET_H

#include "Pose.h"
#include "Clip.h"

template <typename CLIP>
struct TCrossFadeTarget
{
   TCrossFadeTarget();
   TCrossFadeTarget(CLIP* target, Pose& pose, float fadeDuration, bool lock);

   Pose  mPose;
   CLIP* mClip;
   float mPlaybackTime;
   float mFadeDuration;
   float mFadeTime;
   bool  mLock;
};

typedef TCrossFadeTarget<Clip> CrossFadeTarget;
typedef TCrossFadeTarget<FastClip> FastCrossFadeTarget;

#endif
