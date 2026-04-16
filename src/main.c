#include <util.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "grammar.h"
#include "lexer.c"
#include <grammar.h>
#include <rdesc.h>
#include <cst_macros.h>

bool separators[] = {
    [(size_t)'\n'] = true,
    [(size_t)'='] = true,
};
const size_t SEMINFO_SIZE = 1024;

static void bc_tk_destroyer(uint16_t tk, void *seminfo_) {
    (void) tk;
    (void) seminfo_;
}

void node_printer(FILE *out, const struct rdesc_node *node) {
    if (rtype(node) == RDESC_TOKEN) {
        fprintf(out, "token_type: %s - '%s'", tk_names[rid(node)-1], (char*)rseminfo(node));
    } else {
        fprintf(out, "nt_type: %s", nt_names[rid(node)]);
    }
}

int tokenizer(char *seminfo) {
    size_t size = strlen(seminfo);
    if (size == 1 && seminfo[0] == '=')
        return 2;
    if (size == 1 && seminfo[0] == '\n')
        return 3;
    if (size == 0) // empty token
        return 0;
    return 1;
}

struct abnf_rule {
    size_t name;
    size_t *expr;
    size_t size;
    size_t cap;
    size_t id;
};

struct abnf_document {
    size_t *rule_freq;
    char **name_space;
    size_t name_space_cap;
    size_t name_space_count;
    struct abnf_rule *rules;
    size_t rules_cap;
    size_t rules_count;

    int max_alternative;
    int max_size;
};

void abnf_document_init(struct abnf_document *doc) {
    doc->rules_cap = 10;
    doc->rules = (struct abnf_rule*) malloc(sizeof(struct abnf_rule)*doc->rules_cap);
    doc->rules_count = 0;
    doc->name_space_cap = 10;
    doc->name_space = (char**) malloc(sizeof(char**)*doc->name_space_cap);
    doc->name_space_count = 0;
    doc->rule_freq = (size_t*) malloc(sizeof(size_t*)*doc->name_space_cap);
    doc->max_alternative = -1;
    doc->max_size = -1;
}

char *strdup(char *str) {
    size_t len = strlen(str)+1;
    char* new_str = (char*)malloc(len*sizeof(char));
    strcpy(new_str, str);
    new_str[len-1] = '\0';
    return new_str;
}

char *dupupper(char *lower) {
    char *res = strdup(lower);
    for (size_t i = 0; i < strlen(res); i++) {
        if (res[i] >= 'a' && res[i] <= 'z')
            res[i] -= 32;
    }
    return res;
}

int abnf_add_name(struct abnf_document *doc, char *name, bool is_rule) {
    int name_index = -1;
    for (size_t i = 0; i < doc->name_space_count; i++) {
        if (strcmp(doc->name_space[i], name) == 0) {
            name_index = i;
            break;
        }
    }
    if (name_index == -1) {
        if (doc->name_space_count == doc->name_space_cap) {
            doc->name_space_cap *= 2;
            doc->name_space = (char**)realloc(doc->name_space, doc->name_space_cap*sizeof(char**));
            doc->rule_freq = (size_t*)realloc(doc->rule_freq, doc->name_space_cap*sizeof(size_t*));
        }
        name_index = doc->name_space_count++;
        doc->name_space[name_index] = name;
        doc->rule_freq[name_index] = 0;
    }
    if (is_rule) {
        doc->rule_freq[name_index]++;
    }
    return name_index;
}

void abnf_add_ident(struct abnf_rule *rule, size_t index) {
    if (rule->size == rule->cap) {
        if (rule->cap == 0)
            rule->cap = 5;
        rule->cap *= 2;
        rule->expr = (size_t*)realloc(rule->expr, sizeof(size_t)*rule->cap);
    }
    rule->expr[rule->size++] = index;
}

struct abnf_rule* abnf_add_rule(struct abnf_document *doc, char *name, size_t *expr) {
    int name_index = abnf_add_name(doc, strdup(name), true);
    if (doc->rules_count >= doc->rules_cap) {
        doc->rules_cap *= 2;
        doc->rules = (struct abnf_rule*) realloc(doc->rules, sizeof(struct abnf_rule)*doc->rules_cap);
    }
    doc->rules[doc->rules_count++] = (struct abnf_rule) {
        .name = name_index,
        .expr = expr,
        .id = doc->rule_freq[name_index]-1,
    };
    return &doc->rules[doc->rules_count-1];
}

