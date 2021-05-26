in vec3  position;
in vec3  normal;
in vec2  texCoord;
in vec4  weights;
in ivec4 joints;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// x contains the normal.y value (1 or -1) of the clipping plane and y contains the height of the clipping plane
uniform vec2 horizontalClippingPlaneYNormalAndHeight;

uniform mat4 animated[120];

out vec3  norm;
out vec3  fragPos;
out float clipDistance;
out vec2  uv;

void main()
{

   mat4 skin = (animated[joints.x] * weights.x) +
               (animated[joints.y] * weights.y) +
               (animated[joints.z] * weights.z) +
               (animated[joints.w] * weights.w);

   fragPos = vec3(model * skin * vec4(position, 1.0f));

   // gl_ClipDistance[0] isn't supported in OpenGL ES 2.0
   //gl_ClipDistance[0] = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;
   clipDistance = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;

   gl_Position = projection * view * model * skin * vec4(position, 1.0f);

   norm    = vec3(model * skin * vec4(normal, 0.0f));
   uv      = texCoord;
}
