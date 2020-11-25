#include <glad/glad.h>

#include "resource_manager.h"
#include "shader_loader.h"
#include "SkeletonViewer.h"

SkeletonViewer::SkeletonViewer()
   : mColorPalette{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.65f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f)}
{
   glGenVertexArrays(1, &mBonesVAO);
   glGenBuffers(1, &mBonesVBO);

   glGenVertexArrays(1, &mJointsVAO);
   glGenBuffers(1, &mJointsVBO);
   glGenBuffers(1, &mJointsEBO);

   mJointShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/joint.vert",
                                                                                "resources/shaders/joint.frag");
   mJointShader->use(true);
   mJointShader->setUniformVec3("pointLights[0].worldPos", glm::vec3(0.0f, 2.0f, 10.0f));
   mJointShader->setUniformVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mJointShader->setUniformFloat("pointLights[0].constantAtt", 1.0f);
   mJointShader->setUniformFloat("pointLights[0].linearAtt", 0.01f);
   mJointShader->setUniformFloat("pointLights[0].quadraticAtt", 0.0f);
   mJointShader->setUniformVec3("pointLights[1].worldPos", glm::vec3(0.0f, 2.0f, -10.0f));
   mJointShader->setUniformVec3("pointLights[1].color", glm::vec3(1.0f, 1.0f, 1.0f));
   mJointShader->setUniformFloat("pointLights[1].constantAtt", 1.0f);
   mJointShader->setUniformFloat("pointLights[1].linearAtt", 0.01f);
   mJointShader->setUniformFloat("pointLights[1].quadraticAtt", 0.0f);
   mJointShader->setUniformInt("numPointLightsInScene", 2);
   mJointShader->use(false);

   LoadJointBuffers();
   ConfigureJointsVAO(mJointShader->getAttributeLocation("inPos"), mJointShader->getAttributeLocation("inNormal"));
}

SkeletonViewer::~SkeletonViewer()
{
   glDeleteVertexArrays(1, &mBonesVAO);
   glDeleteBuffers(1, &mBonesVBO);

   glDeleteVertexArrays(1, &mJointsVAO);
   glDeleteBuffers(1, &mJointsVBO);
   glDeleteBuffers(1, &mJointsEBO);
}

SkeletonViewer::SkeletonViewer(SkeletonViewer&& rhs) noexcept
   : mBonesVAO(std::exchange(rhs.mBonesVAO, 0))
   , mBonesVBO(std::exchange(rhs.mBonesVBO, 0))
   , mJointsVAO(std::exchange(rhs.mJointsVAO, 0))
   , mJointsVBO(std::exchange(rhs.mJointsVBO, 0))
   , mJointsEBO(std::exchange(rhs.mJointsEBO, 0))
   , mColorPalette(std::move(rhs.mColorPalette))
   , mJointPositions(std::move(rhs.mJointPositions))
   , mJointColors(std::move(rhs.mJointColors))
{

}

