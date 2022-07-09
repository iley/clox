#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "debug.h"
#include "value.h"

static vm_t vm;

// Forward declarations.
static execute_result_t vm_run();
static void stack_reset();
static void stack_debug_print();

void vm_init() {
  stack_reset();
}

void vm_free() {
}

execute_result_t execute(const char* source) {
  compile(source);
  (void)vm_run; // unused for now
  return EXECUTE_OK;
}

static execute_result_t vm_run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
  do { \
    double b = stack_pop(); \
    double a = stack_pop(); \
    stack_push(a op b); \
  } while (false)

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
      case OP_NEGATE: stack_push(-stack_pop()); break;
      case OP_ADD: BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_RETURN:
        value_print(stack_pop());
        printf("\n");
        return EXECUTE_OK;
    }
  }

  return EXECUTE_RUNTIME_ERROR;

#undef BINARY_OP
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
