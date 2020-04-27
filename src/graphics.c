#include <stdlib.h>
#include <stddef.h>

#include "gl.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <math.h>
#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vector.h"
#include "hashtable.h"
#include "util.h"

#include "fpsutil.h"

#define WHITE {1.0, 1.0, 1.0, 1.0}
#define RED {1.0, 0.0, 0.0, 1.0}
#define GREEN {0.0, 1.0, 0.0, 1.0}
#define BLUE {0.0, 0.0, 1.0, 1.0}

typedef enum {spacescreen, space3d, spaceuninit} space_t;

typedef enum {
	shader_fill,
	shader_tex,
	shader_cubemap,
	shader_txt,
	shader_pbr
} shaderkind;

typedef struct {
	vec3 pos;
	vec3 normal;
	vec2 texpos;
	vec3 tangent;
} vertex;

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
	vec4 color;
	vec3 pos;
	float dist;
} pointlight;

typedef struct {
	vec4 color;
	vec3 dir;
} dirlight;

#define MAX_LIGHTS 10

typedef struct {
	unsigned global_env_enabled;
	unsigned local_env_enabled;

	unsigned pad[2]; //pad to 16 bytes;

	vec4 local_envpos;
	float local_envdist;
} ibl_lighting;

typedef struct {
	int pointlights_enabled;
	pointlight pointlights[MAX_LIGHTS];

	int dirlights_enabled;
	dirlight dirlights[MAX_LIGHTS];

	ibl_lighting ibl;
	vec4 ambient;
} lighting;

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

