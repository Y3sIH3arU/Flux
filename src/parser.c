#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int current = 0;

static Node* parse_statement(Token *tokens, int count);
static Node* parse_if(Token *tokens, int count);
static Node* parse_define(Token *tokens, int count);
static Node* parse_expression(Token *tokens, int count);

Node* parse(Token *tokens, int count) {
    current = 0;
    Node *head = NULL;
    Node *tail = NULL;
    
    while (current < count && tokens[current].type != TOKEN_EOF) {
        Node *stmt = parse_statement(tokens, count);
        if (stmt) {
            if (!head) {
                head = tail = stmt;
            } else {
                tail->next = stmt;
                tail = stmt;
            }
        }
    }
    return head;
}

static Node* parse_define(Token *tokens, int count) {
    current++;
    if (current >= count || tokens[current].type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Parser error: expected block name after 'define'\n");
        return NULL;
    }
    char block_name[64];
    strncpy(block_name, tokens[current].value, 63);
    block_name[63] = '\0';
    current++;
    if (current >= count || tokens[current].type != TOKEN_AS) {
        fprintf(stderr, "Parser error: expected 'as' after block name\n");
        return NULL;
    }
    current++;
    
    Node *define_node = malloc(sizeof(Node));
    define_node->type = NODE_DEFINE;
    strncpy(define_node->var_name, block_name, 63);
    define_node->var_name[63] = '\0';
    define_node->value[0] = '\0';
    define_node->is_identifier = 0;
    define_node->body = NULL;
    define_node->next = NULL;
    
    Node *body_head = NULL, *body_tail = NULL;
    while (current < count && tokens[current].type != TOKEN_END && tokens[current].type != TOKEN_EOF) {
        Node *stmt = parse_statement(tokens, count);
        if (stmt) {
            if (!body_head) {
                body_head = body_tail = stmt;
            } else {
                body_tail->next = stmt;
                body_tail = stmt;
            }
        } else break;
    }
    if (current < count && tokens[current].type == TOKEN_END) current++;
    else fprintf(stderr, "Parser error: expected 'end'\n");
    define_node->body = body_head;
    return define_node;
}

