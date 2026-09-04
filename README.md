# Prototype

**Working name.** An experiment in one question: what a Proto-shaped tool — a
file that declares its own grammar in its header and is then read with it —
looks like if **every mention of foreign text inside a directive is a string**.
Language agnostic; `.pt` in, `.out` out.

Nothing is implemented. What exists is a notation and five files written in it.

- [docs/notation.md](docs/notation.md) — the rule, the grammar, what falls out
  of it, what it costs, and what is unproven.
- [examples/tour.pt](examples/tour.pt) — the idea in one file.
- [examples/clike.pt](examples/clike.pt) — the six things Proto's
  `lib/clike.pro` lists as impossible, written.
- [examples/pascal.pt](examples/pascal.pt) — Pascal in, C out; neither language
  the tool's. With [pascal.out](examples/pascal.out), whose parenthesis noise is
  the cost of agnosticism showing itself.
- [examples/poem.pt](examples/poem.pt) — `@mode text`: prose in, HTML out.
- [examples/reserved.pt](examples/reserved.pt) — every character Prototype uses
  to write a directive, declared as an operator by a directive.

## Why

Proto's grammar and the grammars a Proto file declares are drawn from one pool
of characters. `;` cannot separate statements because it opens a comment; `|`
cannot be an operator because it delimits a block's parameters; `.` `,` and `:`
are spoken for; a pattern's holes are written `<x>` with two characters that are
themselves operator characters; `42` cannot be an integer because the lexer said
so first. Each is a reasonable local decision and together they are a budget
that runs out.

Putting the mention in quotes spends nothing, because a string's boundaries are
the one thing every reader already agrees on.

## Proto is read-only

`../Proto` is another process's working tree. **Read it, do not write to it** —
no edits, no branches, no commits, not even to files that look untouched. What
comes back from there is insight and prior art, nothing else.
