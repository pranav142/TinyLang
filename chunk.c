#include "chunk.h"
#include "memory.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>

void init_chunk(Chunk *chunk) {
  chunk->byte_code = NULL;
  chunk->lines = NULL;
  chunk->count = 0;
  chunk->capacity = 0;
  init_value_array(&chunk->constants);
}

// returns the index of the added constant
int add_constant(Chunk *chunk, Value value) {
  add_value(&chunk->constants, value);
  return chunk->constants.count - 1;
}

void write_chunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    size_t new_capacity = get_new_array_capacity(chunk->capacity);
    void *result =
        grow_array_size(chunk->byte_code, new_capacity * sizeof(uint8_t));
    if (result == NULL) {
      printf("ran out of memory when writing to a chunk\n");
      exit(1);
    }
    chunk->byte_code = (uint8_t *)result;

    result = grow_array_size(chunk->lines, new_capacity * sizeof(int));
    if (result == NULL) {
      printf("ran out of memory when writing to a chunk\n");
      exit(1);
    }
    chunk->lines = (int *)result;
    chunk->capacity = new_capacity;
  }

  chunk->byte_code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

void free_chunk(Chunk *chunk) {
  free_value_array(&chunk->constants);
  free(chunk->byte_code);
  free(chunk->lines);
  init_chunk(chunk);
}

int print_simple_instruction(const char *instruction, size_t index) {
  printf("%s\n", instruction);
  return index + 1;
}

int print_constant_instruction(const char *instruction, Chunk *chunk,
                               size_t index) {
  uint8_t constant_index = chunk->byte_code[index + 1];
  printf("constant index: %04d %s constant value: ", constant_index,
         instruction);
  print_value(chunk->constants.values[constant_index]);
  printf("\n");
  return index + 2;
}

int print_jump_instruction(const char *instruction, Chunk *chunk, int index) {
  uint16_t jump = (uint16_t)((chunk->byte_code[index + 1] << 8) |
                             chunk->byte_code[index + 2]);
  printf("%s jump location: %d\n", instruction, jump);
  return index + 3;
}

int get_line(Chunk *chunk, int index) { return chunk->lines[index]; }

int dissasemble_instruction(Chunk *chunk, size_t index) {
  int line = get_line(chunk, index);
  printf("Line: %d: %04d ", line, (int)index);

  uint8_t instruction = chunk->byte_code[index];
  switch (instruction) {
  case OP_RETURN:
    return print_simple_instruction("OP_RETURN", index);
  case OP_CONSTANT:
    return print_constant_instruction("OP_CONSTANT", chunk, index);
  case OP_NEGATE:
    return print_simple_instruction("OP_NEGATE", index);
  case OP_DIVIDE:
    return print_simple_instruction("OP_DIVIDE", index);
  case OP_SUBTRACT:
    return print_simple_instruction("OP_SUBTRACT", index);
  case OP_ADD:
    return print_simple_instruction("OP_ADD", index);
  case OP_MULTIPLY:
    return print_simple_instruction("OP_MULTIPLY", index);
  case OP_TRUE:
    return print_simple_instruction("OP_TRUE", index);
  case OP_FALSE:
    return print_simple_instruction("OP_FALSE", index);
  case OP_NIL:
    return print_simple_instruction("OP_NIL", index);
  case OP_NOT:
    return print_simple_instruction("OP_NOT", index);
  case OP_EQUAL:
    return print_simple_instruction("OP_EQUAL", index);
  case OP_GREATER:
    return print_simple_instruction("OP_GREATER", index);
  case OP_LESS:
    return print_simple_instruction("OP_LESS", index);
  case OP_PRINT:
    return print_simple_instruction("OP_PRINT", index);
  case OP_POP:
    return print_simple_instruction("OP_POP", index);
  case OP_DEFINE_GLOBAL:
    return print_constant_instruction("OP_DEFINE_GLOBAL", chunk, index);
  case OP_GET_GLOBAL:
    return print_constant_instruction("OP_GET_GLOBAL", chunk, index);
  case OP_SET_GLOBAL:
    return print_constant_instruction("OP_SET_GLOBAL", chunk, index);
  case OP_JUMP_IF_FALSE:
    return print_jump_instruction("OP_JUMP_IF_FALSE", chunk, index);
  case OP_JUMP:
    return print_jump_instruction("OP_JUMP", chunk, index);
  case OP_LOOP:
    return print_jump_instruction("OP_LOOP", chunk, index);
  default:
    printf("Unknown opcode %d\n", instruction);
    return index + 1;
  }
}

void dissasemble_chunk(Chunk *chunk, const char *chunk_name) {
  printf("==%s==\n", chunk_name);
  for (size_t i = 0; i < chunk->count;) {
    i = dissasemble_instruction(chunk, i);
  }

  print_value_array(&chunk->constants);
}
