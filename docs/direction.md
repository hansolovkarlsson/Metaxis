# Direction

*Where this could go, and why — the theory of the tool rather than the list of
jobs. [ROADMAP.md](ROADMAP.md) says what to build next and what would settle it;
this page says what the next things are **for**, and which futures are being
declined. It is the one record here allowed to be speculative, and every claim
in it that could be checked against the code has been. When a direction is taken
or abandoned, it is rewritten rather than appended to: a page of superseded
plans is worse than no page.*

---

## What Metaxis is, said accurately

A `.mx` file declares a grammar in its header and is then read with it. In
expression mode that grammar is a Pratt parser — leading words, binding powers,
candidates tried longest-first with the cursor restored. In text mode it is a
backtracking search. Either way, **a rule takes the values its children produced
and combines them into its own**, and that value is text.

Which is to say: Metaxis is a **bottom-up attribute grammar in which the
attribute type is fixed at text**.

`examples/asm.mx` is the sharpest demonstration of that, because there the text
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

The obvious complaint is that Metaxis has no EBNF and therefore does not know
how a language fits together above the level of one rule. That is the wrong
diagnosis, and acting on it would be the most expensive mistake available.

A Pratt grammar is a real grammar. It handles the dangling `else`, right
associativity, mixfix, and prefix-versus-infix on the same word, and it does so
in `examples/pascal.mx` today without a special case anywhere. EBNF is not more
powerful there; it is differently shaped — a *recogniser*, read top-down, where
this is a *rewrite*, read bottom-up.

What is genuinely missing is much narrower, and it turned up four times in one
afternoon of writing Pascal:

- `[ p:name ":" "integer" ]*` is written **twice**, once in `procedure` and once
  in `function`, because there is no way to name a pattern fragment and use it
  in two places.
- The arms of `case` could not be `[ v ":" s ]` in a string template, so
  `examples/pascal.mx` declares an infix `a ":" s` rule meaning *case arm* and
  folds the pair into one value before the group sees it. **A parameter list
  whose types differ is the same limitation with no workaround**: the two lists
  have to come back apart in the output, so folding cannot help, and that file
  writes `int k` for a parameter declared `real` — recorded, and pinned by
  `tests/pascal.sh`.
- `var a, b: integer` is read by reusing the `,` and `:` operator rules, which
  works and is a structural hack.
- `Result :=` exists because a rule cannot know which function it is inside.

A fifth instance arrived from stage 2, in a code generator rather than a
grammar: `examples/asm.mx` wrote the same two lines at eight operands, and
`@template` now names them. **The common cause is that this tool could name a
rule and nothing else** — and both halves of that are now built.

The pattern-shaped half is `@fragment`, spliced with `@name`:

```
@fragment params = "(" [ p:name ":" "integer" ]* sep ";" ")"
@syntax "procedure" f:name @params ";" b => …
```

which is a non-terminal used as a *fragment to splice*, not as a top-down
skeleton. It imposes no parse order and costs the bottom-up character nothing.
**That was the grammar worth adding.** A recogniser grammar over the top is not,
and nothing since has argued otherwise.

**This page sketched it as `@kind params = …`, spliced as `p:params`, and that
was wrong in a way worth keeping.** A kind says what *one hole* holds; a
fragment says what *sequence of elements* goes here, and it arrives carrying
holes of its own. Putting both in the slot after a `:` would have been one
position meaning two unrelated things — the same mistake that splitting
`@template` from this one had just avoided, one level down. The name went with
the spelling: `@fragment` is what this page's own prose had been calling it all
along.

So of the five instances, the fifth is answered by `@template` and the first
three by `@fragment`. **The fourth wants something else, and it is the real
story.**

## Everything that failed, failed for lack of context

| what could not be done | what it needed |
| --- | --- |
| assign to a function's own name, as standard Pascal returns | knowing which function the rule is inside |
| call a parameterless procedure — `Banner;` | telling a call from a variable read |
| `writeln` with arguments of mixed type | types |
| Pascal's `real` beside its `integer` | types |
| the reach-out half of hygiene ([POSTMORTEM.md](POSTMORTEM.md) 3) | seeing a scope |
| C's `typedef` name against an ordinary identifier | a symbol table |
| ~~Python's blocks~~ — **built 2026-09-05** | lexer state that survives between tokens |

Seven entries, one sentence: **a rule sees its own pattern and nothing else.**

