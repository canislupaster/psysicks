#version 400 core

uniform sampler2D tex;
uniform sampler2D depth;
uniform sampler2D normal;

uniform float size;
uniform float view_far;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
	vec3 fragnormal = vec3(texture(normal, fragtexpos));
	float fragdepth = texture(depth, fragtexpos).x;

	if (fragdepth == 1.0) discard;

	//outColor = vec4(fragdepth-0.5); return;

	float view_normal_angle = fragdepth*fragnormal.z;
	vec3 fragnormal_refl = 2*view_normal_angle*fragnormal;
	fragnormal_refl.z = fragdepth;

	fragnormal_refl = normalize(fragnormal_refl);

	//outColor = vec4(fragnormal_refl, 1.0);
	//return;
	//fragnormal_refl.z += fragdepth;

	//fragnormal_refl = vec3(0,1,1);

	//outColor = vec4(fragnormal_refl/6, 1.0);
	//return;
	//fragnormal_refl.z /= view_far;
	//return;

	vec3 step = fragnormal_refl/vec3(1-fragnormal_refl.z,1-fragnormal_refl.z,1);
	vec3 step_abs = abs(step);

	//if (step_abs.y >= step.x && step_abs.x > 0) step /= step_abs.y;
	//else if (step_abs.x > 0) step /= step_abs.x;
	//else if (step_abs.y == 0) discard;
	
	//outColor = vec4(step.y);
	//return;

	step *= vec3(size, size, size/view_far);

	vec2 max_dist = (vec2(1.0) - fragtexpos)/vec2(step);
	float max_steps = max(max_dist.x, max_dist.y);
	vec3 ray_pos = vec3(fragtexpos, fragdepth);

	//outColor = vec4(step/size, 1.0);
	//return;

	for (int i=0; i<1000; i++) {
		ray_pos += step;

		float sample_depth = texture(depth, vec2(ray_pos)).x;

		//outColor = vec4(abs(sample_depth - fragdepth))/(10*step.z);
		//return;

		if (sample_depth == 1.0) {
			break;
		} else if (abs(sample_depth-ray_pos.z) < step.z) {
			outColor = texture(tex, vec2(ray_pos));
			//outColor = vec4((vec2(ray_pos)-fragtexpos), 0, 1.0);
			return;
		}
		//outColor = vec4(ray_pos, 1.0);
	}

	outColor = vec4(texture(tex, vec2(fragtexpos)).r, 0, 0, 1.0);
}
