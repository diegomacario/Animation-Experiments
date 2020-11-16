#include <glad/glad.h>

#include "AnimatedMesh.h"
#include "Transform.h"

AnimatedMesh::AnimatedMesh()
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);
   glGenBuffers(1, &mEBO);
}

AnimatedMesh::~AnimatedMesh()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
   glDeleteBuffers(1, &mEBO);
}

AnimatedMesh::AnimatedMesh(AnimatedMesh&& rhs) noexcept
   : mVertices(std::move(rhs.mVertices))
   , mIndices(std::move(rhs.mIndices))
   , mNumIndices(std::exchange(rhs.mNumIndices, 0))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
   , mEBO(std::exchange(rhs.mEBO, 0))
{

}

AnimatedMesh& AnimatedMesh::operator=(AnimatedMesh&& rhs) noexcept
{
   mVertices   = std::move(rhs.mVertices);
   mIndices    = std::move(rhs.mIndices);
   mNumIndices = std::exchange(rhs.mNumIndices, 0);
   mVAO        = std::exchange(rhs.mVAO, 0);
   mVBO        = std::exchange(rhs.mVBO, 0);
   mEBO        = std::exchange(rhs.mEBO, 0);
   return *this;
}

std::vector<AnimatedVertex>& AnimatedMesh::GetVertices()
{
   return mVertices;
}

std::vector<unsigned int>& AnimatedMesh::GetIndices()
{
   return mIndices;
}

void AnimatedMesh::LoadBuffers()
{
   glBindVertexArray(mVAO);

   // Load the mesh's data into the buffers

   // Positions, normals, texture coordinates, weights and influences
   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(AnimatedVertex), &mVertices[0], GL_STATIC_DRAW); // TODO: Is GL_STREAM_DRAW faster?
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW); // TODO: Is GL_STREAM_DRAW faster?

   // Unbind the VAO first, then the EBO
   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void AnimatedMesh::ConfigureVAO(int posAttribLocation,
                                int normalAttribLocation,
                                int texCoordAttribLocation,
                                int weightAttribLocation,
                                int influenceAttribLocation)
{
   glBindVertexArray(mVAO);
   glBindBuffer(GL_ARRAY_BUFFER, mVBO); // TODO: Is this necessary?

   // Set the vertex attribute pointers

   // Positions
   if (posAttribLocation >= 0)
   {
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)0);
   }

   // Normals
   if (normalAttribLocation >= 0)
   {
      glEnableVertexAttribArray(normalAttribLocation);
      glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, normal));
   }

   // Texture coordinates
   if (texCoordAttribLocation >= 0)
   {
      glEnableVertexAttribArray(texCoordAttribLocation);
      glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, texCoords));
   }

   // Weights
   if (weightAttribLocation >= 0)
   {
      glEnableVertexAttribArray(weightAttribLocation);
      glVertexAttribPointer(weightAttribLocation, 4, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, weights));
   }

   // Influences
   if (influenceAttribLocation >= 0)
   {
      glEnableVertexAttribArray(influenceAttribLocation);
      glVertexAttribPointer(influenceAttribLocation, 4, GL_INT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, influences));
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0); // TODO: Is this necessary?
   glBindVertexArray(0);
}

void AnimatedMesh::Render()
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, 0);
   }
   else
   {
      glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
   }

   glBindVertexArray(0);
}

void AnimatedMesh::RenderInstanced(unsigned int numInstances)
{
   glBindVertexArray(mVAO);

   if (mIndices.size() > 0)
   {
      glDrawElementsInstanced(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, 0, numInstances);
   }
   else
   {
      glDrawArraysInstanced(GL_TRIANGLES, 0, mVertices.size(), numInstances);
   }

   glBindVertexArray(0);
}

