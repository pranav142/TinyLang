#include "stack.h"
#include "memory.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>

void init_stack(Stack *stack) {
  stack->count = 0;
  stack->capacity = 0;
  stack->values = NULL;
}

void free_stack(Stack *stack) {
  free(stack->values);
  init_stack(stack);
}

void stack_push(Stack *stack, Value value) {
  if (stack->capacity <= stack->count + 1) {
    size_t new_capacity = get_new_array_capacity(stack->capacity);
    void *result = grow_array_size(stack->values, new_capacity * sizeof(Value));
    if (result == NULL) {
      printf("failed to grow stack; stack overflow\n");
      exit(1);
    }
    stack->values = (Value *)result;
    stack->capacity = new_capacity;
  }

  stack->values[stack->count++] = value;
}

Value stack_pop(Stack *stack) {
  if (stack->count <= 0) {
    printf("stack underflow\n");
    exit(1);
  }
  stack->count--;
  return stack->values[stack->count];
}

void print_stack(Stack *stack) {
  printf("[");
  for (size_t i = 0; i < stack->count; i++) {
    print_value(stack->values[i]);
    printf(", ");
  }
  printf("]\n");
}

Value *stack_peek(Stack *stack, size_t index) {
  if (stack->count <= 0)
    return NULL;
  return &stack->values[stack->count - index - 1];
}

bool is_stack_empty(Stack *stack) { return stack->capacity == 0; }
