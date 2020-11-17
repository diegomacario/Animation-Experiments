#include <glad/glad.h>

#include "AnimatedMesh.h"
#include "Transform.h"

AnimatedMesh::AnimatedMesh()
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(5, &mVBOs[0]);
   glGenBuffers(1, &mEBO);
}

AnimatedMesh::~AnimatedMesh()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(5, &mVBOs[0]);
   glDeleteBuffers(1, &mEBO);
}

AnimatedMesh::AnimatedMesh(AnimatedMesh&& rhs) noexcept
   : mPositions(std::move(rhs.mPositions))
   , mNormals(std::move(rhs.mNormals))
   , mTexCoords(std::move(rhs.mTexCoords))
   , mWeights(std::move(rhs.mWeights))
   , mInfluences(std::move(rhs.mInfluences))
   , mIndices(std::move(rhs.mIndices))
   , mNumIndices(std::exchange(rhs.mNumIndices, 0))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBOs(std::exchange(rhs.mVBOs, std::array<unsigned int, 5>()))
   , mEBO(std::exchange(rhs.mEBO, 0))
{

}

AnimatedMesh& AnimatedMesh::operator=(AnimatedMesh&& rhs) noexcept
{
   mPositions  = std::move(rhs.mPositions);
   mNormals    = std::move(rhs.mNormals);
   mTexCoords  = std::move(rhs.mTexCoords);
   mWeights    = std::move(rhs.mWeights);
   mInfluences = std::move(rhs.mInfluences);
   mIndices    = std::move(rhs.mIndices);
   mNumIndices = std::exchange(rhs.mNumIndices, 0);
   mVAO        = std::exchange(rhs.mVAO, 0);
   mVBOs       = std::exchange(rhs.mVBOs, std::array<unsigned int, 5>());
   mEBO        = std::exchange(rhs.mEBO, 0);
   return *this;
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void AnimatedMesh::LoadBuffers()
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

void AnimatedMesh::ConfigureVAO(int posAttribLocation,
                                int normalAttribLocation,
                                int texCoordsAttribLocation,
                                int weightsAttribLocation,
                                int influencesAttribLocation)
{
   glBindVertexArray(mVAO);

   // Set the vertex attribute pointers

   // Positions
   if (posAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   }

   // Normals
   if (normalAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
      glEnableVertexAttribArray(normalAttribLocation);
      glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   }

   // Texture coordinates
   if (texCoordsAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::texCoords]);
      glEnableVertexAttribArray(texCoordsAttribLocation);
      glVertexAttribPointer(texCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
   }

   // Weights
   if (weightsAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::weights]);
      glEnableVertexAttribArray(weightsAttribLocation);
      glVertexAttribPointer(weightsAttribLocation, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
   }

   // Influences
   if (influencesAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::influences]);
      glEnableVertexAttribArray(influencesAttribLocation);
      glVertexAttribPointer(influencesAttribLocation, 4, GL_INT, GL_FALSE, 4 * sizeof(int), (void*)0);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
}

// TODO: GL_TRIANGLES shouldn't be hardcoded here
//       Can we load that from the GLTF file?
void AnimatedMesh::Render()
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
void AnimatedMesh::RenderInstanced(unsigned int numInstances)
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

