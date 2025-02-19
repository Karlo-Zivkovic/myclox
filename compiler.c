#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "stdio.h"
#include "value.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token previous;
  Token current;
} Parser;
Parser parser;

Chunk *currentChunk;

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk, byte, parser.previous.line);
}

static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void number();

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static uint8_t emitConstant(Value value) {
  int index = writeValueArray(&currentChunk->constants, value);
  return index;
}

static void consume(TokenType type) {
  if (parser.current.type == type) {
    advance();
    return;
  }
}

static void printStatement() {
  expression();
  emitByte(OP_PRINT);
  consume(TOKEN_SEMICOLON);
}

static void statement() {
  if (parser.current.type == TOKEN_PRINT) {
    advance();
    printStatement();
  }
}

ParseRule rules[] = {[TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
                     [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
                     [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
                     [TOKEN_EOF] = {NULL, NULL, PREC_NONE}};

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn parseFn = getRule(parser.previous.type)->prefix;
  parseFn();
  while (precedence < getRule(parser.current.type)->precedence) {
    advance();
    ParseFn parseFn = getRule(parser.previous.type)->infix;
    parseFn();
  }
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitByte(OP_CONSTANT);
  emitByte(emitConstant(makeNumber(value)));
}

bool compile(const char *source, Chunk *chunk) {
  currentChunk = chunk;
  initScanner(source);
  advance();

  while (parser.current.type != TOKEN_EOF) {
    statement();
  }
  emitByte(OP_RETURN);
  return true;
}
