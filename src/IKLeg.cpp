#include "IKLeg.h"

// TODO: This isn't very nice. It's just here to avoid creating legs on the heap.
IKLeg::IKLeg()
   : mHipIndex(0)
   , mKneeIndex(0)
   , mAnkleIndex(0)
   , mToeIndex(0)
   , mAnkleToGroundOffset(0)
{

}

IKLeg::IKLeg(Skeleton& skeleton, const std::string& hipName, const std::string& kneeName, const std::string& ankleName, const std::string& toeName)
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
      std::string& nameOfCurrJoint = skeleton.GetJointName(jointIndex);

      if (nameOfCurrJoint == hipName)
      {
         mHipIndex = jointIndex;
      }
      else if (nameOfCurrJoint == kneeName)
      {
         mKneeIndex = jointIndex;
      }
      else if (nameOfCurrJoint == ankleName)
      {
         mAnkleIndex = jointIndex;
      }
      else if (nameOfCurrJoint == toeName)
      {
         mToeIndex = jointIndex;
      }
   }
}

void IKLeg::Solve(const Transform& modelTransform, Pose& pose, const glm::vec3& ankleTargetPosition, bool constrained)
{
   mSolver.SetLocalTransform(0, combine(modelTransform, pose.GetGlobalTransform(mHipIndex)));
   mSolver.SetLocalTransform(1, pose.GetLocalTransform(mKneeIndex));
   mSolver.SetLocalTransform(2, pose.GetLocalTransform(mAnkleIndex));
   mIKPose = pose;

   Transform target(ankleTargetPosition + glm::vec3(0, 1, 0) * mAnkleToGroundOffset, Q::quat(), glm::vec3(1, 1, 1));

   if (constrained)
   {
      glm::vec3 characterRight = modelTransform.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
      mSolver.SolveThreeJointLegWithConstraints(target, 1, characterRight);
   }
   else
   {
      mSolver.Solve(target);
   }

   Transform rootWorld = combine(modelTransform, pose.GetGlobalTransform(pose.GetParent(mHipIndex)));
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

const Pose& IKLeg::GetAdjustedPose()
{
   return mIKPose;
}
