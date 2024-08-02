#pragma once

#include "value.h"

typedef struct {
  ObjString *key;
  Value value;
} Entry;

typedef struct {
  size_t count;
  size_t capacity;
  Entry *entries;
} Table;

void init_hash_map(Table *table);
void free_hash_map(Table *table);
Entry *find_entry(Entry *entries, size_t capacity, ObjString *key);
void clear_table(Entry *entries, size_t capacity);
size_t copy_entries(Table *table, Entry *entries, size_t capacity);
void grow_table(Table *table);
bool insert_entry(Table *table, ObjString *key, Value value);
bool get_entry(Table *table, ObjString *key, Value *value);
bool delete_entry(Table *table, ObjString *key);
