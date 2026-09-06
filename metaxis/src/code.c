/* code.c -- the second kind of template.
 *
 * `=> { … }` instead of `=> "…"`. A string template can splice and nothing
 * else, which is where five separate warts in this repository come from: a
 * repeated group that wants a template per element rather than one joiner, an
 * optional part that cannot make the output differ, parentheses written
 * unconditionally because nobody could ask an operand its level, and a literal
 * moved rather than translated.
 *
 * The language is Metaxis's own, so it lives outside the strings; the foreign
 * text it emits lives inside them. That is the one rule read once more, and it
 * is why `{` after the `=>` is enough to tell the two forms apart -- a template
 * has always been a string until now, and a string never starts with a brace.
 *
 * It is deliberately small. Every form below names a customer in
 * docs/ROADMAP.md, and anything that cannot name one is not here.
 */
#include "mx.h"

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
    size_t close;   /* just past the outermost '}', before the lookahead */
} C;

static void cerr(C *c, const char *msg)
{
    if (!c->err) c->err = xfmt("%s:%d: %s", c->file, c->line, msg);
}

static const char *PUNCT[] = {
    "==", "!=", "<=", ">=", "(", ")", "{", "}", ",", "+", "-", "*", "/", "%",
    "<", ">", ";", NULL
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

/* `*` `/` `%` bind tighter than `+` `-`, which bind tighter than a comparison.
   `not` stays where it was, under both, so `not a + b` still reads as it did. */
static Expr *e_mul(C *c)
{
    Expr *a = e_not(c);
    for (;;) {
        if (!a || c->err) return a;
        const char *op = at(c, C_PUNCT, "*") ? "*"
                       : at(c, C_PUNCT, "/") ? "/"
                       : at(c, C_PUNCT, "%") ? "%" : NULL;
        if (!op) return a;
        cnext(c);
        Expr *e = mk(E_BIN);
        e->s = xstrdup(op);
        e->a = a;
        e->b = e_not(c);
        if (!e->b) return NULL;
        a = e;
    }
}

static Expr *e_cat(C *c)
{
    Expr *a = e_mul(c);
    for (;;) {
        if (!a || c->err) return a;
        const char *op = at(c, C_PUNCT, "+") ? "+"
                       : at(c, C_PUNCT, "-") ? "-" : NULL;
        if (!op) return a;
        cnext(c);
        Expr *e = mk(E_BIN);
        e->s = xstrdup(op);
        e->a = a;
        e->b = e_mul(c);
        if (!e->b) return NULL;
        a = e;
    }
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
            /* `for i, x in l` binds the position as well as the turn, the way
               Go and Python's enumerate read it: the first name is the index.
               With one name there is no index and nothing changes. */
            if (take(c, C_PUNCT, ",")) {
                if (c->kind != C_NAME || code_is_keyword(c->text)) {
                    cerr(c, "expected a name after ',' in 'for'");
                    return NULL;
                }
                s.idx = s.var;
                s.var = c->text;
                cnext(c);
            }
            if (!take(c, C_NAME, "in")) { cerr(c, "expected 'in'"); return NULL; }
            s.e = e_or(c);
            if (!s.e) return NULL;
            if (take(c, C_NAME, "sep")) {
                s.sep = e_or(c);
                if (!s.sep) return NULL;
            }
            s.body = block(c, &s.nbody);
            if (c->err) return NULL;
        } else if (c->kind == C_NAME && !code_is_keyword(c->text)) {
            /* A name with a `(` after it, where a statement was expected, is a
               call to a named piece of template. Nothing else can appear here,
               so no word had to be reserved for it. */
            Expr *e = mk(E_CALL);
            e->s = c->text;
            cnext(c);
            /* A word with no `(` after it is not a call and is not assumed to be
               one: it is a statement this language has no form for, and saying
               which forms exist is more use than guessing at the one meant. */
            if (!take(c, C_PUNCT, "(")) {
                cerr(c, "expected 'emit', 'if', 'for' or a template call");
                return NULL;
            }
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
            s.kind = S_CALL;
            s.e = e;
        } else {
            cerr(c, "expected 'emit', 'if', 'for' or a template call");
            return NULL;
        }
        v = push(v, &n, s);
    }
    if (c->err) return NULL;
    /* c->i already sits past the '}'; cnext would read one token beyond it, and
       what follows a template is the directive reader's business -- `terminated`
       lives there. */
    c->close = c->i;
    cnext(c);
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
    *i = c.close;
    return v;
}

/* ------------------------------------------------- checking, at declaration */

