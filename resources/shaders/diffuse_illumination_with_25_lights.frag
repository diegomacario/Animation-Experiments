in vec3 fragPos;
in vec3 norm;
in vec2 uv;

struct PointLight
{
   vec3  worldPos;
   vec3  color;
};

#define MAX_NUMBER_OF_POINT_LIGHTS 25
uniform PointLight pointLights[MAX_NUMBER_OF_POINT_LIGHTS];
uniform int numPointLightsInScene;

uniform vec3 cameraPos;

uniform sampler2D diffuseTex;

uniform float constantAtt;
uniform float linearAtt;
uniform float quadraticAtt;

out vec4 fragColor;

void main()
{
   vec3 viewDir = normalize(cameraPos - fragPos);

   vec3 color = vec3(0.0);
   for(int i = 0; i < numPointLightsInScene; i++)
   {
      // Attenuation
      float distance    = length(pointLights[i].worldPos - fragPos);
      float attenuation = 1.0 / (constantAtt + (linearAtt * distance) + (quadraticAtt * distance * distance));

      // Diffuse
      vec3  lightDir    = normalize(pointLights[i].worldPos - fragPos);
      vec3  diff        = max(dot(lightDir, norm), 0.0) * pointLights[i].color * attenuation;
      vec3  diffuse     = (diff * vec3(texture(diffuseTex, uv)));

      color += diffuse;
   }

   fragColor = vec4(color, 1.0);
}
