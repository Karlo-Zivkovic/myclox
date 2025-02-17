#include "compiler.h"
#include "scanner.h"
#include "stdio.h"
#include "vm.h"

typedef struct {
  Token previous;
  Token current;
} Parser;
Parser parser;

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

bool compile(const char *source) {
  initScanner(source);
  advance();
  return true;

  // while (parser.current.type != TOKEN_EOF)
  // {
  // }
}
