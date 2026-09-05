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

## Reading it

[docs/REFERENCE.md](docs/REFERENCE.md) states what every part of a `.pt` file
means — directives, patterns, groups, kinds, levels, templates, both modes, the
command line and every error message. [docs/notation.md](docs/notation.md) says
why it is shaped that way and what it cost.
[docs/ROADMAP.md](docs/ROADMAP.md) says what is not built and what would have to
be true for it to land.

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
| [first.pt](examples/first.pt) | the smallest file that shows the shape, and what [REFERENCE.md](docs/REFERENCE.md) § 1 opens with |
| [tour.pt](examples/tour.pt) | the idea in one file: infix, prefix, circumfix, mixfix, and `then` used as a word and as a name four lines apart |
| [clike.pt](examples/clike.pt) | the six things Proto's `lib/clike.pro` lists as impossible — `;` between statements, `x++`, `a[i]`, `p->f`, a lone `|`, `for`, and `42` without a sigil |
| [pascal.pt](examples/pascal.pt) | Pascal in, C out — stage 1. `program`, `var`, `begin`/`end`, `if`, `while`, `for`, and the operator words. `pascal.out` keeps its parenthesis noise, which is the cost of agnosticism showing itself, and is the one output here that is expected not to compile |
| [poem.pt](examples/poem.pt) | `@mode text`: prose in, HTML out |
| [reserved.pt](examples/reserved.pt) | every character Prototype writes a directive with — `@`, `=>`, `.`, `:`, `<`, `>`, `"`, `{`, `}` — declared as an operator by a directive |
| [use.pt](examples/use.pt) | `@use`, taking its arithmetic from [lib/arith.pt](lib/arith.pt) and keeping its own comment and separator — a diamond through [lib/vector.pt](lib/vector.pt), and an `override` of one of arith's rules |
| [code.pt](examples/code.pt) | `=> { … }` — `examples/pascal.pt` rule for rule, with the parenthesis noise gone and the literal translated. `diff examples/pascal.out examples/code.out` is the point, and `tests/pascal.sh` compiles this one and runs it |
| [groups.pt](examples/groups.pt) | `[ … ]`, `[ … ]*` and `[ … ]+` — an argument list of any arity in one rule, and an optional part |
| [hygiene.pt](examples/hygiene.pt) | `{~t}`, and the half of hygiene it cannot close. `tests/hygiene.sh` compiles the C it emits and runs it, so the remaining wrong answer is a number |

The headline is `for`, which puts two `;` inside a pattern in a file whose body
ends statements with `;`:

```
@syntax "for" "(" init ";" c ";" step ")" "{" b:stmts "}"
    => "{init}.\n{{ {c} }}:whileTrue({{ {b}. {step} }})"
```

There is nothing to disambiguate because there was never an ambiguity.

## Two kinds of template

`=> "…"` splices, and is enough most of the time. `=> { … }` is a small
interpreted language for when it is not:

```
@syntax "fn" f:name "(" [ p:name ]* sep "," ")"
    => {
        emit "int " + f + "("
        if count(p) == 0 { emit "void" }
        for x in p sep ", " { emit "int " + x }
        emit ")"
    }
```

One character after the `=>` says which form it is — a template had always been
a string, and a string never starts with a brace. The language is Prototype's
own, so it lives outside the strings and the foreign text it emits lives inside
them, which is the same rule the pattern side follows.

## What it costs

The costs are in [docs/notation.md](docs/notation.md) and are demonstrated
rather than asserted. `examples/pascal.out` keeps its parenthesis noise, because
the tool knows the input grammar and nothing about the target's precedence. And
`make check` runs `tests/hygiene.sh`, which compiles the output of
`examples/hygiene.pt` and prints:

```
ok      hygiene.sh: {~t} holds; reaching out still does not
            swap: 2 1      the template's own name, twice over
            bump: 105 0    would be  bump: 100 5
```

Hygiene is two failures and not one. `{~t}` closes the half where a template
**introduces** a name — it asks for a name nobody else has, and two calls of one
rule get two of them. The half where a template **reaches out** for a name the
caller shadowed stays open, and is not a missing feature: there is nothing for a
fresh name to invent, what is wanted is a way to say *the outer one*, and a
template that is a string cannot see a scope. Proto can, because its templates
are trees in a language whose scopes it knows.

**That test passing means the second half is still wrong**, which is deliberate:
a test that merely failed would be turned off, and one that pins the wrong
answer has to be edited by whoever fixes it. Its previous version pinned
`swap: 2 2`; `{~t}` made that wrong, and rewriting it was part of the same
commit.

## Layout

```
prototype/include/pt.h   types and the seams
prototype/src/header.c   the fixed half: directives, and nothing a file can reach
prototype/src/lex.c      the lexer the header wrote
prototype/src/expand.c   Pratt with backtracking, templates, and text mode
prototype/src/code.c     the second kind of template, parsed and run
prototype/cmd/pt.c       pt [-o out] [-g] file.pt
lib/                     files meant to be @use'd
examples/                .pt beside the .out it must still produce
tests/errors.sh          what a file gets told when it is wrong
tests/hygiene.sh         what a template that is a string cannot do, run
docs/REFERENCE.md        every part of a .pt file, exhaustively
docs/notation.md         the one rule, what falls out of it, and what it costs
docs/ROADMAP.md          what is not built, and what would settle it
```

## Proto is read-only

`../Proto` is another process's working tree. **Read it, do not write to it** —
no edits, no branches, no commits, not even to files that look untouched. What
comes back from there is insight and prior art, nothing else.
