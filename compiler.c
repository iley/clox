#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "limits.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

typedef struct {
  token_t previous;
  token_t current;
  bool had_error;
  bool panic_mode;
} parser_t;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY,
} precedence_t;

typedef void (*parse_fn)(bool can_assign);

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence_t precedence;
} parse_rule_t;

typedef struct {
  token_t name;
  int depth;
} local_t;

typedef struct {
  local_t locals[MAX_LOCALS];
  int local_count;
  int scope_depth;
} compiler_t;

// Forward declarations.
static void compiler_init(compiler_t* compiler);
static void advance();
static void consume(token_type_t type, const char* message);
static bool match(token_type_t type);
static bool check(token_type_t type);
static void error_at_current(const char* message);
static void error(const char* message);
static void error_at(token_t* token, const char* message);
static chunk_t* current_chunk();
static void compiler_end();
static void emit_byte(uint8_t byte);
static void emit_bytes(uint8_t byte1, uint8_t byte2);
static void emit_return();
static void emit_constant(value_t value);
static int emit_jump(uint8_t instruction);
static void patch_jump(int offset);
static void emit_loop(int loop_start);
static uint8_t make_constant(value_t value);
static uint8_t identifier_constant(token_t* name);
static uint8_t parse_variable(const char* error_message);
static void define_variable(uint8_t global);
static void declare_variable();
static void mark_initialized();
static void add_local(token_t name);
static int resolve_local(compiler_t* compiler, token_t* name);
static bool identifiers_equal(token_t* first, token_t* second);
static void parse_precedence(precedence_t precedence);
static parse_rule_t* get_rule(token_type_t type);
static void expression();
static void number(bool can_assign);
static void and_(bool can_assign);
static void or_(bool can_assign);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void declaration();
static void statement();
static void print_statement();
static void if_statement();
static void while_statement();
static void for_statement();
static void expression_statement();
static void synchronize();
static void var_declaration();
static void variable(bool can_assign);
static void named_variable(token_t name, bool can_assign);
static void block();
static void scope_begin();
static void scope_end();

parser_t parser;
compiler_t* current = NULL;
chunk_t* compiling_chunk;

parse_rule_t rules[] = {
  [TOKEN_LEFT_PAREN]    = { grouping, NULL,   PREC_NONE },
  [TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE },
  [TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE },
  [TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_MINUS]         = { unary,    binary, PREC_TERM },
  [TOKEN_PLUS]          = { NULL,     binary, PREC_TERM },
  [TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE },
  [TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR },
  [TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
  [TOKEN_BANG]          = { unary,    NULL,   PREC_NONE },
  [TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE },
  [TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
  [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
  [TOKEN_AND]           = { NULL,     and_,   PREC_AND },
  [TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
  [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
  [TOKEN_OR]            = { NULL,     or_,    PREC_OR },
  [TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE },
  [TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE },
  [TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE },
};

bool compile(const char* source, chunk_t* chunk) {
  scanner_init(source);

  compiler_t compiler;
  compiler_init(&compiler);

  compiling_chunk = chunk;
  parser.had_error = false;
  parser.panic_mode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  compiler_end();
  return !parser.had_error;
}

static void compiler_init(compiler_t* compiler) {
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  current = compiler;
}

static void advance() {
  parser.previous = parser.current;
  for (;;) {
    parser.current = scanner_scan_token();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    error_at_current(parser.current.start);
  }
}

static void consume(token_type_t type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static bool match(token_type_t type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static bool check(token_type_t type) {
  return parser.current.type == type;
}

static void error_at_current(const char* message) {
  error_at(&parser.current, message);
}

static void error(const char* message) {
  error_at(&parser.previous, message);
}

static void error_at(token_t* token, const char* message) {
  if (parser.panic_mode) {
    return;
  }
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;

}

static chunk_t* current_chunk() {
  return compiling_chunk;
}

static void compiler_end() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disasm_chunk(current_chunk(), "code");
  }
#endif
}

static void emit_byte(uint8_t byte) {
  chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static void emit_constant(value_t value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->count - 2;
}

static void patch_jump(int offset) {
  // -2 adjusts for bytecode of the jump offset itself
  int jump = current_chunk()->count - offset - 2;
  if (jump > UINT16_MAX) {
    error("too much code to jump over");
  }

  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[offset + 1] = jump & 0xff;
}

static void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);

  int offset = current_chunk()->count - loop_start + 2;
  if (offset > UINT16_MAX) {
    error("loop body too large");
  }

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

static uint8_t identifier_constant(token_t* name) {
  return make_constant(OBJ_VAL(string_copy(name->start, name->length)));
}

static uint8_t make_constant(value_t value) {
  int index = chunk_add_constant(current_chunk(), value);
  if (index > UINT8_MAX) {
    error("too many constants in one chunk");
    return 0;
  }
  return (uint8_t)index;
}

static uint8_t parse_variable(const char* error_message) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable();
  if (current->scope_depth > 0) {
    return 0;
  }

  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }

  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void declare_variable() {
  if (current->scope_depth == 0) {
    return;
  }

  token_t* name = &parser.previous;

  for (int i = current->local_count - 1; i >= 0; i--) {
    local_t* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break;
    }

    if (identifiers_equal(name, &local->name)) {
      error("there's already a variable with the same name in current scope");
    }
  }

  add_local(*name);
}

static void mark_initialized() {
  current->locals[current->local_count - 1].depth = current->scope_depth;
}

static bool identifiers_equal(token_t* first, token_t* second) {
  if (first->length != second->length) {
    return false;
  }
  return memcmp(first->start, second->start, first->length) == 0;
}

static void add_local(token_t name) {
  if (current->local_count == MAX_LOCALS) {
    error("too many local variables\n");
    return;
  }
  local_t* local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
}

static int resolve_local(compiler_t* compiler, token_t* name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    local_t* local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("can't read local variable in its own initializer");
      }
      return i;
    }
  }
  return -1;
}