typedef struct { const char *n[32]; int n_; } Scope;

static const struct { const char *name; int args; const char *what; } BUILTIN[] = {
    { "matched", 1, "whether a hole's group matched"          },
    { "count",   1, "how many turns a repeated hole took"     },
    { "at",      2, "the turn at a position, counting from 0"  },
    { "num",     1, "a hole's text read as a number"            },
    { "level",   1, "the level of what filled a hole"         },
    { "terminated", 1, "whether what filled a hole ends a statement" },
    { "group",   2, "a hole, bracketed when its level is lower" },
    { "replace", 3, "one text with another inside a third"    },
    { "drop",    3, "a text with characters off each end"      },
    { "indent",  2, "a text with every line moved right"       },
    { "fresh",   1, "a name nobody else has"                  },
    { NULL, 0, NULL }
};

static int check_expr(Rule *r, const char *where, Expr *e, Scope *sc, char **err)
{
    if (!e) return 0;
    switch (e->kind) {
    case E_TEXT: case E_INT: return 0;
    case E_NOT:  return check_expr(r, where, e->a, sc, err);
    case E_BIN:  return check_expr(r, where, e->a, sc, err) ||
                 check_expr(r, where, e->b, sc, err);
    case E_NAME:
        for (int i = 0; i < sc->n_; i++) if (!strcmp(sc->n[i], e->s)) return 0;
        if (r && rule_has_hole(r, e->s)) return 0;
        if (!r) {
            *err = xfmt("%s: '%s' is not one of this template's parameters",
                        where, e->s);
            return -1;
        }
        *err = xfmt("%s: the template uses '%s' and the pattern has no such hole",
                    where, e->s);
        return -1;
    case E_CALL:
        for (int i = 0; BUILTIN[i].name; i++) {
            if (strcmp(BUILTIN[i].name, e->s)) continue;
            if (e->nargs != BUILTIN[i].args) {
                *err = xfmt("%s: '%s' takes %d and was given %d -- it gives %s",
                            where, e->s, BUILTIN[i].args, e->nargs, BUILTIN[i].what);
                return -1;
            }
            for (int a = 0; a < e->nargs; a++)
                if (check_expr(r, where, e->args[a], sc, err) < 0) return -1;
            return 0;
        }
        /* Not a builtin. It might be a template used where a value was wanted,
           and templates are not all in yet, so this is settled at seal -- where
           the difference between *no such thing* and *that is a statement* can
           actually be told. */
        for (int a = 0; a < e->nargs; a++)
            if (check_expr(r, where, e->args[a], sc, err) < 0) return -1;
        return 0;
    }
    return 0;
}

static int check_block(Rule *r, const char *where, Stmt *v, int n, Scope *sc, char **err)
{
    for (int i = 0; i < n; i++) {
        /* A call statement's *name* is resolved at seal, when every template is
           in and the order a file wrote them cannot change the answer. Its
           arguments are ordinary expressions and are checked here. */
        if (v[i].kind == S_CALL) {
            for (int a = 0; a < v[i].e->nargs; a++)
                if (check_expr(r, where, v[i].e->args[a], sc, err) < 0) return -1;
        } else if (check_expr(r, where, v[i].e, sc, err) < 0) return -1;
        if (v[i].sep && check_expr(r, where, v[i].sep, sc, err) < 0) return -1;
        if (v[i].kind == S_FOR) {
            if (sc->n_ >= 32) {
                *err = xfmt("%s: loops nested more than 32 deep", where);
                return -1;
            }
            /* `r` is NULL when this is a template's body, which has no rule
               and no holes to collide with -- a template sees its parameters
               and nothing else. The guard is the same one `check_expr` has;
               without it a `for` inside a template read through a null rule and
               crashed, which is what happens to a branch nothing had written
               yet. See POSTMORTEM.md 12. */
            for (int w = 0; w < 2; w++) {
                const char *nm = w ? v[i].idx : v[i].var;
                if (!nm) continue;
                if (r && rule_has_hole(r, nm)) {
                    *err = xfmt("%s: the loop variable '%s' is also a hole -- one of"
                                " them has to be called something else", where, nm);
                    return -1;
                }
            }
            if (v[i].idx && !strcmp(v[i].idx, v[i].var)) {
                *err = xfmt("%s: 'for %s, %s' names the position and the turn the"
                            " same thing", where, v[i].idx, v[i].var);
                return -1;
            }
            sc->n[sc->n_++] = v[i].var;
            if (v[i].idx) {
                if (sc->n_ >= 32) {
                    *err = xfmt("%s: loops nested more than 32 deep", where);
                    return -1;
                }
                sc->n[sc->n_++] = v[i].idx;
            }
            int rc = check_block(r, where, v[i].body, v[i].nbody, sc, err);
            sc->n_--;
            if (v[i].idx) sc->n_--;
            if (rc < 0) return -1;
        } else {
            if (check_block(r, where, v[i].body, v[i].nbody, sc, err) < 0) return -1;
        }
        if (check_block(r, where, v[i].alt, v[i].nalt, sc, err) < 0) return -1;
    }
    return 0;
}

