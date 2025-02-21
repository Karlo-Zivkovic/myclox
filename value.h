#pragma once

typedef enum {
  VAL_NUMBER,
  VAL_STRING,
} ValueType;

typedef struct {
  char *chars;
  int length;
} String;

typedef struct {
  ValueType type;
  union {
    double number;
    String *string;
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
Value makeNumber(double num);
Value makeString(const char *string, int length);
void printValue(Value value);
