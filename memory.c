#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG_LOG_GC
#include "debug.h"
#endif

#include "compiler.h"
#include "object.h"
#include "table.h"
#include "vm.h"

#define GC_HEAP_GROW_FACTOR 2

static void free_object(obj_t* object);
static void mark_roots();
static void trace_references();
static void sweep();
static void blacken_object(obj_t* object);
static void mark_array(value_array_t* array);

void* reallocate(void* previous, size_t old_size, size_t new_size) {
  (void)old_size; // unused
  vm.bytes_allocated += new_size - old_size;

#ifdef DEBUG_STRESS_GC
  if (new_size > old_size) {
    collect_garbage();
  }
#endif // DEBUG_STRESS_GC

  if (vm.bytes_allocated > vm.next_gc) {
    collect_garbage();
  }

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

void mark_value(value_t value) {
  if (IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

void mark_object(obj_t* object) {
  if (object == NULL) {
    return;
  }
  if (object->is_marked) {
    return;
  }
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  value_print(OBJ_VAL(object));
  printf("\n");
#endif // DEBUG_LOG_GC
  object->is_marked = true;

  if (vm.gray_capacity < vm.gray_count + 1) {
    vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
    vm.gray_stack = (obj_t**)realloc(vm.gray_stack, sizeof(obj_t*) * vm.gray_capacity);

    if (vm.gray_stack == NULL) {
      exit(1);
    }
  }

  vm.gray_stack[vm.gray_count++] = object;
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytes_allocated;
#endif // DEBUG_LOG_GC

  mark_roots();
  trace_references();
  table_remove_white(&vm.strings);
  sweep();

  vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
      before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif // DEBUG_LOG_GC
}

void free_objects() {
  for (obj_t* object = vm.objects; object != NULL; ) {
    obj_t* next = object->next;
    free_object(object);
    object = next;
  }

  free(vm.gray_stack);
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
    case OBJ_CLASS: {
      obj_class_t* klass = (obj_class_t*)object;
      table_free(&klass->methods);
      FREE(obj_class_t, object);
      break;
    }
    case OBJ_INSTANCE: {
      obj_instance_t* instance = (obj_instance_t*)object;
      table_free(&instance->fields);
      FREE(obj_instance_t, object);
      break;
    }
    case OBJ_BOUND_METHOD:
      FREE(obj_bound_method_t, object);
      break;
  }
}

static void mark_roots() {
  for (value_t* slot = vm.stack; slot < vm.stack_top; slot++) {
    mark_value(*slot);
  }

  for (int i = 0; i < vm.frame_count; i++) {
    mark_object((obj_t*)vm.frames[i].closure);
  }

  for (obj_upvalue_t* upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
    mark_object((obj_t*)upvalue);
  }

  mark_table(&vm.globals);
  mark_compiler_roots();
}

static void trace_references() {
  while (vm.gray_count > 0) {
    obj_t* object = vm.gray_stack[--vm.gray_count];
    blacken_object(object);
  }
}

static void sweep() {
  obj_t* previous = NULL;
  obj_t* object = vm.objects;
  while (object != NULL) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next;
    } else {
      obj_t* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }
      free_object(unreached);
    }
  }
}

static void blacken_object(obj_t* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  value_print(OBJ_VAL(object));
  printf("\n");
#endif // DEBUG_LOG_GC
  switch (object->type) {
    case OBJ_CLASS: {
      obj_class_t* klass = (obj_class_t*)object;
      mark_object((obj_t*)klass->name);
      mark_table(&klass->methods);
      break;
    }
    case OBJ_INSTANCE: {
      obj_instance_t* instance = (obj_instance_t*)object;
      mark_object((obj_t*)instance->klass);
      mark_table(&instance->fields);
      break;
    }
    case OBJ_BOUND_METHOD: {
      obj_bound_method_t* bound = (obj_bound_method_t*)object;
      mark_value(bound->receiver);
      mark_object((obj_t*)bound->method);
      break;
    }
    case OBJ_CLOSURE: {
      obj_closure_t* closure = (obj_closure_t*)object;
      mark_object((obj_t*)closure->function);
      for (int i = 0; i < closure->upvalue_count; i++) {
        mark_object((obj_t*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      obj_function_t* function = (obj_function_t*)object;
      mark_object((obj_t*)function->name);
      mark_array(&function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      mark_value(((obj_upvalue_t*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

static void mark_array(value_array_t* array) {
  for (int i = 0; i < array->count; i++) {
    mark_value(array->values[i]);
  }
}
