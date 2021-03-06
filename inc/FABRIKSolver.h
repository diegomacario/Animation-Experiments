#ifndef FABRIK_SOLVER_H
#define FABRIK_SOLVER_H

#include <vector>
#include "Transform.h"

class FABRIKSolver
{
public:

   FABRIKSolver();

   unsigned int GetNumberOfJointsInIKChain();
   void         SetNumberOfJointsInIKChain(unsigned int numJoints);

   Transform    GetLocalTransform(unsigned int jointIndex) const;
   void         SetLocalTransform(unsigned int jointIndex, const Transform& transform);
   Transform    GetGlobalTransform(unsigned int jointIndex);

   unsigned int GetNumberOfIterations();
   void         SetNumberOfIterations(unsigned int numIterations);

   float        GetThreshold();
   void         SetThreshold(float threshold);

   bool         Solve(const Transform& target);
   bool         SolveThreeJointLegWithConstraints(const Transform& target, int indexOfKnee, const glm::vec3& characterRight);

private:

   void         IKChainToWorld();
   void         WorldToIKChain();

   void         IterateBackward(const glm::vec3& oriPosOfRoot);
   void         IterateForward(const glm::vec3& goalPos);

   void         ApplyHingeConstraint(int indexOfConstrainedJoint, const glm::vec3& localHingeAxis, const glm::vec3& worldHingeAxis);
   void         ApplyBallAndSocketConstraint(int indexOfConstrainedJoint, float limitOfRotationInDeg);
   void         ApplyBackwardsKneeCorrection(int indexOfKneeJoint, const glm::vec3& referenceNormal);

   std::vector<Transform> mIKChain;
   std::vector<glm::vec3> mWorldPositionsChain;
   std::vector<float>     mDistancesBetweenJoints;
   unsigned int           mNumIterations;
   float                  mConvergenceThreshold;
};

#endif
