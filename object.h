#pragma once
#include "value.h"
#include <stdint.h>

typedef enum {
  OBJ_STRING,
} ObjType;

// This is a form of inheritance
// Will allow us to define more object types
struct Obj {
  ObjType type;
  struct Obj *next;
};

struct ObjString {
  Obj obj;
  int length;
  char *chars;
  uint32_t hash;
};

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

static Obj *allocate_object(Obj *objects, size_t size, ObjType type);
static ObjString *allocate_string(Obj *objects, char *chars, size_t length,
                                  uint32_t hash);
size_t calculate_string_length(char *chars);
ObjString *create_string(Obj *objects, char *chars);
