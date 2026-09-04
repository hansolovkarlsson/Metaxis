/* header.c -- the fixed half.
 *
 * Every string below is a Prototype string, spelled Prototype's way, in every
 * file, whatever the file declares. That is what stops a directive from ever
 * being read as the thing it is declaring.
 */
#include "pt.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Grammar *grammar_new(void)
{
    Grammar *g = xmalloc(sizeof *g);
    memset(g, 0, sizeof *g);
    g->mode = MODE_EXPR;
    g->sep_in = NULL;
    g->sep_out = NULL;
    return g;
}

int class_index(Grammar *g, const char *name)
{
    for (int i = 0; i < g->ncls; i++)
        if (!strcmp(g->cls[i].name, name)) return i;
    return -1;
}

static void *grow(void *base, int n, size_t size)
{
    void *p = xmalloc(size * (size_t)(n + 1));
    if (base) memcpy(p, base, size * (size_t)n);
    return p;
}

/* --------------------------------------------------------------- directives */

typedef struct {
    const char *s;       /* whole source            */
    size_t      i, end;  /* cursor, end of directive */
    const char *file;
    char       *err;
} D;

static void derr(D *d, const char *msg)
{
    if (!d->err)
        d->err = xfmt("%s:%d: %s", d->file, line_at(d->s, d->i), msg);
}

/* `;` ends a directive's text, here and in directive_end. It can afford to,
   because everything a directive says about foreign text is in a string, and
   this runs outside the strings. */
static void dskip(D *d)
{
    for (;;) {
        while (d->i < d->end && (d->s[d->i] == ' ' || d->s[d->i] == '\t' ||
                                 d->s[d->i] == '\n' || d->s[d->i] == '\r'))
            d->i++;
        if (d->i < d->end && d->s[d->i] == ';') {
            while (d->i < d->end && d->s[d->i] != '\n') d->i++;
            continue;
        }
        return;
    }
}

static int dat(D *d, const char *lit)
{
    dskip(d);
    size_t n = strlen(lit);
    return d->i + n <= d->end && !memcmp(d->s + d->i, lit, n);
}

static int dtake(D *d, const char *lit)
{
    if (!dat(d, lit)) return 0;
    d->i += strlen(lit);
    return 1;
}

static int ident_ch(int c) { return isalnum((unsigned char)c) || c == '_'; }

/* A Prototype string: "..." with \" \\ \n \t \r and nothing else. */
static char *dstring(D *d)
{
    dskip(d);
    if (d->i >= d->end || d->s[d->i] != '"') { derr(d, "expected a string"); return NULL; }
    d->i++;
    Buf b = {0};
    while (d->i < d->end && d->s[d->i] != '"') {
        char c = d->s[d->i];
        if (c == '\n') { derr(d, "unterminated string"); return NULL; }
        if (c == '\\') {
            d->i++;
            if (d->i >= d->end) { derr(d, "unterminated string"); return NULL; }
            switch (d->s[d->i]) {
            case 'n':  buf_ch(&b, '\n'); break;
            case 't':  buf_ch(&b, '\t'); break;
            case 'r':  buf_ch(&b, '\r'); break;
            case '\\': buf_ch(&b, '\\'); break;
            case '"':  buf_ch(&b, '"');  break;
            default:   derr(d, "unknown escape"); return NULL;
            }
            d->i++;
            continue;
        }
        buf_ch(&b, c);
        d->i++;
    }
    if (d->i >= d->end) { derr(d, "unterminated string"); return NULL; }
    d->i++;
    if (!b.p) buf_str(&b, "");
    return b.p;
}

static char *dident(D *d)
{
    dskip(d);
    size_t a = d->i;
    if (a >= d->end || !(isalpha((unsigned char)d->s[a]) || d->s[a] == '_')) return NULL;
    while (d->i < d->end && ident_ch(d->s[d->i])) d->i++;
    return xstrndup(d->s + a, d->i - a);
}

static int dnumber(D *d, int *out)
{
    dskip(d);
    size_t a = d->i;
    if (a < d->end && d->s[a] == '-') d->i++;
    if (d->i >= d->end || !isdigit((unsigned char)d->s[d->i])) { d->i = a; return 0; }
    while (d->i < d->end && isdigit((unsigned char)d->s[d->i])) d->i++;
    char *t = xstrndup(d->s + a, d->i - a);
    *out = atoi(t);
    return 1;
}

/* ------------------------------------------------------------------- @syntax */

static const char *KINDS[] = { "expr", "stmts", "text", NULL };

