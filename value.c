#include "value.h"
#include "stdlib.h"
#include <stdio.h>

void initValueArray(ValueArray *constants) {
  constants->count = 0;
  constants->capacity = 0;
  constants->values = NULL;
}

int writeValueArray(ValueArray *constants, Value value) {
  if (constants->capacity <= constants->count) {
    int oldCapacity = constants->capacity;
    constants->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
    constants->capacity = constants->capacity * 2;
    constants->values =
        realloc(constants->values, constants->capacity * sizeof(Value));
    if (constants->values == NULL) {
      exit(1);
    }
  }

  constants->values[constants->count] = value;
  return constants->count++;
}

void freeValueArray(ValueArray *constants) {
  free(constants->values);
  initValueArray(constants);
}

Value makeNumber(double num) {
  Value value;
  value.type = VAL_NUMBER;
  value.as.number = num;
  return value;
}

void printValue(Value value) {
  switch (value.type) {
  case VAL_NUMBER: {
    double number = value.as.number;
    printf("%f\n", number);
  }
  }
}
