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
};

int lexer_init(struct lexer *l, char *file_name, char *tokens, bool *separators) {
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
    fclose(f);
    return 0;
}

/*
 *  ERROR = 0
 *  TK_NUM = 1
 *  TK_SEMI = 2
 *  TK_PLUS = 3
 */
uint16_t lexer_next(struct lexer *l) {
    size_t start = l->cursor;
    if (l->cursor >= l->data_len)
        return 0;
    bool is_num = true;
    while (l->cursor < l->data_len && isspace(l->data[l->cursor])) {
        l->cursor++;
        start++;
    }
    while (l->cursor < l->data_len && !l->separators[(size_t)l->data[l->cursor]]) {
        if (!isdigit(l->data[l->cursor]))
            is_num = false;
        l->cursor++;
    }
    if (l->cursor-start == 0 && l->separators[(size_t)l->data[l->cursor]]) {
        l->cursor++;
        is_num = false;
    }
    l->cur_seminfo = (char*)malloc(sizeof(char)*(l->cursor-start+1));
    memcpy(l->cur_seminfo, &l->data[start], l->cursor-start);
    l->cur_seminfo[l->cursor-start] = '\0';
    if (is_num)
        return 1;
    if (l->cursor-start == 1 && l->cur_seminfo[0] == ';')
        return 2;
    if (l->cursor-start == 1 && l->cur_seminfo[0] == '+')
        return 3;
    return 0;
}

void lexer_destroy(struct lexer *l) {
    free(l->data);
    free(l->cur_seminfo);
}
