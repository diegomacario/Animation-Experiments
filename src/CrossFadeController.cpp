#include "CrossFadeController.h"
#include "Blending.h"

CrossFadeController::CrossFadeController()
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mTargets()
{

}

CrossFadeController::CrossFadeController(Skeleton& skeleton)
   : mCurrentClip(nullptr)
   , mPlaybackTime(0.0f)
   , mSkeleton()
   , mCurrentPose()
   , mWasSkeletonSet(false)
   , mTargets()
{
   SetSkeleton(skeleton);
}

void CrossFadeController::SetSkeleton(Skeleton& skeleton)
{
   mSkeleton       = skeleton;
   mCurrentPose    = mSkeleton.GetRestPose();
   mWasSkeletonSet = true;
}

void CrossFadeController::Play(Clip* clip)
{
   // When asked to play a clip, we clear all the crossfade targets
   mTargets = {};

   mCurrentClip  = clip;
   mPlaybackTime = clip->GetStartTime();
   mCurrentPose  = mSkeleton.GetRestPose();
}

void CrossFadeController::FadeTo(Clip* targetClip, float fadeDuration)
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
      if (mTargets.back().mClip == targetClip)
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
   mTargets.emplace(targetClip, mSkeleton.GetRestPose(), fadeDuration);
}

void CrossFadeController::Update(float dt)
{
   // We cannot update without a current clip or a skeleton
   if (mCurrentClip == nullptr || !mWasSkeletonSet)
   {
      return;
   }

   if (!mTargets.empty())
   {
      CrossFadeTarget& currentTarget = mTargets.front();

      // Check if the first fade in the queue has been completed
      if (currentTarget.mFadeTime >= currentTarget.mFadeDuration)
      {
         // If yes, replace the current clip with the clip of the target that just expired and remove the expired target from the queue
         mCurrentClip  = currentTarget.mClip;
         mPlaybackTime = currentTarget.mFadeTime;
         mCurrentPose  = currentTarget.mPose;
         mTargets.pop();
      }
   }

   // Sample the current clip
   mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

   if (!mTargets.empty())
   {
      CrossFadeTarget& currentTarget = mTargets.front();

      // Sample the clip of the first target in the queue
      currentTarget.mPlaybackTime = currentTarget.mClip->Sample(currentTarget.mPose, currentTarget.mPlaybackTime + dt);
      // Increase the fade time of the first target
      currentTarget.mFadeTime += dt;
      // Calculate the interpolation factor
      float t = currentTarget.mFadeTime / currentTarget.mFadeDuration;
      if (t > 1.0f)
      {
         t = 1.0f;
      }

      // Blend the poses that we sampled from the current clip and the clip of the first target in the queue
      Blend(mCurrentPose, currentTarget.mPose, t, -1, mCurrentPose);
   }
}

Clip* CrossFadeController::GetcurrentClip()
{
   return mCurrentClip;
}

Pose& CrossFadeController::GetCurrentPose()
{
   return mCurrentPose;
}