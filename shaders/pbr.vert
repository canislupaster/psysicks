#version 400 core

uniform object {
  uniform mat4 space;
  uniform mat4 cam;
};

uniform mat4 transform;

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texpos;
layout(location=3) in vec3 tangent;

out vec3 fragpos;
out vec3 fragnormal;
out vec2 fragtexpos;

out vec3 fragtangent;
out vec3 fragbitangent;

void main() {
  vec4 world = transform*vec4(position, 1.0);
  
  gl_Position = space*cam*world;
  fragpos = vec3(world/world.w);

  vec3 transform_normal = normalize(mat3(transpose(inverse(transform)))*normal);
  fragnormal = transform_normal; //perpendicularize slope and discard translation
  fragtexpos = vec2(texpos.x, 1.0-texpos.y);

  fragtangent = tangent;
  fragbitangent = cross(transform_normal, tangent);
}
