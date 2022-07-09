#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main() {
  vm_init();

  chunk_t chunk;
  chunk_init(&chunk);

  int const_1 = chunk_add_constant(&chunk, 3.14);
  int line_no = 42;
  chunk_write(&chunk, OP_CONSTANT, line_no);
  chunk_write(&chunk, const_1, line_no);
  chunk_write(&chunk, OP_NEGATE, line_no);
  chunk_write(&chunk, OP_RETURN, line_no);

  disasm_chunk(&chunk, "chunk");

#ifdef DEBUG_TRACE_EXECUTION
  printf("== trace ==\n");
#endif // DEBUG_TRACE_EXECUTION
  execute(&chunk);

  chunk_free(&chunk);

  vm_free();
  return 0;
}