GLuint prog_new(const char* frag_src, const char* vert_src) {
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(frag, 1, &frag_src, NULL);
	glCompileShader(frag);

	glShaderSource(vert, 1, &vert_src, NULL);
	glCompileShader(vert);

	int succ;
	char err[1024];

	glGetShaderiv(frag, GL_COMPILE_STATUS, &succ);
	if (!succ) {
		glGetShaderInfoLog(frag, 1024, NULL, err);
		errx("shader frag: \n%s\n", err);
	}

	glGetShaderiv(vert, GL_COMPILE_STATUS, &succ);
	if (!succ) {
		glGetShaderInfoLog(vert, 1024, NULL, err);
		errx("shader vert: \n%s\n", err);
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	return prog;
}

obj_shader shader_new(const char* frag_src, const char* vert_src) {
	obj_shader shad = {.prog=prog_new(frag_src, vert_src)};
	shad.obj = glGetUniformBlockIndex(shad.prog, "object");
	glUniformBlockBinding(shad.prog, shad.obj, 0);

	shad.transform = glGetUniformLocation(shad.prog, "transform");

	return shad;
}

tex_shader tex_shader_new(const char* frag_src, const char* vert_src) {
	tex_shader shad = {.prog=prog_new(frag_src, vert_src)};
	shad.tex = glGetUniformLocation(shad.prog, "tex");
	return shad;
}

void update_bounds(render_t* render) {
	glm_mat4_identity(render->spacescreen);
	render->spacescreen[0][0] = 1/render->bounds[0];
	render->spacescreen[1][1] = 1/render->bounds[1];

	glm_perspective(M_PI/3, render->bounds[0]/render->bounds[1], 0.1, 1000.0, render->space3d);

	glViewport(0, 0, render->bounds[0], render->bounds[1]);
}

void unit_texture(GLuint* tex, GLenum format) {
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
}
/*
	 void unit_cube_texture(GLuint* tex, GLenum format) {
	 glGenTextures(1, tex);
	 glBindTexture(GL_TEXTURE_2D, *tex);

	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	 glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, (unsigned char[4]){255,255,255,255});  
	 }
	 */
void load_shaders(render_t* render) {
	render->fill.shader = shader_new(read_file("shaders/fill.frag"), read_file("shaders/fill.vert"));
	render->fill.color = glGetUniformLocation(render->fill.shader.prog, "color");

	render->cubemap.shader = shader_new(read_file("shaders/cubemap.frag"), read_file("shaders/cubemap.vert"));
	render->cubemap.tex = glGetUniformLocation(render->cubemap.shader.prog, "tex");

	render->tex.shader = shader_new(read_file("shaders/tex.frag"), read_file("shaders/tex.vert"));
	render->tex.tex = glGetUniformLocation(render->tex.shader.prog, "tex");

	render->txt.shader = shader_new(read_file("shaders/txt.frag"), read_file("shaders/txt.vert"));
	render->txt.color = glGetUniformLocation(render->txt.shader.prog, "color");
	render->txt.tex = glGetUniformLocation(render->txt.shader.prog, "tex");

	render->pbr.shader = shader_new(read_file("shaders/pbr.frag"), read_file("shaders/pbr.vert"));
	render->pbr.color = glGetUniformLocation(render->pbr.shader.prog, "color");
	render->pbr.emissive = glGetUniformLocation(render->pbr.shader.prog, "emissive");
	render->pbr.metal = glGetUniformLocation(render->pbr.shader.prog, "metal");
	render->pbr.rough = glGetUniformLocation(render->pbr.shader.prog, "rough");
	render->pbr.occlusion = glGetUniformLocation(render->pbr.shader.prog, "occlusion");

	render->pbr.tex.diffuse = glGetUniformLocation(render->pbr.shader.prog, "diffusetex");
	render->pbr.tex.emissive = glGetUniformLocation(render->pbr.shader.prog, "emissivetex");
	render->pbr.tex.normal = glGetUniformLocation(render->pbr.shader.prog, "normaltex");
	render->pbr.tex.orm = glGetUniformLocation(render->pbr.shader.prog, "ormtex");

	render->pbr.local_env = glGetUniformLocation(render->pbr.shader.prog, "local_env");
	render->pbr.global_env = glGetUniformLocation(render->pbr.shader.prog, "global_env");

	GLuint pbr_lighting = glGetUniformBlockIndex(render->pbr.shader.prog, "lighting");
	glUniformBlockBinding(render->pbr.shader.prog, pbr_lighting, 1);

	render->boxfilter.shader =
		tex_shader_new(read_file("shaders/boxfilter.frag"),
				read_file("shaders/boxfilter.vert"));
	render->boxfilter.size =
		glGetUniformLocation(render->boxfilter.shader.prog, "size");
}

void add_rect(object* obj, vec3 width, vec3 height, vec2 tstart, vec2 tend);
void add_cube(object* obj, vec3 start, vec3 end);

object object_new();
void object_init(object* obj);

render_t render_new(vec2 bounds) {
	render_t render;
	render.space_current = spaceuninit;

	glm_vec2_copy(bounds, render.bounds);
	update_bounds(&render);

	glm_mat4_identity(render.cam);

	render.pointlights = vector_new(sizeof(pointlight));
	render.dirlights = vector_new(sizeof(dirlight));
	glm_vec4_zero(render.ambient);

	render.ibl.local_env_enabled = 0;
	render.ibl.global_env_enabled = 0;

	//enable
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);  

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set texture fbo for processing textures
	glGenFramebuffers(1, &render.tex_fbo);

	//set default texture
	unit_texture(&render.default_tex, GL_RGBA);
	unit_texture(&render.default_texrgb, GL_RGB);

	load_shaders(&render);

	//set cube and rect
	render.full_cube = object_new();
	add_cube(&render.full_cube, (vec3){-1,-1,-1}, (vec3){1,1,1});
	object_init(&render.full_cube);

	render.full_rect = object_new();
	add_rect(&render.full_rect, (vec3){-1, -1, 0}, (vec3){1, 1, 0}, (vec2){0,0}, (vec2){1,1});
	object_init(&render.full_rect);

	glGenBuffers(1, &render.uniform_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, render.uniform_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4[2]), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, render.uniform_buffer);

	glGenBuffers(1, &render.lighting_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, render.lighting_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lighting), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, render.lighting_buffer);

	if (FT_Init_FreeType(&render.freetype)) errx("freetype failed to load");

	return render;
}

void render_free(render_t* render) {
	glDeleteProgram(render->fill.shader.prog);
	glDeleteProgram(render->tex.shader.prog);

	glDeleteBuffers(1, &render->tex_fbo);

	glDeleteTextures(1, &render->default_tex);
	glDeleteTextures(1, &render->default_texrgb);
	glDeleteTextures(1, &render->default_texcube);

	FT_Done_FreeType(render->freetype);
}

void object_init(object* obj) {
	glGenVertexArrays(1, &obj->vao);
	glBindVertexArray(obj->vao);

	glGenBuffers(1, &obj->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(obj->vertices.length * obj->vertices.size), obj->vertices.data, GL_DYNAMIC_DRAW);
	glerr();

	glGenBuffers(1, &obj->ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(obj->elements.length * obj->elements.size), obj->elements.data, GL_DYNAMIC_DRAW);
	glerr();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, pos));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texpos));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, tangent));
	glEnableVertexAttribArray(3);

	glerr();
}

