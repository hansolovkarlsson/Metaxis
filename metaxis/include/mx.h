/* mx.h -- Metaxis: a file declares its grammar, then is read with it.
 *
 * The one rule this implements: everything a directive says about foreign
 * text is inside a string, and everything outside a string is Metaxis's.
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
enum { EL_WORD, EL_HOLE, EL_GROUP };
enum { K_EXPR, K_CLASS, K_STMTS, K_TEXT, K_BLOCK };
enum { REP_ONE, REP_STAR, REP_PLUS };

typedef struct { char *name, *src; regex_t re; char *file; int line; } Class;
typedef struct { char *open, *close; int eol; } Comment;

typedef struct Elem Elem;
struct Elem {
    int   kind;      /* EL_WORD, EL_HOLE or EL_GROUP             */
    char *word;      /* EL_WORD: the literal text, as declared   */
    char *hole;      /* EL_HOLE: the hole's name                 */
    int   hk;        /* EL_HOLE: K_*                             */
    int   cls;       /* K_CLASS: index into Grammar.cls          */
    Elem *sub;       /* EL_GROUP: what is inside the brackets    */
    int   nsub;
    int   rep;       /* EL_GROUP: REP_*                          */
    char *sep;       /* EL_GROUP: the input separator, or NULL   */
    char *join;      /* EL_GROUP: the output joiner, or NULL     */
};

/* ------------------------------------------------- interpreted templates */

/* The second kind of template: `=> { … }` rather than `=> "…"`. It is
   Metaxis's own language, so it lives *outside* the strings, and the foreign
   text it emits lives inside them -- the same rule the whole notation rests on,
   read once more. */

enum { E_TEXT, E_INT, E_NAME, E_CALL, E_BIN, E_NOT };
enum { S_EMIT, S_IF, S_FOR, S_CALL };

typedef struct Expr Expr;
typedef struct Stmt Stmt;

struct Expr {
    int    kind;
    char  *s;         /* E_TEXT: the text. E_NAME/E_CALL: the name. E_BIN: op */
    long   n;
    Expr  *a, *b;
    Expr **args;
    int    nargs;
};

struct Stmt {
    int   kind;
    Expr *e;          /* S_EMIT value, S_IF condition, S_FOR list  */
    char *var;        /* S_FOR, the element                         */
    char *idx;        /* S_FOR, the position, or NULL               */
    Expr *sep;        /* S_FOR, the text between turns              */
    Stmt *body; int nbody;
    Stmt *alt;  int nalt;
};

/* A piece of template with a name. It is called as a *statement* and emits into
   whatever called it, which is why it needs no return: `emit` already writes to
   one place. Its body sees its parameters and its own loop variables and
   nothing else -- not the caller's holes -- so it can be checked where it is
   written rather than at each call. */
typedef struct {
    char  *name;
    char **param;
    int    nparam;
    Stmt  *body;
    int    nbody;
    char  *file;
    int    line;
} Tmpl;

/* A piece of *pattern* with a name -- the other half of naming a fragment, and
   deliberately not the same mechanic as a template. It is spliced where it is
   named, at declaration, so by the time any rule is matched its elements are
   indistinguishable from ones written out by hand: nothing downstream of the
   header knows a fragment was ever involved.

   That is why it brings its own holes with it and takes no arguments. A
   template is *called*, has a scope and can recurse; this is nearer to a macro
   over pattern text, and one directive covering both would be one word meaning
   two things. It is spliced with `@name`, which is a namespace of its own --
   not a hole's kind, because a kind says what one hole holds and a fragment
   says what sequence of elements goes here. */
typedef struct {
    char *name;
    Elem *el;
    int   nel;
    char *file;
    int   line;
} Frag;

typedef struct {
    Elem *el;
    int   nel;
    int   level;     /* -1 when the declaration gave none        */
    int   right;     /* right associative                        */
    int   led;       /* el[0] is a hole: an infix or postfix rule */
    char *tmpl;      /* a string template, or NULL               */
    Stmt *body;      /* a code template, or NULL                 */
    int   nbody;
    int   terminated;/* its output ends a statement on its own    */
    int   override;  /* it means to displace an earlier declaration */
    char *file;
    int   line;
} Rule;

/* What a hole came out holding. A hole inside a repeated group holds every turn
   as a list; the string template sees them joined, the code template sees the
   list, and that is the one thing `join` used to throw away. */
typedef struct {
    const char *name;
    char       *val;
    char      **items;
    int         nitems;
    int         set;     /* its group matched at least once   */
    int         level;   /* the level of what filled it, or -1 */
    int         terminated; /* what filled it already ends a statement */
    int         islist;  /* it sits inside a repeated group   */
} Bind;

#define LEVEL_ATOM 1000

typedef struct {
    Class   *cls;   int ncls;
    Comment *com;   int ncom;
    char   **punct; int npunct;   /* every rule word, longest first */
    Rule    *rule;  int nrule;
    Tmpl    *tmpl;  int ntmpl;
    Frag    *frag;  int nfrag;
    char *sep_in, *sep_out;
    int   sep_nl;                 /* the separator is a newline     */
    int   sep_indent;             /* and indentation opens a block  */
    char *sep_file; int sep_line; /* where it was declared          */
    int   mode;
    int   nfiles;                 /* @use depth guard               */
    char **seen; int nseen;       /* @use reads a file once         */
} Grammar;

Grammar *grammar_new(void);
/* Builds the punctuation set, then checks what only a finished header can
   check. Returns 0, or -1 with *err set. */
int      grammar_seal(Grammar *g, char **err);
int      class_index(Grammar *g, const char *name);
int      frag_index(Grammar *g, const char *name);

/* Reads directives from src; *body is set to the offset the body starts at.
   Returns 0, or -1 with *err set. */
int header_read(Grammar *g, const char *src, const char *file,
                size_t *body, char **err);

/* ------------------------------------------------------------------ tokens */

/* T_INDENT and T_DEDENT are the two tokens no file spells. Every other token
   is text somebody wrote; these two are the shape of the whitespace around it,
   and they exist only under `@separator "\n" indent`. They carry no text, so
   `tok_is` can never match one and a quoted word can never name one -- which is
   the whole reason `block` is a kind and not a pair of strings. */
enum { T_EOF, T_PUNCT, T_CLASS, T_INDENT, T_DEDENT };

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
char *pt_fresh(const char *label);

/* code.c */
Stmt *code_parse(const char *s, size_t *i, size_t end, const char *file,
                 int *nout, char **err);
int   code_check(Rule *r, char **err);
/* Resolves every template call in every rule and template, once the header has
   finished and they are all in. Returns 0, or -1 with *err set. */
int   code_check_calls(Grammar *g, char **err);
char *code_eval(Grammar *g, Rule *r, Bind *b, int nb, char **err);
int   rule_has_hole(Rule *r, const char *name);
int   code_mentions(Rule *r, const char *n);
char *expand_text(Grammar *g, const char *src, size_t from,
                  const char *file, char **err);

/* ------------------------------------------------------------------- files */

char *read_file(const char *path, char **err);
int   line_at(const char *src, size_t off);

#endif
