#ifndef _CLOX_COMPILER_H
#define _CLOX_COMPILER_H

#include <stdbool.h>

#include "chunk.h"
#include "object.h"

obj_function_t* compile(const char* source);
void mark_compiler_roots();

#endif // _CLOX_COMPILER_H
