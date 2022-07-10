#ifndef _CLOX_OBJECT_H
#define _CLOX_OBJECT_H

#include <stdint.h>

#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_STRING(value)  ((obj_string_t*)AS_OBJ(value))
#define AS_CSTRING(value) (((obj_string_t*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} obj_type_t;

struct obj_t {
  obj_type_t type;
  struct obj_t* next;
};

struct obj_string_t {
  obj_t obj;
  int length;
  uint32_t hash;
  char* chars;
};

static inline bool is_obj_type(value_t value, obj_type_t type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_string_t* string_copy(const char* chars, int length);
obj_string_t* string_take(char* chars, int length);
void object_print(value_t value);

#endif // _CLOX_OBJECT_H
