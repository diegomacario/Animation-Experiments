#include "TransformTrack.h"

template TTransformTrack<VectorTrack, QuaternionTrack>;
template TTransformTrack<FastVectorTrack, FastQuaternionTrack>;

template <typename VTRACK, typename QTRACK>
TTransformTrack<VTRACK, QTRACK>::TTransformTrack()
{
   mID = 0;
}

template <typename VTRACK, typename QTRACK>
unsigned int TTransformTrack<VTRACK, QTRACK>::GetID()
{
   return mID;
}

template <typename VTRACK, typename QTRACK>
void TTransformTrack<VTRACK, QTRACK>::SetID(unsigned int id)
{
   mID = id;
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
   bool  foundValidValue = false;

   if (mPosition.GetNumberOfFrames() > 1)
   {
      startTime = mPosition.GetStartTime();
      foundValidValue = true;
   }

   if (mRotation.GetNumberOfFrames() > 1)
   {
      float rotStartTime = mRotation.GetStartTime();
      if (rotStartTime < startTime || !foundValidValue)
      {
         startTime = rotStartTime;
         foundValidValue = true;
      }
   }

   if (mScale.GetNumberOfFrames() > 1)
   {
      float scaleStartTime = mScale.GetStartTime();
      if (scaleStartTime < startTime || !foundValidValue)
      {
         startTime = scaleStartTime;
         foundValidValue = true;
      }
   }

   return startTime;
}

template <typename VTRACK, typename QTRACK>
float TTransformTrack<VTRACK, QTRACK>::GetEndTime()
{
   // Find the latest end time out of the position, rotation and scale tracks

   float endTime = 0.0f;
   bool  foundValidValue = false;

   if (mPosition.GetNumberOfFrames() > 1)
   {
      endTime = mPosition.GetEndTime();
      foundValidValue = true;
   }

   if (mRotation.GetNumberOfFrames() > 1)
   {
      float rotEndTime = mRotation.GetEndTime();
      if (rotEndTime > endTime || !foundValidValue)
      {
         endTime = rotEndTime;
         foundValidValue = true;
      }
   }

   if (mScale.GetNumberOfFrames() > 1)
   {
      float scaleEndTime = mScale.GetEndTime();
      if (scaleEndTime > endTime || !foundValidValue)
      {
         endTime = scaleEndTime;
         foundValidValue = true;
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

   result.SetID(transformTrack.GetID());

   result.GetPositionTrack() = OptimizeTrack<glm::vec3, 3>(transformTrack.GetPositionTrack());
   result.GetRotationTrack() = OptimizeTrack<Q::quat, 4>(transformTrack.GetRotationTrack());
   result.GetScaleTrack()    = OptimizeTrack<glm::vec3, 3>(transformTrack.GetScaleTrack());

   return result;
}