int code_check(Rule *r, char **err)
{
    Scope sc;
    sc.n_ = 0;
    return check_block(r, xfmt("%s:%d", r->file, r->line), r->body, r->nbody, &sc, err);
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
    int    nitems, set, level, terminated;
} Val;

typedef struct Frame {
    const char   *name;
    Val           v;
    struct Frame *up;
} Frame;

typedef struct {
    Grammar *g;
    int    depth;
    Rule  *r;
    Bind  *b;
    int    nb;
    Frame *env;
    Buf    out;
    char  *err;
    /* One application, one set of fresh names, exactly as `subst` keeps for
       `{~t}`: two `fresh("L")` in one template are the same name and the next
       use of the rule gets a different one. Without this a template that needs
       a label in two places -- a branch and the label it jumps to -- could not
       have one, which is what writing a code generator found. */
    struct { char *label, *name; } fr[32];
    int    nfr;
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
        v.set        = b->set;
        v.level      = b->level >= 0 ? b->level : LEVEL_ATOM;
        v.terminated = b->terminated;
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

static Tmpl *tmpl_find(Grammar *g, const char *name)
{
    for (int i = 0; i < g->ntmpl; i++)
        if (!strcmp(g->tmpl[i].name, name)) return &g->tmpl[i];
    return NULL;
}

static int call(Ev *ev, Expr *e, Val *out)
{
    Val a[3];
    for (int i = 0; i < e->nargs; i++)
        if (eval(ev, e->args[i], &a[i]) < 0) return -1;

    if (!strcmp(e->s, "matched")) { *out = v_bool(a[0].set);            return 0; }
    if (!strcmp(e->s, "count"))   { *out = v_int(a[0].kind == V_LIST ? a[0].nitems
                                                : (a[0].set ? 1 : 0)); return 0; }
    if (!strcmp(e->s, "at")) {
        long  n = a[1].kind == V_INT ? a[1].num : 0;
        int   have = a[0].kind == V_LIST ? a[0].nitems : (a[0].set ? 1 : 0);
        /* Out of range is an error and not an empty string. Two groups walked
           together is what `at` is for, and two groups of different lengths is
           the mistake that makes; saying so is the whole value of noticing. */
        if (n < 0 || n >= have) {
            ev->err = xfmt("%s:%d: 'at' was given %ld and there %s %d",
                           ev->r->file, ev->r->line, n,
                           have == 1 ? "is" : "are", have);
            return -1;
        }
        *out = v_text(a[0].kind == V_LIST ? a[0].items[n] : as_text(a[0]));
        return 0;
    }
    if (!strcmp(e->s, "num")) {
        if (a[0].kind == V_INT) { *out = a[0]; return 0; }
        char *t = as_text(a[0]);
        while (*t == ' ' || *t == '\t' || *t == '\n' || *t == '\r') t++;
        char *end = NULL;
        long  n = strtol(t, &end, 10);
        while (end && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end++;
        /* The whole text, or none of it. Reading 12 out of `12abc` is the kind
           of quiet wrongness this tree keeps finding, and there is no reason to
           add one on purpose. */
        if (end == t || !end || *end) {
            ev->err = xfmt("%s:%d: 'num' wants a number and was given '%s'",
                           ev->r->file, ev->r->line, as_text(a[0]));
            return -1;
        }
        *out = v_int(n);
        return 0;
    }
    if (!strcmp(e->s, "level"))   { *out = v_int(a[0].level);           return 0; }
    if (!strcmp(e->s, "terminated")) { *out = v_bool(a[0].terminated);   return 0; }
    if (!strcmp(e->s, "fresh")) {
        char *label = as_text(a[0]);
        for (int i = 0; i < ev->nfr; i++)
            if (!strcmp(ev->fr[i].label, label)) { *out = v_text(ev->fr[i].name); return 0; }
        char *n = pt_fresh(label);
        if (!n) {
            ev->err = xfmt("%s:%d: no fresh name for 'fresh(\"%s\")' is free",
                           ev->r->file, ev->r->line, label);
            return -1;
        }
        if (ev->nfr >= 32) {
            ev->err = xfmt("%s:%d: more than 32 fresh labels in one template",
                           ev->r->file, ev->r->line);
            return -1;
        }
        ev->fr[ev->nfr].label = label;
        ev->fr[ev->nfr].name  = n;
        ev->nfr++;
        *out = v_text(n);
        return 0;
    }
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
    /* Every line moved right, including the first, and an empty line left
       empty -- trailing whitespace on a blank line is noise in every language
       this has emitted so far. It is *block* indentation rather than the
       align-to-the-splice-column kind, because what asked for it was a brace:
       examples/code.mx emitted C with no indentation at all, which compiles and
       reads like nothing anybody wrote.

       The string template has no equivalent and is not getting one until it has
       a customer. `examples/pascal.mx` is the file that would use it, and its
       output is recorded as deliberately wrong for other reasons. */
    if (!strcmp(e->s, "indent")) {
        char *t = as_text(a[0]);
        long  n = a[1].kind == V_INT ? a[1].num : 0;
        if (n < 0) n = 0;
        Buf b = {0};
        int at_line_start = 1;
        for (size_t i = 0; t[i]; i++) {
            if (at_line_start && t[i] != '\n')
                for (long k = 0; k < n; k++) buf_ch(&b, ' ');
            buf_ch(&b, t[i]);
            at_line_start = t[i] == '\n';
        }
        if (!b.p) buf_str(&b, "");
        *out = v_text(b.p);
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

        int both = x.kind == V_INT && y.kind == V_INT;

        /* `+` joins text and adds numbers, which is the rule comparison has
           always used: numeric when both sides already are, and never by
           reading a number out of text that only looks like one. `num(h)` is
           how a hole says it meant a number. */
        if (!strcmp(e->s, "+")) {
            if (both) { *out = v_int(x.num + y.num); return 0; }
            *out = v_text(xfmt("%s%s", as_text(x), as_text(y)));
            return 0;
        }
        if (!strcmp(e->s, "-") || !strcmp(e->s, "*") ||
            !strcmp(e->s, "/") || !strcmp(e->s, "%")) {
            if (!both) {
                ev->err = xfmt("%s:%d: '%s' wants two numbers and was given '%s' and"
                               " '%s' -- num(h) reads a hole as one",
                               ev->r->file, ev->r->line, e->s, as_text(x), as_text(y));
                return -1;
            }
            if ((e->s[0] == '/' || e->s[0] == '%') && y.num == 0) {
                ev->err = xfmt("%s:%d: '%s' by zero", ev->r->file, ev->r->line, e->s);
                return -1;
            }
            switch (e->s[0]) {
            case '-': *out = v_int(x.num - y.num); break;
            case '*': *out = v_int(x.num * y.num); break;
            case '/': *out = v_int(x.num / y.num); break;
            default:  *out = v_int(x.num % y.num); break;
            }
            return 0;
        }

        int num = both;
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
        case S_CALL: {
            Tmpl *t = tmpl_find(ev->g, s->e->s);
            if (!t) {   /* resolved at seal; unreachable unless that was skipped */
                ev->err = xfmt("%s:%d: no template called '%s'",
                               ev->r->file, ev->r->line, s->e->s);
                return -1;
            }
            if (++ev->depth > 64) {
                ev->err = xfmt("%s:%d: templates called more than 64 deep -- '%s'"
                               " calls itself without stopping",
                               ev->r->file, ev->r->line, t->name);
                return -1;
            }
            /* Arguments are evaluated where the call is written; the body then
               runs with *only* the parameters in scope, so a template cannot
               reach into the rule that called it and can be read on its own. */
            Frame  fr[8];
            Val    av[8];
            int    np = t->nparam < 8 ? t->nparam : 8;
            for (int k = 0; k < np; k++)
                if (eval(ev, s->e->args[k], &av[k]) < 0) return -1;
            Frame *save = ev->env;
            ev->env = NULL;
            for (int k = 0; k < np; k++) {
                fr[k].name = t->param[k];
                fr[k].v    = av[k];
                fr[k].up   = ev->env;
                ev->env    = &fr[k];
            }
            int rc = run(ev, t->body, t->nbody);
            ev->env = save;
            ev->depth--;
            if (rc < 0) return -1;
            break;
        }
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
                Frame f, fi;
                f.name = s->var;
                f.v    = v_text(c.kind == V_LIST ? c.items[k] : as_text(c));
                f.up   = ev->env;
                ev->env = &f;
                if (s->idx) {
                    fi.name = s->idx;
                    fi.v    = v_int(k);
                    fi.up   = ev->env;
                    ev->env = &fi;
                }
                int rc = run(ev, s->body, s->nbody);
                /* Unwind to what was there before *both* frames. `fi.up` is
                   `&f`, so restoring to it would leave `f` on the stack and the
                   next turn would link `f` to itself. */
                ev->env = f.up;
                if (rc < 0) return -1;
            }
            break;
        }
        }
    }
    return ev->err ? -1 : 0;
}