void add_tangent(vertex* first, vertex* second, vertex* third) {
	if (!vec3_iszero(first->tangent)) return;

	//differentiate average xyz over uv
	vec3 second_dir;
	vec3 third_dir;

	glm_vec3_sub(first->pos, second->pos, second_dir);
	glm_vec3_sub(first->pos, third->pos, third_dir);

	vec2 second_uv;
	vec2 third_uv;

	glm_vec2_sub(first->texpos, second->texpos, second_uv);
	glm_vec2_sub(first->texpos, third->texpos, third_uv);

	vec3 second_u;
	vec3 third_u;
	glm_vec3_divs(second_dir, second_uv[0], second_u);
	glm_vec3_divs(third_dir, third_uv[0], third_u);

	glm_vec3_add(second_u, third_u, first->tangent);
	glm_vec3_normalize(first->tangent);
}

void add_tangents(object* obj) {
	//(re)set tangents
	vector_iterator iter = vector_iterate(&obj->vertices);
	while (vector_next(&iter)) {
		vertex* vert = iter.x;
		glm_vec3_copy((vec3){0,0,0}, vert->tangent);
	}

	//set unset tangents using each triangles
	//assuming texture coordinates are continuous/cubic
	vector_iterator elem_iter = vector_iterate(&obj->elements);
	while (vector_next(&elem_iter)) {
		// ...
		vertex* first = vector_get(&obj->vertices, *(GLuint*)elem_iter.x);
		vector_next(&elem_iter);
		vertex* second = vector_get(&obj->vertices, *(GLuint*)elem_iter.x);
		vector_next(&elem_iter);
		vertex* third = vector_get(&obj->vertices, *(GLuint*)elem_iter.x);

		add_tangent(first, second, third);
		add_tangent(second, first, third);
		add_tangent(third, first, second);
	}
}

tex_t tex_new() {
	tex_t tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glerr();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glerr();

	return tex;
};

void tex_free(tex_t tex) {
	glDeleteTextures(1, &tex);
};

typedef enum {
	texshader_boxfilter
} texshader_type;

typedef union {
	struct {
		float size;
	} boxfilter;
} texshader_params;

