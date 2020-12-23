#include <glm/gtx/norm.hpp>

#include "CCDSolver.h"

// Cyclic Coordinate Descent (CCD) is an algorithm that can be used to pose a chain of joints so that the last joint of the chain
// comes as close as possible to touching a target
// The three important concepts of CCD are:
// - The goal, which is the point in space that we are trying to touch
// - The IK chain, which is a list of all the joints that will need to rotate to reach the goal
// - The end effector, which is the last joint in the chain
// One iteration of the CCD algorithm looks like this in pseudocode:
// - Loop backwards over the IK chain, starting with the joint that comes before the end effector
// - Calculate two vectors: currJointToEndEffector and currJointToGoal
// - Rotate the current joint so that the orientation of the currJointToEndEffector vector matches
//   the orientation of the currJointToGoal vector
// By repeating the previous steps over many iterations, the algorithm may or may not achieve convergence

CCDSolver::CCDSolver()
   : mNumIterations(15)
   , mConvergenceThreshold(0.00001f)
{

}

unsigned int CCDSolver::GetNumberOfJointsInIKChain()
{
   return static_cast<unsigned int>(mIKChain.size());
}

void CCDSolver::SetNumberOfJointsInIKChain(unsigned int numJoints)
{
   mIKChain.resize(numJoints);
}

Transform CCDSolver::GetLocalTransform(unsigned int jointIndex) const
{
   return mIKChain[jointIndex];
}

void CCDSolver::SetLocalTransform(unsigned int jointIndex, const Transform& transform)
{
   mIKChain[jointIndex] = transform;
}

Transform CCDSolver::GetGlobalTransform(unsigned int jointIndex)
{
   // The IK chain is organized so that each joint is preceded by its parent
   Transform result = mIKChain[jointIndex];

   for (int parentIndex = static_cast<int>(jointIndex) - 1; parentIndex >= 0; --parentIndex)
   {
      result = combine(mIKChain[parentIndex], result);
   }

   return result;
}

unsigned int CCDSolver::GetNumberOfIterations()
{
   return mNumIterations;
}

void CCDSolver::SetNumberOfIterations(unsigned int numIterations)
{
   mNumIterations = numIterations;
}

float CCDSolver::GetThreshold()
{
   return mConvergenceThreshold;
}

void CCDSolver::SetThreshold(float threshold)
{
   mConvergenceThreshold = threshold;
}

