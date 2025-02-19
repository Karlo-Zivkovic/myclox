#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "stddef.h"
#include <stdlib.h>

VM vm;

void initVM() {
  vm.chunk = NULL;
  vm.ip = NULL;
  vm.stackCapacity = STACK_INIT;
  vm.stack = malloc(vm.stackCapacity * sizeof(Value));
  vm.stackTop = vm.stack;
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static InterpretResult run() {
  for (;;) {
    switch (*vm.ip++) {
    case OP_CONSTANT: {
      int index = *vm.ip++;
      Value value = vm.chunk->constants.values[index];
      push(value);
      break;
    }
    case OP_PRINT: {
      Value value = pop();
      printValue(value);
      break;
    }
    case OP_RETURN: {

      return INTERPRET_OK;
    }
    }
  }
  return INTERPRET_OK;
}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  initChunk(&chunk);
  initVM();

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }
  vm.chunk = &chunk;
  vm.ip = chunk.code;

  InterpretResult result = run();

  freeChunk(&chunk);
  return result;
}
