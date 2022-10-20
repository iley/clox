#ifndef _CLOX_CHUNK_H
#define _CLOX_CHUNK_H

#include <stdint.h>

#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_NOT,
  OP_PRINT,
  OP_POP,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_RETURN,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_CLASS,
  OP_SET_PROPERTY,
  OP_GET_PROPERTY,
  OP_METHOD,
  OP_INVOKE,
} opcode_t;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  int* lines;
  value_array_t constants;
} chunk_t;

void chunk_init(chunk_t *chunk);
void chunk_free(chunk_t *chunk);
void chunk_write(chunk_t *chunk, uint8_t byte, int line);
int chunk_add_constant(chunk_t *chunk, value_t value);

#endif // _CLOX_CHUNK_H
