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
#include "mx.h"

#include <stdio.h>

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
    /* The index of the last token a rule consumed *as a word*, so that
       p_stmts can tell a statement that ended in `end` from one that ended in
       a name that happens to be spelled `end`. The class wins a tie, so `end`
       is a name token whose text is a word, and the token's kind cannot say
       which of the two a rule made of it. Restored wherever the cursor is. */
    int      wordat;
} P;

#define MAXDEPTH 400

/* ------------------------------------------------------- `-t`: the trace ---
 *
 * `mx -g` prints the grammar a header declared and nothing prints the parse it
 * attempted, which is the half that is hard to reason about from outside:
 * candidates under one leading word are tried longest-first with the token
 * cursor restored on failure, so a rule that never fires looks exactly like a
 * rule that was never reached.
 *
 * It goes to **stderr**, so `mx -t f.mx > out` still writes the expansion and
 * nothing else. And it counts, because the roadmap item it closed -- now
 * docs/COMPLETED.md's "mx -t, and the quadratic it found" -- did not want a
 * feature. It wanted a measurement before a backtracking budget is picked, on
 * the ground that a budget chosen without one is a number somebody made up. */
static int  trace_on;
static long trace_tries, trace_restored;
static int  trace_deepest;

void expand_trace(int on) { trace_on = on; trace_tries = trace_restored = 0; trace_deepest = 0; }

static void pat_show(FILE *f, Elem *el, int nel)
{
    for (int e = 0; e < nel; e++) {
        switch (el[e].kind) {
        case EL_WORD: fprintf(f, "\"%s\" ", el[e].word); break;
        case EL_HOLE: fprintf(f, "%s ", el[e].hole);       break;
        default:
            fprintf(f, "[ ");
            pat_show(f, el[e].sub, el[e].nsub);
            fprintf(f, "]%s", el[e].rep == REP_STAR ? "* " :
                              el[e].rep == REP_PLUS ? "+ " : " ");
        }
    }
}

/* One line per candidate, indented by how deep the parse is, and one more when
   it fails saying which token it could not get past -- which is the thing a
   grammar under construction is nearly always wrong about. */
void expand_summary(void)
{
    fprintf(stderr, "trace: %ld candidate%s tried, %ld restored, deepest %d\n",
            trace_tries, trace_tries == 1 ? "" : "s", trace_restored, trace_deepest);
}

static void trace_before(P *p, Rule *r, int i, int n)
{
    trace_tries++;
    if (p->depth > trace_deepest) trace_deepest = p->depth;
    if (!trace_on) return;
    fprintf(stderr, "%*s try %d/%d  ", p->depth * 2, "", i + 1, n);
    pat_show(stderr, r->el, r->nel);
    fprintf(stderr, " [%s:%d]\n", r->file, r->line);
}

static void trace_after(P *p, Rule *r, int save, char *res)
{
    if (!res) trace_restored++;
    if (!trace_on) return;
    if (res) {
        fprintf(stderr, "%*s  ok\n", p->depth * 2, "");
        return;
    }
    Tok *t = &p->tk->t[p->i < p->tk->n ? p->i : p->tk->n - 1];
    fprintf(stderr, "%*s  no, stopped at '%.*s' (line %d), %d token%s back\n",
            p->depth * 2, "", (int)t->n, t->p, t->line,
            p->i - save, p->i - save == 1 ? "" : "s");
}

/* What a parsed expression turns out to be, besides its text: the level of the
   rule that produced it, so a code template can ask whether an operand needs
   bracketing, and whether that rule said its output ends a statement. */
typedef struct { int level, terminated; } Out;

static Tok *cur(P *p) { return &p->tk->t[p->i]; }

