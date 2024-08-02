#include "lexer.h"
#include "chunk.h"
#include "hash_map.h"
#include "log_error.h"
#include "object.h"
#include "parser.h"
#include "stack.h"
#include "value.h"
#include "vm.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token create_token(TokenType type, char *lexeme, Literal literal, int line) {
  Token token;
  token.type = type;
  token.lexeme = strdup(lexeme);
  token.literal = literal;
  token.line = line;
  return token;
}

TokenList create_token_list(size_t capacity) {
  TokenList token_list;
  token_list.tokens = (Token *)malloc(sizeof(Token) * capacity);
  token_list.count = 0;
  token_list.capacity = capacity;
  return token_list;
}

void free_token_list(TokenList *token_list) { free(token_list->tokens); }

void add_token(TokenList *token_list, Token token) {
  if (token_list->count >= token_list->capacity) {
    token_list->capacity *= 2;
    token_list->tokens =
        realloc(token_list->tokens, token_list->capacity * sizeof(Token));
    if (token_list->tokens == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
  }
  token_list->tokens[token_list->count++] = token;
}

bool is_digit(char character) {
  return ('0' <= character) && (character <= '9');
}

Token handle_number(char **current_char, int line) {
  char buffer[256];
  int length = 0;
  while (**current_char && is_digit(**current_char)) {
    if (length > sizeof(buffer) - 1) {
      log_error(line, "Number cannot be longer than 256 characters");
      exit(1);
    }

    buffer[length++] = **current_char;
    (*current_char)++;
  }

  (*current_char)--;
  buffer[length] = '\0';
  int value = atoi(buffer);
  return create_token(NUMBER, buffer, (Literal){.int_value = value}, line);
}

bool is_alpha(char character) {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z') || character == '_';
}

bool is_alphanumeric(char character) {
  return is_digit(character) || is_alpha(character);
}

Token handle_identifier(char **current_char, int line) {
  char buffer[256];
  int length = 0;

  while (**current_char && is_alphanumeric(**current_char)) {
    if (length > sizeof(buffer) - 1) {
      log_error(line, "identifier exceeded max size");
      exit(1);
    }

    buffer[length++] = **current_char;
    (*current_char)++;
  }

  (*current_char)--;
  buffer[length] = '\0';
  // TODO: MOVE TO A HASHMAP
  Token token;
  if (strcmp(buffer, "var") == 0) {
    token = create_token(VAR, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "False") == 0) {
    token = create_token(FALSE, buffer, (Literal){.bool_value = false}, line);
  } else if (strcmp(buffer, "True") == 0) {
    token = create_token(TRUE, buffer, (Literal){.bool_value = true}, line);
  } else if (strcmp(buffer, "else") == 0) {
    token = create_token(ELSE, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "if") == 0) {
    token = create_token(IF, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "nil") == 0) {
    token = create_token(NIL, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "return") == 0) {
    token = create_token(NIL, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "print") == 0) {
    token = create_token(PRINT, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "while") == 0) {
    token = create_token(WHILE, buffer, (Literal){0}, line);
  } else if (strcmp(buffer, "expr") == 0) {
    token = create_token(EXPR, buffer, (Literal){0}, line);
  } else {
    token = create_token(IDENTIFIER, buffer, (Literal){.string_value = buffer},
                         line);
  }
  return token;
}

Token handle_string(char **current_char, int line) {
  char buffer[256];
  int length = 0;

  (*current_char)++;
  while (**current_char && **current_char != '"') {
    if (length > sizeof(buffer) - 1) {
      log_error(line, "string cannot be longer than 256 characters");
      exit(1);
    }

    buffer[length++] = **current_char;
    (*current_char)++;
  }

  if (!**current_char) {
    log_error(line, "unterminated string");
    exit(1);
  }

  if (length <= 0) {
    log_error(line, "cannot have empty string\n");
    exit(1);
  }

  buffer[length] = '\0';
  return create_token(STRING, buffer, (Literal){.string_value = strdup(buffer)},
                      line);
}

char look_ahead(char *current_char) { return *(current_char + 1); }

bool is_next_character_match(char **current_char, char value) {
  return look_ahead(*current_char) == value;
}

TokenList scan_tokens(char *program) {
  const int initial_token_list_size = 100;
  TokenList token_list = create_token_list(initial_token_list_size);

  int line = 1;

  char *current_char = program;
  while (*current_char) {
    switch (*current_char) {
    case ';': {
      Token token = create_token(SEMICOLON, ";", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '=': {
      Token token;
      if (is_next_character_match(&current_char, '=')) {
        Token token = create_token(EQUAL_EQUAL, "==", (Literal){0}, line);
        current_char++;
      } else {
        Token token = create_token(EQUAL, "=", (Literal){0}, line);
      }
      add_token(&token_list, token);
      break;
    }
    case '(': {
      Token token = create_token(LEFT_PAREN, "(", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case ')': {
      Token token = create_token(RIGHT_PAREN, ")", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '{': {
      Token token = create_token(LEFT_BRACKET, "{", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '}': {
      Token token = create_token(RIGHT_BRACKET, "}", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '-': {
      Token token = create_token(MINUS, "-", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '+': {
      Token token = create_token(PLUS, "+", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '/': {
      Token token = create_token(SLASH, "/", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '*': {
      Token token = create_token(STAR, "*", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '%': {
      Token token = create_token(MOD, "%", (Literal){0}, line);
      add_token(&token_list, token);
      break;
    }
    case '!': {
      Token token;
      if (is_next_character_match(&current_char, '=')) {
        token = create_token(BANG_EQUAL, "!=", (Literal){0}, line);
        current_char++;
      } else {
        token = create_token(BANG, "!", (Literal){0}, line);
      }
      add_token(&token_list, token);
      break;
    }
    case '<': {
      Token token;
      if (is_next_character_match(&current_char, '=')) {
        token = create_token(LESS_EQUAL, "<=", (Literal){0}, line);
        current_char++;
      } else {
        token = create_token(LESS, "<", (Literal){0}, line);
      }
      add_token(&token_list, token);
      break;
    }
    case '>': {
      Token token;
      if (is_next_character_match(&current_char, '=')) {
        token = create_token(GREATER_EQUAL, ">=", (Literal){0}, line);
        current_char++;
      } else {
        token = create_token(GREATER, ">", (Literal){0}, line);
      }
      add_token(&token_list, token);
      break;
    }

    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n':
      line++;
      break;
    case '"': {
      Token token = handle_string(&current_char, line);
      add_token(&token_list, token);
      break;
    }
    default: {
      Token token;
      if (is_digit(*current_char)) {
        Token token = handle_number(&current_char, line);
      } else if (is_alpha(*current_char)) {
        Token token = handle_identifier(&current_char, line);
      } else {
        log_error(line, "failed to handle value");
        exit(1);
      }
      add_token(&token_list, token);
      break;
    }
    }
    current_char++;
  }

  Token token = create_token(END, "", (Literal){0}, line);
  add_token(&token_list, token);
  return token_list;
}

void print_token(Token *token) {
  printf("Type: %d, Lexeme: %s, ", token->type, token->lexeme);

  switch (token->type) {
  case NUMBER:
    printf("Literal: %d", token->literal.int_value);
    break;
  case STRING:
    printf("Literal: %s", token->literal.string_value);
    break;
  case TRUE:
  case FALSE:
    printf("Literal: %s", token->literal.bool_value ? "True" : "False");
    break;
  default:
    printf("Literal: (not applicable)");
    break;
  }

  printf(", Line: %d\n", token->line);
}

void print_token_list(TokenList *token_list) {
  size_t num_tokens = token_list->count;
  for (size_t i = 0; i < num_tokens; i++) {
    print_token(&token_list->tokens[i]);
  }
}

char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening file");
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *content = malloc(file_size + 1);
  if (content == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return NULL;
  }

  fread(content, 1, file_size, file);
  content[file_size] = '\0';

  fclose(file);
  return content;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "provide a path to file\n");
    return 1;
  }

  char *program = read_file(argv[1]);
  TokenList token_list = scan_tokens(program);
  // print_token_list(&token_list);

  Chunk chunk;
  VM vm;

  init_chunk(&chunk);
  init_vm(&vm);
  compile(&vm, &token_list, &chunk);
  interpret(&vm, &chunk);
  free_vm(&vm);
  free_chunk(&chunk);
}
