#include <SDL2/SDL.h>

#include "cfg.h"
#include "cglm/cglm.h"
#include "fpsutil.h"
#include "gl.h"
#include "graphics.h"
#include "hashtable.h"
#include "util.h"

const char* CFG_X = "x";
const char* CFG_Y = "y";

const char* CFG_WIDTH = "width";
const char* CFG_HEIGHT = "height";
const char* CFG_VSYNC = "vsync";

const char* CFG_MOUSE_SENSITIVITY = "mouse_sensitivity";

map_t default_configuration() {
  map_t cfg = map_new();
  map_configure_string_key(&cfg, sizeof(config_val));

  cfg_add(&cfg, CFG_X, cfg_num, (cfg_data)(int)SDL_WINDOWPOS_CENTERED);
  cfg_add(&cfg, CFG_Y, cfg_num, (cfg_data)(int)SDL_WINDOWPOS_CENTERED);
  cfg_add(&cfg, CFG_WIDTH, cfg_num, (cfg_data)1024);
  cfg_add(&cfg, CFG_HEIGHT, cfg_num, (cfg_data)512);
  cfg_add(&cfg, CFG_VSYNC, cfg_num, (cfg_data)1);
  cfg_add(&cfg, CFG_MOUSE_SENSITIVITY, cfg_float, (cfg_data)0.5);

  return cfg;
}

tex_t ao;
tex_t ssr;
unsigned long frame = 0;
gltf_scene sc;

vec2 ssr_bounds;

void update_bounds_main(render_t* render) {
	tex_default(&ao, GL_RED, render->bounds);

	glm_vec2_scale(render->bounds, 1, ssr_bounds);
	tex_default(&ssr, GL_RGB, ssr_bounds);
}

void render_probe(render_t* render, targets_t* targets, tex_t* tex, tex_t* cubemap, GLenum side) {
	render_reset(render);
	render_reset3d_multisample(render, targets);

	map_iterator iter = map_iterate(&sc.objects);
	while (map_next(&iter)) {
		object* obj = iter.x;
		render_object(render, obj);
	}

	render_finish3d_multisample(render);

	process_texture_2d(render, render->bounds, &render->targets->space3d_depth, &ao, texshader_ao, (texshader_params){.ao={.radius = 40.0/render->bounds[0], .samples = 12, .normal=&render->targets->space3d_normal, .seed=(float)frame/500.0}});

	process_texture_2d(render, ssr_bounds, &render->targets->space3d_rough_metal, &ssr, texshader_ssr, (texshader_params){.ssr={.normal=&render->targets->space3d_normal, .depth=&render->targets->space3d_depth, .size = 1.0/render->bounds[0]}});


	process_texture(render, CUBEMAP_BOUNDS, &render->targets->space3d_tex, cubemap, side, 0, texshader_postproc3d, (texshader_params){.postproc3d={.size=1.0/render->bounds[0], .depth=&render->targets->space3d_depth, .ao=&ao, .ssr=&ssr, .rough_metal=&render->targets->space3d_rough_metal}});
	copy_proc_texture(render, tex, CUBEMAP_BOUNDS);
}