static int tok_is(P *p, const char *w)
{
    Tok *t = cur(p);
    /* Only text can be quoted. T_INDENT and T_DEDENT carry none, so no word a
       file writes can name one -- which is what makes `block` a kind. */
    if (t->kind != T_PUNCT && t->kind != T_CLASS) return 0;
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
static char *p_stmts(P *p, const char *term, int dedent, int *terminated);

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
/* `level` and `terminated` are the two things a hole remembers about the rule
   that filled it: how tightly it bound, and whether its output already ends a
   statement. A code template asks the first with `level(h)` and the second with
   `terminated(h)`; both are the rule's own property, not the text's, which is
   why neither can be worked out from `val` afterwards. */
static void bind_put(Bind *b, int nb, const char *name, char *val,
                     int append, const char *join, int level, int terminated)
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
        b[i].set        = 1;
        b[i].level      = level;
        b[i].terminated = terminated;
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
        int   savew = p->wordat;
        if (!m_elems(p, r, e->sub, e->nsub, 0, 0, NULL, b, nb)) {
            p->i = save; p->wordat = savew;
            memcpy(b, snap, sizeof *b * (size_t)nb);
        }
        return 1;
    }

    int turns = 0;
    for (;;) {
        int   save = p->i, savew = p->wordat;
        Bind *snap = bind_copy(b, nb);
        if (turns && e->sep) {
            if (!tok_is(p, e->sep)) break;
            p->wordat = p->i;
            adv(p);
        }
        if (!m_elems(p, r, e->sub, e->nsub, 0, 1, join, b, nb) || p->i == save) {
            p->i = save; p->wordat = savew;
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
            p->wordat = p->i;
            adv(p);
            continue;
        }
        if (e->kind == EL_GROUP) {
            if (!m_group(p, r, e, b, nb)) return 0;
            continue;
        }

        char *v = NULL;
        int   lev = LEVEL_ATOM, term = 0;
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
                lev  = o.level;
                term = o.terminated;
            }
            break;
        }
        case K_STMTS: {
            /* A run of statements is terminated when its *last* one was: that
               is the statement a word after the hole would follow. */
            const char *stop = k + 1 < nel ? el[k + 1].word : NULL;
            v = p_stmts(p, stop, 0, &term);
            if (!v) return 0;
            break;
        }
        case K_BLOCK: {
            /* The hole owns both delimiters, because nothing else can name
               them. Like a `stmts` hole it answers for its last statement --
               the one the `}` a template writes would close over. */
            if (cur(p)->kind != T_INDENT) return 0;
            adv(p);
            v = p_stmts(p, NULL, 1, &term);
            if (!v) return 0;
            if (cur(p)->kind != T_DEDENT) return 0;
            adv(p);
            break;
        }
        default:
            p->err = xfmt("%s:%d: a 'text' hole belongs to @mode text", r->file, r->line);
            return 0;
        }
        bind_put(b, nb, e->hole, v, append, join, lev, term);
    }
    return 1;
}

static char *p_rule(P *p, Rule *r, char *leftval, int leftlev, int leftterm)
{
    int   nb = 0;
    Bind *b  = xmalloc(sizeof *b * (size_t)(count_holes(r->el, r->nel) + 1));
    bind_pre(r->el, r->nel, b, &nb, 0);

    int k = 0;
    if (r->led) { bind_put(b, nb, r->el[0].hole, leftval, 0, NULL, leftlev, leftterm); k = 1; }
    if (!m_elems(p, r, r->el + k, r->nel - k, 1, 0, NULL, b, nb)) return NULL;

    if (r->body) {
        char *err = NULL;
        char *out = code_eval(p->g, r, b, nb, &err);
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
            int save = p->i, savew = p->wordat;
            trace_before(p, c[i], i, n);
            res = p_rule(p, c[i], NULL, LEVEL_ATOM, 0);
            trace_after(p, c[i], save, res);
            if (!res) { p->i = save; p->wordat = savew; }
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
            int save = p->i, savew = p->wordat;
            trace_before(p, c[i], i, n);
            res = p_rule(p, c[i], left, mine.level, mine.terminated);
            trace_after(p, c[i], save, res);
            if (!res) { p->i = save; p->wordat = savew; }
            else { mine.level = c[i]->level; mine.terminated = c[i]->terminated; }
        }
        if (!res) break;
        left = res;
    }
    p->depth--;
    if (o) *o = mine;
    return left;
}

