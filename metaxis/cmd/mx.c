/* mx.c -- mx [-o out] [-b backend] [-t] [-g] file.mx */
#include "mx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fputs("usage: mx [-o output] [-b backend] [-t] [-g] file.mx\n"
          "       -b   which 'as <name>' template each rule emits from\n"
          "       -t   trace the parse to stderr, and count what it tried\n"
          "       -g   print the grammar the header declared, and stop\n", stderr);
    exit(2);
}

/* A space goes *between* elements and never after the last one. This used to
   print one after every element, which left a trailing space on any rule
   without a level -- invisible on a terminal, and impossible for a document to
   hold, so the transcript in REFERENCE.md could never have matched exactly.
   tests/docs.sh is what noticed. */
static void show(Elem *el, int nel)
{
    for (int e = 0; e < nel; e++) {
        if (e) putchar(' ');
        switch (el[e].kind) {
        case EL_WORD: printf("\"%s\"", el[e].word); break;
        case EL_HOLE: printf("%s", el[e].hole);       break;
        default:
            printf("[ ");
            show(el[e].sub, el[e].nsub);
            printf(" ]%s", el[e].rep == REP_STAR ? "*" :
                           el[e].rep == REP_PLUS ? "+" : "");
            if (el[e].sep)  printf(" sep \"%s\"", el[e].sep);
            if (el[e].join) printf(" join \"%s\"", el[e].join);
        }
    }
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
        show(r->el, r->nel);
        if (r->level >= 0) printf(" [%d%s]", r->level, r->right ? " right" : "");
        if (r->terminated) printf(" terminated");
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    const char *in = NULL, *outpath = NULL, *backend = NULL;
    int grammar_only = 0, trace = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) { if (++i >= argc) usage(); outpath = argv[i]; }
        else if (!strcmp(argv[i], "-b")) { if (++i >= argc) usage(); backend = argv[i]; }
        else if (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-g")) grammar_only = 1;
        else if (argv[i][0] == '-' && argv[i][1]) usage();
        else if (!in) in = argv[i];
        else usage();
    }
    if (!in) usage();

    char *err = NULL;
    char *src = read_file(in, &err);
    if (!src) { fprintf(stderr, "mx: %s\n", err); return 1; }

    Grammar *g = grammar_new();
    size_t body = 0;
    if (header_read(g, src, in, &body, &err) < 0) {
        fprintf(stderr, "mx: %s\n", err);
        return 1;
    }
    if (grammar_seal(g, &err) < 0) {
        fprintf(stderr, "mx: %s\n", err);
        return 1;
    }
    /* `-g` is about what the header declared, and a backend is about what a
       rule emits, so the dump comes first and needs no `-b`. A file whose every
       template is tagged can still be inspected. */
    if (grammar_only) {
        for (int i = 0; i < g->nbackend; i++)
            printf("backend    %s\n", g->backend[i]);
        dump(g);
        return 0;
    }

    if (grammar_select(g, backend, &err) < 0) {
        fprintf(stderr, "mx: %s\n", err);
        return 1;
    }

    char *out = NULL;
    if (g->mode == MODE_TEXT) {
        out = expand_text(g, src, body, in, &err);
    } else {
        expand_trace(trace);
        Toks tk;
        if (lex(g, src, body, in, &tk, &err) < 0) {
            fprintf(stderr, "mx: %s\n", err);
            return 1;
        }
        out = expand_expr(g, &tk, &err);
        if (trace) expand_summary();
    }
    if (!out) { fprintf(stderr, "mx: %s\n", err); return 1; }
    out = collect_resolve(g, out);

    FILE *f = stdout;
    if (outpath && !(f = fopen(outpath, "wb"))) {
        fprintf(stderr, "mx: cannot write %s\n", outpath);
        return 1;
    }
    fputs(out, f);
    if (out[0] && out[strlen(out) - 1] != '\n') fputc('\n', f);
    if (f != stdout) fclose(f);
    return 0;
}