void process_texture(render_t* render, GLenum tex_out_target, tex_t tex, tex_t tex_out, int level, texshader_type shad, texshader_params params) {
	glBindFramebuffer(GL_FRAMEBUFFER, render->tex_fbo);
	glerr();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_out_target, tex_out, level);
	glerr();

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); 
	if(status != GL_FRAMEBUFFER_COMPLETE)
		errx("%u", status);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClearColor(0,0,0,0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glerr();

	switch (shad) {
		case texshader_boxfilter:
		{
			glUseProgram(render->boxfilter.shader.prog);
			glUniform1f(render->boxfilter.size, params.boxfilter.size);
			glUniform1i(render->boxfilter.shader.tex, 0);
		};
	}
	glerr();

	glBindVertexArray(render->full_rect.vao);
	glDrawElements(GL_TRIANGLES, render->full_rect.elements.length, GL_UNSIGNED_INT, 0);
	glerr();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#define CUBEMAP_RES 512

void convolve_side(render_t* render, GLenum side, tex_t cubemap, float* data) {
	int level=1;

	tex_t tex = tex_new(); //boxfilter texture	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, data);

	for (int new_c = CUBEMAP_RES/2; new_c >= 1; new_c /= 2) {
		glTexImage2D(side, level, GL_RGB16F, new_c, new_c, 0, GL_RGB, GL_FLOAT, NULL);
		process_texture(render, side, tex, cubemap, level, texshader_boxfilter, (texshader_params){.boxfilter={.size=0.5/(float)new_c}});

		glBindTexture(GL_TEXTURE_2D, tex);
		glBindFramebuffer(GL_FRAMEBUFFER, render->tex_fbo);

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 0, 0, new_c, new_c, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		level++;
	}

	tex_free(tex);
}

tex_t load_hdri(render_t* render, char* hdri) {
	int w, h, channels;

	stbi_set_flip_vertically_on_load(1);
	float* data = stbi_loadf(hdri, &w, &h, &channels, 3);
	stbi_set_flip_vertically_on_load(0);
	if (!data) errx("invalid hdri %s", hdri);

	// ...
	int side_size = CUBEMAP_RES * CUBEMAP_RES * 3 * sizeof(float);
	float* up = heap(side_size);
	float* down = heap(side_size);
	float* left = heap(side_size);
	float* front = heap(side_size);
	float* right = heap(side_size);
	float* back = heap(side_size);

	float* sides[] = {left, front, right, back, NULL};

	float wf = (float)w-1;
	float hf = (float)h-1;

	float cf = CUBEMAP_RES;
	float ch = CUBEMAP_RES/2;

	float corner = 1.0/3.0;

	//render poles
	for (int y=0; y<CUBEMAP_RES; y++) {
		for (int x=0; x<CUBEMAP_RES; x++) {
			vec2 where = {((float)x - ch)/ch, (float)(y - ch)/ch};

			float dist = sinf(fabsf(glm_vec2_norm(where)));

			glm_vec2_normalize(where);
			float angle = acosf(where[0])/(2*M_PI);
			if (where[1] < 0) angle = 1.0-angle;

			vec2 pos = {angle, 1.0 - dist*corner};

			int in_x=(int)(pos[0]*wf) - 1, in_y=(int)(pos[1]*hf) - 1;
			memcpy(&up[3*(CUBEMAP_RES*y + x)], &data[3*(in_y*w + in_x)], sizeof(float[3]));
		}
	}

	for (int y=0; y<CUBEMAP_RES; y++) {
		for (int x=0; x<CUBEMAP_RES; x++) {
			vec2 where = {((float)x - ch)/ch, -(float)(y - ch)/ch};

			float dist = sinf(fabsf(glm_vec2_norm(where)));

			glm_vec2_normalize(where);
			float angle = acosf(where[0])/(2*M_PI);
			if (where[1] < 0) angle = 1.0-angle;

			vec2 pos = {angle, dist*corner};

			int in_x=(int)(pos[0]*wf), in_y=(int)(pos[1]*hf);
			memcpy(&down[3*(CUBEMAP_RES*y + x)], &data[3*(in_y*w + in_x)], sizeof(float[3]));
		}
	}

	float angle_offset = 0;
	for (float** side = sides; *side; side++) {
		for (int x = 0; x < CUBEMAP_RES; x++) {
			for (int y = 0; y < CUBEMAP_RES; y++) {
				//invert texture coordinates (rows are written top to bottom, opengl reads bottom to top)
				vec2 where = {-((float)x-ch)/ch, (float)y/cf};

				float y_angle = asinf(corner + corner*where[1]);
				float x_coeff = 3/4 + 1/(4*cos(y_angle));

				vec2 pos = {angle_offset + (x_coeff*where[0] + 1)/2, corner + corner*(1.0-where[1])};

				int in_x = (int) (pos[0] * wf), in_y = (int) (pos[1] * hf);
				memcpy(&((*side)[3 * (CUBEMAP_RES * y + x)]), &data[3 * (in_y * w + in_x)], sizeof(float[3]));
			}
		}

		angle_offset += 1.0/4.0;
	}


	drop(data);

	tex_t tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, up);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, down);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, left);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, right);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, front);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB16F, CUBEMAP_RES, CUBEMAP_RES, 0, GL_RGB, GL_FLOAT, back);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glerr();
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glerr();

	convolve_side(render, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, tex, up);
	convolve_side(render, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, tex, down);
	convolve_side(render, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, tex, left);
	convolve_side(render, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tex, right);
	convolve_side(render, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, tex, front);
	convolve_side(render, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, tex, back);
	glerr();

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glerr();

	drop(up);
	drop(down);

	drop(left);
	drop(front);
	drop(right);
	drop(back);

	return tex;
}

