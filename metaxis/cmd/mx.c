/* mx.c -- mx [-o out] [-b backend] [-t] [-g] file.mx
                  or  mx -u rules.mx -i input [same flags] */
#include "mx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fputs("usage: mx [-o output] [-b backend] [-t] [-g] file.mx\n"
          "       mx -u rules.mx [...] -i input [-o output] [-b backend] [-t] [-g]\n"
          "       -u   a file of directives, read as '@use' reads one; repeatable\n"
          "       -i   the file to read with them, named as itself in every message\n"
          "       -b   which 'as <name>' template each rule emits from\n"
          "       -t   trace the parse to stderr, and count what it tried\n"
          "       -g   print the grammar the header declared, and stop\n", stderr);
    exit(2);
}

/* A command line the tool could not read exits 2, as a file it could not read
   exits 1 (REFERENCE.md 10). It says what is wrong before the shape, because
   the shape alone does not distinguish the two forms from a mixture of them. */
static void badline(const char *why)
{
    fprintf(stderr, "mx: %s\n", why);
    usage();
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
    for (int i = 0; i < g->nbr; i++)
        printf("bracket    %s %s\n", g->br[i].open, g->br[i].close);
    printf("words     ");
    for (int i = 0; i < g->npunct; i++) printf(" '%s'", g->punct[i]);
    printf("\n");
    for (int i = 0; i < g->nrule; i++) {
        Rule *r = &g->rule[i];
        /* Under @mode text a rule led by a class hole is a nud rule (§7). */
        printf("%-6s    ", r->led && g->mode != MODE_TEXT ? "infix" : "prefix");
        show(r->el, r->nel);
        if (r->level >= 0) printf(" [%d%s]", r->level, r->right ? " right" : "");
        if (r->terminated) printf(" terminated");
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    const char *in = NULL, *outpath = NULL, *backend = NULL, *inputpath = NULL;
    const char **rules = xmalloc((size_t)argc * sizeof *rules);
    int nrules = 0, grammar_only = 0, trace = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) { if (++i >= argc) usage(); outpath = argv[i]; }
        else if (!strcmp(argv[i], "-b")) { if (++i >= argc) usage(); backend = argv[i]; }
        else if (!strcmp(argv[i], "-u")) { if (++i >= argc) usage(); rules[nrules++] = argv[i]; }
        else if (!strcmp(argv[i], "-i")) { if (++i >= argc) usage(); inputpath = argv[i]; }
        else if (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-g")) grammar_only = 1;
        else if (argv[i][0] == '-' && argv[i][1]) usage();
        else if (!in) in = argv[i];
        else usage();
    }

    /* The two forms, and they do not mix. A `.mx` file is a header and a body
       in one place, which is the premise; `-u` and `-i` are the same two
       things named separately, for a file that is not written for any grammar
       and cannot be asked to carry one. Adding `-u` to a file that has a
       header would make position decide which rules come first, and adding
       `-i` to one that has a body would leave a body unread -- both of them
       the quiet kind of wrong this tool refuses everywhere else (§3.10). */
    if (in && (nrules || inputpath))
        badline("a file on its own carries its rules and its body, so it takes no -u and no -i");
    if (!in && !nrules && !inputpath) usage();
    if (nrules && !inputpath && !grammar_only)
        badline("-u gives the rules and -i the file to read with them, and there is no -i here"
                " -- only -g reads a grammar on its own");
    if (inputpath && !nrules)
        badline("-i gives the file to read and -u the rules to read it with, and there is no -u here");

    char *err = NULL;
    Grammar *g = grammar_new();

    /* Where the header comes from, and where the body does. They are one file
       in the first form and two in the second, and nothing below this point
       knows which: the expander has always taken a buffer, an offset into it
       and a name to put in its messages, and a body of its own is that buffer
       at offset 0 under its own name. That is why the messages a split run
       gives name the input file at the input file's own line. */
    char *src = NULL, *bsrc = NULL;
    const char *bfile = NULL;
    size_t body = 0;

    if (in) {
        src = read_file(in, &err);
        if (!src) { fprintf(stderr, "mx: %s\n", err); return 1; }
        if (header_read(g, src, in, &body, &err) < 0) {
            fprintf(stderr, "mx: %s\n", err);
            return 1;
        }
        bsrc = src; bfile = in;
    } else {
        for (int i = 0; i < nrules; i++)
            if (header_use(g, rules[i], &err) < 0) {
                fprintf(stderr, "mx: %s\n", err);
                return 1;
            }
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

    /* Read after `-g` has had its chance to stop, so that inspecting a grammar
       never depends on the file it would be pointed at. */
    if (inputpath) {
        bsrc = read_file(inputpath, &err);
        if (!bsrc) { fprintf(stderr, "mx: %s\n", err); return 1; }
        bfile = inputpath;
    }

    g->body_file = bfile;
    char *out = NULL;
    if (g->mode == MODE_TEXT) {
        out = expand_text(g, bsrc, body, bfile, &err);
    } else {
        expand_trace(trace);
        Toks tk;
        if (lex(g, bsrc, body, bfile, &tk, &err) < 0) {
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
