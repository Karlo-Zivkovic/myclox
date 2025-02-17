#include "scanner.h"
#include "stdbool.h"
#include "stdio.h"
#include <string.h>

typedef struct {
  const char *start;
  const char *current;
  int line;
} Scanner;
Scanner scanner;

void initScanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

bool matchKeyword(const char *keyword, const char *lexeme, int length) {
  if (strlen(keyword) != (size_t)length) {
    return false;
  }
  // Then do character by character comparison
  return strncmp(keyword, lexeme, length) == 0;
}

TokenType keyword(const char *lexeme, int length) {
  switch (lexeme[0]) {
  case 'p':
    if (matchKeyword("print", lexeme, length))
      return TOKEN_PRINT;
    break;
    // ... other cases
  }
  return TOKEN_PRINT;
}

static Token makeToken(TokenType tokenType) {
  Token token;
  token.type = tokenType;
  token.start = scanner.start;
  token.line = scanner.line;
  token.length = (int)(scanner.current - scanner.start);
  return token;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void skipWhitespace() {
  while (scanner.current[0] == ' ') {
    advance();
  }
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;
  char c = advance();

  if (isAlpha(c)) {
    TokenType tokenType = keyword(scanner.start, 5);
    return makeToken(tokenType);
  }
  if (isDigit(c)) {
    return makeToken(TOKEN_NUMBER);
  }

  switch (c) {
  case ';': {
    return makeToken(TOKEN_SEMICOLON);
  }
  }
}
