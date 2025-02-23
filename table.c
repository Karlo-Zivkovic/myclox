#include "table.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "value.h"
#include <stdint.h>
#include <string.h>

void initTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

static uint32_t hashString(const char *chars, int length) {
  uint32_t hash = 2166136261u; // FNV-1a hash

  for (int i = 0; i < length; i++) {
    hash ^= chars[i];
    hash *= 16777619;
  }

  return hash;
}

static Entry *findEntry(Entry *entries, int capacity, String *key) {
  uint32_t hash = hashString(key->chars, key->length);
  uint32_t index = hash % capacity;
  Entry *tombstone = NULL;

  // Linear probing
  for (;;) {
    Entry *entry = &entries[index];

    if (entry->key == NULL) {
      // Empty entry
      if (!entry->isDeleted) {
        // If it's not a tombstone, this is where we insert
        return tombstone != NULL ? tombstone : entry;
      } else {
        // Found a tombstone, remember it
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key->length == key->length &&
               memcmp(entry->key->chars, key->chars, key->length) == 0) {
      // We found the key
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjustCapacity(Table *table, int newCapacity) {
  // Allocate new array
  Entry *entries = calloc(newCapacity, sizeof(Entry));
  table->count = 0; // Will recount as we re-insert

  // Re-insert all existing entries
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL || entry->isDeleted)
      continue;

    Entry *dest = findEntry(entries, newCapacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // Free old array
  free(table->entries);

  table->entries = entries;
  table->capacity = newCapacity;
}

bool tableSet(Table *table, String *key, Value value) {
  // Grow if needed
  if (table->count + 1 > table->capacity * 0.75) {
    int capacity = table->capacity < 8 ? 8 : table->capacity * 2;
    adjustCapacity(table, capacity);
  }

  Entry *entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && !entry->isDeleted)
    table->count++;

  entry->key = key;
  entry->value = value;
  entry->isDeleted = false;
  return isNewKey;
}

bool tableGet(Table *table, String *key, Value *value) {
  if (table->count == 0)
    return false;

  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;

  *value = entry->value;
  // printValue(*value);
  return true;
}

bool tableDelete(Table *table, String *key) {
  if (table->count == 0)
    return false;

  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;

  freeValue(entry->value);

  // Place a tombstone
  entry->key = NULL;
  entry->isDeleted = true;
  return true;
}

void freeTable(Table *table) {
  // Free any string values in the table
  /* for (int i = 0; i < table->capacity; i++) { */
  /*   Entry *entry = &table->entries[i]; */
  /*   if (entry->key != NULL && !entry->isDeleted) { */
  /*     freeValue(entry->value); */
  /*     freeString(entry->key); // Free the key string */
  /*   } */
  /* } */

  free(table->entries);
  initTable(table);
}

void debugPrintTable(Table *table) {
  printf("Table contents:\n");
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key != NULL && !entry->isDeleted) {
      printf("Key: %s, Value: ", entry->key->chars);
      printValue(entry->value);
      printf("\n");
    }
  }
}