char *code_eval(Grammar *g, Rule *r, Bind *b, int nb, char **err)
{
    Ev ev;
    memset(&ev, 0, sizeof ev);
    ev.g = g; ev.r = r; ev.b = b; ev.nb = nb;
    if (run(&ev, r->body, r->nbody) < 0) { *err = ev.err; return NULL; }
    if (!ev.out.p) buf_str(&ev.out, "");
    return ev.out.p;
}

/* ------------------------------------------------- resolving template calls */

static int is_builtin(const char *n)
{
    for (int i = 0; BUILTIN[i].name; i++) if (!strcmp(BUILTIN[i].name, n)) return 1;
    return 0;
}

/* A call in expression position must be a builtin. The two ways to get that
   wrong are worth telling apart, because both are things somebody will write. */
static int resolve_expr(Grammar *g, const char *where, Expr *e, char **err)
{
    if (!e) return 0;
    if (e->kind == E_CALL && !is_builtin(e->s)) {
        if (tmpl_find(g, e->s))
            *err = xfmt("%s: '%s' is a template -- it is called as a statement on a"
                        " line of its own and emits, so it has no value to use here",
                        where, e->s);
        else
            *err = xfmt("%s: no such thing as '%s'", where, e->s);
        return -1;
    }
    if (resolve_expr(g, where, e->a, err) < 0) return -1;
    if (resolve_expr(g, where, e->b, err) < 0) return -1;
    for (int i = 0; i < e->nargs; i++)
        if (resolve_expr(g, where, e->args[i], err) < 0) return -1;
    return 0;
}

