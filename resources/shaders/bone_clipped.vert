layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inCol;

uniform mat4 model;
uniform mat4 projectionView;
// x contains the normal.y value (1 or -1) of the clipping plane and y contains the height of the clipping plane
uniform vec2 horizontalClippingPlaneYNormalAndHeight;

out float clipDistance;
out vec3  col;

void main()
{
   vec3 fragPos = vec3(model * vec4(inPos, 1.0f));

   // gl_ClipDistance[0] isn't supported in OpenGL ES 2.0
   //gl_ClipDistance[0] = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;
   clipDistance = (fragPos.y - horizontalClippingPlaneYNormalAndHeight.y) * horizontalClippingPlaneYNormalAndHeight.x;

   col = inCol;
   gl_Position = projectionView * model * vec4(inPos, 1.0f);
}
