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

### 1 · Let what flows up be a value, not only text

The code template already has integers, booleans and lists; comparison already
works numerically when both sides are already numbers. What it does not have is
arithmetic — `+` concatenates, always — and it has no way to read a hole's text
back as a number. Two small additions and this becomes legal:

```
@syntax a "+" b 60 => { emit num(a) + num(b) }
@syntax a "*" b 70 => { emit num(a) * num(b) }
```

and a `.pt` file stops translating its language and starts **evaluating** it.

That is an interpreter generator, and it is nearly free: the code template is
already an interpreter, with statements, control flow, values and scope. What is
missing is one operator and one conversion. [ROADMAP.md](ROADMAP.md) 1 is the
experiment that decides whether the idea is real, and it is deliberately small,
because the interesting question is not whether it can be built but whether a
file that evaluates reads as well as a file that translates.

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

## What to call it, and the one thing that would make it true

**Hans, 2026-09-05:** *a generic interpreter generator, meant to prototype a new
language.* That is the phrase being aimed at, and it is written down here before
it is true so that it can be scored later rather than drifted into — which is
what [POSTMORTEM.md](POSTMORTEM.md) 9 says a claim has to be shaped like if it
is going to teach anything.

**Half of it is true now.** *Meant to prototype* is the honest part and the
strongest thing about the tool; the only word worth widening is the last one.
**Notation**, not language: `examples/poem.pt` turns prose into HTML, which is a
notation and not a language, and the broader word covers the DSL, the config
format and the markup as well as the thing with an `if` in it.

**The other half is not true yet, and the gap is exactly one roadmap item.**
Today a `.pt` file emits *text*: it generates translators, and nothing it
produces runs. Calling it an interpreter generator would be describing
[ROADMAP.md](ROADMAP.md) 1 as though it had happened. What today's tool is, said
without flattery, is what `CLAUDE.md` already says — **a language-agnostic
rewriter whose grammar is declared in the file that uses it**.

So the two descriptions, kept apart on purpose:

| | |
| --- | --- |
| **now** | a language-agnostic rewriter; the grammar is declared in the header of the file it reads |
| **if [ROADMAP.md](ROADMAP.md) 1 lands** | a generic interpreter generator for prototyping notations |

**What would make the second one true is arithmetic and `num(h)`** — one
operator and one conversion, both small. And the phrase is worth aiming at for a
reason beyond accuracy: it completes the set. Phoenix makes a compiler, Futamura
makes a machine, Prototype would make the interpreter, and the section above
says why the three then compose rather than overlap.

**What would falsify it** is not the build. It is the reading: if
`=> { emit num(a) + num(b) }` turns out to be a rule nobody wants to write, then
evaluation wants a spelling of its own and the phrase is premature for a second
reason as well as the first. When that is known, this section is rewritten to
say which it was.

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

1. **Stage 2, C → assembly** ([ROADMAP.md](ROADMAP.md) Stages). Before any of
   this, because it is the honest test of whether the *output* side generalises
   past targets shaped like the input. If emitting labels and an order is
   painful, that is worth learning before deciding what Prototype is.
2. **Arithmetic and `num(h)`.** Cheap. Falsified if a file that evaluates reads
   worse than one that translates — which is a question about how it reads, and
   so has to be tried rather than argued.
3. **`@kind`, named pattern fragments.** Falsified if the duplication it removes
   turns out to be rarer than stage 1 suggested.
4. **A declared environment.** Falsified if it cannot be made safe across `@use`,
   because rule locality is worth more than any single feature it would buy.