static int resolve_block(Grammar *g, const char *where, Stmt *v, int n, char **err)
{
    for (int i = 0; i < n; i++) {
        if (v[i].kind != S_CALL && resolve_expr(g, where, v[i].e, err) < 0) return -1;
        if (resolve_expr(g, where, v[i].sep, err) < 0) return -1;
        if (v[i].kind == S_CALL) {
            for (int a = 0; a < v[i].e->nargs; a++)
                if (resolve_expr(g, where, v[i].e->args[a], err) < 0) return -1;
            if (is_builtin(v[i].e->s)) {
                *err = xfmt("%s: '%s' is a builtin and gives a value -- put it in an"
                            " 'emit', not on a line of its own", where, v[i].e->s);
                return -1;
            }
            Tmpl *t = tmpl_find(g, v[i].e->s);
            if (!t) {
                *err = xfmt("%s: no template called '%s'", where, v[i].e->s);
                return -1;
            }
            if (v[i].e->nargs != t->nparam) {
                *err = xfmt("%s: '%s' takes %d and was given %d -- declared at %s:%d",
                            where, t->name, t->nparam, v[i].e->nargs, t->file, t->line);
                return -1;
            }
        }
        if (resolve_block(g, where, v[i].body, v[i].nbody, err) < 0) return -1;
        if (resolve_block(g, where, v[i].alt,  v[i].nalt,  err) < 0) return -1;
    }
    return 0;
}

int code_check_calls(Grammar *g, char **err)
{
    for (int i = 0; i < g->nrule; i++) {
        Rule *r = &g->rule[i];
        if (!r->body) continue;
        if (resolve_block(g, xfmt("%s:%d", r->file, r->line), r->body, r->nbody, err) < 0)
            return -1;
    }
    for (int i = 0; i < g->ntmpl; i++) {
        Tmpl *t = &g->tmpl[i];
        char *where = xfmt("%s:%d", t->file, t->line);
        /* A template's body sees its parameters and nothing else, so it is
           checked here in full rather than where it was declared -- the calls
           in it need every other template to be in first. */
        Scope sc;
        sc.n_ = 0;
        for (int k = 0; k < t->nparam && k < 32; k++) sc.n[sc.n_++] = t->param[k];
        if (check_block(NULL, where, t->body, t->nbody, &sc, err) < 0) return -1;
        if (resolve_block(g, where, t->body, t->nbody, err) < 0) return -1;
    }
    return 0;
}
