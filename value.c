#include "value.h"
#include "stdlib.h"
#include <stdio.h>
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
  }
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
  case VAL_NUMBER: {
    break;
  }
  }
}
