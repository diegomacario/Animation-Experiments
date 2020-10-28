#include "Pose.h"

Pose::Pose(unsigned int numJoints)
{
   SetNumberOfJoints(numJoints);
}

Pose::Pose(const Pose& rhs)
{
   *this = rhs;
}

Pose& Pose::operator=(const Pose& rhs)
{
   if (this == &rhs)
   {
      return *this;
   }

   if (mJoints.size() != rhs.mJoints.size())
   {
      mJoints.resize(rhs.mJoints.size());
   }

   if (mParents.size() != rhs.mParents.size())
   {
      mParents.resize(rhs.mParents.size());
   }

   if (mJoints.size() != 0)
   {
      memcpy(&mJoints[0], &rhs.mJoints[0], sizeof(Transform) * mJoints.size());
   }

   if (mParents.size() != 0)
   {
      memcpy(&mParents[0], &rhs.mParents[0], sizeof(int) * mParents.size());
   }

   return *this;
}

bool Pose::operator==(const Pose& rhs)
{
   if (mJoints.size() != rhs.mJoints.size())
   {
      return false;
   }

   if (mParents.size() != rhs.mParents.size())
   {
      return false;
   }

   unsigned int numJoints = static_cast<unsigned int>(mJoints.size());
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      if (mParents[jointIndex] != rhs.mParents[jointIndex])
      {
         return false;
      }

      if (mJoints[jointIndex] != rhs.mJoints[jointIndex])
      {
         return false;
      }
   }

   return true;
}

bool Pose::operator!=(const Pose& rhs)
{
   return !(*this == rhs);
}

Transform Pose::operator[](unsigned int jointIndex)
{
   return GetGlobalTransform(jointIndex);
}

unsigned int Pose::GetNumberOfJoints()
{
   return static_cast<unsigned int>(mJoints.size());
}

void Pose::SetNumberOfJoints(unsigned int numJoints)
{
   mJoints.resize(numJoints);
   mParents.resize(numJoints);
}

Transform Pose::GetLocalTransform(unsigned int jointIndex)
{
   return mJoints[jointIndex];
}

void Pose::SetLocalTransform(unsigned int jointIndex, const Transform& transform)
{
   mJoints[jointIndex] = transform;
}

Transform Pose::GetGlobalTransform(unsigned int jointIndex)
{
   /*
      To calculate the global transform of a joint, we iterate over the parents of that joint, combining their local transforms one by one
      For example, if we wanted the global transform of joint 2, which is illustrated in the diagram below, the code in this function would make these calls:

         0 (root)
        /
       /
      1
       \
        \
         2

                 +----+----+----+
       Joint IDs |  0 |  1 |  2 |
                 +----+----+----+
      Parent IDs | -1 |  0 |  1 |
                 +----+----+----+

      Transform result = mJoints[2];
      result = combine(mJoints[1], result); // The parent of joint 2 is joint 1
      result = combine(mJoints[0], result); // The parent of joint 1 is joint 0
      // Exit the loop because joint 0 is the root, so it has no parent, which we represent with a parent index of -1
   */

   // Start with the local transform of the desired joint
   Transform result = mJoints[jointIndex];

   // Iterate over the parents of the desired joint, combining their local transforms one by one
   for (int parentIndex = mParents[jointIndex]; parentIndex >= 0; parentIndex = mParents[parentIndex])
   {
      // Remember that the Transform::combine function takes the parent first and then the child
      result = combine(mJoints[parentIndex], result);
   }

   return result;
}

