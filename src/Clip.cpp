#include "Clip.h"

template TClip<TransformTrack>;
template TClip<FastTransformTrack>;

template <typename TRACK>
TClip<TRACK>::TClip()
   : mName("Unnamed")
   , mStartTime(0.0f)
   , mEndTime(0.0f)
   , mLooping(true)
{

}

template <typename TRACK>
TRACK& TClip<TRACK>::operator[](unsigned int jointID)
{
   // Loop over the transform tracks and compare their joint IDs with the desired one
   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = static_cast<unsigned int>(mTransformTracks.size());
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      if (mTransformTracks[transfTrackIndex].GetJointID() == jointID)
      {
         return mTransformTracks[transfTrackIndex];
      }
   }

   // If a transform track that animates the desired joint doesn't exist,
   // we create an empty one and return it
   mTransformTracks.push_back(TRACK());
   mTransformTracks[mTransformTracks.size() - 1].SetJointID(jointID);
   return mTransformTracks[mTransformTracks.size() - 1];
}

template <typename TRACK>
unsigned int TClip<TRACK>::GetNumberOfTransformTracks()
{
   return static_cast<unsigned int>(mTransformTracks.size());
}

template <typename TRACK>
unsigned int TClip<TRACK>::GetJointIDOfTransformTrack(unsigned int transfTrackIndex)
{
   return mTransformTracks[transfTrackIndex].GetJointID();
}

template <typename TRACK>
void TClip<TRACK>::SetJointIDOfTransformTrack(unsigned int transfTrackIndex, unsigned int jointID)
{
   return mTransformTracks[transfTrackIndex].SetJointID(jointID);
}

template <typename TRACK>
std::string& TClip<TRACK>::GetName()
{
   return mName;
}

template <typename TRACK>
void TClip<TRACK>::SetName(const std::string& name)
{
   mName = name;
}

template <typename TRACK>
float TClip<TRACK>::GetStartTime()
{
   return mStartTime;
}

template <typename TRACK>
float TClip<TRACK>::GetEndTime()
{
   return mEndTime;
}

template <typename TRACK>
float TClip<TRACK>::GetDuration()
{
   return mEndTime - mStartTime;
}

template <typename TRACK>
void TClip<TRACK>::RecalculateDuration()
{
   // Reset the start time and the end time
   mStartTime = 0.0f;
   mEndTime   = 0.0f;

   // Loop over the transform tracks and find the earliest start time and the latest end time
   bool initialStartTimeFound = false;
   bool initialEndTimeFound   = false;
   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = static_cast<unsigned int>(mTransformTracks.size());
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      if (mTransformTracks[transfTrackIndex].IsValid())
      {
         float transfTrackStartTime = mTransformTracks[transfTrackIndex].GetStartTime();
         float transfTrackEndTime   = mTransformTracks[transfTrackIndex].GetEndTime();

         if (transfTrackStartTime < mStartTime || !initialStartTimeFound)
         {
            mStartTime = transfTrackStartTime;
            initialStartTimeFound = true;
         }

         if (transfTrackEndTime > mEndTime || !initialEndTimeFound)
         {
            mEndTime = transfTrackEndTime;
            initialEndTimeFound = true;
         }
      }
   }
}

template <typename TRACK>
bool TClip<TRACK>::GetLooping()
{
   return mLooping;
}

template <typename TRACK>
void TClip<TRACK>::SetLooping(bool looping)
{
   mLooping = looping;
}

template <typename TRACK>
float TClip<TRACK>::Sample(Pose& ioPose, float time)
{
   if (GetDuration() <= 0.0f)
   {
      // If the duration of the clip is smaller than or equal to zero, it's invalid
      return 0.0f;
   }

   time = AdjustTimeToBeWithinClip(time);

   // Loop over the transform tracks
   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = static_cast<unsigned int>(mTransformTracks.size());
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      // Get the ID of the joint that the current transform track animates
      // and the default local transform of that joint
      // TODO: Clarify if ioPose is always the rest pose
      unsigned int jointID = mTransformTracks[transfTrackIndex].GetJointID();
      Transform localTransfOfJoint = ioPose.GetLocalTransform(jointID);

      // Sample the transform track to get the animated local transform of the joint
      // If the position, rotation or scale of the joint are not animated, then the default values are kept unmodified
      Transform animatedLocalTransfOfJoint = mTransformTracks[transfTrackIndex].Sample(localTransfOfJoint, time, mLooping);

      // Store the animated local transform of the joint in the pose
      // By the end of this loop, the pose is animated
      ioPose.SetLocalTransform(jointID, animatedLocalTransfOfJoint);
   }

   return time;
}

template <typename TRACK>
float TClip<TRACK>::AdjustTimeToBeWithinClip(float time)
{
   if (mLooping)
   {
      float duration = mEndTime - mStartTime;
      if (duration <= 0.0f)
      {
         // If the duration of the clip is smaller than or equal to zero, it's invalid
         return 0.0f;
      }

      // If looping, adjust the time so that it's inside the range of the clip
      // The code below can take a time before the start, in between the start and the end, or after the end
      // and produce a properly looped value
      time = glm::mod(time - mStartTime, duration);
      if (time < 0.0f)
      {
         time += duration;
      }
      time += mStartTime;
   }
   else
   {
      // If not looping, any time before the start should clamp to the start time
      // and any time after the end should clamp to the end time
      if (time < mStartTime)
      {
         time = mStartTime;
      }

      if (time > mEndTime)
      {
         time = mEndTime;
      }
   }
   return time;
}

FastClip OptimizeClip(Clip& clip)
{
   FastClip result;

   result.SetName(clip.GetName());
   result.SetLooping(clip.GetLooping());

   for (unsigned int transfTrackIndex = 0,
        numTransfTracks = clip.GetNumberOfTransformTracks();
        transfTrackIndex < numTransfTracks;
        ++transfTrackIndex)
   {
      unsigned int jointID = clip.GetJointIDOfTransformTrack(transfTrackIndex);
      result[jointID] = OptimizeTransformTrack(clip[jointID]);
   }

   result.RecalculateDuration();

   return result;
}
