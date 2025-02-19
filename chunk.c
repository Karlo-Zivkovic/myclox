#include "chunk.h"
#include "stddef.h"
#include "stdlib.h"
#include "value.h"
#include <stdio.h>

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
  initChunk(chunk);
}