/* A pattern element, and a group of them. `[ … ]` is Prototype's vocabulary and
   lives outside the strings, so it can never be mistaken for the body's own
   brackets -- those would be quoted. */
static Elem *parse_elems(Grammar *g, D *d, Rule *r, int *nout, int depth);

static int parse_group(Grammar *g, D *d, Rule *r, Elem *e, int depth)
{
    e->kind = EL_GROUP;
    e->rep  = REP_ONE;
    e->sub  = parse_elems(g, d, r, &e->nsub, depth + 1);
    if (!e->sub) return -1;
    if (!dtake(d, "]")) { derr(d, "expected ']'"); return -1; }
    if (!e->nsub) { derr(d, "a group needs something in it"); return -1; }

    if (dtake(d, "*"))      e->rep = REP_STAR;
    else if (dtake(d, "+")) e->rep = REP_PLUS;

    /* `sep` and `join` are keywords only where a string follows, so a hole may
       still be called either of them. */
    for (int again = 1; again;) {
        again = 0;
        for (int k = 0; k < 2; k++) {
            const char *kw = k ? "join" : "sep";
            size_t save = d->i;
            if (!dtake(d, kw)) continue;
            if (!dat(d, "\"")) { d->i = save; continue; }
            char *v = dstring(d);
            if (!v) return -1;
            if (e->rep == REP_ONE) {
                derr(d, xfmt("'%s' belongs to a repeated group -- '[ … ]*' or '[ … ]+'", kw));
                return -1;
            }
            if (k) e->join = v; else e->sep = v;
            again = 1;
        }
    }
    if (e->rep != REP_ONE && !e->join) e->join = e->sep ? e->sep : xstrdup("");
    return 0;
}

static Elem *parse_elems(Grammar *g, D *d, Rule *r, int *nout, int depth)
{
    Elem *el = NULL;
    int   nel = 0;

    if (depth > 16) { derr(d, "groups nested more than 16 deep"); return NULL; }

    for (;;) {
        dskip(d);
        if (d->i >= d->end) break;
        if (dat(d, "=>")) break;
        if (depth && dat(d, "]")) break;

        Elem e;
        memset(&e, 0, sizeof e);
        e.cls = -1;

        if (d->s[d->i] == '"') {
            char *w = dstring(d);
            if (!w) return NULL;
            if (!*w) { derr(d, "an empty word matches nothing"); return NULL; }
            e.kind = EL_WORD;
            e.word = w;
        } else if (dtake(d, "[")) {
            if (parse_group(g, d, r, &e, depth) < 0) return NULL;
        } else {
            int lv;
            if (dnumber(d, &lv)) {
                r->level = lv;
                if (dtake(d, "left"))       r->right = 0;
                else if (dtake(d, "right")) r->right = 1;
                continue;
            }
            char *id = dident(d);
            if (!id) { derr(d, "expected a quoted word, a hole, a group or a level"); return NULL; }
            e.kind = EL_HOLE;
            e.hole = id;
            e.hk   = K_EXPR;
            if (dtake(d, ":")) {
                char *k = dident(d);
                if (!k) { derr(d, "expected a kind after ':'"); return NULL; }
                int found = 0;
                for (int i = 0; KINDS[i]; i++)
                    if (!strcmp(k, KINDS[i])) { e.hk = i == 0 ? K_EXPR : (i == 1 ? K_STMTS : K_TEXT); found = 1; }
                if (!found) {
                    int ci = class_index(g, k);
                    if (ci < 0) { derr(d, xfmt("no kind or token class called '%s'", k)); return NULL; }
                    e.hk  = K_CLASS;
                    e.cls = ci;
                }
            }
        }
        el = grow(el, nel, sizeof *el);
        el[nel++] = e;
    }
    *nout = nel;
    return el ? el : (Elem *)grow(NULL, 0, sizeof *el);
}

/* ------------------------------------------------------- checking a pattern */

/* A hole is greedy when it reads an expression or a run of statements: it takes
   everything up to the word that stops it. A class-kind hole takes one token
   and cannot be greedy, which is why `f a:name b:name` is allowed and `f a b`
   is not. */
static int greedy(const Elem *e)
{
    return e->kind == EL_HOLE && (e->hk == K_EXPR || e->hk == K_STMTS);
}

/* Looking through a group's brackets: what could actually come first, or last. */
static const Elem *edge(const Elem *e, int last)
{
    while (e && e->kind == EL_GROUP)
        e = e->nsub ? &e->sub[last ? e->nsub - 1 : 0] : NULL;
    return e;
}

