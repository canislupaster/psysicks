#version 400 core

uniform samplerCube tex;

out vec4 outColor;

in vec3 fragpos;

void main() {
  vec4 color = textureLod(tex, fragpos, 0);
  vec3 color_noalpha = vec3(color);

  outColor = vec4(pow(color_noalpha/(color_noalpha+1), vec3(1/2.2)), color.a);
}