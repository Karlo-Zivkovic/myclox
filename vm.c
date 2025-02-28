#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "table.h"
#include "value.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

VM vm;

void initVM() {
  vm.chunk = NULL;
  vm.ip = NULL;
  vm.stackCapacity = STACK_INIT;
  vm.stack = malloc(vm.stackCapacity * sizeof(Value));
  vm.stackTop = vm.stack;
  initTable(&vm.globals);
  initValueArray(&vm.tempValues);
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
  freeValueArray(&vm.tempValues);
  initVM();
};

static bool isFalsey(Value value) {
  if (value.type == VAL_NIL) {
    return true;
  }
  if (value.type == VAL_BOOL) {
    return !value.as.boolean;
  }
  return false;
}

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
        printf("Failed to define global variable '%s' \n",
               key.as.string->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_ADD: {
      Value b = pop();
      Value a = pop();
      push(addValues(a, b));
      break;
    }
    case OP_LESS: {
      Value b = pop();
      Value a = pop();
      Value result = compareValues(a, b, '<');
      push(result);
      break;
    }
    case OP_GREATER: {
      Value b = pop();
      Value a = pop();
      Value result = compareValues(a, b, '>');
      push(result);
      break;
    }
    case OP_SET_GLOBAL: {
      uint8_t index = *vm.ip++;
      Value key = vm.chunk->constants.values[index];
      Value value = vm.stackTop[-1];

      if (tableSet(&vm.globals, key.as.string, value)) {
        printf("Failed to set global variable '%s' \n", key.as.string->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      // pop();
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

      //  printValue(value);
      push(value);
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = *vm.ip++;
      push(vm.stack[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = *vm.ip++;
      Value value = vm.stackTop[-1];
      vm.stack[slot] = value;
      break;
    }
    case OP_NIL: {
      push(makeNil());
      break;
    }
    case OP_POP: {
      pop();
      break;
    }
    case OP_TRUE: {
      Value value;
      value.type = VAL_BOOL;
      value.as.boolean = true;
      push(value);
      break;
    }
    case OP_FALSE: {
      Value value;
      value.type = VAL_BOOL;
      value.as.boolean = false;
      push(value);
      break;
    }
    case OP_LOOP: {
      uint16_t offset = (uint16_t)((*vm.ip << 8) | *(vm.ip + 1));
      vm.ip += 2;
      vm.ip -= offset;
      break;
    }
    case OP_JUMP: {
      uint16_t offset = (uint16_t)((*vm.ip << 8) | *(vm.ip + 1));
      vm.ip += offset + 2;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = (uint16_t)((*vm.ip << 8) | *(vm.ip + 1));
      vm.ip += 2;

      if (isFalsey(vm.stackTop[-1])) {
        vm.ip += offset;
      }
      break;
    }
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    case OP_NOT: {
      // Value value = *(vm.stackTop - 1);
      Value *value = vm.stackTop - 1;
      negateValue(value);
      break;
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
  debugChunk(vm.chunk);

  InterpretResult result = run();

  freeChunk(&chunk);
  freeVM();
  return result;
}
