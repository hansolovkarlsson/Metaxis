<p align="center"><img src="site/logo.png" width="150" alt="The Metaxis mark"></p>

# Metaxis

[![check](https://github.com/hansolovkarlsson/Metaxis/actions/workflows/check.yml/badge.svg)](https://github.com/hansolovkarlsson/Metaxis/actions/workflows/check.yml)

A language-agnostic rewriter and **evaluator for expressions**: a file declares
its own grammar in its header and is then read with it, in one pass, with
neither the language read nor the language written built in. `.mx` in, `.out`
out. Or, in [examples/calc.mx](examples/calc.mx), an answer, because a template
can compute rather than emit.

**Metaxis** is Greek μεταξύ: Plato's *in-betweenness*, and in Boal's theatre the
state of belonging completely and simultaneously to two autonomous worlds, which
is what a tool that owns neither of its languages does.
[docs/direction.md](docs/direction.md)'s "What to call it" has why, and
[docs/prior-art.md](docs/prior-art.md) § 1 has what it was chosen over. It was
called *Prototype* for its first two days.

Its one premise is that **every mention of foreign text inside a directive is a
string**: quoted words on the pattern side, a quoted template with `{hole}`
splices on the output side.

```
@syntax "if" c "then" t  =>  "if ({c}) {{ {t} }}"
        ~~~~     ~~~~~~      ~~~~~~~~~~~~~~~~~~~
        mentioned, quoted    emitted, quoted
```

## Why

Proto is a sibling project, a language whose files declare notation of their
own, and it is where this one's premise came from, by running out of room.
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

[docs/REFERENCE.md](docs/REFERENCE.md) states what every part of a `.mx` file
means: directives, patterns, groups, kinds, levels, templates, both modes, the
command line and every error message. [docs/notation.md](docs/notation.md) says
why it is shaped that way and what it cost.
[docs/ROADMAP.md](docs/ROADMAP.md) says what is not built and what would have to
be true for it to land. [docs/prior-art.md](docs/prior-art.md) surveys the tools
that do something like this and says what they have that this does not: TXL,
Stratego, Rascal, Comby, Ohm, Coq's `Notation`, Seed7, and META II in 1964.
[docs/languages.md](docs/languages.md) says which languages the tool can be
pointed at and which it cannot, from Pascal and BASIC to C, Lisp and the
shell, and names the property each verdict rests on.

## Reading it

**The website is [hansolovkarlsson.github.io/Metaxis](https://hansolovkarlsson.github.io/Metaxis/)**:
every page of it generated from the documents below by `site/build.py`, and
rebuilt on every push once `make check` has passed.

[docs/tutorial.md](docs/tutorial.md) teaches the tool one concept at a time,
each with a file under `docs/tutorial/` you can run and the output it gives,
and every transcript in it is checked by `make check`. [docs/glossary.md](docs/glossary.md)
explains the terms of art and the concepts behind them, for a reader meeting
them here first: PEG, Pratt, nud and led, attribute grammar, hygiene.
[docs/REFERENCE.md](docs/REFERENCE.md) states what every part of a file means;
[docs/notation.md](docs/notation.md) says why it is shaped that way.

## Build

```
make            # bin/mx
make check      # every example against the .out beside it, then the nine
                # scripts in tests/ -- six of which run what they produced,
                # one of which is large enough to show a quadratic, and one
                # of which runs the transcripts in docs/
```

`make check` runs on every push, on Linux and macOS, through
[.github/workflows/check.yml](.github/workflows/check.yml). The two rows are not
redundant: `tests/asm.sh` assembles and runs its arm64 on macOS and takes its
skip branch on x86 Linux, so both halves are exercised.

C11 and `make`, plus POSIX `<regex.h>` for `@token`.

## The examples

Each is run by `make check` against the `.out` recorded beside it.

| | |
| --- | --- |
| [first.mx](examples/first.mx) | the smallest file that shows the shape, and what [REFERENCE.md](docs/REFERENCE.md) § 1 opens with |
| [calc.mx](examples/calc.mx) | the one file here with no target language: it **runs** its notation instead of translating it, and its header records where that stops |
| [asm.mx](examples/asm.mx) | C in, arm64 assembly out: a target that is a sequence rather than a tree, with labels and an order the input never mentions |
| [tour.mx](examples/tour.mx) | the idea in one file: infix, prefix, circumfix, mixfix, and `then` used as a word and as a name four lines apart |
| [clike.mx](examples/clike.mx) | the six things Proto's `lib/clike.pro` lists as impossible: `;` between statements, `x++`, `a[i]`, `p->f`, a lone `|`, `for`, and `42` without a sigil |
| [pascal.mx](examples/pascal.mx) | Pascal in, C out: stage 1. `program`, `var`, `procedure`, `function`, `begin`/`end`, `if`, `while`, `for`, `repeat`, `case`, calls, and the operator words. `pascal.out` keeps its parenthesis noise, which is the cost of agnosticism showing itself, and is the one output here that is expected not to compile |
| [python.mx](examples/python.mx) | Python in, C out: stage 3, and a block that is an **indentation**. `@separator "\n" indent` gives the lexer a stack of columns and a `block` hole reads what it emits: the one delimiter in the notation that is not a string, because an indent is not text anybody wrote. Its body is real Python, and `tests/python.sh` runs it under `python3` as well as compiling the C, so the two answers can be compared |
| [basic.mx](examples/basic.mx) | BASIC in, C out: stage 4, and a source that **declares nothing**. A line number is the left operand of its statement, `FOR` and `NEXT` are two statements the way BASIC means them, and the type of a variable is the sigil on its name. The declarations C wants first are the aggregate of every line below, and the LET and FOR that meet a name `contribute` its declaration to a collection that leads the output; `tests/basic.sh` compiles the result with nothing supplied but `main` |
| [island.mx](examples/island.mx) | stage 5: **C in, C out, over a file not written for it.** The rules in [lib/island.mx](lib/island.mx) turn one `fprintf` shape into a call and insert its definition, and leave everything else alone; `tests/island.sh` points them at `metaxis/cmd/mx.c` itself, compiles what comes out against the tool's own objects, and runs it. A third rule renames a variable and leaves `outpath`, a string and a comment alone, because the file declares C's tokens as classes and the scan moves by them; a fourth puts text after a hole over a nested call, which is right because the file declares C's brackets. Text mode was an island grammar all along |
| [poem.mx](examples/poem.mx) | `@mode text`: prose in, HTML out |
| [reserved.mx](examples/reserved.mx) | every character Metaxis writes a directive with, declared as an operator by a directive: `@`, `=>`, `.`, `:`, `<`, `>`, `"`, `{`, `}` |
| [use.mx](examples/use.mx) | `@use`, taking its arithmetic from [lib/arith.mx](lib/arith.mx) and keeping its own comment and separator, a diamond through [lib/vector.mx](lib/vector.mx), and an `override` of one of arith's rules |
| [code.mx](examples/code.mx) | `=> { … }`: `examples/pascal.mx` rule for rule, with the parenthesis noise gone, the literal translated, and the C indented. `diff examples/pascal.out examples/code.out` is the point, and `tests/pascal.sh` compiles this one and runs it |
| [backends.mx](examples/backends.mx) | **one grammar, two targets.** Every rule is written once; where the two agree there is one template and no tag, and where they differ a second `=> … as tight` sits under the first. `mx` and `mx -b tight` emit different C from the same file, both compile, and both print `7 2`: the difference is what it reads like, not what it means |
| [groups.mx](examples/groups.mx) | `[ … ]`, `[ … ]*` and `[ … ]+`: an argument list of any arity in one rule, and an optional part |
| [hygiene.mx](examples/hygiene.mx) | `{~t}`, and the half of hygiene it cannot close. `tests/hygiene.sh` compiles the C it emits and runs it, so the remaining wrong answer is a number |

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

One character after the `=>` says which form it is: a template had always been
a string, and a string never starts with a brace. The language is Metaxis's
own, so it lives outside the strings and the foreign text it emits lives inside
them, which is the same rule the pattern side follows.

## What it costs

The costs are in [docs/notation.md](docs/notation.md) and are demonstrated
rather than asserted. `examples/pascal.out` keeps its parenthesis noise, because
the tool knows the input grammar and nothing about the target's precedence. And
`make check` runs `tests/hygiene.sh`, which compiles the output of
`examples/hygiene.mx` and prints:

```
ok      hygiene.sh: {~t} holds; reaching out still does not
            swap: 2 1      the template's own name, twice over
            bump: 105 0    would be  bump: 100 5
```

Hygiene is two failures and not one. `{~t}` closes the half where a template
**introduces** a name: it asks for a name nobody else has, and two calls of one
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
metaxis/include/mx.h   types and the seams
metaxis/src/header.c   the fixed half: directives, and nothing a file can reach
metaxis/src/lex.c      the lexer the header wrote
metaxis/src/expand.c   Pratt with backtracking, templates, and text mode
metaxis/src/code.c     the second kind of template, parsed and run
metaxis/cmd/mx.c       mx [-o out] [-b backend] [-t] [-g] file.mx
lib/                     files meant to be @use'd
examples/                .mx beside the .out it must still produce
tests/errors.sh          what a file gets told when it is wrong
tests/hygiene.sh         four properties: every run goes through limit.sh, a
                         roadmap number cited anywhere resolves, no em dash in
                         prose, and what a string template cannot do, run
tests/pascal.sh          Pascal in, C out, compiled and run -- the number is the test
tests/asm.sh             C in, arm64 out, assembled and run on a CPU
tests/python.sh          Python in, C out -- and the same text run as Python too
tests/scale.sh           one input large enough for a quadratic to show
tests/limit.sh           a wall-clock limit, so a hang is reported and not waited on
.github/workflows/       make check, on Linux and macOS, on every push
docs/REFERENCE.md        every part of a .mx file, exhaustively
docs/notation.md         the one rule, what falls out of it, and what it costs
docs/direction.md        what this could become, and which futures are declined
docs/prior-art.md        what else does this, and what they have that we do not
docs/languages.md        which languages this reads, which it does not, and why
docs/ROADMAP.md          what is not built, and what would settle it
```
