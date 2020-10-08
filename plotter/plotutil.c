#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <limits.h>

#include <SDL.h>

#include "gl.h"

#include <cglm/cglm.h>

#include "util.h"

#define GLERR glerr(__LINE__, __FILE__)

int vec3_iszero(vec3 vec) {
  return vec[0] == 0 && vec[1] == 0 && vec[2] == 0;
}

uint64_t vec3_hash(vec3 vec) {
	//fit three floats into 64 bits by normalizing to int (discarding negative exponents)
	return (uint64_t)vec[0] + (uint64_t)vec[1]*FLT_MAX + (uint64_t)vec[2]*FLT_MAX;
}

void errx(char* s, ...) {
  va_list ap;

	va_start(ap, s);
  vfprintf(stderr, s, ap);
  va_end(ap);
  
  SDL_Quit();
  exit(1);
}

void glerr(int line, char* file) {
  GLenum err;
  while ((err = glGetError())) {
    printf("GL ERROR: %s:%i\n", file, line);

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
