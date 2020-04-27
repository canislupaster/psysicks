#version 400 core

uniform sampler2D tex;
uniform float size;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
  outColor = texture(tex, fragtexpos);
  outColor += texture(tex, fragtexpos+vec2(size, 0));
  outColor += texture(tex, fragtexpos+vec2(0, size));
  outColor += texture(tex, fragtexpos+vec2(size, size));

	outColor /= 4;
}