int main(int argc, char** argv) {
  map_t cfg = default_configuration();
  configure(&cfg, "fpscfg.txt");

  if (SDL_Init(SDL_INIT_VIDEO) != 0) errx("sdlinit failed");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  // why did i make this so complicated
  int x = *cfg_get(&cfg, CFG_X);
  int y = *cfg_get(&cfg, CFG_Y);
  int width = *cfg_get(&cfg, CFG_WIDTH);
  int height = *cfg_get(&cfg, CFG_HEIGHT);
  int vsync = *cfg_get(&cfg, CFG_VSYNC);
  int mouse_sensitivity = *cfg_get(&cfg, CFG_MOUSE_SENSITIVITY);

  SDL_Window* window = SDL_CreateWindow(
      "fps", x, y, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!window) errx("window creation failed");
  sdlerr();

  SDL_GLContext* ctx = SDL_GL_CreateContext(window);
  sdlerr();

  SDL_GL_SetSwapInterval(vsync ? 1 : 0);  // VSYNC

  int w, h;
  SDL_GetWindowSize(window, &w, &h);

  vec2 bounds = {(float)w, (float)h};
  render_t render = render_new(bounds, 4);
	update_bounds_main(&render);

	targets_t default_targets = targets_new(&render, 1);
	render.targets = &default_targets;

  tex_t tex = load_hdri(&render, "./Frozen_Waterfall_Ref.hdr");

  // object r2 = rect(1200, 1200, (vec4)BLUE);
  // object_setpos(&r2, (vec3){-600, -600, 0});

  // r2.texture = tex;
  // r2.space = spacescreen;
  // r2.shader = shader_tex;

  // object_setpos(&r2, (vec3){0, 0, -0.5});

  // font_t font = font_new(&render, "./fonts/Roboto/Roboto-Regular.ttf", 150);
  // object txt = text(&font, "hello world", (vec4)GREEN);
  // sdlerr();
  // GLERR;

  sc = load_gltf(&render, "untitled.glb");
  // gltf_scene cube_sc = load_gltf(&render, "cube.glb");

  object cube = object_new();
  add_cube(&cube, (vec3){-1, -1, -1}, (vec3){2, 2, 2});

  cube.shader = shader_cubemap;
  
  object_init(&cube);

  // r2.shader = shader_tex;
  // r2.texture = scobj->pbr.tex.diffuse;

  map_iterator iter = map_iterate(&sc.cameras);
  map_next(&iter);
  printf("%s - %p\n", *(char**)iter.key, iter.x);

  vec3 player_pos = {0, 0, 0};
  glm_vec3_copy(((vec4*)iter.x)[3], player_pos);
	glm_vec3_negate(player_pos);
	printf("%f, %f, %f\n", player_pos[0], player_pos[1], player_pos[2]);

  vec2 player_rot = {0,0};

  iter = map_iterate(&sc.objects);
  while (map_next(&iter)) {
    object* obj = iter.x;
		obj->params.pbr.rough = 0.3;
  }

  // glm_mat4_ucopy(iter.x, render.cam);
  vector_pushcpy(&render.dirlights, &(dirlight){.dir={1, -1, 0}, .color={1,1,1,0.2}});

  render.global_env = tex;
	render.ibl.global_env_enabled = 1;

  // vector_add(&sc.dirlights, &render.dirlights);

  // render_setambient(&render, (vec4){0.5,0.5,0.5,0.5});

	ao = tex_new();
	ssr = tex_new(); //ssr is already upscaled

	//probes_t probes = probes_new(50.0, &render_probe);
	//probe_t* probe = probe_new(&render, &default_targets, &probes, (vec3){2,1,0});

	render.ibl.global_env_enabled = 1;
	render.ibl.local_env_enabled = 0;
	render.ibl.local_envdist = 10.0;
	update_bounds_main(&render);

	vec3 global_dir = {2,10,1};

	targets_t shadow_targets = targets_new(&render, 1);

  unsigned int tick = 0;

  int mouse_captured = 0;

  while (1) {
    glm_mat4_identity(render.cam);
		
    glm_rotate_x(render.cam, player_rot[1], render.cam);
    glm_rotate_y(render.cam, player_rot[0], render.cam);

    glm_translate(render.cam, player_pos);

    frame++;
		render_reset(&render);

		//cube.texture = probe->cubemap;
		//render_object(&render, &cube);

		//render_texture(&render, render.space3d_tex, texshader_tex, (texshader_params){});

		//render shadows
    render_reset3d_multisample(&render, &shadow_targets);

    iter = map_iterate(&sc.objects);
    while (map_next(&iter)) {
    	object* obj = iter.x;

			vector_iterator viter = vector_iterate(&render.dirlights);
			while (vector_next(&viter)) {
				dirlight* dirl = viter.x;
				render_object_shader_space(&render, obj, shader_shadow, &(objshader_params){.shadow={.light_dirpos=dirl->dir, .softness=0.0, .opacity=dirl->color[3], .height=&heightmap, .heightmap_size=map_scale/map_size}}, space3d);
			}

			viter = vector_iterate(&render.pointlights);
			while (vector_next(&viter)) {
				pointlight* pointl = viter.x;
				render_object_shader_space(&render, obj, shader_shadow, &(objshader_params){.shadow={.light_dirpos=pointl->pos, .softness=1.0/pointl->dist, .opacity=pointl->color[3], .height=&heightmap, .heightmap_size=map_scale/map_size}}, space3d);
			}
    }

		render_finish3d_multisample(&render);

		//render objects
    render_reset3d_multisample(&render, &default_targets);
		
    iter = map_iterate(&sc.objects);
    while (map_next(&iter)) {
    	object* obj = iter.x;
    	//mat4 transform;  // aligned
    	//glm_mat4_ucopy(obj->transform, transform);
    	//glm_rotate_x(in_transform, 0.2, out_transform);
    	//glm_rotate(transform, M_PI / 300, (vec3){0.1, 0.1, 0.0});
    	//glm_mat4_ucopy(transform, obj->transform);

			//probe_select(&render, &probes, obj->transform[3]);
    	render_object(&render, obj);
    }

		render_finish3d_multisample(&render);

		process_texture_2d(&render, render.bounds, &default_targets.space3d_depth, &ao, texshader_ao, (texshader_params){.ao={.radius = 10.0/render.bounds[0], .samples = 12, .normal=&default_targets.space3d_normal, .seed=(float)frame/500.0}});

		process_texture_2d(&render, ssr_bounds, &default_targets.space3d_rough_metal, &ssr, texshader_ssr, (texshader_params){.ssr={.normal=&default_targets.space3d_normal, .depth=&default_targets.space3d_depth, .size = 1.0/render.bounds[0]}});

		render_texture(&render, &default_targets.space3d_tex, texshader_postproc3d, (texshader_params){.postproc3d={.size=1.0/render.bounds[0], .depth=&default_targets.space3d_depth, .ao=&ao, .shadow=&shadow_targets.space3d_tex, .shadow_depth=&shadow_targets.space3d_depth, .ssr=&ssr, .rough_metal=&default_targets.space3d_rough_metal}});
		//render_texture(&render, &heightmap, texshader_tex, (texshader_params){});

    SDL_GL_SwapWindow(window);
    sdlerr();
    GLERR;

    int close = 0;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
        case SDL_QUIT:
          close = 1;
          break;
        case SDL_MOUSEBUTTONUP: {
          SDL_SetRelativeMouseMode(SDL_TRUE);
          mouse_captured = 1;
          break;
        }
        case SDL_MOUSEMOTION: {
          if (!mouse_captured) break;
          player_rot[0] += (float)ev.motion.xrel / render.bounds[0];
          player_rot[1] += (float)ev.motion.yrel / render.bounds[1];

          break;
        }
        case SDL_WINDOWEVENT: {
          switch (ev.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              render.bounds[0] = (float)ev.window.data1;
              render.bounds[1] = (float)ev.window.data2;

              update_bounds_main(&render);

							update_bounds(&render);
							render_update_bounds3d_multisample(&render, &default_targets);
							render_update_bounds3d_multisample(&render, &shadow_targets);
              break;
            }
          }

          break;
        }
        case SDL_KEYDOWN: {
          switch (ev.key.keysym.sym) {
            case SDLK_r:
              load_shaders(&render);
              break;
            case SDLK_ESCAPE: {
              SDL_SetRelativeMouseMode(SDL_FALSE);
              mouse_captured = 0;
              break;
            }
          }
        }
      }
    }

    if (close) break;

    unsigned current_tick = SDL_GetTicks();
    unsigned delta = current_tick - tick;
    tick = current_tick;

    float deltaf = (float)delta / 60.0;

    unsigned const char* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_W]) {
      glm_vec3_muladds((vec3){-sinf(player_rot[0]), sinf(player_rot[1]),
                              cosf(player_rot[0])},
                       deltaf, player_pos);
    }

    SDL_Delay(1000 / 60);
  }

  // object_free(&r2);
  // object_free(scobj);

  // font_free(&font);
  render_free(&render);

  SDL_GetWindowPosition(window, cfg_val(&cfg, CFG_X), cfg_val(&cfg, CFG_Y));
  SDL_GetWindowSize(window, cfg_val(&cfg, CFG_WIDTH),
                    cfg_val(&cfg, CFG_HEIGHT));

  save_configure(&cfg, "fpscfg.txt");

  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
