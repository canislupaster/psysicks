#version 400 core

uniform sampler2D tex;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
  outColor = texture(tex, fragtexpos);
}