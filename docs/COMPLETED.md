# Completed

*What was **built**, and why, in enough detail to be worth reading a year from
now. [ROADMAP.md](ROADMAP.md) is the other half of the ledger and holds what is
not built; an entry is moved between the two and never copied. Each entry ends
with the state it was verified at, so a claim here can be checked rather than
taken. What a thing **costs** is not here — that is
[notation.md](notation.md)'s "What it costs", and it is written down rather than
argued away.*

Newest first.

## Pascal's `real`, and the first parameter list whose types differ

```
procedure Scale(n: integer; k: real);

examples/code.out     void Scale(int n, double k)
examples/pascal.out   void Scale(int n, int k)      <- recorded, and wrong
```

**No code changed.** This is entirely in the examples, and it is here because it
is the first thing to put weight on `@fragment` beyond the case it was built
for.

**A type became a rule of its own** — `@syntax "real" => "double"`, a word
alone. That is what lets a parameter list hold a *hole* where the type goes, so
the fragment is `[ p:name ":" t ]* sep ";"` with **two** holes, and the template
walks them in step with `for i, x in p` and `at(t, i)`. `@fragment` was argued on
2026-09-05 against a spelling that would have capped a fragment at one hole; this
is the customer that would have broken it, arriving the same day.

**The declaration cannot use the hole, and that is a finding.** `a ":" t` reads
every `case` arm as a declaration: `1: writeln(11)` and `mod: integer` are both
`expr ":" expr` and nothing tells them apart without knowing what the left side
is. So declarations keep one quoted rule per type and the parameter list gets
the hole — legal only because inside `"(" … ")"` after a procedure name there is
no arm to confuse it with. It is the same context wall as `writeln`, `Banner;`
and C's `typedef`, reached from the type side.

**The string template writes the wrong type, on purpose.** `join ", int "` puts
one word in front of every turn and cannot vary it, and a string template
splices each list joined with no way to interleave two — the same limitation the
`case` arms met, but with no workaround available, because the two lists have to
come back apart in the output. So `examples/pascal.pt` emits `int k` for a
parameter declared `real`: output that compiles, links, runs and is wrong.

**It is recorded and pinned.** `examples/pascal.out` carries it and
`tests/pascal.sh` fails if it ever stops being true, the same device as that
file's `it''s` literal and `tests/hygiene.sh`'s `bump: 105 0`. The two failures
are different in kind and the file's closing note now says so: the literal is
something the notation *cannot express* and announces itself at the compiler;
the type is something it expresses *wrongly and quietly*, and only the second
kind needs writing down.

**The program body is still shared.** Both files read the same Pascal, so every
difference between the two recorded outputs is still caused by the template form
alone — which is what makes that diff an argument rather than a comparison of
two programs. Keeping that was the reason for choosing this over letting the
bodies diverge.

**What it did not do.** `writeln(k)` is not written, because printing a `real`
would need the rule to know its argument's type. That is [ROADMAP.md](ROADMAP.md)
1's remaining decision and it is left there, marked, rather than faked.

Verified at 12 examples, 62 error cases, `tests/hygiene.sh`, `tests/asm.sh`, and
`tests/pascal.sh` compiling and running the C to `4 44 80 7 42`.

## A class named after a kind is refused

```
@token expr "…"    ->    'expr' is a kind, so a class called that could never
                         be used -- 'x:expr' is read as the kind and this class
                         would never be consulted
```

`expr`, `stmts` and `text` are the built-in kinds, and a hole's kind is resolved
against those names first. So a class called one of them was declared, accepted,
and never consulted: `@token expr` produced no complaint at all and a rule using
`x:expr` quietly read an expression, and `@token text` surfaced as `a 'text'
hole belongs to @mode text` — an error about the wrong thing, naming a mode the
file may never have mentioned.

**Refusing the name at `@token` is the whole fix**, because those are the only
two namespaces that meet. A `@fragment` is spliced with `@name` and shares a
namespace with neither, which is why this item stayed three names and two lines
rather than growing when fragments arrived — see the entry below for why that
was the right side to put it on.

Found while writing a Python example whose string class was called `text`.

Verified at 12 examples byte-identical, 62 error cases, `tests/hygiene.sh`,
`tests/pascal.sh` and `tests/asm.sh`, clean build with no warnings.

## `@fragment`: the other half of naming a fragment

```
@fragment params = "(" [ p:name ":" "integer" ]* sep ";" ")"

@syntax "procedure" f:name @params ";" b               => { … }
@syntax "function"  f:name @params ":" "integer" ";" b => { … }
```

`examples/pascal.pt` and `examples/code.pt` each wrote that parameter list
twice, once in `procedure` and once in `function`, because this tool could name
a rule and a piece of template and nothing else. Each now writes it once, and
both files expand byte-identically to what they expanded to before — which is
what makes it a refactor rather than a rewrite.

