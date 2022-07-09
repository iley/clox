#include "vm.h"

#include <assert.h>
#include <stdio.h>

#include "debug.h"
#include "value.h"

static vm_t vm;

// Forward declarations.
static execute_result_t run();
static void stack_reset();
static void stack_debug_print();

void vm_init() {
  stack_reset();
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
#ifdef DEBUG_TRACE_EXECUTION
    stack_debug_print();
    disasm_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION

    uint8_t instruction = READ_BYTE();
    switch (instruction) {
      case OP_CONSTANT: {
        value_t constant = READ_CONSTANT();
        stack_push(constant);
        break;
      }
      case OP_NEGATE:
        stack_push(-stack_pop());
        break;
      case OP_RETURN:
        value_print(stack_pop());
        printf("\n");
        return EXECUTE_OK;
    }
  }

  return EXECUTE_RUNTIME_ERROR;

#undef READ_CONSTANT
#undef READ_BYTE
}

static void stack_reset() {
  vm.stack_top = vm.stack;
}

void stack_push(value_t value) {
  assert(vm.stack_top - vm.stack < STACK_MAX);
  *vm.stack_top = value;
  vm.stack_top++;
}

value_t stack_pop() {
  assert(vm.stack_top > vm.stack);
  vm.stack_top--;
  return *vm.stack_top;
}

static void stack_debug_print() {
  printf("          ");
  for (value_t* slot = vm.stack; slot < vm.stack_top; slot++) {
    printf("[ ");
    value_print(*slot);
    printf(" ]");
  }
  printf("\n");
}
