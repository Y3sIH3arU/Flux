#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKENS 2048

typedef enum {
    TOKEN_WRITE,
    TOKEN_WAIT,
    TOKEN_FOR,
    TOKEN_USER,
    TOKEN_IF,
    TOKEN_SAID,
    TOKEN_THEN,
    TOKEN_REPLY,
    TOKEN_SET,
    TOKEN_TO,
    TOKEN_DEFINE,
    TOKEN_AS,
    TOKEN_END,
    TOKEN_LOAD,
    TOKEN_CALL,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_UNKNOWN,
    TOKEN_MENU,
    TOKEN_OPTION,
    TOKEN_FUNCTION,
    TOKEN_FROM,
    TOKEN_USE,
    TOKEN_SAVE,
    TOKEN_CODE,
    TOKEN_LIST_OPEN,
    TOKEN_LIST_CLOSE,
    TOKEN_LIST_SEP,
    TOKEN_NUMBERS,
    TOKEN_LETTERS,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_INPUT,
    TOKEN_ELSE,
    TOKEN_ADD,
    TOKEN_TEMPLATE_START,
    TOKEN_TEMPLATE_END
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
} Token;

Token next_token(const char *source, int *pos);
int lex_all(const char *source, Token *tokens);
char* trim_whitespace(char *str);
int is_template_delimiter(const char *line);

#endif
