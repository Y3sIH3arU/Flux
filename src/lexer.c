#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

Token next_token(const char *source, int *pos) {
    Token token;
    token.type = TOKEN_EOF;
    token.value[0] = '\0';
    
    // Skip whitespace and comments
    while (1) {
        while (source[*pos] == ' ' || source[*pos] == '\t' || source[*pos] == '\r') {
            (*pos)++;
        }
        if (source[*pos] == '#') {
            while (source[*pos] != '\n' && source[*pos] != '\0') {
                (*pos)++;
            }
            continue;
        }
        break;
    }
    
    if (source[*pos] == '\0') {
        token.type = TOKEN_EOF;
        return token;
    }
    
    if (source[*pos] == '\n') {
        token.type = TOKEN_NEWLINE;
        (*pos)++;
        return token;
    }
    
    if (source[*pos] == '"') {
        (*pos)++;
        int i = 0;
        while (source[*pos] != '"' && source[*pos] != '\0' && i < 255) {
            token.value[i++] = source[(*pos)++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_STRING;
        if (source[*pos] == '"') (*pos)++;
        return token;
    }
    
    if (isdigit(source[*pos]) || (source[*pos] == '.' && isdigit(source[*pos+1]))) {
        int i = 0;
        while (isdigit(source[*pos]) || source[*pos] == '.') {
            token.value[i++] = source[(*pos)++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_NUMBER;
        return token;
    }
    
    if (isalpha(source[*pos]) || source[*pos] == '_') {
        int i = 0;
        while ((isalnum(source[*pos]) || source[*pos] == '_') && i < 255) {
            token.value[i++] = source[(*pos)++];
        }
        token.value[i] = '\0';
        
        if (strcmp(token.value, "write") == 0) token.type = TOKEN_WRITE;
        // say removed
        else if (strcmp(token.value, "wait") == 0) token.type = TOKEN_WAIT;
        else if (strcmp(token.value, "for") == 0) token.type = TOKEN_FOR;
        else if (strcmp(token.value, "user") == 0) token.type = TOKEN_USER;
        else if (strcmp(token.value, "if") == 0) token.type = TOKEN_IF;
        else if (strcmp(token.value, "said") == 0) token.type = TOKEN_SAID;
        else if (strcmp(token.value, "then") == 0) token.type = TOKEN_THEN;
        else if (strcmp(token.value, "reply") == 0) token.type = TOKEN_REPLY;
        else if (strcmp(token.value, "set") == 0) token.type = TOKEN_SET;
        else if (strcmp(token.value, "to") == 0) token.type = TOKEN_TO;
        else if (strcmp(token.value, "define") == 0) token.type = TOKEN_DEFINE;
        else if (strcmp(token.value, "as") == 0) token.type = TOKEN_AS;
        else if (strcmp(token.value, "end") == 0) token.type = TOKEN_END;
        else if (strcmp(token.value, "load") == 0) token.type = TOKEN_LOAD;
        else if (strcmp(token.value, "call") == 0) token.type = TOKEN_CALL;
        else token.type = TOKEN_IDENTIFIER;
        
        return token;
    }
    
    token.type = TOKEN_UNKNOWN;
    (*pos)++;
    return token;
}

int lex_all(const char *source, Token *tokens) {
    int pos = 0;
    int count = 0;
    Token t;
    do {
        t = next_token(source, &pos);
        if (t.type != TOKEN_UNKNOWN && t.type != TOKEN_NEWLINE) {
            tokens[count++] = t;
        }
    } while (t.type != TOKEN_EOF && count < MAX_TOKENS - 1);
    return count;
}
