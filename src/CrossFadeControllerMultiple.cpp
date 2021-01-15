#include "CrossFadeControllerMultiple.h"
#include "Blending.h"

template TCrossFadeControllerMultiple<Clip>;
template TCrossFadeControllerMultiple<FastClip>;

template <typename CLIP>
TCrossFadeControllerMultiple<CLIP>::TCrossFadeControllerMultiple()
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mTargets()
{

}

template <typename CLIP>
TCrossFadeControllerMultiple<CLIP>::TCrossFadeControllerMultiple(Skeleton& skeleton)
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mTargets()
{
   SetSkeleton(skeleton);
}

template <typename CLIP>
void TCrossFadeControllerMultiple<CLIP>::SetSkeleton(Skeleton& skeleton)
{
   mSkeleton       = skeleton;
   mCurrentPose    = mSkeleton.GetRestPose();
   mWasSkeletonSet = true;
}

template <typename CLIP>
void TCrossFadeControllerMultiple<CLIP>::Play(CLIP* clip)
{
   // When asked to play a clip, we clear all the crossfade targets
   mTargets.clear();

   mCurrentClip  = clip;
   mPlaybackTime = clip->GetStartTime();
   mCurrentPose  = mSkeleton.GetRestPose();
}

template <typename CLIP>
void TCrossFadeControllerMultiple<CLIP>::FadeTo(CLIP* targetClip, float fadeDuration)
{
   // If no clip has been set, simply play the target clip since there is no clip to fade from
   if (mCurrentClip == nullptr)
   {
      Play(targetClip);
      return;
   }

   if (mTargets.size() >= 1)
   {
      // If the last clip in the queue is the same as the target clip, then don't add the target clip to the queue
      // Otherwise, we would eventually fade between identical clips
      if (mTargets[mTargets.size() - 1].mClip == targetClip)
      {
         return;
      }
   }
   else
   {
      // If there are no clips in the queue and the current clip is the same as the target clip, then don't add the target clip to the queue
      // Otherwise, we would immediately fade between identical clips
      if (mCurrentClip == targetClip)
      {
          return;
      }
   }

   // Add the target clip to the queue
   mTargets.emplace_back(targetClip, mSkeleton.GetRestPose(), fadeDuration);
}

template <typename CLIP>
void TCrossFadeControllerMultiple<CLIP>::Update(float dt)
{
   // We cannot update without a current clip or a skeleton
   if (mCurrentClip == nullptr || !mWasSkeletonSet)
   {
      return;
   }

   unsigned int numTargets = static_cast<unsigned int>(mTargets.size());
   for (unsigned int targetIndex = 0; targetIndex < numTargets; ++targetIndex)
   {
      if (mTargets[targetIndex].mFadeTime >= mTargets[targetIndex].mFadeDuration)
      {
         mCurrentClip  = mTargets[targetIndex].mClip;
         mPlaybackTime = mTargets[targetIndex].mFadeTime;
         mCurrentPose  = mTargets[targetIndex].mPose;

         mTargets.erase(mTargets.begin() + targetIndex);
         break;
      }
   }

   numTargets    = static_cast<unsigned int>(mTargets.size());
   //mCurrentPose  = mSkeleton.GetRestPose();
   mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

   for (unsigned int targetIndex = 0; targetIndex < numTargets; ++targetIndex)
   {
      TCrossFadeTarget<CLIP>& target = mTargets[targetIndex];
      target.mPlaybackTime = target.mClip->Sample(target.mPose, target.mPlaybackTime + dt);
      target.mFadeTime += dt;
      float t = target.mFadeTime / target.mFadeDuration;
      if (t > 1.0f)
      {
         t = 1.0f;
      }

      Blend(mCurrentPose, target.mPose, t, -1, mCurrentPose);
   }
}

template <typename CLIP>
void TCrossFadeControllerMultiple<CLIP>::ClearTargets()
{
   mTargets.clear();
}

template <typename CLIP>
CLIP* TCrossFadeControllerMultiple<CLIP>::GetCurrentClip()
{
   return mCurrentClip;
}

template <typename CLIP>
Pose& TCrossFadeControllerMultiple<CLIP>::GetCurrentPose()
{
   return mCurrentPose;
}

template <typename CLIP>
float TCrossFadeControllerMultiple<CLIP>::GetPlaybackTimeOfCurrentClip()
{
   return mPlaybackTime;
}
