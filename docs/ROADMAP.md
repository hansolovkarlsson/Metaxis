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

## 1 · A rule that says it needs no separator after it

`examples/hygiene.out` carries a `};` at file scope because statements are
joined with `@separator`'s output form unconditionally, including after one that
ended in a word. The input side already knows better — a statement ending in a
word needs no separator after it, which is what lets C's `for (…) { … }` stand
alone — and only the output side is unconditional.

This was the fifth customer for the code template and is the one it did not
serve, because it is not a property of the template: a rule would have to say
something about *itself* rather than about the text it produces. Which is the
question to settle before writing anything — whether that is a word on the
`@syntax` line, something the code template can `emit` into a channel other than
the output, or a rule the joiner works out from the last character it sees.

Small, visible in a recorded output, and the last of the code template's five
customers left standing — the other four went with it into
[COMPLETED.md](COMPLETED.md).

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
behind the code template, exactly as an optional group's was — the same wall, met from
a second direction. Both are now through it: the code template can ask which
arm matched the moment there is an arm to ask about.

### Why it is not built

Nothing in `examples/` needs it, which is this tree's own test for whether a
surface has earned its place. The synonym case would justify it on its own and
has not come up in a real file. Proto's `conventions.md` puts the general form
as *a surface does not grow without a customer*; this is an idea with no
customer, filed where such an idea goes.
