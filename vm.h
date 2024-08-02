#pragma once
#include "chunk.h"

#include "hash_map.h"
#include "stack.h"

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Stack stack;
  Obj *objects;
  Table strings;
  Table globals;
} VM;

typedef enum {
  INTERPRET_OK,
  RUNTIME_ERROR,
  COMPILE_ERROR,
} InterpretResponse;

void init_vm(VM *vm);
void free_vm(VM *vm);
static inline uint8_t read_byte(VM *vm);
static void handle_instruction(VM *vm, uint8_t instruction);
InterpretResponse run(VM *vm);
InterpretResponse interpret(VM *vm, Chunk *chunk);
