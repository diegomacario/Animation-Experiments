#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "FABRIKSolver.h"

// Forward and Backward Reaching Inverse Kinematics (FABRIK) is an algorithm that can be used to pose a chain of joints so that the last joint of the chain
// comes as close as possible to touching a target
// The three important concepts of FABRIK are:
// - The goal, which is the point in space that we are trying to touch
// - The IK chain, which is a list of all the joints that will need to rotate to reach the goal
// - The end effector, which is the last joint in the chain
// One iteration of the FABRIK algorithm looks like this in pseudocode:
// - Backward iteration
//   - Move the end effector to where the goal is
//   - Calculate a vector that goes from the joint that comes before the end effector to the end effector
//   - Move the joint that comes before the end effector along the vector from the previous step, maintaining its original distance to the end effector
//   - Repeat the previous steps with each sequential pair of joints until the root is reached. This will move the root out of its original position
// - Forward iteration
//   - Move the root back to its original position
//   - Calculate a vector that goes from the joint that comes after the root to the root
//   - Move the joint that comes after the root along the vector from the previous step, maintaining its original distance to root
//   - Repeat the previous steps with each sequential pair of joints until the end effector is reached.
// By doing backward and forward iterations many times, the algorithm may or may not achieve convergence

FABRIKSolver::FABRIKSolver()
   : mNumIterations(15)
   , mConvergenceThreshold(0.00001f)
{

}

unsigned int FABRIKSolver::GetNumberOfJointsInIKChain()
{
   return static_cast<unsigned int>(mIKChain.size());
}

void FABRIKSolver::SetNumberOfJointsInIKChain(unsigned int numJoints)
{
   mIKChain.resize(numJoints);
   mWorldPositionsChain.resize(numJoints);
   mDistancesBetweenJoints.resize(numJoints);
}

Transform FABRIKSolver::GetLocalTransform(unsigned int jointIndex) const
{
   return mIKChain[jointIndex];
}

void FABRIKSolver::SetLocalTransform(unsigned int jointIndex, const Transform& transform)
{
   mIKChain[jointIndex] = transform;
}

Transform FABRIKSolver::GetGlobalTransform(unsigned int jointIndex)
{
   // The IK chain is organized so that each joint is preceded by its parent
   Transform result = mIKChain[jointIndex];

   for (int parentIndex = static_cast<int>(jointIndex) - 1; parentIndex >= 0; --parentIndex)
   {
      result = combine(mIKChain[parentIndex], result);
   }

   return result;
}

unsigned int FABRIKSolver::GetNumberOfIterations()
{
   return mNumIterations;
}

void FABRIKSolver::SetNumberOfIterations(unsigned int numIterations)
{
   mNumIterations = numIterations;
}

float FABRIKSolver::GetThreshold()
{
   return mConvergenceThreshold;
}

void FABRIKSolver::SetThreshold(float threshold)
{
   mConvergenceThreshold = threshold;
}

bool FABRIKSolver::Solve(const Transform& target)
{
   unsigned int numJoints = GetNumberOfJointsInIKChain();

   // We cannot solve the IK chain if it's empty
   if (numJoints == 0)
   {
      return false;
   }

   unsigned int indexOfEndEffector = numJoints - 1;
   float        thresholdSq        = mConvergenceThreshold * mConvergenceThreshold;
   glm::vec3    goalPos            = target.position;
   glm::vec3    endEffectorPos;

   // Fill mWorldPositionsChain and mDistancesBetweenJoints using the values stored in mIKChain
   IKChainToWorld();

   glm::vec3 oriPosOfRoot = mWorldPositionsChain[0];

   // Loop over the allowed number of iterations until we either converge or reach the maximum number of iterations
   for (unsigned int i = 0; i < mNumIterations; ++i)
   {
      // Update the position of the end effector, since it changes with every iteration of this loop
      endEffectorPos = mWorldPositionsChain[indexOfEndEffector];

      // Check if we converged
      if (glm::length2(goalPos - endEffectorPos) < thresholdSq)
      {
         // If we did, use the positions stored in mWorldPositionsChain to rotate the joints stored in mIKChain to achieve the solved pose
         WorldToIKChain();
         return true;
      }

      // Perform a backward and forward iteration
      IterateBackward(goalPos);
      IterateForward(oriPosOfRoot);
   }

   // Check if we converged in the last iteration
   endEffectorPos = mWorldPositionsChain[indexOfEndEffector];
   if (glm::length2(goalPos - endEffectorPos) < thresholdSq)
   {
      // If we did, use the positions stored in mWorldPositionsChain to rotate the joints stored in mIKChain to achieve the solved pose
      WorldToIKChain();
      return true;
   }
   else
   {
      // If we didn't, use the positions stored in mWorldPositionsChain to rotate the joints stored in mIKChain to
      // achieve the closest thing we found to the solved pose
      WorldToIKChain();
      return false;
   }
}

