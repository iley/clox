#ifndef _CLOX_VM_H
#define _CLOX_VM_H

#include <stdint.h>

#include "chunk.h"

typedef struct {
  chunk_t* chunk;
  uint8_t* ip;
} vm_t;

typedef enum {
  EXECUTE_OK,
  EXECUTE_COMPILE_ERROR,
  EXECUTE_RUNTIME_ERROR,
} execute_result_t;

void vm_init();
void vm_free();
execute_result_t execute(chunk_t* chunk);

#endif // _CLOX_VM_H
