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
static bool call_value(value_t callee, int arg_count);
static bool call(obj_function_t* function, int arg_count);

void vm_init() {
  stack_reset();
  vm.objects = NULL;
  table_init(&vm.globals);
  table_init(&vm.strings);
}

void vm_free() {
  table_free(&vm.strings);
  table_free(&vm.globals);
  free_objects();
}

execute_result_t execute(const char* source) {
  obj_function_t* function = compile(source);
  if (function == NULL) {
    return EXECUTE_COMPILE_ERROR;
  }

  stack_push(OBJ_VAL(function));
  call(function, 0);

  return vm_run();
}

static execute_result_t vm_run() {
  call_frame_t* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
    disasm_instruction(&frame->function->chunk, (int)(frame->ip - vm.chunk->code));
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
      case OP_PRINT:
        value_print(stack_pop());
        printf("\n");
        break;
      case OP_POP:
        stack_pop();
        break;
      case OP_DEFINE_GLOBAL: {
        obj_string_t* name = READ_STRING();
        table_set(&vm.globals, name, stack_peek(0));
        stack_pop();
        break;
      }
      case OP_GET_GLOBAL: {
        obj_string_t* name = READ_STRING();
        value_t value;
        if (!table_get(&vm.globals, name, &value)) {
          runtime_error("undefined variable %s", name->chars);
          return EXECUTE_RUNTIME_ERROR;
        }
        stack_push(value);
        break;
      }
      case OP_SET_GLOBAL: {
        obj_string_t* name = READ_STRING();
        if (table_set(&vm.globals, name, stack_peek(0))) {
          table_delete(&vm.globals, name);
          runtime_error("undefined variable %s", name->chars);
          return EXECUTE_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        stack_push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = stack_peek(0);
        break;
      }
      case OP_RETURN: {
        value_t result = stack_pop();
        vm.frame_count--;
        if (vm.frame_count == 0) {
          stack_pop();
          return EXECUTE_OK;
        }

        vm.stack_top = frame->slots;
        stack_push(result);
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(stack_peek(0))) {
          frame->ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        int arg_count = READ_BYTE();
        if (!call_value(stack_peek(arg_count), arg_count)) {
          return EXECUTE_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
    }
  }

  return EXECUTE_RUNTIME_ERROR;

#undef BINARY_OP
#undef READ_STRING
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_BYTE
}

static void stack_reset() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
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

  for (int i = vm.frame_count - 1; i >= 0; i--) {
    call_frame_t* frame = &vm.frames[i];
    obj_function_t* function = frame->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  call_frame_t* frame = &vm.frames[vm.frame_count - 1];
  size_t instruction = frame->ip - frame->function->chunk.code - 1;
  int line = frame->function->chunk.lines[instruction];
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

static bool call_value(value_t callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), arg_count);
      default:
        break;
    }
  }
  runtime_error("can only call functions and classes");
  return false;
}

static bool call(obj_function_t* function, int arg_count) {
  if (arg_count != function->arity) {
    runtime_error("expected %d arguments but got %d", function->arity, arg_count);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("stack overflow");
    return false;
  }

  call_frame_t* frame = &vm.frames[vm.frame_count++];
  frame->function = function;
  frame->ip = function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;

  return true;
}
