#include "IKLeg.h"

IKLeg::IKLeg(Skeleton& skeleton, const std::string& hip, const std::string& knee, const std::string& ankle, const std::string& toe)
   : mHipIndex(0)
   , mKneeIndex(0)
   , mAnkleIndex(0)
   , mToeIndex(0)
   , mAnkleToGroundOffset(0)
{
   // There are 3 joints in the IK chain of a leg: the hip, the knee and the ankle
   mSolver.SetNumberOfJointsInIKChain(3);

   for (unsigned int jointIndex = 0, numJoints = skeleton.GetRestPose().GetNumberOfJoints(); jointIndex < numJoints; ++jointIndex)
   {
      std::string& jointName = skeleton.GetJointName(jointIndex);

      if (jointName == hip)
      {
         mHipIndex = jointIndex;
      }
      else if (jointName == knee)
      {
         mKneeIndex = jointIndex;
      }
      else if (jointName == ankle)
      {
         mAnkleIndex = jointIndex;
      }
      else if (jointName == toe)
      {
         mToeIndex = jointIndex;
      }
   }
}

void IKLeg::SolveForLeg(const Transform& model, Pose& pose, const glm::vec3& ankleTargetPosition)
{
   mSolver.SetLocalTransform(0, combine(model, pose.GetGlobalTransform(mHipIndex)));
   mSolver.SetLocalTransform(1, pose.GetLocalTransform(mKneeIndex));
   mSolver.SetLocalTransform(2, pose.GetLocalTransform(mAnkleIndex));
   mIKPose = pose;

   Transform target(ankleTargetPosition + glm::vec3(0, 1, 0) * mAnkleToGroundOffset, Q::quat(), glm::vec3(1, 1, 1));
   mSolver.Solve(target);

   Transform rootWorld = combine(model, pose.GetGlobalTransform(pose.GetParent(mHipIndex)));
   mIKPose.SetLocalTransform(mHipIndex, combine(inverse(rootWorld), mSolver.GetLocalTransform(0)));
   mIKPose.SetLocalTransform(mKneeIndex, mSolver.GetLocalTransform(1));
   mIKPose.SetLocalTransform(mAnkleIndex, mSolver.GetLocalTransform(2));
}

unsigned int IKLeg::GetHipIndex()
{
   return mHipIndex;
}

unsigned int IKLeg::GetKneeIndex()
{
   return mKneeIndex;
}

unsigned int IKLeg::GetAnkleIndex()
{
   return mAnkleIndex;
}

unsigned int IKLeg::GetToeIndex()
{
   return mToeIndex;
}

void IKLeg::SetAnkleOffset(float ankleOffset)
{
   mAnkleToGroundOffset = ankleOffset;
}

const ScalarTrack& IKLeg::GetPinTrack()
{
   return mPinTrack;
}

void IKLeg::SetPinTrack(const ScalarTrack& pinTrack)
{
   mPinTrack = pinTrack;
}

const Pose& IKLeg::GetAdjustedPose()
{
   return mIKPose;
}
