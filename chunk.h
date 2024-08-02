#pragma once

#include "value.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t count;
  size_t capacity;
  uint8_t *byte_code;
  int *lines;
  ValueArray constants;
} Chunk;

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_RETURN,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_DIVIDE,
  OP_MULTIPLY,
  OP_MOD,
  OP_NOT,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_PRINT,
  OP_POP,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_JUMP_IF_FALSE,
  OP_JUMP,
  OP_LOOP,
} OpCode;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void free_chunk(Chunk *chunk);
int print_simple_instruction(const char *instruction, size_t index);
int dissasemble_instruction(Chunk *chunk, size_t index);
void dissasemble_chunk(Chunk *chunk, const char *chunk_name);
int add_constant(Chunk *chunk, Value value);
int get_line(Chunk *chunk, int index);
