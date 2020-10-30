#include "TransformTrack.h"

template TTransformTrack<VectorTrack, QuaternionTrack>;
template TTransformTrack<FastVectorTrack, FastQuaternionTrack>;

template <typename VTRACK, typename QTRACK>
TTransformTrack<VTRACK, QTRACK>::TTransformTrack()
   : mJointID(0)
{

}

template <typename VTRACK, typename QTRACK>
unsigned int TTransformTrack<VTRACK, QTRACK>::GetJointID()
{
   return mJointID;
}

template <typename VTRACK, typename QTRACK>
void TTransformTrack<VTRACK, QTRACK>::SetJointID(unsigned int id)
{
   mJointID = id;
}

template <typename VTRACK, typename QTRACK>
VTRACK& TTransformTrack<VTRACK, QTRACK>::GetPositionTrack()
{
   return mPosition;
}

template <typename VTRACK, typename QTRACK>
QTRACK& TTransformTrack<VTRACK, QTRACK>::GetRotationTrack()
{
   return mRotation;
}

template <typename VTRACK, typename QTRACK>
VTRACK& TTransformTrack<VTRACK, QTRACK>::GetScaleTrack()
{
   return mScale;
}

template <typename VTRACK, typename QTRACK>
bool TTransformTrack<VTRACK, QTRACK>::IsValid()
{
   return (mPosition.GetNumberOfFrames() > 1 ||
           mRotation.GetNumberOfFrames() > 1 ||
           mScale.GetNumberOfFrames() > 1);
}

template <typename VTRACK, typename QTRACK>
float TTransformTrack<VTRACK, QTRACK>::GetStartTime()
{
   // Find the earliest start time out of the position, rotation and scale tracks

   float startTime = 0.0f;
   bool  initialStartTimeFound = false;

   if (mPosition.GetNumberOfFrames() > 1)
   {
      startTime = mPosition.GetStartTime();
      initialStartTimeFound = true;
   }

   if (mRotation.GetNumberOfFrames() > 1)
   {
      float rotStartTime = mRotation.GetStartTime();
      if (rotStartTime < startTime || !initialStartTimeFound)
      {
         startTime = rotStartTime;
         initialStartTimeFound = true;
      }
   }

   if (mScale.GetNumberOfFrames() > 1)
   {
      float scaleStartTime = mScale.GetStartTime();
      if (scaleStartTime < startTime || !initialStartTimeFound)
      {
         startTime = scaleStartTime;
         initialStartTimeFound = true;
      }
   }

   return startTime;
}

template <typename VTRACK, typename QTRACK>
float TTransformTrack<VTRACK, QTRACK>::GetEndTime()
{
   // Find the latest end time out of the position, rotation and scale tracks

   float endTime = 0.0f;
   bool  initialEndTimeFound = false;

   if (mPosition.GetNumberOfFrames() > 1)
   {
      endTime = mPosition.GetEndTime();
      initialEndTimeFound = true;
   }

   if (mRotation.GetNumberOfFrames() > 1)
   {
      float rotEndTime = mRotation.GetEndTime();
      if (rotEndTime > endTime || !initialEndTimeFound)
      {
         endTime = rotEndTime;
         initialEndTimeFound = true;
      }
   }

   if (mScale.GetNumberOfFrames() > 1)
   {
      float scaleEndTime = mScale.GetEndTime();
      if (scaleEndTime > endTime || !initialEndTimeFound)
      {
         endTime = scaleEndTime;
         initialEndTimeFound = true;
      }
   }

   return endTime;
}

template <typename VTRACK, typename QTRACK>
Transform TTransformTrack<VTRACK, QTRACK>::Sample(const Transform& defaultTransform, float time, bool looping)
{
   // Assign default values in case any of the tracks is invalid
   Transform result = defaultTransform;

   // Only sample the tracks that are animated

   if (mPosition.GetNumberOfFrames() > 1)
   {
      result.position = mPosition.Sample(time, looping);
   }

   if (mRotation.GetNumberOfFrames() > 1)
   {
      result.rotation = mRotation.Sample(time, looping);
   }

   if (mScale.GetNumberOfFrames() > 1)
   {
      result.scale = mScale.Sample(time, looping);
   }

   return result;
}

FastTransformTrack OptimizeTransformTrack(TransformTrack& transformTrack)
{
   FastTransformTrack result;

   result.SetJointID(transformTrack.GetJointID());

   result.GetPositionTrack() = OptimizeTrack<glm::vec3, 3>(transformTrack.GetPositionTrack());
   result.GetRotationTrack() = OptimizeTrack<Q::quat, 4>(transformTrack.GetRotationTrack());
   result.GetScaleTrack()    = OptimizeTrack<glm::vec3, 3>(transformTrack.GetScaleTrack());

   return result;
}
