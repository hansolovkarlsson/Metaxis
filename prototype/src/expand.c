/* expand.c -- the half the header wrote.
 *
 * A pattern that begins with a hole is a led rule and needs a level; one that
 * begins with a word is a nud rule and does not. Nobody declares which -- it
 * is read off the shape, which is what quoting the words bought.
 *
 * Candidates under one leading word are tried longest first with the token
 * cursor restored on failure. Proto matches them in lockstep and needs no
 * backtracking; this backtracks, which is slower and shorter, and the inputs
 * are files.
 */
#include "pt.h"

#include <ctype.h>
#include <string.h>

typedef struct { char *label; char *name; } Fresh;

/* ------------------------------------------------------------ fresh names */

/* `{~t}` is a name nobody else has. It closes the half of the hygiene problem
   a template can close on its own: the half where the template *introduces* a
   name. The other half -- a template that reaches out for a name the caller
   shadowed -- wants a scope, and a template that is a string has no way to see
   one. See examples/hygiene.pt, which still gets that half wrong on purpose.

   Taken means: it occurs anywhere in the source being expanded, or in any
   template any rule declared, including the ones a @use brought in. That is a
   substring test and so is conservative in the safe direction -- `t__1` is
   refused while `t__12` is in the file -- and it costs one scan per name. */
static const char *fresh_src;
static Grammar    *fresh_g;
static int         fresh_n;

static int name_taken(const char *c)
{
    if (fresh_src && strstr(fresh_src, c)) return 1;
    for (int i = 0; fresh_g && i < fresh_g->nrule; i++) {
        Rule *r = &fresh_g->rule[i];
        if (r->tmpl && strstr(r->tmpl, c)) return 1;
        if (r->body && code_mentions(r, c))  return 1;
    }
    return 0;
}

static char *fresh_name(const char *label);

char *pt_fresh(const char *label) { return fresh_name(label); }

static char *fresh_name(const char *label)
{
    for (int tries = 0; tries < 100000; tries++) {
        char *c = xfmt("%s__%d", label, ++fresh_n);
        if (!name_taken(c)) return c;
    }
    return NULL;
}

typedef struct {
    Grammar *g;
    Toks    *tk;
    int      i, far, depth;
    char    *err;
} P;

#define MAXDEPTH 400

/* What a parsed expression turns out to be, besides its text: the level of the
   rule that produced it, so a code template can ask whether an operand needs
   bracketing, and whether that rule said its output ends a statement. */
typedef struct { int level, terminated; } Out;

static Tok *cur(P *p) { return &p->tk->t[p->i]; }

static int tok_is(P *p, const char *w)
{
    Tok *t = cur(p);
    if (t->kind == T_EOF) return 0;
    size_t n = strlen(w);
    return t->n == n && !memcmp(t->p, w, n);
}

static void adv(P *p)
{
    if (cur(p)->kind != T_EOF) p->i++;
    if (p->i > p->far) p->far = p->i;
}

/* --------------------------------------------------------------- templates */

static char *subst(P *p, Rule *r, Bind *b, int nb)
{
    Buf out = {0};
    const char *s = r->tmpl;

    /* One application, one set of fresh names: two `{~t}` in a template are the
       same name, and the next use of the rule gets a different one. */
    Fresh *fr  = xmalloc(sizeof *fr * (strlen(s) / 4 + 2));
    int    nfr = 0;

    for (size_t i = 0; s[i];) {
        if (s[i] == '{' && s[i + 1] == '{') { buf_ch(&out, '{'); i += 2; continue; }
        if (s[i] == '}' && s[i + 1] == '}') { buf_ch(&out, '}'); i += 2; continue; }
        if (s[i] == '{' && s[i + 1] == '~') {
            size_t j = i + 2;
            while (s[j] && s[j] != '}') j++;
            if (!s[j]) {
                p->err = xfmt("%s:%d: unclosed '{' in a template", r->file, r->line);
                return NULL;
            }
            char *label = xstrndup(s + i + 2, j - i - 2);
            char *name  = NULL;
            for (int k = 0; k < nfr; k++)
                if (!strcmp(fr[k].label, label)) name = fr[k].name;
            if (!name) {
                name = fresh_name(label);
                if (!name) {
                    p->err = xfmt("%s:%d: no fresh name for '{~%s}' is free",
                                  r->file, r->line, label);
                    return NULL;
                }
                fr[nfr].label = label;
                fr[nfr].name  = name;
                nfr++;
            }
            buf_str(&out, name);
            i = j + 1;
            continue;
        }
        if (s[i] == '{') {
            size_t j = i + 1;
            while (s[j] && s[j] != '}') j++;
            if (!s[j]) {
                p->err = xfmt("%s:%d: unclosed '{' in a template", r->file, r->line);
                return NULL;
            }
            char *name = xstrndup(s + i + 1, j - i - 1);
            char *val  = NULL;
            for (int k = 0; k < nb; k++)
                if (!strcmp(b[k].name, name)) val = b[k].val;
            if (!val) {
                p->err = xfmt("%s:%d: the template splices '{%s}' and the pattern has no such hole",
                              r->file, r->line, name);
                return NULL;
            }
            buf_str(&out, val);
            i = j + 1;
            continue;
        }
        buf_ch(&out, s[i]);
        i++;
    }
    if (!out.p) buf_str(&out, "");
    return out.p;
}

