#version 400 core

#define DEPTH_MAX 0.995 //threshold to prevent depth precision loss

uniform sampler2D rough_metal;
uniform sampler2D depth;
uniform sampler2D normal;

uniform float size;
uniform float view_far;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
	vec3 fragnormal = vec3(texture(normal, fragtexpos));
	float fragdepth = texture(depth, fragtexpos).x;

	if (fragdepth >= DEPTH_MAX || texture(rough_metal, fragtexpos).b < 0.5) {
		outColor = vec4(0);
		return;
	}

	//outColor = vec4(fragdepth-0.5); return;

	//fragnormal is already accounted for screen space
	vec3 fragnormal_refl = fragnormal;

	//outColor = vec4(fragnormal_refl, 1.0);
	//return;
	//fragnormal_refl.z += fragdepth;

	//fragnormal_refl = vec3(0,1,1);

	//outColor = vec4(fragnormal_refl/6, 1.0);
	//return;
	//fragnormal_refl.z /= view_far;
	//return;

	//if (abs(fragnormal_refl.z) > 0.7) discard;
	vec3 step = fragnormal_refl/vec3(1-fragnormal_refl.z,1-fragnormal_refl.z,1);
	vec3 step_abs = abs(step);

	if (step_abs.y >= step.x && step_abs.x > 0) step /= step_abs.y;
	else if (step_abs.x > 0) step /= step_abs.x;
	//else if (step_abs.y == 0) {
	//	outColor = vec4(0);
	//	return;
	//}

	//outColor = vec4(step.y);
	//return;

	step /= 3*step.z;

	step *= vec3(size, size, size/view_far);
	vec2 step2 = vec2(step);

	vec2 max_dist = (vec2(1.0) - fragtexpos)/vec2(step);
	float max_steps = max(max_dist.x, max_dist.y);
	vec3 ray_pos = vec3(fragtexpos, fragdepth);

	//outColor = vec4(step/size, 1.0);
	//return;

	float margin = size/view_far * (1.0-fragnormal_refl.z);
	float bound_max = 1-margin;

	for (int i=0; i<max_steps; i++) {
		ray_pos += step;

		float sample_depth = texture(depth, vec2(ray_pos)).x;

		//outColor = vec4(abs(sample_depth - fragdepth))/(10*step.z);
		//return;

		vec2 bounds = abs(vec2(ray_pos));
		if (sample_depth >= DEPTH_MAX || bounds.x > bound_max || bounds.y > bound_max) {
			outColor = vec4(0);
			return;
		} else if (abs(sample_depth-ray_pos.z) < margin) {
			float falloff = 1.0 - max(bounds.x, bounds.y)/bound_max;

			outColor = vec4(vec2(ray_pos), falloff, 0);//vec4(fragnormal_refl, 1.0);
			//outColor = vec4((vec2(ray_pos)-fragtexpos), 0, 1.0);
			return;
		}
		//outColor = vec4(ray_pos, 1.0);
	}

	outColor = vec4(0);
}
