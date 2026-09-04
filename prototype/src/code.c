/* code.c -- the second kind of template.
 *
 * `=> { … }` instead of `=> "…"`. A string template can splice and nothing
 * else, which is where five separate warts in this repository come from: a
 * repeated group that wants a template per element rather than one joiner, an
 * optional part that cannot make the output differ, parentheses written
 * unconditionally because nobody could ask an operand its level, and a literal
 * moved rather than translated.
 *
 * The language is Prototype's own, so it lives outside the strings; the foreign
 * text it emits lives inside them. That is the one rule read once more, and it
 * is why `{` after the `=>` is enough to tell the two forms apart -- a template
 * has always been a string until now, and a string never starts with a brace.
 *
 * It is deliberately small. Every form below names a customer in
 * docs/ROADMAP.md, and anything that cannot name one is not here.
 */
#include "pt.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ tokens */

enum { C_EOF, C_NAME, C_INT, C_STR, C_PUNCT };

typedef struct {
    const char *s;
    size_t      i, end;
    const char *file;
    int         line;
    char       *err;

    int    kind;
    char  *text;
    long   num;
} C;

static void cerr(C *c, const char *msg)
{
    if (!c->err) c->err = xfmt("%s:%d: %s", c->file, c->line, msg);
}

static const char *PUNCT[] = {
    "==", "!=", "<=", ">=", "(", ")", "{", "}", ",", "+", "<", ">", ";", NULL
};

/* The words the language keeps for itself. A hole may not be called one of
   these, and the check at declaration says so rather than shadowing quietly. */
static const char *KEYWORDS[] = {
    "emit", "if", "else", "for", "in", "sep", "not", "and", "or", NULL
};

int code_is_keyword(const char *n)
{
    for (int i = 0; KEYWORDS[i]; i++) if (!strcmp(n, KEYWORDS[i])) return 1;
    return 0;
}

static void cnext(C *c)
{
    for (;;) {
        while (c->i < c->end && isspace((unsigned char)c->s[c->i])) {
            if (c->s[c->i] == '\n') c->line++;
            c->i++;
        }
        if (c->i < c->end && c->s[c->i] == ';') {         /* noise between statements */
            while (c->i < c->end && c->s[c->i] == ';') c->i++;
            continue;
        }
        break;
    }
    if (c->i >= c->end) { c->kind = C_EOF; c->text = xstrdup(""); return; }

    char ch = c->s[c->i];

    if (ch == '"') {
        Buf b = {0};
        c->i++;
        while (c->i < c->end && c->s[c->i] != '"') {
            if (c->s[c->i] == '\n') { cerr(c, "unterminated string"); return; }
            if (c->s[c->i] == '\\') {
                c->i++;
                if (c->i >= c->end) { cerr(c, "unterminated string"); return; }
                switch (c->s[c->i]) {
                case 'n':  buf_ch(&b, '\n'); break;
                case 't':  buf_ch(&b, '\t'); break;
                case 'r':  buf_ch(&b, '\r'); break;
                case '\\': buf_ch(&b, '\\'); break;
                case '"':  buf_ch(&b, '"');  break;
                default: cerr(c, "unknown escape"); return;
                }
                c->i++;
                continue;
            }
            buf_ch(&b, c->s[c->i]);
            c->i++;
        }
        if (c->i >= c->end) { cerr(c, "unterminated string"); return; }
        c->i++;
        if (!b.p) buf_str(&b, "");
        c->kind = C_STR;
        c->text = b.p;
        return;
    }
    if (isdigit((unsigned char)ch)) {
        size_t a = c->i;
        while (c->i < c->end && isdigit((unsigned char)c->s[c->i])) c->i++;
        c->kind = C_INT;
        c->text = xstrndup(c->s + a, c->i - a);
        c->num  = atol(c->text);
        return;
    }
    if (isalpha((unsigned char)ch) || ch == '_') {
        size_t a = c->i;
        while (c->i < c->end &&
               (isalnum((unsigned char)c->s[c->i]) || c->s[c->i] == '_')) c->i++;
        c->kind = C_NAME;
        c->text = xstrndup(c->s + a, c->i - a);
        return;
    }
    for (int k = 0; PUNCT[k]; k++) {
        size_t n = strlen(PUNCT[k]);
        if (c->i + n <= c->end && !memcmp(c->s + c->i, PUNCT[k], n)) {
            c->i += n;
            c->kind = C_PUNCT;
            c->text = xstrdup(PUNCT[k]);
            return;
        }
    }
    cerr(c, xfmt("nothing in this language is written '%c'", ch));
}