**The last one has been struck, and how it was struck is the interesting part.**
Python's blocks were built, and the rule that reads one still sees nothing but
its own pattern. The state went *below* the rules, into the lexer, where
`@separator … indent` put it: the lexer already survives between tokens, so
giving it a stack of columns asked nothing of the property this page is about.
That is the shape of the exception, and it is narrow — **the six that remain all
want state that flows between rules, and no amount of lexer will produce it.**
It is worth knowing which side of that line a wish falls on before costing it.

This is not a defect. It is the property that makes a rule composable, `@use`-able
and language-agnostic, and it is downstream of the premise — a directive
mentions foreign text only inside strings, so a rule is a closed statement about
a shape, with nowhere for a reference to the rest of the program to live.

But it does mean that Metaxis as it stands cannot be a serious C or Python
front end, and that no amount of grammar would change it. Adding EBNF to fix
that list would be adding structure to solve a context problem.

## The two directions that follow

If the attribute is text and flows only upward, there are exactly two ways to
grow, and they are the two halves of an attribute grammar.

### 1 · Let what flows up be a value — **done, and it stopped short of the name**

Built 2026-09-05: `-`, `*`, `/`, `%`, `num(h)`, and a `+` that adds when both
sides are numbers and joins when they are not. `examples/calc.mx` is the first
file here that runs its notation instead of translating it. It reads well, and
**it is a calculator and not an interpreter**, because a hole is expanded before
the template that uses it runs — so a rule can select between computed values and
cannot leave one uncomputed. `if 1 then 10 else (1 / 0)` divides by zero.

**What remains is deferral, not an operator**: a hole that can be held
unexpanded and run on demand. The shape that would fit is a kind binding the
*source* of its subexpression rather than the expansion, plus a builtin that
re-enters the grammar. That puts the parser back on the stack from inside a
template and brings termination with it, so it is a larger idea than this was,
and it is deliberately not on [ROADMAP.md](ROADMAP.md): nothing has asked, and
this page has already been wrong once about how near it was.

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
| **Metaxis** | bottom-up attribute grammar, declared in the file that uses it | *see below* |

The answer to *what is left for Metaxis* is not a third compiler generator. It
is the thing the other two consume.

**A definitional interpreter is exactly what Futamura's projections operate on.**
Specialise an interpreter with respect to a program and the result is a compiled
program; specialise the specialiser and the result is a compiler. If Metaxis
generates interpreters, it does not compete with Futamura — it feeds it, and the
pair covers ground neither does alone: invent a notation in an afternoon, get an
interpreter for it immediately, and project it into a machine when it is worth
keeping.

That is the strongest available answer to *could Metaxis go further than the
first projection*. Not by performing projections, which Futamura already does,
but by producing the artefact they need from a notation that did not exist that
morning.

## What to call it — the description scored, the name chosen

*This section is about two things that were run together for a day and are
better kept apart: **the description** — what sentence introduces the tool — and
**the name**. The description is below and was scored. The name is at the foot,
was surveyed on the same day, and was **decided**.*

### The description

**Hans, 2026-09-05:** *a generic interpreter generator, meant to prototype a new
language.* This section was written the same day to record that phrase before it
was true, so that it could be scored rather than drifted into. It was scored
within hours, and it moved, so it is rewritten here rather than added to.

**What held.** *Meant to prototype* is the honest half and the strongest thing
about the tool. One word widened: **notation**, not language — `examples/poem.mx`
turns prose into HTML, which is a notation without an `if` in it, and the broader
word covers the DSL, the config format and the markup too.

**What this section got wrong.** It said the other half was *exactly one roadmap
item* away, and that arithmetic would make it true. Arithmetic landed and it did
not. `examples/calc.mx` runs its language rather than translating it, and stops
precisely at the point where an interpreter begins: a hole is expanded before
the template that uses it runs, so a rule can select between computed values but
cannot leave one uncomputed. `if 1 then 10 else (1 / 0)` divides by zero.
Evaluation is eager, and everything an interpreter needs that a calculator does
not — a short circuit, a loop, a recursion — is a thing that must *not* happen.

So the three descriptions, kept apart, with the middle one newly earned:

| | |
| --- | --- |
| **before 2026-09-05** | a language-agnostic rewriter; the grammar is declared in the header of the file it reads |
| **now** | that, and an **evaluator for expressions** — a `.mx` file can compute rather than emit, bottom-up and eagerly |
| **not yet** | a generic interpreter generator for prototyping notations |

