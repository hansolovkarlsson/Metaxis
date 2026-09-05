# Prior art

*What else does this, what it has that Prototype does not, and which of those
are worth wanting. This page is a **survey**, not a plan: it describes tools
that exist and scores Prototype against them, and where it names a feature it
says who has it, what it would cost here, and — the part that decides
everything — **whether anything in this tree has asked for it**.
[ROADMAP.md](ROADMAP.md) is what to build next and only holds items with a
customer; [direction.md](direction.md) is the theory. This page feeds both and
outranks neither. Where it disagrees with the code, the code is right.*

*Written 2026-09-05. Every tool below was checked against its own documentation
or paper, and the sources are at the foot.*

---

## 1 · The neighbourhood, in three families

Nothing surveyed is the same tool. The useful thing is that they fail to be
the same tool in **three different directions**, and Prototype sits inside two
of the three at once, which is the whole of what is unusual about it.

| family | what a tool in it is | examples |
| --- | --- | --- |
| **A · Codemod** | a fixed grammar per language, shipped with the tool; you write a *pattern in that language* with holes | Comby, ast-grep, Semgrep, GritQL, OpenRewrite, Coccinelle, `gofmt -r` |
| **B · Transformation system** | you supply a grammar as a separate artefact, then write rules over the trees it builds | TXL, Stratego/Spoofax, Rascal, DMS, ANTLR + StringTemplate, JastAdd, Silver, Nanopass |
| **C · Extensible language** | one host language whose own syntax the program may add to, in the file, as it is read | Racket `#lang`, Seed7, Coq `Notation`, Prolog `op/3`, Katahdin, SugarJ, Raku slangs, Lisp macros |

**A** is language-agnostic in delivery and not in kind: Comby matches any of
thirty languages because it ships a small delimiter description for each, and
it cannot read a notation invented this morning. **B** can read anything and
charges a build step and a second artefact for it. **C** puts the declaration in
the file, which is Prototype's shape exactly — and every member of C declares
syntax **for its own host language**, so the thing being read and the thing
being written are the same language, and the tool knows it.

Prototype is C's placement with B's agnosticism: **the declaration is in the
file, and neither the input language nor the output language is the tool's.**
That combination is what did not turn up anywhere in this survey.

### And what they are called

Worth noticing while the list is in front of us, because this project's name is
recorded as a working one and the field has an unmistakable habit.

**Almost none of these tools is named after a common word from its own domain.**
The invented or borrowed-from-elsewhere names — **Stratego**, **Spoofax**,
**Rascal**, **Ohm**, **Comby**, **Katahdin**, **Coccinelle**, **Silver**,
**Seed7**, **Nanopass** — and the acronyms — **TXL**, **ANTLR**, **SDF**,
**SmPL**, **DMS**, **META II** — cover the whole survey between them. The
handful of exceptions are compounds that carry a qualifier doing the
disambiguating: **ast-grep**, **Semgrep**, **GritQL**, **OpenRewrite**,
**JastAdd**, **SugarJ**, **tree-sitter**.

The habit is not fashion. A tool in this field is found by its name and by
nothing else — there is no category listing to browse — so the name has to be
a term that returns the tool when typed into a search box. Coccinelle means
*ladybird*; that is exactly why it works.

**Against that, `Prototype` scores poorly, and it is worth being specific about
how.**

