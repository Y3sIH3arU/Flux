#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKENS 2048

typedef enum {
    TOKEN_WRITE,
    // TOKEN_SAY removed
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
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
} Token;

Token next_token(const char *source, int *pos);
int lex_all(const char *source, Token *tokens);

#endif
