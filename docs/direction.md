# Direction

*Where this could go, and why — the theory of the tool rather than the list of
jobs. [ROADMAP.md](ROADMAP.md) says what to build next and what would settle it;
this page says what the next things are **for**, and which futures are being
declined. It is the one record here allowed to be speculative, and every claim
in it that could be checked against the code has been. When a direction is taken
or abandoned, it is rewritten rather than appended to: a page of superseded
plans is worse than no page.*

---

## What Prototype is, said accurately

A `.pt` file declares a grammar in its header and is then read with it. In
expression mode that grammar is a Pratt parser — leading words, binding powers,
candidates tried longest-first with the cursor restored. In text mode it is a
backtracking search. Either way, **a rule takes the values its children produced
and combines them into its own**, and that value is text.

Which is to say: Prototype is a **bottom-up attribute grammar in which the
attribute type is fixed at text**.

`examples/asm.pt` is the sharpest demonstration of that, because there the text
is not the output language at all — it is *the code that computes the
subexpression*, accumulated upward, and the tool needed no change to carry it.

That sentence is worth keeping, because everything below follows from it. The
attribute is synthesised — it flows up, from the leaves, as the parser reduces.

One thing does flow down, and naming it sharpens the point rather than spoiling
it: the **binding power** a hole is read at is an inherited parameter, set by the
rule above. It is the only one, the tool owns it, and no template can read or
write it. So there are no inherited attributes *a file can use*, and a rule can
see nothing but its own pattern.

## It is not short of grammar

The obvious complaint is that Prototype has no EBNF and therefore does not know
how a language fits together above the level of one rule. That is the wrong
diagnosis, and acting on it would be the most expensive mistake available.

A Pratt grammar is a real grammar. It handles the dangling `else`, right
associativity, mixfix, and prefix-versus-infix on the same word, and it does so
in `examples/pascal.pt` today without a special case anywhere. EBNF is not more
powerful there; it is differently shaped — a *recogniser*, read top-down, where
this is a *rewrite*, read bottom-up.

What is genuinely missing is much narrower, and it turned up four times in one
afternoon of writing Pascal:

- `[ p:name ":" "integer" ]*` is written **twice**, once in `procedure` and once
  in `function`, because there is no way to name a pattern fragment and use it
  in two places.
- The arms of `case` could not be `[ v ":" s ]` in a string template, so
  `examples/pascal.pt` declares an infix `a ":" s` rule meaning *case arm* and
  folds the pair into one value before the group sees it.
- `var a, b: integer` is read by reusing the `,` and `:` operator rules, which
  works and is a structural hack.
- `Result :=` exists because a rule cannot know which function it is inside.

The first three want **named, reusable pattern fragments** — something like

```
@kind params = [ n:name ":" t ]* sep ";"
@syntax "procedure" f:name "(" p:params ")" ";" b => …
```

which is a non-terminal used as a *fragment to splice*, not as a top-down
skeleton. It imposes no parse order and costs the bottom-up character nothing.
**That is the grammar worth adding.** A recogniser grammar over the top is not.

The fourth wants something else, and it is the real story.

## Everything that failed, failed for lack of context

| what could not be done | what it needed |
| --- | --- |
| assign to a function's own name, as standard Pascal returns | knowing which function the rule is inside |
| call a parameterless procedure — `Banner;` | telling a call from a variable read |
| `writeln` with arguments of mixed type | types |
| Pascal's `real` beside its `integer` | types |
| the reach-out half of hygiene ([POSTMORTEM.md](POSTMORTEM.md) 3) | seeing a scope |
| C's `typedef` name against an ordinary identifier | a symbol table |
| Python's blocks | lexer state that survives between tokens |

Seven entries, one sentence: **a rule sees its own pattern and nothing else.**

This is not a defect. It is the property that makes a rule composable, `@use`-able
and language-agnostic, and it is downstream of the premise — a directive
mentions foreign text only inside strings, so a rule is a closed statement about
a shape, with nowhere for a reference to the rest of the program to live.

But it does mean that Prototype as it stands cannot be a serious C or Python
front end, and that no amount of grammar would change it. Adding EBNF to fix
that list would be adding structure to solve a context problem.

## The two directions that follow

If the attribute is text and flows only upward, there are exactly two ways to
grow, and they are the two halves of an attribute grammar.

### 1 · Let what flows up be a value, not only text — **done, and it taught something**

