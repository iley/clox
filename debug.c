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
    case OP_CONSTANT: return disasm_constant("OP_CONSTANT", chunk, offset);
    case OP_NIL:      return disasm_simple("OP_NIL", offset);
    case OP_TRUE:     return disasm_simple("OP_TRUE", offset);
    case OP_FALSE:    return disasm_simple("OP_FALSE", offset);
    case OP_NEGATE:   return disasm_simple("OP_NEGATE", offset);
    case OP_ADD:      return disasm_simple("OP_ADD", offset);
    case OP_SUBTRACT: return disasm_simple("OP_SUBTRACT", offset);
    case OP_MULTIPLY: return disasm_simple("OP_MULTIPLY", offset);
    case OP_DIVIDE:   return disasm_simple("OP_DIVIDE", offset);
    case OP_NOT:      return disasm_simple("OP_NOT", offset);
    case OP_EQUAL:    return disasm_simple("OP_EQUAL", offset);
    case OP_GREATER:  return disasm_simple("OP_GREATER", offset);
    case OP_LESS:     return disasm_simple("OP_LESS", offset);
    case OP_PRINT:    return disasm_simple("OP_PRINT", offset);
    case OP_POP:      return disasm_simple("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
      return disasm_constant("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
      return disasm_constant("OP_GET_GLOBAL", chunk, offset);
    case OP_RETURN:   return disasm_simple("OP_RETURN", offset);
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
