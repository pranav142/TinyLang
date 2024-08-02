#include "memory.h"
#include <stddef.h>
#include <stdlib.h>

size_t get_new_array_capacity(size_t old_capacity) {
  return (old_capacity) < 8 ? 8 : old_capacity * 2;
}

void *grow_array_size(void *array, size_t new_capacity) {
  if (new_capacity <= 0)
    return NULL;

  void *new_byte_code = realloc(array, new_capacity);
  if (!new_byte_code)
    return NULL;

  return new_byte_code;
}
