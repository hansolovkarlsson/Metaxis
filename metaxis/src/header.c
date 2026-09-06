/* header.c -- the fixed half.
 *
 * Every string below is a Metaxis string, spelled Metaxis's way, in every
 * file, whatever the file declares. That is what stops a directive from ever
 * being read as the thing it is declaring.
 */
#include "mx.h"

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

int frag_index(Grammar *g, const char *name)
{
    for (int i = 0; i < g->nfrag; i++)
        if (!strcmp(g->frag[i].name, name)) return i;
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

/* Nothing may follow a directive but its own words. `@syntax` has always said
   so; the two that now end in an optional bare word have to, because `dtake`
   matches a prefix and `overridden` would otherwise be read as `override` with
   three characters silently dropped after it. */
static int dend(D *d, const char *what)
{
    dskip(d);
    if (d->i >= d->end) return 0;
    derr(d, xfmt("trailing text after %s", what));
    return -1;
}

static int ident_ch(int c) { return isalnum((unsigned char)c) || c == '_'; }

/* A Metaxis string: "..." with \" \\ \n \t \r and nothing else. */
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

static const char *KINDS[]  = { "expr", "stmts", "text", "block", NULL };
static const int   KINDKS[] = { K_EXPR,  K_STMTS, K_TEXT, K_BLOCK };

/* A pattern element, and a group of them. `[ … ]` is Metaxis's vocabulary and
   lives outside the strings, so it can never be mistaken for the body's own
   brackets -- those would be quoted. */
static Elem *parse_elems(Grammar *g, D *d, Rule *r, int *nout, int depth);

/* Splicing copies. A fragment may be spliced into many rules and each one goes
   on to be sealed -- `seal_elems` walks a rule's own elements -- so sharing them
   would make one declaration's words reachable through several rules, which is
   exactly the aliasing the rest of this file avoids. The strings are copied too:
   they are what the punctuation set ends up holding. */
static Elem *elems_copy(const Elem *src, int n)
{
    Elem *out = xmalloc(sizeof *out * (size_t)(n + 1));
    for (int i = 0; i < n; i++) {
        out[i] = src[i];
        if (src[i].word) out[i].word = xstrdup(src[i].word);
        if (src[i].hole) out[i].hole = xstrdup(src[i].hole);
        if (src[i].sep)  out[i].sep  = xstrdup(src[i].sep);
        if (src[i].join) out[i].join = xstrdup(src[i].join);
        if (src[i].sub)  out[i].sub  = elems_copy(src[i].sub, src[i].nsub);
    }
    return out;
}

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
        } else if (d->s[d->i] == '@') {
            /* A splice. `@` can be nothing else here -- a pattern element is a
               quoted word, a hole, a group or a level -- so this reserves no
               name and no file written before today changes meaning. The
               fragment's elements are copied in where it is named, and from the
               next line on nothing can tell them from elements written out by
               hand: the rule is checked, sealed, matched and clashed as one
               pattern.

               It must already be declared, which is what `@token` asks of a
               class used as a kind, and it buys the same thing twice over: a
               fragment cannot splice itself, so no cycle is expressible, and
               there is no order in which this file could have meant something
               else. */
            d->i++;
            char *fn = dident(d);
            if (!fn) { derr(d, "expected a fragment's name after '@'"); return NULL; }
            int fi = frag_index(g, fn);
            if (fi < 0) { derr(d, xfmt("no fragment called '@%s'", fn)); return NULL; }
            Frag *f  = &g->frag[fi];
            Elem *cp = elems_copy(f->el, f->nel);
            for (int k = 0; k < f->nel; k++) {
                el = grow(el, nel, sizeof *el);
                el[nel++] = cp[k];
            }
            continue;
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
                    if (!strcmp(k, KINDS[i])) { e.hk = KINDKS[i]; found = 1; }
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

/* Two holes with one name.
 *
 * A template splices a hole *by name* and `bind_put` fills the first it finds,
 * so a pattern declaring one name twice fills one and drops the other --
 * silently, which is the shape of defect this project keeps meeting. It was
 * always a mistake and nobody had made it, so nothing refused it. Splicing
 * makes it easy to make by accident: one fragment named twice in one rule is
 * two of every hole it declares. So it is refused where the pattern is checked,
 * and the message names the hole rather than the fragment, because a rule that
 * wrote the collision out by hand deserves the same answer. */
static const char *hole_dup(const Elem *el, int nel, char ***seen, int *n)
{
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_GROUP) {
            const char *bad = hole_dup(el[i].sub, el[i].nsub, seen, n);
            if (bad) return bad;
            continue;
        }
        if (el[i].kind != EL_HOLE) continue;
        for (int j = 0; j < *n; j++)
            if (!strcmp((*seen)[j], el[i].hole)) return el[i].hole;
        *seen = grow(*seen, *n, sizeof **seen);
        (*seen)[(*n)++] = el[i].hole;
    }
    return NULL;
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
    r.el = el; r.nel = nel;
    r.led = el[0].kind == EL_HOLE;

    /* One `=>` per target. A file with one target writes one, untagged, and
       nothing about such a file has changed -- one emit with no tag is what a
       rule has always had. `as <tag>` is what lets a grammar be written once
       and read out more than one way, and its customer was measured before it
       existed: examples/code.mx duplicated 272 lines of pattern from
       examples/pascal.mx to change nothing but what came after the arrow.

       `terminated` belongs to the emit and not to the rule, because it is a
       statement about the *output* -- one target may brace a branch where
       another does not, which is exactly the difference those two files were
       written to show. */
    Emit *em = NULL; int nem = 0;
    if (!dtake(d, "=>")) { derr(d, "expected '=>'"); return -1; }
    do {
        Emit e; memset(&e, 0, sizeof e);
        e.line = line;
        dskip(d);
        /* A template has always been a string, and a string never starts with
           a brace, so one character says which of the two forms this is. */
        if (d->i < d->end && d->s[d->i] == '{') {
            char *cerr = NULL;
            e.body = code_parse(d->s, &d->i, d->end, d->file, &e.nbody, &cerr);
            if (!e.body) { if (!d->err) d->err = cerr; return -1; }
        } else {
            e.tmpl = dstring(d);
            if (!e.tmpl) return -1;
        }
        /* Either order, neither twice -- as `terminated` and `override` have
           always been. `as` is a keyword only here, where a bare word cannot
           be a hole, so a hole may still be called `as`. */
        for (int again = 1; again;) {
            again = 0;
            if (!e.terminated && dtake(d, "terminated")) { e.terminated = 1; again = 1; }
            if (!e.tag && dtake(d, "as")) {
                e.tag = dident(d);
                if (!e.tag) { derr(d, "expected a name after 'as'"); return -1; }
                again = 1;
            }
        }
        for (int i = 0; i < nem; i++) {
            int same = (!em[i].tag && !e.tag) ||
                       (em[i].tag && e.tag && !strcmp(em[i].tag, e.tag));
            if (!same) continue;
            if (e.tag) derr(d, xfmt("this rule already emits '%s'", e.tag));
            else       derr(d, "this rule already has an untagged template"
                               " -- write 'as <name>' on all but one");
            return -1;
        }
        em = grow(em, nem, sizeof *em);
        em[nem++] = e;
    } while (dtake(d, "=>"));

    /* `override` is the rule's and not any one template's: it displaces an
       earlier declaration of this pattern, whatever either declares. */
    if (dtake(d, "override")) r.override = 1;
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

    char **seen = NULL;
    int    nseen = 0;
    const char *dup = hole_dup(el, nel, &seen, &nseen);
    if (dup) {
        derr(d, xfmt("two holes called '%s': a template splices a hole by name,"
                     " so only one of them could ever be reached", dup));
        return -1;
    }

    /* Every template is checked here rather than at the first use of the rule,
       so that a splice nobody wrote a hole for is an error at the line that
       wrote it -- and every one of them, not only the one a later `-b` happens
       to select. A backend that is never asked for is still checked. */
    for (int k = 0; k < nem; k++) {
        r.tmpl = em[k].tmpl; r.body = em[k].body; r.nbody = em[k].nbody;
        if (r.body) {
            char *cerr = NULL;
            if (code_check(&r, &cerr) < 0) { if (!d->err) d->err = cerr; return -1; }
            continue;
        }
        char *tmpl = r.tmpl;
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
    }

    /* Every tag this file has seen, so that `mx -b` can refuse one nothing
       declares rather than silently expanding the defaults. */
    for (int k = 0; k < nem; k++) {
        if (!em[k].tag) continue;
        int have = 0;
        for (int i = 0; i < g->nbackend; i++)
            if (!strcmp(g->backend[i], em[k].tag)) { have = 1; break; }
        if (have) continue;
        g->backend = grow(g->backend, g->nbackend, sizeof *g->backend);
        g->backend[g->nbackend++] = xstrdup(em[k].tag);
    }

    r.emit = em; r.nemit = nem;
    r.tmpl = em[0].tmpl; r.body = em[0].body; r.nbody = em[0].nbody;
    r.terminated = em[0].terminated;

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
        int mode;
        if (!strcmp(m, "expression")) mode = MODE_EXPR;
        else if (!strcmp(m, "text"))  mode = MODE_TEXT;
        else { derr(d, "expected 'expression' or 'text'"); goto fail; }
        /* `override` after the mode, and trailing text refused, for the same
           reasons `@separator` does both. `@mode` was the last global doing
           neither: a second one replaced the first in silence, and anything
           after the word was ignored -- so `@mode expression override` was
           accepted today and meant nothing.

           It takes `override` rather than being flatly refused because two
           *used* files are the case that cannot be written around. A file with
           no body can still declare the mode its rules need -- a set of
           text-mode rules is only usable in text mode -- and a file that uses
           two such libraries has to be able to say which it meant. That is the
           same problem `override` was built for one directive over, and giving
           `@mode` a second mechanic of its own would be a new concept for no
           gain. A second `@mode` in one file is still always a mistake; it is
           just a mistake the word makes you write down. */
        int over = dtake(d, "override");
        if (dend(d, "@mode") < 0) goto fail;
        if (g->mode_file && !over) {
            derr(d, xfmt("the mode is already declared at %s:%d"
                         " -- write 'override' to mean it",
                         g->mode_file, g->mode_line));
            goto fail;
        }
        if (!g->mode_file && over) {
            derr(d, "'override', but no mode was declared before it");
            goto fail;
        }
        free(g->mode_file);
        g->mode_file = xstrdup(file);
        g->mode_line = line;
        g->mode = mode;
        return 0;
    }
    if (!strcmp(name, "token")) {
        char *n = dident(d);
        if (!n) { derr(d, "expected a class name"); goto fail; }
        /* A class named after a kind could never be used. `x:expr` is resolved
           as the *kind* and a class of that name is never consulted, so the
           rule parsed and ran and read something other than what it said --
           `@token text` reached `a 'text' hole belongs to @mode text`, which is
           an error about the wrong thing, and `@token expr` said nothing at all.
           Refusing the name is the whole fix, because the two namespaces meet
           only here: `@fragment` is spliced with `@name` and shares neither. */
        for (int i = 0; KINDS[i]; i++) {
            if (strcmp(n, KINDS[i])) continue;
            derr(d, xfmt("'%s' is a kind, so a class called that could never be"
                         " used -- 'x:%s' is read as the kind and this class"
                         " would never be consulted", n, n));
            goto fail;
        }
        char *re = dstring(d);
        if (!re) goto fail;
        int over = dtake(d, "override");
        if (dend(d, "@token") < 0) goto fail;
        Class c;
        c.name = n;
        c.src  = re;
        c.file = xstrdup(file);
        c.line = line;
        char *anchored = xfmt("^(%s)", re);
        int rc = regcomp(&c.re, anchored, REG_EXTENDED);
        if (rc) {
            char msg[256];
            regerror(rc, &c.re, msg, sizeof msg);
            derr(d, xfmt("bad pattern for '%s': %s", n, msg));
            goto fail;
        }
        int old = class_index(g, n);
        if (old >= 0) {
            if (!over) {
                derr(d, xfmt("the class '%s' is already declared at %s:%d"
                             " -- write 'override' to mean it",
                             n, g->cls[old].file, g->cls[old].line));
                goto fail;
            }
            g->cls[old] = c;
            return 0;
        }
        if (over) {
            derr(d, xfmt("'override', but no class '%s' was declared before it", n));
            goto fail;
        }
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
        /* `indent` is read before `override` because it belongs to what is
           being declared and `override` belongs to the declaring. */
        int ind  = dtake(d, "indent");
        int over = dtake(d, "override");
        if (dend(d, "@separator") < 0) goto fail;
        if (ind && !strchr(in, '\n')) {
            derr(d, "'indent' needs a separator with a newline in it:"
                    " indentation is what a line break leads to");
            goto fail;
        }
        if (g->sep_in && !over) {
            derr(d, xfmt("the separator is already declared at %s:%d"
                         " -- write 'override' to mean it",
                         g->sep_file, g->sep_line));
            goto fail;
        }
        if (!g->sep_in && over) {
            derr(d, "'override', but no separator was declared before it");
            goto fail;
        }
        g->sep_file = xstrdup(file);
        g->sep_line = line;
        g->sep_in  = in;
        g->sep_out = out ? out : xstrdup(in);
        g->sep_nl  = strchr(in, '\n') != NULL;
        g->sep_indent = ind;
        return 0;
    }
    if (!strcmp(name, "template")) {
        char *n = dident(d);
        if (!n) { derr(d, "expected a name after '@template'"); goto fail; }
        if (!dtake(d, "(")) { derr(d, "expected '(' after a template's name"); goto fail; }
        Tmpl t;
        memset(&t, 0, sizeof t);
        t.name = n;
        t.file = xstrdup(file);
        t.line = line;
        if (!dat(d, ")")) {
            for (;;) {
                char *pn = dident(d);
                if (!pn) { derr(d, "expected a parameter name"); goto fail; }
                if (t.nparam >= 8) {
                    derr(d, xfmt("a template takes at most 8 parameters, and '%s'"
                                 " was given more", n));
                    goto fail;
                }
                t.param = grow(t.param, t.nparam, sizeof *t.param);
                t.param[t.nparam++] = pn;
                if (!dtake(d, ",")) break;
            }
        }
        if (!dtake(d, ")")) { derr(d, "expected ')'"); goto fail; }
        dskip(d);
        if (d->i >= d->end || d->s[d->i] != '{') {
            derr(d, "a template's body is a block: '@template name(x) { … }'");
            goto fail;
        }
        char *cerr = NULL;
        t.body = code_parse(d->s, &d->i, d->end, d->file, &t.nbody, &cerr);
        if (!t.body) { if (!d->err) d->err = cerr; goto fail; }
        int over = dtake(d, "override");
        if (dend(d, "@template") < 0) goto fail;
        for (int i = 0; i < g->ntmpl; i++) {
            if (strcmp(g->tmpl[i].name, n)) continue;
            if (!over) {
                derr(d, xfmt("the template '%s' is already declared at %s:%d"
                             " -- write 'override' to mean it",
                             n, g->tmpl[i].file, g->tmpl[i].line));
                goto fail;
            }
            g->tmpl[i] = t;
            return 0;
        }
        if (over) {
            derr(d, xfmt("'override', but no template '%s' was declared before it", n));
            goto fail;
        }
        g->tmpl = grow(g->tmpl, g->ntmpl, sizeof *g->tmpl);
        g->tmpl[g->ntmpl++] = t;
        return 0;
    }
    if (!strcmp(name, "fragment")) {
        char *n = dident(d);
        if (!n) { derr(d, "expected a name after '@fragment'"); goto fail; }
        /* `override` sits before the `=`, which is the opposite of everywhere
           else and is forced. A rule puts it after the template because that is
           the one place a bare word cannot be anything else; a fragment's
           pattern runs to the end of the directive, so there is no *after* --
           a trailing `override` would be read as a hole called `override`,
           which is precisely the silent misreading the word exists to prevent.
           Before the `=` it is a modifier on the declaration, which is what it
           has always been. */
        int over = dtake(d, "override");
        if (!dtake(d, "=")) { derr(d, "expected '=' after a fragment's name"); goto fail; }

        Rule scratch;
        memset(&scratch, 0, sizeof scratch);
        scratch.level = -1;

        Frag f;
        memset(&f, 0, sizeof f);
        f.name = n;
        f.file = xstrdup(file);
        f.line = line;
        f.el   = parse_elems(g, d, &scratch, &f.nel, 0);
        if (!f.el) goto fail;
        if (!f.nel) { derr(d, "a fragment needs something in it"); goto fail; }
        if (scratch.level >= 0) {
            derr(d, "a level belongs to a rule and not to a fragment: it says how"
                    " tightly one rule binds, and a fragment is spliced into any"
                    " number of them");
            goto fail;
        }
        if (dend(d, "@fragment") < 0) goto fail;

        /* A fragment's pattern is not checked here. It is checked at every
           splice, as part of the rule it lands in, because every check there is
           about a *whole* pattern: what stops a greedy hole is the element after
           it, and a fragment does not know what will follow it. */
        int old = frag_index(g, n);
        if (old >= 0) {
            if (!over) {
                derr(d, xfmt("the fragment '%s' is already declared at %s:%d"
                             " -- write 'override' to mean it",
                             n, g->frag[old].file, g->frag[old].line));
                goto fail;
            }
            /* Rules already spliced from the earlier declaration keep what they
               copied. That is what splicing at declaration means, and it is the
               same answer `@token override` gives: what was read is read. */
            g->frag[old] = f;
            return 0;
        }
        if (over) {
            derr(d, xfmt("'override', but no fragment '%s' was declared before it", n));
            goto fail;
        }
        g->frag = grow(g->frag, g->nfrag, sizeof *g->frag);
        g->frag[g->nfrag++] = f;
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

/* Skips whitespace, `;` comments -- which are Metaxis's own and always
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

/* A file is read once, however many times it is reached.
 *
 * Without this a diamond -- `a` and `b` both using `base`, and one file using
 * both -- declares everything in `base` twice, which under the rule below is a
 * file colliding with itself over declarations nobody wrote twice. Proto reads
 * once for exactly this reason. Identity is the resolved path, so two spellings
 * of one file are one file; a path that cannot be resolved is used as written,
 * which at worst reads it a second time and cannot read the wrong thing.
 *
 * A cycle now ends rather than erroring: the second visit finds the file
 * already read and returns. The depth guard stays for genuinely deep nesting. */
static int seen_before(Grammar *g, const char *full)
{
    char *real = realpath(full, NULL);
    const char *key = real ? real : full;
    for (int i = 0; i < g->nseen; i++)
        if (!strcmp(g->seen[i], key)) { free(real); return 1; }
    g->seen = grow(g->seen, g->nseen, sizeof *g->seen);
    g->seen[g->nseen++] = real ? real : xstrdup(full);
    return 0;
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

    if (seen_before(g, full)) { g->nfiles--; return 0; }

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

/* A block-kind hole is the one kind text mode cannot honour.
 *
 * `expr` and `stmts` both mean *read up to the word that stops you*, which is
 * what a text-mode hole does anyway, so those degrade honestly. A class means
 * *one token, matching this regex*, and since 2026-09-06 text mode consults
 * the classes a file declares, so that is honoured too (text_tok in expand.c).
 * Before that it was refused here, because `"[" x:name "]"` took everything up
 * to the `]` and the kind was never consulted: it did not fail, it read as if
 * it had worked, which is the shape of defect this project keeps meeting. A
 * block is still that: its delimiters are two tokens the lexer makes out of
 * whitespace, and the scan measures no indentation.
 *
 * The check is here rather than in the rule that declared it because `@mode`
 * is a directive like any other: a rule may be written before the mode is, or
 * in a file that `@use` pulled in and that names no mode at all. By the time
 * the header has finished speaking, the mode is settled and every rule is in. */
static int seal_check(Grammar *g, Rule *r, Elem *el, int nel, char **err)
{
    (void)g;
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_GROUP) {
            if (seal_check(g, r, el[i].sub, el[i].nsub, err) < 0) return -1;
            continue;
        }
        if (el[i].kind != EL_HOLE || el[i].hk != K_BLOCK) continue;
        *err = xfmt("%s:%d: '%s:block' asks for an indented run of statements,"
                    " and text mode has no tokens to measure the indentation of",
                    r->file, r->line, el[i].hole);
        return -1;
    }
    return 0;
}