static int at(C *c, int kind, const char *text)
{
    return c->kind == kind && !strcmp(c->text, text);
}

static int take(C *c, int kind, const char *text)
{
    if (!at(c, kind, text)) return 0;
    cnext(c);
    return 1;
}

/* ------------------------------------------------------------------ parsing */

static Expr *e_or(C *c);
static Stmt *block(C *c, int *nout);

static Expr *mk(int kind)
{
    Expr *e = xmalloc(sizeof *e);
    memset(e, 0, sizeof *e);
    e->kind = kind;
    return e;
}

static Expr *primary(C *c)
{
    if (c->err) return NULL;
    if (c->kind == C_STR) {
        Expr *e = mk(E_TEXT);
        e->s = c->text;
        cnext(c);
        return e;
    }
    if (c->kind == C_INT) {
        Expr *e = mk(E_INT);
        e->n = c->num;
        cnext(c);
        return e;
    }
    if (take(c, C_PUNCT, "(")) {
        Expr *e = e_or(c);
        if (!e) return NULL;
        if (!take(c, C_PUNCT, ")")) { cerr(c, "expected ')'"); return NULL; }
        return e;
    }
    if (c->kind == C_NAME) {
        char *n = c->text;
        if (code_is_keyword(n)) {
            cerr(c, xfmt("'%s' is one of this language's own words and cannot be a value", n));
            return NULL;
        }
        cnext(c);
        if (take(c, C_PUNCT, "(")) {
            Expr *e = mk(E_CALL);
            e->s = n;
            if (!at(c, C_PUNCT, ")")) {
                for (;;) {
                    Expr *a = e_or(c);
                    if (!a) return NULL;
                    Expr **v = xmalloc(sizeof *v * (size_t)(e->nargs + 1));
                    if (e->args) memcpy(v, e->args, sizeof *v * (size_t)e->nargs);
                    v[e->nargs] = a;
                    e->args = v;
                    e->nargs++;
                    if (!take(c, C_PUNCT, ",")) break;
                }
            }
            if (!take(c, C_PUNCT, ")")) { cerr(c, "expected ')'"); return NULL; }
            return e;
        }
        Expr *e = mk(E_NAME);
        e->s = n;
        return e;
    }
    cerr(c, "expected a value");
    return NULL;
}

static Expr *e_not(C *c)
{
    if (take(c, C_NAME, "not")) {
        Expr *e = mk(E_NOT);
        e->a = e_not(c);
        return e->a ? e : NULL;
    }
    return primary(c);
}

static Expr *e_cat(C *c)
{
    Expr *a = e_not(c);
    while (a && !c->err && take(c, C_PUNCT, "+")) {
        Expr *e = mk(E_BIN);
        e->s = xstrdup("+");
        e->a = a;
        e->b = e_not(c);
        if (!e->b) return NULL;
        a = e;
    }
    return a;
}

static Expr *e_cmp(C *c)
{
    Expr *a = e_cat(c);
    if (!a || c->err) return a;
    static const char *OPS[] = { "==", "!=", "<=", ">=", "<", ">", NULL };
    for (int k = 0; OPS[k]; k++) {
        if (!at(c, C_PUNCT, OPS[k])) continue;
        Expr *e = mk(E_BIN);
        e->s = xstrdup(OPS[k]);
        cnext(c);
        e->a = a;
        e->b = e_cat(c);
        return e->b ? e : NULL;
    }
    return a;
}

static Expr *e_and(C *c)
{
    Expr *a = e_cmp(c);
    while (a && !c->err && take(c, C_NAME, "and")) {
        Expr *e = mk(E_BIN);
        e->s = xstrdup("and");
        e->a = a;
        e->b = e_cmp(c);
        if (!e->b) return NULL;
        a = e;
    }
    return a;
}