**It is spliced, not called, and that is the design decision.** `@template`
named a piece of template on the same day and the open question was whether
these were one mechanic or two. They are two. A template is *called* at
expansion, takes arguments, has a scope and can recurse; a fragment is copied
into the pattern *at declaration* and has none of those. By the time any rule is
matched, its elements are indistinguishable from ones written out by hand — the
rule is checked, sealed, matched and clashed as one pattern, and nothing
downstream of the header knows a fragment was ever involved.

**It brings its own holes**, which is why the two rules above splice `{p}`
without declaring `p`. That is also the argument for the spelling. The obvious
alternative was to write it where a hole's kind goes — `p:params`, which is what
[direction.md](direction.md) had sketched and what ROADMAP called `@kind`. A
*kind* says what one hole holds; a fragment says what sequence of elements goes
here. Putting both in one syntactic slot would have been the same mistake that
splitting `@template` from this one had just avoided: one position meaning two
unrelated things. So a splice is `@name`, in a namespace of its own, and a
`@fragment` and a `@token` class may share a name without either shadowing the
other.

**It must be declared before it is spliced.** That is what `@token` asks of a
class used as a kind, and it buys two things: a fragment cannot splice itself,
so no cycle is expressible and no depth guard is needed, and there is no order
in which a file could have meant something else. A fragment may splice another
fragment, since that one is already declared.

**`override` sits before the `=`**, which is the opposite of every other
directive and is forced rather than chosen. Everywhere else the word goes last,
because that is the one place a bare word cannot be part of the thing being
declared. A fragment's pattern runs to the end of the directive, so there is no
such place after it: a trailing `override` would be read as a hole called
`override` — precisely the silent misreading the word exists to prevent.

**What it cost was one new refusal, and not the one expected.** A pattern that
declares one hole name twice is now an error. `bind_put` fills the first hole it
finds by name, so the second could never be reached; writing that by hand was
always a mistake and nothing in the tree had ever written one, so nothing
refused it. Splicing one fragment twice into a rule is two of every hole it
declares, which makes the mistake easy to make by accident — so the check landed
with the feature that asked for it, and it refuses the hand-written case too.

Verified at 12 examples byte-identical, 59 error cases, `tests/hygiene.sh`,
`tests/pascal.sh` and `tests/asm.sh`, clean build with no warnings.

## `@template`: the first thing besides a rule that can be named

```
@template load(x) {
    if level(x) == 1000 { emit "\tmov x0, #" + x + "\n…" } else { emit x }
}

@syntax a "+" b 60 => { load(a) load(b) emit "\tadd x0, x0, x1\n" }
```

`examples/asm.pt` had written that test out at every operand — eight times,
identically — because this tool could name a rule and nothing else. It now
writes it once.

**Called as a statement, and that is the design decision.** A template emits
into whatever called it rather than returning a value, because `emit` already
writes to one place and a return would have needed a second mechanism for
nothing. So a template call is a statement form, a builtin call stays an
expression, and the two mistakes that invites each get their own message rather
than a shared confusing one: `'level' is a builtin and gives a value — put it in
an 'emit'`, and `'t' is a template — it is called as a statement`.

**Its body sees its parameters and nothing else.** Not the caller's holes. That
is what makes it readable on its own and checkable where it is written, and it
is the difference between a named fragment and a macro. `'y' is not one of this
template's parameters` is the whole of the scoping rule.

**Calls resolve at seal**, for the third time this week and for the same reason
the class-kind check and the collision rule do: a rule may call a template
declared after it, or one that arrived through `@use`, and the order a file
writes its directives in cannot change the answer.

**What it actually bought, measured rather than asserted.** The rules section of
`examples/asm.pt` went from 2458 bytes to 1473 — a little under half — and
*gained three lines*, because eight copies of a hundred-character test became
eight calls of eight characters plus a three-line declaration. The expansion is
byte-identical to what the file emitted before, which is the proof that the
refactor is one: `diff` against the recorded output is empty.

**Two things found while building it.** A statement that is a bare word with no
`(` is not assumed to be a call — saying which statement forms exist is more use
than guessing at the one meant, so that message is repinned rather than
narrowed. And parameters were capped at 8 in the evaluator with a silent
truncation, which surfaced as `nothing here is called 'i'` for a parameter that
was plainly declared; it is refused at declaration now.

*Verified at 12 examples, 48 error cases, `tests/hygiene.sh`, `tests/pascal.sh`
and `tests/asm.sh`; `make check` clean.*

## Stage 2: C in, arm64 assembly out, assembled and run

`examples/asm.pt` reads a C subset and emits arm64 that `tests/asm.sh`
assembles, links against a four-line runtime and runs on the CPU it is written
for. `2 + 3 * 4` prints 14, `1 < 2 ? 10 : 20` prints 10, and the conditional
jumps to labels the input never mentions.

