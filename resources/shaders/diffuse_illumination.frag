in vec3 fragPos;
in vec3 norm;
in vec2 uv;

uniform sampler2D diffuseTex;

out vec4 fragColor;

void main()
{
   fragColor = texture(diffuseTex, uv);
}
