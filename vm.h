#pragma once

typedef enum {
 INTERPRET_OK,
 INTERPRET_COMPILE_ERROR,
 INTERPRET_RUNTIME_ERROR,
} InterpretResult; 

InterpretResult interpret(const char *source);
void initVM();
void freeVM();