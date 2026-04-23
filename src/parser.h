#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    NODE_WRITE,
    NODE_WAIT,
    NODE_IF_SAID,
    NODE_REPLY,
    NODE_SET,
    NODE_DEFINE,
    NODE_CALL,
    NODE_LOAD,
    NODE_FCALL,
    NODE_NUMBER,
    NODE_STRING,
    NODE_MENU,
    NODE_OPTION,
    NODE_FUNCTION,
    NODE_ITERATE,
    NODE_SAVE,
    NODE_INPUT_CHECK,
    NODE_LIST,
    NODE_CODESTRING
} NodeType;

typedef enum {
    STATE_NORMAL,
    STATE_IN_TEMPLATE,
    STATE_EXPECT_FUNC
} ParserState;

typedef struct Node {
    NodeType type;
    char var_name[64];
    char value[256];
    int is_identifier;
    struct Node *body;
    struct Node *next;
} Node;

Node* parse(Token *tokens, int count);
Node* parse_file(const char *filename);
Node* parse_codestring(const char *code);
void free_ast(Node *node);

#endif
