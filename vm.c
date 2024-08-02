#include "vm.h"
#include "chunk.h"
#include "hash_map.h"
#include "log_error.h"
#include "object.h"
#include "stack.h"
#include "value.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void init_vm(VM *vm) {
  vm->ip = NULL;
  vm->chunk = NULL;
  vm->objects = NULL;
  init_stack(&vm->stack);
  init_hash_map(&vm->strings);
  init_hash_map(&vm->globals);
}

void free_object(Obj *objects) {
  Obj *object = objects;
  while (object != NULL) {
    Obj *next = object->next;
    free(object);
    object = next;
  }
}

void free_vm(VM *vm) {
  free_stack(&vm->stack);
  free_hash_map(&vm->strings);
  free_hash_map(&vm->globals);
  free_object(vm->objects);
}

uint8_t read_byte(VM *vm) { return *(vm->ip++); }

uint16_t read_short(VM *vm) {
  vm->ip += 2;
  return (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]);
}

char *concatenate(Stack *stack) {
  ObjString *b = AS_STRING(stack_pop(stack));
  ObjString *a = AS_STRING(stack_pop(stack));

  size_t length = a->length + b->length;
  char *chars = malloc(length);
  if (chars == NULL) {
    printf("ran out of memory concatenating string\n");
    exit(1);
  }
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  return chars;
}

bool binary_operation(Obj *objects, Stack *stack, OpCode op_code) {
#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    double b = AS_NUMBER(stack_pop(stack));                                    \
    double a = AS_NUMBER(stack_pop(stack));                                    \
    stack_push(stack, value_type(a op b));                                     \
  } while (false)

  if ((!IS_NUMBER(*stack_peek(stack, 0)) ||
       !IS_NUMBER(*stack_peek(stack, 1))) &&
      (!IS_STRING(*stack_peek(stack, 0)) ||
       !IS_STRING(*stack_peek(stack, 1)))) {
    return false;
  }

  switch (op_code) {
  case OP_ADD:
    if (IS_STRING(*stack_peek(stack, 0)) && IS_STRING(*stack_peek(stack, 0))) {
      char *chars = concatenate(stack);
      ObjString *result = create_string(objects, chars);
      stack_push(stack, OBJ_VAL(result));
    } else {
      BINARY_OP(NUMBER_VAL, +);
    }
    break;
  case OP_SUBTRACT:
    BINARY_OP(NUMBER_VAL, -);
    break;
  case OP_DIVIDE:
    BINARY_OP(NUMBER_VAL, /);
    break;
  case OP_MULTIPLY:
    BINARY_OP(NUMBER_VAL, *);
    break;
  case OP_MOD: {
    double b = AS_NUMBER(stack_pop(stack));
    double a = AS_NUMBER(stack_pop(stack));
    stack_push(stack, NUMBER_VAL(fmod(a, b)));
    break;
  }
  case OP_GREATER:
    if (IS_STRING(*stack_peek(stack, 0)) && IS_STRING(*stack_peek(stack, 0))) {
      ObjString *b = AS_STRING(stack_pop(stack));
      ObjString *a = AS_STRING(stack_pop(stack));
      stack_push(stack, BOOL_VAL((a->length > b->length)));
    } else {
      BINARY_OP(BOOL_VAL, >);
    }
    break;
  case OP_LESS:
    if (IS_STRING(*stack_peek(stack, 0)) && IS_STRING(*stack_peek(stack, 0))) {
      ObjString *b = AS_STRING(stack_pop(stack));
      ObjString *a = AS_STRING(stack_pop(stack));
      stack_push(stack, BOOL_VAL((a->length < b->length)));
    } else {
      BINARY_OP(BOOL_VAL, <);
    }
    break;
  default:
    return false;
  }

  return true;
#undef BINARY_OP
}

int get_current_instruction_index(VM *vm) {
  return (int)(vm->ip - vm->chunk->byte_code);
}

void log_vm_error(VM *vm, const char *message) {
  log_error(get_line(vm->chunk, get_current_instruction_index(vm)), message);
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type)
    return false;
  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  default:
    return false;
  }
}

bool is_same_type_values_equal(Value a, Value b) {
  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJ: {
    ObjString *a_string = AS_STRING(a);
    ObjString *b_string = AS_STRING(b);
    return a_string->length == b_string->length &&
           memcmp(a_string->chars, b_string->chars, a_string->length) == 0;
  }
  default:
    return false;
  }
}

