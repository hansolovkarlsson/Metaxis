# Roadmap

*What is not built, why it is wanted, and what would have to be true for it to
land. An item moves out of here when it is settled — **including settled
against**, which is a result and stays written down with its reason.
[notation.md](notation.md)'s "Not done" is the same list seen from the other
side: what the notation admits it cannot do. This file is what to do about it.*

Ordered by how much is already known about them, not by size.

---

## 1 · A second kind of template: `=> { … }`

**Hans, 2026-09-04.** Today a template is a string and expansion is splicing.
The other form is a small interpreted language:

```
@syntax "fn" f:name "(" [ p:name ]* sep "," ")" "{" b:stmts "}"
    => { … }
```

### It does not collide, and this is the reason to write it down now

A template is *always* a string today. `{` is not how a Prototype string
starts, so **one character after the `=>` tells the two forms apart**, with no
lookahead and no ambiguity. Nothing has to be reserved and no existing file
changes meaning.

More than that, it keeps the one rule intact rather than bending it:

> Everything a directive says about text is inside a string; everything outside
> a string is Prototype's own vocabulary.

**The interpreted language is Prototype's own, so it belongs outside the
strings — and the text it emits is foreign, so that stays inside them.** A code
template would be full of string literals, and they would be Prototype strings
spelled Prototype's way, exactly as `@syntax`'s own words are. The two template
forms are then the same rule read twice: a string template is the degenerate
case where the code is one literal.

### It has five customers already in the tree

Each is a defect or a wart that exists today and that a string template cannot
fix. This is the argument for building it, and the specification for how far it
has to reach.

**1. Per-element output for a repeated group.** `examples/groups.pt` can write
`f(a, b, c)` because `join` puts one fixed text between the turns. It cannot
write C's `int a, int b, int c`, because that wants a *template per element*
rather than a joiner. `join` covers the easy half and was built knowing so.

**2. Output that depends on whether an optional part matched.**
`examples/groups.pt` ends with a `loop … or …` whose `or` part is optional, and
with no `or` the expansion still writes `if (!0) { ; }`. An optional group can
leave a hole empty and can do nothing else, because a splice is the only thing
a string can vary. This is the plainest customer and the one the example points
at by name.

**3. Output parenthesisation.** `examples/pascal.out` is recorded with
`if (((((i % mod) == 0)) && ((i != 9))))` because every rule writes its own
parentheses unconditionally — the tool knows the input grammar and nothing
about the target's precedence. A rule that could *ask* whether its operand
needs bracketing would write them only where they are wanted. See
[notation.md](notation.md), "What it costs".

**4. Translating a literal rather than moving it.** `examples/pascal.pt` emits
`puts('it''s middling')` into C, because a `string` hole splices the source text
it matched. Rewriting `'it''s'` as `"it's"` is a loop over characters, which is
the smallest thing this language would have to be able to do.

**5. The output separator after a statement that ended in a word.**
`examples/hygiene.out` carries a `};` at file scope because statements are
joined unconditionally. A rule that could say *I end in a brace, no separator
after me* would settle it.

### What it would need to be, and what it must not become

The five customers above are a loop, a conditional, string building, and a
question or two about a hole. That is the whole shopping list:

- values are **text**, **lists of text**, and probably **integers** for a level;
- holes are in scope by name, and a list hole is a list rather than a
  pre-joined string — which is the one thing the current `join` throws away;
- `if`, a `for` over a list, `+` or interpolation, and something like
  `emit`/return;
- a way to ask a hole a question: *did you match*, *what level were you parsed
  at*, *how many turns*;
- `{~t}` keeps working, because a fresh name is orthogonal to which template
  form asked for it.

**What it must not become is a general-purpose language.** Proto's
`conventions.md` puts it as *a surface does not grow without a customer*, and
every item on the list above names a file in this repository. Anything that
cannot name one waits.

### What it still would not fix

The reach-out half of hygiene. `examples/hygiene.pt`'s `bump` means the
file-scope `total` that was in scope where the rule was written, and a caller
that shadows `total` gets the shadow. Knowing that requires knowing the output
language's binding rules, and no template form — string or code — gets that for
free. It is the price of being agnostic and it stays where it is, in
[notation.md](notation.md) under "What it costs".

---

## 2 · Groups in text mode

`@mode text` refuses a rule with a `[ … ]` in it: *a group belongs to `@mode
expression`*. The text matcher scans left to right and finds a hole's end by
looking for the pattern's next literal word, and a group makes *the next word*
a question with more than one answer.

It is wanted — an optional part is at least as natural in prose as in a
language — and it is a real piece of work rather than a missing line. What
would have to be settled: whether the text matcher backtracks over positions the
way the expression parser backtracks over tokens, or whether groups in text mode
are restricted to the cases where the next word is unambiguous.

## 3 · A class-kind hole in text mode

`@syntax "[" x:name "]"` in text mode ignores the kind and takes everything up
to the `]`, silently. It should either match the class at that position or be
refused at declaration. The second is one line and is probably right, since a
`text` hole is what a text-mode rule almost always wants; the first is what
somebody will eventually ask for.

## 4 · Two files declaring one word

`@use` two files that both declare `"+"` and the later one wins, silently.
Proto's README has a section on this and a rule; this has neither. The question
is not how to detect it — that is easy — but what the right answer is: refuse,
warn, or let a file say which it meant.

## 5 · Source maps

The output has no way back to the line that produced it, so an error from a
downstream compiler points into text nobody wrote. Proto emits a `.map` beside
its output. Nothing here has needed one yet, which is the only reason it is
this far down.
