#ifndef CROSSFADE_CONTROLLER_MULTIPLE_H
#define CROSSFADE_CONTROLLER_MULTIPLE_H

#include "CrossFadeTarget.h"
#include "Skeleton.h"

template <typename CLIP>
class TCrossFadeControllerMultiple
{
public:

   TCrossFadeControllerMultiple();
   TCrossFadeControllerMultiple(Skeleton& skeleton);

   void SetSkeleton(Skeleton& skeleton);

   void Play(CLIP* clip);
   void FadeTo(CLIP* targetClip, float fadeDuration);
   void Update(float dt);
   void ClearTargets();

   CLIP* GetCurrentClip();
   Pose& GetCurrentPose();
   float GetPlaybackTimeOfCurrentClip();

private:

   CLIP*                               mCurrentClip;
   float                               mPlaybackTime;
   Skeleton                            mSkeleton;
   Pose                                mCurrentPose;
   bool                                mWasSkeletonSet;

   std::vector<TCrossFadeTarget<CLIP>> mTargets;
};

typedef TCrossFadeControllerMultiple<Clip> CrossFadeControllerMultiple;
typedef TCrossFadeControllerMultiple<FastClip> FastCrossFadeControllerMultiple;

#endif
