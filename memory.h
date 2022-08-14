#ifndef _CLOX_MEMORY_H
#define _CLOX_MEMORY_H

#include <stddef.h>

#include "object.h"
#include "value.h"

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
  (type*)reallocate((pointer), sizeof(type) * (old_count), sizeof(type) * (new_count))

#define ALLOCATE(type, count) \
  (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) \
  reallocate(pointer, sizeof(type), 0)
#define FREE_ARRAY(type, pointer, old_count) \
  reallocate((pointer), sizeof(type) * (old_count), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);
void mark_value(value_t value);
void mark_object(obj_t* object);
void collect_garbage();
void free_objects();

#endif // _CLOX_MEMORY_H
