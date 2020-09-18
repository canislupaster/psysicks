#version 400 core

uniform sampler2D tex;
uniform vec4 color;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
  outColor = color * vec4(1.0, 1.0, 1.0, texture(tex, fragtexpos).r);
}