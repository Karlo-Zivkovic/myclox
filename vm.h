#include "chunk.h"
#include "table.h"

#pragma once

#define STACK_INIT 256

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
  Chunk *chunk;
  uint8_t *ip;

  // stack
  Value *stack;
  Value *stackTop;
  int stackCapacity;

  // globals
  Table globals;

} VM;

InterpretResult interpret(const char *source);
void initVM();
void freeVM();
