#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define MAX_TEMPLATES 64
#define TEMPLATE_SIZE 8192
#define MAX_LINE_LENGTH 1024

static int current = 0;
static ParserState parser_state = STATE_NORMAL;
static char template_buffer[TEMPLATE_SIZE] = {0};
static char current_codestring_name[64] = {0};
static int template_pos = 0;

static Node* parse_statement(Token *tokens, int count);
static Node* parse_if(Token *tokens, int count);
static Node* parse_define(Token *tokens, int count);
static Node* parse_expression(Token *tokens, int count);
static Node* parse_menu(Token *tokens, int count);
static Node* parse_option(Token *tokens, int count);
static Node* parse_function(Token *tokens, int count);
static Node* parse_iterate(Token *tokens, int count);
static Node* parse_save(Token *tokens, int count);
static Node* parse_input_check(Token *tokens, int count);
Node* parse_codestring(const char *code);

static void reset_template_state(void) {
    template_buffer[0] = '\0';
    current_codestring_name[0] = '\0';
    template_pos = 0;
    parser_state = STATE_NORMAL;
}

static void append_to_template(const char *line) {
    size_t line_len = strlen(line);
    if (template_pos + line_len + 1 > TEMPLATE_SIZE) {
        fprintf(stderr, "Template overflow: template exceeds %d bytes\n", TEMPLATE_SIZE);
        // Stop accumulating to prevent overflow
        return;
    }
    memcpy(template_buffer + template_pos, line, line_len);
    template_pos += line_len;
    template_buffer[template_pos] = '\0'; // Ensure null termination, though not necessary if line includes it
}

Node* parse_codestring(const char *code) {
    Token tokens[2048];
    int count = lex_all(code, tokens);
    current = 0;
    Node *head = NULL, *tail = NULL;
    
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
            while (current < count && tokens[current].type != TOKEN_WRITE &&
                   tokens[current].type != TOKEN_WAIT &&
                   tokens[current].type != TOKEN_IF &&
                   tokens[current].type != TOKEN_REPLY &&
                   tokens[current].type != TOKEN_SET &&
                   tokens[current].type != TOKEN_DEFINE &&
                   tokens[current].type != TOKEN_LOAD &&
                   tokens[current].type != TOKEN_CALL &&
                   tokens[current].type != TOKEN_IDENTIFIER &&
                   tokens[current].type != TOKEN_EOF) {
                current++;
            }
        }
    }
    return head;
}

