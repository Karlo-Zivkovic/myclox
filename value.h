#pragma once

#include <stdbool.h>

typedef enum { VAL_NUMBER, VAL_STRING, VAL_BOOL, VAL_NIL } ValueType;

typedef struct {
  char *chars;
  int length;
} String;

typedef struct {
  ValueType type;
  union {
    double number;
    String *string;
    bool boolean;
  } as;
} Value;

typedef struct {
  int count;
  int capacity;
  Value *values;
} ValueArray;

void initValueArray(ValueArray *constants);
int writeValueArray(ValueArray *constants, Value value);
void freeValueArray(ValueArray *constants);
void freeValue(Value value);
void freeString(String *string);
Value makeNumber(double num);
Value addValues(Value a, Value b);
Value makeString(const char *string, int length);
void printValue(Value value);
void negateValue(Value *value);
Value makeNil();
Value compareValues(Value a, Value b, char operator_);
