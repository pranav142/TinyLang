#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACKET,
  RIGHT_BRACKET,
  SEMICOLON,

  // math
  MINUS,
  PLUS,
  SLASH,
  STAR,
  MOD,

  EQUAL,
  BANG,
  BANG_EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,

  // literals
  NUMBER,
  IDENTIFIER,
  STRING,

  // keywords
  VAR,
  FALSE,
  TRUE,
  IF,
  ELSE,
  NIL,
  PRINT,
  RETURN,
  WHILE,
  EXPR,

  ERROR,
  END,
} TokenType;

typedef union {
  int int_value;
  char *string_value;
  bool bool_value;
  void *null_value;
} Literal;

typedef struct {
  TokenType type;
  char *lexeme;
  Literal literal;
  int line;
} Token;

typedef struct {
  Token *tokens;
  size_t count;
  size_t capacity;
} TokenList;

Token create_token(TokenType type, char *lexeme, Literal literal, int line);
TokenList create_token_list(size_t capacity);
void free_token_list(TokenList *token_list);
void add_token(TokenList *token_list, Token token);
bool is_digit(char character);
Token handle_number(char **current_char, int line);
bool is_alpha(char character);
bool is_alphanumeric(char character);
Token handle_identifier(char **current_char, int line);
Token handle_string(char **current_char, int line);
char look_ahead(char *current_char);
bool is_next_character_match(char **current_char, char value);
TokenList scan_tokens(char *program);
void print_token(Token *token);
void print_token_list(TokenList *token_list);
char *read_file(const char *filename);
