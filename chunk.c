#include "chunk.h"
#include "value.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->line = 1;
  initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity <= chunk->count) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
    chunk->capacity = chunk->capacity * 2;
    chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
    if (chunk->code == NULL) {
      exit(1);
    }
  }

  chunk->code[chunk->count] = byte;
  chunk->line = line;
  chunk->count++;
}

void freeChunk(Chunk *chunk) {
  free(chunk->code);
  freeValueArray(&chunk->constants);
  free(chunk->constants.values);
  initChunk(chunk);
}

void debugChunk(Chunk *chunk) {
  printf("=== CHUNK ===\n");

  // Print constants
  printf("Constants:\n");
  for (int i = 0; i < chunk->constants.count; i++) {
    printf("[%d] ", i);
    printValue(chunk->constants.values[i]);
    printf("\n");
  }

  // Print bytecode
  printf("\nBytecode:\n");
  for (int offset = 0; offset < chunk->count;) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
    case OP_CONSTANT: {
      uint8_t constant = chunk->code[offset + 1];
      printf("%-16s %4d = ", "OP_CONSTANT",
             constant); // Changed formatting here
      printValue(chunk->constants.values[constant]);
      printf("\n");
      offset += 2;
      break;
    }
    case OP_GET_GLOBAL: {
      uint8_t constant = chunk->code[offset + 1];
      printf("%-16s %4d = ", "OP_GET_GLOBAL",
             constant); // Changed formatting here
      printValue(chunk->constants.values[constant]);
      printf("\n");
      offset += 2;
      break;
    }
    case OP_DEFINE_GLOBAL: {
      uint8_t constant = chunk->code[offset + 1];
      printf("%-16s %4d = ", "OP_DEFINE_GLOBAL",
             constant); // Changed formatting here
      printValue(chunk->constants.values[constant]);
      printf("\n");
      offset += 2;
      break;
    }
    case OP_SET_GLOBAL: {
      uint8_t constant = chunk->code[offset + 1];
      printf("%-16s %4d = ", "OP_SET_GLOBAL",
             constant); // Changed formatting here
      printValue(chunk->constants.values[constant]);
      printf("\n");
      offset += 2;
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = chunk->code[offset + 1];
      printf("%-16s %4d\n", "OP_GET_LOCAL", slot);
      offset += 2;
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = chunk->code[offset + 1];
      printf("%-16s %4d\n", "OP_SET_LOCAL", slot);
      offset += 2;
      break;
    }
    case OP_RETURN: {
      printf("OP_RETURN\n"); // Removed extra formatting for simple instructions
      offset += 1;
      break;
    }
    case OP_PRINT: {
      printf("OP_PRINT\n");
      offset += 1;
      break;
    }
    case OP_ADD: {
      printf("OP_ADD\n");
      offset += 1;
      break;
    }
    case OP_LESS: {
      printf("OP_LESS\n");
      offset += 1;
      break;
    }
    case OP_GREATER: {
      printf("OP_GREATER\n");
      offset += 1;
      break;
    }
    case OP_TRUE: {
      printf("OP_TRUE\n");
      offset += 1;
      break;
    }
    case OP_FALSE: {
      printf("OP_FALSE\n");
      offset += 1;
      break;
    }
    case OP_POP: {
      printf("OP_POP\n");
      offset += 1;
      break;
    }
    case OP_NOT: {
      printf("OP_NOT\n");
      offset += 1;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t jump =
          (uint16_t)((chunk->code[offset + 1] << 8) | chunk->code[offset + 2]);
      printf("OP_JUMP_IF_FALSE %d\n", jump);
      offset += 3;
      break;
    }
    case OP_JUMP: {
      uint16_t jump =
          (uint16_t)((chunk->code[offset + 1] << 8) | chunk->code[offset + 2]);
      printf("OP_JUMP %d\n", jump);
      offset += 3;
      break;
    }
    case OP_LOOP: {
      uint16_t jump =
          (uint16_t)((chunk->code[offset + 1] << 8) | chunk->code[offset + 2]);
      printf("OP_LOOP %d\n", jump);
      offset += 3;
      break;
    }
    }
  }
  printf("=== end of CHUNK ===\n\n");
}

// Not used at the moment
void dumpChunkRaw(Chunk *chunk) {
  printf("Chunk contents:\n");
  printf("Count: %d, Capacity: %d\n", chunk->count, chunk->capacity);

  printf("\nBytecode (raw):\n");
  for (int i = 0; i < chunk->count; i++) {
    printf("%02x ", chunk->code[i]);
    if ((i + 1) % 16 == 0)
      printf("\n");
  }
  printf("\n");

  printf("\nConstants:\n");
  for (int i = 0; i < chunk->constants.count; i++) {
    printf("[%d] = ", i);
    printValue(chunk->constants.values[i]);
    printf("\n");
  }
}
