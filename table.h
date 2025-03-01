#include "value.h"
#include <stdbool.h>

#pragma once

typedef struct {
  String *key;
  Value value;
  bool isDeleted;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry *entries;
} Table;

void initTable(Table *table);
bool tableSet(Table *table, String *key, Value value);
bool tableGet(Table *table, String *key, Value *value);
void freeTable(Table *table);
void debugPrintTable(Table *table);