// This function fills mWorldPositionsChain with the world positions of the joints that make up the unsolved IK chain
// It does this by calculating the global transform of each joint using the local transforms stored in mIKChain
// It also fills mDistancesBetweenJoints, which contains the distance between each joint and its parent
// Note that since joint 0 doesn't have a parent, mDistancesBetweenJoints[0] is set equal to 0.0f
void FABRIKSolver::IKChainToWorld()
{
   unsigned int numJoints = GetNumberOfJointsInIKChain();

   // Loop over the joints in the IK chain
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      // Calculate the world position of the current joint and store it
      glm::vec3 worldPosOfCurrJoint = GetGlobalTransform(jointIndex).position;
      mWorldPositionsChain[jointIndex] = worldPosOfCurrJoint;

      // If the current joint has a parent (i.e. it's not the root joint), calculate the distance between it and its parent and store it
      if (jointIndex >= 1)
      {
         glm::vec3 worldPosOfPrevJoint = mWorldPositionsChain[jointIndex - 1];
         mDistancesBetweenJoints[jointIndex] = glm::length(worldPosOfCurrJoint - worldPosOfPrevJoint);
      }
   }

   // If the IK chain is not empty, set the distance between the root joint and its parent equal to 0.0f
   if (numJoints > 0)
   {
      mDistancesBetweenJoints[0] = 0.0f;
   }
}