static Expr *e_or(C *c)
{
    Expr *a = e_and(c);
    while (a && !c->err && take(c, C_NAME, "or")) {
        Expr *e = mk(E_BIN);
        e->s = xstrdup("or");
        e->a = a;
        e->b = e_and(c);
        if (!e->b) return NULL;
        a = e;
    }
    return a;
}

static Stmt *push(Stmt *v, int *n, Stmt s)
{
    Stmt *p = xmalloc(sizeof *p * (size_t)(*n + 1));
    if (v) memcpy(p, v, sizeof *p * (size_t)*n);
    p[*n] = s;
    (*n)++;
    return p;
}

static Stmt *block(C *c, int *nout)
{
    if (!take(c, C_PUNCT, "{")) { cerr(c, "expected '{'"); return NULL; }
    Stmt *v = NULL;
    int   n = 0;

    while (!c->err && !at(c, C_PUNCT, "}")) {
        if (c->kind == C_EOF) { cerr(c, "a block ends in the middle of something"); return NULL; }
        Stmt s;
        memset(&s, 0, sizeof s);

        if (take(c, C_NAME, "emit")) {
            s.kind = S_EMIT;
            s.e = e_or(c);
            if (!s.e) return NULL;
        } else if (take(c, C_NAME, "if")) {
            s.kind = S_IF;
            s.e = e_or(c);
            if (!s.e) return NULL;
            s.body = block(c, &s.nbody);
            if (c->err) return NULL;
            if (take(c, C_NAME, "else")) {
                s.alt = block(c, &s.nalt);
                if (c->err) return NULL;
            }
        } else if (take(c, C_NAME, "for")) {
            s.kind = S_FOR;
            if (c->kind != C_NAME || code_is_keyword(c->text)) {
                cerr(c, "expected a name after 'for'");
                return NULL;
            }
            s.var = c->text;
            cnext(c);
            if (!take(c, C_NAME, "in")) { cerr(c, "expected 'in'"); return NULL; }
            s.e = e_or(c);
            if (!s.e) return NULL;
            if (take(c, C_NAME, "sep")) {
                s.sep = e_or(c);
                if (!s.sep) return NULL;
            }
            s.body = block(c, &s.nbody);
            if (c->err) return NULL;
        } else {
            cerr(c, "expected 'emit', 'if' or 'for'");
            return NULL;
        }
        v = push(v, &n, s);
    }
    if (c->err) return NULL;
    cnext(c);                                    /* the '}' */
    *nout = n;
    return v ? v : xmalloc(1);
}

Stmt *code_parse(const char *s, size_t *i, size_t end, const char *file,
                 int *nout, char **err)
{
    C c;
    memset(&c, 0, sizeof c);
    c.s = s; c.i = *i; c.end = end; c.file = file;
    c.line = line_at(s, *i);
    cnext(&c);
    Stmt *v = block(&c, nout);
    if (c.err || !v) { *err = c.err ? c.err : xfmt("%s: bad template", file); return NULL; }
    *i = c.i;
    return v;
}

/* ------------------------------------------------- checking, at declaration */

typedef struct { const char *n[32]; int n_; } Scope;

static const struct { const char *name; int args; const char *what; } BUILTIN[] = {
    { "matched", 1, "whether a hole's group matched"          },
    { "count",   1, "how many turns a repeated hole took"     },
    { "level",   1, "the level of what filled a hole"         },
    { "group",   2, "a hole, bracketed when its level is lower" },
    { "replace", 3, "one text with another inside a third"    },
    { "drop",    3, "a text with characters off each end"      },
    { "fresh",   1, "a name nobody else has"                  },
    { NULL, 0, NULL }
};

