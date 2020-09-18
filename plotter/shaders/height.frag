#version 400 core

layout(location=0) out vec4 outColor;

in vec3 fragpos;

uniform float heightmap_size;

uniform bool compare;
uniform sampler2D compare_from;

void main() {
	if (compare && texture(compare_from, vec2(fragpos*heightmap_size)).x <= fragpos.y) {
		discard;
	}

  outColor = vec4(vec3(fragpos.y), 1.0);
}