/* ------------------------------------------------------------------- parser */

static char *p_expr(P *p, int minbp, Out *o);
static char *p_stmts(P *p, const char *term);

/* Every hole the pattern declares, groups included, starts bound to nothing.
   A group that matched no turns leaves its holes empty rather than unbound, so
   a template never has to ask whether a part was there. */
static int count_holes(Elem *el, int nel)
{
    int n = 0;
    for (int i = 0; i < nel; i++)
        if (el[i].kind == EL_HOLE) n++;
        else if (el[i].kind == EL_GROUP) n += count_holes(el[i].sub, el[i].nsub);
    return n;
}

static void bind_pre(Elem *el, int nel, Bind *b, int *nb, int inrep)
{
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_HOLE) {
            memset(&b[*nb], 0, sizeof b[*nb]);
            b[*nb].name   = el[i].hole;
            b[*nb].val    = xstrdup("");
            b[*nb].level  = -1;
            b[*nb].islist = inrep;
            (*nb)++;
        } else if (el[i].kind == EL_GROUP) {
            bind_pre(el[i].sub, el[i].nsub, b, nb,
                     inrep || el[i].rep != REP_ONE);
        }
    }
}

/* A hole in a repeated group keeps both shapes: the turns joined, which is what
   a string template splices, and the turns themselves, which is what a code
   template loops over. Keeping only the first is what `join` used to throw
   away, and is the whole difference between the two kinds of template. */
static void bind_put(Bind *b, int nb, const char *name, char *val,
                     int append, const char *join, int level)
{
    for (int i = 0; i < nb; i++) {
        if (strcmp(b[i].name, name)) continue;
        if (append) {
            char **v = xmalloc(sizeof *v * (size_t)(b[i].nitems + 1));
            if (b[i].items) memcpy(v, b[i].items, sizeof *v * (size_t)b[i].nitems);
            v[b[i].nitems] = val;
            b[i].items = v;
            b[i].nitems++;
            b[i].val = b[i].set ? xfmt("%s%s%s", b[i].val, join ? join : "", val) : val;
        } else {
            b[i].val = val;
        }
        b[i].set   = 1;
        b[i].level = level;
        return;
    }
}

static Bind *bind_copy(Bind *b, int nb)
{
    Bind *c = xmalloc(sizeof *c * (size_t)(nb + 1));
    memcpy(c, b, sizeof *c * (size_t)nb);
    return c;
}

static int m_elems(P *p, Rule *r, Elem *el, int nel, int tail,
                   int append, const char *join, Bind *b, int nb);

static int m_group(P *p, Rule *r, Elem *e, Bind *b, int nb)
{
    const char *join = e->join;

    if (e->rep == REP_ONE) {
        int   save = p->i;
        Bind *snap = bind_copy(b, nb);
        /* A group is read at binding power 0: it is delimited by its own words,
           not by precedence. */
        if (!m_elems(p, r, e->sub, e->nsub, 0, 0, NULL, b, nb)) {
            p->i = save;
            memcpy(b, snap, sizeof *b * (size_t)nb);
        }
        return 1;
    }

    int turns = 0;
    for (;;) {
        int   save = p->i;
        Bind *snap = bind_copy(b, nb);
        if (turns && e->sep) {
            if (!tok_is(p, e->sep)) break;
            adv(p);
        }
        if (!m_elems(p, r, e->sub, e->nsub, 0, 1, join, b, nb) || p->i == save) {
            p->i = save;
            memcpy(b, snap, sizeof *b * (size_t)nb);
            break;
        }
        turns++;
    }
    return !(e->rep == REP_PLUS && turns == 0);
}