void render_object(render_t* render, object* obj) {
	glBindBuffer(GL_UNIFORM_BUFFER, render->uniform_buffer);

	if (render->space_current != obj->space) {
		vec4* spacemat = obj->space==spacescreen ? render->spacescreen : render->space3d;
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), spacemat);

		vec4* cam = obj->space==spacescreen ? GLM_MAT4_IDENTITY : render->cam;
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), cam);

		render->space_current = obj->space;
	}

	glerr();

	obj_shader* shad;
	switch (obj->shader) {
		case shader_tex: shad = &render->tex.shader; break;
		case shader_cubemap: shad = &render->cubemap.shader; break;
		case shader_txt: shad = &render->txt.shader; break;
		case shader_fill: shad = &render->fill.shader; break;
		case shader_pbr: shad = &render->pbr.shader; break;
	}

	glUseProgram(shad->prog);

	switch (obj->shader) {
		case shader_tex: {
											 glActiveTexture(GL_TEXTURE0);
											 glBindTexture(GL_TEXTURE_2D, obj->texture);
											 glUniform1i(render->tex.tex, 0);
											 break;
										 }
		case shader_cubemap: {
													 glActiveTexture(GL_TEXTURE0);
													 glBindTexture(GL_TEXTURE_CUBE_MAP, obj->texture);
													 glUniform1i(render->cubemap.tex, 0);
													 break;
												 }
		case shader_fill: {
												glUniform4fv(render->fill.color, 1, obj->col);
												break;
											}
		case shader_txt: {
											 glUniform4fv(render->txt.color, 1, obj->col);

											 glActiveTexture(GL_TEXTURE0);
											 glBindTexture(GL_TEXTURE_2D, obj->texture);
											 glUniform1i(render->txt.tex, 0);
											 break;
										 }
		case shader_pbr: {
											 glActiveTexture(GL_TEXTURE0);
											 glBindTexture(GL_TEXTURE_2D, obj->pbr.tex.diffuse);
											 glUniform1i(render->pbr.tex.diffuse, 0);
											 glActiveTexture(GL_TEXTURE1);
											 glBindTexture(GL_TEXTURE_2D, obj->pbr.tex.emissive);
											 glUniform1i(render->pbr.tex.emissive, 1);
											 glActiveTexture(GL_TEXTURE2);
											 glBindTexture(GL_TEXTURE_2D, obj->pbr.tex.normal);
											 glUniform1i(render->pbr.tex.normal, 2);
											 glActiveTexture(GL_TEXTURE3);
											 glBindTexture(GL_TEXTURE_2D, obj->pbr.tex.orm);
											 glUniform1i(render->pbr.tex.orm, 3);
											 glUniform4fv(render->pbr.color, 1, obj->pbr.col);

											 glUniform3fv(render->pbr.emissive, 1, obj->pbr.emissive);
											 glUniform1f(render->pbr.metal, obj->pbr.metal);
											 glUniform1f(render->pbr.rough, obj->pbr.rough);
											 glUniform1f(render->pbr.occlusion, obj->pbr.occlusion);

											 if (render->ibl.global_env_enabled) {
												 glActiveTexture(GL_TEXTURE4);
												 glBindTexture(GL_TEXTURE_CUBE_MAP, render->global_env);
											 }

											 if (render->ibl.local_env_enabled) {
												 glActiveTexture(GL_TEXTURE5);
												 glBindTexture(GL_TEXTURE_CUBE_MAP, render->local_env);
											 }

											 glUniform1i(render->pbr.global_env, 4);
											 glUniform1i(render->pbr.local_env, 5);

											 break;
										 }
	}

	glerr();

	glUniformMatrix4fv(shad->transform, 1, GL_FALSE, obj->transform[0]);
	glerr();

	glBindVertexArray(obj->vao);
	glerr();

	glDrawElements(GL_TRIANGLES, obj->elements.length, GL_UNSIGNED_INT, 0);
	glerr();
}

//call before frame to reset spaces and fix lights
void render_reset(render_t* render) {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render->space_current = spaceuninit;

	vector_truncate(&render->pointlights, MAX_LIGHTS);
	vector_truncate(&render->dirlights, MAX_LIGHTS);

	glBindBuffer(GL_UNIFORM_BUFFER, render->lighting_buffer);

	int enabled = render->pointlights.length;
	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, pointlights_enabled), sizeof(int), &enabled);
	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, pointlights), render->pointlights.size * render->pointlights.length, render->pointlights.data);

	enabled = render->dirlights.length;
	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, dirlights_enabled), sizeof(int), &enabled);
	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, dirlights), render->dirlights.size * render->dirlights.length, render->dirlights.data);

	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, ibl), sizeof(ibl_lighting), &render->ibl);
}

void render_setambient(render_t* render, vec4 ambient) {
	glBindBuffer(GL_UNIFORM_BUFFER, render->lighting_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, offsetof(lighting, ambient), sizeof(vec4), ambient);
}

object object_new() {
	object obj = {.transform=GLM_MAT4_IDENTITY_INIT, .shader=shader_fill, .space=space3d};
	obj.vertices = vector_new(sizeof(vertex));
	obj.elements = vector_new(sizeof(GLuint));

	return obj;
}

void object_setpos(object* obj, vec3 pos) {
	glm_translate(obj->transform, pos);
}

void object_scale(object* obj, vec3 scale) {
	glm_scale(obj->transform, scale);
}

void cam_setpos(render_t* render, vec3 pos) {
	glm_translate(render->cam, pos);
}

void object_free(object* obj) {
	glDeleteVertexArrays(1, &obj->vao);
	glDeleteBuffers(1, &obj->vbo);
	glDeleteBuffers(1, &obj->ebo);

	switch (obj->shader) {
		case shader_pbr: {
											 glDeleteTextures(1, &obj->pbr.tex.diffuse);
											 glDeleteTextures(1, &obj->pbr.tex.emissive);
											 glDeleteTextures(1, &obj->pbr.tex.normal);
											 glDeleteTextures(1, &obj->pbr.tex.orm);
											 break;
										 };
		case shader_tex:
		case shader_cubemap:
		case shader_txt: {
											 glDeleteTextures(1, &obj->texture);
											 break;
										 }

		default:;
	}
}