static char *p_stmts(P *p, const char *term, int dedent, int *terminated)
{
    Grammar *g = p->g;
    Buf b = {0};
    int first = 1;

    if (terminated) *terminated = 0;
    if (!g->sep_in) {
        if (term && tok_is(p, term)) return xstrdup("");
        Out one = { LEVEL_ATOM, 0 };
        char *only = p_expr(p, 0, &one);
        if (only && terminated) *terminated = one.terminated;
        return only;
    }
    int prev_terminated = 0;
    for (;;) {
        while (tok_is(p, g->sep_in)) adv(p);
        if (cur(p)->kind == T_EOF) break;
        if (term && tok_is(p, term)) break;
        if (dedent && cur(p)->kind == T_DEDENT) break;
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
        if (terminated) *terminated = o.terminated;
        if (tok_is(p, g->sep_in)) continue;
        /* A separator is wanted between two statements, and not after one that
           ended in a word. That is what lets `}` stand on its own, in C and in
           Pascal alike, without a rule having to declare itself terminating.
           Until 2026-09-06 this asked whether the last token was punctuation,
           which `}` is and `end` is not -- the class wins a tie, so `end` is a
           name token -- and every example wrote `end;`, so nothing noticed
           until a tutorial file wrote `end` on a line of its own. */
        if (p->i > 0 && p->wordat == p->i - 1) continue;
        /* A block that just closed is the same case as a `}` or an `end`: the
           statement before it ended in something that was not a statement, and
           what follows starts a new one without a separator saying so. */
        if (p->i > 0 && p->tk->t[p->i - 1].kind == T_DEDENT) continue;
        break;
    }
    if (!b.p) buf_str(&b, "");
    return b.p;
}

char *expand_expr(Grammar *g, Toks *tk, char **err)
{
    fresh_src = tk->src; fresh_g = g; fresh_n = 0;
    P p = { g, tk, 0, 0, 0, NULL, -1 };
    char *out = p_stmts(&p, NULL, 0, NULL);
    if (out && cur(&p)->kind != T_EOF) out = NULL;
    if (!out) {
        if (p.err) { *err = p.err; return NULL; }
        Tok *t = &tk->t[p.far];
        if (t->kind == T_EOF)
            *err = xfmt("%s:%d: the file ends in the middle of something", tk->file, t->line);
        else if (t->kind == T_INDENT)
            *err = xfmt("%s:%d: this line is indented and no rule opened a block here",
                        tk->file, t->line);
        /* Reachable only if a block hole ever stops consuming the dedent that
           closes it, which is the one invariant holding these two tokens in
           pairs. It is here so that breaking that invariant says a sentence
           rather than quoting the empty text a synthetic token carries. */
        else if (t->kind == T_DEDENT)
            *err = xfmt("%s:%d: this line closes a block no rule here is inside",
                        tk->file, t->line);
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

/* Text mode's one lexer step. Where a declared class matches, the scan moves
   by the token and not by the character: a rule's word may begin only where a
   token begins and end only where one ends, a hole's candidate stops are the
   same places, and a token no rule fired on is copied through whole. That is
   how `err` is not found inside `stderr`, and how a string or a comment that
   the file declared as a class is passed over rather than searched. A file
   that declares no class gets the scan it always had, one character at a time.

   The window is one for the run: text mode nests, a hole's text being expanded
   in its turn, and the copy grows to the longest token, not to the file. */
static Win text_win;

static size_t text_tok(Grammar *g, const char *s, size_t len, size_t pos)
{
    int which;
    if (!g->ncls || pos >= len) return 0;
    return class_at(g, &text_win, s + pos, len - pos, &which);
}

/* The next place the scan may stop after `pos`. */
static size_t text_step(Grammar *g, const char *s, size_t len, size_t pos)
{
    size_t n = text_tok(g, s, len, pos);
    return pos + (n ? n : 1);
}

/* Whether `w` stands at `pos` and ends where a token ends. `pos` is always a
   boundary, since everything here moves by text_step, so only the far end is
   asked: a word that would stop inside a token, `err` at `errno`, does not
   match. */
static int text_word(Grammar *g, const char *s, size_t len, size_t pos, const char *w)
{
    size_t n = strlen(w);
    if (pos + n > len || memcmp(s + pos, w, n)) return 0;
    size_t j = pos;
    while (j < pos + n) j = text_step(g, s, len, j);
    return j == pos + n;
}

/* The first boundary at or after `pos` where `w` stands, or `len` for nowhere. */
static size_t find_word(Grammar *g, const char *s, size_t len, size_t pos, const char *w)
{
    for (size_t j = pos; j < len; j = text_step(g, s, len, j))
        if (text_word(g, s, len, j, w)) return j;
    return len;
}

/* Matching a text-mode pattern is a search, not a scan.
 *
 * It was a scan until groups arrived: walk the elements once, and find a hole's
 * end by looking for the next literal word. A group makes that impossible --
 * an optional part may or may not be there, so what really follows a hole is
 * not known until the rest of the pattern has been tried. So the matcher takes
 * an alternative, tries the whole remainder, and puts the cursor and every
 * binding back if it fails.
 *
 * `Cont` is what is left to match after the current array runs out. When it
 * carries a group, that array was one turn of a repetition and the choice at
 * the end of it is another turn or moving on.
 */
typedef struct Cont {
    Elem  *el; int nel; int k;   /* continue here                       */
    Elem  *grp;                  /* the turn just finished, if any      */
    int    turns;
    struct Cont *up;
} Cont;

typedef struct {
    Grammar *g;
    Rule    *r;
    const char *s;
    size_t   len;
    Bind    *b;
    int      nb;
    int      depth, steps;
    size_t   end;
    char    *closer;
    char   **err;
} TM;

static int tm_match(TM *t, Elem *el, int nel, int k, size_t pos, Cont *cont,
                    int append, const char *join);

/* The word that closes the rule: the last literal one in its pattern, groups
   looked into. **A hole may not span it.** Without that, `"[[" t "|" u "]]"`
   given `[[here]] and a bar|pipe` lets `t` walk past the `]]` it should have
   stopped at and take the `|` from three words later, which is the defect
   POSTMORTEM.md 4 records.

   The first version of this bound was *every* word still to come, which is
   stricter and was wrong: in `"![" alt "](" src [ " " title ] ")"` the group's
   space is a later word, and `alt` is allowed to contain spaces. The closer is
   the one word whose arrival means this construct has ended. */
static char *closing_word(Elem *el, int nel)
{
    char *last = NULL;
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_WORD) last = el[i].word;
        else if (el[i].kind == EL_GROUP) {
            char *w = closing_word(el[i].sub, el[i].nsub);
            if (w) last = w;
        }
    }
    return last;
}

