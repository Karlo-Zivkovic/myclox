#include "value.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  for (int i = 0; i < constants->count; i++) {
    freeValue(constants->values[i]);
  }
  free(constants->values);
  initValueArray(constants);
}

Value addValues(Value a, Value b) {
  Value result;

  // Number operations
  if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
    result.type = VAL_NUMBER;
    result.as.number = a.as.number + b.as.number;
    return result;
  }

  // String concatenation
  if (a.type == VAL_STRING && b.type == VAL_STRING) {
    String *aString = a.as.string;
    String *bString = b.as.string;
    int length = aString->length + bString->length;
    char *chars = malloc(length + 1);
    memcpy(chars, aString->chars, aString->length);
    memcpy(chars + aString->length, bString->chars, bString->length);
    chars[length] = '\0'; // Null terminate
    result = makeString(chars, length);
    free(chars);
    writeValueArray(&vm.tempValues, result);
  }

  // Handle error case - incompatible types
  return result;
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
    break;
  }
  case VAL_STRING: {
    String *string = value.as.string;
    printf("%s\n", string->chars);
    break;
  }
  case VAL_BOOL: {
    printf(value.as.boolean ? "true" : "false");
    break;
  }
  case VAL_NIL:
    printf("nil");
    break;
  }
}

Value makeNil() {
  Value value;
  value.type = VAL_NIL;
  return value;
}

String *createString(const char *chars, int length) {
  String *string = (String *)malloc(sizeof(String));
  string->chars = (char *)malloc(length + 1);
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  string->length = length;
  return string;
}

Value makeString(const char *chars, int length) {
  Value value;
  value.type = VAL_STRING;
  value.as.string = createString(chars, length);
  return value;
}

void freeString(String *string) {
  free(string->chars);
  free(string);
}

// Function to free a value
void freeValue(Value value) {
  switch (value.type) {
  case VAL_STRING:
    freeString(value.as.string);
    break;
  default: {
    return;
  }
  }
}

void negateValue(Value *value) {
  if (value->type == VAL_NUMBER) {
    value->as.number = -value->as.number;
  }
  // TODO: Do i negate string here?
}

Value compareValues(Value a, Value b, char operator_) {
  if (a.type != VAL_NUMBER || b.type != VAL_NUMBER) {
    // TODO: Report error and return INTERPRET_RUNTIME_ERROR
  }

  Value result;
  result.type = VAL_BOOL;
  switch (operator_) {
  case '<':
    result.as.boolean = a.as.number < b.as.number;
    break;
  case '>':
    result.as.boolean = a.as.number > b.as.number;
    break;
  default:
    result.as.boolean = false;
  }

  return result;
}
