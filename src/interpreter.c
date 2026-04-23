#define _GNU_SOURCE
#include "interpreter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <math.h>

extern Node* parse_codestring(const char *code);

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

#define MAX_LIST_ITEMS 256
typedef struct {
    char *items[MAX_LIST_ITEMS];
    int count;
} ItemList;

#define MAX_CODESTRINGS 16
typedef struct {
    char name[64];
    char content[256];
} Codestring;
static Codestring codestrings[MAX_CODESTRINGS];
static int codestring_count = 0;

// Recursion depth guard
#define MAX_RECURSION_DEPTH 100
static int recursion_depth = 0;

// Whitelist of allowed library basenames
static const char* allowed_libs[] = {
    "libm.so.6",
    "libc.so.6",
    NULL
};

// Whitelist of allowed function names for FFI
static const char* allowed_functions[] = {
    "sqrt",
    "puts",
    "sin",
    "cos",
    NULL
};

static int is_library_allowed(const char *name) {
    // Reject paths with directory separators, parent dir, or absolute paths
    if (strchr(name, '/') != NULL || strstr(name, "..") != NULL || name[0] == '.' || !name || !*name) {
        return 0;
    }
    for (int i = 0; allowed_libs[i] != NULL; i++) {
        if (strcmp(name, allowed_libs[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_function_allowed(const char *name) {
    for (int i = 0; allowed_functions[i] != NULL; i++) {
        if (strcmp(name, allowed_functions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

#define TMP_BUF_SIZE 4
static char tmp_buf[TMP_BUF_SIZE][256];
static unsigned int tmp_idx = 0;

static const char* eval_expression(Node *expr) {
    if (!expr) return "";
    char *buf = tmp_buf[tmp_idx];
    tmp_idx = (tmp_idx + 1) % TMP_BUF_SIZE;
    buf[0] = '\0';
    
    switch (expr->type) {
        case NODE_STRING:
            if (expr->is_identifier) {
                char *val = get_var(expr->value);
                if (val) {
                    safe_strcpy(buf, val, 256);
                } else {
                    fprintf(stderr, "Runtime error: undefined variable '%s'\n", expr->value);
                }
            } else {
                safe_strcpy(buf, expr->value, 256);
            }
            break;
        case NODE_NUMBER:
            safe_strcpy(buf, expr->value, 256);
            break;
        case NODE_NUMBERS:
        case NODE_LETTERS:
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
    } else {
        fprintf(stderr, "Runtime error: too many variables defined (max %d)\n", MAX_VARS);
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
            fprintf(stderr, "Warning: redefining block '%s'\n", name);
            if (blocks[i].body) free_ast(blocks[i].body);
            blocks[i].body = body;
            return;
        }
    }
    if (block_count < MAX_BLOCKS) {
        safe_strcpy(blocks[block_count].name, name, 64);
        blocks[block_count].body = body;
        block_count++;
    } else {
        fprintf(stderr, "Runtime error: too many blocks defined (max %d)\n", MAX_BLOCKS);
        free_ast(body);
    }
}

static void call_foreign_function(const char *name, Node *args) {
    if (!is_function_allowed(name)) {
        fprintf(stderr, "Runtime error: function '%s' not allowed (security policy)\n", name);
        return;
    }
    
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
    
    typedef double (*math_func_t)(double);
    typedef int (*puts_func_t)(const char*);

    if (strcmp(name, "sqrt") == 0) {
        if (!args) { fprintf(stderr, "sqrt expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        math_func_t f = (math_func_t)func_ptr;
        printf("%g\n", f(x));
    } else if (strcmp(name, "puts") == 0) {
        if (!args) { fprintf(stderr, "puts expects one argument\n"); return; }
        const char *arg = eval_expression(args);
        puts_func_t f = (puts_func_t)func_ptr;
        f(arg);
    } else if (strcmp(name, "sin") == 0) {
        if (!args) { fprintf(stderr, "sin expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        math_func_t f = (math_func_t)func_ptr;
        printf("%g\n", f(x));
    } else if (strcmp(name, "cos") == 0) {
        if (!args) { fprintf(stderr, "cos expects one argument\n"); return; }
        double x = atof(eval_expression(args));
        math_func_t f = (math_func_t)func_ptr;
        printf("%g\n", f(x));
    } else {
        fprintf(stderr, "Runtime error: unsupported function '%s'\n", name);
    }
}

static void save_codestring(const char *name, const char *content) {
    for (int i = 0; i < codestring_count; i++) {
        if (strcmp(codestrings[i].name, name) == 0) {
            safe_strcpy(codestrings[i].content, content, 256);
            return;
        }
    }
    if (codestring_count < MAX_CODESTRINGS) {
        safe_strcpy(codestrings[codestring_count].name, name, 64);
        safe_strcpy(codestrings[codestring_count].content, content, 256);
        codestring_count++;
    } else {
        fprintf(stderr, "Runtime error: too many codestrings defined (max %d)\n", MAX_CODESTRINGS);
    }
}

static char* get_codestring(const char *name) {
    for (int i = 0; i < codestring_count; i++) {
        if (strcmp(codestrings[i].name, name) == 0) {
            return codestrings[i].content;
        }
    }
    return NULL;
}

static void display_menu(Node *menu_node) {
    if (!menu_node || menu_node->type != NODE_MENU) return;

    printf("\n=== Menu ===\n");
    int idx = 1;
    Node *item = menu_node->body;
    while (item) {
        if (item->type == NODE_OPTION) {
            printf("%d. %s\n", idx++, item->value);
        }
        item = item->next;
    }
    printf("> ");
    fflush(stdout);
    // Read user selection
    char input[256];
    if (fgets(input, sizeof(input), stdin)) {
        int choice = atoi(input);
        if (choice >= 1 && choice <= idx - 1) {
            // Find the selected option
            Node *selected = menu_node->body;
            for (int i = 1; i < choice && selected; i++) {
                selected = selected->next;
            }
            if (selected && selected->type == NODE_OPTION) {
                // Set last_input to the selected value for if user said
                safe_strcpy(last_input, selected->value, 256);
            }
        }
    }
}

static void free_item_list(char **items, int count) {
    for (int i = 0; i < count; i++) {
        if (items[i]) free(items[i]);
    }
}

static ItemList parse_item_list(const char *data) {
    ItemList list = {.count = 0};
    if (!data || !data[0]) return list;

    char *buffer = strdup(data);
    if (!buffer) return list;

    int success = 1;
    if (strncmp(buffer, "NUMBERS:", 8) == 0) {
        char *nums = buffer + 8;
        char *token = strtok(nums, "|");
        while (token && list.count < MAX_LIST_ITEMS) {
            list.items[list.count] = strdup(token);
            if (!list.items[list.count]) {
                success = 0;
                break;
            }
            list.count++;
            token = strtok(NULL, "|");
        }
    } else if (strncmp(buffer, "LETTERS:", 8) == 0) {
        char *lets = buffer + 8;
        char *token = strtok(lets, "|");
        while (token && list.count < MAX_LIST_ITEMS) {
            list.items[list.count] = strdup(token);
            if (!list.items[list.count]) {
                success = 0;
                break;
            }
            list.count++;
            token = strtok(NULL, "|");
        }
    } else {
        char *token = strtok(buffer, "|");
        while (token && list.count < MAX_LIST_ITEMS) {
            list.items[list.count] = strdup(token);
            if (!list.items[list.count]) {
                success = 0;
                break;
            }
            list.count++;
            token = strtok(NULL, "|");
        }
    }
    free(buffer);
    if (!success) {
        // Free already allocated items on failure
        for (int i = 0; i < list.count; i++) {
            free(list.items[i]);
        }
        list.count = 0;
    }
    return list;
}

static void print_combinations(ItemList *list, const char *success_msg) {
    printf("Generating combinations...\n");
    for (int i = 0; i < list->count; i++) {
        printf("Trying: %s\n", list->items[i]);
        // In a real brute-force, this would attempt login or something
        // For this demo, we just print success
        printf("%s\n", success_msg);
    }
}

void execute(Node *node) {
    while (node) {
        switch (node->type) {
            case NODE_WRITE: {
                const char *out = eval_expression(node);
                printf("%s\n", out);
                break;
            }
            case NODE_WAIT:
                printf("> ");
                fflush(stdout);
                if (fgets(last_input, sizeof(last_input), stdin) == NULL) {
                    last_input[0] = '\0';
                } else {
                    last_input[strcspn(last_input, "\n")] = '\0';
                }
                break;
            case NODE_IF_SAID: {
                const char *expected = node->value;
                if (node->is_identifier) {
                    char *v = get_var(node->value);
                    if (!v) {
                        fprintf(stderr, "Runtime error: undefined variable '%s'\n", node->value);
                        break;
                    }
                    expected = v;
                }
                // Exact match for now
                if (strcmp(last_input, expected) == 0 && node->body)
                    execute(node->body);
                else if (node->else_body)
                    execute(node->else_body);
                break;
            }
            case NODE_REPLY: {
                const char *out = eval_expression(node);
                printf("%s\n", out);
                break;
            }
            case NODE_SET: {
                const char *val = eval_expression(node);
                set_var(node->var_name, val);
                break;
            }
            case NODE_DEFINE:
                define_block(node->var_name, node->body);
                break;
            case NODE_CALL: {
                if (recursion_depth >= MAX_RECURSION_DEPTH) {
                    fprintf(stderr, "Runtime error: maximum recursion depth exceeded\n");
                    break;
                }
                Node *body = get_block(node->value);
                if (body) {
                    recursion_depth++;
                    execute(body);
                    recursion_depth--;
                } else {
                    fprintf(stderr, "Runtime error: undefined block '%s'\n", node->value);
                }
                break;
            }
            case NODE_LOAD: {
                if (!is_library_allowed(node->value)) {
                    fprintf(stderr, "Runtime error: library '%s' not allowed (security policy)\n", node->value);
                    break;
                }
                void *h = dlopen(node->value, RTLD_LAZY);
                if (!h) {
                    fprintf(stderr, "Runtime error: cannot load '%s': %s\n", node->value, dlerror());
                    break;
                }
                if (lib_count < MAX_LIBS) {
                    lib_handles[lib_count++] = h;
                } else {
                    dlclose(h);
                    fprintf(stderr, "Runtime error: too many libraries\n");
                }
                break;
            }
            case NODE_FCALL:
                call_foreign_function(node->var_name, node->body);
                break;
            case NODE_MENU:
                display_menu(node);
                break;
            case NODE_SAVE:
                if (node->body) {
                    save_codestring(node->var_name, node->value);
                }
                break;
            case NODE_FUNCTION: {
                if (node->body) {
                    ItemList list = parse_item_list(node->body->value);
                    if (list.count > 0) {
                        print_combinations(&list, "Access granted");
                    }
                    free_item_list(list.items, list.count);
                }
                break;
            }
            case NODE_INPUT_CHECK: {
                // This node type is for checking input, but implementation is minimal
                // It could be used for more complex input validation
                break;
            }
            case NODE_CODESTRING: {
                if (node->value[0] != '\0') {
                    Node *body = parse_codestring(node->value);
                    if (body) {
                        define_block(node->var_name, body);
                    }
                }
                break;
            }
            default: break;
        }
        node = node->next;
    }
}

void execute_codestring(const char *name) {
    Node *body = get_block(name);
    if (body) {
        if (recursion_depth >= MAX_RECURSION_DEPTH) {
            fprintf(stderr, "Runtime error: maximum recursion depth exceeded\n");
            return;
        }
        recursion_depth++;
        execute(body);
        recursion_depth--;
    } else {
        fprintf(stderr, "Runtime error: codestring '%s' not found\n", name);
    }
}

void cleanup_libraries() {
    for (int i = 0; i < lib_count; i++) {
        if (lib_handles[i]) dlclose(lib_handles[i]);
    }
    lib_count = 0;
}
