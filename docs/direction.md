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
  folds the pair into one value before the group sees it. **A parameter list
  whose types differ is the same limitation with no workaround**: the two lists
  have to come back apart in the output, so folding cannot help, and that file
  writes `int k` for a parameter declared `real` — recorded, and pinned by
  `tests/pascal.sh`.
- `var a, b: integer` is read by reusing the `,` and `:` operator rules, which
  works and is a structural hack.
- `Result :=` exists because a rule cannot know which function it is inside.

A fifth instance arrived from stage 2, in a code generator rather than a
grammar: `examples/asm.pt` wrote the same two lines at eight operands, and
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

But it does mean that Prototype as it stands cannot be a serious C or Python
front end, and that no amount of grammar would change it. Adding EBNF to fix
that list would be adding structure to solve a context problem.

## The two directions that follow

If the attribute is text and flows only upward, there are exactly two ways to
grow, and they are the two halves of an attribute grammar.

### 1 · Let what flows up be a value — **done, and it stopped short of the name**

Built 2026-09-05: `-`, `*`, `/`, `%`, `num(h)`, and a `+` that adds when both
sides are numbers and joins when they are not. `examples/calc.pt` is the first
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

## What to call it — the description scored, the name still open

*This section is about two things that were run together for a day and are
better kept apart: **the description** — what sentence introduces the tool — and
**the name**. The description is below and was scored. The name is at the foot,
was surveyed on the same day, and is **open**.*

### The description

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

### The name — surveyed 2026-09-05, and open

**Hans, 2026-09-05:** *so how about the name? Is it used as a programming tool
already?*

`README.md` has said **Working name** since the first commit, so nothing has to
be undone here — only chosen. The evidence is
[prior-art.md](prior-art.md) § 1; this is what it costs and what it buys.

**What is genuinely wrong with it is searchability, and nothing else.** There is
**no language tool, compiler, parser generator or transformation system named
Prototype** — the survey looked and found none, so this tool would not be
confused for a peer. What it collides with is a dormant JavaScript framework
that still holds the domain and the org, and — the larger problem — a word that
in a programming context already means five other things: C's function
prototypes, JavaScript's `Object.prototype`, the object model of Self and Io,
the GoF pattern, and Spring's bean scope. *Prototype parser* and *prototype
grammar* return the paradigm. And `.pt` is PyTorch's checkpoint extension, which
a reader meets before they meet the name at all.

**The field's own habit says the same thing from the other side.** Stratego,
Spoofax, Rascal, Ohm, Comby, Katahdin, Coccinelle, Seed7, Nanopass; TXL, ANTLR,
SDF, DMS, META II. Almost nothing here is named after a common word from its own
domain, and the exceptions carry a qualifier that does the work — ast-grep,
Semgrep, OpenRewrite, tree-sitter. A tool in this field is found by its name and
by nothing else, because there is no catalogue to browse.

**What the name buys, and it is not nothing.** It states the purpose, and the
purpose is the half of the description above that **held** when the other half
was scored and failed. *Meant to prototype a notation* is the strongest true
sentence about this tool, and a name that says it is worth more than a name that
merely indexes well. The section above spent a day learning that the honest half
is the half to keep; renaming to something opaque would be trading the one word
that is already honest for one that has to be explained.

**So the decision is genuinely balanced, and it is not being taken here.** What
is written down instead is the shape of it, so that whoever takes it is not
re-deriving the evidence:

- **Keeping it costs discoverability and nothing else.** Nothing is
  functionally blocked: crates.io is free, and npm and PyPI hold stubs rather
  than live projects. A qualified form — the way the field's own exceptions are
  built — recovers most of the search profile while keeping the word that is
  true.
- **Changing it costs the one honest word**, and the replacement has to earn
  back what *prototype* says for free. An opaque name is a promise to explain
  the tool every time it is mentioned.
- **`.pt` is separable from the name** and may be the cheaper half to change,
  since the PyTorch collision is the one a reader actually hits and the
  extension is spelled in every example, test and document here.

#### The shortlist — three candidates, checked 2026-09-05

**Three findings narrowed the field more than any individual candidate did**, and
they are worth more than the list itself:

1. **Every real word in the semantic field is already taken.** *argot* is an
   option parser, *idiolect* is three GitHub projects, *maquette* is a virtual
   DOM library, *hapax* is an AI-infrastructure tool, and *rubric* and
   *cartouche* are occupied on all three registries. That is not bad luck. It is
   why this field's tools are called Stratego, Spoofax, Comby, Katahdin,
   Coccinelle and Rascal: **borrowing from outside the domain is not taste, it is
   the only thing that works.**
2. **A near-word is worse than a far-word.** `foretext` is free on every registry
   and was nearly the recommendation — the header *is* the text before the text.
   It sits one letter from `pretext`, a JavaScript text-layout library, and from
   **PreTeXt**, an authoring language for textbooks. A name misheard as an
   existing tool **in the same domain** is worse than one nobody knows yet.
3. **A name with meaning has to come from outside software entirely, or it has
   to be effectively invented — there is no third option.** Four rounds of
   candidates produced this and it is the most useful thing on the page. The
   current name, then the `pro`/`syn`/`lingua` morphemes, then `genesis`: every
   meaning-bearing word in or near this domain came back occupied, and the three
   that were free on every registry were killed by what they *return* —
   `foretext` by PreTeXt, `genlex` by **OCaml's `Genlex` module**, a generic
   lexer parameterised by your language's keywords, and the whole `-lingua`
   branch by **Lingua Franca**, a published coordination language whose pitch is
   *polyglot*. [prior-art.md](prior-art.md) § 1 has all three branches with what
   closed each. **A clean registry sweep is not evidence of anything.**
