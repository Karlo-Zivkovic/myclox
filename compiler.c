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

typedef struct {
  Token token;
  int depth;
} Local;

typedef struct {
  Local locals[256];
  int localCount;
  int currentScopeDepth;
} Compiler;

Compiler compiler;

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

static void errorAt(Token *token, const char *message) {
  fprintf(stderr, "[Line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": '%s\n'", message);
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk, byte, parser.previous.line);
}

static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void string();
static void number();
static void variable();
static void binary();

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static uint8_t emitConstant(Value value) {
  uint8_t index = writeValueArray(&currentChunk->constants, value);
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

void addLocal(Token token) {
  if (compiler.localCount >= 256) {
    // TODO: write error function
    // error("Too many local variables in function.");
    return;
  }
  Local *local = &compiler.locals[compiler.localCount];
  local->token = token;
  local->depth = compiler.currentScopeDepth;
  compiler.localCount++;
}

static void varStatement() {
  advance();
  uint8_t globalIndex =
      emitConstant(makeString(parser.previous.start, parser.previous.length));
  // FIX: Supposed to do global variable not local
  // addLocal(parser.previous);
  consume(TOKEN_EQUAL);
  expression();
  emitByte(OP_DEFINE_GLOBAL);
  emitByte(globalIndex);
  advance();
}

static void expressionStatement() {
  // advance();
  /* uint8_t globalIndex = */
  /*     emitConstant(makeString(parser.previous.start,
   * parser.previous.length)); */
  expression();
  /* emitByte(OP_DEFINE_GLOBAL); */
  /* emitByte(globalIndex); */
  ///
  advance();
}

static void statement() {
  if (parser.current.type == TOKEN_PRINT) {
    advance();
    printStatement();
  } else if (parser.current.type == TOKEN_VAR) {
    advance();
    varStatement();
  } else {
    expressionStatement();
  }
}

ParseRule rules[] = {[TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
                     [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
                     [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
                     [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
                     [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
                     [TOKEN_STRING] = {string, NULL, PREC_NONE},
                     [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
                     [TOKEN_PLUS] = {NULL, binary, PREC_TERM}

};

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixFn = getRule(parser.previous.type)->prefix;
  if (prefixFn == NULL) {
    // TODO: Handle error: expected expression
    return;
  }
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixFn();
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixFn = getRule(parser.previous.type)->infix;
    infixFn();
    canAssign = false;
  }
  if (canAssign && parser.current.type == TOKEN_EQUAL) {
    errorAt(&parser.current, "Invalid assignment target.");
  }
}

static void string() {
  emitByte(OP_CONSTANT);
  emitByte(emitConstant(
      makeString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitByte(OP_CONSTANT);
  emitByte(emitConstant(makeNumber(value)));
}

static void variable() {
  uint8_t index =
      emitConstant(makeString(parser.previous.start, parser.previous.length));

  if (parser.current.type == TOKEN_EQUAL) {
    advance();
    expression();
    emitByte(OP_SET_GLOBAL);
    emitByte(index);
  }

  emitByte(OP_GET_GLOBAL);
  emitByte(index);
}

static void binary() {
  // if there is "5 + 3 / 1", need to handle the precedence and call the
  // parsePrecedence again
  switch (parser.previous.type) {
  case OP_ADD: {
    emitByte(OP_ADD);
    break;
  }
  default: {
    return;
  }
  }
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
