#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

char *runFile(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'\n", filename);
    exit(74);
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read '%s'\n", filename);
    exit(70);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);

  if (bytes_read < (size_t)file_size) {
    fprintf(stderr, "Could not read file '%s'\n", filename);
    free(buffer);
    fclose(file);
    exit(74);
  }
  buffer[bytes_read] = '\0';
  fclose(file);

  return buffer;
};

int main(int argc, char *argv[]) {
  if (argc == 1) {
    // runRepl();
  } else if (argc == 2) {
    char *source = runFile(argv[1]);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR) {
      exit(65);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
      exit(70);
    }
  } else {
    fprintf(stderr, "Start the program with command: clox [file]\n");
    return 1;
  }
  return 0;
}
