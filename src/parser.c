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
        } else {
            // Skip to next statement on error (consume until newline-ish)
            while (current < count && tokens[current].type != TOKEN_WRITE &&
                   tokens[current].type != TOKEN_WAIT && tokens[current].type != TOKEN_IF &&
                   tokens[current].type != TOKEN_REPLY && tokens[current].type != TOKEN_SET &&
                   tokens[current].type != TOKEN_DEFINE && tokens[current].type != TOKEN_LOAD &&
                   tokens[current].type != TOKEN_CALL && tokens[current].type != TOKEN_IDENTIFIER &&
                   tokens[current].type != TOKEN_EOF) {
                current++;
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
    if (strlen(tokens[current].value) >= 64) {
        fprintf(stderr, "Parser error: block name too long (max 63 chars)\n");
        return NULL;
    }
    strcpy(block_name, tokens[current].value);
    current++;
    if (current >= count || tokens[current].type != TOKEN_AS) {
        fprintf(stderr, "Parser error: expected 'as' after block name\n");
        return NULL;
    }
    current++;
    
    Node *define_node = malloc(sizeof(Node));
    define_node->type = NODE_DEFINE;
    strcpy(define_node->var_name, block_name);
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
        } else {
            // Error already printed; try to recover
            break;
        }
    }
    if (current < count && tokens[current].type == TOKEN_END) {
        current++;
    } else {
        fprintf(stderr, "Parser error: expected 'end' to close block definition\n");
        // Still return the node so execution can continue with partial AST
    }
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
        strcpy(node->value, t.value);
        node->is_identifier = 0;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else if (t.type == TOKEN_STRING) {
        node = malloc(sizeof(Node));
        node->type = NODE_STRING;
        strcpy(node->value, t.value);
        node->is_identifier = 0;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else if (t.type == TOKEN_IDENTIFIER) {
        node = malloc(sizeof(Node));
        node->type = NODE_STRING;
        strcpy(node->value, t.value);
        node->is_identifier = 1;
        node->body = NULL;
        node->next = NULL;
        current++;
    } else {
        fprintf(stderr, "Parser error: expected expression, got token type %d\n", t.type);
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
            if (current >= count) {
                fprintf(stderr, "Parser error: expected argument after 'write'\n");
                return NULL;
            }
            Token arg = tokens[current];
            if (arg.type == TOKEN_STRING || arg.type == TOKEN_IDENTIFIER) {
                node = malloc(sizeof(Node));
                node->type = NODE_WRITE;
                if (strlen(arg.value) >= 256) {
                    fprintf(stderr, "Parser error: string too long for write (max 255)\n");
                    free(node);
                    return NULL;
                }
                strcpy(node->value, arg.value);
                node->is_identifier = (arg.type == TOKEN_IDENTIFIER);
                node->body = NULL;
                node->next = NULL;
                current++;
            } else {
                fprintf(stderr, "Parser error: expected string or identifier after 'write'\n");
            }
            break;
        }
        case TOKEN_WAIT: {
            current++;
            if (current + 1 < count &&
                tokens[current].type == TOKEN_FOR &&
                tokens[current+1].type == TOKEN_USER) {
                node = malloc(sizeof(Node));
                node->type = NODE_WAIT;
                node->value[0] = '\0';
                node->is_identifier = 0;
                node->body = NULL;
                node->next = NULL;
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
            if (current >= count) {
                fprintf(stderr, "Parser error: expected argument after 'reply'\n");
                return NULL;
            }
            Token arg = tokens[current];
            if (arg.type == TOKEN_STRING || arg.type == TOKEN_IDENTIFIER) {
                node = malloc(sizeof(Node));
                node->type = NODE_REPLY;
                if (strlen(arg.value) >= 256) {
                    fprintf(stderr, "Parser error: string too long for reply (max 255)\n");
                    free(node);
                    return NULL;
                }
                strcpy(node->value, arg.value);
                node->is_identifier = (arg.type == TOKEN_IDENTIFIER);
                node->body = NULL;
                node->next = NULL;
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
            if (strlen(tokens[current].value) >= 64) {
                fprintf(stderr, "Parser error: variable name too long (max 63)\n");
                return NULL;
            }
            strcpy(var_name, tokens[current].value);
            current++;
            if (current >= count || tokens[current].type != TOKEN_TO) {
                fprintf(stderr, "Parser error: expected 'to' after variable name\n");
                return NULL;
            }
            current++;
            if (current >= count) {
                fprintf(stderr, "Parser error: unexpected end after 'to'\n");
                return NULL;
            }
            Token val = tokens[current];
            node = malloc(sizeof(Node));
            node->type = NODE_SET;
            strcpy(node->var_name, var_name);
            node->body = NULL;
            node->next = NULL;
            if (val.type == TOKEN_STRING || val.type == TOKEN_IDENTIFIER || val.type == TOKEN_NUMBER) {
                if (strlen(val.value) >= 256) {
                    fprintf(stderr, "Parser error: value too long (max 255)\n");
                    free(node);
                    return NULL;
                }
                strcpy(node->value, val.value);
                node->is_identifier = (val.type == TOKEN_IDENTIFIER);
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
            const char *lib = tokens[current].value;
            // Security: reject path traversal (also done in interpreter)
            if (strchr(lib, '/') || strstr(lib, "..") || lib[0] == '.') {
                fprintf(stderr, "Parser error: library name must be a simple filename (no paths)\n");
                return NULL;
            }
            node = malloc(sizeof(Node));
            node->type = NODE_LOAD;
            if (strlen(lib) >= 256) {
                fprintf(stderr, "Parser error: library name too long\n");
                free(node);
                return NULL;
            }
            strcpy(node->value, lib);
            node->is_identifier = 0;
            node->body = NULL;
            node->next = NULL;
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
            if (strlen(tokens[current].value) >= 64) {
                fprintf(stderr, "Parser error: function name too long\n");
                return NULL;
            }
            strcpy(func_name, tokens[current].value);
            current++;
            node = malloc(sizeof(Node));
            node->type = NODE_FCALL;
            strcpy(node->var_name, func_name);
            node->value[0] = '\0';
            node->is_identifier = 0;
            node->body = NULL;
            node->next = NULL;
            
            // Parse arguments (only one for now)
            Node *arg_head = NULL, *arg_tail = NULL;
            while (current < count &&
                   tokens[current].type != TOKEN_WRITE && tokens[current].type != TOKEN_WAIT &&
                   tokens[current].type != TOKEN_IF && tokens[current].type != TOKEN_REPLY &&
                   tokens[current].type != TOKEN_SET && tokens[current].type != TOKEN_DEFINE &&
                   tokens[current].type != TOKEN_LOAD && tokens[current].type != TOKEN_CALL &&
                   tokens[current].type != TOKEN_IDENTIFIER &&
                   tokens[current].type != TOKEN_EOF) {
                Node *arg = parse_expression(tokens, count);
                if (arg) {
                    if (!arg_head) arg_head = arg_tail = arg;
                    else { arg_tail->next = arg; arg_tail = arg; }
                } else break;
                break; // only one argument for simplicity
            }
            node->body = arg_head;
            break;
        }
        case TOKEN_IDENTIFIER: {
            // Block call
            node = malloc(sizeof(Node));
            node->type = NODE_CALL;
            if (strlen(t.value) >= 256) {
                fprintf(stderr, "Parser error: block name too long\n");
                free(node);
                return NULL;
            }
            strcpy(node->value, t.value);
            node->is_identifier = 0;
            node->body = NULL;
            node->next = NULL;
            current++;
            break;
        }
        default:
            fprintf(stderr, "Parser error: unexpected token type %d ('%s')\n", t.type, t.value);
            current++;
            break;
    }
    return node;
}

static Node* parse_if(Token *tokens, int count) {
    current++;
    if (current + 2 >= count ||
        tokens[current].type != TOKEN_USER ||
        tokens[current+1].type != TOKEN_SAID) {
        fprintf(stderr, "Parser error: malformed 'if user said'\n");
        return NULL;
    }
    current += 2;
    if (current >= count) {
        fprintf(stderr, "Parser error: expected value after 'said'\n");
        return NULL;
    }
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
    if (strlen(cond.value) >= 256) {
        fprintf(stderr, "Parser error: condition string too long\n");
        free(if_node);
        return NULL;
    }
    strcpy(if_node->value, cond.value);
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