void abnf_print_rules(struct abnf_document *doc) {
    for (size_t i = 0; i < doc->rules_count; i++) {
        printf("= Rule (alt size: %lu, alt id: %lu): %s\n", doc->rule_freq[doc->rules[i].name], doc->rules[i].id, doc->name_space[doc->rules[i].name]);
        printf("\t");
        for (size_t j = 0; j < doc->rules[i].size; j++) {
            if (doc->rule_freq[doc->rules[i].expr[j]] != 0)
                printf("<%s> ", doc->name_space[doc->rules[i].expr[j]]);
            else
                printf("%s ", doc->name_space[doc->rules[i].expr[j]]);
        }
        printf("\n");
    }
}

void abnf_print_names(struct abnf_document *doc) {
    for (size_t i = 0; i < doc->name_space_count; i++) {
        printf("%s = %lu\n", doc->name_space[i], doc->rule_freq[i]);
    }
}

int interpretModeIdent(struct rdesc *p, struct rdesc_node *n, struct abnf_document *doc, size_t rule_index) {
    if (rtype(n) != RDESC_NONTERMINAL) return -1;
    if (rid(n) != NT_MORE_IDENT) return -1;
    size_t name_id = abnf_add_name(doc, dupupper(rseminfo(rchild(p, n, 0))), false);
    abnf_add_ident(&doc->rules[rule_index], name_id);
    if (ralt_idx(n) == 0)
        return interpretModeIdent(p, rchild(p, n, 1), doc, rule_index);
    return 0;
}

int interpretRule(struct rdesc *p, struct abnf_document *doc, struct rdesc_node *n) {
    if (rtype(n) != RDESC_NONTERMINAL) return -1;
    if (rid(n) != NT_RULE) return -1;
    abnf_add_rule(doc, dupupper((char*)rseminfo(rchild(p, n, 0))), NULL);
    if (rtype(rchild(p, n, 2)) == RDESC_NONTERMINAL && rid(rchild(p, n, 2)) == NT_MORE_IDENT) {
        interpretModeIdent(p, rchild(p, n, 2), doc, doc->rules_count-1);
    }
    return 0;
}

int interpreter(struct rdesc *p, struct abnf_document *doc, struct rdesc_node *n) {
    for (int i = 0; i < rchild_count(n); i++) {
        if (rtype(n) != RDESC_NONTERMINAL) {
            fprintf(stderr, "root node is not a nonterminal\n");
        }
        switch (rid(n)) {
            case NT_RULE:
                return interpretRule(p, doc, n);
                break;
            case NT_MORE_IDENT:
                fprintf(stderr, "Identifiers without a rule!\n");
                break;
        }
    }
    return 0;
}

int max(int a, int b) {
    return (a>=b)*a+(b>a)*b;
}

void calculate_maximums(struct abnf_document *doc) {
    for (size_t i = 0; i < doc->rules_count; i++) {
        doc->max_alternative = max(doc->max_alternative, doc->rules[i].id);
        doc->max_size = max(doc->max_size, doc->rules[i].size);
    }
}

