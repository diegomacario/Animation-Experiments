in vec3  fragPos;
in float clipDistance;
in vec3  norm;

struct PointLight
{
   vec3  worldPos;
   vec3  color;
   float constantAtt;
   float linearAtt;
   float quadraticAtt;
};

#define MAX_NUMBER_OF_POINT_LIGHTS 4
uniform PointLight pointLights[MAX_NUMBER_OF_POINT_LIGHTS];
uniform int numPointLightsInScene;

uniform vec3 cameraPos;

out vec4 fragColor;

vec3 calculateContributionOfPointLight(PointLight light, vec3 viewDir);

void main()
{
   if (clipDistance < 0.0)
   {
      discard;
   }

   vec3 viewDir = normalize(cameraPos - fragPos);

   vec3 color = vec3(0.0);
   for(int i = 0; i < numPointLightsInScene; i++)
   {
      color += calculateContributionOfPointLight(pointLights[i], viewDir);
   }

   fragColor = vec4(color, 1.0);
}

vec3 calculateContributionOfPointLight(PointLight light, vec3 viewDir)
{
   // Attenuation
   float distance    = length(light.worldPos - fragPos);
   float attenuation = 1.0 / (light.constantAtt + (light.linearAtt * distance) + (light.quadraticAtt * distance * distance));

   // Diffuse
   vec3  lightDir    = normalize(light.worldPos - fragPos);
   vec3  diff        = max(dot(lightDir, norm), 0.0) * light.color * attenuation;
   vec3  diffuse     = (diff * vec3(0.0f, 1.0f, 0.0f));

   return diffuse + vec3(0.0f, 0.25f, 0.0f);
}
