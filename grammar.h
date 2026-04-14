#include <grammar.h>
#include <rule_macros.h>

#define ABNF_PRODUCTION_COUNT 4
#define ABNF_MAX_ALTERNATIVE_COUNT 2
#define ABNF_MAX_ALTERNATIVE_SIZE 3

/*
 * <Statement> ::= <Expression> ;
 * <Expression> ::= <Number> + <Expression>
 * <Expression> ::= <Number>
 */

enum abnf_tk {
    TK_NUM = 1,
    TK_SEMI,
    TK_PLUS,
};

enum abnf_nt {
    NT_STATEMENT, NT_EXPRESSION
};

static const struct rdesc_grammar_symbol abnf_grammar[ABNF_PRODUCTION_COUNT]
[ABNF_MAX_ALTERNATIVE_COUNT + 1][ABNF_MAX_ALTERNATIVE_SIZE  + 1] = {
// STATEMENT
    r(NT(EXPRESSION), TK(SEMI)),
// EXPRESSION
    r(TK(NUM), TK(PLUS), NT(EXPRESSION)
alt   TK(NUM))
};

const char* tk_names[] = {
    "number", "semicolon", "plus"
};

const char* nt_names[] = {
    "Statement", "Expression"
};
