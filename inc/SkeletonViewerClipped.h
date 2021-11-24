#ifndef SKELETON_VIEWER_CLIPPED_H
#define SKELETON_VIEWER_CLIPPED_H

#include <array>

#include "Pose.h"
#include "shader.h"

class SkeletonViewerClipped
{
public:

   SkeletonViewerClipped();
   ~SkeletonViewerClipped();

   SkeletonViewerClipped(const SkeletonViewerClipped&) = delete;
   SkeletonViewerClipped& operator=(const SkeletonViewerClipped&) = delete;

   SkeletonViewerClipped(SkeletonViewerClipped&& rhs) noexcept;
   SkeletonViewerClipped& operator=(SkeletonViewerClipped&& rhs) noexcept;

   void InitializeBones(const Pose& pose);

   void UpdateBones(const Pose& animatedPose, const std::vector<glm::mat4>& animatedPosePalette);

   void RenderBones(const Transform& model, const glm::mat4& projectionView, const glm::vec2& horizontalClippingPlaneYNormalAndHeight);
   void RenderJoints(const Transform& model, const glm::mat4& projectionView, const std::vector<glm::mat4>& animatedPosePalette, const glm::vec2& horizontalClippingPlaneYNormalAndHeight);

private:

   void LoadBoneBuffers();
   void ConfigureBonesVAO(int posAttribLocation, int colorAttribLocation);

   void LoadJointBuffers();
   void ConfigureJointsVAO(int posAttribLocation, int normalAttribLocation);

   void ResizeBoneContainers(const Pose& animatedPose);

   unsigned int             mBonesVAO;
   unsigned int             mBonesVBO;

   unsigned int             mJointsVAO;
   unsigned int             mJointsVBO;
   unsigned int             mJointsEBO;

   std::shared_ptr<Shader>  mBoneShader;
   std::shared_ptr<Shader>  mJointShader;

   std::array<glm::vec3, 3> mBoneColorPalette;
   std::vector<glm::vec3>   mBonePositions;
   std::vector<glm::vec3>   mBoneColors;
};

#endif
