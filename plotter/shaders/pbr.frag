#version 400 core

#define MAX_LIGHTS 10
#define ENV_MIPMAPS_OFFSET 7

// math.glsl
#define PI 3.1415926538

struct pointlight {
  vec4 color;
  vec3 pos;
  float dist;
};

struct dirlight {
  vec4 color;
  vec3 dir;
};

layout(std140) uniform lighting {
  int pointlights_enabled; //#enabled from 0
  pointlight pointlights[MAX_LIGHTS];

  int dirlights_enabled; //#enabled from 0
  dirlight dirlights[MAX_LIGHTS];

	bvec2 env_enabled;

	vec4 local_envpos;
	float local_envdist;

  vec4 ambient;
};

uniform samplerCube envtex[MAX_LIGHTS];

uniform object {
  uniform mat4 space;
  uniform mat4 cam;
};

uniform vec4 color;
uniform vec3 emissive;

uniform float metal;
uniform float rough;
uniform float normal;
uniform float occlusion;

uniform sampler2D diffusetex;
uniform sampler2D normaltex;
uniform sampler2D emissivetex;
uniform sampler2D ormtex;

uniform samplerCube global_env;
uniform samplerCube local_env;

in vec3 fragpos;
in vec3 fragnormal;
in vec2 fragtexpos;

in vec3 fragbitangent;
in vec3 fragtangent;

layout(location=0) out vec4 outColor;
layout(location=1) out vec4 outNormal;
layout(location=2) out vec4 outRoughMetal;

const float dielectric_baserefl = 0.04;

float do_fresnel(float is_metal, float view_offset) {
  float baserefl = mix(dielectric_baserefl, 0.5, is_metal);
  float fresnel = baserefl + (1-baserefl)*pow(1-view_offset, 5);

  return fresnel;
}

vec4 do_light(vec3 light_angle, vec4 light_color, vec3 view_angle, float fresnel, float view_normal_angle, vec4 color, float is_metal, float is_rough, vec3 displaced_normal) {
  vec3 half_angle = normalize(light_angle + view_angle); //microsurface normal
  
  float normal_angle = abs(dot(half_angle, displaced_normal));

  float light_normal_angle = dot(light_angle, displaced_normal);

  float view_half_angle = dot(view_angle, half_angle);
  float normal_half_angle = dot(half_angle, displaced_normal);

  if (light_normal_angle<0) return vec4(0);

  float roughsq = pow(is_rough, 2);

  //offset exponents from tangent/"height" (2, 0.5) -> (1, -1)
  //lmao
  float view_shadow = view_normal_angle/(view_normal_angle*(1-roughsq) + roughsq);
  float light_shadow = light_normal_angle/(light_normal_angle*(1-roughsq) + roughsq);

  //equivalent to dividing roughness/spread by ellipse where x is scaled by roughness squared
  float ggx = roughsq/(1 - pow(normal_half_angle, 2)*(1-roughsq));

	float spec = fresnel*ggx;

  return color*view_shadow*light_shadow*normal_angle*light_color.a
    * ((1.0-is_metal) + is_metal*light_color*spec);
}

vec4 do_env(samplerCube envtex, float dist, float radius, vec3 displaced_normal, vec3 view_angle, float fresnel, float view_normal_angle, vec4 color, float is_metal, float is_rough) {
	float edge_dist = radius-dist;
 	float size = edge_dist/(PI*radius); //approximate projected hemisphere out of total radius
	
	vec3 coord = 2*displaced_normal - view_angle;

 	float mipmap = max(log2(size * is_rough)+ENV_MIPMAPS_OFFSET, textureQueryLod(envtex, coord).y);

 	vec4 light_color = textureLod(envtex, coord, mipmap);

  float roughsq = pow(is_rough, 2);
  float view_shadow = view_normal_angle/(view_normal_angle*(1-roughsq) + roughsq);

 	return color*view_shadow*view_normal_angle*light_color*(1.0-is_metal + is_metal*fresnel);
	//return vec4(view_shadow)*spec*light_color*color;
  //return vec4(view_angle, 1.0)*0.5;
	//return vec4(spec);
}

void main() {
  vec4 textured_color = color*texture(diffusetex, fragtexpos);
  vec3 emission = emissive*vec3(texture(emissivetex, fragtexpos));
  float is_metal = metal*texture(ormtex, fragtexpos).b;
  float is_rough = rough*texture(ormtex, fragtexpos).g;
  float occluded = occlusion*texture(ormtex, fragtexpos).r;

  vec3 normal_displace = normal*vec3(texture(normaltex, fragtexpos));
  vec3 displaced_normal = fragnormal + fragnormal*normal_displace.y + fragtangent*normal_displace.x + fragbitangent*normal_displace.z;

	vec4 view_to = cam*vec4(fragpos, 1.0);
  vec3 view_angle = -normalize(vec3(view_to/view_to.w));

  float view_normal_angle = (1+dot(view_angle, displaced_normal))/2;
	if (view_normal_angle<0) return;
	
	float fresnel = do_fresnel(is_metal, view_normal_angle);

  vec4 lit_color = vec4(0);

	for (int i=0; i<pointlights_enabled; i++) {
		pointlight light = pointlights[i];

		vec3 dist = light.pos - fragpos;
		vec3 light_angle = normalize(dist);
		float falloff = pow(light.dist - min(length(dist), light.dist), 2) * 1/(light.dist*PI);
		lit_color += do_light(light_angle, light.color, view_angle, fresnel, view_normal_angle, textured_color, is_metal, is_rough, displaced_normal)*falloff;
	}
	
	for (int i=0; i<dirlights_enabled; i++) {
    dirlight light = dirlights[i];

    lit_color += do_light(normalize(light.dir), light.color, view_angle, fresnel, view_normal_angle, textured_color, is_metal, is_rough, displaced_normal);
  }

	if (env_enabled[0]) {
		//global env is always unit distance away
		lit_color += do_env(global_env, 0.0, 1.0, displaced_normal, view_angle, fresnel, view_normal_angle, textured_color, is_metal, is_rough);
	}

	if (env_enabled[1]) {
		vec3 normalized_pos = fragpos - vec3(local_envpos);
		float dist = length(normalized_pos); //0-local_envdist;
		if (dist<local_envdist) {
			lit_color += do_env(local_env, dist, local_envdist, displaced_normal, view_angle, fresnel, view_normal_angle, textured_color, is_metal, is_rough);
		}
	}

  lit_color += textured_color*ambient*ambient.a*occluded;
  
  vec3 outcolor_noalpha = emission + vec3(lit_color);
  
  //tonemapping
  outColor = vec4(pow(outcolor_noalpha/(outcolor_noalpha+1), vec3(1/2.2)), color.a);
	//outColor = vec4(max(-fragpos, 0), 1.0);

	outNormal = vec4(vec3(transpose(inverse(cam))*vec4(displaced_normal, 1.0)), 1.0);

	//center for negative values
	outNormal += 1.0;
	outNormal /= 2.0;

	outRoughMetal = vec4(is_rough, is_metal, fresnel, 1.0);
	//outNormal = vec3(0.0);
	//outColor = vec4(view_angle, 1.0);
}
