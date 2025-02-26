#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "stdio.h"
#include "value.h"
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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token previous;
  Token current;
  bool hadError;
  bool panicMode;
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

void initCompiler() {
  compiler.localCount = 0;
  compiler.currentScopeDepth = 0;
}

static void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();
}

static void errorAt(Token *token, const char *message) {
  if (parser.panicMode) {
    return;
  }
  parser.panicMode = true;
  fprintf(stderr, "[Line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": '%s'\n", message);
  parser.hadError = true;
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;

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
    default: {
      // Do nothing.
    }
    }
    advance();
  }
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk, byte, parser.previous.line);
}

static void statement();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void string(bool canAssign);
static void number(bool canAssign);
static void variable(bool canAssign);
static void binary(bool canAssign);

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static uint8_t emitConstant(Value value) {
  uint8_t index = writeValueArray(&currentChunk->constants, value);
  return index;
}

static void consume(TokenType type, const char *errorMessage) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAt(&parser.current, errorMessage);
}

static void printStatement() {
  expression();
  emitByte(OP_PRINT);
  consume(TOKEN_SEMICOLON, "Expect ';' after value in print statement");
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
  consume(TOKEN_EQUAL, "Expect '=' after variable name");
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

static void beginScope() { compiler.currentScopeDepth++; }
static void endScope() { compiler.currentScopeDepth--; }
static void block() {
  while (parser.current.type != TOKEN_RIGHT_BRACE) {
    statement();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement() {
  if (parser.current.type == TOKEN_PRINT) {
    advance();
    printStatement();
  } else if (parser.current.type == TOKEN_VAR) {
    advance();
    varStatement();

  } else if (parser.current.type == TOKEN_LEFT_BRACE) {
    advance();
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }

  if (parser.panicMode) {
    synchronize();
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
  prefixFn(canAssign);
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixFn = getRule(parser.previous.type)->infix;
    infixFn(canAssign);
    canAssign = false;
  }
  if (!canAssign && parser.current.type == TOKEN_EQUAL) {
    errorAt(&parser.current, "Invalid assignment target.");
  }
}

static void string(bool canAssign) {
  (void)canAssign;
  emitByte(OP_CONSTANT);
  emitByte(emitConstant(
      makeString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void number(bool canAssign) {
  (void)canAssign;
  double value = strtod(parser.previous.start, NULL);
  emitByte(OP_CONSTANT);
  emitByte(emitConstant(makeNumber(value)));
}

static void variable(bool canAssign) {
  (void)canAssign;
  uint8_t index =
      emitConstant(makeString(parser.previous.start, parser.previous.length));

  if (canAssign && parser.current.type == TOKEN_EQUAL) {
    advance();
    expression();
    if (compiler.currentScopeDepth > 0) {
      emitByte(OP_SET_LOCAL);
      emitByte(index);
    }

    emitByte(OP_SET_GLOBAL);
    emitByte(index);
  }

  emitByte(OP_GET_GLOBAL);
  emitByte(index);
}

static void binary(bool canAssign) {
  (void)canAssign;
  // if there is "5 + 3 / 1", need to handle the precedence and call the
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // parsePrecedence again
  switch (operatorType) {
  case TOKEN_PLUS: {
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
  initCompiler();
  advance();
  parser.hadError = false;
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    statement();
  }
  emitByte(OP_RETURN);
  return !parser.hadError;
}
