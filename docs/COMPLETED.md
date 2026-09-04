# Completed

*What was **built**, and why, in enough detail to be worth reading a year from
now. [ROADMAP.md](ROADMAP.md) is the other half of the ledger and holds what is
not built; an entry is moved between the two and never copied. Each entry ends
with the state it was verified at, so a claim here can be checked rather than
taken. What a thing **costs** is not here — that is
[notation.md](notation.md)'s "What it costs", and it is written down rather than
argued away.*

Newest first.

## Groups: a part that repeats and a part that need not be there

`[ … ]` once or not at all, `[ … ]*` zero or more, `[ … ]+` one or more, with
`sep "s"` saying how turns are told apart on the way in and `join "j"` how they
are put back together on the way out.

The brackets are Prototype's own vocabulary and live outside the strings, which
is what makes them safe: a file that wants `[` and `]` in its own language
quotes them. `examples/clike.pt` declares `a "[" i "]"` for an index in the same
tool that reads `[ x ]* sep ","`.

Proto declined repetition and optional parts three times, on the grounds that no
program had asked. A language-agnostic tool has argument lists everywhere and
asks on the first file: `examples/groups.pt` writes one call rule covering every
arity there is.

Every hole is bound whether or not its group matched, so a template never has to
ask whether a part was there — and cannot ask, which is why an absent optional
part still leaves `if (!0) { ; }` in the output. That is
[ROADMAP.md](ROADMAP.md) 1's plainest customer.

*Verified at 8 examples, 20 error cases, `tests/hygiene.sh`; `make check` clean.*

## `{~t}`: a name nobody else has

A template may ask for a name nothing else uses. One expansion, one name: two
`{~t}` in a template are the same name and the next use of the rule is a
different one. A candidate is refused if it occurs anywhere in the source or in
any template any rule declared, `@use` included — a substring test, so it errs
in the safe direction.

**It closes exactly half of hygiene**, and finding out which half was the
result. A template that *introduces* a name is fixed. A template that *reaches
out* for one the caller shadowed is not, and is not fixable this way: there is
nothing to invent, and a template that is a string cannot see a scope. That half
moved out of "not done" and into "what it costs", where it belongs.

*Verified by `tests/hygiene.sh`, which compiles the C the example emits and runs
it: `swap: 2 1`, `bump: 105 0`.*

## The reference

[REFERENCE.md](REFERENCE.md) states what every part of a `.pt` file means —
directives, patterns, groups, kinds, levels, templates, both modes, the command
line, every error message the tool can produce, and the limits. It states and
does not argue; [notation.md](notation.md) argues.

Writing it meant checking every claim against the code rather than against
memory, and two of them were wrong. See [POSTMORTEM.md](POSTMORTEM.md) 2.

## The tool

C11 and `make`, plus POSIX `<regex.h>` for `@token` — the only thing here that
Proto does not also need, and the price of letting a file say what a literal is.

`prototype/src/header.c` is the fixed half: the directive grammar, which no file
can reach. `prototype/src/lex.c` is the lexer the header wrote.
`prototype/src/expand.c` is Pratt with backtracking, templates and text mode.

Six things quoting does not decide by itself were settled here, each written
down in [notation.md](notation.md) under a heading of its own: a hole's kind is
how far it reaches; two holes may not sit next to each other; a tie between a
token class and a declared word goes to the class; comments are looked for
before words; candidates under one leading word are tried longest first; and a
separator is wanted between two statements and not after one that ended in a
word.

*About 1550 lines of C.*

## The notation

Everything a directive says about text — the text it recognises and the text it
emits — is inside a string, and everything outside a string is Prototype's own
fixed vocabulary.

Two things fell out of it rather than being designed in. Four rule directives
collapsed to one, because once words are quoted a pattern's shape says whether
it is prefix, infix, postfix, circumfix or mixfix — and a pattern beginning with
a hole is a led rule while one beginning with a word is a nud rule, which is the
Pratt distinction read off instead of declared. And the lexer stopped being a
fixed budget, because the header is read before the body.

`examples/clike.pt` writes all six things Proto's `lib/clike.pro` lists as
impossible.
