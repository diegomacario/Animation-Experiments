#include <iostream>

#include "mesh.h"

Mesh::Mesh(const std::vector<Vertex>&       vertices,
           const std::vector<unsigned int>& indices,
           const Material&                  material)
   : mNumIndices(static_cast<unsigned int>(indices.size()))
   , mMaterial(material)
{
   configureVAO(vertices, indices);
}

Mesh::~Mesh()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
   glDeleteBuffers(1, &mEBO);
}

Mesh::Mesh(Mesh&& rhs) noexcept
   : mNumIndices(std::exchange(rhs.mNumIndices, 0))
   , mMaterial(std::move(rhs.mMaterial))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
   , mEBO(std::exchange(rhs.mEBO, 0))
{

}

Mesh& Mesh::operator=(Mesh&& rhs) noexcept
{
   mNumIndices = std::exchange(rhs.mNumIndices, 0);
   mMaterial   = std::move(rhs.mMaterial);
   mVAO        = std::exchange(rhs.mVAO, 0);
   mVBO        = std::exchange(rhs.mVBO, 0);
   mEBO        = std::exchange(rhs.mEBO, 0);
   return *this;
}

void Mesh::render(const Shader& shader) const
{
   bindMaterialTextures(shader);
   setMaterialTextureAvailabilities(shader);
   setMaterialConstants(shader);

   glBindVertexArray(mVAO);
   glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

void Mesh::configureVAO(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);
   glGenBuffers(1, &mEBO);

   glBindVertexArray(mVAO);

   // Load the mesh's data into the buffers

   // Positions, normals and texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Set the vertex attribute pointers

   // Positions
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
   // Normals
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
   // Texture coords
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

   glBindVertexArray(0);
}

void Mesh::bindMaterialTextures(const Shader& shader) const
{
   unsigned int texUnit = 0;

   for (unsigned int i = 0; i < mMaterial.textures.size(); ++i)
   {
      // Get the location of the sampler2D uniform that should exist in the shader
      int uniformLoc = glGetUniformLocation(shader.getID(), mMaterial.textures[i].uniformName.c_str());

      if (uniformLoc != -1)
      {
         // Bind the texture
         mMaterial.textures[i].texture->bind(texUnit, uniformLoc);
         ++texUnit;
      }
      else
      {
         std::cout << "Error - Mesh::bindMaterialTextures - The following sampler2D uniform does not exist: " << mMaterial.textures[i].uniformName << "\n";
      }
   }

   glActiveTexture(GL_TEXTURE0);
}

void Mesh::setMaterialTextureAvailabilities(const Shader& shader) const
{
   shader.setUniformInt("materialTextureAvailabilities.ambientTexIsAvailable", mMaterial.textureAvailabilities.test(static_cast<unsigned int>(MaterialTextureTypes::ambient)));
   shader.setUniformInt("materialTextureAvailabilities.emissiveTexIsAvailable", mMaterial.textureAvailabilities.test(static_cast<unsigned int>(MaterialTextureTypes::emissive)));
   shader.setUniformInt("materialTextureAvailabilities.diffuseTexIsAvailable", mMaterial.textureAvailabilities.test(static_cast<unsigned int>(MaterialTextureTypes::diffuse)));
   shader.setUniformInt("materialTextureAvailabilities.specularTexIsAvailable", mMaterial.textureAvailabilities.test(static_cast<unsigned int>(MaterialTextureTypes::specular)));
}

void Mesh::setMaterialConstants(const Shader& shader) const
{
   shader.setUniformVec3("materialConstants.ambient", mMaterial.constants.ambientColor);
   shader.setUniformVec3("materialConstants.emissive", mMaterial.constants.emissiveColor);
   shader.setUniformVec3("materialConstants.diffuse", mMaterial.constants.diffuseColor);
   shader.setUniformVec3("materialConstants.specular", mMaterial.constants.specularColor);
   shader.setUniformFloat("materialConstants.shininess", mMaterial.constants.shininess);
}
