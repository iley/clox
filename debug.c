#include "debug.h"

#include <stdio.h>

static int disasm_constant(const char* name, chunk_t* chunk, int offset);
static int disasm_simple(const char* name, int offset);

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
    case OP_CONSTANT:
      return disasm_constant("OP_CONSTANT", chunk, offset);
    case OP_NEGATE:
      return disasm_simple("OP_NEGATE", offset);
    case OP_RETURN:
      return disasm_simple("OP_RETURN", offset);
    default:
      printf("unknown instruction %02x\n", instruction);
      return offset + 1;
  }
}

static int disasm_constant(const char* name, chunk_t* chunk, int offset) {
  uint8_t constant = chunk->code[offset+1];
  printf("%-16s %4d '", name, constant);
  value_print(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int disasm_simple(const char* name, int offset) {
  printf("%s\n", name);
  return offset+1;
}
