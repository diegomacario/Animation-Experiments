#include <glad/glad.h>

#include "SkeletonViewer.h"
#include "Transform.h"

SkeletonViewer::SkeletonViewer()
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);
}

SkeletonViewer::~SkeletonViewer()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
}

SkeletonViewer::SkeletonViewer(SkeletonViewer&& rhs) noexcept
   : mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
   , mPointsOfSkeleton(std::move(rhs.mPointsOfSkeleton))
{

}

SkeletonViewer& SkeletonViewer::operator=(SkeletonViewer&& rhs) noexcept
{
   mVAO              = std::exchange(rhs.mVAO, 0);
   mVBO              = std::exchange(rhs.mVBO, 0);
   mPointsOfSkeleton = std::move(rhs.mPointsOfSkeleton);
   return *this;
}

void SkeletonViewer::ExtractPointsOfSkeletonFromPose(const Pose& animatedPose, const std::vector<glm::mat4>& animatedPosePalette)
{
   ResizeContainerOfPointsOfSkeleton(animatedPose);

   int parentJointIndex = -1;
   unsigned int numJoints = animatedPose.GetNumberOfJoints();
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      parentJointIndex = animatedPose.GetParent(jointIndex);
      if (parentJointIndex < 0)
      {
         continue;
      }

      // Store the position of the child joint followed by the position of its parent joint
      // That way, they will be rendered as a line
      mPointsOfSkeleton.push_back(glm::vec3(animatedPosePalette[jointIndex][3]));
      mPointsOfSkeleton.push_back(glm::vec3(animatedPosePalette[parentJointIndex][3]));
   }
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void SkeletonViewer::LoadBuffers()
{
   glBindVertexArray(mVAO);

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, mPointsOfSkeleton.size() * sizeof(glm::vec3), &mPointsOfSkeleton[0], GL_STATIC_DRAW); // TODO: GL_DYNAMIC?
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glBindVertexArray(0);
}

void SkeletonViewer::ConfigureVAO(int posAttribLocation)
{
   glBindVertexArray(mVAO);

   // Set the vertex attribute pointers
   BindFloatAttribute(posAttribLocation, mVBO, 3);

   glBindVertexArray(0);
}

void SkeletonViewer::UnconfigureVAO(int posAttribLocation)
{
   glBindVertexArray(mVAO);

   // Unset the vertex attribute pointers
   UnbindAttribute(posAttribLocation, mVBO);

   glBindVertexArray(0);
}

void SkeletonViewer::BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glEnableVertexAttribArray(attribLocation);
      glVertexAttribPointer(attribLocation, numComponents, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
}

void SkeletonViewer::UnbindAttribute(int attribLocation, unsigned int VBO)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glDisableVertexAttribArray(attribLocation);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
}

void SkeletonViewer::Render()
{
   glBindVertexArray(mVAO);

   glDrawArrays(GL_LINES, 0, static_cast<unsigned int>(mPointsOfSkeleton.size()));

   glBindVertexArray(0);
}

void SkeletonViewer::ResizeContainerOfPointsOfSkeleton(const Pose& animatedPose)
{
   unsigned int numPointsOfSkeleton = 0;
   unsigned int numJoints = animatedPose.GetNumberOfJoints();
   for (unsigned int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
   {
      // If the current joint doesn't have a parent, we can't create a line that connects it to its parent,
      // so we don't increase the number of points of the skeleton
      if (animatedPose.GetParent(jointIndex) < 0)
      {
         continue;
      }

      // Increase the number of points of the skeleton by two (one for the child joint and one for its parent joint)
      numPointsOfSkeleton += 2;
   }

   mPointsOfSkeleton.resize(numPointsOfSkeleton);
}
