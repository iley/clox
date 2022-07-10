#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
  ((type*)object_allocate(sizeof(type), object_type))

static obj_t* object_allocate(size_t size, obj_type_t type);

obj_string_t* string_allocate(int length) {
  obj_string_t* string = (obj_string_t*)object_allocate(sizeof(obj_string_t) + length + 1, OBJ_STRING);
  string->length = length;
  return string;
}

obj_string_t* string_copy(const char* chars, int length) {
  obj_string_t* string = string_allocate(length);
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  return string;
}

void object_print(value_t value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}

static obj_t* object_allocate(size_t size, obj_type_t type) {
  obj_t* object = (obj_t*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;

  return object;
}
