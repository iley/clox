#include "value.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "memory.h"

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
  printf("%g", value);
}
