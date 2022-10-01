#ifndef _CLOX_OBJECT_H
#define _CLOX_OBJECT_H

#include <stdint.h>

#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_CLASS(value) is_obj_type(value, OBJ_CLASS)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_UPVALUE(VALUE) is_obj_type(value, OBJ_UPVALUE)

#define AS_STRING(value)  ((obj_string_t*)AS_OBJ(value))
#define AS_CSTRING(value) (((obj_string_t*)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((obj_function_t*)AS_OBJ(value))
#define AS_NATIVE(value) ((obj_native_t*)AS_OBJ(value))
#define AS_CLOSURE(value) ((obj_closure_t*)AS_OBJ(value))
#define AS_CLASS(value) ((obj_class_t*)AS_OBJ(value))
#define AS_INSTANCE(value) ((obj_instance_t*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((obj_bound_method_t*)AS_OBJ(value))
#define AS_UPVALUE(value) ((obj_upvalue_t*)AS_OBJ(value))

typedef enum {
  OBJ_STRING,
  OBJ_UPVALUE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_CLASS,
  OBJ_INSTANCE,
  OBJ_BOUND_METHOD,
} obj_type_t;

struct obj_t {
  obj_type_t type;
  bool is_marked;
  struct obj_t* next;
};

struct obj_string_t {
  obj_t obj;
  int length;
  uint32_t hash;
  char* chars;
};

typedef struct obj_upvalue_t {
  obj_t obj;
  value_t closed;
  value_t* location;
  struct obj_upvalue_t* next;
} obj_upvalue_t;

typedef struct {
  obj_t obj;
  int arity;
  int upvalue_count;
  chunk_t chunk;
  obj_string_t* name;
} obj_function_t;

typedef value_t (*native_fn_t)(int arg_count, value_t* args);

typedef struct {
  obj_t obj;
  native_fn_t function;
  int arity;
} obj_native_t;

typedef struct {
  obj_t obj;
  obj_function_t* function;
  obj_upvalue_t** upvalues;
  int upvalue_count;
} obj_closure_t;

typedef struct {
  obj_t obj;
  obj_string_t* name;
  table_t methods;
} obj_class_t;

typedef struct {
  obj_t obj;
  obj_class_t* klass;
  table_t fields;
} obj_instance_t;

typedef struct {
  obj_t obj;
  value_t receiver;
  obj_closure_t* method;
} obj_bound_method_t;

static inline bool is_obj_type(value_t value, obj_type_t type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_string_t* string_copy(const char* chars, int length);
obj_string_t* string_take(char* chars, int length);
obj_upvalue_t* upvalue_new(value_t* slot);
obj_function_t* function_new();
obj_native_t* native_new(native_fn_t function, int arity);
obj_closure_t* closure_new(obj_function_t* function);
obj_class_t* class_new(obj_string_t* name);
obj_instance_t* instance_new(obj_class_t* klass);
obj_bound_method_t* bound_method_new(value_t receiver, obj_closure_t* method);
void object_print(value_t value);

#endif // _CLOX_OBJECT_H
