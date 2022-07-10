#include "vm.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

vm_t vm;

// Forward declarations.
static execute_result_t vm_run();
static void stack_reset();
static void stack_debug_print();
static value_t stack_peek(int distance);
static void runtime_error(const char* format, ...);
static bool is_falsey(value_t value);
static void concatenate_strings();

void vm_init() {
  stack_reset();
  vm.objects = NULL;
  table_init(&vm.strings);
}

void vm_free() {
  table_free(&vm.strings);
  free_objects();
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
      case OP_EQUAL: {
        value_t b = stack_pop();
        value_t a = stack_pop(); 
        stack_push(BOOL_VAL(values_equal(a, b)));
        break;
      }
      case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:    BINARY_OP(BOOL_VAL, <); break;
      case OP_NEGATE:
        if (!IS_NUMBER(stack_peek(0))) {
          runtime_error("operand must be a number");
          return EXECUTE_RUNTIME_ERROR;
        }
        stack_push(NUMBER_VAL(-AS_NUMBER(stack_pop())));
        break;
      case OP_ADD:
        if (IS_STRING(stack_peek(0)) && IS_STRING(stack_peek(1))) {
          concatenate_strings();
        } else if (IS_NUMBER(stack_peek(0)) && IS_NUMBER(stack_peek(1))) {
          BINARY_OP(NUMBER_VAL, +);
        } else {
          runtime_error("operands of + must be two numbers or two strings");
          return EXECUTE_RUNTIME_ERROR;
        }
        break;
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        stack_push(BOOL_VAL(is_falsey(stack_pop())));
        break;
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

static bool is_falsey(value_t value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate_strings() {
  obj_string_t* second = AS_STRING(stack_pop());
  obj_string_t* first = AS_STRING(stack_pop());

  int length = first->length + second->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, first->chars, first->length);
  memcpy(chars + first->length, second->chars, second->length);
  chars[length] = '\0';

  obj_string_t* result = string_take(chars, length);
  stack_push(OBJ_VAL(result));
}
