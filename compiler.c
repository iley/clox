#include "compiler.h"

#include <stdio.h>

#include "scanner.h"

void compile(const char* source) {
  scanner_init(source);
  int line = -1;
  for (;;) {
    token_t token = scanner_scan_token();
    if (token.line == line) {
      printf("   | ");
    } else {
      printf("%4d ", token.line);
      line = token.line;
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if (token.type == TOKEN_EOF) {
      break;
    }
  }
}