void AnimatedMesh::CPUSkinWithMatrices(Skeleton& skeleton, Pose& pose)
{
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   const std::vector<glm::mat4>& invBindPosePalette = skeleton.GetInvBindPose();
   pose.GetMatrixPalette(mPosePalette);

   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      const glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      const glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      glm::mat4 m0 = (mPosePalette[influencesOfCurrVertex.x] * invBindPosePalette[influencesOfCurrVertex.x]) * weightsOfCurrVertex.x;
      glm::mat4 m1 = (mPosePalette[influencesOfCurrVertex.y] * invBindPosePalette[influencesOfCurrVertex.y]) * weightsOfCurrVertex.y;
      glm::mat4 m2 = (mPosePalette[influencesOfCurrVertex.z] * invBindPosePalette[influencesOfCurrVertex.z]) * weightsOfCurrVertex.z;
      glm::mat4 m3 = (mPosePalette[influencesOfCurrVertex.w] * invBindPosePalette[influencesOfCurrVertex.w]) * weightsOfCurrVertex.w;

      glm::mat4 skinMatrix = m0 + m1 + m2 + m3;

      mSkinnedPositions[vertexIndex] = skinMatrix * glm::vec4(mPositions[vertexIndex], 1.0f);
      mSkinnedNormals[vertexIndex]   = skinMatrix * glm::vec4(mNormals[vertexIndex], 0.0f);
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void AnimatedMesh::CPUSkinWithTransforms(Skeleton& skeleton, Pose& pose)
{
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   const Pose& bindPose = skeleton.GetBindPose();

   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      const glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      const glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      Transform skin0 = combine(pose.GetGlobalTransform(influencesOfCurrVertex.x), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.x)));
      glm::vec3 p0    = transformPoint(skin0, mPositions[vertexIndex]);
      glm::vec3 n0    = transformVector(skin0, mNormals[vertexIndex]);

      Transform skin1 = combine(pose.GetGlobalTransform(influencesOfCurrVertex.y), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.y)));
      glm::vec3 p1    = transformPoint(skin1, mPositions[vertexIndex]);
      glm::vec3 n1    = transformVector(skin1, mNormals[vertexIndex]);

      Transform skin2 = combine(pose.GetGlobalTransform(influencesOfCurrVertex.z), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.z)));
      glm::vec3 p2    = transformPoint(skin2, mPositions[vertexIndex]);
      glm::vec3 n2    = transformVector(skin2, mNormals[vertexIndex]);

      Transform skin3 = combine(pose.GetGlobalTransform(influencesOfCurrVertex.w), inverse(bindPose.GetGlobalTransform(influencesOfCurrVertex.w)));
      glm::vec3 p3    = transformPoint(skin3, mPositions[vertexIndex]);
      glm::vec3 n3    = transformVector(skin3, mNormals[vertexIndex]);

      mSkinnedPositions[vertexIndex] = (p0 * weightsOfCurrVertex.x) + (p1 * weightsOfCurrVertex.y) + (p2 * weightsOfCurrVertex.z) + (p3 * weightsOfCurrVertex.w);
      mSkinnedNormals[vertexIndex]   = (n0 * weightsOfCurrVertex.x) + (n1 * weightsOfCurrVertex.y) + (n2 * weightsOfCurrVertex.z) + (n3 * weightsOfCurrVertex.w);
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// animatedPose is simply the result of the following: mPosePalette[] * invBindPosePalette[]
void AnimatedMesh::CPUSkin(std::vector<glm::mat4>& animatedPose)
{
   unsigned int numVertices = static_cast<unsigned int>(mPositions.size());
   if (numVertices == 0)
   {
      return;
   }

   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      glm::ivec4& influencesOfCurrVertex = mInfluences[vertexIndex];
      glm::vec4&  weightsOfCurrVertex    = mWeights[vertexIndex];

      glm::vec3 p0 = animatedPose[influencesOfCurrVertex.x] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 p1 = animatedPose[influencesOfCurrVertex.y] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 p2 = animatedPose[influencesOfCurrVertex.z] * glm::vec4(mPositions[vertexIndex], 1.0f);
      glm::vec3 p3 = animatedPose[influencesOfCurrVertex.w] * glm::vec4(mPositions[vertexIndex], 1.0f);
      mSkinnedPositions[vertexIndex] = p0 * weightsOfCurrVertex.x + p1 * weightsOfCurrVertex.y + p2 * weightsOfCurrVertex.z + p3 * weightsOfCurrVertex.w;

      glm::vec3 n0 = animatedPose[influencesOfCurrVertex.x] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 n1 = animatedPose[influencesOfCurrVertex.y] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 n2 = animatedPose[influencesOfCurrVertex.z] * glm::vec4(mNormals[vertexIndex], 0.0f);
      glm::vec3 n3 = animatedPose[influencesOfCurrVertex.w] * glm::vec4(mNormals[vertexIndex], 0.0f);
      mSkinnedNormals[vertexIndex] = n0 * weightsOfCurrVertex.x + n1 * weightsOfCurrVertex.y + n2 * weightsOfCurrVertex.z + n3 * weightsOfCurrVertex.w;
   }

   // TODO: Should I bind the VAO here?

   // Load the skinned positions and normals into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::positions]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedPositions.size() * sizeof(glm::vec3), &mSkinnedPositions[0]);
   // Normals
   glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBOTypes::normals]);
   glBufferSubData(GL_ARRAY_BUFFER, 0, mSkinnedNormals.size() * sizeof(glm::vec3), &mSkinnedNormals[0]);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
