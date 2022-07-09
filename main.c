#include <stdio.h>

#include "chunk.h"

int main() {
  chunk_t chunk;
  chunk_init(&chunk);
  chunk_write(&chunk, OP_RETURN);
  chunk_free(&chunk);
  return 0;
}