// Once the IK chain has been solved, we are left with the following:
// - mIKChain contains the local transforms of the joints that make up the unsolved IK chain
// - mWorldPositionsChain contains the world positions of the joints the make up the solved IK chain
// So now all we have to do is use the world positions stored in mWorldPositionsChain to calculate how
// the joints in mIKChain should rotate to achieve the solved pose
// That's what this function does, and the comments in it explain how it does it
void FABRIKSolver::WorldToIKChain()
{
   unsigned int numJoints = GetNumberOfJointsInIKChain();

   // Loop over the joints in the IK chain
   for (unsigned int jointIndex = 0; jointIndex < numJoints - 1; ++jointIndex)
   {
      // Get the unsolved global transforms of the current joint and the next one
      Transform originalWorldTransfOfCurrJoint = GetGlobalTransform(jointIndex);
      Transform originalWorldTransfOfNextJoint = GetGlobalTransform(jointIndex + 1);

      // Get the unsolved world position and rotation of the current joint
      glm::vec3 originalWorldPosOfCurrJoint = originalWorldTransfOfCurrJoint.position;
      Q::quat   originalWorldRotOfCurrJoint = originalWorldTransfOfCurrJoint.rotation;

      // Calculate a vector that goes from the unsolved world position of the current joint to the unsolved world position of the next joint
      glm::vec3 toNext = originalWorldTransfOfNextJoint.position - originalWorldPosOfCurrJoint;
      // Calculate a vector that goes from the unsolved world position of the current joint to the solved world position of the next joint
      glm::vec3 toDesired = mWorldPositionsChain[jointIndex + 1] - originalWorldPosOfCurrJoint;

      // NOTE: The quaternion operations below are reversed because q * p is implemented as p * q

      /*
         The rotation that goes from toNext to toDesired is the rotation that we need to apply to the current joint to achieve the solved pose
         Since toNext and toDesired are world space vectors, the rotation below is a world rotation:

            Q::quat worldDelta = Q::fromTo(toNext, toDesired);

         To apply worldDelta to the world rotation of the current joint we must multiply it on the right, since it's a world rotation:

            Q::quat newWorldRotOfCurrJoint = originalWorldRotOfCurrJoint * worldDelta;

         Finally, we must convert newWorldRotOfCurrJoint into a local rotation so that we can store it in mIKChain
         We can do that as follows:

            Q::quat newLocalRotOfCurrJoint = newWorldRotOfCurrJoint;
            if (i > 0)
            {
               // Here we multiply the new global rotation of the current joint with the inverse of the global rotation of its parent,
               // which leaves us with the new local rotation of the current joint
               newLocalRotOfCurrJoint = newWorldRotOfCurrJoint * Q::inverse(GetGlobalTransform(i - 1).rotation);
            }

         To understand the code above, imagine if the local transforms in our IK chain looked like this:

            (root) D ---- C
                          |
                          |
                          B ---- A'

         Where the new local transform of the current joint is A',
         and where the new global transform of the current joint is (D * C * B * A')
         You can think of the last quaternion multiplication that we performed above in this way (in standard notation i.e. q * p is not implemented as p * q):

            newLocalRotOfCurrJoint =  Q::inverse(GetGlobalTransform(i - 1).rotation) * newWorldRotOfCurrJoint = (D * C * B)^-1 * (D * C * B * A') = (B^-1 * C^-1 * D^-1) * (D * C * B * A') = A'

         Note that the if-statement is necessary because the root doesn't have a parent, so its world rotation is the same as its local rotation

         The problem with the code above is that it requires us to calculate the global transform of the current joint's parent, which is expensive
         We can avoid doing that with this approach:

            // Here we take toNext and toDesired from world space into the local space of the current joint by
            // multiplying them with the inverse of the world rotation of the current joint
            toNext = Q::inverse(originalWorldRotOfCurrJoint) * toNext;
            toDesired = Q::inverse(originalWorldRotOfCurrJoint) * toDesired;

            // With the vectors in the local space of the current joint, we can now calculate the local rotation that goes from toNext to toDesired
            Q::quat localDelta = Q::fromTo(toNext, toDesired);

            // Finally, we can apply the local rotation to the rotation of the current joint to achieve the solved pose
            // Note how we multiply it on the left, since it's a local rotation
            Q::quat newLocalRotOfCurrJoint = localDelta * mIKChain[jointIndex].rotation;

         That's the approach that's implemented below
      */

      // We want to use the two vectors that we calculated above to determine the local rotation that we must apply to the current joint to achieve the solved pose
      // Since we want to determine the necessary LOCAL rotation, we must multiply both vectors by the inverse of the world rotation of the current joint,
      // which takes them from world space to the local space of the current joint
      toNext = Q::inverse(originalWorldRotOfCurrJoint) * toNext;
      toDesired = Q::inverse(originalWorldRotOfCurrJoint) * toDesired;

      // With the vectors in the local space of the current joint, we can now calculate the local rotation that goes from toNext to toDesired
      Q::quat localDelta = Q::fromTo(toNext, toDesired);

      // Finally, we can apply the local rotation to the rotation of the current joint to achieve the solved pose
      mIKChain[jointIndex].rotation = localDelta * mIKChain[jointIndex].rotation;
   }
}

