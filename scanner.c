#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>
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

static bool isAtEnd() { return *scanner.current == '\0'; }
static char peek() { return *scanner.current; }
static char peekNext() { return *(scanner.current + 1); }

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
// static char peekNext() { return scanner.current[1]; }

bool matchKeyword(const char *keyword, const char *lexeme, int length) {
  if (strlen(keyword) != (size_t)length) {
    return false;
  }
  scanner.current = scanner.current + length - 1;
  // Then do character by character comparison
  return strncmp(keyword, lexeme, length) == 0;
}

TokenType keyword(const char *lexeme) {
  switch (lexeme[0]) {
  case 'w': {
    if (matchKeyword("while", lexeme, 5))
      return TOKEN_WHILE;
    break;
  }
  case 'e': {
    if (matchKeyword("else", lexeme, 4))
      return TOKEN_ELSE;
    break;
  }
  case 'i': {
    if (matchKeyword("if", lexeme, 2))
      return TOKEN_IF;
    break;
  }
  case 'v':
    if (matchKeyword("var", lexeme, 3))
      return TOKEN_VAR;
    break;
  case 'p':
    if (matchKeyword("print", lexeme, 5))
      return TOKEN_PRINT;
    break;
  case 't':
    if (matchKeyword("true", lexeme, 4))
      return TOKEN_TRUE;
    break;
  case 'f':
    if (matchKeyword("false", lexeme, 5))
      return TOKEN_FALSE;
    break;
  }

  // it has to be identifier
  while (isAlpha(peek())) {
    advance();
  }
  return TOKEN_IDENTIFIER;
}

static Token makeToken(TokenType tokenType) {
  Token token;
  token.type = tokenType;
  token.start = scanner.start;
  token.line = scanner.line;
  token.length = (int)(scanner.current - scanner.start);
  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t': {
      advance();
      break;
    }
    case '\n': {
      scanner.line++;
      advance();
      break;
    }
    case '/': {
      if (peekNext() == '/') {
        while (peek() != '\n') {
          advance();
        }
        advance();
      }
      break;
    }
    default: {
      return;
    }
    }
  }
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.line = scanner.line;
  token.length = strlen(message);
  return token;
}

Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }
  if (isAtEnd()) {
    return errorToken("Unterminated string");
  }
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;
  char c = advance();

  if (isAlpha(c)) {
    TokenType tokenType = keyword(scanner.start);
    Token token = makeToken(tokenType);

    return token;
    // return makeToken(tokenType);
  }
  if (isDigit(c)) {
    return makeToken(TOKEN_NUMBER);
  }

  switch (c) {
  case '(': {
    return makeToken(TOKEN_LEFT_PAREN);
  }
  case ')': {
    return makeToken(TOKEN_RIGHT_PAREN);
  }
  case '{': {
    return makeToken(TOKEN_LEFT_BRACE);
  }
  case '}': {
    return makeToken(TOKEN_RIGHT_BRACE);
  }
  case ';': {
    return makeToken(TOKEN_SEMICOLON);
  }
  case '\0': {
    return makeToken(TOKEN_EOF);
  }
  case '=': {
    return makeToken(TOKEN_EQUAL);
  }
  case '"': {
    return string();
  }
  case '<': {
    return makeToken(TOKEN_LESS);
  }
  case '>': {
    return makeToken(TOKEN_GREATER);
  }
  case '+': {
    return makeToken(TOKEN_PLUS);
  }
  case '|': {
    if (peek() == '|') {
      advance();
      return makeToken(TOKEN_OR);
    }
    break;
  }
  case '&': {
    if (peek() == '&') {
      advance();
      return makeToken(TOKEN_AND);
    }
    break;
  }
  case '!': {
    if (peek() == '=') {
      advance();
      return makeToken(TOKEN_BANG_EQUAL);
    }
    return makeToken(TOKEN_BANG);
  }
  }
  return errorToken("Unexpected character.");
}
