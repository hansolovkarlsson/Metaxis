# Completed

*What was **built**, and why, in enough detail to be worth reading a year from
now. [ROADMAP.md](ROADMAP.md) is the other half of the ledger and holds what is
not built; an entry is moved between the two and never copied. Each entry ends
with the state it was verified at, so a claim here can be checked rather than
taken. What a thing **costs** is not here — that is
[notation.md](notation.md)'s "What it costs", and it is written down rather than
argued away.*

Newest first.

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
