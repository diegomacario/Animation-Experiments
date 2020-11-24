#ifndef SKELETON_VIEWER_H
#define SKELETON_VIEWER_H

#include <array>

#include "Pose.h"

class SkeletonViewer
{
public:

   SkeletonViewer();
   ~SkeletonViewer();

   SkeletonViewer(const SkeletonViewer&) = delete;
   SkeletonViewer& operator=(const SkeletonViewer&) = delete;

   SkeletonViewer(SkeletonViewer&& rhs) noexcept;
   SkeletonViewer& operator=(SkeletonViewer&& rhs) noexcept;

   void ExtractPointsOfSkeletonFromPose(const Pose& animatedPose, const std::vector<glm::mat4>& animatedPosePalette);

   void LoadBuffers();

   void ConfigureVAO(int posAttribLocation, int colorAttribLocation);

   void UnconfigureVAO(int posAttribLocation, int colorAttribLocation);

   void BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void UnbindAttribute(int attribLocation, unsigned int VBO);

   void Render();

private:

   void ResizeContainers(const Pose& animatedPose);

   unsigned int           mVAO;
   unsigned int           mVBO;

   std::array<glm::vec3, 3> mColorPalette;
   std::vector<glm::vec3>   mJointPositions;
   std::vector<glm::vec3>   mJointColors;
};

#endif