static void parse_precedence(precedence_t precedence) {
  advance();
  parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("expected expression");
    return;
  }

  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    parse_fn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("invalid assignment target");
  }
}

static parse_rule_t* get_rule(token_type_t type) {
  return &rules[type];
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static void number(bool can_assign) {
  (void)can_assign; // unused
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void and_(bool can_assign) {
  (void)can_assign; // unused
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(end_jump);
}

static void or_(bool can_assign) {
  (void)can_assign; // unused
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);

  patch_jump(else_jump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}

static void grouping(bool can_assign) {
  (void)can_assign; // unused
  expression();
  consume(TOKEN_RIGHT_PAREN, "expect ) after expression");
}

static void unary(bool can_assign) {
  (void)can_assign; // unused
  token_type_t operator_type = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch (operator_type) {
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    case TOKEN_BANG: emit_byte(OP_NOT); break;
    default: return;
  }
}

static void binary(bool can_assign) {
  (void)can_assign; // unused
  token_type_t operator_type = parser.previous.type;
  parse_rule_t* rule = get_rule(operator_type);
  parse_precedence((precedence_t)(rule->precedence + 1));
  switch (operator_type) {
    case TOKEN_PLUS:          emit_byte(OP_ADD); break;
    case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
    case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
    default: return;
  }
}

static void literal(bool can_assign) {
  (void)can_assign; // unused
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    case TOKEN_NIL: emit_byte(OP_NIL); break;
    default: return;
  }
}

static void string(bool can_assign) {
  (void)can_assign; // unused
  emit_constant(OBJ_VAL(string_copy(parser.previous.start + 1, parser.previous.length - 2)));
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    statement();
  }

  if (parser.panic_mode) {
    synchronize();
  }
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    scope_begin();
    block();
    scope_end();
  } else {
    expression_statement();
  }
}

static void print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "expected ; after value in print statement");
  emit_byte(OP_PRINT);
}

static void if_statement() {
  consume(TOKEN_LEFT_PAREN, "expected ( after if");
  expression();
  consume(TOKEN_RIGHT_PAREN, "expected ) after condition in if");

  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();

  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  if (match(TOKEN_ELSE)) {
    statement();
  }
  patch_jump(else_jump);
}

static void while_statement() {
  int loop_start = current_chunk()->count;

  consume(TOKEN_LEFT_PAREN, "expected ( after while");
  expression();
  consume(TOKEN_RIGHT_PAREN, "expected ) after conidtion in while");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

static void for_statement() {
  scope_begin();
  consume(TOKEN_LEFT_PAREN, "expected ( after for");

  if (match(TOKEN_SEMICOLON)) {
    // no initializer
  } else if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    expression_statement();
  }

  int loop_start = current_chunk()->count;
  int exit_jump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "expected ; after loop condition");

    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); // condition
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = current_chunk()->count;
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "expect ) after for clauses");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  scope_end();
}

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "expected ; after expression");
  emit_byte(OP_POP);
}

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }

    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:
        ; // do nothing
    }

    advance();
  }
}

static void var_declaration() {
  uint8_t global = parse_variable("expected variable name after var");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "expected ; after variable declaration");

  define_variable(global);
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void named_variable(token_t name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(current, &name);

  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else {
    arg = identifier_constant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(set_op, (uint8_t)arg);
  } else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "expected } after block");
}

static void scope_begin() {
  current->scope_depth++;
}

static void scope_end() {
  current->scope_depth--;

  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth > current->scope_depth) {
    emit_byte(OP_POP);
    current->local_count--;
  }
}
