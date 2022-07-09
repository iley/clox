#include "memory.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void* reallocate(void* previous, size_t old_size, size_t new_size) {
  (void)old_size; // unused

  if (new_size == 0) {
    free(previous);
    return NULL;
  }

  void* result = realloc(previous, new_size);
  if (result == NULL) {
    printf("out of memory!\n");
    exit(1);
  }

  return result;
}
