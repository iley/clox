#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
  ((type*)object_allocate(sizeof(type), object_type))

static obj_string_t* string_allocate(char* chars, int length);
static obj_t* object_allocate(size_t size, obj_type_t type);
static uint32_t hash_string(const char* str, int length);

obj_string_t* string_copy(const char* chars, int length) {
  char* heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return string_allocate(heap_chars, length);
}

obj_string_t* string_take(char* chars, int length) {
  return string_allocate(chars, length);
}

void string_update_hash(obj_string_t* string) {
  string->hash = hash_string(string->chars, string->length);
}

void object_print(value_t value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}

static obj_string_t* string_allocate(char* chars, int length) {
  obj_string_t* string = ALLOCATE_OBJ(obj_string_t, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

static obj_t* object_allocate(size_t size, obj_type_t type) {
  obj_t* object = (obj_t*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;

  return object;
}

static uint32_t hash_string(const char* str, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)str[i];
    hash *= 16777619;
  }
  return hash;
}
