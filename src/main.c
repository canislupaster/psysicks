#include <SDL2/SDL.h>
#include "gl.h"

#include "cglm/cglm.h"

#include "hashtable.h"
#include "util.h"
#include "cfg.h"

#include "fpsutil.h"
#include "graphics.h"

const char* CFG_X = "x";
const char* CFG_Y = "y";

const char* CFG_WIDTH = "width";
const char* CFG_HEIGHT = "height";
const char* CFG_VSYNC = "vsync";

map_t default_configuration() {
  map_t cfg = map_new();
  map_configure_string_key(&cfg, sizeof(config_val));

  cfg_add(&cfg, CFG_X, cfg_num, (cfg_data)(int)SDL_WINDOWPOS_CENTERED);
  cfg_add(&cfg, CFG_Y, cfg_num, (cfg_data)(int)SDL_WINDOWPOS_CENTERED);
  cfg_add(&cfg, CFG_WIDTH, cfg_num, (cfg_data)1024);
  cfg_add(&cfg, CFG_HEIGHT, cfg_num, (cfg_data)512);
  cfg_add(&cfg, CFG_VSYNC, cfg_num, (cfg_data)1);

  return cfg;
}

int main(int argc, char** argv) {
  map_t cfg = default_configuration();
  configure(&cfg, "fpscfg.txt");

  if (SDL_Init(SDL_INIT_VIDEO)!=0) errx("sdlinit failed");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  //why did i make this so complicated
  int x = *cfg_get(&cfg, CFG_X);
  int y = *cfg_get(&cfg, CFG_Y);
  int width = *cfg_get(&cfg, CFG_WIDTH);
  int height = *cfg_get(&cfg, CFG_HEIGHT);
  int vsync = *cfg_get(&cfg, CFG_VSYNC);

  SDL_Window* window = SDL_CreateWindow("fps", x, y, width, height,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  
  if (!window) errx("window creation failed");
  sdlerr();
  
  SDL_GLContext* ctx = SDL_GL_CreateContext(window);
  sdlerr();

  SDL_GL_SetSwapInterval(vsync ? 1 : 0); //VSYNC

  int w,h;
  SDL_GetWindowSize(window, &w, &h);

  vec2 bounds = {(float)w,(float)h};
  render_t render = render_new(bounds);
  
  object r2 = rect(0.2, 0.2, (vec4)BLUE);
  
  // object_setpos(&r2, (vec3){0, 0, -0.5});

  // font_t font = font_new(&render, "./fonts/Roboto/Roboto-Regular.ttf", 150);
  // object txt = text(&font, "hello world", (vec4)GREEN);
  // sdlerr();
  // glerr();

  // gltf_scene sc = load_gltf(&render, "untitled.glb");
  
  // object* scobj = vector_get(&sc.objects, 0);
  
  // r2.shader = shader_tex;
  // r2.texture = scobj->pbr.tex.diffuse;

  cam_setpos(&render, (vec3){0,0,-2});

  vector_pushcpy(&render.dirlights, &(dirlight){.dir={1, 1, 0}, .color=WHITE});

  // vector_add(&sc.dirlights, &render.dirlights);
  // vector_add(&sc.pointlights, &render.pointlights);

  render_setambient(&render, (vec4){0.2,0.2,0.2,0.2});

  unsigned long frame = 0;
  while (1) {
    frame++;
    render_reset(&render);

    

    // glm_rotate(scobj->transform, -M_PI/100, (vec3){1.0, 1.0, 1.0});

    // render_object(&render, &r2);
    // vector_iterator obj_iter = vector_iterate(&sc.objects);
    // while (vector_next(&obj_iter)) {
    //   render_object(&render, obj_iter.x);
    // }

    // render_object(&render, scobj);

    SDL_GL_SwapWindow(window);
    sdlerr();
    glerr();

    int close = 0;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
        case SDL_QUIT: close=1; break;
        case SDL_WINDOWEVENT: {
          switch (ev.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              render.bounds[0] = (float)ev.window.data1;
              render.bounds[1] = (float)ev.window.data2;

              update_bounds(&render);
              break;
            }
          }

          break;
        }
        case SDL_KEYDOWN: {
          if (ev.key.keysym.sym == SDLK_r) {
            
          }
        }
      }
    }

    if (close) break;
    SDL_Delay(1000/60);
  }

  // object_free(&r2);
  // object_free(scobj);

  // font_free(&font);
  render_free(&render);

  SDL_GetWindowPosition(window, cfg_val(&cfg, CFG_X), cfg_val(&cfg, CFG_Y));
  SDL_GetWindowSize(window, cfg_val(&cfg, CFG_WIDTH), cfg_val(&cfg, CFG_HEIGHT));

  save_configure(&cfg, "fpscfg.txt");

  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}