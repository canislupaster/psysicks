#include <SDL.h>

#include "cfg.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"
#include "plotutil.h"
#include "gl.h"
#include "graphics.h"
#include "hashtable.h"
#include "util.h"
#include "vector.h"
#include "field.h"

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

unsigned long frame = 0;

tex_t ao;
tex_t ssr;

tex_t shadow_tex;
tex_t shadow_dep;

void update_bounds_main(render_t* render) {
	tex_default(&ao, GL_RED, render->bounds);
	ssr = render->default_texrgb;
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
  render_t render = render_new(argv[0], bounds, 4);

	targets_t default_targets = targets_new(&render, 1);
	render.targets = &default_targets;

  vec2 player_rot = {0,0};

	float radius = 10;

  vector_pushcpy(&render.dirlights, &(dirlight){.dir={1, -1, 0}, .color={1,1,1,0.2}});

	render.ibl.global_env_enabled = 0;

  render_setambient(&render, (vec4){0.2,0.2,0.2,0.2});

	object_t cube = object_new();
	add_cube(&cube, (vec3){-1, -1, -1}, (vec3){2, 2, 2});

	cube.shader = shader_fill;
	glm_vec4_one(cube.params.col);

	object_init(&cube);

	render.ibl.global_env_enabled = 1;
	render.ibl.local_env_enabled = 0;
	render.ibl.local_envdist = 10.0;
	update_bounds_main(&render);

  unsigned int tick = 0;

  int mouse_captured = 0;

	field_t field = field_new();
	field.scale = 0.1;
	glm_vec3_one(field.gen.default_val);
	field_get(&field, (vec3){0,0,-1});

	GLERR;
	object_t arrow = object_new();
	add_rect(&arrow, (vec3){0,0,0}, (vec3){1,1,0}, (vec2){0,0}, (vec2){1,1});
	arrow.shader = shader_arrow;
  object_init(&arrow);
	GLERR;

	ao = tex_new();
	shadow_tex = unit_texture(GL_RGB, 0);
	shadow_dep = unit_texture(GL_DEPTH_COMPONENT, 0);

	GLERR;

	update_bounds_main(&render);

	while (1) {
    glm_mat4_identity(render.cam);

		glm_translate_z(render.cam, -radius);

    glm_rotate_x(render.cam, player_rot[1], render.cam);
    glm_rotate_y(render.cam, player_rot[0], render.cam);

    frame++;
		render_reset(&render);
		render_reset3d_multisample(&render, &default_targets);
		
		arrow.transform[0][0] = field.scale;
		arrow.transform[1][1] = field.scale;

		axis_iter_t a_iter = axis_iter(&field.z);
		while (axis_next(&a_iter)) {
			field_fromint(&field, a_iter.indices, arrow.transform[3]);
			arrow.params.arrow = a_iter.x;
			render_object(&render, &arrow);
		}

		render_finish3d_multisample(&render);

		process_texture_2d(&render, render.bounds, &default_targets.space3d_depth, &ao, texshader_ao, (texshader_params){.ao={.radius = 10.0/render.bounds[0], .samples = 12, .normal=&default_targets.space3d_normal, .seed=(float)frame/500.0}});

		render_texture(&render, &default_targets.space3d_tex, texshader_postproc3d, (texshader_params){.postproc3d={.size=1.0/render.bounds[0], .depth=&default_targets.space3d_depth, .ao=&ao, .shadow=&shadow_tex, .shadow_depth=&shadow_dep, .ssr=&ssr, .rough_metal=&default_targets.space3d_rough_metal}});

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
              break;
            }
          }

          break;
        }
        case SDL_KEYDOWN: {
          switch (ev.key.keysym.sym) {
            case SDLK_r:
              load_shaders(&render, argv[0]);
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
			radius -= deltaf;
    } else if (keys[SDL_SCANCODE_S]) {
			radius += deltaf;
		}

    SDL_Delay(1000 / 60);
  }

  object_free(&arrow);

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
