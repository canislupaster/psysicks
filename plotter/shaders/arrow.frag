#version 400 core

#define ARROW_HEAD_HEIGHT 0.4
#define ARROW_THICKNESS 0.2
//len -> full height
#define ARROW_HEIGHT_RANGE 4
//len -> red
#define ARROW_RED_RANGE 10

in vec2 fragtexpos;

layout(location=0) out vec4 outColor;

in vec3 view_vec;
in float view_vec_len;

void main() {
    //shape arrow
    if (view_vec_len==0) discard;

    float h = min(view_vec_len/ARROW_HEIGHT_RANGE,1-ARROW_HEAD_HEIGHT);

    float x = fragtexpos.x-0.5;
    if (fragtexpos.y > h) {
        float y = ARROW_HEAD_HEIGHT-(fragtexpos.y-h);
        if (x > y || x < -y) discard;
    } else if (x > ARROW_THICKNESS/2 || x < -ARROW_THICKNESS/2) {
        discard;
    }

    float red = view_vec_len / ARROW_RED_RANGE;
    outColor = vec4(red, 1-red, 0.5-red*0.5, 1.0);
}
