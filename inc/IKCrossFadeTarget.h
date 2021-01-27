#ifndef IK_CROSSFADE_TARGET_H
#define IK_CROSSFADE_TARGET_H

#include "Pose.h"
#include "Clip.h"

template <typename CLIP>
struct TIKCrossFadeTarget
{
   TIKCrossFadeTarget();
   TIKCrossFadeTarget(CLIP*        clip,
                      ScalarTrack* leftFootPinTrack,
                      ScalarTrack* rightFootPinTrack,
                      Pose&        pose,
                      float        fadeDuration,
                      bool         lock);

   CLIP*        mClip;
   ScalarTrack* mLeftFootPinTrack;
   ScalarTrack* mRightFootPinTrack;
   Pose         mPose;
   float        mLeftFootPinTrackValue;
   float        mRightFootPinTrackValue;
   float        mPlaybackTime;
   float        mFadeDuration;
   float        mFadeTime;
   bool         mLock;
};

typedef TIKCrossFadeTarget<Clip> IKCrossFadeTarget;
typedef TIKCrossFadeTarget<FastClip> FastIKCrossFadeTarget;

#endif
