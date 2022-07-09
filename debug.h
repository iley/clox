#ifndef _DEBUG_H
#define _DEBUG_H

#include "chunk.h"

void disasm_chunk(chunk_t *chunk, const char* name);
int disasm_instruction(chunk_t *chunk, int offset);

#endif // _DEBUG_H
