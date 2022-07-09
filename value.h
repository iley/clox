#ifndef _VALUE_H
#define _VALUE_H

typedef double value_t;

typedef struct {
  int count;
  int capacity;
  value_t *values;
} value_array_t;

void value_array_init(value_array_t *array);
void value_array_free(value_array_t *array);
void value_array_write(value_array_t *array, value_t value);
void value_print(value_t value);

#endif // _VALUE_H
