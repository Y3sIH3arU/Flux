#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

Token next_token(const char *source, int *pos) {
    Token token;
    token.type = TOKEN_EOF;
    token.value[0] = '\0';
    
    // Skip whitespace and comments with bounds checking
    while (source[*pos] != '\0') {
        if (source[*pos] == ' ' || source[*pos] == '\t' || source[*pos] == '\r') {
            (*pos)++;
        } else if (source[*pos] == '#') {
            while (source[*pos] != '\n' && source[*pos] != '\0') {
                (*pos)++;
            }
        } else {
            break;
        }
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
    
    // String literal with overflow detection
    if (source[*pos] == '"') {
        (*pos)++;
        int i = 0;
        while (source[*pos] != '"' && source[*pos] != '\0') {
            if (i < 255) {
                token.value[i++] = source[*pos];
            }
            (*pos)++;
        }
        token.value[i] = '\0';
        
        if (i >= 255 && source[*pos] != '"') {
            fprintf(stderr, "Lexer error: string literal exceeds 255 characters (truncated)\n");
        }
        
        if (source[*pos] == '"') {
            token.type = TOKEN_STRING;
            (*pos)++;
        } else {
            token.type = TOKEN_UNKNOWN;
            fprintf(stderr, "Lexer error: unterminated string literal\n");
        }
        return token;
    }
    
    // Number literal
    if (isdigit(source[*pos]) || (source[*pos] == '.' && isdigit(source[*pos+1]))) {
        int i = 0;
        while (isdigit(source[*pos]) || source[*pos] == '.') {
            if (i < 255) {
                token.value[i++] = source[*pos];
            }
            (*pos)++;
        }
        token.value[i] = '\0';
        token.type = TOKEN_NUMBER;
        if (i >= 255) {
            fprintf(stderr, "Lexer warning: number truncated to 255 characters\n");
        }
        return token;
    }
    
    // Identifier or keyword with overflow detection
    if (isalpha(source[*pos]) || source[*pos] == '_') {
        int i = 0;
        while ((isalnum(source[*pos]) || source[*pos] == '_')) {
            if (i < 255) {
                token.value[i++] = source[*pos];
            }
            (*pos)++;
        }
        token.value[i] = '\0';
        
        if (i >= 255) {
            fprintf(stderr, "Lexer error: identifier exceeds 255 characters (truncated)\n");
        }
        
        // Match keywords
        if (strcmp(token.value, "write") == 0) token.type = TOKEN_WRITE;
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
    
    // Unknown character
    token.type = TOKEN_UNKNOWN;
    token.value[0] = source[*pos];
    token.value[1] = '\0';
    (*pos)++;
    return token;
}

int lex_all(const char *source, Token *tokens) {
    int pos = 0;
    int count = 0;
    Token t;
    do {
        if (count >= MAX_TOKENS - 1) {
            fprintf(stderr, "Lexer error: too many tokens (max %d)\n", MAX_TOKENS);
            break;
        }
        t = next_token(source, &pos);
        if (t.type != TOKEN_UNKNOWN && t.type != TOKEN_NEWLINE) {
            tokens[count++] = t;
        }
    } while (t.type != TOKEN_EOF);
    return count;
}
