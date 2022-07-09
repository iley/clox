#include "debug.h"

#include <stdio.h>

static int disasm_constant(chunk_t* chunk, int offset, const char* name);

void disasm_chunk(chunk_t* chunk, const char* name) {
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->count;) {
    offset = disasm_instruction(chunk, offset);
  }
}

int disasm_instruction(chunk_t* chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset-1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_RETURN:
      printf("OP_RETURN\n");
      return offset + 1;
    case OP_CONSTANT:
      return disasm_constant(chunk, offset, "OP_CONSTANT");
    default:
      printf("unknown instruction %02x\n", instruction);
      return offset + 1;
  }
}

static int disasm_constant(chunk_t* chunk, int offset, const char* name) {
  uint8_t constant = chunk->code[offset+1];
  printf("%-16s %4d '", name, constant);
  value_print(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}
