#include <util.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "grammar.h"
#include "lexer.c"
#include <grammar.h>
#include <rdesc.h>
#include <cst_macros.h>

enum abnf_tk tokens[] = {
    TK_NUM, TK_PLUS, TK_NUM, TK_PLUS, TK_NUM, TK_SEMI
};

bool separators[] = {
    [(size_t)' '] = true,
    [(size_t)'\n'] = true,
    [(size_t)';'] = true,
    [(size_t)'+'] = true,
};

static void bc_tk_destroyer(uint16_t tk, void *seminfo_) {
    (void) tk;
    (void) seminfo_;
}

void node_printer(FILE *out, const struct rdesc_node *node) {
    if (rtype(node) == RDESC_TOKEN) {
        fprintf(out, "'%s'", (char*)rseminfo(node));
    } else {
        if (rid(node) == NT_STATEMENT)
            fprintf(out, "Statement");
        else if (rid(node) == NT_EXPRESSION)
            fprintf(out, "Expression");
    }
}

int interpretExpression(struct rdesc *p, struct rdesc_node *n) {
    if (rtype(n) != RDESC_NONTERMINAL) return -1;
    if (rid(n) != NT_EXPRESSION) return -1;
    int res = 0;
    struct rdesc_node *num = rchild(p, n, 0);
    switch (ralt_idx(n)) {
        case 0: // num + expr
            ;
            struct rdesc_node *expr2 = rchild(p, n, 2);
            res = atoi(rseminfo(num)) + interpretExpression(p, expr2);
            break;
        case 1: // only num
            res = atoi(rseminfo(num));
            break;
        default:
            fprintf(stderr, "interpreter error\n");
            break;
    }
    return res;
}

int interpretStatement(struct rdesc *p, struct rdesc_node *n) {
    if (rtype(n) != RDESC_NONTERMINAL) return -1;
    if (rid(n) != NT_STATEMENT) return -1;
    return interpretExpression(p, rchild(p, n, 0));
}

int interpreter(struct rdesc *p, struct rdesc_node *n) {
    for (int i = 0; i < rchild_count(n); i++) {
        if (rtype(n) != RDESC_NONTERMINAL) {
            fprintf(stderr, "root node is not a nonterminal\n");
        }
        switch (rid(n)) {
            case NT_STATEMENT:
                return interpretStatement(p, n);
                break;
            case NT_EXPRESSION:
                return interpretExpression(p, n);
                break;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: ./main <file-name>\n");
        return -1;
    }
    struct rdesc_grammar grammar;
    struct rdesc parser;
    struct lexer l;
    assert(rdesc_grammar_init_checked(&grammar, ABNF_PRODUCTION_COUNT,
                ABNF_MAX_ALTERNATIVE_COUNT, ABNF_MAX_ALTERNATIVE_SIZE,
                abnf_grammar) == 0);
    printf("Grammar initialized\n");
    lexer_init(&l, argv[1], NULL, separators);
    // printf("Source file loaded: %s\n", l.data);
    printf("Source file (%s) size: %ld\n", argv[1], l.data_len);

    if (argc > 1 && strcmp(argv[1], "dump_bnf") == 0) {
        rdesc_dump_bnf(stdout, &grammar, tk_names, nt_names);
        return 0;
    }

    if (rdesc_init(&parser, &grammar, sizeof(void*), bc_tk_destroyer) != 0) {
        fprintf(stderr, "rdesc_init error\n");
        return -1;
        // vendor/rdesc/src/common.h
    }

	int pump_res;
    while (l.data_len > l.cursor) {
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

        printf("All tokens are pumped\n");
        if (pump_res == RDESC_READY) {
            struct rdesc_node *cst;
            (void) cst;
            cst = rdesc_root(&parser);
            //rdesc_dump_cst(stdout, &parser, node_printer);
            printf("> %d\n", interpreter(&parser, cst));
        }
    }

	rdesc_destroy(&parser);
	rdesc_grammar_destroy(&grammar);
    return 0;
}

