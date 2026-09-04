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
#include "pt.h"

#include <string.h>

static void push(Toks *tk, Tok t)
{
    Tok *p = xmalloc(sizeof *p * (size_t)(tk->n + 1));
    if (tk->t) memcpy(p, tk->t, sizeof *p * (size_t)tk->n);
    p[tk->n] = t;
    tk->t = p;
    tk->n++;
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

int lex(Grammar *g, const char *src, size_t from, const char *file,
        Toks *out, char **err)
{
    memset(out, 0, sizeof *out);
    out->src  = src;
    out->file = file;

    size_t i = from;
    int    pending_nl = 0;

    for (;;) {
        for (;;) {
            if (src[i] == ' ' || src[i] == '\t' || src[i] == '\r') { i++; continue; }
            if (src[i] == '\n') { pending_nl = 1; i++; continue; }
            size_t after;
            if (comment_at(g, src, i, &after)) { i = after; continue; }
            break;
        }

        if (g->sep_nl && pending_nl && out->n && src[i]) {
            Tok t = { T_PUNCT, -1, "\n", 1, i, line_at(src, i) };
            push(out, t);
        }
        pending_nl = 0;

        if (!src[i]) break;

        size_t best_cls = 0;
        int    which    = -1;
        for (int c = 0; c < g->ncls; c++) {
            regmatch_t m[2];
            if (regexec(&g->cls[c].re, src + i, 2, m, 0)) continue;
            size_t len = (size_t)m[0].rm_eo;
            if (len > best_cls) { best_cls = len; which = c; }
        }

        size_t best_pun = 0;
        for (int w = 0; w < g->npunct; w++) {
            size_t n = strlen(g->punct[w]);
            if (n > best_pun && !memcmp(src + i, g->punct[w], n)) best_pun = n;
        }

        Tok t;
        t.off  = i;
        t.line = line_at(src, i);
        t.p    = src + i;
        if (best_cls && best_cls >= best_pun) {
            t.kind = T_CLASS; t.cls = which; t.n = best_cls;
        } else if (best_pun) {
            t.kind = T_PUNCT; t.cls = -1;    t.n = best_pun;
        } else {
            *err = xfmt("%s:%d: nothing here is anything this file declared: '%.12s'",
                        file, line_at(src, i), src + i);
            return -1;
        }
        push(out, t);
        i += t.n;
    }

    Tok eof = { T_EOF, -1, src + i, 0, i, line_at(src, i) };
    push(out, eof);
    return 0;
}
