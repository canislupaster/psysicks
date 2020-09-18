#version 400 core

#define MAX_STEPS 300
#define DEPTH_MIN 0.1
#define SLOPE_MARGIN 0.8

uniform sampler2D rough_metal;
uniform sampler2D depth;
uniform sampler2D normal;

uniform float size;
uniform mat4 space;

in vec2 fragtexpos;

out vec4 outColor;

void main() {
	vec3 fragnormal = vec3(texture(normal, fragtexpos));
	fragnormal *= 2.0;
	fragnormal -= 1.0;

	float slope = length(vec2(fragnormal));

	//boy i hope this is optimized well and good
	float proj_a = space[2][2];
	float proj_b = space[3][2];

	float fragdepth_normalized = -texture(depth, fragtexpos).x;
	float fragdepth = -proj_b / (fragdepth_normalized - proj_a);

	if (texture(rough_metal, fragtexpos).r > 0.5 || fragdepth_normalized == -1.0 || fragdepth <= DEPTH_MIN
			|| fragnormal.z/fragdepth > slope-SLOPE_MARGIN) {
		discard;
	}

	vec3 fragnormal_refl = fragnormal;

	vec2 refl_offset = mat2(space)*(2*(fragtexpos-0.5)*(fragnormal_refl.z/fragdepth));
	refl_offset *= 2;
	//outColor = vec4(refl_offset*10, 0, 1);
	//return;
	
	fragnormal_refl.z = 1.0-fragnormal_refl.z;
	fragnormal_refl = normalize(fragnormal_refl+vec3(refl_offset,0));

	//if (1.0-fragnormal_refl.z >= slope) discard;
	//outColor = vec4(slope);
	//return;
	//if (fragnormal_refl.z < SLOPE_MARGIN || fragnormal_refl.z >= slope) discard;

	vec3 step = fragnormal_refl;//vec3(1-fragnormal_refl.z,1-fragnormal_refl.z,1);
	vec3 step_abs = abs(step);

	if (step_abs.y >= step_abs.x && step_abs.x > 0) step /= step_abs.y;
	else if (step_abs.x > 0) step /= step_abs.x;
	//else if (step_abs.y == 0) {
	//	outColor = vec4(0);
	//	return;
	//}

	//step /= step.z;

	step *= vec3(size, size, size);

	vec2 max_dist = (vec2(1.0) - fragtexpos)/vec2(step);
	float steps = max(max_dist.x, max_dist.y);

	vec3 ray_pos = vec3(fragtexpos, fragdepth);

	float margin = 2*step.z;
	float bound_max = 0.5-margin;

	for (int i=0; i<min(steps, MAX_STEPS); i++) {
		ray_pos += step;
		//this should work idfk if it does
		step.x *= 1-step.z;
		step.y *= 1-step.z;
		
		vec2 sample_pos = vec2(ray_pos);

		float sample_depth_normalized = -texture(depth, sample_pos).x;
		float sample_depth = -proj_b / (sample_depth_normalized - proj_a);

		vec2 bounds = abs(sample_pos-0.5);
		if (sample_depth_normalized == -1.0 || bounds.x > bound_max || bounds.y > bound_max) {
			outColor = vec4(0);
			return;
		} else if (abs(sample_depth-ray_pos.z) < margin) {
			float falloff = 4 - 8*max(bounds.x, bounds.y)/bound_max;

			outColor = vec4(sample_pos, falloff, 1);//vec4(fragnormal_refl, 1.0);
			//outColor = vec4(falloff);
			//outColor = vec4((vec2(ray_pos)-fragtexpos), 0, 1.0);
			return;
		}
		//outColor = vec4(ray_pos, 1.0);
	}

	outColor = vec4(0);
}