#if 1
void AnimatedMesh::CPUSkin(Skeleton& skeleton, Pose& pose)
{
   unsigned int numVertices = static_cast<unsigned int>(mVertices.size());

   if (numVertices == 0)
   {
      return;
   }

   mSkinnedPositions.resize(numVertices);
   mSkinnedNormals.resize(numVertices);

   pose.GetMatrixPalette(mPosePalette);
   std::vector<glm::mat4> invBindPosePalette = skeleton.GetInvBindPose();

   for (unsigned int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
   {
      glm::ivec4& influencesOfCurrVertex = mVertices[vertexIndex].influences;
      glm::vec4&  weightsOfCurrVertex    = mVertices[vertexIndex].weights;

      glm::mat4 m0 = (mPosePalette[influencesOfCurrVertex.x] * invBindPosePalette[influencesOfCurrVertex.x]) * weightsOfCurrVertex.x;
      glm::mat4 m1 = (mPosePalette[influencesOfCurrVertex.y] * invBindPosePalette[influencesOfCurrVertex.y]) * weightsOfCurrVertex.y;
      glm::mat4 m2 = (mPosePalette[influencesOfCurrVertex.z] * invBindPosePalette[influencesOfCurrVertex.z]) * weightsOfCurrVertex.z;
      glm::mat4 m3 = (mPosePalette[influencesOfCurrVertex.w] * invBindPosePalette[influencesOfCurrVertex.w]) * weightsOfCurrVertex.w;

      glm::mat4 skinMatrix = m0 + m1 + m2 + m3;

      // We have reached an impasse here
      mSkinnedPositions[vertexIndex] = skinMatrix * glm::vec4(mVertices[vertexIndex].position, 1.0f);
      mSkinnedNormals[vertexIndex]   = skinMatrix * glm::vec4(mVertices[vertexIndex].normal, 0.0f);
   }

   // TODO: This is inefficient because we only modified positions/normals, but we update all buffers
   LoadBuffers();
}
#else
void AnimatedMesh::CPUSkin(Skeleton& skeleton, Pose& pose)
{
   unsigned int numVerts = (unsigned int)mPosition.size();
   if (numVerts == 0) { return; }

   mSkinnedPosition.resize(numVerts);
   mSkinnedNormal.resize(numVerts);
   Pose& bindPose = skeleton.GetBindPose();

   for (unsigned int i = 0; i < numVerts; ++i)
   {
      glm::ivec4& joint = mInfluences[i];
      glm::vec4& weight = mWeights[i];

      Transform skin0 = combine(pose[joint.x], inverse(bindPose[joint.x]));
      glm::vec3 p0 = transformPoint(skin0, mPosition[i]);
      glm::vec3 n0 = transformVector(skin0, mNormal[i]);

      Transform skin1 = combine(pose[joint.y], inverse(bindPose[joint.y]));
      glm::vec3 p1 = transformPoint(skin1, mPosition[i]);
      glm::vec3 n1 = transformVector(skin1, mNormal[i]);

      Transform skin2 = combine(pose[joint.z], inverse(bindPose[joint.z]));
      glm::vec3 p2 = transformPoint(skin2, mPosition[i]);
      glm::vec3 n2 = transformVector(skin2, mNormal[i]);

      Transform skin3 = combine(pose[joint.w], inverse(bindPose[joint.w]));
      glm::vec3 p3 = transformPoint(skin3, mPosition[i]);
      glm::vec3 n3 = transformVector(skin3, mNormal[i]);
      mSkinnedPosition[i] = p0 * weight.x + p1 * weight.y + p2 * weight.z + p3 * weight.w;
      mSkinnedNormal[i] = n0 * weight.x + n1 * weight.y + n2 * weight.z + n3 * weight.w;
   }

   mPosAttrib->Set(mSkinnedPosition);
   mNormAttrib->Set(mSkinnedNormal);
}
#endif

void AnimatedMesh::CPUSkin(std::vector<glm::mat4>& animatedPose)
{
   unsigned int numVerts = (unsigned int)mPosition.size();
   if (numVerts == 0) { return; }

   mSkinnedPosition.resize(numVerts);
   mSkinnedNormal.resize(numVerts);

   for (unsigned int i = 0; i < numVerts; ++i)
   {
      glm::ivec4& j = mInfluences[i];
      glm::vec4& w  = mWeights[i];

      glm::vec3 p0 = transformPoint(animatedPose[j.x], mPosition[i]);
      glm::vec3 p1 = transformPoint(animatedPose[j.y], mPosition[i]);
      glm::vec3 p2 = transformPoint(animatedPose[j.z], mPosition[i]);
      glm::vec3 p3 = transformPoint(animatedPose[j.w], mPosition[i]);
      mSkinnedPosition[i] = p0 * w.x + p1 * w.y + p2 * w.z + p3 * w.w;

      glm::vec3 n0 = transformVector(animatedPose[j.x], mNormal[i]);
      glm::vec3 n1 = transformVector(animatedPose[j.y], mNormal[i]);
      glm::vec3 n2 = transformVector(animatedPose[j.z], mNormal[i]);
      glm::vec3 n3 = transformVector(animatedPose[j.w], mNormal[i]);
      mSkinnedNormal[i] = n0 * w.x + n1 * w.y + n2 * w.z + n3 * w.w;
   }

   mPosAttrib->Set(mSkinnedPosition);
   mNormAttrib->Set(mSkinnedNormal);
}
