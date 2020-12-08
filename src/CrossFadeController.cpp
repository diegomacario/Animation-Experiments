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
   mTargets.clear();

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
   mTargets.push_back(CrossFadeTarget(targetClip, mSkeleton.GetRestPose(), fadeDuration));
}

void CrossFadeController::Update(float dt)
{
   // We cannot update without a current clip or a skeleton
   if (mCurrentClip == nullptr || !mWasSkeletonSet)
   {
      return;
   }

   unsigned int numTargets = mTargets.size();
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

   numTargets    = mTargets.size();
   //mCurrentPose  = mSkeleton.GetRestPose();
   mPlaybackTime = mCurrentClip->Sample(mCurrentPose, mPlaybackTime + dt);

   for (unsigned int targetIndex = 0; targetIndex < numTargets; ++targetIndex)
   {
      CrossFadeTarget& target = mTargets[targetIndex];
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

Clip* CrossFadeController::GetcurrentClip()
{
   return mCurrentClip;
}

Pose& CrossFadeController::GetCurrentPose()
{
   return mCurrentPose;
}