Node* parse_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return NULL;
    }
    
    reset_template_state();
    Node *head = NULL, *tail = NULL;
    char line[MAX_LINE_LENGTH];
    char current_func_name[64] = {0};
    int in_template = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        if (!trimmed || *trimmed == '\0') continue;

        if (in_template) {
            if (is_template_delimiter(trimmed)) {
                fprintf(stderr, "DEBUG: closing template, state=%d\n", parser_state);
                in_template = 0;
                parser_state = STATE_EXPECT_FUNC;
                continue;
            }
            append_to_template(line);
            continue;
        }
        
        if (is_template_delimiter(trimmed)) {
            in_template = 1;
            parser_state = STATE_IN_TEMPLATE;
            template_buffer[0] = '\0';
            template_pos = 0;
            continue;
        }
        
        if (parser_state == STATE_EXPECT_FUNC) {
            fprintf(stderr, "DEBUG STATE_EXPECT: trimmed='%s', len=%zu\n", trimmed, strlen(trimmed));
            fprintf(stderr, "DEBUG STATE_EXPECT: first17='%.*s'\n", 17, trimmed);
            if (strncmp(trimmed, "function add + (\"", 17) == 0) {
                char *start = strchr(trimmed + 17, '"');
                if (start) {
                    start++;
                    char *end = strchr(start, '"');
                    if (end) {
                        *end = '\0';
                        if (strlen(start) >= sizeof(current_func_name)) {
                            fprintf(stderr, "Parser error: function name too long\n");
                            parser_state = STATE_NORMAL;
                            continue;
                        }
                        strcpy(current_func_name, start);
                        
                        Node *func_node = malloc(sizeof(Node));
                        func_node->type = NODE_CODESTRING;
                        strcpy(func_node->var_name, current_func_name);
                        strcpy(func_node->value, template_buffer);
                        func_node->body = NULL;
                        func_node->next = NULL;
                        
                        if (!head) {
                            head = tail = func_node;
                        } else {
                            tail->next = func_node;
                            tail = func_node;
                        }
                        parser_state = STATE_NORMAL;
                        continue;
                    }
                }
            } else if (trimmed[0] == '\0' || trimmed[0] == '#') {
                continue;
            } else {
                fprintf(stderr, "Parser error: expected 'function add + (\"NAME\")' after template\n");
                parser_state = STATE_NORMAL;
                continue;
            }
        }
        
        Token tokens[2048];
        int count = lex_all(line, tokens);
        current = 0;
        
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
                while (current < count && tokens[current].type != TOKEN_WRITE &&
                       tokens[current].type != TOKEN_WAIT &&
                       tokens[current].type != TOKEN_IF &&
                       tokens[current].type != TOKEN_REPLY &&
                       tokens[current].type != TOKEN_SET &&
                       tokens[current].type != TOKEN_DEFINE &&
                       tokens[current].type != TOKEN_LOAD &&
                       tokens[current].type != TOKEN_CALL &&
                       tokens[current].type != TOKEN_MENU &&
                       tokens[current].type != TOKEN_SAVE &&
                       tokens[current].type != TOKEN_FUNCTION &&
                       tokens[current].type != TOKEN_INPUT &&
                       tokens[current].type != TOKEN_IDENTIFIER &&
                       tokens[current].type != TOKEN_EOF) {
                    current++;
                }
                if (current < count) current++;
            }
        }
    }
    
    fclose(fp);
    
    if (parser_state == STATE_IN_TEMPLATE) {
        fprintf(stderr, "Parser error: unclosed template (missing closing '\\-')\n");
    }
    
    return head;
}

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
    if (!define_node) return NULL;
    define_node->type = NODE_DEFINE;
    strcpy(define_node->var_name, block_name);
    define_node->value[0] = '\0';
    define_node->is_identifier = 0;
    define_node->body = NULL;
    define_node->else_body = NULL;
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
        free_ast(define_node);
        return NULL;
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
            if (!lib || !*lib) {
                fprintf(stderr, "Parser error: library name cannot be empty\n");
                return NULL;
            }
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
        case TOKEN_MENU: {
            node = parse_menu(tokens, count);
            break;
        }
        case TOKEN_SAVE: {
            node = parse_save(tokens, count);
            break;
        }
        case TOKEN_FUNCTION: {
            node = parse_function(tokens, count);
            break;
        }
        case TOKEN_NUMBERS:
        case TOKEN_LETTERS: {
            node = parse_expression(tokens, count);
            break;
        }
        case TOKEN_INPUT: {
            node = parse_input_check(tokens, count);
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
    if (!if_node) return NULL;
    if_node->type = NODE_IF_SAID;
    if (strlen(cond.value) >= 256) {
        fprintf(stderr, "Parser error: condition string too long\n");
        free(if_node);
        return NULL;
    }
    strcpy(if_node->value, cond.value);
    if_node->is_identifier = (cond.type == TOKEN_IDENTIFIER);
    if_node->body = parse_statement(tokens, count);
    if_node->else_body = NULL;
    if_node->next = NULL;
    if (current < count && tokens[current].type == TOKEN_ELSE) {
        current++;
        if_node->else_body = parse_statement(tokens, count);
    }
    return if_node;
}

static Node* parse_menu(Token *tokens, int count) {
    current++;
    Node *menu_node = malloc(sizeof(Node));
    menu_node->type = NODE_MENU;
    menu_node->value[0] = '\0';
    menu_node->is_identifier = 0;
    menu_node->body = NULL;
    menu_node->next = NULL;
    
    if (current >= count || tokens[current].type != TOKEN_LIST_OPEN) {
        free(menu_node);
        return NULL;
    }
    current++;
    
    Node *item_head = NULL, *item_tail = NULL;
    while (current < count && tokens[current].type != TOKEN_LIST_CLOSE && tokens[current].type != TOKEN_EOF) {
        if (tokens[current].type == TOKEN_STRING) {
            Node *item = malloc(sizeof(Node));
            item->type = NODE_OPTION;
            strcpy(item->value, tokens[current].value);
            item->is_identifier = 0;
            item->body = NULL;
            item->next = NULL;
            
            if (!item_head) item_head = item_tail = item;
            else { item_tail->next = item; item_tail = item; }
        }
        current++;
    }
    if (current < count && tokens[current].type == TOKEN_LIST_CLOSE) current++;
    menu_node->body = item_head;
    return menu_node;
}

