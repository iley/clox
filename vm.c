#include "vm.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static vm_t vm;

// Forward declarations.
static execute_result_t vm_run();
static void stack_reset();
static void stack_debug_print();
static value_t stack_peek(int distance);
static void runtime_error(const char* format, ...);

void vm_init() {
  stack_reset();
}

void vm_free() {
}

execute_result_t execute(const char* source) {
  chunk_t chunk;
  chunk_init(&chunk);
  if (!compile(source, &chunk)) {
    return EXECUTE_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  execute_result_t result = vm_run();

  chunk_free(&chunk);
  return result;
}

static execute_result_t vm_run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(value_type, op) \
  do { \
    if (!IS_NUMBER(stack_peek(0)) || !IS_NUMBER(stack_peek(1))) { \
      runtime_error("operands must be numbers"); \
      return EXECUTE_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(stack_pop()); \
    double a = AS_NUMBER(stack_pop()); \
    stack_push(value_type(a op b)); \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    stack_debug_print();
    disasm_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#else
    (void)stack_debug_print; // unused
#endif // DEBUG_TRACE_EXECUTION

    uint8_t instruction = READ_BYTE();
    switch (instruction) {
      case OP_CONSTANT: {
        value_t constant = READ_CONSTANT();
        stack_push(constant);
        break;
      }
      case OP_NIL: stack_push(NIL_VAL); break;
      case OP_TRUE: stack_push(BOOL_VAL(true)); break;
      case OP_FALSE: stack_push(BOOL_VAL(false)); break;
      case OP_NEGATE:
        if (!IS_NUMBER(stack_peek(0))) {
          runtime_error("operand must be a number");
          return EXECUTE_RUNTIME_ERROR;
        }
        stack_push(NUMBER_VAL(-AS_NUMBER(stack_pop())));
        break;
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
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

static value_t stack_peek(int distance) {
  return vm.stack_top[-1 - distance];
}

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  stack_reset();
}