/* A `block` hole is the one hole whose delimiters no file spells, so the thing
   that makes them -- a separator that nests -- has to have been declared. The
   check is here for `seal_check`'s reason, one directive over: a rule may be
   written above the `@separator` it depends on, or in a file that `@use` pulled
   in and that declares no separator at all. */
static int seal_block(Grammar *g, Rule *r, Elem *el, int nel, char **err)
{
    for (int i = 0; i < nel; i++) {
        if (el[i].kind == EL_GROUP) {
            if (seal_block(g, r, el[i].sub, el[i].nsub, err) < 0) return -1;
            continue;
        }
        if (el[i].kind != EL_HOLE || el[i].hk != K_BLOCK) continue;
        if (g->sep_indent) continue;
        *err = xfmt("%s:%d: '%s:block' wants a block, and nothing here opens one"
                    " -- write '@separator \"\\n\" indent'",
                    r->file, r->line, el[i].hole);
        return -1;
    }
    return 0;
}

/* Two patterns are the same when nothing about matching tells them apart.
   Hole *names* are not part of it: `a "+" b` and `x "+" y` match the same text,
   so the second is as unreachable as a verbatim copy would be. Levels are not
   part of it either -- two rules for one pattern at two levels is a grammar
   that cannot say which it means, and saying `override` is how it says. */
