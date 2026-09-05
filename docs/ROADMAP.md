# Roadmap

*What is not built, why it is wanted, and what would have to be true for it to
land. An item moves out of here when it is settled — **including settled
against**, which is a result and stays written down with its reason.
[notation.md](notation.md)'s "Not done" is the same list seen from the other
side: what the notation admits it cannot do; [direction.md](direction.md) is the
longer horizon, and says what these items are **for**. This file is what to do
about it.*

**Stages** below says which translator the work is being driven by, and the
items are ordered by that first and by how much is wanted after — not by size
and not by how much is known. The last entry is the best worked out and the
least asked for, which is why it is last.

---

## Stages

**Hans, 2026-09-05:** *would it be prudent to focus on one set of conversion for
now? Let's say we're building a Pascal→C translator for now, and that can be
what goes into the examples in the documents, and then in the next stage, we do
C→assembly, and then next we do something else useful, and that way we can work
on introducing new mechanics and test them out properly.*

| | reads | writes | what it is for |
| --- | --- | --- | --- |
| **1 · done** | Pascal | C | the mechanics that exist, finished and made testable |
| **2 · done** | C | arm64 | output that is not an expression tree — labels, order, a machine |
| **3 · next** | Python | C | a language whose blocks are indentation |

Stage 2 landed on 2026-09-05 as `examples/asm.pt` and `tests/asm.sh`. **The
output side generalises**, which was the question: a rule's value became *the
code that computes the phrase* rather than the phrase, and nothing in the tool
had to change for that. What it cost was `@template`, which is built, and what
it found is [POSTMORTEM.md](POSTMORTEM.md) 10.

[direction.md](direction.md) says what the stages are ultimately *for*, and why
stage 2 is assembly rather than another expression language.

**This is a rule about where new mechanics come from, not a restriction on the
tool.** A `.pt` file still declares any language in and writes any language out;
`examples/` deliberately holds several, and REFERENCE.md's do too. What the
staging fixes is where the *pressure* comes from: one translator at a time, taken
far enough to be compiled and run, so that every feature added has a customer
that asked for it before it was built. That is this tree's existing test for
whether a surface has earned its place, made into a schedule.

**Why C is the output for testing.** It can be compiled and run, so the suite
can check the output is *correct* rather than merely *unchanged*.
`tests/hygiene.sh` already does this and is the only test here that catches a
wrong answer rather than a changed one — which is why the hygiene defect reads
as `bump: 105 0`, a number, instead of a diff somebody has to squint at. Every
recorded `.out` beside it catches change and nothing else.

**Why assembly is stage 2 and not stage 4.** It is the first target that is not
shaped like the input. A Pascal expression becomes a C expression one node at a
time; a C expression becomes a *sequence* with labels and an order, and the only
machinery here for that today is `{~label}` — fresh names, which already exist
and are already tested, and were built for hygiene rather than for codegen. That
is either a happy accident or the feature's second customer, and stage 2 is how
we find out which.

---

## 1 · Stage 1 — what Pascal→C still owes

**Where it got to.** `examples/pascal.pt` and `examples/code.pt` read `program`,
a `var` section with comma-separated declarations, `integer` and `boolean`, and
an outer `begin … end.` that becomes `main`. `tests/pascal.sh` expands `code.pt`,
compiles the C, runs it and checks the number, so the arithmetic, the
precedence, `mod`, the loop and the branches are checked by a compiler and a
result rather than by `diff`. `pascal.pt` is expected **not** to compile, and
that half is pinned too.

**The mechanic this stage asked for is built.** `terminated(h)` — a code
template reading back the flag a rule declares about itself — is what decides
whether a branch needs its semicolon, and `code.pt` now emits idiomatic C
without bracing single statements. See COMPLETED.md.

**`procedure` and `function` are in**, with parameter lists, calls, and Free
Pascal's `Result` for the return. They needed no new mechanics: a repeated group
with `sep ";"` is the parameter list, a `stmts` hole stopping at `end` is the
body, and a led `"(" … ")"` at 95 is the call.

**`repeat … until` and `case … of` are in**, and the mechanic `case` turned out
to need — `for i, x in h` with `at(h, n)` — is built. `examples/code.pt` writes
its arms as `[ v ":" s ]*` and walks the two lists in step;
`examples/pascal.pt` still folds the pair into one hole with an infix rule,
because a string template cannot interleave two lists at all.

**What is left to write:** the types past `integer` and `boolean`, which is one
of the decisions below rather than a rule.

**And three that need a decision rather than a rule.**

- `writeln` takes a variable number of arguments of mixed type and C wants a
  format string per type. There are two rules for it now, one for a literal and
  one for a number; a third argument type means a third rule.
