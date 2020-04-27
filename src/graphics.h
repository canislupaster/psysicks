// Automatically generated header.

#pragma once
#include <stddef.h>
#include "gl.h"
#include <ft2build.h>
#include <math.h>
#include "stb_image.h"
#include "vector.h"
#include "hashtable.h"
#include <stdlib.h>
#include FT_FREETYPE_H
#include <cglm/cglm.h>
#include "cgltf.h"
#include "util.h"
typedef enum {spacescreen, space3d, spaceuninit} space_t;
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

		//heinous hell
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

	unsigned pad[2]; //pad to 16 bytes;

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
	} tex;

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

	FT_Library freetype;

	vec2 bounds;

	GLuint uniform_buffer;

	//unit 1x1 textures
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
	object full_cube;
} render_t;
void update_bounds(render_t* render);
void load_shaders(render_t* render);
;

render_t render_new(vec2 bounds);
void render_free(render_t* render);
tex_t load_hdri(render_t* render, char* hdri);
void render_object(render_t* render, object* obj);
void render_reset(render_t* render);
typedef struct {
	map_t objects;
	map_t pointlights;
	map_t dirlights;
	map_t cameras;
} gltf_scene;
gltf_scene load_gltf(render_t* render, char* path);