static Node* parse_expression(Token *tokens, int count) {
    if (current >= count) return NULL;
    Token t = tokens[current];
    Node *node = NULL;
    
    if (t.type == TOKEN_NUMBER) {
        node = malloc(sizeof(Node));
        node->type = NODE_NUMBER;
        strncpy(node->value, t.value, 255);
        node->value[255] = '\0';
        node->is_identifier = 0;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else if (t.type == TOKEN_STRING) {
        node = malloc(sizeof(Node));
        node->type = NODE_STRING;
        strncpy(node->value, t.value, 255);
        node->value[255] = '\0';
        node->is_identifier = 0;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else if (t.type == TOKEN_IDENTIFIER) {
        node = malloc(sizeof(Node));
        node->type = NODE_STRING;  // treat as string expression with variable flag
        strncpy(node->value, t.value, 255);
        node->value[255] = '\0';
        node->is_identifier = 1;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else {
        fprintf(stderr, "Parser error: expected expression, got type %d\n", t.type);
    }
    return node;
}

static Node* parse_statement(Token *tokens, int count) {
    if (current >= count) return NULL;
    Token t = tokens[current];
    Node *node = NULL;
    
    switch (t.type) {
        case TOKEN_WRITE: {
            current++;
            if (current >= count) { fprintf(stderr, "Parser error: expected argument after 'write'\n"); return NULL; }
            Token arg = tokens[current];
            if (arg.type == TOKEN_STRING) {
                node = malloc(sizeof(Node));
                node->type = NODE_WRITE;
                strncpy(node->value, arg.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 0;
                current++;
            } else if (arg.type == TOKEN_IDENTIFIER) {
                node = malloc(sizeof(Node));
                node->type = NODE_WRITE;
                strncpy(node->value, arg.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 1;
                current++;
            } else {
                fprintf(stderr, "Parser error: expected string or identifier after 'write'\n");
            }
            break;
        }
        case TOKEN_WAIT: {
            current++;
            if (current + 1 < count && tokens[current].type == TOKEN_FOR && tokens[current+1].type == TOKEN_USER) {
                node = malloc(sizeof(Node));
                node->type = NODE_WAIT;
                node->value[0] = '\0';
                node->is_identifier = 0;
                current += 2;
            } else {
                fprintf(stderr, "Parser error: expected 'for user' after 'wait'\n");
            }
            break;
        }
        case TOKEN_IF: {
            node = parse_if(tokens, count);
            break;
        }
        case TOKEN_REPLY: {
            current++;
            if (current >= count) { fprintf(stderr, "Parser error: expected argument after 'reply'\n"); return NULL; }
            Token arg = tokens[current];
            if (arg.type == TOKEN_STRING) {
                node = malloc(sizeof(Node));
                node->type = NODE_REPLY;
                strncpy(node->value, arg.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 0;
                current++;
            } else if (arg.type == TOKEN_IDENTIFIER) {
                node = malloc(sizeof(Node));
                node->type = NODE_REPLY;
                strncpy(node->value, arg.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 1;
                current++;
            } else {
                fprintf(stderr, "Parser error: expected string or identifier after 'reply'\n");
            }
            break;
        }
        case TOKEN_SET: {
            current++;
            if (current >= count || tokens[current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Parser error: expected variable name after 'set'\n");
                return NULL;
            }
            char var_name[64];
            strncpy(var_name, tokens[current].value, 63);
            var_name[63] = '\0';
            current++;
            if (current >= count || tokens[current].type != TOKEN_TO) {
                fprintf(stderr, "Parser error: expected 'to' after variable name\n");
                return NULL;
            }
            current++;
            if (current >= count) { fprintf(stderr, "Parser error: unexpected end after 'to'\n"); return NULL; }
            Token val = tokens[current];
            node = malloc(sizeof(Node));
            node->type = NODE_SET;
            strncpy(node->var_name, var_name, 63);
            node->var_name[63] = '\0';
            if (val.type == TOKEN_STRING) {
                strncpy(node->value, val.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 0;
            } else if (val.type == TOKEN_IDENTIFIER) {
                strncpy(node->value, val.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 1;
            } else if (val.type == TOKEN_NUMBER) {
                strncpy(node->value, val.value, 255);
                node->value[255] = '\0';
                node->is_identifier = 0;
            } else {
                fprintf(stderr, "Parser error: expected string, number, or identifier after 'to'\n");
                free(node);
                return NULL;
            }
            current++;
            break;
        }
        case TOKEN_DEFINE: {
            node = parse_define(tokens, count);
            break;
        }
        case TOKEN_LOAD: {
            current++;
            if (current >= count || tokens[current].type != TOKEN_STRING) {
                fprintf(stderr, "Parser error: expected library path string after 'load'\n");
                return NULL;
            }
            node = malloc(sizeof(Node));
            node->type = NODE_LOAD;
            strncpy(node->value, tokens[current].value, 255);
            node->value[255] = '\0';
            current++;
            break;
        }
        case TOKEN_CALL: {
            current++;
            if (current >= count || tokens[current].type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Parser error: expected function name after 'call'\n");
                return NULL;
            }
            char func_name[64];
            strncpy(func_name, tokens[current].value, 63);
            func_name[63] = '\0';
            current++;
            node = malloc(sizeof(Node));
            node->type = NODE_FCALL;
            strncpy(node->var_name, func_name, 63);
            node->var_name[63] = '\0';
            node->value[0] = '\0';
            // Parse arguments (expressions)
            Node *arg_head = NULL, *arg_tail = NULL;
            while (current < count &&
                   tokens[current].type != TOKEN_WRITE && tokens[current].type != TOKEN_WAIT &&
                   tokens[current].type != TOKEN_IF && tokens[current].type != TOKEN_REPLY &&
                   tokens[current].type != TOKEN_SET && tokens[current].type != TOKEN_DEFINE &&
                   tokens[current].type != TOKEN_LOAD && tokens[current].type != TOKEN_CALL &&
                   tokens[current].type != TOKEN_IDENTIFIER && // statement-level identifier could be block call
                   tokens[current].type != TOKEN_EOF) {
                Node *arg = parse_expression(tokens, count);
                if (arg) {
                    if (!arg_head) arg_head = arg_tail = arg;
                    else { arg_tail->next = arg; arg_tail = arg; }
                } else break;
                // For simplicity, we only support one argument for now
                break;
            }
            node->body = arg_head;
            break;
        }
        case TOKEN_IDENTIFIER: {
            // Block call at statement level
            node = malloc(sizeof(Node));
            node->type = NODE_CALL;
            strncpy(node->value, t.value, 255);
            node->value[255] = '\0';
            node->is_identifier = 0;
            current++;
            break;
        }
        default:
            fprintf(stderr, "Parser error: unexpected token type %d\n", t.type);
            current++;
            break;
    }
    if (node) {
        node->body = NULL;
        node->next = NULL;
    }
    return node;
}

static Node* parse_if(Token *tokens, int count) {
    current++;
    if (current + 2 >= count || tokens[current].type != TOKEN_USER || tokens[current+1].type != TOKEN_SAID) {
        fprintf(stderr, "Parser error: malformed 'if user said'\n");
        return NULL;
    }
    current += 2;
    if (current >= count) { fprintf(stderr, "Parser error: expected value after 'said'\n"); return NULL; }
    Token cond = tokens[current];
    if (cond.type != TOKEN_STRING && cond.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Parser error: expected string or identifier after 'said'\n");
        return NULL;
    }
    current++;
    if (current >= count || tokens[current].type != TOKEN_THEN) {
        fprintf(stderr, "Parser error: expected 'then'\n");
        return NULL;
    }
    current++;
    Node *if_node = malloc(sizeof(Node));
    if_node->type = NODE_IF_SAID;
    strncpy(if_node->value, cond.value, 255);
    if_node->value[255] = '\0';
    if_node->is_identifier = (cond.type == TOKEN_IDENTIFIER);
    if_node->body = parse_statement(tokens, count);
    if_node->next = NULL;
    return if_node;
}

void free_ast(Node *node) {
    while (node) {
        Node *next = node->next;
        if (node->body) free_ast(node->body);
        free(node);
        node = next;
    }
}