4. **A descriptive name has to be scored, and this page has already shown what
   that costs.** The description above was written and refuted inside a day. A
   name that encodes the current stage inherits exactly that. **Which is the real
   case against `Prototype`, and it has nothing to do with searchability:** it
   means *the small model made before the real thing*, and the third row of the
   table above is aiming at a tool that makes real interpreters. The name would
   have to be abandoned precisely when the project succeeded.

| | | why | against it |
| --- | --- | --- | --- |
| **Metaxis** — `mx`, `.mx` | **recommended** | Greek **μεταξύ**, Plato's *in-betweenness*; in Boal's theatre, **"the state of belonging completely and simultaneously to two different autonomous worlds."** This page's own corrected claim is that *neither the language read nor the language written is the tool's own* — **the name is that sentence in one word.** Free on all three registries, returns nothing in software, and `meta-` primes toward metacompilation while `-xis` echoes *syntaxis*, so it reads native to the field without asserting anything in it | `meta-` is a loaded prefix in 2026 and invites a company association it does not mean. Two spellings exist in the literature, *metaxis* and *metaxy* |
| **Schorre** — `schorre`, `.m2` | strong second, and a **citation** | D. V. Schorre, META II, 1964. [prior-art.md](prior-art.md) § 2 establishes with evidence that META II is **this tool's premise, both halves**: a quoted string is a literal input token, a bare name is a nonterminal, output is emitted from inside quotes. Naming it Schorre is an attribution rather than a label, **and it matches the house**: Futamura is already a tool named for the person whose idea it instantiates. Primes toward metacompilers, which is exactly right | **nobody knows how to pronounce it.** A name people must be told how to say is a tax the other three do not charge |
| **Landin** — `landin`, `.iswim` | third | Peter Landin, and three connections where Schorre has one: *"The Next 700 Programming Languages"* is a schema instantiated per problem, which is the pitch; **the off-side rule is his**, and `@separator "\n" indent` implements it; and he coined **syntactic sugar**, which is what a rule here produces. Unambiguous to say | primes toward functional programming — ISWIM, SECD, the ML lineage — which this tool is not. Precise priming is most of what a name is for here |
| **Ostracon** — `ost`, `.ost` | fourth | A potsherd: the cheap, throwaway surface drafts were written on because papyrus was expensive. **The surface is cheap, not the result** — the honest half of the description without the word | eight letters, needs a sentence of explanation, taken on npm, and it still faintly implies *draft*, which is the door this section is trying to keep open |
| **Protaxis** | **withdrawn** | was recommended for scanning as `pro-` + `taxis` | **and its etymology is `prot-` + `axis`, a geology term from 1890** — the appealing story was wrong, and `metaxis` is the same sound with a meaning that is real and exactly right. Kept here as a correction, not an option |
| **Incip** | fifth | free on all three registries, five letters, and an *incipit* is the opening that identifies a manuscript | reads as a truncation, looks like a typo for *incipient* |
| **Maquette** | **declined** | the best pure synonym for *prototype* | which is the objection: it hard-codes *preliminary* and closes the same door, only more elegantly. Also a virtual DOM library |

**The two at the top are the two kinds of answer**, and choosing between them is
one question: *should the name carry a meaning, or carry an attribution?*
**Metaxis** names what the tool **is** — a thing belonging wholly to two
languages, neither of them its own. **Schorre** names who got there **first**,
and matches the house: Proto, Phoenix, Futamura and Solveig are a myth, a
scientist and a literary figure, and **`Prototype` is the only descriptive name
among them.**

**And one pool is closed rather than unlucky.** Eight mythological candidates —
`thoth`, `seshat`, `ogma`, `nabu`, `kvasir`, `bragi`, `ratatoskr`, `proteus` —
are taken on all three registries, eight for eight, and the ones with the best
meanings are in-domain disasters: **Babel** is the JavaScript compiler,
**Hermes** is Meta's JavaScript engine, **Janus** is a reversible programming
language *and* the WebRTC server, **Odin** is a programming language. Surnames
are the opposite: four for four free, and used nowhere as product names. Phoenix
already holds the myth slot in the house in any case.

Everything tried across five rounds that is not in the table above landed on
somebody, and the branches that closed — `-lingua`, `proto-`, `genesis`, myth —
are recorded with their reasons in [prior-art.md](prior-art.md) § 1, dated, so
that reopening this decision is choosing rather than searching.

**And `.pt` should change whichever way this goes.** It is PyTorch's checkpoint
extension, it is the collision a reader meets before they meet the name, and it
is spelled in every example, test and document here — separable from the naming
question, more expensive to change, and more likely to be worth it.

**And there is nothing to build either way, which is why this is not on
[ROADMAP.md](ROADMAP.md).** A name is settled by deciding, not by working, and
the page that holds it should be this one. When it is decided, this subsection
is rewritten rather than added to, as the rest of the page is.

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
annotations because nothing here can infer one. `examples/python.pt` says all
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
and arm64 on Tuesday. A Prototype directive means *this shape becomes that text*
and nothing more, which is why one tool reads `examples/poem.pt` and
`examples/asm.pt`.

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