static int check_expr(Rule *r, Expr *e, Scope *sc, char **err)
{
    if (!e) return 0;
    switch (e->kind) {
    case E_TEXT: case E_INT: return 0;
    case E_NOT:  return check_expr(r, e->a, sc, err);
    case E_BIN:  return check_expr(r, e->a, sc, err) || check_expr(r, e->b, sc, err);
    case E_NAME:
        for (int i = 0; i < sc->n_; i++) if (!strcmp(sc->n[i], e->s)) return 0;
        if (rule_has_hole(r, e->s)) return 0;
        *err = xfmt("%s:%d: the template uses '%s' and the pattern has no such hole",
                    r->file, r->line, e->s);
        return -1;
    case E_CALL:
        for (int i = 0; BUILTIN[i].name; i++) {
            if (strcmp(BUILTIN[i].name, e->s)) continue;
            if (e->nargs != BUILTIN[i].args) {
                *err = xfmt("%s:%d: '%s' takes %d and was given %d -- it gives %s",
                            r->file, r->line, e->s, BUILTIN[i].args, e->nargs,
                            BUILTIN[i].what);
                return -1;
            }
            for (int a = 0; a < e->nargs; a++)
                if (check_expr(r, e->args[a], sc, err) < 0) return -1;
            return 0;
        }
        *err = xfmt("%s:%d: no such thing as '%s'", r->file, r->line, e->s);
        return -1;
    }
    return 0;
}

static int check_block(Rule *r, Stmt *v, int n, Scope *sc, char **err)
{
    for (int i = 0; i < n; i++) {
        if (check_expr(r, v[i].e, sc, err) < 0) return -1;
        if (v[i].sep && check_expr(r, v[i].sep, sc, err) < 0) return -1;
        if (v[i].kind == S_FOR) {
            if (sc->n_ >= 32) {
                *err = xfmt("%s:%d: loops nested more than 32 deep", r->file, r->line);
                return -1;
            }
            if (rule_has_hole(r, v[i].var)) {
                *err = xfmt("%s:%d: the loop variable '%s' is also a hole -- one of"
                            " them has to be called something else",
                            r->file, r->line, v[i].var);
                return -1;
            }
            sc->n[sc->n_++] = v[i].var;
            int rc = check_block(r, v[i].body, v[i].nbody, sc, err);
            sc->n_--;
            if (rc < 0) return -1;
        } else {
            if (check_block(r, v[i].body, v[i].nbody, sc, err) < 0) return -1;
        }
        if (check_block(r, v[i].alt, v[i].nalt, sc, err) < 0) return -1;
    }
    return 0;
}

int code_check(Rule *r, char **err)
{
    Scope sc;
    sc.n_ = 0;
    return check_block(r, r->body, r->nbody, &sc, err);
}

/* A fresh name must avoid the text a code template emits, exactly as it avoids
   a string template's. The literals are in the tree rather than in one string,
   so finding them is a walk instead of a strstr. */
static int expr_mentions(Expr *e, const char *n);

static int block_mentions(Stmt *v, int c, const char *n)
{
    for (int i = 0; i < c; i++) {
        if (expr_mentions(v[i].e, n) || expr_mentions(v[i].sep, n)) return 1;
        if (block_mentions(v[i].body, v[i].nbody, n)) return 1;
        if (block_mentions(v[i].alt,  v[i].nalt,  n)) return 1;
    }
    return 0;
}

static int expr_mentions(Expr *e, const char *n)
{
    if (!e) return 0;
    if (e->kind == E_TEXT) return strstr(e->s, n) != NULL;
    if (e->kind == E_NAME || e->kind == E_CALL) {
        if (!strcmp(e->s, n)) return 1;
        for (int i = 0; i < e->nargs; i++)
            if (expr_mentions(e->args[i], n)) return 1;
        return 0;
    }
    return expr_mentions(e->a, n) || expr_mentions(e->b, n);
}

int code_mentions(Rule *r, const char *n)
{
    return block_mentions(r->body, r->nbody, n);
}

/* -------------------------------------------------------------- evaluating */

enum { V_TEXT, V_INT, V_BOOL, V_LIST };

typedef struct {
    int    kind;
    char  *text;
    long   num;
    char **items;
    int    nitems, set, level;
} Val;

typedef struct Frame {
    const char   *name;
    Val           v;
    struct Frame *up;
} Frame;

typedef struct {
    Rule  *r;
    Bind  *b;
    int    nb;
    Frame *env;
    Buf    out;
    char  *err;
} Ev;