**The question stage 2 existed to answer was whether the output side
generalises past targets shaped like the input, and it does.** A Pascal
expression becomes a C expression one node at a time — the output nests where
the input nested. Assembly does not nest: it is a sequence, and an expression is
a run of instructions leaving a value somewhere agreed. So the value a rule
passes up stopped being *the phrase* and became *the code that computes the
phrase*, and **nothing in the tool had to change for that**. It is the same
bottom-up combining it always was, which is the strongest confirmation
[direction.md](direction.md)'s framing has had.

The discipline is a stack machine: every rule's output is code that leaves
exactly one value pushed, so a binary operator emits its left operand's code,
then its right operand's, then pops two and pushes one. No register survives a
rule, which is what keeps the rules composable at all.

**Two things had to be worked around, and both are findings.**

**A literal is not code.** `3` is a bare token, a rule cannot match a bare token,
so nothing turns it into `mov x0, #3`. What distinguishes a literal from a
subexpression is `level(h)`: every rule in the file declares a level, so
`level(h) == 1000` means *nothing here produced this*. That works, and it is the
same limitation `examples/pascal.pt` meets when it cannot translate a string
literal — met from the opposite direction, at the leaves of a code generator
instead of inside a call.

**A label is needed twice**, at the branch and at the place it jumps to, and
`fresh("L")` returned a different name on every call. That is
[POSTMORTEM.md](POSTMORTEM.md) 10: the reference had said `fresh` and `{~t}` were
the same thing, `{~t}` was exercised by two examples and a compile-and-run test,
`fresh` had never been used by any file in the repository, and the unexercised
half had drifted. It now keeps the same per-application memo `subst` keeps.

**And it cost one repetition eight times over**, which is [ROADMAP.md](ROADMAP.md)
1: the two lines that load an operand are written once per operand of every rule,
identically, because nothing here can name a piece of template. direction.md had
already predicted that *named reusable fragments* was the structural gap, giving
the pattern-side example; stage 2 arrived at the same gap from the template side
without looking for it.

*Verified at 12 examples, 42 error cases, `tests/hygiene.sh`, `tests/pascal.sh`
and `tests/asm.sh`; `make check` clean.*

## Arithmetic, `num(h)`, and a file that runs its language instead of writing it

`-`, `*`, `/` and `%` in a code template, binding tighter than `+` `-`, which
bind tighter than a comparison. `num(h)` reads a hole's text as a number — the
whole text or none of it, so `'12abc'` is an error rather than 12. `+` now
**adds** when both sides are already numbers and **joins** when they are not,
which is the rule comparison has always used; nothing else about it changed.

`examples/calc.pt` is the first file here with no target language:

```
@syntax a "*" b 70 => { emit num(a) * num(b) }
```
```
2 + 3 * 4      →   14        (2 + 3) * 4    →   20
100 - 7 - 3    →   90        100 % 7        →   2
```

**Nothing about the tool made this possible except the arithmetic.** A rule has
always taken the values its children produced and combined them into its own;
the only change is that the combining may now be `*` instead of concatenation.
That is what [direction.md](direction.md)'s *bottom-up attribute grammar* means
spelled out, and seeing it run is worth more than the phrase was.

**It reads well, which was the question the experiment was given.** `num` on
every operand is the only noise and it is deliberate: a hole holds text, and
reading a number out of text that merely looks like one is the coercion this
tool refuses everywhere else. So `-` `*` `/` `%` are an error on non-numbers
rather than a silent zero.

**And it failed a test nobody had written down, which is the result worth
having.** [ROADMAP.md](ROADMAP.md) and [direction.md](direction.md) both said
this would make Prototype an interpreter generator. It does not:

```
if 1 then 10 else (1 / 0)      →      pt: '/' by zero
```

A hole is filled by parsing and expanding its subexpression **before** the
template runs, so a rule can select between two values that have already been
computed and cannot leave one uncomputed. **Evaluation is eager and cannot be
otherwise.** Every construct an interpreter needs that a calculator does not —
a short circuit, a loop, a recursion, a definition used before it runs — is a
thing that must *not* happen, and none of them is reachable.

So what arithmetic bought is exact: an **evaluator for expressions**. The gap to
an interpreter is **deferral** — a hole held unexpanded and run on demand — which
is a much larger idea than this was, and is not on the roadmap because nothing
has asked for it yet. direction.md's naming section is rewritten to say so, and
records that the risk it had predicted (would it read well?) was not the one that
bit.

*Verified at 11 examples, 42 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## `for i, x in h` and `at(h, n)`: two holes of one group, walked together

A repeated group may hold more than one hole, and until now that was a shape
nothing could use: `[ v ":" s ]*` gives two **parallel lists**, and neither kind
of template could put them back together. A string one splices each of them
joined; a code one's `for` walked a single list with no way to say *where* it
was. So the pair had to be folded into one value by an infix rule before the
group ever saw it — which is what `examples/pascal.pt` still does for its `case`
arms, and what it now says it is doing.

