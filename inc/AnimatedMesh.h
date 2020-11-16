#ifndef ANIMATED_MESH_H
#define ANIMATED_MESH_H

#include "Skeleton.h"
#include "Pose.h"

struct AnimatedVertex
{
   AnimatedVertex(const glm::vec3&  position,
                  const glm::vec3&  normal,
                  const glm::vec2&  texCoords,
                  const glm::vec4&  weights,
                  const glm::ivec4& influences)
      : position(position)
      , normal(normal)
      , texCoords(texCoords)
      , weights(weights)
      , influences(influences)
   {

   }

   ~AnimatedVertex() = default;

   AnimatedVertex(const AnimatedVertex&) = default;
   AnimatedVertex& operator=(const AnimatedVertex&) = default;

   AnimatedVertex(AnimatedVertex&& rhs) noexcept
      : position(std::exchange(rhs.position, glm::vec3(0.0f)))
      , normal(std::exchange(rhs.normal, glm::vec3(0.0f)))
      , texCoords(std::exchange(rhs.texCoords, glm::vec2(0.0f)))
      , weights(std::exchange(rhs.weights, glm::vec4(0.0f)))
      , influences(std::exchange(rhs.influences, glm::ivec4(0.0f)))
   {

   }

   AnimatedVertex& operator=(AnimatedVertex&& rhs) noexcept
   {
      position   = std::exchange(rhs.position, glm::vec3(0.0f));
      normal     = std::exchange(rhs.normal, glm::vec3(0.0f));
      texCoords  = std::exchange(rhs.texCoords, glm::vec2(0.0f));
      weights    = std::exchange(rhs.weights, glm::vec4(0.0f));
      influences = std::exchange(rhs.influences, glm::ivec4(0.0f));
      return *this;
   }

   glm::vec3  position;
   glm::vec3  normal;
   glm::vec2  texCoords;
   glm::vec4  weights;
   glm::ivec4 influences;
};

class AnimatedMesh
{
public:

   AnimatedMesh();
   ~AnimatedMesh();

   AnimatedMesh(const AnimatedMesh&) = delete;
   AnimatedMesh& operator=(const AnimatedMesh&) = delete;

   AnimatedMesh(AnimatedMesh&& rhs) noexcept;
   AnimatedMesh& operator=(AnimatedMesh&& rhs) noexcept;

   std::vector<AnimatedVertex>& GetVertices();
   std::vector<unsigned int>&   GetIndices();

   void                         LoadBuffers();

   void                         ConfigureVAO(int posAttribLocation,
                                             int normalAttribLocation,
                                             int texCoordAttribLocation,
                                             int weightAttribLocation,
                                             int influenceAttribLocation);

   void                         Render();
   void                         RenderInstanced(unsigned int numInstances);

   void                         CPUSkin(Skeleton& skeleton, Pose& pose);
   void                         CPUSkin(std::vector<glm::mat4>& animated);

private:

   std::vector<AnimatedVertex> mVertices;
   std::vector<unsigned int>   mIndices;

   unsigned int                mNumIndices;
   unsigned int                mVAO;
   unsigned int                mVBO;
   unsigned int                mEBO;

   std::vector<glm::vec3>      mSkinnedPositions;
   std::vector<glm::vec3>      mSkinnedNormals;
   std::vector<glm::mat4>      mPosePalette;
};

#endif