static Val v_text(char *t)
{
    Val v; memset(&v, 0, sizeof v);
    v.kind = V_TEXT; v.text = t; v.level = LEVEL_ATOM; v.set = *t != 0;
    return v;
}

static Val v_int(long n)
{
    Val v; memset(&v, 0, sizeof v);
    v.kind = V_INT; v.num = n; v.text = xfmt("%ld", n);
    return v;
}

static Val v_bool(int b)
{
    Val v; memset(&v, 0, sizeof v);
    v.kind = V_BOOL; v.num = b; v.text = xstrdup(b ? "true" : "false");
    return v;
}

static int truthy(Val v)
{
    switch (v.kind) {
    case V_BOOL: case V_INT: return v.num != 0;
    case V_LIST: return v.nitems != 0;
    default:     return v.text && *v.text;
    }
}

static char *as_text(Val v)
{
    if (v.kind != V_LIST) return v.text ? v.text : xstrdup("");
    Buf b = {0};
    for (int i = 0; i < v.nitems; i++) buf_str(&b, v.items[i]);
    if (!b.p) buf_str(&b, "");
    return b.p;
}

static int lookup(Ev *ev, const char *n, Val *out)
{
    for (Frame *f = ev->env; f; f = f->up)
        if (!strcmp(f->name, n)) { *out = f->v; return 1; }

    for (int i = 0; i < ev->nb; i++) {
        if (strcmp(ev->b[i].name, n)) continue;
        Bind *b = &ev->b[i];
        Val v;
        memset(&v, 0, sizeof v);
        if (b->islist) {
            v.kind = V_LIST;
            v.items = b->items;
            v.nitems = b->nitems;
            v.text = b->val;
        } else {
            v.kind = V_TEXT;
            v.text = b->val;
        }
        v.set   = b->set;
        v.level = b->level >= 0 ? b->level : LEVEL_ATOM;
        *out = v;
        return 1;
    }
    return 0;
}

static char *replace_all(const char *s, const char *from, const char *to)
{
    if (!*from) return xstrdup(s);
    Buf b = {0};
    size_t n = strlen(from);
    for (size_t i = 0; s[i];) {
        if (!strncmp(s + i, from, n)) { buf_str(&b, to); i += n; }
        else { buf_ch(&b, s[i]); i++; }
    }
    if (!b.p) buf_str(&b, "");
    return b.p;
}

static int eval(Ev *ev, Expr *e, Val *out);

static int call(Ev *ev, Expr *e, Val *out)
{
    Val a[3];
    for (int i = 0; i < e->nargs; i++)
        if (eval(ev, e->args[i], &a[i]) < 0) return -1;

    if (!strcmp(e->s, "matched")) { *out = v_bool(a[0].set);            return 0; }
    if (!strcmp(e->s, "count"))   { *out = v_int(a[0].kind == V_LIST ? a[0].nitems
                                                : (a[0].set ? 1 : 0)); return 0; }
    if (!strcmp(e->s, "level"))   { *out = v_int(a[0].level);           return 0; }
    if (!strcmp(e->s, "fresh"))   { *out = v_text(pt_fresh(as_text(a[0]))); return 0; }
    if (!strcmp(e->s, "replace")) {
        *out = v_text(replace_all(as_text(a[0]), as_text(a[1]), as_text(a[2])));
        return 0;
    }
    if (!strcmp(e->s, "drop")) {
        char  *t = as_text(a[0]);
        size_t n = strlen(t);
        long   f = a[1].kind == V_INT ? a[1].num : 0;
        long   b = a[2].kind == V_INT ? a[2].num : 0;
        if (f < 0) f = 0;
        if (b < 0) b = 0;
        if ((size_t)(f + b) >= n) *out = v_text(xstrdup(""));
        else *out = v_text(xstrndup(t + f, n - (size_t)(f + b)));
        return 0;
    }
    if (!strcmp(e->s, "group")) {
        char *t = as_text(a[0]);
        long  n = a[1].kind == V_INT ? a[1].num : 0;
        *out = v_text(a[0].level < n ? xfmt("(%s)", t) : t);
        return 0;
    }
    ev->err = xfmt("%s:%d: no such thing as '%s'", ev->r->file, ev->r->line, e->s);
    return -1;
}

