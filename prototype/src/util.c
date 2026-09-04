#include "pt.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One-shot tool: it allocates and does not free. The alternative is an arena
   and a lifetime argument on every function, bought with nothing. */

void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) { fputs("pt: out of memory\n", stderr); exit(2); }
    return p;
}

char *xstrndup(const char *s, size_t n)
{
    char *p = xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

char *xstrdup(const char *s) { return xstrndup(s, strlen(s)); }

char *xfmt(const char *fmt, ...)
{
    va_list a, b;
    va_start(a, fmt);
    va_copy(b, a);
    int n = vsnprintf(NULL, 0, fmt, a);
    va_end(a);
    char *p = xmalloc((size_t)n + 1);
    vsnprintf(p, (size_t)n + 1, fmt, b);
    va_end(b);
    return p;
}

void buf_add(Buf *b, const char *s, size_t n)
{
    if (b->n + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap : 64;
        while (c < b->n + n + 1) c *= 2;
        char *p = xmalloc(c);
        if (b->p) memcpy(p, b->p, b->n);
        b->p = p;
        b->cap = c;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}

void  buf_str(Buf *b, const char *s) { buf_add(b, s, strlen(s)); }
void  buf_ch (Buf *b, char c)        { buf_add(b, &c, 1); }

char *buf_take(Buf *b)
{
    if (!b->p) return xstrdup("");
    return b->p;
}

char *read_file(const char *path, char **err)
{
    FILE *f = fopen(path, "rb");
    if (!f) { *err = xfmt("cannot open %s", path); return NULL; }
    Buf b = {0};
    char chunk[8192];
    size_t n;
    while ((n = fread(chunk, 1, sizeof chunk, f)) > 0) buf_add(&b, chunk, n);
    fclose(f);
    if (!b.p) buf_str(&b, "");
    return b.p;
}

int line_at(const char *src, size_t off)
{
    int line = 1;
    for (size_t i = 0; i < off; i++) if (src[i] == '\n') line++;
    return line;
}
