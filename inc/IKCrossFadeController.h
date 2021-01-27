#ifndef IK_CROSSFADE_CONTROLLER_H
#define IK_CROSSFADE_CONTROLLER_H

#include "IKCrossFadeTarget.h"
#include "Skeleton.h"

template <typename CLIP>
class TIKCrossFadeController
{
public:

   TIKCrossFadeController();
   TIKCrossFadeController(Skeleton& skeleton);

   void SetSkeleton(Skeleton& skeleton);

   void Play(CLIP* clip, ScalarTrack* leftFootPinTrack, ScalarTrack* rightFootPinTrack, bool lock);
   void FadeTo(CLIP* targetClip, ScalarTrack* leftFootPinTrack, ScalarTrack* rightFootPinTrack, float fadeDuration, bool lock);
   void Update(float dt);
   void ClearTargets();

   CLIP* GetCurrentClip();
   Pose& GetCurrentPose();
   float GetCurrentLeftFootPinTrackValue();
   float GetCurrentRightFootPinTrackValue();

   bool  IsCurrentClipFinished();
   bool  IsLocked();
   void  Unlock();

   float GetPlaybackTime();

private:

   CLIP*                                 mCurrentClip;
   ScalarTrack*                          mCurrentLeftFootPinTrack;
   ScalarTrack*                          mCurrentRightFootPinTrack;
   float                                 mPlaybackTime;
   Skeleton                              mSkeleton;
   Pose                                  mCurrentPose;
   float                                 mCurrentLeftFootPinTrackValue;
   float                                 mCurrentRightFootPinTrackValue;
   bool                                  mWasSkeletonSet;
   bool                                  mLock;

   std::vector<TIKCrossFadeTarget<CLIP>> mTargets;
};

typedef TIKCrossFadeController<Clip> IKCrossFadeController;
typedef TIKCrossFadeController<FastClip> FastIKCrossFadeController;

#endif
