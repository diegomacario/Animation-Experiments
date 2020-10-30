#ifndef TRANSFORM_TRACK_H
#define TRANSFORM_TRACK_H

#include "Track.h"
#include "Transform.h"

// TODO: If we unify the Track and FastTrack classes, this wouldn't have to be a template anymore
//       and we could delete the OptimizeTransformTrack function
template <typename VTRACK, typename QTRACK>
class TTransformTrack
{
public:

   TTransformTrack();

   unsigned int GetJointID();
   void         SetJointID(unsigned int id);

   VTRACK&      GetPositionTrack();
   QTRACK&      GetRotationTrack();
   VTRACK&      GetScaleTrack();

   float        GetStartTime();
   float        GetEndTime();

   bool         IsValid();

   Transform    Sample(const Transform& defaultTransform, float time, bool looping);

protected:

   // Each TransformTrack stores the ID of the joint it animates
   unsigned int mJointID;

   VTRACK       mPosition;
   QTRACK       mRotation;
   VTRACK       mScale;
};

typedef TTransformTrack<VectorTrack, QuaternionTrack>         TransformTrack;
typedef TTransformTrack<FastVectorTrack, FastQuaternionTrack> FastTransformTrack;

FastTransformTrack OptimizeTransformTrack(TransformTrack& transformTrack);

#endif
