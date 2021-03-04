#include "IKCrossFadeTarget.h"

template <typename CLIP>
TIKCrossFadeTarget<CLIP>::TIKCrossFadeTarget()
   : mClip(nullptr)
   , mLeftFootPinTrack(nullptr)
   , mRightFootPinTrack(nullptr)
   , mLeftFootPinTrackValue(0.0f)
   , mRightFootPinTrackValue(0.0f)
   , mPlaybackTime(0.0f)
   , mFadeDuration(0.0f)
   , mFadeTime(0.0f)
   , mLock(false)
{

}

template <typename CLIP>
TIKCrossFadeTarget<CLIP>::TIKCrossFadeTarget(CLIP*        clip,
                                             ScalarTrack* leftFootPinTrack,
                                             ScalarTrack* rightFootPinTrack,
                                             Pose&        pose,
                                             float        fadeDuration,
                                             bool         lock)
   : mClip(clip)
   , mLeftFootPinTrack(leftFootPinTrack)
   , mRightFootPinTrack(rightFootPinTrack)
   , mPose(pose)
   , mLeftFootPinTrackValue(0.0f)
   , mRightFootPinTrackValue(0.0f)
   , mPlaybackTime(clip->GetStartTime())
   , mFadeDuration(fadeDuration)
   , mFadeTime(0.0f)
   , mLock(lock)
{

}

// Instantiate the desired IKCrossFadeTarget structs from the IKCrossFadeTarget struct template
template struct TIKCrossFadeTarget<Clip>;
template struct TIKCrossFadeTarget<FastClip>;