static int m_elems(P *p, Rule *r, Elem *el, int nel, int tail,
                   int append, const char *join, Bind *b, int nb)
{
    for (int k = 0; k < nel; k++) {
        Elem *e = &el[k];

        if (e->kind == EL_WORD) {
            if (!tok_is(p, e->word)) return 0;
            adv(p);
            continue;
        }
        if (e->kind == EL_GROUP) {
            if (!m_group(p, r, e, b, nb)) return 0;
            continue;
        }

        char *v = NULL;
        int   lev = LEVEL_ATOM;
        switch (e->hk) {
        case K_CLASS: {
            Tok *t = cur(p);
            if (t->kind != T_CLASS || t->cls != e->cls) return 0;
            v = xstrndup(t->p, t->n);
            adv(p);
            break;
        }
        case K_EXPR: {
            int bp = 0;
            if (tail && k == nel - 1) {
                if (r->led) bp = r->right ? r->level - 1 : r->level;
                else if (r->level >= 0) bp = r->level;
            }
            {
                Out o = { LEVEL_ATOM, 0 };
                v = p_expr(p, bp, &o);
                if (!v) return 0;
                lev = o.level;
            }
            break;
        }
        case K_STMTS: {
            const char *term = k + 1 < nel ? el[k + 1].word : NULL;
            v = p_stmts(p, term);
            if (!v) return 0;
            break;
        }
        default:
            p->err = xfmt("%s:%d: a 'text' hole belongs to @mode text", r->file, r->line);
            return 0;
        }
        bind_put(b, nb, e->hole, v, append, join, lev);
    }
    return 1;
}

static char *p_rule(P *p, Rule *r, char *leftval, int leftlev)
{
    int   nb = 0;
    Bind *b  = xmalloc(sizeof *b * (size_t)(count_holes(r->el, r->nel) + 1));
    bind_pre(r->el, r->nel, b, &nb, 0);

    int k = 0;
    if (r->led) { bind_put(b, nb, r->el[0].hole, leftval, 0, NULL, leftlev); k = 1; }
    if (!m_elems(p, r, r->el + k, r->nel - k, 1, 0, NULL, b, nb)) return NULL;

    if (r->body) {
        char *err = NULL;
        char *out = code_eval(r, b, nb, &err);
        if (!out) { p->err = err; return NULL; }
        return out;
    }
    return subst(p, r, b, nb);
}

/* Candidates under one leading word, longest pattern first: that is what makes
   `if c then t else f` win over `if c then t`, and it is the whole of the
   dangling else -- the inner `if` takes the `else` because it is asked first. */
static int collect(P *p, Rule **out, int led, int minbp)
{
    int n = 0;
    for (int i = 0; i < p->g->nrule; i++) {
        Rule *r = &p->g->rule[i];
        if (r->led != led) continue;
        Elem *w = led ? &r->el[1] : &r->el[0];
        if (!tok_is(p, w->word)) continue;
        if (led && !(r->level > minbp)) continue;
        int j = n++;
        while (j > 0 && out[j - 1]->nel < r->nel) { out[j] = out[j - 1]; j--; }
        out[j] = r;
    }
    return n;
}

/* The level of what a rule produced, so that a code template can ask an operand
   whether it needs bracketing. A bare token, and a rule that declared no level,
   are atoms: nothing binds tighter than they do. */
static char *p_nud(P *p, Out *o)
{
    o->level = LEVEL_ATOM;
    o->terminated = 0;
    if (++p->depth > MAXDEPTH) {
        p->err = xstrdup("the grammar recurses without consuming anything");
        return NULL;
    }
    char *res = NULL;
    Tok  *t   = cur(p);
    if (t->kind != T_EOF) {
        Rule **c = xmalloc(sizeof *c * (size_t)(p->g->nrule + 1));
        int    n = collect(p, c, 0, 0);
        for (int i = 0; i < n && !res && !p->err; i++) {
            int save = p->i;
            res = p_rule(p, c[i], NULL, LEVEL_ATOM);
            if (!res) p->i = save;
            else {
                if (c[i]->level >= 0) o->level = c[i]->level;
                o->terminated = c[i]->terminated;
            }
        }
        if (!res && !p->err && t->kind == T_CLASS) {
            res = xstrndup(t->p, t->n);
            adv(p);
        }
    }
    p->depth--;
    return res;
}

