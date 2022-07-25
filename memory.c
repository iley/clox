#include "memory.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "object.h"
#include "vm.h"

static void free_object(obj_t* object);

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

void free_objects() {
  for (obj_t* object = vm.objects; object != NULL; ) {
    obj_t* next = object->next;
    free_object(object);
    object = next;
  }
}

void free_object(obj_t* object) {
  switch (object->type) {
    case OBJ_STRING: {
      obj_string_t* string = (obj_string_t*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(obj_string_t, object);
      break;
    }
    case OBJ_UPVALUE:
      FREE(obj_upvalue_t, object);
      break;
    case OBJ_FUNCTION: {
      obj_function_t* function = (obj_function_t*)object;
      chunk_free(&function->chunk);
      FREE(obj_function_t, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(obj_native_t, object);
      break;
    case OBJ_CLOSURE: {
      obj_closure_t* closure = (obj_closure_t*)object;
      FREE_ARRAY(obj_upvalue_t*, closure->upvalues, closure->upvalue_count);
      FREE(obj_closure_t, object);
      break;
    }
  }
}
