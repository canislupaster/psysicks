#version 400 core

#define RANDOM_DIVISIONS 1000
#define VIEW_FAR 100
#define ANGLE_BIAS 0.0

uniform sampler2D tex;
uniform sampler2D normal;

uniform float radius;
uniform int samples;

uniform float seed;

in vec2 fragtexpos;

out vec4 outColor;

//from https://briansharpe.wordpress.com/2011/11/15/a-fast-and-simple-32bit-floating-point-hash-function/
vec2 rand(vec2 seed) {
	const vec2 OFFSET = vec2(54.0, 84.0);
	const float DOMAIN = 93.457;
	const float SOMELARGEFLOAT = 135.135664;

	vec4 P = vec4(seed.xy, seed.xy + vec2(0.5));
	
	P = mod(P, DOMAIN);
	P += OFFSET.xyxy;
	P *= P;

	vec2 res = P.xz * P.yw * (1.0/SOMELARGEFLOAT);
	return fract(res);
}

vec2 smoothrand(vec2 seed) {
	vec2 i;
	vec2 frac = modf(seed*RANDOM_DIVISIONS, i);

	return mix(mix(rand(i), rand(i+vec2(1,0)), vec2(frac.x)), mix(rand(i+vec2(0,1)), rand(i+vec2(1,1)), vec2(frac.x)), vec2(frac.y));
}

void main() {
	vec3 fragnormal = vec3(texture(normal, fragtexpos));
	float slope = length(vec2(fragnormal))*(radius/VIEW_FAR);

	//center hemisphere
	vec2 size = 1.0 - abs(vec2(fragnormal))/2; //approximate height of hemisphere (z=1, y=0.5)
	size *= radius;

	vec2 size_sgn = sign(vec2(fragnormal))*size;

	vec2 pos = fragtexpos;

	if (size_sgn.x == 0) size_sgn.x = size.x*radius;
	else if (size_sgn.x < 0) pos += size_sgn.x;

	if (size_sgn.y == 0) size_sgn.y = size.y*radius;
	else if (size_sgn.y < 0) pos += size_sgn.y;
	
	float center_tex = texture(tex, fragtexpos).x;
	float average_occlusion = 0;
	float max_length = length(size);
	
	float seed_i;
	float seed_blend = modf(seed, seed_i);

	for (int i=0; i<samples; i++) {
		vec2 posrand = rand(fragtexpos*i);
		vec2 sample_pos = mix(rand(posrand + seed_i), rand(posrand + seed_i + 1), seed_blend);
		sample_pos *= size;
  
		float sample_depth = texture(tex, sample_pos+pos).x;
		float diff = abs(center_tex-sample_depth);
		//if (sample_depth < center_tex) continue;

		float diff_unnormalized = diff*VIEW_FAR;
		if (diff_unnormalized < ANGLE_BIAS+slope || diff_unnormalized > 1.0) continue;

		float falloff = 1.0 - length(sample_pos)/max_length;
		falloff *= 1.0 - diff_unnormalized;
		falloff *= falloff;

		average_occlusion += max(diff_unnormalized*5*falloff, 0);
	}

	average_occlusion /= samples;
	average_occlusion *= max_length/radius;

	outColor = vec4(1.0-average_occlusion, 0.0, 0.0, 1.0);
	//outColor = vec4(1.0);
	//outColor = vec4(length(smoothrand(fragtexpos+3)));
	//outColor = vec4(size.y, 0.0, 0.0, 1.0);
}