static int same_pattern(Elem *a, int na, Elem *b, int nb)
{
    if (na != nb) return 0;
    for (int i = 0; i < na; i++) {
        if (a[i].kind != b[i].kind) return 0;
        switch (a[i].kind) {
        case EL_WORD:
            if (strcmp(a[i].word, b[i].word)) return 0;
            break;
        case EL_HOLE:
            if (a[i].hk != b[i].hk || a[i].cls != b[i].cls) return 0;
            break;
        default:
            if (a[i].rep != b[i].rep) return 0;
            if (!!a[i].sep != !!b[i].sep) return 0;
            if (a[i].sep && strcmp(a[i].sep, b[i].sep)) return 0;
            if (!!a[i].join != !!b[i].join) return 0;
            if (a[i].join && strcmp(a[i].join, b[i].join)) return 0;
            if (!same_pattern(a[i].sub, a[i].nsub, b[i].sub, b[i].nsub)) return 0;
        }
    }
    return 1;
}

/* Two files declaring one thing.
 *
 * A rule is found by its pattern, so two rules with the same pattern are one
 * rule declared twice and the second could never fire -- candidates are tried
 * longest-first with declaration order breaking a tie, so it is the *earlier*
 * that wins here and the later that is dead. Silently, which is the part worth
 * fixing: the file that wrote the second template gets the first one's output
 * and nothing says so.
 *
 * The answer is that a file says which it meant. Unmarked, it is refused and
 * both lines are named. Marked `override`, the later displaces the earlier and
 * nothing is said, because it was said in the source. And `override` with
 * nothing to displace is refused too, so the word cannot quietly become noise
 * when the declaration it was written against moves away. */
