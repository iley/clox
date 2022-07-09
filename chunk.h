#ifndef _CHUNK_H
#define _CHUNK_H

#include <stdint.h>

typedef enum {
  OP_RETURN,
} opcode_t;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
} chunk_t;

void chunk_init(chunk_t *chunk);
void chunk_free(chunk_t *chunk);
void chunk_write(chunk_t *chunk, uint8_t byte);

#endif // _CHUNK_H
