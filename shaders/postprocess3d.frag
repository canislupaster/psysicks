#version 400 core

uniform sampler2D tex;
uniform sampler2D depth;

uniform sampler2D ao;
uniform sampler2D shadow;
uniform sampler2D shadow_depth;
uniform sampler2D ssr;
uniform sampler2D rough_metal;
uniform sampler2D local_refl;

uniform float size;

in vec2 fragtexpos;

out vec4 outColor;

#define BLUR_SIZE 3
#define SSR_BLUR_SIZE 3

void main() {
	float fragdepth = texture(depth, fragtexpos).x;

  float ao_blurred = 0;

	for (int x=-BLUR_SIZE; x<BLUR_SIZE; x++) {
		for (int y=-BLUR_SIZE; y<BLUR_SIZE; y++) {
			ao_blurred += texture(ao, fragtexpos + vec2(size*x, size*y)).r;
		}
	}

	ao_blurred /= 4*BLUR_SIZE*BLUR_SIZE;

  float shadow_blurred = 0;

	int count=0;
	for (int x=-BLUR_SIZE; x<BLUR_SIZE; x++) {
		for (int y=-BLUR_SIZE; y<BLUR_SIZE; y++) {
			float shadow = texture(shadow, fragtexpos + vec2(size*x, size*y)).r;
			float shadow_depth = texture(shadow_depth, fragtexpos + vec2(size*x, size*y)).x;

			if (shadow_depth > fragdepth) continue;

			shadow_blurred += shadow;
			count++;
		}
	}

	shadow_blurred /= max(count, 1);

	vec3 is_rough_metal = vec3(texture(rough_metal, fragtexpos));

  vec4 ssr_blurred = vec4(0);

	if (is_rough_metal.r < 0.5) {
		int ssr_blur_size = int((is_rough_metal.r+1)*SSR_BLUR_SIZE);
		
		for (int x=-ssr_blur_size; x<ssr_blur_size; x++) {
			for (int y=-ssr_blur_size; y<ssr_blur_size; y++) {
				vec4 ssr_res = texture(ssr, fragtexpos + vec2(size*x, size*y));
				if (ssr_res.b == 0) continue;

				ssr_blurred += texture(tex, vec2(ssr_res))*ssr_res.b;
			}
		}
		
		ssr_blurred /= 4*ssr_blur_size*ssr_blur_size;
	}

	vec4 ssr = texture(ssr, fragtexpos);

	//outColor = vec4(is_rough_metal, 1.0);
	//return;

  outColor = texture(tex, fragtexpos);//*ao_blurred + ssr_blurred*is_rough_metal.b;
	outColor *= 1.0-shadow_blurred;
	//outColor = texture(tex, fragtexpos) + vec4(shadow_blurred);
	//outColor = vec4(texture(ao, fragtexpos).r);
	//outColor = vec4(ssr);
}