```
@syntax "case" e "of" [ v ":" s ]* sep ";" "end"
    => {
        emit "switch (" + e + ") {"
        for i, x in v sep "\n" { emit "case " + x + ": " + at(s, i) + "; break;" }
        emit "}"
    }
```

**The index is the first name**, the way Go and Python's `enumerate` read it,
counting from 0. One name is the old form and means what it always did.

**`at` out of range is an error, not an empty string**, and that is the part
worth arguing about. Two groups of different lengths is exactly the mistake this
feature invites — it is the failure mode of walking two lists in step — so the
one place that can notice it says so, rather than emitting nothing and letting a
short arm list read as a translation that worked.

**The bug in it was mine and it hung.** Two frames go on the environment for a
loop with an index, and the restore said `ev->env = fi.up` — which is the *first*
frame, not what was there before both. The second turn then linked a frame to
itself, and `lookup` walked a cycle forever. It surfaced as the first test
hanging rather than as a wrong answer, which is the one kind of failure this
tree's recorded outputs cannot express: there is no `.out` for *did not
terminate*.

**What it cost the two example files is a fifth difference**, and the sharpest
one on the page. `examples/code.pt` writes the arms as the grammar describes
them; `examples/pascal.pt` declares an infix `a ":" s` meaning *case arm*, which
then applies to every colon not already claimed by a longer pattern, and is
correct only because `a ":" "integer"` happens to be declared above it. That is
a rule whose correctness depends on the order of the file, and it is what the
absence of one builtin was costing.

*Verified at 10 examples, 39 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## `repeat` and `case`, and the prediction that `case` would be free

```
repeat                       do {
  n := n + 1        →        n = n + 1;
until n > 3                  } while (!(n > 3))

case n of                    switch (n) {
  1: writeln(11);            case 1: printf("%d\n", 11); break;
  4: writeln(44)     →       case 4: printf("%d\n", 44); break;
else                         default: printf("%d\n", 0); break;
  writeln(0)                 }
end
```

Both translate rather than rename. Pascal's `until` says when to *stop* and C's
`while` says when to *go on*, so the condition is inverted. Pascal's case arms do
not fall through and C's do, so every arm gains a `break` the source never wrote
— and the recorded output proves it, because a missing `break` would print the
default arm as well and `tests/pascal.sh` checks the numbers.

**The roadmap said `case` needed no new mechanics. That was wrong**, and finding
out how is the useful part. The arms want to be a repeated group of
`[ v ":" s ]`, and they cannot be: two holes in one group come out as **two
parallel lists**, and nothing puts them back together. A code template's `for`
walks one list and has no index; a string template splices each of them joined.
There is no `zip` and no `v[i]`.

**The workaround is to make the arm a rule** — `a ":" s` — so the pair becomes
one value before the group ever sees it, and the group has a single hole to
loop over. It works, and it is not a general answer: it needs the two parts to
be joinable by an operator rule, and it only parses because `a ":" "integer"` is
declared first and both patterns are three elements long, so declaration order
is what tries the type before the arm. Add a second Pascal type and it has to go
above that line, with nothing enforcing it. Both files say so in a comment.

So the roadmap gained an item with a customer rather than a guess, which is the
second time this stage has produced one: `for` wants an index, or a second loop
variable. Its next customer is already visible — a `var` section whose
declarations have different types is the same shape, and `real` beside `integer`
walks into it.

**`repeat` needed nothing at all**, which is what the prediction was right
about: a `stmts` hole stopping at `until`, and `group(c, 80)` in the code
template to bracket the condition only when C's `!` would otherwise take it
apart. `examples/pascal.pt` brackets unconditionally and `while (!(1))` is what
that costs.

*Verified at 10 examples, 36 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## `procedure` and `function`, and a parameter list that is a group

Pascal's subprograms, in both example files and checked by a compiler:

```
function Double(n: integer): integer;      int Double(int n) {
begin                                      return n * 2;
  Result := n * 2                          }
end;
```

**Nothing in the tool changed**, which was the prediction the roadmap made about
this stage and is the reason to record it. A parameter list is a repeated group
with `sep ";"`; a body is a `stmts` hole stopping at `end`; a call is a led
`"(" … ")"` at 95, which never has to be told apart from the circumfix `(` rule
because the two are in different positions. `tests/pascal.sh` now checks four
values instead of two.

**The return value is Free Pascal's `Result`, and the reason is worth keeping.**
Standard Pascal returns by assigning to the function's own name, and that is not
reachable here: the `:=` inside a body is an ordinary rule, and nothing tells it
which function it is inside. A rule sees its own pattern and no context at all.
So `Result := e` is a rule of its own — which also demonstrates a nud rule
claiming a token a led rule would otherwise have taken, and leaves `Result`
usable as an ordinary name everywhere else.

