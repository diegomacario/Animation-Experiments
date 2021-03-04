#include "CrossFadeControllerSingle.h"
#include "Blending.h"

template <typename CLIP>
TCrossFadeControllerSingle<CLIP>::TCrossFadeControllerSingle()
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mFading(false)
   , mTarget()
{

}

template <typename CLIP>
TCrossFadeControllerSingle<CLIP>::TCrossFadeControllerSingle(Skeleton& skeleton)
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mFading(false)
   , mTarget()
{
   SetSkeleton(skeleton);
}

template <typename CLIP>
void TCrossFadeControllerSingle<CLIP>::SetSkeleton(Skeleton& skeleton)
{
   mSkeleton       = skeleton;
   mCurrentPose    = mSkeleton.GetRestPose();
   mWasSkeletonSet = true;
}

template <typename CLIP>
void TCrossFadeControllerSingle<CLIP>::Play(CLIP* clip)
{
   // When asked to play a clip, clear the crossfade target
   mTarget = {};

   mCurrentClip  = clip;
   mPlaybackTime = clip->GetStartTime();
   mCurrentPose  = mSkeleton.GetRestPose();
}

template <typename CLIP>
void TCrossFadeControllerSingle<CLIP>::FadeTo(CLIP* targetClip, float fadeDuration)
{
   // If no clip has been set, simply play the target clip since there is no clip to fade from
   if (mCurrentClip == nullptr)
   {
      Play(targetClip);
      return;
   }

   if (mFading)
   {
      // If the clip that we are currently fading to is the same as the target clip, then ignore the target clip
      // Otherwise, we would be fading between identical clips
      if (mTarget.mClip == targetClip)
      {
         return;
      }

      // Interrupt the current fade and update the target
      mCurrentClip  = mTarget.mClip;
      mPlaybackTime = mTarget.mFadeTime;
      mCurrentPose  = mTarget.mPose;
      mTarget       = TCrossFadeTarget<CLIP>(targetClip, mSkeleton.GetRestPose(), fadeDuration, false);
   }
   else
   {
      // If we are not fading and the current clip is the same as the target clip, then ignore the target clip
      // Otherwise, we would be fading between identical clips
      if (mCurrentClip == targetClip)
      {
         return;
      }

      // Update the target and begin fading
      mTarget = TCrossFadeTarget<CLIP>(targetClip, mSkeleton.GetRestPose(), fadeDuration, false);
      mFading = true;
   }
}

template <typename CLIP>
void TCrossFadeControllerSingle<CLIP>::Update(float dt)
{
   // We cannot update without a current clip or a skeleton
   if (mCurrentClip == nullptr || !mWasSkeletonSet)
   {
      return;
   }

   if (mFading)
   {
      // Check if the fade has been completed
      if (mTarget.mFadeTime >= mTarget.mFadeDuration)
      {
         // If yes, replace the current clip with the clip of the target that just expired and reset the expired target
         mCurrentClip  = mTarget.mClip;
         mPlaybackTime = mTarget.mPlaybackTime;
         mCurrentPose  = mTarget.mPose;
         mTarget = {};
         mFading = false;
      }
   }

   // Sample the current clip
   mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

   if (mFading)
   {
      // Sample the clip of the target
      mTarget.mPlaybackTime = mTarget.mClip->Sample(mTarget.mPose, mTarget.mPlaybackTime + dt);
      // Increase the fade time of the target
      mTarget.mFadeTime += dt;
      // Calculate the interpolation factor
      float t = mTarget.mFadeTime / mTarget.mFadeDuration;
      if (t > 1.0f)
      {
         t = 1.0f;
      }

      // Blend the poses that we sampled from the current clip and the clip of the target
      Blend(mCurrentPose, mTarget.mPose, t, -1, mCurrentPose);
   }
}

template <typename CLIP>
CLIP* TCrossFadeControllerSingle<CLIP>::GetCurrentClip()
{
   return mCurrentClip;
}

template <typename CLIP>
Pose& TCrossFadeControllerSingle<CLIP>::GetCurrentPose()
{
   return mCurrentPose;
}

// Instantiate the desired CrossFadeControllerSingle classes from the CrossFadeControllerSingle class template
template class TCrossFadeControllerSingle<Clip>;
template class TCrossFadeControllerSingle<FastClip>;
