# Roadmap

*What is not built, why it is wanted, and what would have to be true for it to
land. An item moves out of here when it is settled — **including settled
against**, which is a result and stays written down with its reason.
[notation.md](notation.md)'s "Not done" is the same list seen from the other
side: what the notation admits it cannot do. This file is what to do about it.*

Ordered by how much is wanted, not by size and not by how much is known —
the last entry is the best worked out and the least asked for, which is why
it is last.

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

---

## 6 · Alternation inside a pattern — explored, not wanted yet

**Hans, 2026-09-04, exploring, and saying so:** *anything regarding alternation
can wait to later, if we even need it.* It is last on this page for that reason
and not because it is the largest. Nothing asks for it, no file is worse without
it, and it may never be built.

It is kept because two things came out of the exploring that are worth more than
the feature would be, and that somebody would otherwise re-derive: the bracket
argument, and a constraint in how rules are found. Both are below.

### What exists instead

**Alternation between whole rules already works.** Rules sharing a leading word
are candidates, tried longest-pattern-first with the token cursor restored on
failure, which is how `if c then t` and `if c then t else f` coexist. So the
missing case is narrow and specific: alternation in a **nested** position, where
lifting it out would mean writing the whole rule once per arm.

### The shape it would take, if it were built

```
( a | b )      one of them, required
[ a | b ]      one of them, or none
( a | b )*     repeat a choice
```

`[ … ]` is already *optional*, so `[ a | b ]` alone would be doing two jobs and
there would be no way to say *one of these, required*. A second bracket is what
separates them, and each then has one job: **`( … )` groups, `[ … ]` makes
optional, `*` and `+` repeat.**

**And it is the reason `( … )` stays unspent.** The same day, `[ … ]` for an
optional group was queried and swapping it for `( … )` was offered; the offer
was declined and the brackets stay as they are. Even if this entry is never
built, keeping a second bracket free costs nothing and having spent it would
have been hard to undo.

**Ordered choice**, first arm that matches winning, is the rule to pick: it
matches PEG, and it matches *declaration order breaks a tie* elsewhere in this
tool. Holes in an arm that did not match bind to the empty string, as they do in
an optional group, so nothing is ever unbound. Two arms may share a hole name —
`( a:name | a:number )` binding `a` either way is the point rather than an
accident.

**None of this is a decision.** It reads well and it is written down for that
reason alone; when and if somebody builds this, a different shape may turn out
better and nothing here outranks it.

### What it would and would not buy

**Synonyms work under any spelling and need nothing else:**

```
( "fn" | "func" | "function" ) f:name "(" … ")"
```

Several spellings in, one thing out. That is the honest customer, and it is real
but small.

**And it is the one case the dispatcher currently forbids.** A rule is found by
its first word — `collect()` compares the current token against `el[0].word` —
which is why `a rule is found by its first word, so it cannot begin with a
group` is an error today. The synonym form above begins with a group, so
allowing it means a rule can be registered under **several** leading words
rather than one, and every arm of a leading choice must begin with a word for
that to be possible at all.

That is not hard, and it is not free either: it is the difference between
alternation being a matching feature and alternation reaching into how rules are
found. Worth knowing before starting, because it decides whether a leading
choice is in scope or whether the first arm's word has to be shared — and the
narrower version, alternation anywhere *except* first, is a strictly smaller
piece of work that still covers everything nested.

**Everything else wants to know which arm matched.** `( "+" | "-" )` in one rule
is useless while the two need different output, and a string template can vary a
splice and nothing else. So the interesting half of alternation is blocked
behind item 1 above, exactly as an optional group's is — the same wall, met from
a second direction, which is worth noticing as evidence about item 1 rather than
about this one.

### Why it is not built

Nothing in `examples/` needs it, which is this tree's own test for whether a
surface has earned its place. The synonym case would justify it on its own and
has not come up in a real file. Proto's `conventions.md` puts the general form
as *a surface does not grow without a customer*; this is an idea with no
customer, filed where such an idea goes.
