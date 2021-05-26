in vec3 toCameraVector;
in vec3 fromLightVector;
in vec2 uv;
in vec4 clipSpace;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D dudvMap;
uniform float moveFactor;
uniform sampler2D normalMap;
uniform vec3 lightColor;
// The refraction depth texture
uniform sampler2D depthMap;

out vec4 fragColor;

const float waveStrength = 0.04;
const float shineDamper = 20.0;
const float reflectivity = 0.5;

void main()
{
   // To do projective texturing, we need to sample our textures using
   // the screen space coordinates (NDC)of the points that make up the water quad
   // However, remember that the origin of the NDC is at the center of the screen,
   // while the origin of texture coordinates is at the bottom left corner
   // To convert between the two, we divide by 2 and add 0.5
   vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
   vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);
   vec2 refractTexCoords = vec2(ndc.x, ndc.y);

   // Sample the refraction depth texture using the refraction texture coordinates
   // The depth is stored in the red value
   float depth = texture(depthMap, refractTexCoords).r;
   float near = 0.1;
   float far = 500.0;
   // Screen coords -> NDC -> World
   float floorDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));

   depth = gl_FragCoord.z;
   float waterDistance = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
   float waterDepth = floorDistance - waterDistance;

   // Distortion is stored in the red and green values
   // Values range from 0.0 to 1.0
   // To make them range from -1.0 to 1.0, we multiply by 2 and subtract 1
   vec2 distortedTexCoords = texture(dudvMap, vec2(uv.x + moveFactor, uv.y)).rg * 0.1;
   distortedTexCoords = uv + vec2(distortedTexCoords.x, distortedTexCoords.y + moveFactor);
   vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength * clamp(waterDepth / 20.0, 0.0, 1.0);

   reflectTexCoords += totalDistortion;
   reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
   reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);

   refractTexCoords += totalDistortion;
   refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);

   vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
   vec4 refractColor = texture(refractionTexture, refractTexCoords);

   // Normal
   vec4 normalMapColor = texture(normalMap, distortedTexCoords);
   vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b * 3.0, normalMapColor.g * 2.0 - 1.0);
   normal = normalize(normal);

   // Fresnel effect
   vec3 viewVector = normalize(toCameraVector);
   float refractiveFactor = dot(viewVector, normal);
   refractiveFactor = pow(refractiveFactor, 0.5);
   refractiveFactor = clamp(refractiveFactor, 0.0, 1.0);

   // Specularity
   vec3 reflectedLight = reflect(normalize(fromLightVector), normal);
   float specular = max(dot(reflectedLight, viewVector), 0.0);
   specular = pow(specular, shineDamper);
   vec3 specularHighlights = lightColor * specular * reflectivity * clamp(waterDepth / 5.0, 0.0, 1.0);

   fragColor = mix(reflectColor, refractColor, refractiveFactor);
   fragColor = mix(fragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + vec4(specularHighlights, 0.0);
   fragColor.a = clamp(waterDepth / 5.0, 0.0, 1.0);

   fragColor = vec4(vec3(waterDepth / 50.0), 1.0);
   //fragColor = vec4(vec3(waterDistance / 50.0), 1.0);
}
