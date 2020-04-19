#version 400 core

#define MAX_LIGHTS 10

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

uniform lighting {
  int pointlights_enabled; //#enabled from 0
  pointlight pointlights[MAX_LIGHTS];

  int dirlights_enabled; //#enabled from 0
  dirlight dirlights[MAX_LIGHTS];

  vec4 ambient;
};

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

in vec3 fragpos;
in vec3 fragnormal;
in vec2 fragtexpos;

in vec3 fragbitangent;
in vec3 fragtangent;

out vec4 outColor;

const float dielectric_baserefl = 0.04;

vec4 do_light(vec3 light_angle, vec4 light_color, vec3 view_angle, vec4 color, float is_metal, float is_rough, vec3 displaced_normal) {
  vec3 half_angle = normalize(light_angle + view_angle); //microsurface normal
  
  float normal_angle = abs(dot(half_angle, displaced_normal));

  float view_normal_angle = dot(view_angle, displaced_normal);
  float light_normal_angle = dot(light_angle, displaced_normal);

  float view_half_angle = dot(view_angle, half_angle);
  float normal_half_angle = dot(half_angle, displaced_normal);

  if (light_normal_angle<0 || view_normal_angle<0) return vec4(0);

  float baserefl = mix(dielectric_baserefl, 1.0, is_metal);
  float fresnel = baserefl + (1-baserefl)*pow(1-(view_half_angle), 5);
  
  //equivalent to dividing roughness/spread by ellipse where x is scaled by roughness squared
  float roughsq = pow(is_rough, 2);
  float ggx = roughsq/(1 - pow(normal_half_angle, 2)*(1-roughsq));

  //offset exponents from tangent/"height" (2, 0.5) -> (1, -1)
  //lmao
  float view_shadow = view_normal_angle/(view_normal_angle*(1-roughsq) + roughsq);
  float light_shadow = light_normal_angle/(light_normal_angle*(1-roughsq) + roughsq);

  float spec = ggx*fresnel;

  return color*view_shadow*light_shadow*normal_angle
    * ((1.0-is_metal)*(1.0-spec) + light_color*light_color.a*spec);
}

void main() {
  vec4 textured_color = color*texture(diffusetex, fragtexpos);
  vec3 emission = emissive*vec3(texture(emissivetex, fragtexpos));
  float is_metal = metal*texture(ormtex, fragtexpos).b;
  float is_rough = rough*texture(ormtex, fragtexpos).g;
  float occluded = occlusion*texture(ormtex, fragtexpos).r;

  vec3 normal_displace = normal*vec3(texture(normaltex, fragtexpos));
  vec3 displaced_normal = fragnormal + fragnormal*normal_displace.y + fragtangent*normal_displace.x + fragbitangent*normal_displace.z;

  vec3 view_angle = -normalize(vec3(transpose(cam)*vec4(fragpos, 1.0)));

  vec4 lit_color = vec4(0);

  // for (int i=0; i<pointlights_enabled; i++) {
  //   pointlight light = pointlights[i];

  //   vec3 dist = light.pos - fragpos;
  //   vec3 light_angle = normalize(dist);
  //   float falloff = pow(light.dist - min(length(dist), light.dist), 2) * 1/(light.dist*PI);
    
  //   lit_color += do_light(light_angle, light.color, view_angle, textured_color, is_metal, is_rough, displaced_normal)*falloff;
  // }

  for (int i=0; i<dirlights_enabled; i++) {
    dirlight light = dirlights[i];
    lit_color += do_light(normalize(light.dir), light.color, view_angle, textured_color, is_metal, is_rough, displaced_normal);
  }

  lit_color += textured_color*ambient*ambient.a*occluded;
  
  vec3 outcolor_noalpha = emission + vec3(lit_color);
  
  //tonemapping
  outColor = vec4(pow(outcolor_noalpha/(outcolor_noalpha+1), vec3(1/2.2)), color.a);
}