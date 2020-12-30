#ifndef IK_LEG_H
#define IK_LEG_H

#include "Skeleton.h"
#include "Track.h"
#include "FABRIKSolver.h"

class IKLeg
{
public:

   IKLeg();
   IKLeg(Skeleton& skeleton, const std::string& hipName, const std::string& kneeName, const std::string& ankleName, const std::string& toeName);

   void               SolveForLeg(const Transform& model, Pose& pose, const glm::vec3& ankleTargetPosition);

   unsigned int       GetHipIndex();
   unsigned int       GetKneeIndex();
   unsigned int       GetAnkleIndex();
   unsigned int       GetToeIndex();

   void               SetAnkleOffset(float ankleOffset);

   const ScalarTrack& GetPinTrack();
   void               SetPinTrack(const ScalarTrack& pinTrack);

   const Pose&        GetAdjustedPose();

private:

   unsigned int mHipIndex;
   unsigned int mKneeIndex;
   unsigned int mAnkleIndex;
   unsigned int mToeIndex;

   float        mAnkleToGroundOffset;

   ScalarTrack  mPinTrack;
   FABRIKSolver mSolver;
   Pose         mIKPose;
};

#endif