static Bind *tm_save(TM *t)
{
    Bind *c = xmalloc(sizeof *c * (size_t)(t->nb + 1));
    memcpy(c, t->b, sizeof *c * (size_t)t->nb);
    return c;
}

static void tm_load(TM *t, Bind *c)
{
    memcpy(t->b, c, sizeof *c * (size_t)t->nb);
}

static int tm_done(TM *t, size_t pos, Cont *cont, int append, const char *join)
{
    if (!cont) { t->end = pos; return 1; }

    if (cont->grp) {
        Elem *g = cont->grp;
        size_t p2 = pos;
        int    ok = 1;
        if (g->sep) {
            size_t n = strlen(g->sep);
            if (pos + n <= t->len && !memcmp(t->s + pos, g->sep, n)) p2 = pos + n;
            else ok = 0;
        }
        if (ok) {
            Cont c2 = { cont->el, cont->nel, cont->k, g, cont->turns + 1, cont->up };
            Bind *snap = tm_save(t);
            if (tm_match(t, g->sub, g->nsub, 0, p2, &c2, 1, g->join)) return 1;
            tm_load(t, snap);
        }
    }
    return tm_match(t, cont->el, cont->nel, cont->k, pos, cont->up, append, join);
}

static int tm_match(TM *t, Elem *el, int nel, int k, size_t pos, Cont *cont,
                    int append, const char *join)
{
    if (--t->steps < 0) {
        if (!*t->err) *t->err = xfmt("%s:%d: this rule has too many ways to match",
                                     t->r->file, t->r->line);
        return 0;
    }
    if (k == nel) return tm_done(t, pos, cont, append, join);

    Elem *e = &el[k];

    if (e->kind == EL_WORD) {
        if (!text_word(t->g, t->s, t->len, pos, e->word)) return 0;
        return tm_match(t, el, nel, k + 1, pos + strlen(e->word), cont, append, join);
    }

    if (e->kind == EL_GROUP) {
        Bind *snap = tm_save(t);
        Cont  c    = { el, nel, k + 1, e->rep == REP_ONE ? NULL : e, 1, cont };
        if (tm_match(t, e->sub, e->nsub, 0, pos, &c,
                     e->rep == REP_ONE ? append : 1,
                     e->rep == REP_ONE ? join : e->join)) return 1;
        tm_load(t, snap);
        if (e->rep == REP_PLUS) return 0;
        return tm_match(t, el, nel, k + 1, pos, cont, append, join);
    }

    /* A class hole: one token of that class at the cursor, taken exactly, with
       no search. It has to be the token the scan would take, the longest match
       of any class, so that `x:int` does not take the `3` from `3.14` when a
       class that reads the whole number was declared. Spliced as its source
       text, as in expression mode, and not expanded. */
    if (e->hk == K_CLASS) {
        if (pos >= t->len) return 0;
        size_t n = class_match(&text_win, &t->g->cls[e->cls].re, t->s + pos, t->len - pos);
        if (!n || n != text_tok(t->g, t->s, t->len, pos)) return 0;
        Bind *snap = tm_save(t);
        bind_put(t->b, t->nb, e->hole, xstrndup(t->s + pos, n), append, join, LEVEL_ATOM, 0);
        if (tm_match(t, el, nel, k + 1, pos + n, cont, append, join)) return 1;
        tm_load(t, snap);
        return 0;
    }

    /* A text hole. Shortest first, stopping only where a token ends, and never
       past the word that closes the rule. */
    size_t cap = t->closer ? find_word(t->g, t->s, t->len, pos, t->closer) : t->len;
    for (size_t stop = pos;; stop = text_step(t->g, t->s, t->len, stop)) {
        Bind *snap = tm_save(t);
        char *v = text_expand(t->g, t->s + pos, stop - pos, t->depth + 1, t->err);
        if (!v) return 0;
        bind_put(t->b, t->nb, e->hole, v, append, join, LEVEL_ATOM, 0);
        if (tm_match(t, el, nel, k + 1, stop, cont, append, join)) return 1;
        tm_load(t, snap);
        if (stop >= cap) break;
    }
    return 0;
}

