/* lex.c -- the lexer the header wrote.
 *
 * Proto's operator characters are a closed set, because its lexer runs before
 * it knows what the file declared. Here the header is read first and this runs
 * second, so the token set is the file's own.
 *
 * At each position: the longest match from a declared token class, and the
 * longest match from a declared word. **The class wins a tie**, which is the
 * whole of "an alphabetic word is not reserved" -- `div` is a name token whose
 * text happens to be `div`, and a rule that wants the word compares text.
 * `divisor` is a longer class match than the word and never splits.
 */
#include "mx.h"

#include <string.h>

/* Doubling, not one-more-each-time. The old shape allocated an array of n+1
   and copied all n tokens **for every token**, which is quadratic and was the
   whole of it: 4985 lines of Pascal took 67 seconds, and about 12 million Tok
   copies of that were this function. Everything else in the file was a
   rounding error, including the line counting that looked guilty first.

   The roadmap had asked for a measurement before picking a backtracking
   budget; that item is docs/COMPLETED.md's "mx -t, and the quadratic it
   found" now. The measurement found the parser was never the problem -- the
   heaviest example restores 24 candidates -- and that the lexer's token array
   was. See docs/POSTMORTEM.md. */
static void push(Toks *tk, Tok t)
{
    if (tk->n == tk->cap) {
        int cap = tk->cap ? tk->cap * 2 : 64;
        Tok *p  = xmalloc(sizeof *p * (size_t)cap);
        if (tk->t) memcpy(p, tk->t, sizeof *p * (size_t)tk->n);
        tk->t = p;
        tk->cap = cap;
    }
    tk->t[tk->n++] = t;
}

/* Comments are looked for before words, so a file whose comment opener is also
   an operator gets the comment. It is the only precedence the lexer has that
   the file did not set, and it is stated here because it is not derivable. */
static int comment_at(Grammar *g, const char *s, size_t i, size_t *out)
{
    for (int c = 0; c < g->ncom; c++) {
        size_t n = strlen(g->com[c].open);
        if (memcmp(s + i, g->com[c].open, n)) continue;
        i += n;
        if (g->com[c].eol) while (s[i] && s[i] != '\n') i++;
        else {
            size_t m = strlen(g->com[c].close);
            while (s[i] && memcmp(s + i, g->com[c].close, m)) i++;
            if (s[i]) i += m;
        }
        *out = i;
        return 1;
    }
    return 0;
}

/* The indent stack, and the two tokens no file spells.
 *
 * A column is counted only while nothing but whitespace has been seen since the
 * last newline, which is what makes a **blank or comment-only line produce no
 * dedent**: every newline resets the count, so the indentation that is measured
 * is always that of the line carrying the next real token. A tab advances to
 * the next multiple of 8 -- a number this file picks, because nobody can derive
 * it, and stated here rather than left to be discovered.
 *
 * An indent is emitted *instead of* the separator: a line break that leads to a
 * deeper line is that indent and does not also separate two statements. A
 * dedent is emitted *after* one, because the statement it closes over did end. */
#define TABSTOP 8

typedef struct { int *v; int n, cap; } Stack;

static void col_push(Stack *s, int c)
{
    if (s->n == s->cap) {
        int cap = s->cap ? s->cap * 2 : 16;
        int *v  = xmalloc(sizeof *v * (size_t)cap);
        if (s->v) memcpy(v, s->v, sizeof *v * (size_t)s->n);
        s->v = v; s->cap = cap;
    }
    s->v[s->n++] = c;
}

/* line_at() counts newlines from the start of the source, which is right and
   cheap when a header asks it a handful of times. The lexer asks it once per
   token, and rescanning the whole file per token is quadratic.

   **This was the first guess at why lexing was slow, and it was wrong** -- the
   file got no faster when it was fixed. It is kept because it is right, and
   because it would have become the bottleneck once the real one was gone.
 
   The lexer walks the source forward and never goes back, so it can carry the
   count with it. `lc_line` advances a cursor to the offset asked for and is
   O(1) amortised; if it is ever asked for an earlier offset it falls back to
   line_at and is merely correct. Nothing else changes -- every token gets the
   same line number it got before. */
typedef struct { const char *src; size_t at; int line; } LC;

static int lc_line(LC *c, size_t off)
{
    if (off < c->at) return line_at(c->src, off);
    while (c->at < off) if (c->src[c->at++] == '\n') c->line++;
    return c->line;
}

/* regexec() takes a NUL-terminated string and the C library measures it, so
   handing it `src + i` costs O(rest of file) on every call. The match itself is
   anchored and short; the measuring is not. Three classes per token is what
   made lexing quadratic, and the proof is a file with the *same tokens* and
   200KB of trailing comment: 4.1s became 32.6s.

   Every class pattern is compiled anchored -- `^(...)`, see `@token` in
   header.c -- so a match can only begin where we are looking, and matching
   against a bounded copy is enough. The window doubles only when a match
   actually reaches its edge, which is bounded by the length of the token being
   matched and not by the file.

   One honest consequence: a pattern that anchors its *end* with `$` now sees
   the window's end rather than the file's. No `@token` here writes one, and a
   class is by definition a token rather than a line, so `$` in one was already
   asking for something the lexer does not offer. */