static int rule_clash(Grammar *g, char **err)
{
    int *dead = xmalloc(sizeof *dead * (size_t)(g->nrule + 1));
    memset(dead, 0, sizeof *dead * (size_t)(g->nrule + 1));

    for (int i = 0; i < g->nrule; i++) {
        Rule *r = &g->rule[i];
        int   prev = -1;
        for (int j = 0; j < i; j++)
            if (!dead[j] && same_pattern(g->rule[j].el, g->rule[j].nel, r->el, r->nel))
                prev = j;

        if (prev < 0) {
            if (!r->override) continue;
            *err = xfmt("%s:%d: 'override', but nothing with this pattern was"
                        " declared before it", r->file, r->line);
            return -1;
        }
        if (!r->override) {
            *err = xfmt("%s:%d: this pattern is already declared at %s:%d"
                        " -- write 'override' after the template to mean it",
                        r->file, r->line, g->rule[prev].file, g->rule[prev].line);
            return -1;
        }
        dead[prev] = 1;
    }

    int n = 0;
    for (int i = 0; i < g->nrule; i++)
        if (!dead[i]) g->rule[n++] = g->rule[i];
    g->nrule = n;
    return 0;
}

/* Which template each rule uses. Runs once, between the header and anything
   that reads a rule, so that everything downstream sees a rule with exactly
   one template and never learns that `as` exists.

   A rule with no emit for the wanted tag falls back to its untagged one. That
   is what makes a second backend cheap to add: only the rules that differ need
   an `as` clause, and a grammar where nine rules in ten are the same for both
   targets says so by staying silent. A rule with neither is an error naming the
   rule, because the alternative is expanding it to nothing. */