**What the third row now costs is deferral, not an operator.** A hole that can be
held unexpanded and run on demand — see the first of the two directions above.
That is a much larger idea than the one this section originally priced, and no
date is being put on it, because the estimate here has already been wrong once
in the direction of *too near*.

**The phrase is still worth aiming at**, and for the reason it always was: it
completes the set. Phoenix makes a compiler, Futamura makes a machine, Metaxis
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

### The name — **decided 2026-09-05: Metaxis**

**Hans, 2026-09-05:** *rename it to Metaxis.* And, on reading it back:
*Meta as in meta-compiler, Taxis as in order, metaxis as in between. It fits.*

The tool was called **Prototype** for its first two days and is now **Metaxis**,
`mx` on the command line, `.mx` on a file. `README.md` had said *working name*
since the first commit, so nothing had to be undone — only chosen. This
subsection is rewritten rather than added to, as this page requires of itself;
the evidence for every candidate, and the four branches that closed, are in
[prior-art.md](prior-art.md) § 1, dated, so that none of it needs re-deriving.

**Why this one.** Greek **μεταξύ** — Plato's *in-betweenness*, and in Augusto
Boal's theatre *"the state of belonging completely and simultaneously to two
different autonomous worlds."* The property this page states as the tool's own
is that **neither the language read nor the language written is the tool's own**:
a thing belonging wholly to two languages and owning neither. **The name is that
sentence in one word**, and the sentence was written some hours before the name
was proposed by someone who was not looking at it.

**It carries three readings and only one of them is the dictionary's**, which is
worth writing down in that order rather than blurring them:

| reading | | |
| --- | --- | --- |
| **between** | μεταξύ, *metaxú* | **the dictionary one.** Plato, then Voegelin, then Boal |
| **meta-** | as in *metacompiler* | true of the tool, and what a programmer sees first |
| **-taxis** | as in *order*, *arrangement* | **constructed, and productive.** `syntax` really is `syn-` + `taxis`, so a reader in this field will parse `metaxis` the same way and be right about the tool. But μεταξύ is not `meta` + `taxis`, and this reading is apt rather than etymological |

That distinction is kept because the round before it was lost. `Protaxis` was
recommended here for *scanning* as `pro-` + `taxis`; its actual derivation is
`prot-` + `axis`, a geology term from 1890, and the reason had been built after
the sound was liked. Metaxis survives the same test: **the meaning is real, the
third reading is labelled as constructed, and the name does not depend on it.**

**What it beat, in one line each.** `Schorre` — a citation, since META II is this
tool's premise both halves ([prior-art.md](prior-art.md) § 2), and it matches the
house, but nobody can pronounce it. `Landin` — the 700 languages, the off-side
rule, *syntactic sugar*; primes toward functional programming, which this is not.
`Ostracon` — a cheap surface for drafts; still faintly implies *draft*.
`Maquette` — declined for closing the same door `Prototype` did.

**And the door that closed is the real reason the old name had to go.** The
searchability was the loud problem — a dormant JavaScript framework, five other
meanings of the word in this field, and `.pt` already being PyTorch's checkpoint
extension. The deep problem was that **`Prototype` means the small model made
before the real thing**, and the third row of the table above aims at a tool that
makes real interpreters: the name was a claim that would have had to be abandoned
precisely when the project succeeded. `Metaxis` asserts nothing that can go
stale.

**What the rename touched, and what it deliberately did not.** The code, the
build, the examples, the tests and the present-tense records all say Metaxis and
`.mx`. **The work journal, `CHANGELOG.md` and `POSTMORTEM.md` were left alone**,
because they are dated accounts of what happened and the tool really was called
Prototype when they were written — rewriting them would make yesterday's entries
claim a name that did not exist. [work-journal/README.md](work-journal/README.md)
says so at the top.

**The containing folder followed on 2026-09-06.** For a day after everything
inside it said Metaxis the working tree itself was still called `Prototype`; the
GitHub repository was already `Metaxis`, so the directory was the last
disagreement left, and the only one a reader meets before opening anything. It
is now `Metaxis` too. Nothing in the build, the tests or the records reads its
own path, so the rename was a `mv` and this paragraph — `make check` passes
unchanged from the new location.

## What it should not become

**A serious C or Python front end.** It plays to every weakness in the table
above — context, types, symbol tables — it duplicates Phoenix, and the honest
answer to "can it read C" is *not without becoming a different tool*. C's hard
part is not its grammar; it is that `x * y` cannot be parsed without knowing
whether `x` is a typedef. Python's hard part is not its grammar either; it is
the lexer. Both are context, and both are the wall.

