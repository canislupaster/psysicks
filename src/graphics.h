// Automatically generated header.

#pragma once
#include <ft2build.h>
#include <stddef.h>
#include "gl.h"
#include <math.h>
#include "hashtable.h"
#include "stb_image.h"
#include "vector.h"
#include <stdlib.h>
#include FT_FREETYPE_H
#include <cglm/cglm.h>
#include "cgltf.h"
#include "util.h"
#define CUBEMAP_RES 512
#define CUBEMAP_BOUNDS (vec2){(float)CUBEMAP_RES, (float)CUBEMAP_RES}
typedef enum { spacescreen, space3d, spaceuninit } space_t;
typedef enum {
  shader_fill,
  shader_tex,
  shader_cubemap,
  shader_txt,
  shader_pbr
} shaderkind;
typedef GLuint tex_t;
typedef struct {
  vector_t vertices;
  vector_t elements;

  mat4 transform;

  shaderkind shader;
  union {
    struct {
      vec4 col;
      tex_t texture;
    };

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
  };

  space_t space;

  GLuint vbo;
  GLuint ebo;
  GLuint vao;
} object;
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
  unsigned global_env_enabled;
  unsigned local_env_enabled;

  unsigned pad[2];  // pad to 16 bytes;

  vec4 local_envpos;
  float local_envdist;
} ibl_lighting;
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
    GLint color;
    GLint tex;
  } txt;

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
		GLint view_far;
  } ssr;

  struct {
    tex_shader shader;
		GLint size;
		GLint depth;
		GLint ao;
		GLint ssr;
		GLint rough_metal;
  } postproc3d;

  struct {
    tex_shader shader;
    GLint tex;
  } tex;

  FT_Library freetype;

  vec2 bounds;

  GLuint uniform_buffer;

  // unit 1x1 textures
  GLuint default_tex;
  GLuint default_texrgb;
  GLuint default_texcube;

  mat4 spacescreen;
  mat4 space3d;
  mat4 cam;

  space_t space_current;

  vector_t pointlights;
  vector_t dirlights;
  vec4 ambient;

  ibl_lighting ibl;
  tex_t global_env;
  tex_t local_env;

  GLuint lighting_buffer;

  GLuint tex_fbo;
  object full_rect;

  GLuint space3d_fbo;
  tex_t space3d_tex;
  tex_t space3d_depth;
	tex_t space3d_normal;
	tex_t space3d_rough_metal;
} render_t;
void update_bounds(render_t* render);
void load_shaders(render_t* render);
void add_rect(object* obj, vec3 width, vec3 height, vec2 tstart, vec2 tend);
void add_cube(object* obj, vec3 start, vec3 end);
object object_new();
void object_init(object* obj);
tex_t tex_new();
render_t render_new(vec2 bounds);
void render_free(render_t* render);
void object_init(object* obj);
tex_t tex_new();
void tex_default_rgb(tex_t tex, vec2 bounds);
void tex_single_channel(tex_t tex, vec2 bounds);
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
		tex_t depth;
		tex_t ao; //ao (non-blurred)
		tex_t ssr; //ssr (non-blurred)
		tex_t rough_metal;
	} postproc3d;
	struct {
		float radius;
		int samples;
		tex_t normal; //tex = depth
		float seed;
	} ao;
	struct {
		tex_t normal;
		tex_t depth;
		float size;
		float view_far;
	} ssr;
} texshader_params;
void process_texture(render_t* render, vec2 bounds, tex_t tex, tex_t tex_out,
                     GLenum tex_out_target, int level, texshader_type shadtype,
                     texshader_params params);
void process_texture_2d(render_t* render, vec2 bounds, tex_t tex, tex_t tex_out, texshader_type shadtype, texshader_params params);
void render_texture(render_t* render, tex_t tex, texshader_type shadtype,
                   texshader_params params);
void copy_proc_texture(render_t* render, tex_t out, vec2 bounds);
tex_t load_hdri(render_t* render, char* hdri);
void render_object(render_t* render, object* obj);
void render_reset(render_t* render);
object object_new();
typedef struct {
	vec3 pos;
	tex_t cubemap;
} probe_t;
typedef struct {
	float radius;
	
	map_t probes;
	void (*render_probe)(render_t* render, tex_t side_tex, tex_t cubemap, GLenum side); //renders into cubemap and side_tex
} probes_t;
probes_t probes_new(float radius, void (*render_probe)(render_t* render, tex_t side_tex, tex_t cubemap, GLenum side));
probe_t* probe_new(render_t* render, probes_t* probes, vec3 pos);
void probe_select(render_t* render, probes_t* probes, vec3 pos);
void add_rect(object* obj, vec3 start, vec3 end, vec2 tstart, vec2 tend);
void add_cube(object* obj, vec3 start, vec3 end);
typedef struct {
  map_t objects;
  map_t pointlights;
  map_t dirlights;
  map_t cameras;
} gltf_scene;
gltf_scene load_gltf(render_t* render, char* path);