**The parameter list is the one place the code template bought nothing**, and
the first draft of both files claimed otherwise. Pascal writes
`(a: integer; b: integer)` and C wants the type once per parameter;
`examples/code.pt` loops over the list, and `examples/pascal.pt` gets *identical*
text from `join ", int "`, because every turn needs the same word in front of
it. The comment in each file now says so. `join` stops being enough the moment
two parameters have different types, which is where the loop earns its place and
where `real` will put it.

**And one thing that cannot be written at all.** A parameterless procedure is
called in Pascal by writing its bare name — `Banner;` — and nothing distinguishes
that from reading a variable called `Banner`. Same token, same position; telling
them apart wants a symbol table, which this tool does not have and has not
claimed to. So the example declares none: it could be written and never called.
It is on the roadmap beside `writeln`'s argument types, as a decision rather
than a rule.

*Verified at 10 examples, 36 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## `terminated(h)`: a template asks what a rule said about itself

`level(h)` had always been half of a pair. It answers *how tightly did the rule
that filled this hole bind*, so a template can decide whether to **bracket** it.
`terminated(h)` answers *did that rule declare its output already ends a
statement*, so a template can decide whether to **punctuate** it.

```
@syntax "if" c "then" t "else" f
    => {
        emit "if (" + c + ") " + t
        if not terminated(t) { emit ";" }
        emit " else " + f
    }
```

C wants `if (c) x = 1; else …` and forbids `if (c) { … }; else …`, and which of
the two a branch is depends entirely on the rule that filled the hole. Before
this, `examples/code.pt` braced every branch unconditionally — correct either
way, and noise around every single statement.

**Nothing had to be computed; something had to stop being thrown away.** The
flag already existed per rule, and the parse already carried it: `Out` returns
`{ level, terminated }` from `p_expr` and only `level` reached `bind_put`.
`Bind` gained the field beside `level`, `Val` gained it beside `level`, and the
builtin reads it the way `level` does. That is the whole change, and the shape
of it is the point — a question a template could not ask was one field of
plumbing, not a new idea.

**A `stmts` hole answers for its last statement**, which is the one a word after
the hole would follow. `p_stmts` was already tracking exactly that to decide its
own joining and discarding it at the end. A bare token, and a hole nothing
filled, are false.

**Two things only a compiler could have said.** The first draft emitted the
semicolon at the end of a rule as well as before an `else`, and got `x = 1;;`
— because `@separator` is what ends a statement, and a rule that also ends one
is writing it twice. The rule is that **a template punctuates inside itself and
never at its end**. The second: a block's last statement had never had a
semicolon at all, since a separator goes *between* two statements and never
after the last, and `begin … end` closed straight over it. Both were invisible
until `tests/pascal.sh` started compiling, and one of them predates this change.

**It does not help a string template**, and an earlier note here said it would.
A string template has no way to emit conditionally, so this is a code-template
builtin exactly as `level` and `group` are. `examples/pascal.pt` still braces
every branch, and the recorded diff between the two files now shows three
differences rather than two — parentheses, punctuation, and the literal — one
for each thing a template can ask that a string cannot.

*Verified at 10 examples, 36 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## Stage 1 begins: Pascal declares its variables, and a compiler checks the C

`examples/pascal.pt` and `examples/code.pt` now read `program`, a `var` section
with comma-separated declarations, `integer` and `boolean`, and an outer
`begin … end.` that becomes `main`. What comes out of `code.pt` is a whole C
program, `#include` and all, and `tests/pascal.sh` compiles it, runs it, and
checks the number it prints.

```
ok      pascal.sh: the C compiles, runs, and computes 39
            pascal.pt still cannot spell C's quotes -- as recorded
```

**The number is the point.** Every other example is pinned to a recorded `.out`,
which catches a change and nothing else — an expansion can be wrong in any way
that still looks plausible and the diff passes it. `39` is what the Pascal
computes, worked out from the Pascal; if `mod`, the precedence, the loop or a
branch were translated wrongly it would be some other number and nothing else in
the suite would have to notice. That is what choosing C as the target buys, and
`tests/hygiene.sh` had been the only place collecting it.

**It paid immediately.** Both files had been emitting `if (c) x = 1 else x = 2`,
which is not C: a branch that is an expression needs a `;` before the `else`, and
a branch that is a block must not have one. Wrong since the day they were
written, green in every recorded output the whole time, because it was plausible.
Every branch is now braced, which is correct either way.

**And it named the next mechanic.** Which of those two a branch is depends on the
rule that filled the hole, and that is exactly what `terminated` already records
— per rule, tracked through the parse. A template cannot ask: `level(h)` reads
`Bind.level` and there is no `Bind.terminated` beside it. So the braces are the
brace-shaped twin of `pascal.out`'s parenthesis noise, with the same cause, and
`terminated(h)` is on the roadmap with a customer that asked for it rather than
a guess that it would be wanted.