- Pascal's `real` beside its `integer` is the same question one layer down, and
  it reaches the parameter lists too: `join ", int "` writes one type in front of
  every turn, and a group whose parameters differ needs the loop form.
- **A parameterless procedure cannot be called.** Pascal writes `Banner;` where C
  writes `Banner();`, and nothing distinguishes that from reading a variable
  called `Banner` — the two are the same token in the same position, and telling
  them apart wants a symbol table. That is why the example declares no
  parameterless procedure: it could be written and not called.

This tool has no types and is not obviously entitled to any. All three are
honest limits of a rewriter that moves tokens rather than understanding them,
and all three should be *written into the example* rather than faked.

**Settled, and staying wrong on purpose.** `pascal.pt` cannot translate
`'it''s'` into `"it's"`, because a rule cannot match a bare token and so nothing
can rewrite a literal where it stands — it has to happen inside a rule that has
a word in it, which is what `code.pt` does inside `writeln`. That is the one
thing between `pascal.out` and a program that runs, it is recorded in the file's
own closing note, and `tests/pascal.sh` fails if it ever starts compiling.

## 2 · Stage 3 — a block that is an indentation

Python ends a block by out-denting, and there is no way to say that here. Three
things stand in the way, and only the first is obvious:

- **A `stmts` hole is refused unless a literal word follows it to stop at** —
  `a 'stmts' hole needs a word after it to stop at`, checked at declaration.
  Python has no such word: no `}`, no `end`.
- **The lexer has no notion of indentation.** Leading whitespace is skipped, and
  the only way a newline becomes significant is `@separator "\n"`, which
  collapses a *run* of newlines into one separator — flattening exactly the
  structure Python needs.
- **The obvious way round it does not work.** A `@token` class whose regex
  matches an indented run is expressible, since `.` matches a newline. But a
  class-kind hole splices the source text it matched *verbatim* and never
  expands it, so the block would be copied through untranslated, which is the
  whole point of translating it.

So it wants block-open and block-close tokens out of the lexer, and a way for a
rule to say *a block goes here*. **And it is the first thing that would put a
delimiter inside the tool rather than inside a string**, which is the premise
everything else rests on. Whether that is a new kind, a form of `@separator`, or
a directive of its own is the interesting question and it is not answered here.

Python's *expressions* — arithmetic, comparison, `and`/`or`/`not`, `x if c else
y`, calls, subscripts — need none of this and read correctly today. That is worth
knowing but is not worth an example on its own: a file that says Python and
cannot write an `if` claims more than it does.

## 3 · A class named after a kind

`@token expr "…"` declares a class that can never be used, because `x:expr` is
resolved as the *kind* `expr` and the class is never consulted — silently, and
the rule parses and runs. `@token text` is the same shadowing but happens to
produce `a 'text' hole belongs to @mode text`, which is an error about the wrong
thing. `stmts` is the third.

Refusing the three names at `@token` is about two lines and the message writes
itself. Found while writing a Python example whose string class was called
`text`.

**`@fragment` deliberately stayed out of this namespace**, which is why it is
still only these three names and still about two lines. A fragment is spliced
with `@name` rather than written after a `:`, so a `@fragment` and a `@token`
class may share a name and neither shadows the other. See
[COMPLETED.md](COMPLETED.md) for why that was the right side to put it on.

## 4 · A budget for expression-mode backtracking

Candidates under one leading word are retried with the cursor restored, and the
only thing that stops it is a recursion depth of 400. Text mode was in the same
position until its matcher became a search, and got a budget of 200000 attempts
per rule at the same time — and a measurement, 113KB of markdown in 60ms. The
expression side has neither.

Nothing has hit it, which is why it is here rather than done: a grammar that is
slow to parse would have to be written on purpose, and none has been. What the
item is really asking for is the **measurement** — a large program in one of the
declared dialects, timed — because a budget picked without one is a number
somebody made up. `programs/` in Proto is where that kind of evidence lives
there; there is no equivalent here yet.

## 5 · `@mode` declared twice

A rule's pattern, `@token`, `@separator`, `@template` and `@fragment` are all
refused when declared twice without `override` (REFERENCE.md §3.10). `@mode` is
the one global that is not:
a second `@mode` silently replaces the first, which is the same silence the
`override` work was done to remove, one directive over. It was left out because
the scope of that work was settled as those three and widening it unasked is
what the item it came from existed to prevent.

It is small — the same shape as `@separator`'s check — and the only real
question is whether `@mode expression` after `@mode text` is a thing a file
could ever mean, or whether a second `@mode` should simply be an error with no
`override` at all.

## 6 · Source maps

The output has no way back to the line that produced it, so an error from a
downstream compiler points into text nobody wrote. Proto emits a `.map` beside
its output. Nothing here has needed one yet, which is the only reason it is
this far down.

---

## 7 · Alternation inside a pattern — explored, not wanted yet

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