**Stage 3 walked up to that wall from the Python side and stopped where it
should.** The lexer *was* the hard part and the lexer is what got the state, so
blocks read. What did not follow, and was not made to: `elif` is a rule per arm
count, a wrapped call is not read at all, and the C types come off Python's own
annotations because nothing here can infer one. `examples/python.mx` says all
three in its own closing note. That is what "a subset, and it says so" looks
like in practice, and it is the difference between reaching and claiming.

Reading a *subset* of either, to demonstrate that the notation reaches, is
worth doing and is what the stages in [ROADMAP.md](ROADMAP.md) are for. Claiming
the whole language is not.

## What is actually different about it — **rewritten 2026-09-05, second pass**

*This section used to read "One property, and no other tool here has it: the
language definition and the program that uses it live in the same file and are
read in one pass." That claim was surveyed against the tools that actually
exist and **four of them falsify it** — Seed7, Coq's `Notation`, Prolog's
`op/3` and Katahdin all put a syntax declaration in the file and use it on the
next line, and Prolog has done so since 1972. The survey, the evidence and what
survives are [prior-art.md](prior-art.md) § 2. The property is restated below
with the clause that was missing, and the smaller claim is the more interesting
one, because it says what the tool is for rather than what it is first at.*

One property, and it is a conjunction rather than a single fact:

> **The language definition and the program that uses it live in the same file
> and are read in one pass — and neither the language read nor the language
> written is the tool's own.**

Every tool that shares the first half declares syntax **for its own host
language**, so a declaration means something only against that host's semantics.
Seed7 declares Seed7. Coq declares notation for Coq terms. Katahdin extends
Katahdin. None of them is a rewriter and none can be pointed at Pascal on Monday
and arm64 on Tuesday. A Metaxis directive means *this shape becomes that text*
and nothing more, which is why one tool reads `examples/poem.mx` and
`examples/asm.mx`.

And what follows from the first half is unchanged, because it was never the
part in dispute: no grammar to compile, no build step, no second tool, no
artefact between the idea and the run. The loop from *what if the syntax were this* to *here is what
it does* is one command and a few seconds. That is what the name says, and it is
what the architecture is genuinely good at: **inventing notations, quickly**.

Every direction above is chosen to sharpen that rather than to trade it away. An
interpreter generator makes the loop shorter still, because the notation runs
instead of merely being translated. Named fragments make a growing grammar
readable. A declared environment is the one that risks it, and is last for that
reason.

## Order, and what would falsify each step

*Rewritten 2026-09-05, second pass. Three of the four steps below were taken the
same day this page was written; what is kept here is what each one settled, in
one line, because a list of struck-through plans is the thing this page opens by
refusing to become. The full accounts are in
[COMPLETED.md](COMPLETED.md), and what the three wrong estimates taught is
[POSTMORTEM.md](POSTMORTEM.md) 11.*

**Taken.** Stage 2 showed the output side generalises and cost nothing in the
tool. Arithmetic and `num(h)` made an evaluator for expressions and not an
interpreter. `@template` named a piece of template and, in doing so, settled that
naming a *pattern* fragment is a second mechanic rather than the same one.
`@fragment` then built that second mechanic, and refuted this page's spelling
for it on the way.

**Taken since.** Stage 3 was the first delimiter the *tool* owns rather than one
a string declares, and the notation it wanted was the decision. It went to a
`block` kind over two synthetic quoted words, on the ground that a string
naming an indent would be quoting text the source does not contain — the
premise kept in letter and spent in meaning. The three stages are done.

**Next, and what would falsify each:**

1. **A declared environment.** Falsified if it cannot be made safe across `@use`,
   because rule locality is worth more than any single feature it would buy.
   Still last, and still the one that would change what this tool is.

**And the standing lesson from the three that were taken:** this page was right
about every shape and wrong about every distance. Structure can be reasoned
about from the code; distance is what you find on the way. Where an entry here
names a number of steps, read it as a guess and build the cheapest thing that
tests it.

**Stage 3 is the one estimate on this page that came in right, and it is not a
counter-example — it is the method.** Its distances were not reasoned about
either; they were *measured*, by rehearsing the whole feature by hand before a
line of it was written. [POSTMORTEM.md](POSTMORTEM.md) 15 has the account. The
sentence above stands, with one clause added: **build the cheapest thing that
tests it, and then write the plan.**
