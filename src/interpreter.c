#define _GNU_SOURCE
#include "interpreter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <math.h>

static char last_input[256] = "";

#define MAX_VARS 256
typedef struct {
    char name[64];
    char value[256];
} Variable;
static Variable vars[MAX_VARS];
static int var_count = 0;

#define MAX_BLOCKS 256
typedef struct {
    char name[64];
    Node *body;
} Block;
static Block blocks[MAX_BLOCKS];
static int block_count = 0;

#define MAX_LIBS 16
static void* lib_handles[MAX_LIBS];
static int lib_count = 0;

// Safe string copy
static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

// Temporary buffer for expression results (ring buffer, no leak)
#define TMP_BUF_SIZE 4
static char tmp_buf[TMP_BUF_SIZE][256];
static int tmp_idx = 0;

static const char* eval_expression(Node *expr) {
    if (!expr) return "";
    char *buf = tmp_buf[tmp_idx];
    tmp_idx = (tmp_idx + 1) % TMP_BUF_SIZE;
    buf[0] = '\0';
    
    switch (expr->type) {
        case NODE_STRING:
            if (expr->is_identifier) {
                char *val = NULL;
                for (int i = 0; i < var_count; i++) {
                    if (strcmp(vars[i].name, expr->value) == 0) {
                        val = vars[i].value;
                        break;
                    }
                }
                if (val) safe_strcpy(buf, val, 256);
            } else {
                safe_strcpy(buf, expr->value, 256);
            }
            break;
        case NODE_NUMBER:
            safe_strcpy(buf, expr->value, 256);
            break;
        default:
            break;
    }
    return buf;
}

static char* get_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, name) == 0) return vars[i].value;
    return NULL;
}

static void set_var(const char *name, const char *value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            safe_strcpy(vars[i].value, value, 256);
            return;
        }
    }
    if (var_count < MAX_VARS) {
        safe_strcpy(vars[var_count].name, name, 64);
        safe_strcpy(vars[var_count].value, value, 256);
        var_count++;
    }
}

static Node* get_block(const char *name) {
    for (int i = 0; i < block_count; i++)
        if (strcmp(blocks[i].name, name) == 0) return blocks[i].body;
    return NULL;
}

static void define_block(const char *name, Node *body) {
    for (int i = 0; i < block_count; i++) {
        if (strcmp(blocks[i].name, name) == 0) {
            blocks[i].body = body;
            return;
        }
    }
    if (block_count < MAX_BLOCKS) {
        safe_strcpy(blocks[block_count].name, name, 64);
        blocks[block_count].body = body;
        block_count++;
    }
}

static void call_foreign_function(const char *name, Node *args) {
    void *func_ptr = NULL;
    for (int i = 0; i < lib_count; i++) {
        func_ptr = dlsym(lib_handles[i], name);
        if (func_ptr) break;
    }
    if (!func_ptr) func_ptr = dlsym(RTLD_DEFAULT, name);
    if (!func_ptr) {
        fprintf(stderr, "Runtime error: function '%s' not found\n", name);
        return;
    }
    
    if (strcmp(name, "sqrt") == 0) {
        if (!args) { fprintf(stderr, "sqrt expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        double (*f)(double) = (double (*)(double))func_ptr;
        printf("%g\n", f(x));
    } else if (strcmp(name, "puts") == 0) {
        if (!args) { fprintf(stderr, "puts expects one argument\n"); return; }
        const char *arg = eval_expression(args);
        int (*f)(const char*) = (int (*)(const char*))func_ptr;
        f(arg);
    } else if (strcmp(name, "sin") == 0) {
        if (!args) { fprintf(stderr, "sin expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        double (*f)(double) = (double (*)(double))func_ptr;
        printf("%g\n", f(x));
    } else if (strcmp(name, "cos") == 0) {
        if (!args) { fprintf(stderr, "cos expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        double (*f)(double) = (double (*)(double))func_ptr;
        printf("%g\n", f(x));
    } else {
        fprintf(stderr, "Runtime error: unsupported function '%s'\n", name);
    }
}

void execute(Node *node) {
    while (node) {
        switch (node->type) {
            case NODE_WRITE: {
                const char *out = node->value;
                if (node->is_identifier) {
                    char *v = get_var(node->value);
                    if (v) out = v; else out = "(undefined)";
                }
                printf("%s\n", out);
                break;
            }
            case NODE_WAIT:
                printf("> ");
                fflush(stdout);
                fgets(last_input, sizeof(last_input), stdin);
                last_input[strcspn(last_input, "\n")] = '\0';
                break;
            case NODE_IF_SAID: {
                const char *expected = node->value;
                if (node->is_identifier) {
                    char *v = get_var(node->value);
                    if (v) expected = v; else expected = "";
                }
                if (strcmp(last_input, expected) == 0 && node->body)
                    execute(node->body);
                break;
            }
            case NODE_REPLY: {
                const char *out = node->value;
                if (node->is_identifier) {
                    char *v = get_var(node->value);
                    if (v) out = v; else out = "(undefined)";
                }
                printf("%s\n", out);
                break;
            }
            case NODE_SET: {
                const char *val = node->value;
                if (node->is_identifier) {
                    char *v = get_var(node->value);
                    if (v) val = v; else val = "";
                }
                set_var(node->var_name, val);
                break;
            }
            case NODE_DEFINE:
                define_block(node->var_name, node->body);
                break;
            case NODE_CALL: {
                Node *body = get_block(node->value);
                if (body) execute(body);
                else fprintf(stderr, "Runtime error: undefined block '%s'\n", node->value);
                break;
            }
            case NODE_LOAD: {
                void *h = dlopen(node->value, RTLD_LAZY);
                if (!h) fprintf(stderr, "Runtime error: cannot load '%s': %s\n", node->value, dlerror());
                else if (lib_count < MAX_LIBS) lib_handles[lib_count++] = h;
                else { dlclose(h); fprintf(stderr, "Runtime error: too many libraries\n"); }
                break;
            }
            case NODE_FCALL:
                call_foreign_function(node->var_name, node->body);
                break;
            default: break;
        }
        node = node->next;
    }
}
