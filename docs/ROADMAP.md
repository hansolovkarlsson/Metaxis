# Roadmap

*What is not built, why it is wanted, and what would have to be true for it to
land. An item moves out of here when it is settled, **including settled
against**, which is a result and stays written down with its reason.
[notation.md](notation.md)'s "Not done" is the same list seen from the other
side: what the notation admits it cannot do; [direction.md](direction.md) is the
longer horizon, and says what these items are **for**. This file is what to do
about it. And [prior-art.md](prior-art.md) is where an idea with **no** customer
lives: a survey of what other tools have, so that a feature nobody here has
asked for does not have to be put on this page to be written down.*

**Stages** below says which translator the work is being driven by, and the
items are ordered by that first and by how much is wanted after, not by size
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
| **2 · done** | C | arm64 | output that is not an expression tree: labels, order, a machine |
| **3 · done** | Python | C | a language whose blocks are indentation |
| **4 · done** | BASIC | C | a source that declares nothing: the head of the output is determined by its body |
| **5 · done** | C | C | a real file, not one written to fit: the tool rewriting its own front end |

Stage 2 landed on 2026-09-05 as `examples/asm.mx` and `tests/asm.sh`. **The
output side generalises**, which was the question: a rule's value became *the
code that computes the phrase* rather than the phrase, and nothing in the tool
had to change for that. What it cost was `@template`, which is built, and what
it found is [POSTMORTEM.md](POSTMORTEM.md) 10.

Stage 3 landed on 2026-09-05 as `examples/python.mx` and `tests/python.sh`.
**The tool took its first delimiter**, which was the question:
`@separator "\n" indent` gives the lexer an indent stack and the `block` kind
reads what it emits. The premise held: a `block` hole is spelled outside the
quotes precisely *because* an indent is not text somebody wrote. The rest fell
out untaught: nesting, a word after a block, and blank lines closing nothing.
What it cost is in [COMPLETED.md](COMPLETED.md); what the estimate taught is
[POSTMORTEM.md](POSTMORTEM.md) 15.

Its test does something no other one here does: **the body of
`examples/python.mx` is run by `python3` as well as compiled as C**, and the two
answers compared. A translation that is wrong the same way on both sides of an
operator passes a diff and passes `tests/pascal.sh`, and fails that.

[direction.md](direction.md) says what the stages are ultimately *for*, and why
stage 2 is assembly rather than another expression language.

Stage 4 landed on 2026-09-06 as `examples/basic.mx` and `tests/basic.sh`, and
it was picked by the rule in the next paragraph: the survey's shortlist said
collection attributes had a customer, and **BASIC is the source that cannot be
translated without them**. It declares nothing: a variable exists because a
line mentions it. C wants every one declared before the first statement, which
is the aggregate of every line below. The translator reached that wall exactly
where it was expected to, and nowhere else: line numbers read as the left
operand of an infix keyword, `FOR` and `NEXT` are two statements the way they
are in BASIC, and the type of a variable is read off the sigil on its name with
`replace`, which is the wall stage 1 hit at `writeln` dissolved by choice of
source. **It cost the tool nothing**: no directive, no builtin, no line of C.
What it owed was collections, built the same day. `tests/basic.sh` wrote the
four declarations by hand, pinned to fail the day the translator started
writing them, and it did, the same evening: collections are built, in
[COMPLETED.md](COMPLETED.md).

Stage 5 landed on 2026-09-06 as `lib/island.mx`, `examples/island.mx` and
`tests/island.sh`, the same day it was rehearsed. The survey had said the
island rule, text the file has no rule for passes through, was the strongest
thing the tool lacked and the one that would change what it could be pointed
at. **Text mode had it already.** A five-line rewrite over `metaxis/cmd/mx.c`,
the tool's own front end and a file written for no grammar, turns six error
calls into `complain(err)` and inserts the definition; the test compiles the
result against the tree's own objects, runs it, and it is the same tool. What
it cost: one error, for a led rule in text mode, which was accepted and silently
never fired until the rehearsal wrote one. What it owes is 7 below: the three
things text mode does not know, measured on that file rather than predicted.

