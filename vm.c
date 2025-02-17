#include "vm.h"
#include "compiler.h"

InterpretResult interpret(const char *source) {
  if (!compile(source)) {
    return INTERPRET_COMPILE_ERROR;
  }
  return INTERPRET_OK;
}
