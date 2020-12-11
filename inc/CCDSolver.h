#ifndef CCD_SOLVER_H
#define CCD_SOLVER_H

#include <vector>

#include "Transform.h"

class CCDSolver
{
public:

   CCDSolver();

   unsigned int GetNumberOfJointsInIKChain();
   void         SetNumberOfJointsInIKChain(unsigned int numJoints);

   Transform    GetLocalTransform(unsigned int jointIndex) const;
   void         SetLocalTransform(unsigned int jointIndex, const Transform& transform);
   Transform    GetGlobalTransform(unsigned int jointIndex);

   unsigned int GetNumberOfIterations();
   void         SetNumberOfIterations(unsigned int numIterations);

   float        GetThreshold();
   void         SetThreshold(float threshold);

   bool         Solve(const Transform& goal);

protected:

   std::vector<Transform> mIKChain;
   unsigned int           mNumIterations;
   float                  mConvergenceThreshold;
};

#endif