| | |
| --- | --- |
| **the framework** | [Prototype.js](http://prototypejs.org/), Rails' default through 2007. Last stable 1.7.3, September 2015; roughly 0.6% of websites in 2021. Dormant, and it holds `prototypejs.org`, the `prototypejs/prototype` GitHub org, and the mindshare |
| **the word, in this field** | C's function prototypes; JavaScript's `Object.prototype`; the prototype-based object model of Self and Io; the GoF Prototype pattern; Spring's `prototype` bean scope. **In a programming context the word almost never denotes a tool**, so *prototype parser*, *prototype grammar* and *prototype compiler* each return the paradigm |
| **the registries** | `prototype` is taken on **npm** (v0.0.5, *"Implementation of Prototypejs in Node.js"*) and on **PyPI** (v0.2, *"Javascript's Prototyping OO for Python"*) — both stubs rather than live projects. **On crates.io it is free**, checked 2026-09-05 |
| **the extension** | `.pt` is PyTorch's checkpoint extension, which `torch.save` writes. That is the more common meaning today by a wide margin, and it is the collision a reader meets before they meet the name |

**And the thing that is *not* wrong with it, which the first pass got backwards.**
There is **no language tool, compiler, parser generator or transformation
system named Prototype**. The collision is with a dormant JavaScript library and
with an overloaded English word — not with a peer. Nothing here would be
confused for another tool in this survey; it would be hard to *find*, which is a
different and smaller problem.

### What the alternatives cost, checked 2026-09-05

Occupancy on **crates.io**, **npm** and **PyPI**, and what each name already
returns when searched. Registry occupancy is the weaker signal of the two —
`prototype` is "taken" on npm and PyPI by two stubs — so the last column is the
one that decides. **Three separate candidates were free on all three registries
and killed by the search**: `foretext`, `genlex` and the whole `-lingua` branch.
A clean registry sweep is not evidence of anything.

| name | crates.io | npm | PyPI | what it already returns |
| --- | --- | --- | --- | --- |
| `prototype` | free | stub, v0.0.5 | stub, v0.2 | the JS framework, and five meanings of the word |
| `argot` | taken | taken | taken | an option parser for the Neut language — **the same domain** |
| `idiolect` | free | free | taken | three GitHub projects, one of them topical |
| `maquette` | free | taken | taken | a virtual DOM library |
| `hapax` | free | taken | taken | an AI-infrastructure tool |
| `rubric` | taken | taken | taken | grading rubrics, edtech |
| `cartouche` | taken | taken | taken | nothing prominent |
| `foretext` | **free** | **free** | **free** | nothing — **but one letter from `pretext`, a JS text-layout library, and from PreTeXt, an authoring language.** Ruled out for that |
| **`incip`** | **free** | **free** | **free** | nothing |
| **`ostracon`** | **free** | taken | **free** | nothing |
| `uncial` | free | taken | free | nothing prominent |

**Three further branches were tried and all three closed**, and the reason each
one closed is worth more than the names were. Registry occupancy did not catch a
single one of them — every name in this table was free where it mattered, and
each was killed by what it *returns*.

| branch | candidates | crates / npm / PyPI | what closed it |
| --- | --- | --- | --- |
| **`-lingua`** | `prolingua`, `protolingua`, `synlingua` | free · free · free | [**Lingua Franca**](https://www.lf-lang.org/), an active reactor-oriented coordination language from Berkeley and TU Dresden whose tagline is *"intuitive concurrent programming in any language"* — **a published language project whose pitch is polyglot**, which is next door in meaning to *language-agnostic* |
| **`proto-`** | `protoglot`, `protax`, `protogenesis` | free except `protoglot` on npm | not a collision but a **claim**: `proto-` keeps the assertion that this makes the model *before* the real thing, which is the deep objection to the current name and survives renaming |
| **`genesis`** | `genesis`, `genesys` | taken · taken · taken | four active software meanings — a financial-markets low-code platform, the WordPress parent-theme framework, the Genesys contact-centre company, and the console. **More contested than `prototype`, which has one** |
| | `genlex` | free · free · free | **OCaml's `Genlex` stdlib module**: *"a generic lexical analyzer… parameterized by the set of keywords of your language."* Not merely the same domain — **nearly the same function** as `prototype/src/lex.c` |
| | `gensyn` | free · free · taken | Gensyn, a funded ML-compute company |
| | `genera` | free · taken · taken | **Symbolics Genera**, the Lisp Machine OS |
| **survivor** | **`protaxis`** | **free · free · free** | nothing in software. A real dictionary word — New Latin `prot-` + `axis`, a geology term from 1890 for *the line of initial uplift in a mountain range*. **Its apparent etymology is wrong and worth recording as wrong**: it is not `pro-` + `taxis`, though it scans that way to anyone who knows `syntax` is `syn-` + `taxis`, which is why it sounds native to this field while asserting nothing in it |

**And the conclusion the four rounds actually produced**, which is worth more
than any candidate on this page:

> **A name with meaning has to come from outside software entirely, or it has to
> be effectively invented. There is no third option.**

Four rounds reached for a meaningful word — the current name, then the
`pro`/`syn`/`lingua` morphemes, then `genesis` — and every meaning-bearing word
in or near this domain came back occupied. That is the same statement as the one
below, arrived at from the other side.

**What the table says, more than any row does: every real word in this semantic
field is already occupied.** That is the mechanism behind the habit above rather
than a coincidence — a tool here is found by its name, the near words are gone,
and so the field reaches outside its own domain and takes a board game, a
mountain or a ladybird.

[direction.md](direction.md)'s "What to call it" carries what that costs, what
the name buys in exchange, and a shortlist of three. This page holds only the
evidence.

## 2 · The uniqueness claim, scored

[direction.md](direction.md) closes with:

> **One property, and no other tool here has it: the language definition and
> the program that uses it live in the same file and are read in one pass.**

**As written, that is false, and four tools falsify it.**

| | what it does | |
| --- | --- | --- |
| **Seed7** | `$ syntax expr: .while.().do.().end.while is -> 25;` — a statement's syntax, its keywords, its holes and its **priority**, declared in the program, usable on the next line | the closest single line in this survey to `@syntax … 25` |
| **Coq `Notation`** | `Notation "'IF' c1 'then' c2 'else' c3" := (…) (at level 200, right associativity).` — quoted words, bare holes, a level, an associativity | quoting *and* levels, both, in a directive |
| **Prolog `op/3`** | `:- op(700, xfx, ===).` — precedence and fixity declared in the source file, in force for the rest of it | 1972 |
| **Katahdin** | the running program modifies the grammar that defines its own syntax and semantics; new statements are usable on the next line | the same file, and not even one pass — it is at runtime |

SugarJ adds a fifth with a caveat: the sugar is declared in a *library* and
imported, but the import affects the parse of everything after it **in the
importing file**, which is the same self-applicative property. Racket's `#lang`
is a sixth with a bigger caveat: the line is in the file, the language it names
is not.

**What survives, and it is the sentence worth keeping:**

> The language definition and the program that uses it live in the same file
> and are read in one pass — **and neither the language read nor the language
> written is the tool's own.**

Seed7 declares Seed7's syntax. Coq declares notation for Coq terms. Prolog
declares operators over Prolog terms. Katahdin extends Katahdin. Every one of
them needs a host semantics for the declaration to mean anything, which is why
none is a *rewriter* and none can be pointed at Pascal on Monday and arm64 on
Tuesday. Prototype's directive means nothing but *this shape becomes that text*,
which is why the same tool reads `examples/poem.pt` and `examples/asm.pt`.

**That is a smaller claim and a true one.** It is also the more interesting
one, because it says what the tool is *for* rather than what it is first at.

### The ancestor, and it is 1964

**META II** (Schorre, 1964) wrote its rules like this:

```
expr = term $( '+' term .OUT('ADD') / '-' term .OUT('SUB') );
```

A quoted string is a literal token of the input. A bare name is a nonterminal.
Output is emitted from inside `.OUT(' … ')` — quoted. **That is Prototype's
premise, both halves of it, sixty-two years early**, on a machine with 8K of
six-bit memory. TREE-META added *unparse rules*: patterns over the tree it had
built, each with an output template.

The convergence is worth writing down rather than hiding. What Prototype has
that META II did not is the level number in the directive rather than a rule
per precedence tier, holes named and spliced rather than an output stack, and
the declaration living in the same file as the thing it reads. What META II had
that Prototype does not is **alternation** (`/`), which is [ROADMAP.md](ROADMAP.md) 8,
and which every tool in this survey has.

## 3 · What they have that this does not

Nine features, each with who has it, what it would cost here, and whether
anything in this tree has asked. **Four have a customer already in the tree;
five do not, and say so.**

---

### 3.1 A second backend without a second grammar — *has a customer, and it is measured*

**Who has it.** Ohm separates a grammar from its semantics entirely, so one
grammar carries many semantics objects. ANTLR + StringTemplate makes the same
split its selling point: *"a translator may be retargeted by swapping in a new
view (set of templates) — all without recompiling the translator."*

**The customer is in this repository and it is 272 lines.**
`examples/code.pt` opens by saying so: *"The body below is character for
character the body of pascal.pt. Every rule is the same rule. The only
difference is that a template here is `=> { … }` rather than `=> "…"`."* Two
files, 280 and 272 lines, differing only after the arrow. Every pattern, every
level, every `@token`, every group is written twice, and a fix to the Pascal
grammar has to be made twice or the diff stops meaning what it is for.

**`@use` does not solve it and cannot.** A used file holds directives and
nothing else, so `pascal.pt` cannot be used — it has a body. And `override`
re-declares the *whole rule*, pattern included (§3.10), so overriding a
template means writing the pattern again, which is the thing being avoided.

**The shape that fits.** A rule carrying more than one template, tagged, with
one chosen at the command line:

```
@syntax a "*" b 70
    => "({a} * {b})"                        as plain
    => { emit group(a, 70) + " * " + group(b, 71) }   as tight
```

`pt -b tight examples/pascal.pt`. The tag namespace is new; nothing else is,
and the flag letters here are illustrative — `-b` and `-t` below are simply two
that `pt` has not spent.

**And it makes the demonstration better rather than worse** — today the
argument for the code template is a diff between two files that a reader has to
be told are identical; then it is a diff between two runs of one file, and the
identity is a fact rather than a claim.

**What would falsify it.** If the two template sets turn out to want different
*patterns* often enough that the sharing is a lie. This tree is the evidence
against that: they want the same patterns exactly, and the file says so.

---

### 3.2 A guard on a rule — *architecturally free, customer is thin*

**Who has it.** Everything in family A and most of B. Comby: `where :[a] ==
:[b], :[a] != "x == 500"`. TXL: `where` conditions on a rule. ast-grep:
`constraints:`. Coccinelle: metavariable constraints, including a predicate
written in OCaml or Python.

**What it is.** A condition evaluated *during* matching, whose failure rejects
the candidate rather than producing wrong output. Prototype has nothing of the
kind: a code template runs **after** the match is committed, and its only exit
is `emit`. There is no way to write *this rule applies only when the hole looks
like this.*

**It would cost almost nothing.** The expression parser already tries
candidates under one leading word longest-first with the token cursor restored
on failure. A guard that returns false is one more way for a candidate to fail,
and the restore path is written. The expression language for the guard is the
code template's, already parsed and already checked at the `@syntax` that wrote
it.

**The honest part.** Asked for the customer, this tree offers two and neither
is strong. `writeln` wanting one rule per argument type
([ROADMAP.md](ROADMAP.md) 1) would collapse into one rule with a guard on the
literal's shape — but two rules already work and are readable. Telling a `case`
arm from a declaration — `1: writeln(11)` against `mod: integer` — reads like a
guard's job and is really a types problem, and a guard on the spelling would
paper over the wall rather than reach it.

**So: cheapest strong feature on this page, and nothing has asked.** It is
written down because the *fit* is unusually good and somebody costing it later
should not have to rediscover that the backtracking is already there.

---

### 3.3 An island rule — text this file has no rule for passes through

*Strongest finding on this page, and the one that changes what the tool can be
pointed at.*

**Who has it.** TXL calls it **agile parsing**: the effective grammar is a base
grammar plus explicit *overrides*, and *"grammar overrides are used to express
robust parsing, and to avoid errors or exceptions due to input not explained by
the grammar."* **Island grammars** are the limit case — describe only the
constructs you care about, treat everything else as water. Comby is island
parsing by construction: it knows brackets, strings and comments, and nothing
else about any of its thirty languages, which is exactly why it works on all of
them.

**Prototype already has this, on one side of the tool.** `@mode text` *"scans
the body, fires a rule where one matches, and copies everything else through"*
(§3.6). That is an island grammar, and `examples/poem.pt` is one. Expression
mode is the opposite by construction: anything unmatched is an error.

**What it would buy is the thing [direction.md](direction.md) says must not be
attempted.** That page's "What it should not become" declines a serious C or
Python front end, correctly, because *"the honest answer to 'can it read C' is
not without becoming a different tool"*. An island rule dissolves the question:
**you never need the whole language if the rest passes through.** A `.pt` file
declares the twenty constructs it wants to rewrite, and everything else in the
file arrives at the output unchanged. That is what every tool in family A does
for a living, and it is the reason they are useful on real code while family B
needs a complete grammar first.

It is also the honest version of what the stages have been doing by hand.
`examples/python.pt` reads a subset and *says so in its closing note*; the
subset is enforced by the input being written to fit. With an island rule the
subset is enforced by the tool, and what falls outside it is visible in the
output instead of stopping the run.

**What it costs, and it is not small.** Text mode has nothing to resume into —
it is a scan. Expression mode has a parse in progress, so the question is
where a skipped span *ends* and what the surrounding rule is handed for it.
Three shapes, none obviously right:

- **skip a token** — the smallest thing that works, and it makes
  `no rule reads '@' here` into `'@'` appearing in the output. Cheap, and wrong
  wherever the unread thing is a construct rather than a character.
- **skip to the separator** — a statement nothing reads passes through whole.
  Matches how `stmts` already works, and gives the useful unit.
- **a declared fallback rule**, `@syntax fallback => "{...}"`, so the file says
  what happens rather than the tool deciding.

**And one thing it would break.** Expression mode's completeness is a
*feature*: `no rule reads X here` is how a grammar under construction tells you
it is incomplete. An island mode must be opted into per file, never a default,
or every grammar bug becomes silent output. The shape of the opt-in is the
decision, and it is the same decision `@mode` already exists to make.

---

### 3.4 Collection attributes — the composable form of a declared environment

*This is the survey's answer to [direction.md](direction.md)'s open question,
and it is a better-shaped answer than the one that page proposes.*

**The question.** direction.md's second direction is *"let a file declare what
flows down"*, sketched as a store the templates keep — `remember("type:" + x,
t)` and `recall(…)`. That page names its own problem: *"It is also the one
change that compromises rule locality, which is the thing holding `@use`
together… Two used files writing one key is the same problem two used files
declaring one word had, and it will want the same kind of answer that
`override` was."*

**The known answer.** JastAdd and Silver both have **collection attributes**: a
node *contributes* a value to a named collection, contributions may come from
arbitrarily far away in the tree, and the collection is the aggregate of all of
them. JastAdd's tutorial pairs it with *broadcasting* and calls it *"the
synthesizing counterpart"* — values propagating **up** rather than down.

**Why that is the right shape here and a key/value store is not.** A store is
*assignment*: two writers to one key conflict, order decides, and `@use`'s
whole guarantee — that reading two files cannot make one declaration mean
something else — is gone. A collection is *union*: contributions are unordered
and additive, two used files contributing to one collection is not a conflict
but the intended case, and **there is nothing for `override` to be needed for.**
Rule locality survives, because a rule still says only *what it contributes*,
never what anyone else did.

It is also the direction the tool already flows. [direction.md](direction.md)
establishes that Prototype is a bottom-up attribute grammar with the attribute
fixed at text; a collection attribute is *still synthesised*, still upward, and
still needs no rule to see another rule. **It is the one context mechanism that
does not want the thing this tool refuses to have.**

**A customer, and it is small.** `examples/code.pt` emits `#include <stdio.h>`
unconditionally, because nothing can say *this file needs stdio because a
`writeln` fired*. Under a collection the `writeln` rule contributes the include,
the program rule splices the aggregate, and a program with no output gets no
include. `examples/asm.pt` has the same shape for a data section, and so does
every code generator that ever wanted a prologue determined by its body.

**What it costs, named honestly.** The aggregate is not known until the last
contribution is made, and the rule that wants to splice it usually runs
**first** — a program's head is expanded before its body. So the splice cannot
be a splice; it has to be a placeholder resolved after expansion, which is a
second pass over the output text and a new thing in a tool that has one pass.
That is the real price, it is not hidden by any of the systems above (they have
a whole tree in memory and Prototype does not), and it is what would have to be
designed before this is a roadmap item rather than a note.

**What would falsify it.** If the placeholder pass turns out to need to know
anything about the output language — where it is safe to substitute, how to
indent it — then the agnosticism is spent and the feature is not worth it.

---

### 3.5 Layout-aware splicing — cheap, and stage 3's mirror image

**Who has it.** StringTemplate indents a spliced multi-line value to the column
of its splice point, as a documented property rather than a convenience.
Rascal's string templates use the margin the same way. SDF3 goes furthest:
*"template productions … consider the whitespace surrounding symbols when
deriving a pretty-printer"*, so one syntax definition yields both a parser and
a formatter.

**Where it bites here.** Every nested output in `examples/` is indented by hand
with `\n` and literal spaces inside the template, and the indentation is wrong
as soon as a rule nests inside another — the inner template does not know how
deep it was spliced. `examples/code.pt`'s C output and `examples/asm.pt`'s
listing both pay this.

**And the joke is worth stating.** Stage 3 taught the *lexer* to read
indentation — `@separator "\n" indent`, a stack of columns, the `block` kind.
The templates still cannot write it. The tool understands indentation in
exactly one direction.

**Cost: small, and the spelling is the whole decision.** The mechanic is *"a
splice of a multi-line value indents every line after the first to the column
the splice began at"*. It must be opt-in, because it changes recorded output
everywhere. `{~t}` already established that a sigil inside the braces is how a
splice says it is special, so there is a position for it.

---

### 3.6 Bidirectional rules — one declaration that reads *and* writes

**Who has it.** Coq: *"The command `Notation` has an effect both on the Coq
parser and on the Coq printer."* One declaration, two directions. Isabelle's
syntax translations are the same idea. SDF3's template productions generate the
pretty-printer from the grammar.

**What it would buy, and it is a test rather than a feature.** If a rule could
run backwards, a `.pt` file would come with a formatter for the notation it
declares — and, more valuably, with the strongest test a grammar can be given:
**print then re-read, and check you got the same tree.** This tree's tests are
already unusually serious about this — four of five scripts run what they
produced rather than diffing it — and a round-trip check is the same instinct
applied to the grammar itself rather than to the translation.

**Why it is declined, at least as stated.** A Prototype template is a string of
foreign text. Running it backwards means *parsing the output language*, which
the tool by construction does not know — it knows the input grammar and nothing
about the target. Bidirectionality would be available only for the degenerate
case where output and input are the same language, which is the case Prototype
is least interested in.

**But it explains something.** `examples/pascal.out`'s parenthesis noise is
recorded as *"the cost of agnosticism showing itself"*, and that is right, but
the sharper name is available: it is a **pretty-printing** problem, and every
system that solved it did so by knowing the *output* language's precedences.
`group(h, n)` in the code template is Prototype's version and it is honest
about its limit — it asks the operand what level the **input** grammar parsed it
at, which is the right answer exactly when the two languages agree about
precedence and a plausible-looking wrong one when they do not.

---

### 3.7 A parse trace — *serves the one thing the tool is good at*

**Who has it.** The Ohm editor visualises the parse interactively as you type
the grammar. ANTLR has `-trace` and a parse-tree GUI. Spoofax gives you an
editor for the language as you define it. Every tool in family B treats
*watching the parse* as part of the job, because a grammar under construction
is the normal state.

**Prototype has `-g`**, which prints the grammar the header built, and nothing
that prints the parse it attempted. When expression mode says `no rule reads
'X' here`, it does not say which candidates were tried, in what order, or how
far each got before the cursor was restored. Candidates ordered longest-first
with backtracking is precisely the mechanism that cannot be reasoned about from
the outside.

**This is the feature most in line with what [direction.md](direction.md)
claims the tool is for** — *"inventing notations, quickly"*, the loop from
*what if the syntax were this* to *here is what it does*. A wrong guess about a
level or a candidate order is the commonest way that loop stalls, and there is
currently no instrument for it.

**Cost: small.** `pt -t` printing each candidate tried, its pattern, and the
token it failed on, indented by depth. **And it is two features for one price**:
[ROADMAP.md](ROADMAP.md) 3 wants a *measurement* of expression-mode
backtracking before a budget is picked, and says so — *"a budget picked without
one is a number somebody made up."* A trace with a counter is that instrument.

---

### 3.8 An editor mode emitted from the header — *nobody has asked*

**Who has it.** Spoofax's central pitch: *"With SDF3, it is possible to
modularly describe a language's syntax, generating a parser, a pretty printer,
and basic editor features such as syntactic code completion and syntax
highlighting."* One definition, several artefacts.

**`pt -g` already proves the information is there** — token classes as regexes,
comment delimiters, and the complete word list, which is precisely a syntax
highlighter's input. `pt --highlight` emitting a `.tmLanguage` or a Vim syntax
file is a couple of hundred lines of no new thinking, and it makes a notation
invented five minutes ago legible in an editor.

**No customer.** It is a pleasure rather than a need, and this tree's rule is
that a surface does not grow without one. Filed, not proposed.

---

### 3.9 More than one error per run — *small, no customer*

ANTLR's error recovery reports every error in a file rather than the first.
`pt` stops at the first. For a grammar under construction that is the wrong
number, and for everything else it is right. Nothing has asked; `tests/errors.sh`
covers 69 cases one at a time and is not made worse by the current behaviour.

---

## 4 · What Prototype has that they do not

The comparison is only worth having if it runs both ways.

**Against family A** (Comby, ast-grep, Semgrep, GritQL, Coccinelle,
OpenRewrite): they cannot read a notation that did not exist this morning. Every
one of them matches *fragments of a language whose grammar the tool ships* —
Comby's thirty delimiter descriptions, tree-sitter's grammar set, Coccinelle's
C. Their patterns are code with holes punched in it, and there is no code to
punch holes in until somebody has written a grammar for the language. Prototype
declares the language in the file and then reads it. **`examples/tour.pt` is
not expressible in any tool in family A**, because the language it reads has
no grammar anywhere.

**Against family B** (TXL, Stratego, Rascal, DMS, ANTLR, Spoofax): they can all
do everything on this page and much more, and they charge a *project* for it —
a grammar file, a generator, a build step, an artefact between the idea and the
run. Spoofax will give you an IDE for your language; it will not give you one in
a single command with no files but the one you are editing. The loop length is
the entire difference and it is not a small one.

**Against family C** (Seed7, Coq, Prolog, Katahdin, SugarJ, Racket): the
declaration means *this shape becomes that text* and nothing else, so there is
no host semantics to satisfy and the output language is free. None of them can
be pointed at Pascal on Monday and arm64 on Tuesday, because their declarations
are about their own terms.

**And one property nobody else here has at all:** the premise that
[notation.md](notation.md) argues — that a directive mentions foreign text
**only inside strings**, on both sides of the arrow. Stratego needed explicit
quotation brackets `|[ … ]|` and antiquotation `~x` and a paper on
*disambiguating* the result. TXL, Rascal and SDF put the object grammar in a
separate artefact partly so the two vocabularies cannot collide. Comby spends
`:[…]` and hopes. Prototype spends the one delimiter every reader already
agrees on and gets `for` with two `;` inside its pattern in a file whose body
ends statements with `;`, with nothing to disambiguate.

## 5 · Declined

**Whole-tree, in-place operation.** Comby, ast-grep, OpenRewrite and
`spatch -dir` all run over a codebase and edit files where they sit; that is
what a codemod is. Prototype is one file that carries its own grammar, and the
unit is right: a `.pt` file *is* the grammar, so "run it over a tree" would mean
running one invented notation over files that are not written in it. The tools
that do this can only do it because the grammar belongs to the tool. **This is
the axis Prototype is not competing on, and it should stay that way.**

**A recogniser grammar over the top.** EBNF, a start symbol, a top-down parse.
[direction.md](direction.md) already refuses this and gives the reason —
*"Adding EBNF to fix that list would be adding structure to solve a context
problem"* — and the survey supports it rather than complicating it: the systems
that took the EBNF route are family B, and they took the build step with it.

**CFG-path matching.** Coccinelle's `...` matches *along the control flow
graph*, so a rule can say "this call, then eventually that one, with no
intervening free". It is the most powerful matching construct surveyed and it
requires a CFG, which requires knowing the language's semantics. Out of reach
by construction, and correctly so.

## 6 · The shortlist

Ordered by *has a customer* first and cost second, which is this tree's own
rule. **Nothing here is a roadmap item yet**; the four with customers are the
ones that could become one without inventing a reason.

| | | customer | cost |
| --- | --- | --- | --- |
| 1 | **A second backend without a second grammar**, `pt -b` (§3.1) | `examples/code.pt`, 272 duplicated lines, stated in the file | small — a tag namespace and a flag |
| 2 | **A parse trace, `pt -t`** (§3.7) | grammar-under-construction; and [ROADMAP.md](ROADMAP.md) 3 wants the measurement | small |
| 3 | **Layout-aware splicing** (§3.5) | every nested output in `examples/` | small, and the spelling is the decision |
| 4 | **Collection attributes** (§3.4) | `#include <stdio.h>` emitted unconditionally in `code.pt` | medium — wants a second pass over the output, which the tool does not have |
| 5 | **An island rule** (§3.3) | none yet, and it would change what the tool can be pointed at | medium, and it can make grammar bugs silent |
| 6 | **A guard on a rule** (§3.2) | thin — two candidates, neither strong | very small; the backtracking is already there |
| 7 | Editor mode from the header (§3.8) | none | small |
| 8 | More than one error per run (§3.9) | none | small |
| — | Bidirectional rules (§3.6) | — | declined: wants the output language's grammar |

**And one correction rather than a feature:** [direction.md](direction.md)'s
uniqueness claim is falsified as stated and true in the narrower form §2 gives.

---

## Sources

- TXL — [Cordy, *The TXL source transformation language*](https://www.sciencedirect.com/science/article/pii/S0167642306000669); [*TXL – A Language for Programming Language Tools and Applications*](https://research.cs.queensu.ca/home/cordy/Papers/Cordy_TXL_LDTA04.pdf); [Dean, Cordy & Malton, *Agile Parsing in TXL*](https://www.cs.usask.ca/faculty/kas/Publications_files/JASE_AP.pdf)
- Stratego / Spoofax — [Concrete Object Syntax](http://www.metaborg.org/en/latest/source/langdev/meta/lang/stratego/strategoxt/11-concrete-object-syntax.html); [SDF3 Overview](https://spoofax.dev/references/sdf3/); [SDF3 Templates](https://spoofax.dev/references/syntax/templates/)
- Rascal — [Concrete Syntax](https://www.rascal-mpl.org/docs/Rascal/Expressions/ConcreteSyntax/); [Concrete Patterns](https://www.rascal-mpl.org/docs/Rascal/Patterns/Concrete/); [Visit](https://www.rascal-mpl.org/docs/Rascal/Expressions/Visit/)
- Comby — [The Basics](https://comby.dev/docs/basic-usage); [Advanced usage and rules](https://comby.dev/docs/advanced-usage)
- ast-grep — [Introduction](https://ast-grep.github.io/guide/introduction); [Core Concepts](https://ast-grep.github.io/advanced/core-concepts)
- GritQL — [biomejs/gritql](https://github.com/biomejs/gritql); [docs.grit.io](https://docs.grit.io/)
- OpenRewrite — [openrewrite/rewrite](https://github.com/openrewrite/rewrite)
- Coccinelle — [Semantic Patches](https://coccinelle.gitlabpages.inria.fr/website/sp.html); [The SmPL Grammar](https://coccinelle.gitlabpages.inria.fr/website/docs/main_grammar.html)
- Ohm — [Introduction](https://ohmjs.org/docs/intro); [Syntax Reference](https://ohmjs.org/docs/syntax-reference)
- ANTLR / StringTemplate — [Motivation](https://github.com/antlr/stringtemplate4/blob/master/doc/motivation.md); [Language Translation Using ANTLR and StringTemplate](https://theantlrguy.atlassian.net/wiki/spaces/ST/pages/1409118/Language+Translation+Using+ANTLR+and+StringTemplate)
- JastAdd — [Hedin, *An Introductory Tutorial on JastAdd Attribute Grammars*](https://link.springer.com/chapter/10.1007/978-3-642-18023-1_4); [*Patterns for Name Analysis and Type Analysis with JastAdd*](https://arxiv.org/pdf/2002.01842)
- Silver / ableC — [Silver: an Extensible Attribute Grammar System](https://www.sciencedirect.com/science/article/pii/S0167642309001099); [MELT group](https://melt.cs.umn.edu/silver/)
- Nanopass — [nanopass.org](https://nanopass.org/); [user guide](https://github.com/nanopass/nanopass-framework-scheme/blob/main/doc/user-guide.stex)
- META II / TREE-META — [Schorre, *META II: A Syntax-Oriented Compiler Writing Language* (1964)](https://ibm-1401.info/Meta-II-schorre.pdf); [META II](https://en.wikipedia.org/wiki/META_II); [TREE-META](https://en.wikipedia.org/wiki/TREE-META)
- Seed7 — [Structured syntax definition](https://seed7.net/manual/syntax.htm)
- Coq / Rocq — [Syntax extensions and notation scopes](https://rocq-prover.org/doc/V8.18.0/refman/user-extensions/syntax-extensions.html)
- Katahdin — [*A Programming Language Where the Syntax and Semantics Are Mutable at Runtime*](https://www.researchgate.net/publication/281455032_A_Programming_Language_Where_the_Syntax_and_Semantics_Are_Mutable_at_Runtime)
- SugarJ — [*SugarJ: Library-based Syntactic Language Extensibility*](https://www.pl.informatik.uni-mainz.de/files/2019/04/sugarj.pdf)
- Racket — [The Reader](https://docs.racket-lang.org/reference/reader.html); [Beautiful Racket: Syntax objects](https://beautifulracket.com/explainer/syntax-objects.html)