InterpretResponse run(VM *vm) {
// #define VM_DEBUG
#define UNARY_OP(value_type, op)                                               \
  do {                                                                         \
    double b = AS_BOOLEAN(stack_pop(stack));                                   \
    double a = AS_NUMBER(stack_pop(stack));                                    \
    stack_push(stack, value_type(a op b));                                     \
  } while (false)

  uint8_t instruction;
  for (;;) {
#ifdef VM_DEBUG
    printf("Stack: ");
    print_stack(&vm->stack);
    dissasemble_instruction(vm->chunk, get_current_instruction_index(vm));
    printf("\n");
#endif
    instruction = read_byte(vm);
    switch (instruction) {
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    case OP_PRINT: {
      if (is_stack_empty(&vm->stack)) {
        log_vm_error(vm, "Nothing to print\n");
        return RUNTIME_ERROR;
      }
      print_value(*stack_peek(&vm->stack, 0));
      printf("\n");
      break;
    }
    case OP_CONSTANT: {
      stack_push(&vm->stack, (vm->chunk->constants.values[read_byte(vm)]));
      break;
    }
    case OP_NIL:
      stack_push(&vm->stack, NIL_VAL);
      break;
    case OP_FALSE:
      stack_push(&vm->stack, BOOL_VAL(false));
      break;
    case OP_TRUE:
      stack_push(&vm->stack, BOOL_VAL(true));
      break;
    case OP_NEGATE: {
      if (!IS_NUMBER(*stack_peek(&vm->stack, 0))) {
        log_vm_error(vm, "negation operand must be a number\n");
        return RUNTIME_ERROR;
      }
      stack_push(&vm->stack, NUMBER_VAL(-AS_NUMBER(stack_pop(&vm->stack))));
      break;
    }
    case OP_POP: {
      stack_pop(&vm->stack);
      break;
    }
    case OP_NOT: {
      if (!IS_BOOL(*stack_peek(&vm->stack, 0)) &&
          !IS_NIL(*stack_peek(&vm->stack, 0)) &&
          !IS_NUMBER(*stack_peek(&vm->stack, 0))) {
        log_vm_error(vm, "not operand must be a boolean or nil value\n");
        return RUNTIME_ERROR;
      }

      Value value = stack_pop(&vm->stack);
      bool result = (!AS_BOOL(value) && IS_BOOL(value) || IS_NIL(value)) ||
                    (!AS_NUMBER(value) && IS_NUMBER(value));
      stack_push(&vm->stack, BOOL_VAL(result));
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString *name = AS_STRING(vm->chunk->constants.values[read_byte(vm)]);
      insert_entry(&vm->globals, name, *stack_peek(&vm->stack, 0));
      stack_pop(&vm->stack);
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = AS_STRING(vm->chunk->constants.values[read_byte(vm)]);
      Value value;
      if (!get_entry(&vm->globals, name, &value)) {
        log_vm_error(vm, "Variable not found\n");
        return RUNTIME_ERROR;
      }
      stack_push(&vm->stack, value);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = AS_STRING(vm->chunk->constants.values[read_byte(vm)]);
      if (insert_entry(&vm->globals, name, *stack_peek(&vm->stack, 0))) {
        log_vm_error(vm, "Undeclared variable\n");
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_EQUAL: {
      Value b = stack_pop(&vm->stack);
      Value a = stack_pop(&vm->stack);
      if (a.type != b.type && (IS_NIL(a) || IS_NIL(b))) {
        stack_push(&vm->stack, BOOL_VAL(IS_NIL(a) && IS_NIL(b)));
      } else if (a.type == b.type) {
        stack_push(&vm->stack, BOOL_VAL(is_same_type_values_equal(a, b)));
      } else {
        log_vm_error(vm, "Cannot compare values of different types\n");
        return RUNTIME_ERROR;
      }
      break;
    }
    case OP_GREATER:
    case OP_LESS:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_DIVIDE:
    case OP_MOD:
    case OP_MULTIPLY:
      if (!binary_operation(vm->objects, &vm->stack, instruction)) {
        log_vm_error(vm, "Failed to perform arithmetic operation\n");
        return RUNTIME_ERROR;
      }
      break;
    case OP_JUMP_IF_FALSE: {
      uint16_t jump = read_short(vm);
      if (!IS_BOOL(*stack_peek(&vm->stack, 0))) {
        log_vm_error(vm,
                     "expected branch expression to evaaluate to boolean\n");
        return RUNTIME_ERROR;
      }
      if (!AS_BOOL(*stack_peek(&vm->stack, 0))) {
        vm->ip += jump;
      }
      break;
    }
    case OP_JUMP: {
      uint16_t jump = read_short(vm);
      vm->ip += jump;
      break;
    }
    case OP_LOOP: {
      uint16_t jump = read_short(vm);
      vm->ip -= jump;
      break;
    }
    default: {
      printf("unhandled instruction: %04d\n", instruction);
      return RUNTIME_ERROR;
    }
    }
  }
}

InterpretResponse interpret(VM *vm, Chunk *chunk) {
  if (!chunk->byte_code) {
    return RUNTIME_ERROR;
  }
  vm->chunk = chunk;
  vm->ip = chunk->byte_code;
  return run(vm);
}
