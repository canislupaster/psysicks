#version 400 core

uniform object {
  mat4 space;
  mat4 cam;
};

uniform mat4 transform;

layout(location=0) in vec3 position;

out vec3 fragpos;

void main() {
	vec4 worldpos = transform*vec4(position, 1.0);
  gl_Position = space*cam*worldpos;
  fragpos = vec3(worldpos/worldpos.w);
}
