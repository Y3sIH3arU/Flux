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
    
    // List syntax: [item;item;...]
    if (source[*pos] == '[') {
        (*pos)++;
        int i = 0;
        // Check for numbers or letters prefix
        int is_numbers = 0;
        int is_letters = 0;
        if (strncmp(&source[*pos], "numbers:", 8) == 0) {
            is_numbers = 1;
            (*pos) += 8;
        } else if (strncmp(&source[*pos], "letters:", 8) == 0) {
            is_letters = 1;
            (*pos) += 8;
        }
        
        // Collect list items until ]
        while (source[*pos] != ']' && source[*pos] != '\0') {
            // Read item
            while (source[*pos] != ';' && source[*pos] != ']' && source[*pos] != '\0') {
                if (source[*pos] == ' ' || source[*pos] == '\t') {
                    (*pos)++;
                    continue;
                }
                if (i < 255) {
                    token.value[i++] = source[*pos];
                }
                (*pos)++;
            }
            if (source[*pos] == ';') {
                if (i < 255) {
                    token.value[i++] = '|';
                }
                (*pos)++;
            } else {
                break;
            }
        }
        token.value[i] = '\0';
        
        if (source[*pos] == ']') {
            if (is_numbers) {
                token.type = TOKEN_NUMBERS;
                // Prefix with "NUMBERS:" marker
                char prefixed[260];
                snprintf(prefixed, sizeof(prefixed), "NUMBERS:%s", token.value);
                strncpy(token.value, prefixed, 255);
            } else if (is_letters) {
                token.type = TOKEN_LETTERS;
                char prefixed[260];
                snprintf(prefixed, sizeof(prefixed), "LETTERS:%s", token.value);
                strncpy(token.value, prefixed, 255);
            } else {
                token.type = TOKEN_LIST_OPEN;
            }
            (*pos)++;
        } else {
            token.type = TOKEN_UNKNOWN;
        }
        return token;
    }
    
    // List separator
    if (source[*pos] == ';') {
        token.type = TOKEN_LIST_SEP;
        token.value[0] = '\0';
        (*pos)++;
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
        else if (strcmp(token.value, "menu") == 0) token.type = TOKEN_MENU;
        else if (strcmp(token.value, "option") == 0) token.type = TOKEN_OPTION;
        else if (strcmp(token.value, "function") == 0) token.type = TOKEN_FUNCTION;
        else if (strcmp(token.value, "from") == 0) token.type = TOKEN_FROM;
        else if (strcmp(token.value, "use") == 0) token.type = TOKEN_USE;
        else if (strcmp(token.value, "save") == 0) token.type = TOKEN_SAVE;
        else if (strcmp(token.value, "codestring") == 0) token.type = TOKEN_CODE;
        else if (strcmp(token.value, "numbers") == 0) token.type = TOKEN_NUMBERS;
        else if (strcmp(token.value, "letters") == 0) token.type = TOKEN_LETTERS;
        else if (strcmp(token.value, "input") == 0) token.type = TOKEN_INPUT;
        else token.type = TOKEN_IDENTIFIER;
        
        return token;
    }
    
    // Unknown character
    token.type = TOKEN_UNKNOWN;
    token.value[0] = source[*pos];
    token.value[1] = '\0';
    if (source[*pos] == '+') token.type = TOKEN_ADD;
    (*pos)++;
    return token;
}

int is_inside_string(const char *line, const char *pos) {
    int in_string = 0;
    const char *p = line;
    while (p < pos) {
        if (*p == '"' && (p == line || *(p-1) != '\\')) {
            in_string = !in_string;
        }
        p++;
    }
    return in_string;
}

int is_template_delimiter(const char *line) {
    const char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (strncmp(p, "\\-", 2) == 0) {
        const char *after = p + 2;
        if (*after == '\0' || *after == '\n' || *after == '\r' || isspace((unsigned char)*after)) {
            return 1;
        }
    }
    return 0;
}

char* trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = '\0';
    return str;
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