**`var` needed no new machinery**, which was the useful thing to learn about the
stage. A type is a quoted *word* rather than a `name` hole, because a hole
splices the token it matched and `integer` has to come out as `int`; `,` is an
ordinary infix rule at 20 and `:` one at 15, so `total, mod: integer` parses the
way it reads; and `var` is a rule that does nothing to what follows it, which is
what lets one `var` cover a section the way Pascal writes it. Everything else
stage 1 still wants — `procedure`, `function`, `repeat`, `case` — is the same
kind of work.

**The half that stays wrong is pinned too.** `pascal.pt` cannot translate
`'it''s'` into `"it's"`, and its output is the one in this tree expected *not*
to compile. `tests/pascal.sh` fails if it ever starts, so whoever fixes it has
to edit the test and the file's closing note in the same commit — the same
device `tests/hygiene.sh` uses on `bump: 105 0`, for the same reason.

*Verified at 10 examples, 36 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## Two files declaring one thing, and the word a file says it with

Three things could be declared twice and quietly were: a rule's pattern, a
`@token` class name, and `@separator`. Now the second is an error naming both
lines, unless it says `override`, in which case it wins and nothing is said —
because it was said in the source.

```
@use "../lib/arith.pt"
@syntax a "/" b 70 => "{a}:idiv({b})" override
```

**The roadmap described this backwards, and finding that out was most of the
work.** It said the later declaration wins, silently. For `@token` and
`@separator` it did. For a *rule* it is the opposite: candidates under one
leading word are tried longest-first with declaration order breaking a tie, so
the **earlier** wins and the later is unreachable — the file that wrote the
second template gets the first one's output. So the tool had two opposite silent
behaviours and the item's own example, `"+"`, was the one it described wrongly.

**A diamond had to stop being a collision before a collision rule could mean
anything.** `@use` read a file once per route to it, so `a` and `b` both using
`base`, and one file using both, declared everything in `base` twice — a file
colliding with itself over declarations nobody wrote twice, which any rule here
would have fired on first. A file is now read once, identity being the resolved
path. Proto does the same and for the same reason. A cycle now ends instead of
hitting the depth guard.

**Refuse-and-let-a-file-say-so rather than Proto's rule**, which is *later wins,
and the compiler says so*, graded by who could have known. Proto's answer needs
a warning channel this tool does not have and would have flipped rule resolution
from earlier-wins to later-wins; more to the point, it decides by position what
this tool declines to decide by position everywhere else. `override` makes the
file say it. `override` with nothing to displace is refused too, so the word
stays true when the declaration it was written against moves away.

**The word could only go in one place, and the notation put it there.** Left of
the `=>` a bare word is a hole — the premise the whole notation rests on — and a
rule whose first hole is called `override` is legal. After the template there is
no pattern, so a bare word cannot be anything else; `terminated` was already
there for exactly that reason. `terminated` and `override` may both appear, in
either order, and a hole may still be called either.

**What counts as one pattern is what matching can tell apart**: same elements,
same order, same words, same hole kinds, same group shapes. Hole names are not
part of it, since `a "+" b` and `x "+" y` match the same text. Levels are not
either. Two rules that merely share a leading word do not collide — that is the
candidate mechanism, and `if`/`if…else` and `examples/poem.pt`'s `-`/`--`/`---`
depend on it.

The rule check runs at seal, like the class-kind check below it, so directive
order cannot let one through and a rule that arrived through `@use` is named at
the line that wrote it. A class and a separator are checked where they are
written, because a later one replaces the earlier in place.

`examples/use.pt` is the recorded evidence: it uses `lib/arith.pt` and a new
`lib/vector.pt` that uses `lib/arith.pt` too — a diamond — and overrides
arith's `/`.

**And one thing found while testing it.** `@token name "…" garbage` and
`@separator ";" garbage` were accepted with the extra word ignored — `@syntax`
had always refused trailing text and these two never had. It stopped being
harmless the moment they took an optional bare word: `dtake` matches a prefix,
so `overridden` would have been read as `override` with three characters
dropped in silence. Both now refuse trailing text.

*Verified at 9 examples, 36 error cases, `tests/hygiene.sh`; `make check` clean.*

## A class-kind hole is refused in text mode

`@syntax "[" x:name "]"` under `@mode text` used to take everything up to the
`]` and never consult the kind. It is now refused where it is written:
`'x:name' asks for one token of a class, and text mode has no tokens — every
hole there is text`.