static Node* parse_save(Token *tokens, int count) {
    current++;
    Node *save_node = malloc(sizeof(Node));
    save_node->type = NODE_SAVE;
    save_node->body = NULL;
    save_node->next = NULL;
    
    if (current >= count) {
        fprintf(stderr, "Parser error: expected codestring name after 'save'\n");
        free(save_node);
        return NULL;
    }
    
    if (tokens[current].type == TOKEN_CODE) {
        current++;
        if (current >= count || tokens[current].type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Parser error: expected codestring identifier\n");
            free(save_node);
            return NULL;
        }
        strcpy(save_node->var_name, tokens[current].value);
        save_node->value[0] = '\0';
        save_node->is_identifier = 0;
        current++;
    } else if (tokens[current].type == TOKEN_IDENTIFIER) {
        strcpy(save_node->var_name, tokens[current].value);
        current++;
        if (current < count && tokens[current].type == TOKEN_AS) {
            current++;
            if (current < count && tokens[current].type == TOKEN_STRING) {
                strcpy(save_node->value, tokens[current].value);
                current++;
            } else if (current < count && tokens[current].type == TOKEN_IDENTIFIER) {
                strcpy(save_node->value, tokens[current].value);
                save_node->is_identifier = 1;
                current++;
            }
        }
    }
    return save_node;
}

static Node* parse_input_check(Token *tokens, int count) {
    Node *input_node = malloc(sizeof(Node));
    input_node->type = NODE_INPUT_CHECK;
    input_node->body = NULL;
    input_node->next = NULL;
    
    if (tokens[current].type == TOKEN_INPUT) {
        current++;
        if (current < count && (tokens[current].type == TOKEN_STRING || tokens[current].type == TOKEN_IDENTIFIER)) {
            strcpy(input_node->var_name, tokens[current].value);
            input_node->is_identifier = (tokens[current].type == TOKEN_IDENTIFIER);
            current++;
        } else {
            input_node->var_name[0] = '\0';
            input_node->is_identifier = 0;
        }
    }
    
    if (current < count && tokens[current].type == TOKEN_USE) {
        current++;
        if (current < count) {
            input_node->body = parse_statement(tokens, count);
        }
    }
    
    return input_node;
}

static Node* parse_function(Token *tokens, int count) {
    current++;
    Node *func_node = malloc(sizeof(Node));
    func_node->type = NODE_FUNCTION;
    func_node->body = NULL;
    func_node->next = NULL;
    
    if (current >= count) {
        free(func_node);
        return NULL;
    }
    
    if (tokens[current].type == TOKEN_ADD) {
        current++;
        if (current >= count || tokens[current].type != TOKEN_LIST_OPEN) {
            free(func_node);
            return NULL;
        }
        current++;
        if (current < count) {
            Node *data = parse_expression(tokens, count);
            func_node->body = data;
        }
        while (current < count && tokens[current].type != TOKEN_LIST_CLOSE) current++;
        if (current < count) current++;
    } else if (tokens[current].type == TOKEN_USE) {
        current++;
        if (current < count && tokens[current].type == TOKEN_IDENTIFIER) {
            strcpy(func_node->value, tokens[current].value);
            current++;
            if (current < count && tokens[current].type == TOKEN_FROM) {
                current++;
                if (current < count && tokens[current].type == TOKEN_CODE) {
                    current++;
                    if (current < count && tokens[current].type == TOKEN_IDENTIFIER) {
                        strcpy(func_node->var_name, tokens[current].value);
                        current++;
                    }
                }
            }
        }
    } else if (tokens[current].type == TOKEN_IDENTIFIER) {
        strcpy(func_node->value, tokens[current].value);
        current++;
    }
    
    return func_node;
}

void free_ast(Node *node) {
    while (node) {
        Node *next = node->next;
        if (node->body) free_ast(node->body);
        if (node->else_body) free_ast(node->else_body);
        free(node);
        node = next;
    }
}
