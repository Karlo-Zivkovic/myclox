#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  Token name;
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
static void unary(bool canAssign);
static void literal(bool canAssign);
static void and_(bool canAssign);
static void or_(bool canAssign);

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
  local->name = token;
  local->depth = compiler.currentScopeDepth;
  compiler.localCount++;
}

static void varStatement() {
  consume(TOKEN_IDENTIFIER, "Expect variable name.");
  if (compiler.currentScopeDepth == 0) {
    uint8_t globalIndex =
        emitConstant(makeString(parser.previous.start, parser.previous.length));

    if (parser.current.type == TOKEN_EQUAL) {
      consume(TOKEN_EQUAL, "Expect '=' after variable name");
      expression();
    } else {
      emitByte(OP_NIL);
    }

    emitByte(OP_DEFINE_GLOBAL);
    emitByte(globalIndex);
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  } else {
    // declare local variable
    Local local;
    local.name = parser.previous;
    local.depth = compiler.currentScopeDepth;
    compiler.locals[compiler.localCount++] = local;

    if (parser.current.type == TOKEN_EQUAL) {
      consume(TOKEN_EQUAL, "Expect '=' after variable name");
      expression();
    } else {
      emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  }
}

static void patchJump(int offset) {
  int jump = currentChunk->count - offset - 2;
  if (jump > UINT16_MAX) {
    errorAt(&parser.previous, "Jump too large");
  }
  currentChunk->code[offset] = (jump >> 8) & 0xFF;
  currentChunk->code[offset + 1] = jump & 0xFF;
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xFF);
  emitByte(0xFF);
  return currentChunk->count - 2;
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after if.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

  int thenJumpIndex = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJumpIndex = emitJump(OP_JUMP);
  patchJump(thenJumpIndex);
  emitByte(OP_POP);

  if (parser.current.type == TOKEN_ELSE) {
    advance();
    statement();
  }

  patchJump(elseJumpIndex);
}

static void expressionStatement() {
  expression();
  advance();
}

static void beginScope() { compiler.currentScopeDepth++; }

static void endScope() {
  compiler.currentScopeDepth--;
  while (compiler.localCount > 0 &&
         compiler.locals[compiler.localCount - 1].depth >
             compiler.currentScopeDepth) {
    emitByte(OP_POP);
    compiler.localCount--;
  }
}

static void block() {
  while (parser.current.type != TOKEN_RIGHT_BRACE) {
    statement();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  int offset = currentChunk->count - loopStart + 2;
  if (offset > UINT16_MAX) {
    errorAt(&parser.previous, "Loop body too large.");
  }
  emitByte((offset >> 8) & 0xFF);
  emitByte(offset & 0xFF);
};

static void whileStatement() {
  int loopStart = currentChunk->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
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
  } else if (parser.current.type == TOKEN_IF) {
    advance();
    ifStatement();
  } else if (parser.current.type == TOKEN_WHILE) {
    advance();
    whileStatement();
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
                     [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
                     [TOKEN_BANG] = {unary, NULL, PREC_NONE},
                     [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
                     [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
                     [TOKEN_AND] = {NULL, and_, PREC_AND},
                     [TOKEN_OR] = {NULL, or_, PREC_OR},
                     [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
                     [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON}

};

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixFn = getRule(parser.previous.type)->prefix;
  if (prefixFn == NULL) {
    // TODO: Handle error: expected expression, maybe needs a fix
    errorAt(&parser.previous, "Expected expression");
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

static void or_(bool canAssign) {
  (void)canAssign;
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void and_(bool canAssign) {
  (void)canAssign;
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);
  patchJump(endJump);
}

static void literal(bool canAssign) {
  (void)canAssign;
  switch (parser.previous.type) {
  case TOKEN_TRUE: {
    emitByte(OP_TRUE);
    break;
  }
  case TOKEN_FALSE: {
    emitByte(OP_FALSE);
    break;
  }
  default: {
    return;
  }
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

static bool identifiersEqual(Token *a, Token *b) {
  if (a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Token *name) {
  for (int i = compiler.localCount - 1; i >= 0; i--) {
    Local *local = &compiler.locals[i];
    if (identifiersEqual(&local->name, name)) {
      return i;
    }
  }
  return -1;
}

static void variable(bool canAssign) {
  int localIndex = resolveLocal(&parser.previous);

  if (localIndex != -1) {
    if (canAssign && parser.current.type == TOKEN_EQUAL) {
      advance();
      expression();
      emitByte(OP_SET_LOCAL);
      emitByte((uint8_t)localIndex);
    } else {
      emitByte(OP_GET_LOCAL);
      emitByte((uint8_t)localIndex);
    }
  } else {
    uint8_t globalIndex =
        emitConstant(makeString(parser.previous.start, parser.previous.length));

    if (canAssign && parser.current.type == TOKEN_EQUAL) {
      advance();
      expression();
      emitByte(OP_SET_GLOBAL);
      emitByte(globalIndex);
    } else {
      emitByte(OP_GET_GLOBAL);
      emitByte(globalIndex);
    }
  }
}

static void unary(bool canAssign) {
  (void)canAssign;
  TokenType operatorType = parser.previous.type;

  parsePrecedence(PREC_UNARY);

  switch (operatorType) {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS: {
    emitByte(OP_NEGATE);
    break;
  }
  default: {
    return;
  }
  }
}

static void binary(bool canAssign) {
  (void)canAssign;

  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_LESS: {
    emitByte(OP_LESS);
    break;
  }
  case TOKEN_GREATER: {
    emitByte(OP_GREATER);
    break;
  }
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
