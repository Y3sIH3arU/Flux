#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

void execute(Node *node);
void execute_codestring(const char *name);
void cleanup_libraries();

#endif
