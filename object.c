#include "object.h"
#include "memory.h"
#include "value.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Obj *allocate_object(Obj *objects, size_t size, ObjType type) {
  Obj *object = (Obj *)malloc(size);
  object->type = type;
  object->next = objects;
  objects = object;
  return object;
}

static ObjString *allocate_string(Obj *objects, char *chars, size_t length,
                                  uint32_t hash) {
  ObjString *string =
      (ObjString *)allocate_object(objects, sizeof(ObjString), OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  return string;
}

size_t calculate_string_length(char *chars) {
  size_t length = 0;
  while (chars[length] != '\0') {
    length++;
  }
  return length;
}

uint32_t FNV32(const char *s) {
  uint32_t hash = FNV_OFFSET_32, i;
  for (i = 0; i < strlen(s); i++) {
    hash = hash ^ (s[i]);
    hash = hash * FNV_PRIME_32;
  }
  return hash;
}

ObjString *create_string(Obj *objects, char *chars) {
  uint32_t hash = FNV32(chars);
  return allocate_string(objects, chars, calculate_string_length(chars), hash);
}
