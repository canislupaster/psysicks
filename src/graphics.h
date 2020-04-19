// Automatically generated header.

#pragma once
#include <stddef.h>
#include "gl.h"
#include <ft2build.h>
#include <math.h>
#include "stb_image.h"
#include "vector.h"
#include <stdlib.h>
#include FT_FREETYPE_H
#include <cglm/cglm.h>
#include "cgltf.h"
#include "util.h"
#define WHITE {1.0, 1.0, 1.0, 1.0}
typedef enum {spacescreen, space3d, spaceuninit} space_t;
typedef enum {
  shader_fill,
  shader_tex,
  shader_txt,
  shader_pbr
} shaderkind;
typedef struct {
  vector_t vertices;
  vector_t elements;

  mat4 transform;

  shaderkind shader;
  union {
    struct {
      vec4 col;
      GLuint texture;
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
  vec4 color;
  vec3 dir;
} dirlight;
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

    struct {
      GLint diffuse, normal, emissive, orm;
    } tex;
  } pbr;
  
  FT_Library freetype;

  vec2 bounds;

  GLuint uniform_buffer;

  //unit 1x1 textures
  GLuint default_tex;
  GLuint default_texrgb;

  mat4 spacescreen;
  mat4 space3d;
  mat4 cam;

  space_t space_current;

  vector_t pointlights;
  vector_t dirlights;
  vec4 ambient;
  GLuint lighting_buffer;
} render_t;
void update_bounds(render_t* render);
render_t render_new(vec2 bounds);
void render_free(render_t* render);
void render_object(render_t* render, object* obj);
void render_reset(render_t* render);
void render_setambient(render_t* render, vec4 ambient);
void cam_setpos(render_t* render, vec3 pos);
void object_free(object* obj);
typedef struct {
  vector_t objects;
  vector_t pointlights;
  vector_t dirlights;
} gltf_scene;
gltf_scene load_gltf(render_t* render, char* path);
