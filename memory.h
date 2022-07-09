#ifndef _MEMORY_H
#define _MEMORY_H

#include <stddef.h>

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
  (type*)reallocate((pointer), sizeof(type) * (old_count), sizeof(type) * (new_count))

void* reallocate(void* pointer, size_t old_size, size_t new_size);

#endif // _MEMORY_H