**The staging exists to say where the *pressure* comes from**: one translator
at a time, taken far enough to be compiled and run, so that no feature is built
without a customer that asked for it first. A stage is not invented to have
one. What remains below is what the four stages left owing, and every item on
it has a customer or says plainly that it does not. **When the next mechanic is
wanted, the thing to pick first is the translator that would ask for it**, not
the mechanic. Stage 4 is the first time that rule was applied deliberately, and
the wall it found is the one the survey had predicted.

**This is a rule about where new mechanics come from, not a restriction on the
tool.** A `.mx` file still declares any language in and writes any language out;
`examples/` deliberately holds several, and REFERENCE.md's do too. What the
staging fixes is where the *pressure* comes from: one translator at a time, taken
far enough to be compiled and run, so that every feature added has a customer
that asked for it before it was built. That is this tree's existing test for
whether a surface has earned its place, made into a schedule.

**Why C is the output for testing.** It can be compiled and run, so the suite
can check the output is *correct* rather than merely *unchanged*.
`tests/hygiene.sh` already does this and is the only test here that catches a
wrong answer rather than a changed one, which is why the hygiene defect reads
as `bump: 105 0`, a number, instead of a diff somebody has to squint at. Every
recorded `.out` beside it catches change and nothing else.

**Why assembly is stage 2 and not stage 4.** It is the first target that is not
shaped like the input. A Pascal expression becomes a C expression one node at a
time; a C expression becomes a *sequence* with labels and an order, and the only
machinery here for that today is `{~label}`: fresh names, which already exist
and are already tested, and were built for hygiene rather than for codegen. That
is either a happy accident or the feature's second customer, and stage 2 is how
we find out which.

---

## 1 · Stage 1: what Pascal→C still owes

**Where it got to.** `examples/pascal.mx` and `examples/code.mx` read `program`,
a `var` section with comma-separated declarations, `integer` and `boolean`, and
an outer `begin … end.` that becomes `main`. `tests/pascal.sh` expands `code.mx`,
compiles the C, runs it and checks the number, so the arithmetic, the
precedence, `mod`, the loop and the branches are checked by a compiler and a
result rather than by `diff`. `pascal.mx` is expected **not** to compile, and
that half is pinned too.

**The mechanic this stage asked for is built.** `terminated(h)`, a code
template reading back the flag a rule declares about itself, is what decides
whether a branch needs its semicolon, and `code.mx` now emits idiomatic C
without bracing single statements. See COMPLETED.md.

**`procedure` and `function` are in**, with parameter lists, calls, and Free
Pascal's `Result` for the return. They needed no new mechanics: a repeated group
with `sep ";"` is the parameter list, a `stmts` hole stopping at `end` is the
body, and a led `"(" … ")"` at 95 is the call.

**`repeat … until` and `case … of` are in**, and the mechanic `case` turned out
to need, `for i, x in h` with `at(h, n)`, is built. `examples/code.mx` writes
its arms as `[ v ":" s ]*` and walks the two lists in step;
`examples/pascal.mx` still folds the pair into one hole with an infix rule,
because a string template cannot interleave two lists at all.

**`real` is in**, and it is what gave `@fragment` its first two-hole customer.
A type is now a rule of its own: a word alone, `@syntax "real" => "double"`.
So a parameter list can hold a *hole* where the type goes and translate each
one separately. `examples/code.mx` writes `void Scale(int n, double k)` and
`tests/pascal.sh` compiles and runs it. See COMPLETED.md for what it cost.

**What is left to write:** nothing, for the types this program uses. What
remains are the two decisions below.

**And two that need a decision rather than a rule.**

- `writeln` takes a variable number of arguments of mixed type and C wants a
  format string per type. There are two rules for it now, one for a literal and
  one for a number; a third argument type means a third rule. **`real` did not
  settle this and deliberately did not try**: a `real` can be declared, passed
  and typed correctly, and printing one would need the rule to know its
  argument's type, which is the wall below.
- **A parameterless procedure cannot be called.** Pascal writes `Banner;` where C
  writes `Banner();`, and nothing distinguishes that from reading a variable
  called `Banner`: the two are the same token in the same position, and telling
  them apart wants a symbol table. That is why the example declares no
  parameterless procedure: it could be written and not called.