int grammar_select(Grammar *g, const char *want, char **err)
{
    char *list = xstrdup("");
    for (int i = 0; i < g->nbackend; i++)
        list = xfmt("%s%s%s", list, i ? ", " : "", g->backend[i]);

    if (want) {
        int have = 0;
        for (int i = 0; i < g->nbackend; i++)
            if (!strcmp(g->backend[i], want)) { have = 1; break; }
        if (!have) {
            *err = g->nbackend
                 ? xfmt("no rule emits '%s' -- this file declares %s", want, list)
                 : xfmt("no rule emits '%s' -- this file declares no 'as' at all", want);
            return -1;
        }
    }
    for (int i = 0; i < g->nrule; i++) {
        Rule *r = &g->rule[i];
        Emit *pick = NULL, *dflt = NULL;
        for (int k = 0; k < r->nemit; k++) {
            if (!r->emit[k].tag) dflt = &r->emit[k];
            else if (want && !strcmp(r->emit[k].tag, want)) pick = &r->emit[k];
        }
        if (!pick) pick = dflt;
        if (!pick) {
            /* Falling back to the first declared template would let position
               decide the output, which is the one question this tool has
               always declined to answer by position -- see `override`. So it
               says which templates there are and asks. */
            *err = want
                 ? xfmt("%s:%d: this rule emits nothing for '%s', and has no"
                        " untagged template to fall back to", r->file, r->line, want)
                 : xfmt("%s:%d: every template here is tagged, so there is no"
                        " default -- name one with '-b <name>'. This file"
                        " emits: %s", r->file, r->line, list);
            return -1;
        }
        r->tmpl = pick->tmpl;
        r->body = pick->body;
        r->nbody = pick->nbody;
        r->terminated = pick->terminated;
    }
    return 0;
}

