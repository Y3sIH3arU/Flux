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
    NODE_CALL,      // block call
    NODE_LOAD,
    NODE_FCALL,     // foreign call
    NODE_NUMBER,
    NODE_STRING
} NodeType;

typedef struct Node {
    NodeType type;
    char var_name[64];
    char value[256];
    int is_identifier;
    struct Node *body;
    struct Node *next;
} Node;

Node* parse(Token *tokens, int count);
void free_ast(Node *node);

#endif