This tool has no types and is not obviously entitled to any. Both are honest
limits of a rewriter that moves tokens rather than understanding them, and both
should be *written into the example* rather than faked, which is what was done
with the third: `real` went in as far as it goes and stopped where it would have
needed a symbol table, with the stopping place recorded.

**A third instance of the same wall turned up while doing it.** The obvious way
to write a declaration is `a ":" t`, with the type as a hole. It cannot be done:
`1: writeln(11)` and `mod: integer` are both `expr ":" expr`, and nothing
distinguishes a `case` arm from a declaration without knowing what the left side
*is*. So the declaration keeps one quoted rule per type while the parameter list
gets the hole, the difference being that inside `"(" … ")"` after a procedure
name there is no case arm to be confused with. Context again, from a new
direction.

**Settled, and staying wrong on purpose.** `pascal.mx` cannot translate
`'it''s'` into `"it's"`, because a rule cannot match a bare token and so nothing
can rewrite a literal where it stands: it has to happen inside a rule that has
a word in it, which is what `code.mx` does inside `writeln`. That is the one
thing between `pascal.out` and a program that runs, it is recorded in the file's
own closing note, and `tests/pascal.sh` fails if it ever starts compiling.

## 2 · A line that continues inside brackets

`f(a,` newline `b)` is `no rule reads '\n' here` under a newline separator.
Python's lexer suppresses the newline between an opening bracket and its match,
and this one does not.

**This is the piece stage 3 left out, and it was left out named.** The item it
came from listed three obstacles to reading Python and this was a fourth, found
by running the thing rather than by reading it, which is why it is here and not
in a comment somewhere. `examples/python.mx` says so in its own closing note and
avoids wrapped calls; nothing in the suite would otherwise mention it.

It wants a second piece of lexer state beside the indent stack, a bracket
depth, and **that is the interesting part**: the lexer cannot know what a
bracket is. Every other thing it knows came out of a directive, and there is no
directive that says *these two words nest*. So this is not the same size as the
indent stack even though it is the same kind of state, and the shape of the
declaration is the decision:

- a suffix naming the pairs, `@separator "\n" indent joining "(" ")" "[" "]"`,
  which is honest and gets long;
- a directive of its own, `@bracket "(" ")"`, which is a second global that has
  to agree with the first;
- or reading it off the rules: any rule whose pattern is a word, then holes,
  then a word. That is free, silent, and wrong the moment a rule is shaped
  like that and is not a bracket.

Nothing has asked yet: the example does not need it, and Python without wrapped
calls is still Python. It is here so that the next file that wants one finds the
question already asked.

## 3 · The three rules `as` cannot share

**The mechanic is built and this is what it did not reach.** `as` and `mx -b`
are in ([COMPLETED.md](COMPLETED.md)), `examples/backends.mx` is one grammar
read out to two targets, and the item they came from expected them to collapse
`examples/pascal.mx` and `examples/code.mx` into one file. **They do not**, and
the measurement is worth more than the expectation was:

| | |
| --- | --- |
| **36 of 39 rules** | share a pattern exactly: `as` merges these today |
| **the two `case` rules** | do not. `pascal.mx` writes `[ arm ]*`; `code.mx` writes `[ v ":" s ]*` and walks two parallel lists, because a string template cannot interleave two lists at all |
| **one rule more** | `a ":" s`, the infix arm rule, which only the folding version needs |

**`as` chooses a template and never a pattern**, so those three cannot be
shared and the 272 duplicated lines are still there. That is the falsification
this item wrote down for itself before the work, met partially and precisely.

**What would settle it is a decision, not a mechanic.** Three shapes, and none
is obviously right:

- **A rule that belongs to one target**: `@syntax … as tight` on the *rule*
  rather than the template, so a target may have a rule the other has not. It
  is the smallest change and the largest consequence: the **grammar** would
  then vary per target, not just the output, and two targets could read the
  same file differently. That is a different tool from the one whose rules are
  a single agreed reading.
- **Let a string template interleave two lists**, which removes the reason the
  two `case` rules differ at all. This is the narrowest fix and it attacks the
  actual cause, but it is a new power for the weaker template, and the whole
  argument for the code template is that some things need it.