**Refused rather than honoured, and only the class kind.** `expr` and `stmts`
both mean *read up to the word that stops you*, which is exactly what a
text-mode hole already does, so those degrade honestly and were left alone. A
class kind says something else — *one token, matching this regex* — and text
mode has no tokens at all to say it about. So it was not a kind being
approximated; it was a kind being ignored while the file went on looking
correct. Honouring it, by running the class's regex at that position, is what to
build if somebody asks for it; nobody has, and refusing costs nothing that
`:text` or a bare hole does not already give.

**The check runs at seal, not at the rule that declared it**, which is the part
worth keeping. `@mode` is a directive like any other: a rule may be written
before the mode is, or in a file that `@use` pulled in and that names no mode at
all. Checking inside `@syntax` would have caught the file that happens to
declare its mode first and let the other two through — the same silent pass, one
directive-order away. Once the header has finished speaking the mode is settled
and every rule is in, so all three are caught, and the error still points at the
line and the file that wrote the hole. `grammar_seal` gained an error path for
it, which is where anything else that only a finished header can decide now
belongs.

This was the last silent wrongness on the roadmap. Nothing in `examples/` used
the form, which is why no recorded output changed.

*Verified at 9 examples, 28 error cases, `tests/hygiene.sh`; `make check` clean.*

## Groups in text mode, and the matcher that made them possible

`[ … ]`, `[ … ]*` and `[ … ]+` work in `@mode text`. `examples/poem.pt` writes a
markdown image whose title is optional and a shortcode whose argument list is
not, one rule each, and both use a code template because both need to ask
something — whether the optional part was there, and what each turn was.

**What it cost was the matcher.** Text mode was a single forward scan: walk the
elements once, find a hole's end by looking for the next literal word. A group
makes that impossible, because an optional part may or may not be there and what
really follows a hole is not known until the rest of the pattern has been tried.
So matching is now a search — take an alternative, try the whole remainder, put
the cursor and every binding back on failure — with a step budget against a
pattern that has too many ways to match.

**And one bound, which took two attempts.** A hole may not span the word that
*closes* the rule. The first version said *every word still to come*, which
fixed the defect it was written for and quietly broke
`"![" alt "](" src [ " " title ] ")"` — the group's space is a later word, and
`alt` is allowed to contain spaces. The image rule simply stopped firing, and
the output looked like prose that no rule had claimed, which is what text mode
does with anything it does not recognise. The closer is the one word whose
arrival means this construct has ended.

A search costs more than a scan, so it was measured rather than assumed: 113KB
of markdown, 2000 lines, through the `**`, `[[…]]` and dash rules of
`examples/poem.pt`, in 60ms, correct. The budget is per attempted match and does
not accumulate across a document.

*Verified at 9 examples, 27 error cases, `tests/hygiene.sh`; `make check` clean.*

## `terminated`: a rule that says its output ends a statement

The output separator used to be joined between every pair of statements,
including after one that ended in a word, so `examples/hygiene.out` carried a
`};` at file scope where C wants none. A rule may now be declared `terminated`,
and nothing is joined after it.

**The cheap answer would have been wrong, and wrong in both directions.**
Looking at the last character emitted and skipping the separator after a `}`
fails for `examples/clike.pt`, which reads C's braces and emits Solveig — where
a `.` is wanted between two statements however the one before ended — and it
fails the other way for C's own `struct { … };`. `examples/groups.pt` reads the
same braces as clike.pt and emits JavaScript, where the brace does end a
statement. So the input rule and the output rule are about two different
languages and had to stay two rules. Guessing is what this tool declines to do
about precedence and about scopes, and it declined here for the same reason.

The word sits **after the template**, which is the one place in a rule where a
bare word cannot be anything else, since a hole only appears in the pattern. So
it reserves nothing: `examples/reserved.pt` has a rule whose hole is called
`terminated` and which is itself `terminated`, four words apart.

This was the fifth of the code template's five customers, and the one that did
not come with the other four, because it turned out not to be a property of a
template at all but of a rule — so it works on both kinds.

*Verified at 9 examples, 28 error cases, `tests/hygiene.sh`; `make check` clean.*

## The code template: `=> { … }`

A second kind of template, an interpreted block rather than a string. A string
template splices and does nothing else; this one loops over a repeated hole,
asks whether an optional part matched, asks an operand what level it was parsed
at, and builds text with `emit`.

It cost nothing to tell the two apart. A template had always been a string and a
string never starts with a brace, so one character after the `=>` decides, with
no lookahead, nothing reserved, and no file written before it changing meaning.
And it keeps the one rule rather than bending it: the language is Prototype's
own, so it lives outside the strings, and the foreign text it emits lives inside
them.

Four of the five customers [ROADMAP.md](ROADMAP.md) recorded for it are served,
each named a file and each is now a recorded output:

  - **per-element output** — `for x in p sep ", " { emit "int " + x }`, where
    `join` could only put one fixed text between turns;
  - **output that depends on an optional part** — `matched(h)` and `count(h)`;
  - **parenthesisation** — `level(h)` and `group(h, n)`, which needed the
    expander to start recording the level a hole was parsed at;
  - **translating a literal** — `replace` and `drop`.