Built on 2026-09-05. `-`, `*`, `/`, `%` and `num(h)` are in, `+` adds when both
sides are numbers and joins when they are not, and `examples/calc.pt` is the
first file here that does not translate its language but runs it:

```
@syntax a "*" b 70 => { emit num(a) * num(b) }
```
```
2 + 3 * 4      ->   14
100 - 7 - 3    ->   90
```

**It reads well**, which was the stated question. `num` on every operand is the
only noise, and it is noise on purpose: a hole holds text, and reading a number
out of text that merely looks like one is the coercion this tool refuses
everywhere else.

**And it is not an interpreter generator, which is what the experiment was
actually worth.** This page said it would be. It is not, and the reason is
structural rather than a missing feature:

```
if 1 then 10 else (1 / 0)      →      pt: '/' by zero
```

A hole is filled by parsing and expanding its subexpression **before** the
template runs. So a rule can *select* between two values that have already been
computed, and cannot leave one uncomputed. **Evaluation is eager and cannot be
otherwise**, which rules out every construct whose meaning is that something does
*not* happen: a short circuit, a loop, a recursion, a definition used before it
runs. What arithmetic bought is exact and is worth naming exactly — an
**evaluator for expressions**, which is a calculator and not an interpreter.

**What the gap actually is, now that it has been measured: deferral.** An
interpreter needs a hole it can hold without running, and a way to run it later.
The shape that would fit is a kind that binds the *source* of its subexpression
rather than the expansion — `text` already does something like this in text mode
— together with a builtin that re-enters the grammar on demand. That is a much
larger idea than arithmetic was: it puts the parser back on the stack from
inside a template, and brings termination questions with it. It is not on
[ROADMAP.md](ROADMAP.md) yet, because nothing has asked for it and this page has
already been wrong once about how near it was.

### 2 · Let a file declare what flows down

The second half is context, and the shape that fits this tool is not a symbol
table the tool imposes but **a store the templates keep**:

```
@syntax "var" [ n:name ":" t ]*  => { for i, x in n { remember("type:" + x, at(t, i)) } … }
@syntax "writeln" "(" x ")"      => { if recall("type:" + x) == "real" { … } }
```

The tool still knows nothing about types, scopes or declarations. The **file**
says what context it needs and builds it as the text is consumed — which is the
bottom-up version of a symbol table, accumulated rather than computed by a pass.
Every row of the table above becomes reachable.

**It is also the one change that compromises rule locality**, which is the thing
holding `@use` together, and it should be built last for that reason. Two used
files writing one key is the same problem two used files declaring one word had,
and it will want the same kind of answer that `override` was.

## Where it sits beside Phoenix and Futamura

| | shape | what it makes |
| --- | --- | --- |
| **Phoenix** | EBNF, top-down, recogniser | a compiler |
| **Futamura** | a description of a machine | a VM, by the first projection |
| **Prototype** | bottom-up attribute grammar, declared in the file that uses it | *see below* |

The answer to *what is left for Prototype* is not a third compiler generator. It
is the thing the other two consume.

**A definitional interpreter is exactly what Futamura's projections operate on.**
Specialise an interpreter with respect to a program and the result is a compiled
program; specialise the specialiser and the result is a compiler. If Prototype
generates interpreters, it does not compete with Futamura — it feeds it, and the
pair covers ground neither does alone: invent a notation in an afternoon, get an
interpreter for it immediately, and project it into a machine when it is worth
keeping.

That is the strongest available answer to *could Prototype go further than the
first projection*. Not by performing projections, which Futamura already does,
but by producing the artefact they need from a notation that did not exist that
morning.

## What to call it — scored 2026-09-05, and the answer moved

**Hans, 2026-09-05:** *a generic interpreter generator, meant to prototype a new
language.* This section was written the same day to record that phrase before it
was true, so that it could be scored rather than drifted into. It was scored
within hours, and it moved, so it is rewritten here rather than added to.

**What held.** *Meant to prototype* is the honest half and the strongest thing
about the tool. One word widened: **notation**, not language — `examples/poem.pt`
turns prose into HTML, which is a notation without an `if` in it, and the broader
word covers the DSL, the config format and the markup too.