- **Accept it.** Merge nothing, and let the two files stand as they are with
  this entry saying why. **The duplication is real and so is the reason for
  it**, and a file that shares 36 rules and lies about 3 would be worse than
  two honest files.

Nothing has picked one, and nothing should until a second grammar wants the
same thing: one instance is a fact and two are a pattern.

## 7 · The island rule: text mode wants a lexer

*Numbered 7 because a number here is stable once given; by this page's own
order it belongs first, being stage 5's. It was lost from this page for one
commit on 2026-09-06 by an edit that sliced from item 4 to item 5, and the
transcript check did not notice because no transcript was involved.*

**The customer.** `tests/island.sh` rewrites `metaxis/cmd/mx.c` with
`lib/island.mx` and runs the result, so a rewrite over a real file exists and
is checked. It works because its two rules stay inside what text mode knows.
The rehearsal that preceded it, `scratch/island/` on the day and the journal
after, tried the three things a rewrite over real C wants next, and each one
failed in a way that is now measured:

- **No tokens.** Renaming `usage` also rewrote the word inside the usage string;
  renaming `err` turned `stderr` into `stde`. A rename over real code is wrong
  without an identifier boundary, and a text-mode hole is a run of characters.
  [REFERENCE.md](REFERENCE.md) §7 already refuses a class kind in text mode
  and says honouring it is this page's job *if anybody asks*. This is somebody
  asking.
- **No brackets.** A hole over `f(x, g(y))` stops at the first `)`. The rewrite
  in `lib/island.mx` comes out right only because its template keeps the hole
  last, so the leftover `))` is copied through behind it; a template that reused
  the hole wrote `log(f(x, g(y); complain(f(x, g(y)))`. `examples/island.mx`
  records the accident on purpose.
- **Comment-aware means comment-removing.** Declaring C's `/* */` so that rules
  stop firing inside comments deleted the comments from the output, six lines
  of `mx.c`, because a text-mode comment is removed, which is right for a
  document and wrong for a rewrite that must give the file back.

**What this says about the shape.** The survey offered three shapes for
islands in *expression* mode: skip a token, skip to the separator, a declared
fallback rule. The rehearsal did not ask for any of them. What the rewrite
wants is text mode plus the three things Comby knows and nothing more:
identifiers, balanced brackets, and strings and comments that are *skipped over*
rather than removed. That is a smaller mechanic than any of the three, it does
not touch expression mode, and it does not touch the completeness that lets an
expression grammar report its own bugs. Each piece has its own decision:

- a class kind in text mode would have to mean *one token of this class, at a
  boundary*, which is a lexer inside a scan, and where the token ends is the
  question;
- a bracket-aware hole needs to know which characters are brackets, and
  [ROADMAP.md](ROADMAP.md) 2 has already met the question of whether a bracket
  is declared (`@bracket "(" ")"`) or known;
- a comment that is skipped rather than removed is a third thing `@comment`
  would mean, and may want its own word.

**And one thing the stage could not do**, which is not this item's: the
definition of `complain` goes in front of `usage` because that line happens to
exist. Collections let the rule that needs it contribute it, but the honest
place is *before the first function*, and saying that means reading C.

**Why it is not built today.** Each piece has a customer in the rehearsal and
none in a file that ships; the rewrite that ships was written to need none of
them. Build the first piece when a rewrite is wanted that a bare-word rule gets
wrong, which will be the first rename.

## 5 · Source maps

The output has no way back to the line that produced it, so an error from a
downstream compiler points into text nobody wrote. Proto emits a `.map` beside
its output. Nothing here has needed one yet, which is the only reason it is
this far down.

---

## 8 · A conformance suite: for a second engine, when one is wanted

**Hans, 2026-09-06:** *we perhaps need a conformance suite if someone likes
the Metaxis syntax but wants to write their own engine and needs to confirm
compatibility.*

Most of one exists. Every script in `tests/` takes the engine as its first
argument and defaults to `./bin/mx`; the sixteen examples and the 83 error
cases are input and expected output with no C in them. A second engine could
be pointed at the tree today. What was missing was not the suite but four
decisions the suite would force, each a place where
[REFERENCE.md](REFERENCE.md) stated this implementation rather than the
language. **All four were settled the same day, as sentences in the
reference**, and the bullets below say where:

