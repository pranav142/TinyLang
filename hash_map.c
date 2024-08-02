#include "hash_map.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Allocate more space one 75 percent full
#define TABLE_MAX_LOAD 0.75

void init_hash_map(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_hash_map(Table *table) {
  if (table->entries) {
    free(table->entries);
  }
  init_hash_map(table);
}

static void create_tombstone_entry(Entry *entry) {
  entry->value = BOOL_VAL(true);
  entry->key = NULL;
}

static bool is_tombstone_entry(Entry *entry) {
  return (entry->key == NULL &&
          (IS_BOOL(entry->value) && AS_BOOL(entry->value)));
}

Entry *find_entry(Entry *entries, size_t capacity, ObjString *key) {
  int index = key->hash % capacity;
  Entry *tombstone = NULL;

  for (;;) {
    Entry *entry = &entries[index];

    if (entry->key == NULL) {
      // if we reach a empty entry return latest tombstone for reuse
      if (!is_tombstone_entry(entry)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (memcmp(entry->key->chars, key->chars, key->length) == 0) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

void clear_table(Entry *entries, size_t capacity) {
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
}

// returns number of entries
size_t copy_entries(Table *table, Entry *entries, size_t capacity) {
  size_t count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    // skips over tombstones to not waste space
    if (entry->key == NULL)
      continue;

    Entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    count++;
  }
  return count;
}

void grow_table(Table *table) {
  size_t capacity = get_new_array_capacity(table->capacity);
  Entry *entries = (Entry *)malloc(capacity * sizeof(Entry));
  if (entries == NULL) {
    printf("failed to allocate memory when inserting into hash map\n");
    exit(1);
  }
  clear_table(entries, capacity);
  // TODO: is there a better way to create new hashmap without recopying all
  // previous entries
  size_t num_entries = copy_entries(table, entries, capacity);

  free_hash_map(table);
  table->entries = entries;
  table->capacity = capacity;
  table->count = num_entries;
}

bool insert_entry(Table *table, ObjString *key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    grow_table(table);
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);
  bool is_new_key = entry->key == NULL;
  // if not a tombstone and a new key
  if (is_new_key && !is_tombstone_entry(entry))
    table->count++;

  entry->value = value;
  entry->key = key;
  return is_new_key;
}

bool get_entry(Table *table, ObjString *key, Value *value) {
  if (table->count <= 0) {
    printf("Failed to retrieve entry, no entries in table\n");
    return false;
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);
  bool is_key_match = entry->key != NULL;
  if (is_key_match) {
    *value = entry->value;
  }
  return is_key_match;
}

bool delete_entry(Table *table, ObjString *key) {
  if (table->count <= 0) {
    printf("Failed to delete entry, no entries in table\n");
    return false;
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);
  bool key_found = entry->key != NULL;
  if (key_found) {
    // add tombstone entry
    create_tombstone_entry(entry);
  }

  return key_found;
}
