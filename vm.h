#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include <stdint.h>

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
  chunk_t* chunk;
  uint8_t* ip;
  value_t stack[STACK_MAX];
  value_t* stack_top;
  table_t strings;
  obj_t* objects;
} vm_t;

typedef enum {
  EXECUTE_OK,
  EXECUTE_COMPILE_ERROR,
  EXECUTE_RUNTIME_ERROR,
} execute_result_t;

extern vm_t vm;

void vm_init();
void vm_free();
execute_result_t execute(const char* source);
void stack_push(value_t value);
value_t stack_pop();

#endif // _CLOX_VM_H
