#ifndef POSE_H
#define POSE_H

#include <vector>
#include "Transform.h"

/*
   A Pose stores a collection of joints, which are represented as local transforms,
   and a parallel list of their parents, as illustrated below:

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
*/

class Pose
{
public:

   Pose() = default;
   Pose(unsigned int numJoints);
   Pose(const Pose& rhs);
   Pose&        operator=(const Pose& rhs);

   bool         operator==(const Pose& rhs);
   bool         operator!=(const Pose& rhs);
   // TODO: Remove this operator. Call GetGlobalTransform directly instead
   Transform    operator[](unsigned int jointIndex);

   unsigned int GetNumberOfJoints();
   void         SetNumberOfJoints(unsigned int numJoints);

   Transform    GetLocalTransform(unsigned int jointIndex);
   void         SetLocalTransform(unsigned int jointIndex, const Transform& transform);
   Transform    GetGlobalTransform(unsigned int jointIndex);

   void         GetMatrixPalette(std::vector<glm::mat4>& palette);

   int          GetParent(unsigned int jointIndex);
   void         SetParent(unsigned int jointIndex, int parentIndex);

protected:

   std::vector<Transform> mJointLocalTransforms;
   std::vector<int>       mParentIndices;
};

#endif