int grammar_seal(Grammar *g, char **err)
{
    if (rule_clash(g, err) < 0) return -1;
    /* Template calls resolve here rather than where they are written, so a rule
       may call a template declared after it or in a file it `@use`d, and the
       order a header is written in cannot change the answer. */
    if (code_check_calls(g, err) < 0) return -1;

    for (int r = 0; r < g->nrule; r++)
        seal_elems(g, g->rule[r].el, g->rule[r].nel);
    if (g->sep_in && !g->sep_nl) seal_word(g, g->sep_in);
    qsort(g->punct, (size_t)g->npunct, sizeof *g->punct, cmp_len);

    if (g->mode == MODE_TEXT)
        for (int r = 0; r < g->nrule; r++) {
            /* Text mode fires nud rules only: a led rule wants a left operand
               already parsed, and a scan has none. Until 2026-09-06 such a
               rule was accepted and simply never fired -- a rule that reads as
               if it worked, which is the shape of defect seal_check exists to
               refuse. The island rehearsal in the journal is what found it. */
            if (g->rule[r].led) {
                *err = xfmt("%s:%d: a rule that begins with a hole is infix, and"
                            " text mode has nothing for it to continue -- it could"
                            " never fire", g->rule[r].file, g->rule[r].line);
                return -1;
            }
            if (seal_check(g, &g->rule[r], g->rule[r].el, g->rule[r].nel, err) < 0)
                return -1;
        }
    for (int r = 0; r < g->nrule; r++)
        if (seal_block(g, &g->rule[r], g->rule[r].el, g->rule[r].nel, err) < 0)
            return -1;
    return 0;
}
