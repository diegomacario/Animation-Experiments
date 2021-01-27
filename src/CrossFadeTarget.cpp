#include "CrossFadeTarget.h"

template TCrossFadeTarget<Clip>;
template TCrossFadeTarget<FastClip>;

template <typename CLIP>
TCrossFadeTarget<CLIP>::TCrossFadeTarget()
   : mClip(nullptr)
   , mPlaybackTime(0.0f)
   , mFadeDuration(0.0f)
   , mFadeTime(0.0f)
   , mLock(false)
{

}

template <typename CLIP>
TCrossFadeTarget<CLIP>::TCrossFadeTarget(CLIP* clip, Pose& pose, float fadeDuration, bool lock)
   : mClip(clip)
   , mPose(pose)
   , mPlaybackTime(clip->GetStartTime())
   , mFadeDuration(fadeDuration)
   , mFadeTime(0.0f)
   , mLock(lock)
{

}
