#ifndef SKELETON_VIEWER_H
#define SKELETON_VIEWER_H

#include "Skeleton.h"
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

   void                       LoadBuffers();

   void                       ConfigureVAO(int posAttribLocation,
                                           int normalAttribLocation,
                                           int texCoordsAttribLocation,
                                           int weightsAttribLocation,
                                           int influencesAttribLocation);

   void                       UnconfigureVAO(int posAttribLocation,
                                             int normalAttribLocation,
                                             int texCoordsAttribLocation,
                                             int weightsAttribLocation,
                                             int influencesAttribLocation);

   void                       BindFloatAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void                       BindIntAttribute(int attribLocation, unsigned int VBO, int numComponents);
   void                       UnbindAttribute(int attribLocation, unsigned int VBO);

   void                       Render();
   void                       RenderInstanced(unsigned int numInstances);

private:

   unsigned int           mVAO;
   unsigned int           mVBO;
   unsigned int           mEBO;

   std::vector<glm::vec3> mJointPositions;
};

#endif