- **Token regexes are POSIX ERE.** `@token` hands its pattern to the host
  `<regex.h>`. An engine in another language has a different alternation and
  class semantics. Either the reference names a regex subset the language
  owns, or it says tokens are host-defined and the suite avoids the
  differences. *Settled, §3.1: the dialect is POSIX ERE, leftmost-longest,
  and an engine implements that rule rather than handing the pattern to its
  host.*
- **Fresh names are observable.** §8.2's generated names are bytes a `.out`
  records, so a byte-diff suite makes the naming scheme part of the language.
  Pin it deliberately, or compare after normalising it. *Settled, §8.2:
  pinned, `label__N` on one counter for the run, advancing per candidate
  tried.*
- **Error messages are pinned to their text.** Right for one implementation,
  wrong for a second. The conformance form is *refused, at this line*, with
  the wording left to the engine. *Settled, §10: exactly that. Refused, on
  standard error, status 1, file and line named; the wording is the engine's.*
- **The limits table.** §11 needs one sentence saying whether those are
  minimums a conforming engine must reach or facts about this one. *Settled,
  §11: minimums, except the tab, which is a definition.*

Two tiers, if it is built: the byte-diff pairs are required, and the six
scripts that compile and run what they produced are optional, because they
need a C compiler, Free Pascal and `python3` and test meaning rather than
bytes. `-t`'s trace is this tool's and is excluded.

**What it costs.** A published suite says the syntax is settled, and 7 above
and 6 below say it is not. It would need a version stamp, and every item that
moves would move the suite too: a cost per change, forever. That is why the
four decisions were settled the day they were named, as sentences in the
reference, and the suite was not: the answer to a second engine is now *point
it at `tests/` and read §3.1, §8.2, §10 and §11*, and no door has been closed.
Nobody has asked; this item says so, and stays until someone does.

---

## 9 · The dated accounts, under the prose rule

`CLAUDE.md`'s style guide reached every live document on 2026-09-06, and
`tests/hygiene.sh` holds the line there. It does not scan `POSTMORTEM.md`,
`CHANGELOG.md` or the journal, which hold 354 em dashes between them, 227 of
those in the journal and 178 in one day's file. Punctuation is not a fact, so
rewriting them falsifies nothing; but each is a record of a day by its own
charter, and the journal in particular is a diary of closed days. The choice
is between sweeping the two ledgers and leaving the journal, which keeps the
site's pages uniform and the diary as written, and sweeping all six, after
which there is nothing to explain. Either way the check widens by one line.
Nobody has decided; this item says so.

---

## 6 · Alternation inside a pattern: explored, not wanted yet

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
an optional group, so nothing is ever unbound. Two arms may share a hole name:
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
its first word: `collect()` compares the current token against `el[0].word`.
That is why `a rule is found by its first word, so it cannot begin with a
group` is an error today. The synonym form above begins with a group, so
allowing it means a rule can be registered under **several** leading words
rather than one, and every arm of a leading choice must begin with a word for
that to be possible at all.

That is not hard, and it is not free either: it is the difference between
alternation being a matching feature and alternation reaching into how rules are
found. Worth knowing before starting, because it decides whether a leading
choice is in scope or whether the first arm's word has to be shared. The
narrower version, alternation anywhere *except* first, is a strictly smaller
piece of work that still covers everything nested.

**Everything else wants to know which arm matched.** `( "+" | "-" )` in one rule
is useless while the two need different output, and a string template can vary a
splice and nothing else. So the interesting half of alternation is blocked
behind the code template, exactly as an optional group's was: the same wall,
met from a second direction. Both are now through it: the code template can ask
which arm matched the moment there is an arm to ask about.

### Why it is not built

Nothing in `examples/` needs it, which is this tree's own test for whether a
surface has earned its place. The synonym case would justify it on its own and
has not come up in a real file. Proto's `conventions.md` puts the general form
as *a surface does not grow without a customer*; this is an idea with no
customer, filed where such an idea goes.