static char *p_expr(P *p, int minbp, Out *o)
{
    Out   mine  = { LEVEL_ATOM, 0 };
    char *left  = p_nud(p, &mine);
    if (!left) return NULL;
    if (++p->depth > MAXDEPTH) {
        p->err = xstrdup("the grammar recurses without consuming anything");
        return NULL;
    }
    for (;;) {
        if (cur(p)->kind == T_EOF || p->err) break;
        Rule **c = xmalloc(sizeof *c * (size_t)(p->g->nrule + 1));
        int    n = collect(p, c, 1, minbp);
        char  *res = NULL;
        for (int i = 0; i < n && !res && !p->err; i++) {
            int save = p->i;
            res = p_rule(p, c[i], left, mine.level);
            if (!res) p->i = save;
            else { mine.level = c[i]->level; mine.terminated = c[i]->terminated; }
        }
        if (!res) break;
        left = res;
    }
    p->depth--;
    if (o) *o = mine;
    return left;
}

static char *p_stmts(P *p, const char *term)
{
    Grammar *g = p->g;
    Buf b = {0};
    int first = 1;

    if (!g->sep_in) {
        if (term && tok_is(p, term)) return xstrdup("");
        return p_expr(p, 0, NULL);
    }
    int prev_terminated = 0;
    for (;;) {
        while (tok_is(p, g->sep_in)) adv(p);
        if (cur(p)->kind == T_EOF) break;
        if (term && tok_is(p, term)) break;
        Out o = { LEVEL_ATOM, 0 };
        char *s = p_expr(p, 0, &o);
        if (!s) return NULL;
        /* The separator goes between two statements unless the one before said
           its own output already ends one. The input side has its own rule and
           they are deliberately not the same rule: one is about the language
           being read and the other about the language being written, and this
           tool is not entitled to assume they agree. */
        if (!first) buf_str(&b, prev_terminated ? "\n" : g->sep_out);
        buf_str(&b, s);
        first = 0;
        prev_terminated = o.terminated;
        if (tok_is(p, g->sep_in)) continue;
        /* A separator is wanted between two statements, and not after one that
           ended in a word. That is what lets `}` stand on its own, in C and in
           Pascal alike, without a rule having to declare itself terminating. */
        if (p->i > 0 && p->tk->t[p->i - 1].kind == T_PUNCT) continue;
        break;
    }
    if (!b.p) buf_str(&b, "");
    return b.p;
}

char *expand_expr(Grammar *g, Toks *tk, char **err)
{
    fresh_src = tk->src; fresh_g = g; fresh_n = 0;
    P p = { g, tk, 0, 0, 0, NULL };
    char *out = p_stmts(&p, NULL);
    if (out && cur(&p)->kind != T_EOF) out = NULL;
    if (!out) {
        if (p.err) { *err = p.err; return NULL; }
        Tok *t = &tk->t[p.far];
        if (t->kind == T_EOF)
            *err = xfmt("%s:%d: the file ends in the middle of something", tk->file, t->line);
        else
            *err = xfmt("%s:%d: no rule reads '%.*s' here",
                        tk->file, t->line, (int)t->n, t->p);
        return NULL;
    }
    return out;
}

/* ---------------------------------------------------------------- text mode */

static char *text_expand(Grammar *g, const char *s, size_t len, int depth, char **err);

static int text_comment(Grammar *g, const char *s, size_t len, size_t i,
                        size_t *start, size_t *end)
{
    for (int c = 0; c < g->ncom; c++) {
        size_t n = strlen(g->com[c].open);
        if (i + n > len || memcmp(s + i, g->com[c].open, n)) continue;
        size_t j = i + n;
        if (g->com[c].eol) {
            while (j < len && s[j] != '\n') j++;
            /* A comment that is the whole line takes the line with it. */
            size_t a = i;
            while (a > 0 && (s[a - 1] == ' ' || s[a - 1] == '\t')) a--;
            if (a == 0 || s[a - 1] == '\n') {
                if (j < len && s[j] == '\n') j++;
                *start = a;
            } else *start = i;
        } else {
            size_t m = strlen(g->com[c].close);
            while (j < len && (j + m > len || memcmp(s + j, g->com[c].close, m))) j++;
            if (j < len) j += m;
            *start = i;
        }
        *end = j;
        return 1;
    }
    return 0;
}