static int hole_named(const Elem *el, int nel, const char *n)
{
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_HOLE && !strcmp(el[i].hole, n)) return 1;
        if (el[i].kind == EL_GROUP && hole_named(el[i].sub, el[i].nsub, n)) return 1;
    }
    return 0;
}

int rule_has_hole(Rule *r, const char *name)
{
    return hole_named(r->el, r->nel, name);
}

static int check_elems(D *d, Elem *el, int nel)
{
    for (int i = 0; i < nel; i++) {
        Elem *e = &el[i];

        if (e->kind == EL_HOLE && e->hk == K_STMTS &&
            (i + 1 >= nel || el[i + 1].kind != EL_WORD)) {
            derr(d, "a 'stmts' hole needs a word after it to stop at");
            return -1;
        }
        if (i + 1 < nel) {
            const Elem *a = edge(e, 1), *b = edge(&el[i + 1], 0);
            if (a && b && greedy(a) && b->kind == EL_HOLE) {
                derr(d, "two holes in a row: the first would take everything the second wants");
                return -1;
            }
        }
        if (e->kind != EL_GROUP) continue;

        if (e->rep != REP_ONE && !e->sep) {
            const Elem *a = edge(e, 1), *b = edge(e, 0);
            if (a && b && greedy(a) && b->kind == EL_HOLE) {
                derr(d, "a repeated group that ends in a greedy hole and begins with"
                        " a hole needs a 'sep' to know where one turn stops");
                return -1;
            }
        }
        if (check_elems(d, e->sub, e->nsub) < 0) return -1;
    }
    return 0;
}

static int rule_syntax(Grammar *g, D *d, int line)
{
    Rule r;
    memset(&r, 0, sizeof r);
    r.level = -1;
    r.file  = xstrdup(d->file);
    r.line  = line;

    int   nel = 0;
    Elem *el  = parse_elems(g, d, &r, &nel, 0);
    if (!el) return -1;

    if (!nel) { derr(d, "a rule needs a pattern"); return -1; }
    if (!dtake(d, "=>")) { derr(d, "expected '=>'"); return -1; }

    /* A template has always been a string, and a string never starts with a
       brace, so one character says which of the two forms this is. Nothing had
       to be reserved and no file written before today changes meaning. */
    char *tmpl = NULL;
    dskip(d);
    r.el = el; r.nel = nel;
    r.led = el[0].kind == EL_HOLE;

    if (d->i < d->end && d->s[d->i] == '{') {
        char *cerr = NULL;
        r.body = code_parse(d->s, &d->i, d->end, d->file, &r.nbody, &cerr);
        if (!r.body) { if (!d->err) d->err = cerr; return -1; }
    } else {
        tmpl = dstring(d);
        if (!tmpl) return -1;
        r.tmpl = tmpl;
    }
    /* `terminated` sits after the template, which is the one place in a rule
       where a bare word cannot be anything else -- a hole can only appear in the
       pattern. So it needs no quoting and reserves nothing. */
    if (dtake(d, "terminated")) r.terminated = 1;
    dskip(d);
    if (d->i < d->end) { derr(d, "trailing text after the template"); return -1; }

    if (el[0].kind == EL_GROUP) {
        derr(d, "a rule is found by its first word, so it cannot begin with a group");
        return -1;
    }
    if (r.led && r.level < 0) {
        derr(d, "a rule that begins with a hole is infix or postfix and needs a level");
        return -1;
    }
    if (r.led && (nel < 2 || el[1].kind != EL_WORD)) {
        derr(d, "a rule that begins with a hole must have a word after it");
        return -1;
    }
    if (check_elems(d, el, nel) < 0) return -1;

    /* The template is checked here rather than at the first use of the rule,
       so that a splice nobody wrote a hole for is an error at the line that
       wrote it. Both forms are checked; only the spelling differs. */
    if (r.body) {
        char *cerr = NULL;
        if (code_check(&r, &cerr) < 0) { if (!d->err) d->err = cerr; return -1; }
        g->rule = grow(g->rule, g->nrule, sizeof *g->rule);
        g->rule[g->nrule++] = r;
        return 0;
    }
    for (size_t i = 0; tmpl[i];) {
        if (tmpl[i] == '{' && tmpl[i + 1] == '{') { i += 2; continue; }
        if (tmpl[i] == '}' && tmpl[i + 1] == '}') { i += 2; continue; }
        if (tmpl[i] != '{') { i++; continue; }

        int fresh = tmpl[i + 1] == '~';
        size_t a = i + 1 + (size_t)fresh, j = a;
        while (tmpl[j] && tmpl[j] != '}') j++;
        if (!tmpl[j]) { derr(d, "unclosed '{' in a template"); return -1; }
        char *n = xstrndup(tmpl + a, j - a);
        i = j + 1;

        int hole = hole_named(el, nel, n);

        if (fresh) {
            if (!*n) { derr(d, "a fresh name needs a label: '{~name}'"); return -1; }
            if (hole) {
                derr(d, xfmt("'{~%s}' and '{%s}' would read as one thing: a fresh"
                             " name and a hole cannot share a label", n, n));
                return -1;
            }
        } else if (!hole) {
            derr(d, xfmt("the template splices '{%s}' and the pattern has no such hole", n));
            return -1;
        }
    }

    g->rule = grow(g->rule, g->nrule, sizeof *g->rule);
    g->rule[g->nrule++] = r;
    return 0;
}

