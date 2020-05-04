#version 400 core

uniform sampler2D tex;
uniform sampler2D depth;

uniform sampler2D ao;

uniform float size;

in vec2 fragtexpos;

out vec4 outColor;

#define BLUR_SIZE 3

void main() {
  float ao_blurred = 0;

	for (int x=-BLUR_SIZE; x<BLUR_SIZE; x++) {
		for (int y=-BLUR_SIZE; y<BLUR_SIZE; y++) {
			ao_blurred += texture(ao, fragtexpos + vec2(size*x, size*y)).r;
		}
	}

	ao_blurred /= 4*BLUR_SIZE*BLUR_SIZE;

  outColor = texture(tex, fragtexpos)*ao_blurred;
	//outColor = vec4(ao_blurred);
}
