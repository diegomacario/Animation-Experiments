#ifndef WATER_H
#define WATER_H

#include "Transform.h"
#include "shader.h"

class Water
{
public:

   Water(const Transform& modelTransform);
   ~Water();

   void BindReflectionFBO();
   void BindRefractionFBO();
   void UnbindCurrentFBO();
   void Render(const glm::mat4& projectionView,
               const glm::vec3& cameraPosition,
               const glm::vec3& lightPosition,
               const glm::vec3& lightColor);

   float GetWaterHeight() { return mModelTransform.position.y; }
   void  UpdateMoveFactor(float deltaTime);

private:

   void ConfigureVAO();
   void ConfigureReflectionFBO();
   void ConfigureRefractionFBO();

   unsigned int CreateColorTextureAttachment(int width, int height);
   unsigned int CreateDepthTextureAttachment(int width, int height);
   unsigned int CreateDepthBufferAttachment(int width, int height);

   Transform                mModelTransform;

   unsigned int             mWaterVAO;
   unsigned int             mWaterVBO;
   unsigned int             mWaterEBO;

   unsigned int             mReflectionFBO;
   unsigned int             mReflectionColorTexture;
   unsigned int             mReflectionDepthBuffer;

   unsigned int             mRefractionFBO;
   unsigned int             mRefractionColorTexture;
   unsigned int             mRefractionDepthTexture;

   std::shared_ptr<Shader>  mWaterShader;
   std::shared_ptr<Texture> mDuDvMap;
   float                    mWaveSpeed;
   float                    mMoveFactor;

   std::shared_ptr<Texture> mNormalMap;
};

#endif
