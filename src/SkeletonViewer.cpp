#include <glad/glad.h>

#include "SkeletonViewer.h"
#include "Transform.h"

SkeletonViewer::SkeletonViewer()
   : mColorPalette{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f)}
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
   , mColorPalette(std::move(rhs.mColorPalette))
   , mJointPositions(std::move(rhs.mJointPositions))
   , mJointColors(std::move(rhs.mJointColors))
{

}

SkeletonViewer& SkeletonViewer::operator=(SkeletonViewer&& rhs) noexcept
{
   mVAO            = std::exchange(rhs.mVAO, 0);
   mVBO            = std::exchange(rhs.mVBO, 0);
   mColorPalette   = std::move(rhs.mColorPalette);
   mJointPositions = std::move(rhs.mJointPositions);
   mJointColors    = std::move(rhs.mJointColors);
   return *this;
}

void SkeletonViewer::ExtractPointsOfSkeletonFromPose(const Pose& animatedPose, const std::vector<glm::mat4>& animatedPosePalette)
{
   ResizeContainers(animatedPose);

   int posIndex = 0;
   int colorIndex = 0;
   int colorPaletteIndex = 0;
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
      mJointPositions[posIndex++] = animatedPosePalette[jointIndex][3];
      mJointPositions[posIndex++] = animatedPosePalette[parentJointIndex][3];

      colorPaletteIndex %= 3;
      mJointColors[colorIndex++] = mColorPalette[colorPaletteIndex];
      mJointColors[colorIndex++] = mColorPalette[colorPaletteIndex];
      ++colorPaletteIndex;
   }
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void SkeletonViewer::LoadBuffers()
{
   glBindVertexArray(mVAO);

   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, (mJointPositions.size() + mJointColors.size()) * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW); // TODO: GL_DYNAMIC?
   // Positions
   glBufferSubData(GL_ARRAY_BUFFER, 0, mJointPositions.size() * sizeof(glm::vec3), &mJointPositions[0]); // TODO: GL_DYNAMIC?
   // Colors
   glBufferSubData(GL_ARRAY_BUFFER, mJointPositions.size() * sizeof(glm::vec3), mJointColors.size() * sizeof(glm::vec3), &mJointColors[0]);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glBindVertexArray(0);
}

void SkeletonViewer::ConfigureVAO(int posAttribLocation, int colorAttribLocation)
{
   glBindVertexArray(mVAO);

   // Set the vertex attribute pointers

   if (posAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBO);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   if (colorAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBO);
      glEnableVertexAttribArray(colorAttribLocation);
      glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)(mJointPositions.size() * sizeof(glm::vec3)));
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   glBindVertexArray(0);
}

void SkeletonViewer::UnconfigureVAO(int posAttribLocation, int colorAttribLocation)
{
   glBindVertexArray(mVAO);

   // Unset the vertex attribute pointers
   UnbindAttribute(posAttribLocation, mVBO);
   UnbindAttribute(colorAttribLocation, mVBO);

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

   glDrawArrays(GL_LINES, 0, static_cast<unsigned int>(mJointPositions.size()));

   glBindVertexArray(0);
}

void SkeletonViewer::ResizeContainers(const Pose& animatedPose)
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

   mJointPositions.resize(numPointsOfSkeleton);
   mJointColors.resize(numPointsOfSkeleton);
}
