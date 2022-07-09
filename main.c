#include <stdio.h>

#include "chunk.h"
#include "debug.h"

int main() {
  chunk_t chunk;
  chunk_init(&chunk);
  chunk_write(&chunk, OP_RETURN);
  disassemble_chunk(&chunk, "chunk");
  chunk_free(&chunk);
  return 0;
}