SkeletonViewer& SkeletonViewer::operator=(SkeletonViewer&& rhs) noexcept
{
   mBonesVAO       = std::exchange(rhs.mBonesVAO, 0);
   mBonesVBO       = std::exchange(rhs.mBonesVBO, 0);
   mJointsVAO      = std::exchange(rhs.mJointsVAO, 0);
   mJointsVBO      = std::exchange(rhs.mJointsVBO, 0);
   mJointsEBO      = std::exchange(rhs.mJointsEBO, 0);
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
void SkeletonViewer::LoadBoneBuffers()
{
   glBindVertexArray(mBonesVAO);

   glBindBuffer(GL_ARRAY_BUFFER, mBonesVBO);
   glBufferData(GL_ARRAY_BUFFER, (mJointPositions.size() + mJointColors.size()) * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW); // TODO: GL_DYNAMIC?
   // Positions
   glBufferSubData(GL_ARRAY_BUFFER, 0, mJointPositions.size() * sizeof(glm::vec3), &mJointPositions[0]); // TODO: GL_DYNAMIC?
   // Colors
   glBufferSubData(GL_ARRAY_BUFFER, mJointPositions.size() * sizeof(glm::vec3), mJointColors.size() * sizeof(glm::vec3), &mJointColors[0]);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glBindVertexArray(0);
}

// TODO: Experiment with GL_STATIC_DRAW, GL_STREAM_DRAW and GL_DYNAMIC_DRAW to see which is faster
void SkeletonViewer::LoadJointBuffers()
{
   std::vector<float> positions = { 0.000000f, 1.000000f, -0.000000f, -0.500000f, 0.000000f,  0.500000f,  0.500000f, 0.000000f,  0.500000f,
                                    0.000000f, 1.000000f, -0.000000f,  0.500000f, 0.000000f,  0.500000f,  0.500000f, 0.000000f, -0.500000f,
                                    0.000000f, 1.000000f, -0.000000f,  0.500000f, 0.000000f, -0.500000f, -0.500000f, 0.000000f, -0.500000f,
                                    0.000000f, 1.000000f, -0.000000f, -0.500000f, 0.000000f, -0.500000f, -0.500000f, 0.000000f,  0.500000f,
                                   -0.500000f, 0.000000f,  0.500000f,  0.000000f, 0.000000f, -0.000000f,  0.500000f, 0.000000f,  0.500000f,
                                    0.500000f, 0.000000f,  0.500000f,  0.000000f, 0.000000f, -0.000000f,  0.500000f, 0.000000f, -0.500000f,
                                    0.500000f, 0.000000f, -0.500000f,  0.000000f, 0.000000f, -0.000000f, -0.500000f, 0.000000f, -0.500000f,
                                   -0.500000f, 0.000000f, -0.500000f,  0.000000f, 0.000000f, -0.000000f, -0.500000f, 0.000000f,  0.500000f };

   std::vector<float> normals = { 0.000000f,  1.000000f, -0.000000f, -0.662600f, -0.349100f,  0.662600f,  0.662600f, -0.349100f,  0.662600f,
                                  0.000000f,  1.000000f, -0.000000f,  0.662600f, -0.349100f,  0.662600f,  0.662600f, -0.349100f, -0.662600f,
                                  0.000000f,  1.000000f, -0.000000f,  0.662600f, -0.349100f, -0.662600f, -0.662600f, -0.349100f, -0.662600f,
                                  0.000000f,  1.000000f, -0.000000f, -0.662600f, -0.349100f, -0.662600f, -0.662600f, -0.349100f,  0.662600f,
                                 -0.662600f, -0.349100f,  0.662600f,  0.000000f, -1.000000f, -0.000000f,  0.662600f, -0.349100f,  0.662600f,
                                  0.662600f, -0.349100f,  0.662600f,  0.000000f, -1.000000f, -0.000000f,  0.662600f, -0.349100f, -0.662600f,
                                  0.662600f, -0.349100f, -0.662600f,  0.000000f, -1.000000f, -0.000000f, -0.662600f, -0.349100f, -0.662600f,
                                 -0.662600f, -0.349100f, -0.662600f,  0.000000f, -1.000000f, -0.000000f, -0.662600f, -0.349100f,  0.662600f };

   std::vector<unsigned int> indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };

   glBindVertexArray(mJointsVAO);

   glBindBuffer(GL_ARRAY_BUFFER, mJointsVBO);
   glBufferData(GL_ARRAY_BUFFER, (positions.size() + normals.size()) * sizeof(float), nullptr, GL_STATIC_DRAW); // TODO: GL_DYNAMIC?
   // Positions
   glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), &positions[0]); // TODO: GL_DYNAMIC?
   // Normals
   glBufferSubData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), normals.size() * sizeof(float), &normals[0]);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mJointsEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Unbind the VAO first, then the EBO
   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SkeletonViewer::ConfigureBonesVAO(int posAttribLocation, int colorAttribLocation)
{
   glBindVertexArray(mBonesVAO);

   // Set the vertex attribute pointers

   if (posAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mBonesVBO);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   if (colorAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mBonesVBO);
      glEnableVertexAttribArray(colorAttribLocation);
      glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)(mJointPositions.size() * sizeof(glm::vec3)));
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   glBindVertexArray(0);
}

void SkeletonViewer::ConfigureJointsVAO(int posAttribLocation, int normalAttribLocation)
{
   glBindVertexArray(mJointsVAO);

   // Set the vertex attribute pointers

   if (posAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mJointsVBO);
      glEnableVertexAttribArray(posAttribLocation);
      glVertexAttribPointer(posAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   if (normalAttribLocation >= 0)
   {
      glBindBuffer(GL_ARRAY_BUFFER, mJointsVBO);
      glEnableVertexAttribArray(normalAttribLocation);
      glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)(72 * sizeof(float)));
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

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

void SkeletonViewer::RenderBones()
{
   glBindVertexArray(mBonesVAO);

   glDrawArrays(GL_LINES, 0, static_cast<unsigned int>(mJointPositions.size()));

   glBindVertexArray(0);
}

void SkeletonViewer::RenderJoints(const Transform& model, const glm::mat4& projectionView, const std::vector<glm::mat4>& animatedPosePalette)
{
   mJointShader->use(true);

   // We need to combine 3 transforms:
   // - The model transform of the entire 3D character
   // - The transform of each joint
   // - The scale transform that increases the size of the little pyramids
   // The order is simple:
   // - The model transform is applied first
   // - The transform of each joint is applied second as a child of the model transform
   // - The scale transform is applied third as a child of the previous two transforms
   // What does this look like?
   // 1) Place the pyramids in the right place with the right orientation/scale
   // 2) Place the pyramids at the right joints with the right orientations/scales
   // 3) Scale the pyramids in place at their joints

   // Scale transform to increase the size of the little pyramids that represent the joints
   Transform jointScale(glm::vec3(0.0f, 0.0f, 0.0f), Q::quat(), glm::vec3(7.5f));

   // Loop over all the transforms of the pose
   std::vector<glm::mat4> combinedTransforms(animatedPosePalette.size());
   for (unsigned int i = 0; i < static_cast<unsigned int>(combinedTransforms.size()); ++i)
   {
      // Combine the transforms and store the result
      Transform modelFirstThenPose = combine(model, mat4ToTransform(animatedPosePalette[i]));
      Transform modelFirstThenPoseThenScale = combine(modelFirstThenPose, jointScale);
      combinedTransforms[i] = transformToMat4(modelFirstThenPoseThenScale);
   }

   // Render the joints
   mJointShader->setUniformMat4("projectionView", projectionView);
   mJointShader->setUniformMat4Array("modelMatrices[0]", combinedTransforms);
   glBindVertexArray(mJointsVAO);
   glDrawElementsInstanced(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0, static_cast<unsigned int>(animatedPosePalette.size()));
   glBindVertexArray(0);
   mJointShader->use(false);
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
