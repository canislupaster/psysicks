// Automatically generated header.

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <SDL.h>
#include "gl.h"
#include <cglm/cglm.h>
#include "util.h"
#define GLERR glerr(__LINE__, __FILE__)
int vec3_iszero(vec3 vec);
uint64_t vec3_hash(vec3 vec);
void errx(char* s, ...);
void glerr(int line, char* file);
void sdlerr();