static int eval(Ev *ev, Expr *e, Val *out)
{
    switch (e->kind) {
    case E_TEXT: *out = v_text(e->s); return 0;
    case E_INT:  *out = v_int(e->n);  return 0;
    case E_NAME:
        if (lookup(ev, e->s, out)) return 0;
        ev->err = xfmt("%s:%d: nothing here is called '%s'", ev->r->file, ev->r->line, e->s);
        return -1;
    case E_CALL: return call(ev, e, out);
    case E_NOT: {
        Val v;
        if (eval(ev, e->a, &v) < 0) return -1;
        *out = v_bool(!truthy(v));
        return 0;
    }
    case E_BIN: {
        Val x, y;
        if (eval(ev, e->a, &x) < 0) return -1;
        if (!strcmp(e->s, "and")) { *out = v_bool(truthy(x) && (eval(ev, e->b, &y) == 0 && truthy(y))); return ev->err ? -1 : 0; }
        if (!strcmp(e->s, "or"))  { *out = v_bool(truthy(x) || (eval(ev, e->b, &y) == 0 && truthy(y))); return ev->err ? -1 : 0; }
        if (eval(ev, e->b, &y) < 0) return -1;
        if (!strcmp(e->s, "+")) { *out = v_text(xfmt("%s%s", as_text(x), as_text(y))); return 0; }

        int num = x.kind == V_INT && y.kind == V_INT;
        int cmp = num ? (x.num < y.num ? -1 : x.num > y.num ? 1 : 0)
                      : strcmp(as_text(x), as_text(y));
        if (!strcmp(e->s, "==")) *out = v_bool(cmp == 0);
        else if (!strcmp(e->s, "!=")) *out = v_bool(cmp != 0);
        else if (!strcmp(e->s, "<"))  *out = v_bool(cmp <  0);
        else if (!strcmp(e->s, ">"))  *out = v_bool(cmp >  0);
        else if (!strcmp(e->s, "<=")) *out = v_bool(cmp <= 0);
        else                          *out = v_bool(cmp >= 0);
        return 0;
    }
    }
    return -1;
}

static int run(Ev *ev, Stmt *v, int n)
{
    for (int i = 0; i < n && !ev->err; i++) {
        Stmt *s = &v[i];
        Val   c;
        switch (s->kind) {
        case S_EMIT:
            if (eval(ev, s->e, &c) < 0) return -1;
            buf_str(&ev->out, as_text(c));
            break;
        case S_IF:
            if (eval(ev, s->e, &c) < 0) return -1;
            if (truthy(c)) { if (run(ev, s->body, s->nbody) < 0) return -1; }
            else           { if (run(ev, s->alt,  s->nalt)  < 0) return -1; }
            break;
        case S_FOR: {
            if (eval(ev, s->e, &c) < 0) return -1;
            char  *sep = NULL;
            if (s->sep) {
                Val sv;
                if (eval(ev, s->sep, &sv) < 0) return -1;
                sep = as_text(sv);
            }
            int    n2 = c.kind == V_LIST ? c.nitems : (c.set ? 1 : 0);
            for (int k = 0; k < n2; k++) {
                if (k && sep) buf_str(&ev->out, sep);
                Frame f;
                f.name = s->var;
                f.v    = v_text(c.kind == V_LIST ? c.items[k] : as_text(c));
                f.up   = ev->env;
                ev->env = &f;
                int rc = run(ev, s->body, s->nbody);
                ev->env = f.up;
                if (rc < 0) return -1;
            }
            break;
        }
        }
    }
    return ev->err ? -1 : 0;
}

char *code_eval(Rule *r, Bind *b, int nb, char **err)
{
    Ev ev;
    memset(&ev, 0, sizeof ev);
    ev.r = r; ev.b = b; ev.nb = nb;
    if (run(&ev, r->body, r->nbody) < 0) { *err = ev.err; return NULL; }
    if (!ev.out.p) buf_str(&ev.out, "");
    return ev.out.p;
}
