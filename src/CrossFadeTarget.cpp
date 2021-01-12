#include "CrossFadeTarget.h"

template TCrossFadeTarget<Clip>;
template TCrossFadeTarget<FastClip>;

template <typename CLIP>
TCrossFadeTarget<CLIP>::TCrossFadeTarget()
   : mClip(nullptr)
   , mPlaybackTime(0.0f)
   , mFadeDuration(0.0f)
   , mFadeTime(0.0f)
{

}

template <typename CLIP>
TCrossFadeTarget<CLIP>::TCrossFadeTarget(CLIP* target, Pose& pose, float fadeDuration)
   : mClip(target)
   , mPlaybackTime(target->GetStartTime())
   , mPose(pose)
   , mFadeDuration(fadeDuration)
   , mFadeTime(0.0f)
{

}
