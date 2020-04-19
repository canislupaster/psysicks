#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include "gl.h"

#include <cglm/cglm.h>

#include "util.h"

int vec3_iszero(vec3 vec) {
  return vec[0] == 0 && vec[1] == 0 && vec[2] == 0;
}

void errx(char* s, ...) {
  va_list ap;

	va_start(ap, s);
  vfprintf(stderr, s, ap);
  va_end(ap);
  
  SDL_Quit();
  exit(1);
}

void glerr() {
  GLenum err;
  while ((err = glGetError()) && err!=0) {
    printf("GL ERROR: ");

    switch (err) {
      case GL_INVALID_ENUM: printf("%s", "GL_INVALID_ENUM"); break;
      case GL_INVALID_VALUE: printf("%s", "GL_INVALID_VALUE"); break;
      case GL_INVALID_OPERATION: printf("%s", "GL_INVALID_OPERATION"); break;
      case GL_INVALID_FRAMEBUFFER_OPERATION: printf("%s", "GL_INVALID_FRAMEBUFFER_OPERATION"); break;
      case GL_OUT_OF_MEMORY: printf("%s", "GL_OUT_OF_MEMORY"); break;
    }

    printf("\n");

    #if BUILD_DEBUG
    trace tr = stacktrace();
    print_trace(&tr);
    #endif
    
    errx("stopped ^^^");
  }
}

void sdlerr() {
  const char* err = SDL_GetError();
  if (*err) {
    fprintf(stderr, "SDL: %s\n", err);
    SDL_ClearError();
  }
}