The fifth, a rule that says it needs no separator after it, is not a property of
a template and did not come with the others; it landed as `terminated` in the
entry above.

`examples/code.pt` is `examples/pascal.pt` with every rule rewritten in the new
form and the body left character for character alone, so the diff between the
two recorded outputs is the whole argument:

```
-for (int i = 1; i <= 20; i++) if (((((i % mod) == 0)) && ((i != 9))))
+for (int i = 1; i <= 20; i++) if ((i % mod == 0) && (i != 9))
-if ((!((total > 100)))) puts('it''s middling') else puts('big')
+if (!(total > 100)) puts("it's middling") else puts("big")
```

Both files stay. The string template's cost is real and is now a **choice**: it
is what a string template costs, and what a string template buys is being four
times shorter.

What did not move is the reach-out half of hygiene, which the roadmap predicted
would not and is the one prediction there that held.

*Verified at 9 examples, 27 error cases, `tests/hygiene.sh`; `make check` clean.*

## Groups: a part that repeats and a part that need not be there

`[ … ]` once or not at all, `[ … ]*` zero or more, `[ … ]+` one or more, with
`sep "s"` saying how turns are told apart on the way in and `join "j"` how they
are put back together on the way out.

The brackets are Prototype's own vocabulary and live outside the strings, which
is what makes them safe: a file that wants `[` and `]` in its own language
quotes them. `examples/clike.pt` declares `a "[" i "]"` for an index in the same
tool that reads `[ x ]* sep ","`.

Proto declined repetition and optional parts three times, on the grounds that no
program had asked. A language-agnostic tool has argument lists everywhere and
asks on the first file: `examples/groups.pt` writes one call rule covering every
arity there is.

Every hole is bound whether or not its group matched, so a template never has to
ask whether a part was there — and, when this was written, could not ask, which
is why an absent optional part still left `if (!0) { ; }` in the output. That
was the plainest customer for the code template, which arrived two entries above
and can ask with `matched(h)`.

*Verified at 8 examples, 20 error cases, `tests/hygiene.sh`; `make check` clean.*

## `{~t}`: a name nobody else has

A template may ask for a name nothing else uses. One expansion, one name: two
`{~t}` in a template are the same name and the next use of the rule is a
different one. A candidate is refused if it occurs anywhere in the source or in
any template any rule declared, `@use` included — a substring test, so it errs
in the safe direction.

**It closes exactly half of hygiene**, and finding out which half was the
result. A template that *introduces* a name is fixed. A template that *reaches
out* for one the caller shadowed is not, and is not fixable this way: there is
nothing to invent, and a template that is a string cannot see a scope. That half
moved out of "not done" and into "what it costs", where it belongs.

*Verified by `tests/hygiene.sh`, which compiles the C the example emits and runs
it: `swap: 2 1`, `bump: 105 0`.*

## The reference

[REFERENCE.md](REFERENCE.md) states what every part of a `.pt` file means —
directives, patterns, groups, kinds, levels, templates, both modes, the command
line, every error message the tool can produce, and the limits. It states and
does not argue; [notation.md](notation.md) argues.

Writing it meant checking every claim against the code rather than against
memory, and two of them were wrong. See [POSTMORTEM.md](POSTMORTEM.md) 2.

## The tool

C11 and `make`, plus POSIX `<regex.h>` for `@token` — the only thing here that
Proto does not also need, and the price of letting a file say what a literal is.

`prototype/src/header.c` is the fixed half: the directive grammar, which no file
can reach. `prototype/src/lex.c` is the lexer the header wrote.
`prototype/src/expand.c` is Pratt with backtracking, templates and text mode.

Six things quoting does not decide by itself were settled here, each written
down in [notation.md](notation.md) under a heading of its own: a hole's kind is
how far it reaches; two holes may not sit next to each other; a tie between a
token class and a declared word goes to the class; comments are looked for
before words; candidates under one leading word are tried longest first; and a
separator is wanted between two statements and not after one that ended in a
word.

*About 1550 lines of C.*

## The notation

Everything a directive says about text — the text it recognises and the text it
emits — is inside a string, and everything outside a string is Prototype's own
fixed vocabulary.

Two things fell out of it rather than being designed in. Four rule directives
collapsed to one, because once words are quoted a pattern's shape says whether
it is prefix, infix, postfix, circumfix or mixfix — and a pattern beginning with
a hole is a led rule while one beginning with a word is a nud rule, which is the
Pratt distinction read off instead of declared. And the lexer stopped being a
fixed budget, because the header is read before the body.

`examples/clike.pt` writes all six things Proto's `lib/clike.pro` lists as
impossible.
