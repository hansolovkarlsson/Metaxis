# Prototype

**Working name.** A language-agnostic rewriter in the shape of Proto: a file
declares its own grammar in its header and is then read with it. `.pt` in,
`.out` out.

Its one premise is that **every mention of foreign text inside a directive is a
string** — quoted words on the pattern side, a quoted template with `{hole}`
splices on the output side.

```
@syntax "if" c "then" t  =>  "if ({c}) {{ {t} }}"
        ~~~~     ~~~~~~      ~~~~~~~~~~~~~~~~~~~
        mentioned, quoted    emitted, quoted
```

## Why

Proto's grammar and the grammars a Proto file declares are drawn from one pool
of characters. `;` cannot separate statements because it opens a comment; `|`
cannot be an operator because it delimits a block's parameters; `.` `,` and `:`
are spoken for; a pattern's holes are written `<x>` with two characters that are
themselves operator characters; `42` cannot be an integer because the lexer said
so first. Each is a reasonable local decision and together they are a budget
that runs out.

Putting the mention in quotes spends nothing, because a string's boundaries are
the one thing every reader already agrees on. What it buys, and what it costs,
is [docs/notation.md](docs/notation.md).

## Build

```
make            # bin/pt
make check      # every example against the .out beside it, then tests/errors.sh
```

C11 and `make`, plus POSIX `<regex.h>` for `@token`.

## The examples

Each is run by `make check` against the `.out` recorded beside it.

| | |
| --- | --- |
| [tour.pt](examples/tour.pt) | the idea in one file: infix, prefix, circumfix, mixfix, and `then` used as a word and as a name four lines apart |
| [clike.pt](examples/clike.pt) | the six things Proto's `lib/clike.pro` lists as impossible — `;` between statements, `x++`, `a[i]`, `p->f`, a lone `|`, `for`, and `42` without a sigil |
| [pascal.pt](examples/pascal.pt) | Pascal in, C out; neither language the tool's. `pascal.out` keeps its parenthesis noise, which is the cost of agnosticism showing itself |
| [poem.pt](examples/poem.pt) | `@mode text`: prose in, HTML out |
| [reserved.pt](examples/reserved.pt) | every character Prototype writes a directive with — `@`, `=>`, `.`, `:`, `<`, `>`, `"`, `{`, `}` — declared as an operator by a directive |
| [use.pt](examples/use.pt) | `@use`, taking its arithmetic from [lib/arith.pt](lib/arith.pt) and keeping its own comment and separator |

The headline is `for`, which puts two `;` inside a pattern in a file whose body
ends statements with `;`:

```
@syntax "for" "(" init ";" c ";" step ")" "{" b:stmts "}"
    => "{init}.\n{{ {c} }}:whileTrue({{ {b}. {step} }})"
```

There is nothing to disambiguate because there was never an ambiguity.

## Layout

```
include/pt.h   types and the seams
src/header.c   the fixed half: directives, and nothing a file can reach
src/lex.c      the lexer the header wrote
src/expand.c   Pratt with backtracking, templates, and text mode
cmd/pt.c       pt [-o out] [-g] file.pt
lib/           files meant to be @use'd
examples/      .pt beside the .out it must still produce
tests/errors.sh  what a file gets told when it is wrong
```

## Proto is read-only

`../Proto` is another process's working tree. **Read it, do not write to it** —
no edits, no branches, no commits, not even to files that look untouched. What
comes back from there is insight and prior art, nothing else.
