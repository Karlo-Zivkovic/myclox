#include "chunk.h"
#include "table.h"
#include "value.h"

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

  // temporary values
  ValueArray tempValues;

} VM;

extern VM vm;
InterpretResult interpret(const char *source);
void debugStack(VM *vm);
