// Automatically generated header.

#pragma once
#include <SDL2/SDL.h>
#include "gl.h"
#include <stdlib.h>
#include <stdio.h>
#include <cglm/cglm.h>
#include "util.h"
#define GLERR glerr(__LINE__, __FILE__)
int vec3_iszero(vec3 vec);
void errx(char* s, ...);
void glerr(int line, char* file);
void sdlerr();
