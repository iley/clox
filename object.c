#include "object.h"

#include <stdio.h>
#include <string.h>

#include "table.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
  ((type*)object_allocate(sizeof(type), object_type))

static obj_string_t* string_allocate(char* chars, int length, uint32_t hash);
static obj_t* object_allocate(size_t size, obj_type_t type);
static uint32_t hash_string(const char* str, int length);
static void function_print(obj_function_t* function);

obj_string_t* string_copy(const char* chars, int length) {
  uint32_t hash = hash_string(chars, length);

  obj_string_t* interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }

  char* heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return string_allocate(heap_chars, length, hash);
}

obj_string_t* string_take(char* chars, int length) {
  uint32_t hash = hash_string(chars, length);

  obj_string_t* interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return string_allocate(chars, length, hash);
}

obj_function_t* function_new() {
  obj_function_t* function = ALLOCATE_OBJ(obj_function_t, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  chunk_init(&function->chunk);
  return function;
}

obj_native_t* native_new(native_fn_t function, int arity) {
  obj_native_t* native = ALLOCATE_OBJ(obj_native_t, OBJ_NATIVE);
  native->function = function;
  native->arity = arity;
  return native;
}

void object_print(value_t value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_FUNCTION:
      function_print(AS_FUNCTION(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
  }
}

static obj_string_t* string_allocate(char* chars, int length, uint32_t hash) {
  obj_string_t* string = ALLOCATE_OBJ(obj_string_t, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  table_set(&vm.strings, string, NIL_VAL);
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

static void function_print(obj_function_t* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}
