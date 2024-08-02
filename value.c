#include "value.h"
#include "memory.h"
#include "object.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void init_value_array(ValueArray *value_array) {
  value_array->capacity = 0;
  value_array->count = 0;
  value_array->values = NULL;
}

void add_value(ValueArray *value_array, Value value) {

  if (value_array->capacity < value_array->count + 1) {
    size_t new_capacity = get_new_array_capacity(value_array->capacity);
    void *result =
        grow_array_size(value_array->values, new_capacity * sizeof(Value));
    if (result == NULL) {
      printf("ran out of memory when writing to a value array\n");
      exit(1);
    }
    value_array->values = (Value *)result;
    value_array->capacity = new_capacity;
  }

  value_array->values[value_array->count++] = value;
}

void free_value_array(ValueArray *value_array) {
  if (!value_array->values)
    return;
  free(value_array->values);
  init_value_array(value_array);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  }
}

void print_value(Value value) {
  switch (value.type) {
  case VAL_NIL:
    printf("NULL");
    break;
  case VAL_BOOL:
    printf("%s", AS_BOOL(value) ? "true" : "false");
    break;
  case VAL_NUMBER:
    printf("%.01f", AS_NUMBER(value));
    break;
  case VAL_OBJ:
    print_object(value);
    break;
  default:
    printf("invalid value");
    break;
  }
}

void print_value_array(ValueArray *value_array) {
  printf("Values: ");
  for (size_t i = 0; i < value_array->count; i++) {
    print_value(value_array->values[i]);
  }
  printf("\n");
}
