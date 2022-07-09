#ifndef _CLOX_COMPILER_H
#define _CLOX_COMPILER_H

#include <stdbool.h>

#include "chunk.h"

bool compile(const char* source, chunk_t* chunk);

#endif // _CLOX_COMPILER_H
