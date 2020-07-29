#version 400 core

uniform object {
  mat4 space;
  mat4 cam;
};

#define FADE_MIN 0.01

uniform mat4 transform;

uniform vec3 light_dirpos;

uniform float softness;
uniform float opacity;

uniform sampler2D heightmap;
uniform float heightmap_size;

layout(location=0) in vec3 position;

out float fade;

void main() {
	vec4 worldpos_w = transform*vec4(position, 1.0);
	worldpos_w /= worldpos_w.w;

	vec3 worldpos = vec3(worldpos_w);

	vec3 light_dir;

	if (softness == 0.0) {
		light_dir = light_dirpos;
		fade = opacity;
	} else {
		light_dir = worldpos - light_dirpos;
	}

	if (opacity*(1.0-softness)*length(light_dir) < FADE_MIN) return;

	float height = 0; //texture(heightmap, vec2(worldpos)*heightmap_size).x;
	if (height >= worldpos.y) return;

	float diff = worldpos.y - height;
	if (sign(light_dir.y) == sign(diff)) return; //both light and ground point away

	vec3 light_dir_normalized = vec3(-1,1,-1)*light_dir/abs(light_dir.y);

	vec3 offset = light_dir_normalized*diff;

	worldpos += offset;
	
	//do one more pass in case there were two height differences
	//disabled for now
	//height = 0; //texture(heightmap, vec2(worldpos)*heightmap_size).x;
	//if (height < worldpos.y) {
	//	offset += light_dir_normalized*(height - worldpos.z);
	//	worldpos += offset;
	//}

	if (softness != 0.0) {
		fade = max(1.0 - softness*length(light_dirpos - worldpos), 0.0);
	}

	worldpos += vec3(0,-0.001*offset.y,0); //offset to appear above ground
  gl_Position = space*cam*vec4(worldpos, 1.0);
}