void add_rect(object* obj, vec3 start, vec3 end, vec2 tstart, vec2 tend) {
	vertex* verts;

	//cglm arrays are hell
	if (end[1] == 0) {
		//x-z plane
		verts = (vertex[]){
			{.pos={start[0], start[1], start[2]}, .texpos={tstart[0], tstart[1]}},
				{.pos={start[0]+end[0], start[1], start[2]}, .texpos={tstart[0]+tend[0], tstart[1]}},
				{.pos={start[0], start[1], start[2]+end[2]}, .texpos={tstart[0], tstart[1]+tend[1]}},
				{.pos={start[0]+end[0], start[1], start[2]+end[2]}, .texpos={tstart[0]+tend[0], tstart[1]+tend[1]}},
		};
	} else {
		//x-y or y-z
		verts = (vertex[]){
			{.pos={start[0], start[1], start[2]}, .texpos={tstart[0], tstart[1]}},
				{.pos={start[0]+end[0], start[1], start[2]+end[2]}, .texpos={tstart[0]+tend[0], tstart[1]}},
				{.pos={start[0], start[1]+end[1], start[2]}, .texpos={tstart[0], tstart[1]+tend[1]}},
				{.pos={start[0]+end[0], start[1]+end[1], start[2]+end[2]}, .texpos={tstart[0]+tend[0], tstart[1]+tend[1]}},
		};
	}

	unsigned long i = obj->vertices.length;
	GLuint elems[] = {i, i+1, i+2, i+1, i+2, i+3};

	vector_stockcpy(&obj->vertices, 4, verts);
	vector_stockcpy(&obj->elements, 6, elems);
}

//does not handle textures, extrudes a rectangle from last edge
void extrude(object* obj, vec3 offset) {
	vertex* edge[] = {vector_get(&obj->vertices, obj->vertices.length-2), vector_get(&obj->vertices, obj->vertices.length-1)};
	vertex verts[2];
	glm_vec3_add(edge[0]->pos, offset, verts[0].pos);
	glm_vec3_add(edge[1]->pos, offset, verts[1].pos);

	unsigned long i = obj->vertices.length;
	GLuint elems[] = {i-2, i-1, i+1, i-2, i, i+1};

	vector_stockcpy(&obj->vertices, 2, verts);
	vector_stockcpy(&obj->elements, 6, elems);
}

void add_cube(object* obj, vec3 start, vec3 end) {
	add_rect(obj, start, (vec3){end[0], end[1], 0}, (vec2){0,0}, (vec2){1,1});
	extrude(obj, (vec3){0,0,end[2]});
	extrude(obj, (vec3){0,-end[1],0});
	extrude(obj, (vec3){0,0,-end[2]});

	//add sides
	add_rect(obj, (vec3){start[0]+end[0], start[1], start[2]}, (vec3){0,end[1],end[2]}, (vec2){0,0}, (vec2){1,1});
	add_rect(obj, (vec3){start[0], start[1], start[2]}, (vec3){0,end[1],end[2]}, (vec2){0,0}, (vec2){1,1});
}

object rect(float width, float height, vec4 col) {
	object obj = object_new();
	glm_vec4_ucopy(col, obj.col);

	add_rect(&obj, (vec3){0,0,0}, (vec3){width,height,0}, (vec2){0,0}, (vec2){1,1});

	object_init(&obj);

	return obj;
}

typedef struct {
	char empty;
	unsigned x;
	vec2 bounds;

	unsigned x_bearing;
	unsigned y_bearing;

	unsigned x_advance;
} atlas_pos;

//simple texture atlas for fonts
typedef struct {
	GLuint atlas;
	atlas_pos chars[96];
	unsigned width, height;
} font_t;