static char *text_rule(Grammar *g, Rule *r, const char *s, size_t len,
                       size_t i, size_t *end, int depth, char **err)
{
    Bind *b  = xmalloc(sizeof *b * (size_t)(r->nel + 1));
    int   nb = 0;
    size_t pos = i;
    memset(b, 0, sizeof *b * (size_t)(r->nel + 1));

    for (int k = 0; k < r->nel; k++) {
        Elem *e = &r->el[k];
        if (e->kind == EL_WORD) {
            size_t n = strlen(e->word);
            if (pos + n > len || memcmp(s + pos, e->word, n)) return NULL;
            pos += n;
            continue;
        }
        const char *term = k + 1 < r->nel && r->el[k + 1].kind == EL_WORD
                         ? r->el[k + 1].word : NULL;
        size_t stop;
        if (!term) stop = len;
        else {
            size_t tn = strlen(term), j = pos;
            while (j + tn <= len && memcmp(s + j, term, tn)) j++;
            if (j + tn > len) return NULL;
            stop = j;
        }
        char *v = text_expand(g, s + pos, stop - pos, depth + 1, err);
        if (!v) return NULL;
        b[nb].name  = e->hole;
        b[nb].val   = v;
        b[nb].set   = 1;
        b[nb].level = LEVEL_ATOM;
        nb++;
        pos = stop;
    }
    *end = pos;

    if (r->body) return code_eval(r, b, nb, err);

    P p = { g, NULL, 0, 0, 0, NULL };
    char *out = subst(&p, r, b, nb);
    if (!out) *err = p.err;
    return out;
}

static char *text_expand(Grammar *g, const char *s, size_t len, int depth, char **err)
{
    if (depth > 64) { *err = xstrdup("a text rule expands into itself"); return NULL; }
    Buf out = {0};
    for (size_t i = 0; i < len;) {
        size_t cs, ce;
        if (text_comment(g, s, len, i, &cs, &ce)) {
            size_t back = i - cs;              /* un-emit the line's indent */
            if (back > out.n) back = out.n;
            out.n -= back;
            if (out.p) out.p[out.n] = 0;
            i = ce;
            continue;
        }
        /* Longest leading word first, so `---` is tried before `-` and text
           mode munches the way the lexer does. Declaration order breaks a tie
           between two rules whose first word is the same length. */
        char *res = NULL;
        size_t end = i, best = 0;
        for (int k = 0; k < g->nrule; k++) {
            Rule *r = &g->rule[k];
            if (r->led || r->el[0].kind != EL_WORD) continue;
            size_t n = strlen(r->el[0].word);
            if (n > best && i + n <= len && !memcmp(s + i, r->el[0].word, n)) best = n;
        }
        for (size_t n = best; n > 0 && !res; n--)
            for (int k = 0; k < g->nrule && !res; k++) {
                Rule *r = &g->rule[k];
                if (r->led || r->el[0].kind != EL_WORD) continue;
                if (strlen(r->el[0].word) != n) continue;
                if (i + n > len || memcmp(s + i, r->el[0].word, n)) continue;
                res = text_rule(g, r, s, len, i, &end, depth, err);
                if (*err) return NULL;
            }
        if (res) { buf_str(&out, res); i = end; continue; }
        buf_ch(&out, s[i]);
        i++;
    }
    if (!out.p) buf_str(&out, "");
    return out.p;
}

static int has_group(Elem *el, int nel)
{
    for (int i = 0; i < nel; i++)
        if (el[i].kind == EL_GROUP ||
            (el[i].kind == EL_GROUP && has_group(el[i].sub, el[i].nsub))) return 1;
    return 0;
}

char *expand_text(Grammar *g, const char *src, size_t from,
                  const char *file, char **err)
{
    (void)file;
    for (int i = 0; i < g->nrule; i++)
        if (has_group(g->rule[i].el, g->rule[i].nel)) {
            *err = xfmt("%s:%d: a group belongs to @mode expression",
                        g->rule[i].file, g->rule[i].line);
            return NULL;
        }
    fresh_src = src; fresh_g = g; fresh_n = 0;
    return text_expand(g, src + from, strlen(src + from), 0, err);
}
