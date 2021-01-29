#include <glm/gtx/compatibility.hpp>

#include "IKCrossFadeController.h"
#include "Blending.h"

template TIKCrossFadeController<Clip>;
template TIKCrossFadeController<FastClip>;

template <typename CLIP>
TIKCrossFadeController<CLIP>::TIKCrossFadeController()
   : mCurrentClip(nullptr)
   , mCurrentLeftFootPinTrack(nullptr)
   , mCurrentRightFootPinTrack(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mCurrentLeftFootPinTrackValue(0.0f)
   , mCurrentRightFootPinTrackValue(0.0f)
   , mWasSkeletonSet(false)
   , mLock(false)
   , mTargets()
{

}

template <typename CLIP>
TIKCrossFadeController<CLIP>::TIKCrossFadeController(Skeleton& skeleton)
   : mCurrentClip(nullptr)
   , mCurrentLeftFootPinTrack(nullptr)
   , mCurrentRightFootPinTrack(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mCurrentLeftFootPinTrackValue(0.0f)
   , mCurrentRightFootPinTrackValue(0.0f)
   , mWasSkeletonSet(false)
   , mLock(false)
   , mTargets()
{
   SetSkeleton(skeleton);
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::SetSkeleton(Skeleton& skeleton)
{
   mSkeleton       = skeleton;
   mCurrentPose    = mSkeleton.GetRestPose();
   mWasSkeletonSet = true;
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::Play(CLIP* clip, ScalarTrack* leftFootPinTrack, ScalarTrack* rightFootPinTrack, bool lock)
{
   // When asked to play a clip, we clear all the crossfade targets
   mTargets.clear();

   mCurrentClip              = clip;
   mCurrentLeftFootPinTrack  = leftFootPinTrack;
   mCurrentRightFootPinTrack = rightFootPinTrack;
   mPlaybackTime             = clip->GetStartTime();
   mCurrentPose              = mSkeleton.GetRestPose();
   mLock                     = lock;
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::FadeTo(CLIP* targetClip, ScalarTrack* leftFootPinTrack, ScalarTrack* rightFootPinTrack, float fadeDuration, bool lock)
{
   if (mLock)
   {
      return;
   }

   // If no clip has been set, simply play the target clip since there is no clip to fade from
   if (mCurrentClip == nullptr)
   {
      Play(targetClip, leftFootPinTrack, rightFootPinTrack, lock);
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
      // If there are no clips in the queue and the current clip is the same as the target clip,
      // then don't add the target clip to the queue unless the current clip is already finished
      if (mCurrentClip == targetClip)
      {
         if (mCurrentClip->IsTimePastEnd(mPlaybackTime))
         {
            mTargets.emplace_back(targetClip, leftFootPinTrack, rightFootPinTrack, mSkeleton.GetRestPose(), fadeDuration, lock);
         }

         return;
      }
   }

   // Add the target clip to the queue
   mTargets.emplace_back(targetClip, leftFootPinTrack, rightFootPinTrack, mSkeleton.GetRestPose(), fadeDuration, lock);
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::Update(float dt)
{
   // We cannot update without a current clip or a skeleton
   if (mCurrentClip == nullptr || !mWasSkeletonSet)
   {
      return;
   }

   if (mLock)
   {
      mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

      float normalizedPlaybackTime = (mPlaybackTime - mCurrentClip->GetStartTime()) / mCurrentClip->GetDuration();
      mCurrentLeftFootPinTrackValue = mCurrentLeftFootPinTrack->Sample(normalizedPlaybackTime, true);
      mCurrentRightFootPinTrackValue = mCurrentRightFootPinTrack->Sample(normalizedPlaybackTime, true);
   }
   else
   {
      unsigned int numTargets = static_cast<unsigned int>(mTargets.size());
      for (unsigned int targetIndex = 0; targetIndex < numTargets; ++targetIndex)
      {
         if (mTargets[targetIndex].mFadeTime >= mTargets[targetIndex].mFadeDuration)
         {
            mCurrentClip              = mTargets[targetIndex].mClip;
            mCurrentLeftFootPinTrack  = mTargets[targetIndex].mLeftFootPinTrack;
            mCurrentRightFootPinTrack = mTargets[targetIndex].mRightFootPinTrack;
            mPlaybackTime             = mTargets[targetIndex].mPlaybackTime;
            mCurrentPose              = mTargets[targetIndex].mPose;
            mLock                     = mTargets[targetIndex].mLock;

            mTargets.erase(mTargets.begin() + targetIndex);

            // If the new clip locks the crossfade controller, we sample it and return immediately to avoid blending it with the targets
            if (mLock)
            {
               mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

               float normalizedPlaybackTime   = (mPlaybackTime - mCurrentClip->GetStartTime()) / mCurrentClip->GetDuration();
               mCurrentLeftFootPinTrackValue  = mCurrentLeftFootPinTrack->Sample(normalizedPlaybackTime, true);
               mCurrentRightFootPinTrackValue = mCurrentRightFootPinTrack->Sample(normalizedPlaybackTime, true);
               return;
            }

            break;
         }
      }

      mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

      float normalizedPlaybackTime   = (mPlaybackTime - mCurrentClip->GetStartTime()) / mCurrentClip->GetDuration();
      mCurrentLeftFootPinTrackValue  = mCurrentLeftFootPinTrack->Sample(normalizedPlaybackTime, true);
      mCurrentRightFootPinTrackValue = mCurrentRightFootPinTrack->Sample(normalizedPlaybackTime, true);

      numTargets = static_cast<unsigned int>(mTargets.size());
      for (unsigned int targetIndex = 0; targetIndex < numTargets; ++targetIndex)
      {
         TIKCrossFadeTarget<CLIP>& target = mTargets[targetIndex];

         target.mPlaybackTime = target.mClip->Sample(target.mPose, target.mPlaybackTime + dt);

         float normalizedPlaybackTime   = (target.mPlaybackTime - target.mClip->GetStartTime()) / target.mClip->GetDuration();
         target.mLeftFootPinTrackValue  = target.mLeftFootPinTrack->Sample(normalizedPlaybackTime, true);
         target.mRightFootPinTrackValue = target.mRightFootPinTrack->Sample(normalizedPlaybackTime, true);

         target.mFadeTime += dt;
         float t = target.mFadeTime / target.mFadeDuration;
         if (t > 1.0f)
         {
            t = 1.0f;
         }

         // Blend the poses
         Blend(mCurrentPose, target.mPose, t, -1, mCurrentPose);

         // Blend the pin tracks
         mCurrentLeftFootPinTrackValue = glm::lerp(mCurrentLeftFootPinTrackValue, target.mLeftFootPinTrackValue, t);
         mCurrentRightFootPinTrackValue = glm::lerp(mCurrentRightFootPinTrackValue, target.mRightFootPinTrackValue, t);
      }
   }
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::ClearTargets()
{
   mTargets.clear();
}

template <typename CLIP>
CLIP* TIKCrossFadeController<CLIP>::GetCurrentClip()
{
   return mCurrentClip;
}

template <typename CLIP>
Pose& TIKCrossFadeController<CLIP>::GetCurrentPose()
{
   return mCurrentPose;
}

template <typename CLIP>
float TIKCrossFadeController<CLIP>::GetCurrentLeftFootPinTrackValue()
{
   return mCurrentLeftFootPinTrackValue;
}

template <typename CLIP>
float TIKCrossFadeController<CLIP>::GetCurrentRightFootPinTrackValue()
{
   return mCurrentRightFootPinTrackValue;
}

template <typename CLIP>
bool TIKCrossFadeController<CLIP>::IsCurrentClipFinished()
{
   return mCurrentClip->IsTimePastEnd(mPlaybackTime);
}

template <typename CLIP>
bool TIKCrossFadeController<CLIP>::IsLocked()
{
   return mLock;
}

template <typename CLIP>
void TIKCrossFadeController<CLIP>::Unlock()
{
   mLock = false;
}

template <typename CLIP>
float TIKCrossFadeController<CLIP>::GetPlaybackTime()
{
   return mPlaybackTime;
}
