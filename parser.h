#pragma once
#include "chunk.h"
#include "lexer.h"
#include "vm.h"

typedef struct {
  Token *current_token;
  Token *previous_token;
  bool had_error;
  bool panic_mode;
  Chunk *chunk;
  VM *vm;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(Parser *parser);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

bool compile(VM *vm, TokenList *token_list, Chunk *chunk);
static void grouping(Parser *parser);
static void expression(Parser *parser);
static void binary(Parser *parser);
static void unary(Parser *parser);
static void literal(Parser *parser);
static void statement(Parser *parser);
static void decleration(Parser *parser);
static void variable(Parser *parser);
