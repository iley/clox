#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "debug.h"
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

typedef void (*parse_fn)();

typedef struct {
  parse_fn prefix;
  parse_fn infix;
  precedence_t precedence;
} parse_rule_t;

// Forward declarations.
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
static uint8_t make_constant(value_t value);
static uint8_t identifier_constant(token_t* name);
static uint8_t parse_variable(const char* error_message);
static void define_variable(uint8_t global);
static void parse_precedence(precedence_t precedence);
static parse_rule_t* get_rule(token_type_t type);
static void expression();
static void number();
static void grouping();
static void unary();
static void binary();
static void literal();
static void string();
static void declaration();
static void statement();
static void print_statement();
static void expression_statement();
static void synchronize();
static void var_declaration();

parser_t parser;
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
  [TOKEN_IDENTIFIER]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
  [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
  [TOKEN_AND]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
  [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
  [TOKEN_OR]            = { NULL,     NULL,   PREC_NONE },
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
  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void parse_precedence(precedence_t precedence) {
  advance();
  parse_fn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("expected expression");
    return;
  }

  prefix_rule();

  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    parse_fn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule();
  }
}

static parse_rule_t* get_rule(token_type_t type) {
  return &rules[type];
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "expect ) after expression");
}

static void unary() {
  token_type_t operator_type = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch (operator_type) {
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    case TOKEN_BANG: emit_byte(OP_NOT); break;
    default: return;
  }
}

static void binary() {
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

static void literal() {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    case TOKEN_NIL: emit_byte(OP_NIL); break;
    default: return;
  }
}

static void string() {
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
  } else {
    expression_statement();
  }
}

static void print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "expected ; after value in print statement");
  emit_byte(OP_PRINT);
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