**What this section got wrong.** It said the other half was *exactly one roadmap
item* away, and that arithmetic would make it true. Arithmetic landed and it did
not. `examples/calc.pt` runs its language rather than translating it, and stops
precisely at the point where an interpreter begins: a hole is expanded before
the template that uses it runs, so a rule can select between computed values but
cannot leave one uncomputed. `if 1 then 10 else (1 / 0)` divides by zero.
Evaluation is eager, and everything an interpreter needs that a calculator does
not — a short circuit, a loop, a recursion — is a thing that must *not* happen.

So the three descriptions, kept apart, with the middle one newly earned:

| | |
| --- | --- |
| **before 2026-09-05** | a language-agnostic rewriter; the grammar is declared in the header of the file it reads |
| **now** | that, and an **evaluator for expressions** — a `.pt` file can compute rather than emit, bottom-up and eagerly |
| **not yet** | a generic interpreter generator for prototyping notations |

**What the third row now costs is deferral, not an operator.** A hole that can be
held unexpanded and run on demand — see the first of the two directions above.
That is a much larger idea than the one this section originally priced, and no
date is being put on it, because the estimate here has already been wrong once
in the direction of *too near*.

**The phrase is still worth aiming at**, and for the reason it always was: it
completes the set. Phoenix makes a compiler, Futamura makes a machine, Prototype
would make the interpreter, and the section above says why the three then compose
rather than overlap. What has changed is only the distance, and knowing the
distance is worth more than the phrase was.

**The method, though, is the part to keep.** A claim written down before the
evidence, specific enough to be wrong in public, was wrong in public within a
day — and the useful correction was not the one it predicted. It had named
*reading* as the risk; reading was fine, and eagerness was the wall. That is the
ordinary case rather than a surprise, and it is the argument for building the
cheap thing rather than reasoning about it, which is the same argument
[POSTMORTEM.md](POSTMORTEM.md) 9 makes about the staging.

## What it should not become

**A serious C or Python front end.** It plays to every weakness in the table
above — context, types, symbol tables — it duplicates Phoenix, and the honest
answer to "can it read C" is *not without becoming a different tool*. C's hard
part is not its grammar; it is that `x * y` cannot be parsed without knowing
whether `x` is a typedef. Python's hard part is not its grammar either; it is
the lexer. Both are context, and both are the wall.

Reading a *subset* of either, to demonstrate that the notation reaches, is
worth doing and is what the stages in [ROADMAP.md](ROADMAP.md) are for. Claiming
the whole language is not.

## What is actually different about it

One property, and no other tool here has it:

> **The language definition and the program that uses it live in the same file
> and are read in one pass.**

No grammar to compile, no build step, no second tool, no artefact between the
idea and the run. The loop from *what if the syntax were this* to *here is what
it does* is one command and a few seconds. That is what the name says, and it is
what the architecture is genuinely good at: **inventing notations, quickly**.

Every direction above is chosen to sharpen that rather than to trade it away. An
interpreter generator makes the loop shorter still, because the notation runs
instead of merely being translated. Named fragments make a growing grammar
readable. A declared environment is the one that risks it, and is last for that
reason.

## Order, and what would falsify each step

1. ~~**Stage 2, C → assembly.**~~ **Done, 2026-09-05.** The output side
   generalises: a rule's value became the *code that computes* the phrase rather
   than the phrase, and nothing in the tool had to change. Emitting labels and an
   order was not painful; what was painful was writing the same two lines eight
   times, which is now [ROADMAP.md](ROADMAP.md) 1 and is the same *named
   fragments* gap this page predicted from the pattern side.
2. ~~**Arithmetic and `num(h)`.**~~ **Done, 2026-09-05.** It read well, which
   was the test it was given, and it failed a test nobody had written down:
   evaluation is eager, so it makes a calculator and not an interpreter. The
   falsification that mattered was not the one predicted, which is the ordinary
   case and the reason for building the cheap thing first.
3. **Named fragments, of a pattern and of a template.** No longer a prediction:
   stage 1 wanted the first and stage 2 wanted the second, independently, and
   [ROADMAP.md](ROADMAP.md) 1 holds the case. The open question is whether they
   are one mechanic or two — a pattern fragment is spliced at declaration, a
   template fragment is called at expansion and needs arguments and a scope.
   Falsified if the next non-trivial target does not repeat itself, which would
   make the template half a property of code generation rather than of the tool.
4. **A declared environment.** Falsified if it cannot be made safe across `@use`,
   because rule locality is worth more than any single feature it would buy.
