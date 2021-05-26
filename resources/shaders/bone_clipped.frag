in float clipDistance;
in vec3  col;

out vec4 fragColor;

void main()
{
   if (clipDistance < 0.0)
   {
      discard;
   }

   fragColor = vec4(col, 1.0f);
}
