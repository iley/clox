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

obj_upvalue_t* upvalue_new(value_t* slot) {
  obj_upvalue_t* upvalue = ALLOCATE_OBJ(obj_upvalue_t, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

obj_function_t* function_new() {
  obj_function_t* function = ALLOCATE_OBJ(obj_function_t, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
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

obj_closure_t* closure_new(obj_function_t* function) {
  obj_upvalue_t** upvalues = ALLOCATE(obj_upvalue_t*, function->upvalue_count);
  for (int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }

  obj_closure_t* closure = ALLOCATE_OBJ(obj_closure_t, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

obj_class_t* class_new(obj_string_t* name) {
  obj_class_t* klass = ALLOCATE_OBJ(obj_class_t, OBJ_CLASS);
  klass->name = name;
  return klass;
}

obj_instance_t* instance_new(obj_class_t* klass) {
  obj_instance_t* instance = ALLOCATE_OBJ(obj_instance_t, OBJ_INSTANCE);
  instance->klass = klass;
  table_init(&instance->fields);
  return instance;
}

void object_print(value_t value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
    case OBJ_FUNCTION:
      function_print(AS_FUNCTION(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_CLOSURE:
      function_print(AS_CLOSURE(value)->function);
      break;
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
  }
}

static obj_string_t* string_allocate(char* chars, int length, uint32_t hash) {
  obj_string_t* string = ALLOCATE_OBJ(obj_string_t, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  stack_push(OBJ_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  stack_pop();
  return string;
}

static obj_t* object_allocate(size_t size, obj_type_t type) {
  obj_t* object = (obj_t*)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;

  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif // DEBUG_LOG_GC

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
