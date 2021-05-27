in vec3 inPos;
in vec3 inNormal;

uniform mat4 modelMatrices[120];
uniform mat4 projectionView;
// x contains the normal.y value (1 or -1) of the clipping plane and y contains the height of the clipping plane
uniform vec2 horizontalClippingPlaneYNormalAndHeight;

out vec3  fragPos;
out float clipDistance;
out vec3  norm;

void main()
{
   fragPos = vec3(modelMatrices[gl_InstanceID] * vec4(inPos, 1.0f));

   // gl_ClipDistance[0] isn't supported in OpenGL ES 2.0
   //gl_ClipDistance[0] = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;
   clipDistance = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;

   norm    = vec3(normalize(modelMatrices[gl_InstanceID] * vec4(inNormal, 0.0f)));

   gl_Position = projectionView * vec4(fragPos, 1.0);
}
