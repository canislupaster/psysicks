#version 400 core

layout(location=0) out vec4 outColor;

in float fade;

void main() {
	outColor = vec4(fade, 0.0, 0.0, 1.0);
}