bool CCDSolver::Solve(const Transform& goal)
{
   unsigned int numJoints = GetNumberOfJointsInIKChain();

   // We cannot solve the IK chain if it's empty
   if (numJoints == 0)
   {
      return false;
   }

   unsigned int indexOfEndEffector = numJoints - 1;
   float        thresholdSq        = mConvergenceThreshold * mConvergenceThreshold;
   glm::vec3    goalPos            = goal.position;
   glm::vec3    endEffectorPos     = GetGlobalTransform(indexOfEndEffector).position;

   // TODO: Possibly unnecessary
   // Check if convergence has already been achieved
   if (glm::length2(goalPos - endEffectorPos) < thresholdSq)
   {
      return true;
   }

   // Loop over the allowed number of iterations until we either converge or reach the maximum number of iterations
   for (unsigned int iteration = 0; iteration < mNumIterations; ++iteration)
   {
      // Loop backwards over the IK chain, starting with the joint that comes before the end effector
      for (int jointIndex = static_cast<int>(numJoints) - 2; jointIndex >= 0; --jointIndex)
      {
         // Update the position of the end effector, since it changes with every iteration of this loop
         endEffectorPos = GetGlobalTransform(indexOfEndEffector).position;

         // Get the position and rotation of the current joint
         Transform globalTransfOfCurrJoint = GetGlobalTransform(jointIndex);
         glm::vec3 posOfCurrJoint          = globalTransfOfCurrJoint.position;
         Q::quat   rotationOfCurrJoint     = globalTransfOfCurrJoint.rotation;

         // Calculate the two vectors that we need to align
         glm::vec3 jointToEndEffector = endEffectorPos - posOfCurrJoint;
         glm::vec3 jointToGoal        = goalPos - posOfCurrJoint;

         // Calculate the quaternion that rotates from jointToEndEffector to jointToGoal
         // Note that there is an edge case in which the jointToGoal vector is a zero vector
         // If that's the case, the effectorToGoal quaternion is left as a unit quaternion
         Q::quat effectorToGoal;
         if (glm::length2(jointToGoal) > 0.00001f) // TODO: Use constant
         {
            effectorToGoal = Q::fromTo(jointToEndEffector, jointToGoal);
         }

         // NOTE: The quaternion operations below are reversed because q * p is implemented as p * q

         // effectorToGoal is the world rotation that takes us from jointToEndEffector to jointToGoal
         // Since it's a world rotation, we multiply it from the right below so that it's applied with respect to the axes of the world
         Q::quat newGlobalRotationOfJoint = rotationOfCurrJoint * effectorToGoal;

         /*
            Now we need to turn newGlobalRotationOfJoint into a local rotation so that we can insert it into the IK chain
            If the transforms in our IK chain looked like this:

            (root) D ---- C
                          |
                          |
                          B ---- A

            Where the local transform of the current joint is A,
            and where the global transform of the current joint is (D * C * B * A),
            then getting the local transform would be as simple as doing this:
         
            (D * C * B)^-1 * (D * C * B * A) = (B^-1 * C^-1 * D^-1) * (D * C * B * A) = A

            In other words, we would need to multiply the global transform of A with the global transform of its parent, which is B
            If we did that, the code would look like this:

               // If we are processing the root of the chain, globalTransfOfParentJoint stays as a unit transform
               Transform globalTransfOfParentJoint;
               if (j >= 1)
               {
                  globalTransfOfParentJoint = GetGlobalTransform(j - 1);
               }

               // Here we multiply the new global rotation of the current joint with the inverse of the global rotation of its parent,
               // which leaves us with the new local rotation of the current joint
               // You can think of this operation in this way (in standard notation i.e. q * p is not implemented as p * q):
               //
               // globalRotOfParent^-1 * newGlobalRotOfCurrJoint = (D * C * B)^-1 * (D * C * B * A_new) = (B^-1 * C^-1 * D^-1) * (D * C * B * A_new) = A_new
               //
               Q::quat newLocalRotationOfJoint = newGlobalRotationOfJoint * inverse(globalTransfOfParentJoint.rotation);

               // Finally, we store the new local rotation of the current joint in the IK chain
               mIKChain[j].rotation = newLocalRotationOfJoint;

            The problem with the code above is that it requires us to calculate the global transform of the current joint's parent, which is expensive
            We can avoid doing that with this approach:

               // Here we multiply the new global rotation of the current joint with the inverse of its old global rotation,
               // which leaves us with the following (in standard notation i.e. q * p is not implemented as p * q):
               //
               // oldGlobalRotOfCurrJoint^-1 * newGlobalRotOfCurrJoint = (D * C * B * A_old)^-1 * (D * C * B * A_new) = (A_old^-1 * B^-1 * C^-1 * D^-1) * (D * C * B * A_new) = A_old^-1 * A_new
               //
               Q::quat newLocalRotationOfJoint = newGlobalRotationOfJoint * inverse(rotationOfCurrJoint);

               // Finally, we multiply the new local rotation of the joint with its old local rotation,
               // which leaves us with the following (in standard notation i.e. q * p is not implemented as p * q):
               //
               // mIKChain[jointIndex].rotation * newLocalRotationOfJoint = A_old * (A_old^-1 * A_new) = A_new
               //
               mIKChain[jointIndex].rotation = newLocalRotationOfJoint * mIKChain[jointIndex].rotation;

            That's the approach that's implemented below
         */
         Q::quat newLocalRotationOfJoint = newGlobalRotationOfJoint * inverse(rotationOfCurrJoint);
         mIKChain[jointIndex].rotation = newLocalRotationOfJoint * mIKChain[jointIndex].rotation;

         // Now that the current joint has been rotated we can calculate the new position of the end effector
         // and check if we converged
         endEffectorPos = GetGlobalTransform(indexOfEndEffector).position;
         if (glm::length2(goalPos - endEffectorPos) < thresholdSq)
         {
            return true;
         }
      }
   }

   return false;
}
