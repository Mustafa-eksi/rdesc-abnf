#include <util.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "adder_grammar.h"
#include "../src/lexer.c"
#include <grammar.h>
#include <rdesc.h>
#include <cst_macros.h>

bool separators[] = {
    [(size_t)'\n'] = true,
    [(size_t)'+'] = true,
    [(size_t)';'] = true,
};
const size_t SEMINFO_SIZE = 1024;
static void bc_tk_destroyer(uint16_t tk, void *seminfo_) {
    (void) tk;
    (void) seminfo_;
}

void node_printer(FILE *out, const struct rdesc_node *node) {
    if (rtype(node) == RDESC_TOKEN) {
        fprintf(out, "token_type: '%s'", (char*)rseminfo(node));
    } else {
        fprintf(out, "nt_type: %d", rid(node));
    }
}

int tokenizer(char *seminfo) {
    size_t len = strlen(seminfo);
    if (len == 1 && seminfo[0] == ';') {
        return TK_SEMI;
    } else if (len == 1 && seminfo[0] == '+') {
        return TK_PLUS;
    } else if (len == 1 && seminfo[0] == '\n') {
        return TK_CRLF;
    } else {
        bool isnum = true;
        for (size_t i = 0; i < len; i++)
            if (!isdigit(seminfo[i]))
                isnum = false;
        if (isnum)
            return TK_NUMBER;
    }
    return 0;
}

int interpret(struct rdesc *r, struct rdesc_node *n) {
    if (rtype(n) == RDESC_NONTERMINAL) {
        switch (rid(n)) {
            case NT_STATEMENT:
                return interpret(r, rchild(r, n, 0));
            case NT_EXPR:
                ;
                int num = interpret(r, rchild(r, n, 0));
                if (ralt_idx(n) == 0)
                    num += interpret(r, rchild(r, n, 2));
                return num;
            default:
                fprintf(stderr, "Unreachable!\n");
                return -1;
        }
    } else { // RDESC_TOKEN
        if (rid(n) != TK_NUMBER)
            return 0;
        return atoi(rseminfo(n));
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: ./main <file-name>\n");
        return -1;
    }
    struct rdesc_grammar grammar;
    struct rdesc parser;
    struct lexer l;
    assert(rdesc_grammar_init_checked(&grammar, ADDER_PRODUCTION_COUNT,
                ADDER_MAX_ALTERNATIVE_COUNT, ADDER_MAX_ALTERNATIVE_SIZE,
                adder_grammar) == 0);
    printf("Grammar initialized\n");
    lexer_init(&l, argv[1], separators, tokenizer);
    // printf("Source file loaded: %s\n", l.data);
    printf("Source file (%s) size: %ld\n", argv[1], l.data_len);

    if (rdesc_init(&parser, &grammar, SEMINFO_SIZE, bc_tk_destroyer) != 0) {
        fprintf(stderr, "rdesc_init error\n");
        return -1;
        // vendor/rdesc/src/common.h
    }

    while (l.data_len > l.cursor) {
        int pump_res = -1;
        if (rdesc_start(&parser, NT_STATEMENT) != 0) {
            printf("Couldn't start rdesc\n");
            return -1;
        }

        for (uint16_t lt = lexer_next(&l); lt != 0; lt = lexer_next(&l)) {
            // printf("lexer token type = '%d'\n", lt);
            // printf("lexer token = '%s'\n", l.cur_seminfo);
            pump_res = rdesc_pump(&parser, lt, l.cur_seminfo);
            if (pump_res != RDESC_CONTINUE)
                break;
        }

        if (pump_res == RDESC_READY) {
            struct rdesc_node *cst;
            cst = rdesc_root(&parser);
            // rdesc_dump_cst(stdout, &parser, node_printer);
            printf("output: %d\n", interpret(&parser, cst));
        }
    }

	rdesc_destroy(&parser);
	rdesc_grammar_destroy(&grammar);
    return 0;
}
