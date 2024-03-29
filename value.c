#include "value.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

void value_array_init(value_array_t* array) {
  array->count = 0;
  array->capacity = 0;
  array->values = NULL;
}

void value_array_free(value_array_t* array) {
  free(array->values);
  value_array_init(array);
}

void value_array_write(value_array_t* array, value_t value) {
  if (array->count + 1 > array->capacity) {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values = GROW_ARRAY(value_t, array->values, old_capacity, array->capacity);
  }
  array->values[array->count] = value;
  array->count++;
}

void value_print(value_t value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    object_print(value);
  }
#else
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      printf("nil");
      break;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(value));
      break;
    case VAL_OBJ:
      object_print(value);
      break;
  }
#endif // NAN_BOXING
}

bool values_equal(value_t first, value_t second) {
#ifdef NAN_BOXING
  if (IS_NUMBER(first) && IS_NUMBER(second)) {
    return AS_NUMBER(first) == AS_NUMBER(second);
  }
  return first == second;
#else
  if (first.type != second.type) {
    return false;
  }
  switch (first.type) {
    case VAL_BOOL:   return AS_BOOL(first) == AS_BOOL(second);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(first) == AS_NUMBER(second);
    case VAL_OBJ:    return AS_OBJ(first) == AS_OBJ(second);
    default:         return false;  // unreachable
  }
#endif // NAN_BOXING
}