// This function implements a backward iteration over the IK chain
void FABRIKSolver::IterateBackward(const glm::vec3& goalPos)
{
   int numJoints = static_cast<int>(GetNumberOfJointsInIKChain());

   // Move the end effector to where the goal is
   if (numJoints > 0)
   {
      mWorldPositionsChain[numJoints - 1] = goalPos;
   }

   // Loop backwards over the joints in the IK chain, starting with the one that comes before the end effector
   for (int i = numJoints - 2; i >= 0; --i)
   {
      // Calculate a vector that goes from the current joint to the next one
      glm::vec3 dirToNext = normalizeWithZeroLengthCheck(mWorldPositionsChain[i + 1] - mWorldPositionsChain[i]);
      // mDistancesBetweenJoints contains the distance between each joint and its parent,
      // so the distance between the current joint and the next one is stored in the index of the next one (mDistancesBetweenJoints[i + 1])
      float distToNext = mDistancesBetweenJoints[i + 1];

      // To move the current joint towards the next one while preserving the original distance between them, we would need to calculate that distance as follows:
      //
      // float distToMove = glm::length(mWorldPositionsChain[i + 1] - mWorldPositionsChain[i]) - distToNext;
      //
      // We can avoid doing that calculation by moving the next joint towards the current one, in which case distToMove is equal to distToNext
      // That's what we do below, but note how dirToNext is negated, since we calculated it as a vector that goes from the current joint to the next one,
      // and we want move in the opposite direction
      mWorldPositionsChain[i] = mWorldPositionsChain[i + 1] - (dirToNext * distToNext);
   }
}

// This function implements a forward iteration over the IK chain
void FABRIKSolver::IterateForward(const glm::vec3& oriPosOfRoot)
{
   int numJoints = static_cast<int>(GetNumberOfJointsInIKChain());

   // Move the root to its original position
   if (numJoints > 0)
   {
      mWorldPositionsChain[0] = oriPosOfRoot;
   }

   // Loop forwards over the joints in the IK chain, starting with the one that comes after the root
   for (int i = 1; i < numJoints; ++i)
   {
      // Calculate a vector that goes from the current joint to the previous one
      glm::vec3 dirToPrev = normalizeWithZeroLengthCheck(mWorldPositionsChain[i - 1] - mWorldPositionsChain[i]);
      // mDistancesBetweenJoints contains the distance between each joint and its parent,
      // so the distance between the current joint and the previous one is stored in the index of the current one (mDistancesBetweenJoints[i])
      float distToPrev = mDistancesBetweenJoints[i];

      // To move the current joint towards the previous one while preserving the original distance between them, we would need to calculate that distance as follows:
      //
      // float distToMove = glm::length(mWorldPositionsChain[i - 1] - mWorldPositionsChain[i]) - distToPrev;
      //
      // We can avoid doing that calculation by moving the previous joint towards the current one, in which case distToMove is equal to distToPrev
      // That's what we do below, but note how dirToPrev is negated, since we calculated it as a vector that goes from the current joint to the previous one,
      // and we want move in the opposite direction
      mWorldPositionsChain[i] = mWorldPositionsChain[i - 1] - (dirToPrev * distToPrev);
   }
}

void FABRIKSolver::ApplyHingeConstraint(int indexOfConstrainedJoint, const glm::vec3& hingeAxis)
{
   Transform constrainedJoint         = GetGlobalTransform(indexOfConstrainedJoint);
   Transform parentOfConstrainedJoint = GetGlobalTransform(indexOfConstrainedJoint - 1);

   glm::vec3 currHinge    = constrainedJoint.rotation * hingeAxis;
   glm::vec3 desiredHinge = parentOfConstrainedJoint.rotation * hingeAxis;

   // Construct a world rotation that goes from the current hinge to the desired one
   Q::quat worldRotFromCurrHingeToDesiredHinge = Q::fromTo(currHinge, desiredHinge);

   // Apply the hinge-to-hinge world rotation to the world rotation of the constrained joint
   Q::quat newWorldRotOfConstrainedJoint = constrainedJoint.rotation * worldRotFromCurrHingeToDesiredHinge;

   // Multiply the new world rotation of the constrained joint by the inverse of its old world rotation to get:
   // constrainedJoint.rotation^-1 * newWorldRotOfConstrainedJoint = (D * C * B * A_old)^-1 * (D * C * B * A_new) = (A_old^-1 * B^-1 * C^-1 * D^-1) * (D * C * B * A_new) = A_old^-1 * A_new
   Q::quat newLocalRotOfConstrainedJoint = newWorldRotOfConstrainedJoint * inverse(constrainedJoint.rotation);

   // Get the local transform of the joint
   Q::quat localRotOfConstrainedJoint = mIKChain[indexOfConstrainedJoint].rotation;

   // Multiply newLocalRotOfConstrainedJoint by the old local rotation of the constrained joint to get:
   // localRotOfConstrainedJoint * newLocalRotOfConstrainedJoint = A_old * A_old^-1 * A_new = A_new
   mIKChain[indexOfConstrainedJoint].rotation = newLocalRotOfConstrainedJoint * localRotOfConstrainedJoint;
}