typedef struct { char *p; size_t cap; } Win;

static int match_here(Win *w, const regex_t *re, const char *s,
                      size_t avail, size_t *len)
{
    for (size_t n = 128;;) {
        if (n > avail) n = avail;
        if (n + 1 > w->cap) {
            size_t c = w->cap ? w->cap : 256;
            while (c < n + 1) c *= 2;
            w->p = xmalloc(c);
            w->cap = c;
        }
        memcpy(w->p, s, n);
        w->p[n] = 0;
        regmatch_t m[2];
        if (regexec(re, w->p, 2, m, 0)) return 0;
        size_t got = (size_t)m[0].rm_eo;
        if (got < n || n == avail) { *len = got; return 1; }
        n *= 2;   /* the match ran to the edge, so it may run past it */
    }
}

static void mark(Toks *out, LC *lc, const char *src, size_t i, int kind)
{
    Tok t = { kind, -1, src + i, 0, i, lc_line(lc, i) };
    push(out, t);
}

int lex(Grammar *g, const char *src, size_t from, const char *file,
        Toks *out, char **err)
{
    LC lc = { src, 0, 1 };
    Win win = { NULL, 0 };
    size_t srclen = strlen(src);
    memset(out, 0, sizeof *out);
    out->src  = src;
    out->file = file;

    size_t i = from;
    int    pending_nl = 0;
    int    col = 0, only_ws = 1;
    Stack  st = { NULL, 0, 0 };
    if (g->sep_indent) col_push(&st, 0);

    for (;;) {
        for (;;) {
            if (src[i] == ' ')  { if (only_ws) col++; i++; continue; }
            if (src[i] == '\t') { if (only_ws) col += TABSTOP - col % TABSTOP; i++; continue; }
            if (src[i] == '\r') { i++; continue; }
            if (src[i] == '\n') { pending_nl = 1; col = 0; only_ws = 1; i++; continue; }
            size_t after;
            /* A comment ends the run of whitespace this line opened with, so
               what was counted before it is the line's indentation and nothing
               after it adds to that. An end-of-line comment then meets its own
               newline and the count starts over, which is why a comment-only
               line is invisible here. */
            if (comment_at(g, src, i, &after)) { i = after; only_ws = 0; continue; }
            break;
        }

        if (g->sep_indent && pending_nl && src[i]) {
            if (col > st.v[st.n - 1]) {
                mark(out, &lc, src, i, T_INDENT);
                col_push(&st, col);
                pending_nl = 0;
            } else if (col < st.v[st.n - 1]) {
                if (out->n) { Tok t = { T_PUNCT, -1, "\n", 1, i, lc_line(&lc, i) }; push(out, t); }
                while (col < st.v[st.n - 1]) { st.n--; mark(out, &lc, src, i, T_DEDENT); }
                if (col != st.v[st.n - 1]) {
                    *err = xfmt("%s:%d: this line ends a block but lines up with"
                                " nothing that opened one", file, lc_line(&lc, i));
                    return -1;
                }
                pending_nl = 0;
            }
        }

        if (g->sep_nl && pending_nl && out->n && src[i]) {
            Tok t = { T_PUNCT, -1, "\n", 1, i, lc_line(&lc, i) };
            push(out, t);
        }
        pending_nl = 0;

        if (!src[i]) {
            /* Whatever is still open closes at the end of the file. */
            while (st.n > 1) { st.n--; mark(out, &lc, src, i, T_DEDENT); }
            break;
        }

        size_t best_cls = 0;
        int    which    = -1;
        for (int c = 0; c < g->ncls; c++) {
            size_t len;
            if (!match_here(&win, &g->cls[c].re, src + i, srclen - i, &len)) continue;
            if (len > best_cls) { best_cls = len; which = c; }
        }

        size_t best_pun = 0;
        for (int w = 0; w < g->npunct; w++) {
            size_t n = strlen(g->punct[w]);
            if (n > best_pun && !memcmp(src + i, g->punct[w], n)) best_pun = n;
        }

        Tok t;
        t.off  = i;
        t.line = lc_line(&lc, i);
        t.p    = src + i;
        if (best_cls && best_cls >= best_pun) {
            t.kind = T_CLASS; t.cls = which; t.n = best_cls;
        } else if (best_pun) {
            t.kind = T_PUNCT; t.cls = -1;    t.n = best_pun;
        } else {
            *err = xfmt("%s:%d: nothing here is anything this file declared: '%.12s'",
                        file, lc_line(&lc, i), src + i);
            return -1;
        }
        push(out, t);
        i += t.n;
        only_ws = 0;
    }

    Tok eof = { T_EOF, -1, src + i, 0, i, lc_line(&lc, i) };
    push(out, eof);
    return 0;
}
