#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct lexer {
    char *data;
    char *tokens;
    char *cur_seminfo;
    bool *separators;
    size_t data_len;
    size_t cursor;
    int (*tokenizer)(char *seminfo, size_t len);
};

int lexer_init(struct lexer *l, char *file_name, char *tokens, bool *separators, int (*tokenizer)(char *seminfo, size_t len)) {
    if (access(file_name, R_OK) != 0) {
        return -1;
    }
    FILE *f = fopen(file_name, "r");
    if (!f)
        return -2;
    fseek(f, 0L, SEEK_END);
    long long length = ftell(f);
    rewind(f);
    l->data = (char*)malloc(sizeof(char)*(length+1));
    fread(l->data, sizeof(char), length, f);
    l->data[length] = '\0';

    l->data_len = length;
    l->cursor = 0;
    l->tokens = tokens;
    l->separators = separators;
    l->tokenizer = tokenizer;
    fclose(f);
    return 0;
}

uint16_t lexer_next(struct lexer *l) {
    size_t start = l->cursor;
    if (l->cursor >= l->data_len)
        return 0;
    // Skip leading spaces
    while (l->cursor < l->data_len && l->data[l->cursor] == ' ') {
        l->cursor++;
        start++;
    }
    // Continue until lexeme end
    while (l->cursor < l->data_len && !l->separators[(size_t)l->data[l->cursor]] && l->data[l->cursor] != ' ') {
        l->cursor++;
    }
    // Fixing separator tokens
    if (l->cursor-start == 0 && l->separators[(size_t)l->data[l->cursor]]) {
        l->cursor++;
    }

    // FIXME
    l->cur_seminfo = (char*)malloc(sizeof(char)*(l->cursor-start+1));
    memcpy(l->cur_seminfo, &l->data[start], l->cursor-start);
    l->cur_seminfo[l->cursor-start] = '\0';

    // move this logic to a callback
    return l->tokenizer(l->cur_seminfo, l->cursor-start);
}

void lexer_destroy(struct lexer *l) {
    free(l->data);
    free(l->cur_seminfo);
}
