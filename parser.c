#include "parser.h"
#include "chunk.h"
#include "hash_map.h"
#include "lexer.h"
#include "log_error.h"
#include "object.h"
#include "value.h"
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ParseRule rules[] = {
    [LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [MINUS] = {unary, binary, PREC_TERM},
    [BANG] = {unary, NULL, PREC_NONE},
    [PLUS] = {NULL, binary, PREC_TERM},
    [MOD] = {NULL, binary, PREC_TERM},
    [EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [BANG_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [GREATER] = {NULL, binary, PREC_COMPARISON},
    [GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [LESS] = {NULL, binary, PREC_COMPARISON},
    [LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [SEMICOLON] = {NULL, NULL, PREC_NONE},
    [SLASH] = {NULL, binary, PREC_FACTOR},
    [STAR] = {NULL, binary, PREC_FACTOR},
    [NUMBER] = {literal, NULL, PREC_NONE},
    [FALSE] = {literal, NULL, PREC_NONE},
    [NIL] = {literal, NULL, PREC_NONE},
    [TRUE] = {literal, NULL, PREC_NONE},
    [STRING] = {literal, NULL, PREC_NONE},
    [IDENTIFIER] = {variable, NULL, PREC_NONE},
};

void init_parser(Parser *parser) {
  parser->current_token = NULL;
  parser->previous_token = NULL;
  parser->chunk = NULL;
  parser->had_error = false;
  parser->panic_mode = false;
  parser->vm = NULL;
}

static void report_parsing_error(Parser *parser) {
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "Error at token: %s",
           parser->current_token->lexeme);
  log_error(parser->current_token->line, buffer);
}

static void advance(Parser *parser) {
  parser->previous_token = parser->current_token;
  for (;;) {
    parser->current_token += 1;
    if (parser->current_token->type != ERROR)
      break;

    parser->had_error = true;
    if (!parser->panic_mode) {
      parser->panic_mode = true;
      report_parsing_error(parser);
    }
  }
}

static void consume(Parser *parser, TokenType type, const char *message) {
  if (parser->current_token->type == type) {
    advance(parser);
    return;
  }

  log_error(parser->current_token->line, message);
}

static void emit_byte(Parser *parser, uint8_t byte) {
  write_chunk(parser->chunk, byte, parser->previous_token->line);
}

static void emit_bytes(Parser *parser, int num_bytes, ...) {
  va_list args;
  va_start(args, num_bytes);

  for (int i = 0; i < num_bytes; i++) {
    uint8_t byte = (uint8_t)va_arg(args, int);
    emit_byte(parser, byte);
  }

  va_end(args);
}

static void emit_constant(Parser *parser, Value value) {
  int constant_index = add_constant(parser->chunk, value);
  emit_bytes(parser, 2, OP_CONSTANT, constant_index);
}

static void emit_return(Parser *parser) { emit_byte(parser, OP_RETURN); }

static void literal(Parser *parser) {
  switch (parser->previous_token->type) {
  case NUMBER:
    emit_constant(parser,
                  NUMBER_VAL(strtod(parser->previous_token->lexeme, NULL)));
    break;
  case FALSE:
    emit_byte(parser, OP_FALSE);
    break;
  case TRUE:
    emit_byte(parser, OP_TRUE);
    break;
  case NIL:
    emit_byte(parser, OP_NIL);
    break;
  case STRING: {
    ObjString *string =
        create_string(parser->vm->objects, parser->previous_token->lexeme);
    emit_constant(parser, OBJ_VAL(string));
    break;
  }
  default:
    return;
  }
}

static void grouping(Parser *parser) {
  expression(parser);
  consume(parser, RIGHT_PAREN, "");
}

static ParseRule *get_rule(TokenType token_type) { return &rules[token_type]; }

static void parse_precedence(Parser *parser, Precedence prescedence) {
  advance(parser);
  ParseFn prefix_rule = get_rule(parser->previous_token->type)->prefix;
  if (!prefix_rule) {
    log_error(parser->previous_token->line, "expected expression");
    return;
  }

  prefix_rule(parser);

  while (prescedence <= get_rule(parser->current_token->type)->precedence) {
    advance(parser);
    ParseFn infix_rule = get_rule(parser->previous_token->type)->infix;
    infix_rule(parser);
  }
}

static void unary(Parser *parser) {
  TokenType operator_type = parser->previous_token->type;

  parse_precedence(parser, PREC_UNARY);

  switch (operator_type) {
  case MINUS:
    emit_byte(parser, OP_NEGATE);
    break;
  case BANG:
    emit_byte(parser, OP_NOT);
    break;
  default:
    return;
  }
}

static void binary(Parser *parser) {
  TokenType operator_type = parser->previous_token->type;
  ParseRule *rule = get_rule(operator_type);
  // we only want to evaluate stuff with one precedence above so if we add we
  // only evalaute multiplication and above to the right
  parse_precedence(parser, (Precedence)(rule->precedence + 1));

  switch (operator_type) {
  case GREATER:
    emit_byte(parser, OP_GREATER);
    break;
  case GREATER_EQUAL:
    emit_bytes(parser, 2, OP_LESS, OP_NOT);
    break;
  case LESS:
    emit_byte(parser, OP_LESS);
    break;
  case LESS_EQUAL:
    emit_bytes(parser, 2, OP_GREATER, OP_NOT);
    break;
  case PLUS:
    emit_byte(parser, OP_ADD);
    break;
  case MINUS:
    emit_byte(parser, OP_SUBTRACT);
    break;
  case STAR:
    emit_byte(parser, OP_MULTIPLY);
    break;
  case SLASH:
    emit_byte(parser, OP_DIVIDE);
    break;
  case MOD:
    emit_byte(parser, OP_MOD);
    break;
  case EQUAL_EQUAL:
    emit_byte(parser, OP_EQUAL);
    break;
  case BANG_EQUAL:
    emit_bytes(parser, 2, OP_EQUAL, OP_NOT);
  default:
    return;
  }
}

static void variable(Parser *parser) {
  uint8_t arg_index = add_constant(
      parser->chunk, OBJ_VAL(create_string(parser->vm->objects,
                                           parser->previous_token->lexeme)));
  if (parser->current_token->type == EQUAL) {
    advance(parser);
    expression(parser);
    emit_bytes(parser, 2, OP_SET_GLOBAL, arg_index);
  } else {
    emit_bytes(parser, 2, OP_GET_GLOBAL, arg_index);
  }
}

static void expression(Parser *parser) {
  parse_precedence(parser, PREC_ASSIGNMENT);
}

static void print_statement(Parser *parser) {
  expression(parser);
  consume(parser, SEMICOLON, "expected semicolon after print statment\n");
  emit_byte(parser, OP_PRINT);
}

static void expression_statement(Parser *parser) {
  expression(parser);
  consume(parser, SEMICOLON, "expected semicolon after expression statment\n");
  emit_byte(parser, OP_POP);
}

// returns the placeholder index in chunk
static int emit_jump(Parser *parser, uint8_t instruction) {
  const uint8_t placeholder = 0xff;
  emit_bytes(parser, 3, instruction, placeholder, placeholder);
  return parser->chunk->count - 2;
}

static void patch_jump(Parser *parser, int placeholder_index) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = parser->chunk->count - placeholder_index - 2;
  parser->chunk->byte_code[placeholder_index] = (jump >> 8) & 0xff;
  parser->chunk->byte_code[placeholder_index + 1] = jump & 0xff;
}

static void if_statement(Parser *parser) {
  consume(parser, LEFT_PAREN, "expected a '(' after if statement\n");
  expression(parser);
  consume(parser, RIGHT_PAREN, "expected a ')' after expression\n");
  advance(parser);

  int placeholder = emit_jump(parser, OP_JUMP_IF_FALSE);
  emit_byte(parser, OP_POP);
  statement(parser);

  int else_placeholder = emit_jump(parser, OP_JUMP);

  patch_jump(parser, placeholder);
  emit_byte(parser, OP_POP);

  if (parser->current_token->type == ELSE) {
    advance(parser);
    advance(parser);
    statement(parser);
  }
  patch_jump(parser, else_placeholder);
}

static void emit_loop(Parser *parser, int loop_start) {
  const uint8_t placeholder = 0xff;
  emit_byte(parser, OP_LOOP);

  int offset = parser->chunk->count - loop_start + 2;

  emit_byte(parser, (offset >> 8) & placeholder);
  emit_byte(parser, offset & placeholder);
}

static void while_statement(Parser *parser) {
  int loop_start = parser->chunk->count;
  consume(parser, LEFT_PAREN, "expected a '(' after if statement\n");
  expression(parser);
  consume(parser, RIGHT_PAREN, "expected a ')' after expression\n");
  advance(parser);

  int exit_jump = emit_jump(parser, OP_JUMP_IF_FALSE);
  emit_byte(parser, OP_POP);
  statement(parser);
  emit_loop(parser, loop_start);

  patch_jump(parser, exit_jump);
}

static void block_statement(Parser *parser) {
  while (parser->current_token->type != RIGHT_BRACKET) {
    advance(parser);
    statement(parser);
  }
}

static void statement(Parser *parser) {
  if (parser->previous_token->type == PRINT) {
    print_statement(parser);
  } else if (parser->previous_token->type == IF) {
    if_statement(parser);
  } else if (parser->previous_token->type == WHILE) {
    while_statement(parser);
  } else if (parser->previous_token->type == LEFT_BRACKET) {
    block_statement(parser);
  } else if (parser->previous_token->type == EXPR) {
    expression_statement(parser);
  }
}

static void synchronize(Parser *parser) {
  parser->panic_mode = false;

  while (parser->current_token->type != END) {
    if (parser->previous_token->type == SEMICOLON)
      return;
    switch (parser->current_token->type) {
    case VAR:
    case IF:
    case PRINT:
    case RETURN:
      return;

    default:;
    }

    advance(parser);
  }
}

static void variable_decleration(Parser *parser) {
  consume(parser, IDENTIFIER, "expected identifier after var\n");
  uint8_t variable_index = add_constant(
      parser->chunk, OBJ_VAL(create_string(parser->vm->objects,
                                           parser->previous_token->lexeme)));

  if (parser->current_token->type == EQUAL) {
    advance(parser);
    expression(parser);
  } else {
    emit_byte(parser, OP_NIL);
  }

  consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
  emit_bytes(parser, 2, OP_DEFINE_GLOBAL, variable_index);
}

static void declaration(Parser *parser) {
  if (parser->previous_token->type == VAR) {
    variable_decleration(parser);
  } else {
    statement(parser);
  }
  if (parser->panic_mode)
    synchronize(parser);
}

bool compile(VM *vm, TokenList *token_list, Chunk *chunk) {
  Parser parser;
  init_parser(&parser);

  // print_token_list(token_list);
  parser.current_token = token_list->tokens;
  parser.chunk = chunk;
  parser.vm = vm;

  while (!(parser.current_token->type == END)) {
    advance(&parser);
    declaration(&parser);
  }

  emit_return(&parser);
  consume(&parser, END, "failed to reach end of code\n");

  return !parser.had_error;
}
