#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "stddef.h"
#include "stdio.h"
#include "table.h"
#include "value.h"
#include <stdint.h>
#include <stdlib.h>

VM vm;

void initVM() {
  vm.chunk = NULL;
  vm.ip = NULL;
  vm.stackCapacity = STACK_INIT;
  vm.stack = malloc(vm.stackCapacity * sizeof(Value));
  vm.stackTop = vm.stack;
  initTable(&vm.globals);
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static void freeVM() {
  free(vm.stack);
  freeTable(&vm.globals);
  initVM();
};

static InterpretResult run() {
  for (;;) {
    switch (*vm.ip++) {
    case OP_CONSTANT: {
      uint8_t index = *vm.ip++;
      Value value = vm.chunk->constants.values[index];
      push(value);
      break;
    }
    case OP_PRINT: {
      Value value = pop();
      printValue(value);
      break;
    }
    case OP_DEFINE_GLOBAL: {
      uint8_t index = *vm.ip++;
      Value key = vm.chunk->constants.values[index];
      Value value = pop();
      if (!tableSet(&vm.globals, key.as.string, value)) {
        printf("Failed to define global variable\n");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_GLOBAL: {
      Value value;
      uint8_t index = *vm.ip++;
      Value key = vm.chunk->constants.values[index];

      if (!tableGet(&vm.globals, key.as.string, &value)) {
        printf("Undefined variable '%s'\n", key.as.string->chars);
        return INTERPRET_RUNTIME_ERROR;
      }

      push(value);
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }
  return INTERPRET_OK;
}

void debugStack(VM *vm) {
  printf("=== Stack ===\n");
  printf("Capacity: %d\n", vm->stackCapacity);
  printf("Stack size: %ld\n", (long)(vm->stackTop - vm->stack));

  if (vm->stack == vm->stackTop) {
    printf("Stack is empty\n");
  } else {
    printf("Values:\n");
    // Print from bottom to top of stack
    for (Value *slot = vm->stack; slot < vm->stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]\n");
    }
  }
  printf("===========\n");
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
  freeVM();
  return result;
}
