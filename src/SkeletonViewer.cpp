#include <glad/glad.h>

#include "SkeletonViewer.h"
#include "Transform.h"

SkeletonViewer::SkeletonViewer()
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);
   glGenBuffers(1, &mEBO);
}

SkeletonViewer::~SkeletonViewer()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
   glDeleteBuffers(1, &mEBO);
}

SkeletonViewer::SkeletonViewer(SkeletonViewer&& rhs) noexcept
   : mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
   , mEBO(std::exchange(rhs.mEBO, 0))
   , mJointPositions(std::move(rhs.mJointPositions))
{

}

SkeletonViewer& SkeletonViewer::operator=(SkeletonViewer&& rhs) noexcept
{
   mVAO            = std::exchange(rhs.mVAO, 0);
   mVBO            = std::exchange(rhs.mVBO, 0);
   mEBO            = std::exchange(rhs.mEBO, 0);
   mJointPositions = std::move(rhs.mJointPositions);
   return *this;
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void SkeletonViewer::LoadBuffers()
{
   glBindVertexArray(mVAO);

   // Load the mesh's data into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferData(GL_ARRAY_BUFFER, mPositions.size() * sizeof(glm::vec3), &mPositions[0], GL_STATIC_DRAW);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferData(GL_ARRAY_BUFFER, mNormals.size() * sizeof(glm::vec3), &mNormals[0], GL_STATIC_DRAW);
   // Texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::texCoords]);
   glBufferData(GL_ARRAY_BUFFER, mTexCoords.size() * sizeof(glm::vec2), &mTexCoords[0], GL_STATIC_DRAW);
   // Weights
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::weights]);
   glBufferData(GL_ARRAY_BUFFER, mWeights.size() * sizeof(glm::vec4), &mWeights[0], GL_STATIC_DRAW);
   // Influences
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::influences]);
   glBufferData(GL_ARRAY_BUFFER, mInfluences.size() * sizeof(glm::ivec4), &mInfluences[0], GL_STATIC_DRAW);

   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);

   // Unbind the VAO first, then the EBO
   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SkeletonViewer::ConfigureVAO(int posAttribLocation,
                                  int normalAttribLocation,
                                  int texCoordsAttribLocation,
                                  int weightsAttribLocation,
                                  int influencesAttribLocation)
{
   glBindVertexArray(mVAO);

   // Set the vertex attribute pointers
   BindFloatAttribute(posAttribLocation,       mVBOs[VBOTypes::positions], 3);
   BindFloatAttribute(normalAttribLocation,    mVBOs[VBOTypes::normals], 3);
   BindFloatAttribute(texCoordsAttribLocation, mVBOs[VBOTypes::texCoords], 2);
   BindFloatAttribute(weightsAttribLocation,   mVBOs[VBOTypes::weights], 4);
   BindIntAttribute(influencesAttribLocation,  mVBOs[VBOTypes::influences], 4);

   glBindVertexArray(0);
}

void SkeletonViewer::UnconfigureVAO(int posAttribLocation,
                                    int normalAttribLocation,
                                    int texCoordsAttribLocation,
                                    int weightsAttribLocation,
                                    int influencesAttribLocation)
{
   glBindVertexArray(mVAO);

   // Unset the vertex attribute pointers
   UnbindAttribute(posAttribLocation,        mVBOs[VBOTypes::positions]);
   UnbindAttribute(normalAttribLocation,     mVBOs[VBOTypes::normals]);
   UnbindAttribute(texCoordsAttribLocation,  mVBOs[VBOTypes::texCoords]);
   UnbindAttribute(weightsAttribLocation,    mVBOs[VBOTypes::weights]);
   UnbindAttribute(influencesAttribLocation, mVBOs[VBOTypes::influences]);

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

void SkeletonViewer::BindIntAttribute(int attribLocation, unsigned int VBO, int numComponents)
{
   if (attribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glEnableVertexAttribArray(attribLocation);
      glVertexAttribIPointer(attribLocation, numComponents, GL_INT, 0, (void*)0);
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

// TODO: GL_TRIANGLES shouldn't be hardcoded here
//       Can we load that from the GLTF file?
void SkeletonViewer::Render()
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      // TODO: Use mNumIndices here
      glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mIndices.size()), GL_UNSIGNED_INT, 0);
   }
   else
   {
      glDrawArrays(GL_TRIANGLES, 0, static_cast<unsigned int>(mPositions.size()));
   }

   glBindVertexArray(0);
}

// TODO: GL_TRIANGLES shouldn't be hardcoded here
//       Can we load that from the GLTF file?
void SkeletonViewer::RenderInstanced(unsigned int numInstances)
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      // TODO: Use mNumIndices here
      glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(mIndices.size()), GL_UNSIGNED_INT, 0, numInstances);
   }
   else
   {
      glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<unsigned int>(mPositions.size()), numInstances);
   }

   glBindVertexArray(0);
}
