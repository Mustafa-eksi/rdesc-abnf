#include <grammar.h>
#include <rule_macros.h>

#define ABNF_PRODUCTION_COUNT 4
#define ABNF_MAX_ALTERNATIVE_COUNT 2
#define ABNF_MAX_ALTERNATIVE_SIZE 4

/*
 * <Rule> ::= <ident> = <More-Ident> <crlf>
 * <More-Ident> ::= <ident> <More-Ident>
 * <More-Ident> ::= <ident>
 */

enum abnf_tk {
    TK_IDENT = 1,
    TK_EQUALS,
    TK_CRLF,
};

enum abnf_nt {
    NT_RULE, NT_MORE_IDENT
};

static const struct rdesc_grammar_symbol abnf_grammar[ABNF_PRODUCTION_COUNT]
[ABNF_MAX_ALTERNATIVE_COUNT + 1][ABNF_MAX_ALTERNATIVE_SIZE  + 1] = {
// RULE
    r(TK(IDENT), TK(EQUALS), NT(MORE_IDENT), TK(CRLF)),
// MORE_IDENT 
    r(TK(IDENT), NT(MORE_IDENT)
alt   TK(IDENT))
};

const char* tk_names[] = {
    "identifier", "="
};

const char* nt_names[] = {
    "Rule", "More-Identifiers"
};
