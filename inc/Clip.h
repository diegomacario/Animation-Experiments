#ifndef CLIP_H
#define CLIP_H

#include <vector>
#include <string>
#include "TransformTrack.h"
#include "Pose.h"

// TODO: If we unify the Track and FastTrack classes, this wouldn't have to be a template anymore
//       and we could delete the OptimizeClip function
template <typename TRACK>
class TClip
{
public:

   TClip();

   // TODO: Replace this operator with a GetTransformTrackOfJoint method
   TRACK&       operator[](unsigned int jointID);

   unsigned int GetNumberOfTransformTracks();

   unsigned int GetJointIDOfTransformTrack(unsigned int transfTrackIndex);
   void         SetJointIDOfTransformTrack(unsigned int transfTrackIndex, unsigned int jointID);

   std::string& GetName();
   void         SetName(const std::string& name);

   float        GetStartTime();
   float        GetEndTime();
   float        GetDuration();
   void         RecalculateDuration();

   bool         GetLooping();
   void         SetLooping(bool looping);

   float        Sample(Pose& ioPose, float time);

protected:

   float        AdjustTimeToBeWithinClip(float time);

   std::vector<TRACK> mTransformTracks;
   std::string        mName;
   float              mStartTime;
   float              mEndTime;
   bool               mLooping;
};

typedef TClip<TransformTrack> Clip;
typedef TClip<FastTransformTrack> FastClip;

FastClip OptimizeClip(Clip& clip);

#endif
