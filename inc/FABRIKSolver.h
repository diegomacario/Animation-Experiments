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

private:

   void         IKChainToWorld();
   void         WorldToIKChain();

   void         IterateBackward(const glm::vec3& oriPosOfRoot);
   void         IterateForward(const glm::vec3& goalPos);

   std::vector<Transform> mIKChain;
   std::vector<glm::vec3> mWorldPositionsChain;
   std::vector<float>     mDistancesBetweenJoints;
   unsigned int           mNumIterations;
   float                  mConvergenceThreshold;
};

#endif