/* ------------------------------------------------------------ one directive */

static int use_file(Grammar *g, const char *path, const char *from, char **err);

static int directive(Grammar *g, D *d, const char *file, int line,
                     int *ended, char **err)
{
    char *name = NULL;
    if (d->s[d->i] == '@') { d->i++; name = dident(d); }
    if (!name) { derr(d, "expected a directive"); goto fail; }

    if (!strcmp(name, "end")) {
        *ended = 1;
        return 0;
    }
    if (!strcmp(name, "mode")) {
        char *m = dident(d);
        if (!m) { derr(d, "expected 'expression' or 'text'"); goto fail; }
        if (!strcmp(m, "expression")) g->mode = MODE_EXPR;
        else if (!strcmp(m, "text"))  g->mode = MODE_TEXT;
        else { derr(d, "expected 'expression' or 'text'"); goto fail; }
        return 0;
    }
    if (!strcmp(name, "token")) {
        char *n = dident(d);
        if (!n) { derr(d, "expected a class name"); goto fail; }
        char *re = dstring(d);
        if (!re) goto fail;
        Class c;
        c.name = n;
        c.src  = re;
        char *anchored = xfmt("^(%s)", re);
        int rc = regcomp(&c.re, anchored, REG_EXTENDED);
        if (rc) {
            char msg[256];
            regerror(rc, &c.re, msg, sizeof msg);
            derr(d, xfmt("bad pattern for '%s': %s", n, msg));
            goto fail;
        }
        int old = class_index(g, n);
        if (old >= 0) { g->cls[old] = c; return 0; }
        g->cls = grow(g->cls, g->ncls, sizeof *g->cls);
        g->cls[g->ncls++] = c;
        return 0;
    }
    if (!strcmp(name, "comment")) {
        char *open = dstring(d);
        if (!open) goto fail;
        Comment c = { open, NULL, 0 };
        if (dtake(d, "eol")) c.eol = 1;
        else {
            c.close = dstring(d);
            if (!c.close) goto fail;
        }
        g->com = grow(g->com, g->ncom, sizeof *g->com);
        g->com[g->ncom++] = c;
        return 0;
    }
    if (!strcmp(name, "separator")) {
        char *in = dstring(d);
        if (!in) goto fail;
        char *out = NULL;
        if (dtake(d, "=>")) { out = dstring(d); if (!out) goto fail; }
        g->sep_in  = in;
        g->sep_out = out ? out : xstrdup(in);
        g->sep_nl  = strchr(in, '\n') != NULL;
        return 0;
    }
    if (!strcmp(name, "syntax")) {
        if (rule_syntax(g, d, line) < 0) goto fail;
        return 0;
    }
    if (!strcmp(name, "use")) {
        char *p = dstring(d);
        if (!p) goto fail;
        return use_file(g, p, file, err);
    }
    derr(d, xfmt("no directive called '@%s'", name));
fail:
    *err = d->err;
    return -1;
}

/* ------------------------------------------------------------- header lines */

/* Skips whitespace, `;` comments -- which are Prototype's own and always
   work -- and any comment the header has declared for itself so far. */
static size_t skip_trivia(Grammar *g, const char *s, size_t i)
{
    for (;;) {
        while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') i++;
        if (s[i] == ';') {
            while (s[i] && s[i] != '\n') i++;
            continue;
        }
        int hit = 0;
        for (int c = 0; c < g->ncom && !hit; c++) {
            size_t n = strlen(g->com[c].open);
            if (memcmp(s + i, g->com[c].open, n)) continue;
            hit = 1;
            i += n;
            if (g->com[c].eol) { while (s[i] && s[i] != '\n') i++; }
            else {
                size_t m = strlen(g->com[c].close);
                while (s[i] && memcmp(s + i, g->com[c].close, m)) i++;
                if (s[i]) i += m;
            }
        }
        if (!hit) return i;
    }
}

