in vec3 position;
in vec2 texCoord;

uniform mat4 model;
uniform mat4 projectionView;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;

out vec3 toCameraVector;
out vec3 fromLightVector;
out vec2 uv;
out vec4 clipSpace;

void main()
{
   vec4 worldPosition = model * vec4(position, 1.0f);
   toCameraVector = cameraPosition - worldPosition.xyz;
   fromLightVector = worldPosition.xyz - lightPosition;

   uv = texCoord * 4.0;

   clipSpace = projectionView * worldPosition;
   gl_Position = clipSpace;
}