static char *text_rule(Grammar *g, Rule *r, const char *s, size_t len,
                       size_t i, size_t *end, int depth, char **err)
{
    int   nb = 0;
    Bind *b  = xmalloc(sizeof *b * (size_t)(count_holes(r->el, r->nel) + 1));
    bind_pre(r->el, r->nel, b, &nb, 0);

    TM t;
    memset(&t, 0, sizeof t);
    t.g = g; t.r = r; t.s = s; t.len = len;
    t.b = b; t.nb = nb; t.depth = depth; t.steps = 200000; t.err = err;
    t.closer = closing_word(r->el, r->nel);

    if (!tm_match(&t, r->el, r->nel, 0, i, NULL, 0, NULL)) return NULL;
    *end = t.end;

    if (r->body) return code_eval(g, r, b, nb, err);

    P p = { g, NULL, 0, 0, 0, NULL, -1 };
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
            if (n > best && text_word(g, s, len, i, r->el[0].word)) best = n;
        }
        for (size_t n = best; n > 0 && !res; n--)
            for (int k = 0; k < g->nrule && !res; k++) {
                Rule *r = &g->rule[k];
                if (r->led || r->el[0].kind != EL_WORD) continue;
                if (strlen(r->el[0].word) != n) continue;
                if (!text_word(g, s, len, i, r->el[0].word)) continue;
                res = text_rule(g, r, s, len, i, &end, depth, err);
                if (*err) return NULL;
            }
        if (res) { buf_str(&out, res); i = end; continue; }
        /* Nothing fired: the token, or the character, goes through whole. */
        size_t n = text_tok(g, s, len, i);
        if (!n) n = 1;
        buf_add(&out, s + i, n);
        i += n;
    }
    if (!out.p) buf_str(&out, "");
    return out.p;
}

char *expand_text(Grammar *g, const char *src, size_t from,
                  const char *file, char **err)
{
    (void)file;
    fresh_src = src; fresh_g = g; fresh_n = 0;
    return text_expand(g, src + from, strlen(src + from), 0, err);
}
