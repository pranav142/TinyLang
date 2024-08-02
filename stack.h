#pragma once
#include "lexer.h"
#include "value.h"

typedef struct {
  size_t capacity;
  size_t count;
  Value *values;
} Stack;

void init_stack(Stack *stack);
void free_stack(Stack *stack);
void stack_push(Stack *stack, Value value);
Value stack_pop(Stack *stack);
void print_stack(Stack *stack);
bool is_stack_empty(Stack *stack);
Value *stack_peek(Stack *stack, size_t index);
