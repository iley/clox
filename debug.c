#include "debug.h"

#include <stdio.h>

void disassemble_chunk(chunk_t* chunk, const char* name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    offset = disassemble_instruction(chunk, offset);
  }
}

int disassemble_instruction(chunk_t* chunk, int offset) {
  printf("%04d ", offset);
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_RETURN:
      printf("OP_RETURN\n");
      return offset + 1;
    default:
      printf("unknown instruction %02x\n", instruction);
      return offset + 1;
  }
}
