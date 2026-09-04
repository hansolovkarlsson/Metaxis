# Completed

*What was **built**, and why, in enough detail to be worth reading a year from
now. [ROADMAP.md](ROADMAP.md) is the other half of the ledger and holds what is
not built; an entry is moved between the two and never copied. Each entry ends
with the state it was verified at, so a claim here can be checked rather than
taken. What a thing **costs** is not here — that is
[notation.md](notation.md)'s "What it costs", and it is written down rather than
argued away.*

Newest first.

## `terminated`: a rule that says its output ends a statement

The output separator used to be joined between every pair of statements,
including after one that ended in a word, so `examples/hygiene.out` carried a
`};` at file scope where C wants none. A rule may now be declared `terminated`,
and nothing is joined after it.

**The cheap answer would have been wrong, and wrong in both directions.**
Looking at the last character emitted and skipping the separator after a `}`
fails for `examples/clike.pt`, which reads C's braces and emits Solveig — where
a `.` is wanted between two statements however the one before ended — and it
fails the other way for C's own `struct { … };`. `examples/groups.pt` reads the
same braces as clike.pt and emits JavaScript, where the brace does end a
statement. So the input rule and the output rule are about two different
languages and had to stay two rules. Guessing is what this tool declines to do
about precedence and about scopes, and it declined here for the same reason.

The word sits **after the template**, which is the one place in a rule where a
bare word cannot be anything else, since a hole only appears in the pattern. So
it reserves nothing: `examples/reserved.pt` has a rule whose hole is called
`terminated` and which is itself `terminated`, four words apart.

This was the fifth of the code template's five customers, and the one that did
not come with the other four, because it turned out not to be a property of a
template at all but of a rule — so it works on both kinds.

*Verified at 9 examples, 28 error cases, `tests/hygiene.sh`; `make check` clean.*

## The code template: `=> { … }`

A second kind of template, an interpreted block rather than a string. A string
template splices and does nothing else; this one loops over a repeated hole,
asks whether an optional part matched, asks an operand what level it was parsed
at, and builds text with `emit`.

It cost nothing to tell the two apart. A template had always been a string and a
string never starts with a brace, so one character after the `=>` decides, with
no lookahead, nothing reserved, and no file written before it changing meaning.
And it keeps the one rule rather than bending it: the language is Prototype's
own, so it lives outside the strings, and the foreign text it emits lives inside
them.

Four of the five customers [ROADMAP.md](ROADMAP.md) recorded for it are served,
each named a file and each is now a recorded output:

  - **per-element output** — `for x in p sep ", " { emit "int " + x }`, where
    `join` could only put one fixed text between turns;
  - **output that depends on an optional part** — `matched(h)` and `count(h)`;
  - **parenthesisation** — `level(h)` and `group(h, n)`, which needed the
    expander to start recording the level a hole was parsed at;
  - **translating a literal** — `replace` and `drop`.

The fifth, a rule that says it needs no separator after it, is not a property of
a template and stayed behind as [ROADMAP.md](ROADMAP.md) 1.

`examples/code.pt` is `examples/pascal.pt` with every rule rewritten in the new
form and the body left character for character alone, so the diff between the
two recorded outputs is the whole argument:

```
-for (int i = 1; i <= 20; i++) if (((((i % mod) == 0)) && ((i != 9))))
+for (int i = 1; i <= 20; i++) if ((i % mod == 0) && (i != 9))
-if ((!((total > 100)))) puts('it''s middling') else puts('big')
+if (!(total > 100)) puts("it's middling") else puts("big")
```

Both files stay. The string template's cost is real and is now a **choice**: it
is what a string template costs, and what a string template buys is being four
times shorter.

What did not move is the reach-out half of hygiene, which the roadmap predicted
would not and is the one prediction there that held.

*Verified at 9 examples, 27 error cases, `tests/hygiene.sh`; `make check` clean.*

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
