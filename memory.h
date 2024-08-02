#pragma once

#include <stddef.h>

size_t get_new_array_capacity(size_t old_capacity);
void *grow_array_size(void *array, size_t new_capacity);
