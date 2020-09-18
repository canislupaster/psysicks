// Automatically generated header.

#pragma once
#include <ft2build.h>
#include <stddef.h>
#include <stdlib.h>
#include "gl.h"
#include FT_FREETYPE_H
#include <cglm/cglm.h>
#include <math.h>
#include "cgltf.h"
#include "hashtable.h"
#include "stb_image.h"
#include "util.h"
#include "vector.h"
typedef enum { spacescreen, space3d, space3d_ortho, spaceuninit } space_t;
typedef enum {
  shader_fill,
  shader_tex,
  shader_cubemap,
  shader_height,
  shader_txt,
  shader_pbr,
  shader_shadow,
	shader_arrow,
	shader_none
} shaderkind;
typedef struct {
  GLuint tex;
  GLenum format;
  GLenum target;
} tex_t;
typedef union {
  struct {
    vec4 col;
    tex_t* texture;
  };

	struct {
		int compare;
		tex_t* compare_to;
		float heightmap_size;
	} height;

  struct {
		float* light_dirpos;
		float softness;
		float opacity;

		tex_t* height;
		float heightmap_size;
	} shadow;

	float* arrow; //vec3

  // heinous hell
  struct {
    vec4 col;
    float metal;
    float rough;
    float normal;
    float occlusion;
    vec3 emissive;

    struct {
      GLuint diffuse, normal, emissive, orm;
    } tex;
  } pbr;
} objshader_params;
typedef struct {
  vector_t vertices;
  vector_t elements;

  mat4 transform;

  shaderkind shader;
  objshader_params params;

  space_t space;

  GLuint vbo;
  GLuint ebo;
  GLuint vao;
} object_t;
typedef struct {
  GLuint prog;
  GLuint obj;
  GLuint transform;
} obj_shader;
typedef struct {
  GLuint prog;
  GLuint tex;
} tex_shader;
typedef struct {
  vec4 color;
  vec3 dir;
} dirlight;
typedef struct {
  unsigned global_env_enabled;
  unsigned local_env_enabled;

  unsigned pad[2];  // pad to 16 bytes;

  vec4 local_envpos;
  float local_envdist;
} ibl_lighting;
typedef struct {
	struct {
		tex_t space3d_tex;
		tex_t space3d_depth;
		tex_t space3d_normal;
		tex_t space3d_rough_metal;
	} multisample;

  tex_t space3d_tex;
  tex_t space3d_depth;
  tex_t space3d_normal;
  tex_t space3d_rough_metal;

  GLuint space3d_fbo_multisampled;
  GLuint space3d_fbo;

	char space3d_multisample;
} targets_t;
typedef struct {
  struct {
    obj_shader shader;
    GLint color;
  } fill;

  struct {
    obj_shader shader;
    GLint tex;
  } tex3d;

  struct {
    obj_shader shader;
    GLint tex;
  } cubemap;

  struct {
    obj_shader shader;
		GLint compare;
		GLint compare_to;
		GLint heightmap_size;
  } height;

  struct {
    obj_shader shader;
    GLint color;
    GLint tex;
  } txt;

	struct {
		obj_shader shader;
		GLint vec;
	} arrow;

  struct {
    obj_shader shader;
    GLint color;
    GLint metal;
    GLint rough;
    GLint occlusion;
    GLint emissive;

    GLint local_env;
    GLint global_env;

    struct {
      GLint diffuse, normal, emissive, orm;
    } tex;
  } pbr;

  struct {
    obj_shader shader;
    GLint light_dirpos;
		GLint softness;
		GLint opacity;

    GLint heightmap;
		GLint heightmap_size;
  } shadow;

  struct {
    tex_shader shader;
    GLint size;
  } boxfilter;

  struct {
    tex_shader shader;
    GLint radius;
    GLint samples;
    GLint normal;
    GLint seed;
  } ao;

  struct {
    tex_shader shader;
    GLint normal;
    GLint depth;
    GLint size;
    GLint space;
  } ssr;

  struct {
    tex_shader shader;
    GLint size;
    GLint depth;
    GLint ao;
    GLint shadow;
    GLint shadow_depth;
    GLint ssr;
    GLint rough_metal;
  } postproc3d;

  struct {
    tex_shader shader;
  } tex;

  FT_Library freetype;

  vec2 bounds;

  GLuint uniform_buffer;

  // unit 1x1 textures
  tex_t default_tex;
  tex_t default_texred;
  tex_t default_texdep;
  tex_t default_texrgb;

  mat4 spacescreen;
  mat4 space3d;
  mat4 space3d_ortho;
  mat4 cam;

  space_t space_current;
	shaderkind shaderkind_current;
	obj_shader* shader_current;

  vector_t pointlights;
  vector_t dirlights;
  vec4 ambient;

  ibl_lighting ibl;
  tex_t global_env;
  tex_t local_env;

  GLuint lighting_buffer;

  GLuint tex_fbo;
	object_t full_rect;

  targets_t* targets;

  int samples;
} render_t;
void update_bounds(render_t* render);
tex_t tex_new();
tex_t unit_texture(GLenum format, char val);
void load_shaders(render_t* render);
void add_rect(object_t* obj, vec3 width, vec3 height, vec2 tstart, vec2 tend);
void add_cube(object_t* obj, vec3 start, vec3 end);
object_t object_new();
void object_init(object_t* obj);
void render_update_bounds3d_multisample(render_t* render, targets_t* targets);
targets_t targets_new(render_t* render, char multisample);
render_t render_new(vec2 bounds, int samples);
void object_init(object_t* obj);
tex_t tex_new();
void tex_default(tex_t* tex, GLenum format, vec2 bounds);
typedef enum {
  texshader_boxfilter,
  texshader_ao,
  texshader_ssr,
  texshader_postproc3d,
  texshader_tex
} texshader_type;
typedef union {
  struct {
    float size;
  } boxfilter;
  struct {
    float size;
    tex_t* depth;
    tex_t* ao;  // ao (non-blurred)
    tex_t* shadow;
    tex_t* shadow_depth;
    tex_t* ssr;  // ssr (non-blurred)
    tex_t* rough_metal;
  } postproc3d;
  struct {
    float radius;
    int samples;
    tex_t* normal;  // tex = depth
    float seed;
  } ao;
  struct {
    tex_t* normal;
    tex_t* depth;
    float size;
  } ssr;
} texshader_params;
void process_texture_2d(render_t* render, vec2 bounds, tex_t* tex,
                        tex_t* tex_out, texshader_type shadtype,
                        texshader_params params);
void render_texture(render_t* render, tex_t* tex, texshader_type shadtype,
                    texshader_params params);
tex_t load_hdri(render_t* render, char* hdri);
void render_object(render_t* render, object_t* obj);
void render_reset(render_t* render);
void render_reset3d_multisample(render_t* render, targets_t* targets);
void render_finish3d_multisample(render_t* render);
void render_update_bounds3d_multisample(render_t* render, targets_t* targets);
void render_setambient(render_t* render, vec4 ambient);
object_t object_new();
void object_free(object_t* obj);
void add_rect(object_t* obj, vec3 start, vec3 end, vec2 tstart, vec2 tend);
void add_cube(object_t* obj, vec3 start, vec3 end);
void render_free(render_t* render);
