/* pt.h -- Prototype: a file declares its grammar, then is read with it.
 *
 * The one rule this implements: everything a directive says about foreign
 * text is inside a string, and everything outside a string is Prototype's.
 * So the header has a fixed grammar that no file can reach, and the body has
 * no grammar at all until the header has finished speaking.
 */
#ifndef PT_H
#define PT_H

#include <stddef.h>
#include <regex.h>

/* ------------------------------------------------------------------ memory */

void *xmalloc(size_t n);
char *xstrndup(const char *s, size_t n);
char *xstrdup(const char *s);
char *xfmt(const char *fmt, ...);

typedef struct { char *p; size_t n, cap; } Buf;

void  buf_add(Buf *b, const char *s, size_t n);
void  buf_str(Buf *b, const char *s);
void  buf_ch(Buf *b, char c);
char *buf_take(Buf *b);

/* ----------------------------------------------------------------- grammar */

enum { MODE_EXPR, MODE_TEXT };
enum { EL_WORD, EL_HOLE };
enum { K_EXPR, K_CLASS, K_STMTS, K_TEXT };

typedef struct { char *name, *src; regex_t re; } Class;
typedef struct { char *open, *close; int eol; } Comment;

typedef struct {
    int   kind;      /* EL_WORD or EL_HOLE                       */
    char *word;      /* EL_WORD: the literal text, as declared   */
    char *hole;      /* EL_HOLE: the hole's name                 */
    int   hk;        /* EL_HOLE: K_*                             */
    int   cls;       /* K_CLASS: index into Grammar.cls          */
} Elem;

typedef struct {
    Elem *el;
    int   nel;
    int   level;     /* -1 when the declaration gave none        */
    int   right;     /* right associative                        */
    int   led;       /* el[0] is a hole: an infix or postfix rule */
    char *tmpl;
    char *file;
    int   line;
} Rule;

typedef struct {
    Class   *cls;   int ncls;
    Comment *com;   int ncom;
    char   **punct; int npunct;   /* every rule word, longest first */
    Rule    *rule;  int nrule;
    char *sep_in, *sep_out;
    int   sep_nl;                 /* the separator is a newline     */
    int   mode;
    int   nfiles;                 /* @use guard                     */
} Grammar;

Grammar *grammar_new(void);
void     grammar_seal(Grammar *g);          /* build the punctuation set */
int      class_index(Grammar *g, const char *name);

/* Reads directives from src; *body is set to the offset the body starts at.
   Returns 0, or -1 with *err set. */
int header_read(Grammar *g, const char *src, const char *file,
                size_t *body, char **err);

/* ------------------------------------------------------------------ tokens */

enum { T_EOF, T_PUNCT, T_CLASS };

typedef struct {
    int         kind, cls;
    const char *p;
    size_t      n, off;
    int         line;
} Tok;

typedef struct {
    Tok        *t;
    int         n;
    const char *src, *file;
} Toks;

int lex(Grammar *g, const char *src, size_t from, const char *file,
        Toks *out, char **err);

/* --------------------------------------------------------------- expansion */

char *expand_expr(Grammar *g, Toks *tk, char **err);
char *expand_text(Grammar *g, const char *src, size_t from,
                  const char *file, char **err);

/* ------------------------------------------------------------------- files */

char *read_file(const char *path, char **err);
int   line_at(const char *src, size_t off);

#endif
