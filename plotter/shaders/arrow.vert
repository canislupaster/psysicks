#version 400 core

uniform object {
    mat4 space;
    mat4 cam;
};

uniform mat4 transform;
uniform vec3 vec;

layout(location=0) in vec3 position;
layout(location=2) in vec2 texpos;

out vec2 fragtexpos;
out vec3 view_vec;
out float view_vec_len;

void main() {
    view_vec = vec3(cam*vec4(vec,1));
    view_vec_len = abs(length(vec2(view_vec)));
    view_vec = normalize(view_vec);

    mat2 view_vecspace = mat2(view_vec.x, -view_vec.y, view_vec.y, view_vec.x); //reverse skew of basis, so Y is distance

    vec4 pos_off = space*cam*transform*vec4(0,0,0,1);
    gl_Position = vec4(view_vecspace*vec2(position),0,0) + pos_off;

    fragtexpos = texpos;
}
