#include "chunk.h"

#include <stddef.h>
#include <stdlib.h>

#include "memory.h"

void chunk_init(chunk_t* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
}

void chunk_free(chunk_t* chunk) {
  free(chunk->code);
  chunk_init(chunk);
}

void chunk_write(chunk_t* chunk, uint8_t byte) {
  if (chunk->count + 1 > chunk->capacity) {
    int old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->count++;
}