void write_abnf_grammar(struct abnf_document *doc, FILE *out, char *grammar_name_lower) {
    char *grammar_name_upper = dupupper(grammar_name_lower);
    // FIXME: put upper and lower grammer names to the document
    calculate_maximums(doc);
    fprintf(out, "#include <grammar.h>\n");
    fprintf(out, "#include <rule_macros.h>\n");
    fprintf(out, "#define %s_PRODUCTION_COUNT %lu\n", grammar_name_upper, doc->rules_count);
    fprintf(out, "#define %s_MAX_ALTERNATIVE_COUNT %d\n", grammar_name_upper, doc->max_alternative+1);
    fprintf(out, "#define %s_MAX_ALTERNATIVE_SIZE %d\n", grammar_name_upper, doc->max_size);
    // TODO: add source abnf as a comment here
    fprintf(out, "enum %s_tk {\n", grammar_name_lower);
    fprintf(out, "__TK_RESERVED,\n");
    for (size_t i = 0; i < doc->name_space_count; i++) {
        if (doc->rule_freq[i] == 0) {
            // FIXME: make sure this is uppercase
            fprintf(out, "TK_%s,\n", doc->name_space[i]);
        }
    }
    fprintf(out, "};\n");
    fprintf(out, "enum %s_nt {\n", grammar_name_lower);
    for (size_t i = 0; i < doc->name_space_count; i++) {
        if (doc->rule_freq[i] != 0) {
            // FIXME: make sure this is uppercase
            fprintf(out, "NT_%s,\n", doc->name_space[i]);
        }
    }
    fprintf(out, "};\n");
    fprintf(out, "static const struct rdesc_grammar_symbol %s_grammar[%s_PRODUCTION_COUNT]\n", grammar_name_lower, grammar_name_upper);
    fprintf(out, "[%s_MAX_ALTERNATIVE_COUNT + 1][%s_MAX_ALTERNATIVE_SIZE  + 1] = {\n", grammar_name_upper, grammar_name_upper);
    for (size_t i = 0; i < doc->name_space_count; i++) {
        if (doc->rule_freq[i] == 0) continue;
        fprintf(out, "r(\n");
        for (size_t ri = 0; ri < doc->rule_freq[i]; ri++) {
            size_t rule_id = 0;
            size_t target = ri;
            for (size_t k = 0; k < doc->rules_count; k++) {
                if (doc->rules[k].name == i && !(target--)) {
                    rule_id = k;
                }
            }
            for (size_t j = 0; j < doc->rules[rule_id].size; j++) {
                if (doc->rule_freq[doc->rules[rule_id].expr[j]] == 0) {
                    // TK
                    fprintf(out, "TK");
                } else {
                    // NT
                    fprintf(out, "NT");
                }
                fprintf(out, "(%s)", doc->name_space[doc->rules[rule_id].expr[j]]);
                if (j != doc->rules[rule_id].size-1)
                    fprintf(out, ", ");
            }
            fprintf(out, "\n");
            if (ri != doc->rule_freq[i]-1)
                fprintf(out, "alt ");
        }
        fprintf(out, "),\n");
    }
    fprintf(out, "};\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "USAGE: ./main <file-name> <grammar-name>\n");
        return -1;
    }
    struct rdesc_grammar grammar;
    struct rdesc parser;
    struct lexer l;
    assert(rdesc_grammar_init_checked(&grammar, ABNF_PRODUCTION_COUNT,
                ABNF_MAX_ALTERNATIVE_COUNT, ABNF_MAX_ALTERNATIVE_SIZE,
                abnf_grammar) == 0);
    printf("Grammar initialized\n");
    lexer_init(&l, argv[1], separators, tokenizer);
    // printf("Source file loaded: %s\n", l.data);
    printf("Source file (%s) size: %ld\n", argv[1], l.data_len);

    if (argc > 1 && strcmp(argv[1], "dump_bnf") == 0) {
        rdesc_dump_bnf(stdout, &grammar, tk_names, nt_names);
        return 0;
    }

    if (rdesc_init(&parser, &grammar, SEMINFO_SIZE, bc_tk_destroyer) != 0) {
        fprintf(stderr, "rdesc_init error\n");
        return -1;
        // vendor/rdesc/src/common.h
    }

    int pump_res;
    struct abnf_document doc;
    abnf_document_init(&doc);
    while (l.data_len > l.cursor) {
        if (rdesc_start(&parser, NT_RULE) != 0) {
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
            int res = interpreter(&parser, &doc, cst);
            (void) res;
        }
    }

    abnf_print_rules(&doc);
    abnf_print_names(&doc);

    char *output_ending = "_grammar.h";
    size_t output_name_len = (strlen(argv[2])+strlen(argv[2])+1)*sizeof(char);
    char *output_name = (char*)malloc(output_name_len);
    sprintf(output_name, "%s%s", argv[2], output_ending);

    FILE *output_file = fopen(output_name, "w");
    write_abnf_grammar(&doc, output_file, argv[2]);
    fclose(output_file);

    // FIXME: we leak everything

	rdesc_destroy(&parser);
	rdesc_grammar_destroy(&grammar);
    return 0;
}

