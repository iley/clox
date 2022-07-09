#include <stdio.h>

#include "chunk.h"
#include "debug.h"

int main() {
  chunk_t chunk;
  chunk_init(&chunk);

  int const_1 = chunk_add_constant(&chunk, 1);
  int line_no = 42;
  chunk_write(&chunk, OP_CONSTANT, line_no);
  chunk_write(&chunk, const_1, line_no);
  chunk_write(&chunk, OP_RETURN, line_no);

  disasm_chunk(&chunk, "chunk");
  chunk_free(&chunk);
  return 0;
}