font_t font_new(render_t* render, char* fontpath, unsigned size) {
	font_t font;

	FT_Face face;
	if (FT_New_Face(render->freetype, fontpath, 0, &face)) errx("invalid font at %s", fontpath);
	FT_Set_Pixel_Sizes(face, 0, size);

	font.height=0;
	font.width = 0;

	for (int i=32; i<128; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
			errx("failed to render character %c", i);

		if (!face->glyph->bitmap.buffer) continue;
		if (face->glyph->bitmap.rows > font.height)
			font.height = face->glyph->bitmap.rows;

		font.width += face->glyph->bitmap.width + 1;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //override default 4 byte alignment

	glGenTextures(1, &font.atlas);
	glBindTexture(GL_TEXTURE_2D, font.atlas);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char* tex = heap(font.width * font.height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.width, font.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glerr();

	unsigned x=0;
	for (int i=32; i<128; i++) {
		atlas_pos* atp = &font.chars[i-32];
		atp->x = x;

		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
			errx("failed to render character %c", i);

		glm_vec2_copy((vec2){face->glyph->bitmap.width, face->glyph->bitmap.rows}, atp->bounds);

		atp->x_advance = face->glyph->advance.x / 64;
		atp->x_bearing = face->glyph->bitmap_left;
		atp->y_bearing = face->glyph->bitmap_top;

		if (!face->glyph->bitmap.buffer) {
			atp->empty = 1;
			continue;
		} else {
			atp->empty = 0;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows,
				GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		glerr();

		x += face->glyph->bitmap.width + 1; //padding
	}

	FT_Done_Face(face);

	return font;
}

object text(font_t* font, char* txt, vec4 col) {
	object obj = object_new();
	obj.shader = shader_txt;
	obj.texture = font->atlas;
	glm_vec4_ucopy(col, obj.col);

	obj.space = spacescreen;

	float x=0;
	for (int i=0; i<strlen(txt); i++) {
		atlas_pos atp = font->chars[txt[i]-32];

		if (!atp.empty) {
			add_rect(&obj, (vec3){x+atp.x_bearing, (float)atp.y_bearing-atp.bounds[1], 0}, atp.bounds,
					(vec2){(float)atp.x / (float)font->width, atp.bounds[1] / (float)font->height},
					(vec2){atp.bounds[0] / (float)font->width, -atp.bounds[1] / (float)font->height});
		}

		x += atp.x_advance;
	}

	object_init(&obj);
	return obj;
}

void font_free(font_t* font) {
	glDeleteTextures(1, &font->atlas);
}

tex_t load_gltf_texture(cgltf_texture* tex, GLenum format) {
	tex_t gltex;
	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);

	if (tex->sampler) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->sampler->min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->sampler->mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->sampler->wrap_s);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->sampler->wrap_t);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	unsigned char* data = NULL;
	int w, h, channels;

	if (strcmp(tex->image->mime_type, "image/png")==0) {
		unsigned err;

		data = stbi_load_from_memory(tex->image->buffer_view->buffer->data + tex->image->buffer_view->offset, tex->image->buffer_view->size, &w, &h, &channels, format == GL_RGB ? 3 : 4);

		if (!data) {
			errx("stb_image failed to load image");
		}
	} else {
		errx("image in gltf texture not png");
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	drop(data);

	glGenerateMipmap(GL_TEXTURE_2D);

	return gltex;
}

typedef struct {
	map_t objects;
	map_t pointlights;
	map_t dirlights;
	map_t cameras;
} gltf_scene;

void load_gltf_node(render_t* render, cgltf_node* node, gltf_scene* sc) {
	if (node->children_count>0) {
		for (int i=0; i<node->children_count; i++) {
			load_gltf_node(render, node->children[i], sc);
		}

		return;
	}

	char* name = heapstr("%s", node->name);
	printf("%s\n", name);

	if (node->mesh) {
		for (int p=0; p<node->mesh->primitives_count; p++) {
			cgltf_primitive* prim = &node->mesh->primitives[p];
			if (!prim->material) {
				errx("primitive in %s does not have material", name);
			}

			cgltf_texture_view* basetex = &prim->material->pbr_metallic_roughness.base_color_texture;
			cgltf_texture_view* normaltex = &prim->material->normal_texture;
			cgltf_texture_view* emissivetex = &prim->material->emissive_texture;
			cgltf_texture_view* orm = &prim->material->pbr_metallic_roughness.metallic_roughness_texture;

			object obj = object_new();
			obj.shader = shader_pbr;

			if (basetex->texture) obj.pbr.tex.diffuse = load_gltf_texture(basetex->texture, GL_RGBA);
			else obj.pbr.tex.diffuse = render->default_tex;
			if (normaltex->texture) obj.pbr.tex.normal = load_gltf_texture(normaltex->texture, GL_RGB);
			else obj.pbr.tex.normal = render->default_texrgb;
			if (emissivetex->texture) obj.pbr.tex.emissive = load_gltf_texture(emissivetex->texture, GL_RGB);
			else obj.pbr.tex.emissive = render->default_texrgb;
			if (orm->texture) obj.pbr.tex.orm = load_gltf_texture(orm->texture, GL_RGBA);
			else obj.pbr.tex.orm = render->default_texrgb;

			glm_vec4_ucopy(prim->material->pbr_metallic_roughness.base_color_factor, obj.pbr.col);
			obj.pbr.metal = prim->material->pbr_metallic_roughness.metallic_factor;
			obj.pbr.rough = prim->material->pbr_metallic_roughness.roughness_factor;
			obj.pbr.normal = prim->material->normal_texture.texture ? prim->material->normal_texture.scale : 0;
			obj.pbr.occlusion = prim->material->occlusion_texture.texture ? prim->material->occlusion_texture.scale : 1.0;
			glm_vec3_copy(prim->material->emissive_factor, obj.pbr.emissive);

			cgltf_node_transform_world(node, obj.transform[0]);

			int tex = -1;

			for (int a=0; a<prim->attributes_count; a++) {
				cgltf_attribute* attr = &prim->attributes[a];

				if (obj.vertices.length < attr->data->count) {
					vector_stock(&obj.vertices, attr->data->count-obj.vertices.length);
				}

				if (strcmp(attr->name, "POSITION")==0) {
					float positions[3*obj.vertices.length];
					cgltf_accessor_unpack_floats(attr->data, positions, 3*obj.vertices.length);

					vector_iterator iter = vector_iterate(&obj.vertices);
					while (vector_next(&iter)) {
						glm_vec3_copy(&positions[(iter.i-1)*3], ((vertex*)iter.x)->pos);
					}
				} else if (strcmp(attr->name, "NORMAL")==0) {
					float normals[3*obj.vertices.length];
					cgltf_accessor_unpack_floats(attr->data, normals, 3*obj.vertices.length);

					vector_iterator iter = vector_iterate(&obj.vertices);
					while (vector_next(&iter)) {
						glm_vec3_copy(&normals[(iter.i-1)*3], ((vertex*)iter.x)->normal);
					}
				} else {
					if (tex>0) errx("texture coordinate already defined for %s", name);
					if (sscanf(attr->name, "TEXCOORD_%i", &tex)==EOF)
						errx("could not read gltf: attribute %s not defined", attr->name);

					float texpositions[2*obj.vertices.length];
					cgltf_accessor_unpack_floats(attr->data, texpositions, 2*obj.vertices.length);

					vector_iterator iter = vector_iterate(&obj.vertices);
					while (vector_next(&iter)) {
						float* texpos = &texpositions[(iter.i-1)*2];
						texpos[1] = 1.0 - texpos[1];

						glm_vec2_copy(texpos, ((vertex*)iter.x)->texpos);
					}
				}
			}

			vector_stock(&obj.elements, prim->indices->count);

			vector_iterator elem_iter = vector_iterate(&obj.elements);
			while (vector_next(&elem_iter)) {
				*((GLuint*)elem_iter.x) = cgltf_accessor_read_index(prim->indices, elem_iter.i-1);
			}

			object_init(&obj);
			add_tangents(&obj);
			map_insertcpy(&sc->objects, &name, &obj);
		}
	} else if (node->light) {
		mat4 transform;
		cgltf_node_transform_world(node, transform[0]);

		if (node->light->type == cgltf_light_type_directional) {
			dirlight* dirl = map_insert(&sc->dirlights, &name).val;
			glm_vec3_copy(node->light->color, dirl->color);
			dirl->color[3] = node->light->intensity;

			glm_mat4_mulv(transform, (vec4){0,0,-1,0}, dirl->dir);
			glm_normalize(dirl->dir);
		} else if (node->light->type == cgltf_light_type_point) {
			pointlight* pointl = map_insert(&sc->pointlights, &name).val;
			glm_vec3_copy(node->light->color, pointl->color);
			glm_vec3_copy(node->translation, transform[3]); //copy from w column

			pointl->color[3] = node->light->intensity;
			pointl->dist = node->light->range;
		}
	} else if (node->camera) {
		vec4* cam = map_insert(&sc->cameras, &name).val;
		cgltf_node_transform_world(node, cam[0]);
		glm_mat4_scale_p(cam, -1);
	}	
}

gltf_scene load_gltf(render_t* render, char* path) {
	gltf_scene sc = {.objects=map_new(), .pointlights=map_new(), .dirlights=map_new(), .cameras=map_new()};
	map_configure_string_key(&sc.objects, sizeof(object));
	map_configure_string_key(&sc.pointlights, sizeof(pointlight));
	map_configure_string_key(&sc.dirlights, sizeof(dirlight));
	map_configure_string_key(&sc.cameras, sizeof(mat4));

	cgltf_options opt = {0};
	cgltf_data* data = NULL;

	cgltf_result result = cgltf_parse_file(&opt, path, &data);

	if (result != cgltf_result_success) {
		errx("cgltf failed on %s: %i", path, result);
	}

	result = cgltf_load_buffers(&opt, data, path);

	if (result != cgltf_result_success) {
		errx("cgltf failed to load buffers on %s: %i", path, result);
	}

	for (int i=0; i<data->scene->nodes_count; i++) {
		load_gltf_node(render, data->scene->nodes[i], &sc);
	}

	cgltf_free(data);

	return sc;
}
