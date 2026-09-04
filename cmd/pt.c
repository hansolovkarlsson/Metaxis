/* pt.c -- pt [-o out] file.pt */
#include "pt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fputs("usage: pt [-o output] file.pt\n"
          "       -g   print the grammar the header declared, and stop\n", stderr);
    exit(2);
}

static void dump(Grammar *g)
{
    printf("mode       %s\n", g->mode == MODE_TEXT ? "text" : "expression");
    printf("separator  %s\n", g->sep_in ? "declared" : "none");
    for (int i = 0; i < g->ncls; i++)
        printf("token      %-8s %s\n", g->cls[i].name, g->cls[i].src);
    for (int i = 0; i < g->ncom; i++)
        printf("comment    %s %s\n", g->com[i].open,
               g->com[i].eol ? "eol" : g->com[i].close);
    printf("words     ");
    for (int i = 0; i < g->npunct; i++) printf(" '%s'", g->punct[i]);
    printf("\n");
    for (int i = 0; i < g->nrule; i++) {
        Rule *r = &g->rule[i];
        printf("%-6s    ", r->led ? "infix" : "prefix");
        for (int e = 0; e < r->nel; e++)
            if (r->el[e].kind == EL_WORD) printf("\"%s\" ", r->el[e].word);
            else printf("%s ", r->el[e].hole);
        if (r->level >= 0) printf("[%d%s]", r->level, r->right ? " right" : "");
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    const char *in = NULL, *outpath = NULL;
    int grammar_only = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) { if (++i >= argc) usage(); outpath = argv[i]; }
        else if (!strcmp(argv[i], "-g")) grammar_only = 1;
        else if (argv[i][0] == '-' && argv[i][1]) usage();
        else if (!in) in = argv[i];
        else usage();
    }
    if (!in) usage();

    char *err = NULL;
    char *src = read_file(in, &err);
    if (!src) { fprintf(stderr, "pt: %s\n", err); return 1; }

    Grammar *g = grammar_new();
    size_t body = 0;
    if (header_read(g, src, in, &body, &err) < 0) {
        fprintf(stderr, "pt: %s\n", err);
        return 1;
    }
    grammar_seal(g);

    if (grammar_only) { dump(g); return 0; }

    char *out = NULL;
    if (g->mode == MODE_TEXT) {
        out = expand_text(g, src, body, in, &err);
    } else {
        Toks tk;
        if (lex(g, src, body, in, &tk, &err) < 0) {
            fprintf(stderr, "pt: %s\n", err);
            return 1;
        }
        out = expand_expr(g, &tk, &err);
    }
    if (!out) { fprintf(stderr, "pt: %s\n", err); return 1; }

    FILE *f = stdout;
    if (outpath && !(f = fopen(outpath, "wb"))) {
        fprintf(stderr, "pt: cannot write %s\n", outpath);
        return 1;
    }
    fputs(out, f);
    if (out[0] && out[strlen(out) - 1] != '\n') fputc('\n', f);
    if (f != stdout) fclose(f);
    return 0;
}