void Pose::GetMatrixPalette(std::vector<glm::mat4>& palette)
{
   /*
      A matrix palette is a list of all the global transforms of a pose
      Generating it is as simple as doing this:

      for (unsigned int jointIndex = 0; i < numJoints; ++jointIndex)
      {
         Transform t = GetGlobalTransform(jointIndex);
         palette[jointIndex] = transformToMat4(t);
      }

      The problem with that approach is that it redoes the same calculations many times
      For example, if we wanted the matrix palette of the pose illustrated in the diagram below, the code above would do this:

      (root) D ---- C          (root) 3 ---- 2
                    |                        |
                    |                        |
                    B ---- A                 1 ---- 0

                             +----+----+----+----+
      Local Transform Values |  A |  B |  C |  D |
                             +----+----+----+----+
                   Joint IDs |  0 |  1 |  2 |  3 |
                             +----+----+----+----+
                  Parent IDs |  1 |  2 |  3 | -1 |
                             +----+----+----+----+

      palette[0] = D * C * B * A
      palette[1] = D * C * B     // Repeat D * C * B
      palette[2] = D * C         // Repeat D * C
      palette[2] = D

      To avoid redoing the same calculations many times, this project's glTF-loading code
      attempts to reorganize joints while loading them so that a pose like the previous one is stored like this:

      (root) D ---- C          (root) 0 ---- 1
                    |                        |
                    |                        |
                    B ---- A                 2 ---- 3

                             +----+----+----+----+
      Local Transform Values |  D |  C |  B |  A |
                             +----+----+----+----+
                   Joint IDs |  0 |  1 |  2 |  3 |
                             +----+----+----+----+
                  Parent IDs | -1 |  0 |  1 |  2 |
                             +----+----+----+----+

      Where the parent joints have a lower index than their child joints in the array of joints
      For example, joint 0, which is the parent of joints 1, 2 and 3, has a lower index (0) than its children (1, 2 and 3)
      And joint 1, which is the parent of joints 2 and 3, has a lower index (1) than its children (2 and 3)
      And joint 2, which is the parent of joint 3, has a lower index (2) than its child (3)

      Thanks to this reorganization, we can write code that fills the palette while reusing previous calculations:

      palette[0] = D
      palette[1] = D * C
      palette[2] = D * C * B     // Reuse D * C
      palette[3] = D * C * B * A // Reuse D * C * B
   */

   int numJoints = static_cast<int>(GetNumberOfJoints());

   if (static_cast<int>(palette.size()) != numJoints)
   {
      palette.resize(numJoints);
   }

   // Iterate over the array of joints and try to use the optimized method to generate the matrix palette
   int jointIndex = 0;
   for (; jointIndex < numJoints; ++jointIndex)
   {
      // Get the index of the current joint's parent
      int parent = mParents[jointIndex];

      // Remember that for the optimization described above to work, the parent joints must have a lower index than their child joints in the array of joints
      // In other words, the parent joints must come first in the array of joints and their children must come after them
      // If the index of the current joint's parent is greater than the index of the current joint, then that means that the parent comes after the child in the array
      // If this is the case, then the joints haven't been reorganized as needed to use the optimized method to generate the matrix palette,
      // so we fall back on the inefficient method
      if (parent > jointIndex)
      {
         break;
      }

      glm::mat4 globalTransform;
      if (parent >= 0)
      {
         // If the current joint is not a root joint, then combine the global transform of its parent with its local transform
         // Note that since the parent joints come first in the reorganized array of joints, we process the parent joints first,
         // so palette[parent] is guaranteed to contain the global transform of the parent
         // This is what powers the optimization - reusing previous calculations
         globalTransform = palette[parent] * transformToMat4(mJoints[jointIndex]);
      }
      else
      {
         // If the current joint is a root joint, then its global transform is equal to its local transform
         globalTransform = transformToMat4(mJoints[jointIndex]);
      }

      // Store the global transform of the current joint
      palette[jointIndex] = globalTransform;
   }

   // Fall back on the inefficient method to generate the matrix palette
   // This only happens if the joints have not been reorganized by the glTF-loading code
   for (; jointIndex < numJoints; ++jointIndex)
   {
      Transform t = GetGlobalTransform(jointIndex);
      palette[jointIndex] = transformToMat4(t);
   }
}

int Pose::GetParent(unsigned int jointIndex)
{
   return mParents[jointIndex];
}

void Pose::SetParent(unsigned int jointIndex, int parentIndex)
{
   mParents[jointIndex] = parentIndex;
}
