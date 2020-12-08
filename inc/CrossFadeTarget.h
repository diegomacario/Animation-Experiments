#ifndef CROSSFADE_TARGET_H
#define CROSSFADE_TARGET_H

#include "Pose.h"
#include "Clip.h"

struct CrossFadeTarget
{
   inline CrossFadeTarget()
      : mClip(0)
      , mPlaybackTime(0.0f)
      , mFadeDuration(0.0f)
      , mFadeTime(0.0f)
   {

   }

   inline CrossFadeTarget(Clip* target, Pose& pose, float fadeDuration)
      : mClip(target)
      , mPlaybackTime(target->GetStartTime())
      , mPose(pose)
      , mFadeDuration(fadeDuration)
      , mFadeTime(0.0f)
   {

   }

   Pose  mPose;
   Clip* mClip;
   float mPlaybackTime;
   float mFadeDuration;
   float mFadeTime;
};

#endif