/* A directive ends at a newline that is not followed by an indented line, and
   never inside a string. Proto ends one with `.`, which is also the statement
   separator inside its template; there is nothing here to guess. */
static size_t directive_end(const char *s, size_t i)
{
    int instr = 0, depth = 0;
    for (;;) {
        while (s[i] && (instr || depth || s[i] != '\n')) {
            if (instr) {
                if (s[i] == '\\' && s[i + 1]) i++;
                else if (s[i] == '"') instr = 0;
            } else if (s[i] == '"') instr = 1;
            else if (s[i] == '{') depth++;
            else if (s[i] == '}') { if (depth) depth--; }
            else if (s[i] == ';' && !depth) { while (s[i] && s[i] != '\n') i++; break; }
            i++;
        }
        if (!s[i]) return i;
        size_t j = i + 1;
        if (s[j] == ' ' || s[j] == '\t') {
            size_t k = j;
            while (s[k] == ' ' || s[k] == '\t') k++;
            if (s[k] && s[k] != '\n') { i = k; continue; }
        }
        return i;
    }
}

int header_read(Grammar *g, const char *src, const char *file,
                size_t *body, char **err)
{
    size_t i = 0;
    for (;;) {
        size_t at = skip_trivia(g, src, i);
        if (!src[at]) { *body = at; return 0; }
        if (src[at] != '@') { *body = at; return 0; }

        size_t end = directive_end(src, at);
        D d = { src, at, end, file, NULL };
        int ended = 0;
        if (directive(g, &d, file, line_at(src, at), &ended, err) < 0) {
            if (!*err) *err = d.err;
            return -1;
        }
        i = end;
        if (ended) {
            while (src[i] == '\n' || src[i] == '\r') i++;
            *body = i;
            return 0;
        }
    }
}

static int use_file(Grammar *g, const char *path, const char *from, char **err)
{
    if (++g->nfiles > 64) { *err = xstrdup("@use nested more than 64 deep"); return -1; }
    /* Depth, not a total: two files that both use a third meet it twice and
       neither is nested in the other. 64 is the depth Solveig allows an
       @include, which is the number Proto took for the same reason. */

    char *full;
    const char *slash = strrchr(from, '/');
    if (path[0] == '/' || !slash) full = xstrdup(path);
    else full = xfmt("%.*s%s", (int)(slash - from + 1), from, path);

    char *src = read_file(full, err);
    if (!src) return -1;
    size_t body = 0;
    if (header_read(g, src, full, &body, err) < 0) return -1;
    size_t at = skip_trivia(g, src, body);
    if (src[at]) {
        *err = xfmt("%s:%d: a used file holds directives and nothing else",
                    full, line_at(src, at));
        return -1;
    }
    g->nfiles--;
    return 0;
}

/* --------------------------------------------------------------------- seal */

static int cmp_len(const void *a, const void *b)
{
    const char *const *x = a, *const *y = b;
    size_t m = strlen(*x), n = strlen(*y);
    if (m != n) return m < n ? 1 : -1;
    return strcmp(*x, *y);
}

/* The punctuation set is every word any rule quoted, plus the separator.
   Longest first, because the lexer takes the longest it can. */
static void seal_word(Grammar *g, char *w)
{
    if (!w || !*w) return;
    for (int i = 0; i < g->npunct; i++) if (!strcmp(g->punct[i], w)) return;
    g->punct = grow(g->punct, g->npunct, sizeof *g->punct);
    g->punct[g->npunct++] = w;
}

static void seal_elems(Grammar *g, Elem *el, int nel)
{
    for (int e = 0; e < nel; e++) {
        if (el[e].kind == EL_WORD) seal_word(g, el[e].word);
        if (el[e].kind == EL_GROUP) {
            seal_word(g, el[e].sep);
            seal_elems(g, el[e].sub, el[e].nsub);
        }
    }
}

void grammar_seal(Grammar *g)
{
    for (int r = 0; r < g->nrule; r++)
        seal_elems(g, g->rule[r].el, g->rule[r].nel);
    if (g->sep_in && !g->sep_nl) seal_word(g, g->sep_in);
    qsort(g->punct, (size_t)g->npunct, sizeof *g->punct, cmp_len);
}
