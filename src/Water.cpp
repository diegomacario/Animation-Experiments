#include "resource_manager.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "Water.h"

Water::Water(const Transform& modelTransform)
   : mModelTransform(modelTransform)
   , mWaterVAO(0)
   , mWaterVBO(0)
   , mWaterEBO(0)
   , mReflectionFBO(0)
   , mReflectionColorTexture(0)
   , mReflectionDepthBuffer(0)
   , mRefractionFBO(0)
   , mRefractionColorTexture(0)
   , mRefractionDepthTexture(0)
   , mWaterShader()
   , mDuDvMap()
   , mWaveSpeed(0.03f)
   , mMoveFactor(0.0f)
   , mNormalMap()
{
   mWaterShader = ResourceManager<Shader>().loadUnmanagedResource<ShaderLoader>("resources/shaders/water.vert",
                                                                                "resources/shaders/water.frag");

   mDuDvMap = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/water/dudv.png");

   mNormalMap = ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/models/water/normal.png");

   ConfigureVAO();
   ConfigureReflectionFBO();
   ConfigureRefractionFBO();
}

Water::~Water()
{
   glDeleteVertexArrays(1, &mWaterVAO);
   glDeleteBuffers(1, &mWaterVBO);
   glDeleteBuffers(1, &mWaterEBO);

   glDeleteFramebuffers(1, &mReflectionFBO);
   glDeleteTextures(1, &mReflectionColorTexture);
   glDeleteRenderbuffers(1, &mReflectionDepthBuffer);

   glDeleteFramebuffers(1, &mRefractionFBO);
   glDeleteTextures(1, &mRefractionDepthTexture);
   glDeleteTextures(1, &mRefractionDepthTexture);
}

void Water::BindReflectionFBO()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mReflectionFBO);
   glViewport(0, 0, 1280, 720);
   // TODO: Resize glViewport
}

void Water::BindRefractionFBO()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mRefractionFBO);
   glViewport(0, 0, 1280, 720);
   // TODO: Resize glViewport
}

void Water::UnbindCurrentFBO()
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   // TODO: Resize glViewport
}

void Water::Render(const glm::mat4& projectionView, const glm::vec3& cameraPosition, const glm::vec3& lightPosition, const glm::vec3& lightColor)
{
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   mWaterShader->use(true);
   mWaterShader->setUniformMat4("model", transformToMat4(mModelTransform));
   mWaterShader->setUniformMat4("projectionView", projectionView);
   mWaterShader->setUniformFloat("moveFactor",  mMoveFactor);
   mWaterShader->setUniformVec3("cameraPosition", cameraPosition);
   mWaterShader->setUniformVec3("lightPosition", lightPosition);
   mWaterShader->setUniformVec3("lightColor", lightColor);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, mReflectionColorTexture);
   glUniform1i(mWaterShader->getUniformLocation("reflectionTexture"), 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, mRefractionColorTexture);
   glUniform1i(mWaterShader->getUniformLocation("refractionTexture"), 1);

   mDuDvMap->bind(2, mWaterShader->getUniformLocation("dudvMap"));
   mNormalMap->bind(3, mWaterShader->getUniformLocation("normalMap"));

   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, mRefractionDepthTexture);
   glUniform1i(mWaterShader->getUniformLocation("depthMap"), 4);

   glBindVertexArray(mWaterVAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);

   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, 0);

   glActiveTexture(GL_TEXTURE0);

   mDuDvMap->unbind(2);
   mNormalMap->unbind(3);

   mWaterShader->use(false);

   glDisable(GL_BLEND);
}

void Water::UpdateMoveFactor(float deltaTime)
{
   mMoveFactor += mWaveSpeed * deltaTime;

   if (mMoveFactor >= 1.0f)
   {
      mMoveFactor = 0.0f;
   }
};

void Water::ConfigureVAO()
{
   //                              Positions            Tex Coords
   std::vector<float> vertices = { 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // Top right
                                   0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // Bottom right
                                  -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // Bottom left
                                  -0.5f,  0.5f, 0.0f,   0.0f, 1.0f }; // Top left

   std::vector<unsigned int> indices = { 0, 1, 3,   // First triangle
                                         1, 2, 3 }; // Second triangle

   glGenVertexArrays(1, &mWaterVAO);
   glBindVertexArray(mWaterVAO);

   // Load the positions and texture coordinates
   glGenBuffers(1, &mWaterVBO);
   glBindBuffer(GL_ARRAY_BUFFER, mWaterVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

   // Load the indices
   glGenBuffers(1, &mWaterEBO);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mWaterEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Connect the positions with the correct attribute of the water shader
   int positionsAttribLoc = mWaterShader->getAttributeLocation("position");
   glVertexAttribPointer(positionsAttribLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(positionsAttribLoc);

   // Connect the texture coordinates with the correct attribute of the water shader
   int texCoordsAttribLoc = mWaterShader->getAttributeLocation("texCoord");
   glVertexAttribPointer(texCoordsAttribLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
   glEnableVertexAttribArray(texCoordsAttribLoc);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Water::ConfigureReflectionFBO()
{
   glGenFramebuffers(1, &mReflectionFBO);
   glBindFramebuffer(GL_FRAMEBUFFER, mReflectionFBO);

   mReflectionColorTexture = CreateColorTextureAttachment(1280, 720);
   mReflectionDepthBuffer = CreateDepthBufferAttachment(1280, 720);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Water::ConfigureReflectionFBO - Reflection framebuffer is not complete" << "\n";
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Water::ConfigureRefractionFBO()
{
   glGenFramebuffers(1, &mRefractionFBO);
   glBindFramebuffer(GL_FRAMEBUFFER, mRefractionFBO);

   mRefractionColorTexture = CreateColorTextureAttachment(1280, 720);
   mRefractionDepthTexture = CreateDepthTextureAttachment(1280, 720);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Water::ConfigureRefractionFBO - Refraction framebuffer is not complete" << "\n";
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int Water::CreateColorTextureAttachment(int width, int height)
{
   // Create a texture and use it as a color attachment
   unsigned int colorTexture;
   glGenTextures(1, &colorTexture);
   glBindTexture(GL_TEXTURE_2D, colorTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
   return colorTexture;
}

unsigned int Water::CreateDepthTextureAttachment(int width, int height)
{
   // Create a texture and use it as a depth attachment
   unsigned int depthTexture;
   glGenTextures(1, &depthTexture);
   glBindTexture(GL_TEXTURE_2D, depthTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr); // TODO: GL_DEPTH_COMPONENT32 for 3rd param?
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
   return depthTexture;
}

unsigned int Water::CreateDepthBufferAttachment(int width, int height)
{
   // Create a buffer and use it as a depth attachment

   unsigned int depthBuffer;
   glGenRenderbuffers(1, &depthBuffer);
   glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
   return depthBuffer;
}
