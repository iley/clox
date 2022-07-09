#include "vm.h"

#include <stdio.h>

#include "value.h"

static vm_t vm;

static execute_result_t run();

void vm_init() {
  (void)vm; // unused
}

void vm_free() {
}

execute_result_t execute(chunk_t* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}

static execute_result_t run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  for (;;) {
    uint8_t instruction = READ_BYTE();
    switch (instruction) {
      case OP_RETURN:
        return EXECUTE_OK;
      case OP_CONSTANT: {
        value_t constant = READ_CONSTANT();
        value_print(constant);
        printf("\n");
        break;
      }
    }
  }

  return EXECUTE_RUNTIME_ERROR;

#undef READ_CONSTANT
#undef READ_BYTE
}
