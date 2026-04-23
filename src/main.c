#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

char* read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen"); return NULL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

void print_help() {
    printf("Flux - A minimalist interactive scripting language\n\n");
    printf("USAGE:\n  flux <file.fx>       Run a Flux script\n  flux --help          Show this help\n\n");
    printf("COMMANDS:\n");
    printf("  write \"text\" / var   Print text or variable\n");
    printf("  wait for user        Pause for input\n");
    printf("  if user said \"...\" then reply \"...\"\n");
    printf("  set name to \"value\" / other / 42\n");
    printf("  define name as ... end   Define block\n");
    printf("  name                 Call defined block\n");
    printf("  load \"lib.so\"        Load shared library\n");
    printf("  call func arg        Call C function (sqrt, puts, sin, cos)\n");
    printf("  # comment            Ignored\n\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: flux <file.fx>\nTry 'flux --help' for more information.\n");
        return 1;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }
    Node *ast = parse_file(argv[1]);
    if (!ast) return 1;
    execute(ast);
    free_ast(ast);
    return 0;
}
