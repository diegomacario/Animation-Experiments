#ifndef SKELETON_VIEWER_H
#define SKELETON_VIEWER_H

#include <array>

#include "Pose.h"
#include "shader.h"

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

   void LoadBoneBuffers();
   void LoadJointBuffers();

   void ConfigureBonesVAO(int posAttribLocation, int colorAttribLocation);
   void ConfigureJointsVAO(int posAttribLocation, int normalAttribLocation);

   void BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void UnbindAttribute(int attribLocation, unsigned int VBO);

   void RenderBones();
   void RenderJoints(const Transform& model, const glm::mat4& projectionView, const std::vector<glm::mat4>& animatedPosePalette);

private:

   void ResizeContainers(const Pose& animatedPose);

   unsigned int             mBonesVAO;
   unsigned int             mBonesVBO;

   unsigned int             mJointsVAO;
   unsigned int             mJointsVBO;
   unsigned int             mJointsEBO;

   std::shared_ptr<Shader>  mJointShader;

   std::array<glm::vec3, 3> mColorPalette;
   std::vector<glm::vec3>   mJointPositions;
   std::vector<glm::vec3>   mJointColors;
};

#endif