void FABRIKSolver::ApplyBallSocketConstraint(int indexOfConstrainedJoint, float limitOfRotationInDeg)
{
   Q::quat worldRotOfConstrainedJoint         = GetGlobalTransform(indexOfConstrainedJoint).rotation;
   Q::quat worldRotOfParentOfConstrainedJoint = GetGlobalTransform(indexOfConstrainedJoint - 1).rotation;

   glm::vec3 fwdDirOfConstrainedJoint         = worldRotOfConstrainedJoint * glm::vec3(0.0f, 0.0f, 1.0f);
   glm::vec3 fwdDirOfParentOfConstrainedJoint = worldRotOfParentOfConstrainedJoint * glm::vec3(0.0f, 0.0f, 1.0f);

   float angle = glm::angle(fwdDirOfParentOfConstrainedJoint, fwdDirOfConstrainedJoint);

   if (angle > glm::radians(limitOfRotationInDeg))
   {
      glm::vec3 axisOfRot = glm::cross(fwdDirOfParentOfConstrainedJoint, fwdDirOfConstrainedJoint);

      // TODO: Improve the name of this variable
      Q::quat worldSpaceRotation = worldRotOfParentOfConstrainedJoint * Q::angleAxis(glm::radians(limitOfRotationInDeg), axisOfRot);

      mIKChain[indexOfConstrainedJoint].rotation = worldSpaceRotation * inverse(worldRotOfParentOfConstrainedJoint);
   }
}

void FABRIKSolver::ApplyBackwardsKneeCorrection(int indexOfConstrainedJoint, const glm::vec3& referenceNormal)
{
   Transform kneeJoint  = GetGlobalTransform(indexOfConstrainedJoint);
   Transform hipJoint   = GetGlobalTransform(indexOfConstrainedJoint - 1);
   Transform ankleJoint = GetGlobalTransform(indexOfConstrainedJoint + 1);

   glm::vec3 kneeToAnkle = glm::normalize(glm::vec3(ankleJoint.position - kneeJoint.position));
   glm::vec3 kneeToHip   = glm::normalize(glm::vec3(hipJoint.position - kneeJoint.position));

   float angleInDeg = glm::degrees(glm::orientedAngle(kneeToAnkle, kneeToHip, referenceNormal));

   if (angleInDeg < 0.0f && ((180.0f + angleInDeg) > 10.0f))
   {
      float angleHipNeedsToRotate = 180.0f + angleInDeg;
      float angleKneeNeedsToRotate = (180.0f + angleInDeg) * 2.0f;

      glm::vec3 axisOfRot = glm::cross(kneeToHip, kneeToAnkle);

      // Adjust the hip
      Q::quat newWorldRotOfHipJoint                  = hipJoint.rotation * Q::angleAxis(glm::radians(-angleHipNeedsToRotate), axisOfRot);
      Q::quat newLocalRotOfHipJoint                  = newWorldRotOfHipJoint * inverse(hipJoint.rotation);
      Q::quat localRotOfHipJoint                     = mIKChain[indexOfConstrainedJoint - 1].rotation;
      mIKChain[indexOfConstrainedJoint - 1].rotation = newLocalRotOfHipJoint * localRotOfHipJoint;

      // Adjust the knee
      Q::quat newWorldRotOfKneeJoint             = kneeJoint.rotation * Q::angleAxis(glm::radians(angleKneeNeedsToRotate), axisOfRot);
      Q::quat newLocalRotOfKneeJoint             = newWorldRotOfKneeJoint * inverse(kneeJoint.rotation);
      Q::quat localRotOfKneeJoint                = mIKChain[indexOfConstrainedJoint].rotation;
      mIKChain[indexOfConstrainedJoint].rotation = newLocalRotOfKneeJoint * localRotOfKneeJoint;
   }
}
