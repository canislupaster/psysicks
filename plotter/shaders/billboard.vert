#version 400 core

uniform object {
  mat4 space;
  mat4 cam;
};

uniform mat4 transform;

layout(location=0) in vec3 position;
layout(location=2) in vec2 texpos;

out vec2 fragtexpos;

void main() {
  vec4 pos_off = space*cam*transform*vec4(0.0);
	pos_off /= pos_off.w;
	
	gl_Position = position + pos_off;

  fragtexpos = texpos;
}
