#pragma once

typedef enum {
  VAL_NUMBER,
} ValueType;

typedef struct {
  ValueType type;
  union {
    double number;
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
Value makeNumber(double num);
void printValue(Value value);
