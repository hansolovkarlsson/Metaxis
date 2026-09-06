# Completed

*What was **built**, and why, in enough detail to be worth reading a year from
now. [ROADMAP.md](ROADMAP.md) is the other half of the ledger and holds what is
not built; an entry is moved between the two and never copied. Each entry ends
with the state it was verified at, so a claim here can be checked rather than
taken. What a thing **costs** is not here — that is
[notation.md](notation.md)'s "What it costs", and it is written down rather than
argued away.*

Newest first.

## Collections — the head of the output is its body's aggregate

```
$ mx examples/basic.mx
int T;
int I;
int N;
const char *A_s;
…
```

**Three customers, one mechanism, and the rehearsal was the specification.**
BASIC declares nothing, so the line C wants first is the set of every variable
any line below mentions; `examples/code.mx` emitted `#include <stdio.h>`
whether or not a `writeln` fired; and `lib/island.mx` wrote the definition of
`complain` into the landmark it named rather than having the rule that needs
it say so. Each is a rule that knows one thing and a head that needs all of
them, and a rule sees its own holes and nothing else. On 2026-09-06 the whole
mechanism was faked first with markers in the output and a twelve-line awk pass
— [ROADMAP.md](ROADMAP.md) 4 as it then stood — and all three compiled and
ran. What was built is the shape the markers rehearsed and nothing more.

**`contribute("vars", text)`** is a statement in a code template, beside
`emit`: it adds a line to a named collection. A collection keeps one copy of
each distinct text, in the order first contributed — `T` said nine times is
declared once — and `@use`d files contributing to one name is the intended case
and not a conflict, because a contribution is additive and unordered and there
is nothing for `override` to be needed for. Rule locality survives untouched: a
rule still says only what *it* adds.

**`splice("vars")`** is an expression: it gives a placeholder, and the second
pass replaces the placeholder with the aggregate once expansion is over. The
placeholder is a fresh name, so it occurs nowhere in the source or in any
template, exactly as `{~t}` does not. Every line of the aggregate after the
first is given the whitespace the placeholder had in front of it, so a splice
inside an indented block stays in the block; a placeholder alone on its line
with nothing to put there takes the line with it.

**A collection nobody splices goes first.** BASIC has no head, and no rule in a
file with no head can name where *before the first statement* is. So that is
the default, for a source with no head; `examples/basic.mx` contributes and
never splices, and the four declarations lead its output. A source with a head
names its splice point — `program` in `code.mx`, `usage` in `island.mx` — and
the rehearsal showed why the default would be wrong for it: C's definitions
must follow its includes.

**The pass knows nothing.** It replaces marks and never reads what is between
them, which was the falsifier the roadmap item carried — *if the pass has to
know anything about the output language, the agnosticism is spent* — and it
did not fire in the rehearsal or in the build. The cost that stays is that the
tool has a second pass over its output where it had one; it is
`collect_resolve` in `metaxis/src/code.c`, and it is about the size the awk
was.

**What changed in the customers.** `tests/basic.sh` no longer writes a
prologue: its pin flipped, and it now fails if the four declarations are not
each there once and first. `code.mx`'s `program` rule no longer knows about
stdio; a Pascal program that never prints gets no include. `lib/island.mx`
became code templates so that the rewrite rule could contribute the definition
it needs, and `tests/island.sh` still counts one. Neither recorded output for
the last two changed by a byte.

**Three errors** for the ways to get it wrong: `contribute` in an `emit`
(*is a statement*), `contribute` with the wrong count, and `splice` with the
wrong count. `tests/errors.sh` cases 48 to 50.

Verified at 16 examples, 82 error cases and nine check scripts — 116 `ok`
lines.

## Stage 5 — the tool rewrites its own front end, and text mode was the island

```
$ mx examples/island.mx
static void complain(const char *e) { fprintf(stderr, "mx: %s\n", e); }

static void usage(void)
{
    if (!src) { complain(err); return 1; }
    complain(f(x, g(y)));
    fprintf(stderr, "mx: cannot write %s\n", outpath);
}
```

**Rehearsed first, by the stage 3 method, and the rehearsal was the stage.**
The survey's strongest finding was the island rule — text a file has no rule
for passes through — and its three proposed shapes were all for expression
mode. The cheapest thing that tested it cost no tool change: text mode already
scans, fires a rule where one matches, and copies everything else, so a
five-line header was pointed at `metaxis/cmd/mx.c` — the tool's own front end,
a file written for no grammar — to turn `fprintf(stderr, "mx: %s\n", err)`
into `complain(err)` and insert the definition. Six call sites changed, the
seventh with a different format string did not, and the rewritten tree built
and passed every check. That result was then made permanent: the rules are
`lib/island.mx`, `examples/island.mx` runs them over four representative
lines, and `tests/island.sh` concatenates them with the real `mx.c` at test
time — so the two can never drift — compiles what comes out against the
tree's own objects, and runs the binary on `examples/first.mx` and on a
missing file. Same output, same message, through `complain`.

**What the rehearsal measured**, each by running it, is
[ROADMAP.md](ROADMAP.md) 7: a bare word fires inside identifiers and strings
(`usage` → `help` rewrote the usage string; `err` → `e` made `stde`), a hole
stops at the first `)` and is right only when the template keeps it last, and
declaring C's comments removes them. The example's third line records the
bracket accident on purpose, the way `tests/hygiene.sh` records `bump`.

**What it found in the tool.** A led rule in text mode — `a "->" b` — was
accepted and silently never fired: `p->f` came through unchanged with exit 0.
The reference said *a led rule has nothing to continue* and nothing enforced
it, which is the shape of defect `seal_check` was written to refuse. It is
refused now, at the same place, once the header has finished: `a rule that
begins with a hole is infix, and text mode has nothing for it to continue — it
could never fire`. `tests/errors.sh` case 79.

**And two things about writing C from a string template** that the first draft
got wrong on its first run: `{` is a hole, so C's braces are `{{ }}`, and `\n`
is a newline, so the source's own `\n` is `\\n`. A code template has neither
problem.

Verified at 16 examples, 79 error cases and **nine** check scripts — 113 `ok`
lines, seven of them transcripts, the one above included.

## Stage 4 — BASIC→C, and the wall it was picked to reach

```
$ mx examples/basic.mx
int T;
int I;
int N;
const char *A_s;
L10: T = 0;
L20: for (I = 1; I <= 20; I++) {
L30: if (I % 3 == 0 && I != 9) goto L60;
…
L160: if (!(T > 30)) goto L190;
…
L190: return 0;
```

**Picked for a mechanic, by the roadmap's rule.** The three stages were done
and the page said the next thing to choose was a translator, not a mechanic.
The survey's shortlist said collection attributes had a customer — the include
`examples/code.mx` emits whether or not it is needed — and the source that
cannot be translated *at all* without them is BASIC: it declares nothing, a
variable exists because some line mentions it, and C wants every one declared
before the first statement. That line is the aggregate of the whole program,
and a rule sees its own holes and nothing else. So `examples/basic.mx` was
written to reach that wall and record where it is, and `tests/basic.sh` wrote
the four declarations by hand, compiled the rest, ran it, and checked the
numbers — pinned in both directions, so that the program must not build
without the hand-written line and the translator must not have started writing
it. *(Later the same day it did — the entry above this one — and the pin
flipped in that commit; the four lines at the top of the transcript are the
translator's, and the transcript is checked.)*

**It cost the tool nothing.** No directive, no builtin, no line of C. Four
things the notation had to reach, and reached:

- **A line number is the left operand of its statement.** `10 LET T = 0` is
  the number 10 followed by an infix `LET`: a led rule whose left hole is the
  line and whose word is the keyword, which is what a Pratt parser makes of it
  without being told. Every line becomes a C label, so `GOTO 80` is `goto L80`.
- **Each statement is written once and read twice.** A `@template` holds the
  body — `let(v, e)`, `print(x)` — and a led rule with the line number and a
  nud rule without it both call it, so `THEN LET T = T + I` costs one line.
- **The type of a variable is the sigil on its name.** Stage 1 stopped at
  `writeln` because printing a value means knowing its type. `print` reads it
  off the spelling — `replace(x, "$", "") != x` is *a string variable*, a `"`
  is a literal, anything else is a number — and the wall is not met, by choice
  of source and not by a new mechanic.
- **`FOR` and `NEXT` are two statements**, because they are in BASIC:
  `GOTO 70` jumps to a `NEXT` from inside its own loop and BASIC allows it. So
  `FOR` opens a brace and `NEXT` closes one, the way `examples/asm.mx` emits a
  sequence, and the C nests because the BASIC did. `L70: ; }` is what a label
  before a closing brace has to look like.

**What it found, in the order the runs found it.** The first expansion wrote
`!T > 30` for `NOT T > 30`: BASIC's `NOT` binds looser than a comparison and
C's `!` binds tighter than anything, so the operand of `!` is bracketed unless
it is an atom — `group(a, 80)`, one above the top of the ladder, where the
first draft asked at `NOT`'s own level. The first compile said `expected ';'
after return statement`: `END` is the last line, no separator follows it, and
its semicolon has to be its own. And the wall itself arrived with a second face
the roadmap had not named: **BASIC has no head.** Pascal has `program` and a
rule could splice an aggregate there; BASIC's first line is a statement, so the
*file* would have to be able to say *before the first statement*, and nothing
in the notation says that. That is now the second of the three decisions on
[ROADMAP.md](ROADMAP.md) 4, and it is the one the survey did not foresee.

**Two things the file says it does not read.** `REM`, because a comment wins
over a word and would leave the line number standing alone as a statement, so
the body comments with `'`. And `GOSUB`, which wants a return stack C does not
have.

Verified at 15 examples, 78 error cases and **eight** check scripts — 109 `ok`
lines, six of them transcripts, the one above included.

## `tests/docs.sh` — the transcripts in `docs/`, run

```
$ mx examples/first.mx
x = 1;
(add(x, 2) * 2)
```

**A transcript is a claim with a `$` in front of it, and the `$` makes it look
checked.** On 2026-09-05 one of the two in [REFERENCE.md](REFERENCE.md) had been
written instead of run — `mx -g examples/pascal.mx` shown printing a backend
that file does not declare — and [POSTMORTEM.md](POSTMORTEM.md) 19 said the
check was cheap enough to build. It is built. `tests/docs.sh` finds every fenced
`$ mx …` line in `docs/*.md` and `README.md`, runs it from the tree root with
`mx` replaced by the binary under `tests/limit.sh`, and compares what the
terminal would have shown: stdout and stderr together, the rest of the line
handed to the shell as written, so `2>&1 >/dev/null` in a transcript means what
it means at a prompt. The exit status is not looked at, because a transcript
cannot show one either. The block above is one of the five it runs, and was
run before it was pasted.

**The two decisions the roadmap said were not obvious.** *Elision*: a line that
is exactly `…` means *skip ahead* — the next line of the transcript is looked
for further down the output, and what lies between is not compared; a trailing
`…` matches whatever remains. So REFERENCE.md keeps eliding, because forty lines
of grammar would make it worse for a reader, and the check bends instead. *Where
it runs*: from the tree root, so a document writes `examples/use.mx` and never a
path relative to itself. A third the roadmap did not foresee: trailing
whitespace is stripped on both sides before comparing, because an editor strips
it from a document, and a document cannot hold what the tool turned out to be
printing.

**Which documents.** Everything but the two dated accounts, and those are
excluded by name rather than by a pattern: POSTMORTEM.md quotes the invented
transcript *as* the record of the mistake and must go on quoting it, and
CHANGELOG.md says what somebody saw on a given day, which a later day may
rightly change. The journal is the same kind of page and is not under
`docs/*.md` to begin with.

**What it found on its first run.** Two things, and neither was a wrong answer.
`mx -g` printed a trailing space after any rule without a level, and two spaces
before `terminated` — `show` in `metaxis/cmd/mx.c` put a space after every
element instead of between them — invisible on a terminal and impossible for a
document to hold, so the two `-g` transcripts could never have matched exactly.
Spaces now go between; the trace `-t` prints is unchanged. And the first
transcript in REFERENCE.md still said `$ pt examples/first.mx`, the tool's old
name, which the rename had missed: its output was right and its command was not
a command. Reading produced neither; running produced both.

**Proved by breaking it.** Against a scratch document: the invented backend
fails, an elision to a line that is not there fails, output that goes on past
the transcript fails, and a transcript that claims no output fails; a trailing
`…`, an error transcript read from stderr, and a block with a second `$ make`
after the checked command all pass. A document set with no transcripts in it is
a failure and not a pass, for the reason `hygiene.sh` gives — a check whose own
machinery breaks must not read as `ok`.

Verified at 14 examples, 78 error cases and **seven** check scripts — 106 `ok`
lines, five of them transcripts.

## `indent(s, n)`, and C that reads like something somebody wrote

```c
int main(void) {
    total = 0;
    if (total > 30) {
        total = total + 1;
        printf("%d\n", total);
    } else printf("%d\n", total);
```

**`examples/code.out` was flat.** Braces opened and nothing moved — every
statement at column zero, including the bodies of four procedures and a
`switch`. It compiled, it ran, it produced the right numbers, and it read like
nothing anybody wrote. That is the whole customer: the tool's flagship output
was correct and unreadable, and no `.out` diff would ever have complained.

`indent(s, n)` moves every line right by `n` spaces, **the first included**, and
leaves an empty line empty. It is *block* indentation rather than the
align-to-the-splice-column kind, because what asked for it was a brace. **Nesting
composes for free**: an inner block is already indented by the time the outer one
indents it, so a `switch` inside an `if` inside `main` comes out three deep
without any rule knowing how deep it is.

Five rules in `examples/code.mx` use it — `begin…end`, `repeat…until`, both
`case` forms and the outer `begin…end.` that becomes `main`.

**And the check that mattered is that nothing changed.** `tests/pascal.sh`
compiles the emitted C, runs it, and still gets `4 44 80 7 42`. The reading
changed and the meaning did not, which is the only claim worth making about a
formatting feature.

`examples/asm.out` was checked and left alone: it already indents its
instructions by hand and by assembly's own convention, so it has no customer.
**The string template has no equivalent**, and is not getting one until
something asks — `examples/pascal.mx` is the file that would use it, and its
output is recorded as deliberately wrong for other reasons.

Verified at 14 examples, 78 error cases and six check scripts — 100 `ok` lines.

## `tests/scale.sh`: one input large enough to hide nothing

```
ok      scale.sh: 4000 statements expand inside 10s, 4006 lines out
            1000 in 73ms, 4000 in 166ms -- reported, not asserted
            (four times the statements. When this was quadratic, the
             second number was 38767ms -- see docs/POSTMORTEM.md 18)
```

[POSTMORTEM.md](POSTMORTEM.md) 18 ends *"nothing here would have caught it, and
that is the honest answer: every example is small on purpose, and a suite that
runs in three seconds cannot see an O(n²)."* This is the answer to that
sentence, and it is the only check here whose reason for existing is a defect
the suite could not have found.

**It generates rather than stores.** 4000 statements of Pascal, in
`examples/pascal.mx`'s own grammar so that it measures a dialect the tree
actually declares, built by `awk` into a temp directory. Nothing 4000 lines long
goes into the repository.

**The check is that it finishes, not that it finished quickly.** A wall-clock
threshold on a shared runner is a flaky test, and what this guards against is
not a machine 20% slower — it is a cost that squares. The old lexer took ~39
seconds on this input and is killed by `tests/limit.sh`; the fixed one takes a
sixth of a second. The limit is not a budget, it is the gap between those two
answers. The times either side are **printed rather than asserted**, because a
number that is reported gets read and a number that is asserted gets tuned.

**And it checks the answer, not only the clock.** The two programs share a
preamble character for character, so the difference in output lines must equal
the difference in statements exactly — which assumes nothing about how many
lines a `program` and a `var` section become. A lexer that got fast by dropping
tokens would pass a stopwatch and fails this.

**Proved by breaking it.** The window in `metaxis/src/lex.c` was reverted, the
tool rebuilt, and `scale.sh` reported the timeout with the right message; then
restored. A check nobody has watched fail is a check nobody knows the shape of.

Verified at 14 examples, 78 error cases and **six** check scripts — 100 `ok`
lines.

## `mx -t`, and the quadratic it found

```
$ mx -t examples/first.mx 2>&1 >/dev/null
   try 1/1  a "=" b  [examples/first.mx:15]
    ok
   try 1/1  "twice" e  [examples/first.mx:17]
     try 1/1  a "+" b  [examples/first.mx:16]
      ok
    ok
trace: 3 candidates tried, 0 restored, deepest 2
```

`mx -g` prints the grammar a header declared and nothing printed the parse it
attempted, which is the half that cannot be reasoned about from outside:
candidates are tried longest-first with the cursor restored, so a rule that
never fires looks exactly like a rule that was never reached. One line per
candidate, indented by depth, saying which token it could not get past. **It
goes to stderr**, so `mx -t f.mx > out` still writes the expansion and nothing
else.

**It is an instrument before it is a convenience.** The roadmap's **budget for
expression-mode backtracking** asked for a measurement before a budget was picked — *a budget
chosen without one is a number somebody made up* — and specified a large
program in a declared dialect, timed. That measurement is now taken, and it
settles the item **against**:

| | |
| --- | --- |
| heaviest example (`pascal.mx`) | 104 candidates, 24 restored, deepest 6 |
| generated 4985-line Pascal | 31,206 candidates, **0 restored**, deepest 5 |
| the depth limit it runs against | 400 |

**Backtracking was never the problem, and there is no budget to pick.** What
the measurement found instead is in [POSTMORTEM.md](POSTMORTEM.md) 18: expansion
was **quadratic in the size of the input** — 4985 lines took 67 seconds — and
the cause was `regexec()` measuring the whole remaining file on every call,
three classes per token. Matching against a bounded window took the same file to
**174ms**, and lexing is now linear: 16000 statements in 524ms.

Two other quadratics were found and fixed on the way and **neither was the
cause**: `line_at()` rescanning from byte 0 per token, and `push()` copying the
whole token array per token. Both are kept because they are right; their
comments say plainly that they were not it.

Verified at 14 examples, 78 error cases and five check scripts — 99 `ok` lines,
unchanged before and after, because none of this changes what is emitted.

## `as`: a rule may emit more than once — and the reason it was built is half wrong

```
@syntax a "*" b 70 => "({a} * {b})"
                   => { emit group(a, 70) + " * " + group(b, 71) } as tight
```

One `=>` per target, `as <name>` tags one, `mx -b <name>` picks. **The untagged
template is the default and the fallback**, so a second target costs only the
rules that actually differ, and a file with one target is unchanged in every
respect — one emit with no tag is what a rule has always had.

**The shape is what kept it small.** A rule carries an `Emit` array;
`grammar_select()` runs once between the header and expansion and copies the
chosen one into the fields the rest of the tool already read. `expand.c` and
`code.c` were not touched. **`terminated` moved from the rule to the template**,
because it is a statement about the *output* and one target may brace a branch
where another does not.

**A file whose every template is tagged has no default**, and running it without
`-b` is an error naming the rule rather than a fall back to the first
declaration. Letting position decide the output is the question this tool
declines to answer by position everywhere else (§3.10).

`examples/backends.mx` is the customer: one grammar, two targets, both C. `mx`
brackets every operand and braces every branch; `mx -b tight` asks each operand
its level and writes a single statement unbraced. **Both compile and both print
`7 2`** — the difference is what it reads like and not what it means.

### And the falsification, which the item wrote down for itself

[ROADMAP.md](ROADMAP.md) said this would be falsified *"if the two template sets
turn out to want different **patterns** often enough that the sharing is a
lie"*, and offered `examples/pascal.mx` and `examples/code.mx` as the evidence
against that. **The evidence does not say what the item claimed.** Compared
pattern by pattern:

- **36 of 39 rules share a pattern exactly.** That much was right, and it is
  most of the value.
- **The two `case` rules do not.** `pascal.mx` writes `[ arm ]*` and folds the
  pair with an infix `a ":" s` rule; `code.mx` writes `[ v ":" s ]*` and walks
  two parallel lists, because a string template cannot interleave two lists at
  all.
- So `pascal.mx` also carries **one rule the other does not need** — the arm
  rule itself.

`as` chooses a **template**, never a pattern, so those three cannot be shared,
and forcing them would mean giving one target a grammar written for the other.
**The two Pascal files therefore do not merge**, and the 272 duplicated lines
this was built to remove are still there.

What is built is still worth having — it is the general mechanic, it has a
working customer, and it serves 36 of the 39 rules that motivated it. What is
left is narrower and better understood than when the item was written, and it is
back on [ROADMAP.md](ROADMAP.md) as that narrower thing.

**The lesson is the item's own method working.** A falsification condition
written down before the work is what turned "collapse the two files" into a
measurement rather than an argument, and the measurement disagreed. The
condition was met partially and precisely, which is the most useful way for one
to be met.

Verified at 14 examples, 78 error cases and five check scripts — **99 `ok`
lines**, and two of them are one file read out twice.

## `@mode` stops replacing itself in silence

```
@mode text
@mode expression override
```

`@mode` was the last global that a second declaration replaced without saying
so. A rule's pattern, `@token`, `@separator`, `@template` and `@fragment` all
refuse a second unless it says `override` (REFERENCE §3.10); this one did not,
and it was left out because the scope of that work had been settled as three
directives and widening it unasked is what the item it came from existed to
prevent. Now six things take `override` and none is an exception.

**The item named one silence and there were three.** `@mode expression zzz` also
ignored the `zzz` — `@token` and `@separator` had been taught to refuse trailing
text earlier the same day and `@mode` was missed by that work too. And the two
together had made a third: **`@mode expression override` parsed, and meant
nothing**, because the word after the mode was never read. All three are closed
by the same eight lines, and the third could only ever have been found by
writing the first two.

**Why `override` rather than a flat refusal**, which was the item's open
question. A second `@mode` in one file is always a mistake — but **two used
files are the case that cannot be written around.** A file with no body can
still declare the mode its rules need, because a set of text-mode rules is
usable only in text mode, and a file that uses two such libraries has to be able
to say which it meant. That is exactly the problem `override` was built for one
directive over, and giving `@mode` a second mechanic of its own would have been
a new concept for no gain. The word does not become noise: `override` with
nothing to displace is an error here as everywhere else.

Verified at 13 examples, **72 error cases** and five check scripts — three new
cases, one per silence — and **91 `ok` lines**. Closes the roadmap's
**`@mode` declared twice**, which is why the items below it moved up.

*(Named, not numbered. An entry here that said "closes ROADMAP 4" was wrong
within the hour, because closing an item renumbers the ones under it and the
number is handed to something else. A completion record cites what it closed.)*

## The limit guard: a property, where a number used to be

`tests/hygiene.sh` now runs two checks, and the new one is four lines of `awk`
stating a property: **the tool is never run except through `tests/limit.sh`.**

`tests/limit.sh` exists so a hang is reported rather than waited on, and it only
works where it is used — a test script added next month that runs the binary
directly is a hole in the one guard this suite has against the failure no
recorded `.out` can express, and it would pass every check here on the day it
was written.

**What it replaces is the reason it exists.** This file once said "all six
places the suite runs the tool" go through `limit.sh`. There were seven that
morning and eight by the evening, and a close-out read the sentence and did not
count ([POSTMORTEM.md](POSTMORTEM.md) 16). So the guard counts nothing. Three
shapes name the binary without running it — where the path is *stored*, a make
rule header, and handing the path to a script this same check covers — and each
is exempted by name rather than by a pattern broad enough to hide a fourth.

**It lives in `hygiene.sh` rather than in a sixth script** because that file was
already the one that checks a *property* rather than an output, and a whole
script for one grep is one too many. That was the item's open decision and this
is the answer.

**Two things were found by writing it, and both are the same shape.** The first
pattern matched *its own text* — this file is one of the files being scanned —
so the guard reported itself; the bracket classes in it are what fix that, and
they are commented, because they look like decoration and are not. The second is
worse and is the one worth carrying: an early draft had a regex `awk` could not
parse, so **`awk` exited 2, the substitution came back empty, and the guard
printed `ok`.** A check that passes when its own machinery breaks is worse than
no check, because it also silences the check that would have caught the thing.
The `|| exit` after the substitution is that fix, and it is the first thing in
this suite to guard its own failure rather than only the tool's.

Proved by planting a violation — `"$MX" examples/first.mx` appended to
`tests/asm.sh` — and watching it come back with the file and line, then
removing it. Closes the roadmap's **check that every run of `mx` is under the
limit**, which is why the items below it moved up.

Verified at 13 examples, 69 error cases and five check scripts, **88 `ok` lines**
where there were 87.

## CI: `make check`, on a machine that is not the author's

`.github/workflows/check.yml`. `make check LIMIT=30` on `ubuntu-latest` and
`macos-latest`, on push and pull request.

**What it closed.** Until it existed the suite had only ever run on one laptop,
which is the gap [POSTMORTEM.md](POSTMORTEM.md) 16 and 17 are both about: a
claim with no instrument behind it drifts. *"Clean rebuild, no warnings"* was
such a claim, written in every close-out and checked by nobody but the person
writing it. The repository had also just gone public, so it was making that
claim to strangers.

**The matrix is two rows because they exercise different halves**, and the first
run proved both:

```
ubuntu-latest   ok      asm.sh: skipped, examples/asm.mx emits arm64 and this is x86_64
macos-latest    ok      asm.sh: it assembles, runs, and prints 14 90 10 20
```

**The skip branch in `tests/asm.sh` had existed since stage 2 and had never once
been observed to work**, because the author's machine is arm64 and the branch is
unreachable there. Its first execution was on this run. The Linux row is also the
first time this code has been compiled by anything but clang — **gcc 13.3.0,
`-Wall -Wextra`, no warnings** — and the first time `tests/limit.sh` has run
under `dash` rather than macOS `sh`, which matters because that file does process
group management and is the one piece of the suite most likely to be
shell-specific.

`LIMIT` is raised to 30 for the runners. A shared machine is a loaded machine
and `tests/limit.sh` reports a timeout as a failure, so the local default of 10s
is generous there and not here.

Verified at 13 examples, 69 error cases and five check scripts, green on both
rows, 15s on macOS.

## The survey: what else does this, and what they have that this does not

[prior-art.md](prior-art.md). Twenty-six commits in, this project had been
compared against Proto and against nothing else. Every argument in `docs/` had
been checked against the code, which is the only thing the tree can check.

**What it is.** Three families, because the tools that do something like this
fail to be this tool in three different directions rather than one — a codemod
family that is agnostic in delivery and not in kind (Comby ships a delimiter
description per language and cannot read a notation invented this morning), a
transformation-system family that reads anything and charges a grammar artefact
and a build step (TXL, Stratego, Rascal, ANTLR, JastAdd, Silver), and an
extensible-language family that puts the declaration in the file and always
declares syntax **for its own host language** (Racket, Seed7, Coq, Prolog,
Katahdin, SugarJ). Metaxis is the third's placement with the second's
agnosticism, and that is the combination that did not turn up.

**What it cost the tree.** One false claim, corrected:
[direction.md](direction.md)'s "no other tool here has it" is falsified by four
of them, and the property is restated with the clause that was missing.
[POSTMORTEM.md](POSTMORTEM.md) 17 is the scoring, and it is the first entry here
scored by evidence from outside the repository.

**What it found that is worth keeping.** META II wrote quoted literals on the
pattern side and quoted output inside `.OUT` in 1964, which is this notation's
premise, both halves, on a machine with 8K of six-bit memory. `examples/code.mx`
is 272 lines duplicated from `examples/pascal.mx` and that is a maintenance cost
rather than a demonstration — now [ROADMAP.md](ROADMAP.md) 3. And **collection
attributes** (JastAdd, Silver) are a better-shaped answer to
[direction.md](direction.md)'s declared environment than the key/value store it
sketches: contribution is union rather than assignment, so two `@use`'d files
contributing is the intended case and rule locality survives — with a price this
tool is exposed to, since the aggregate is not known until the last contribution
and a one-pass tool would need a placeholder resolved afterwards.

**And what it deliberately did not do.** Nine candidates were found and **one**
became a roadmap item. That page's rule, written the same morning, is that a
mechanic is picked by finding the translator that would ask for it; a survey
finds mechanics the other way round, so the eight without a customer stayed in
`prior-art.md`, which is now where an idea with no customer lives.

Verified at 13 examples, 69 error cases and five check scripts, green — no code
changed.

## Stage 3: a block that is an indentation

```
@separator "\n" => ";\n" indent
@syntax "if" c ":" b:block "else" ":" e:block => { … }
```

Python ends a block by out-denting. Nothing in this tool could say that: a
`stmts` hole is refused unless a word follows it to stop at, and Python has no
such word — no `}`, no `end`. **This is the first delimiter the tool owns rather
than one a string declares**, which is why it was a decision before it was a
task, and the decision is the entry.

**Two halves, and they are independent.** `indent` on `@separator` gives the
lexer a stack of columns and two tokens no file spells; the `block` kind is how
a pattern reads them. The lexer half is the bulk of the work — and it is the
same bulk under every notation that was considered, which is what freed the
notation to be chosen on how it reads.

**Why `block` is a kind and not a pair of quoted words.** A synthetic marker —
`"⇥" b:stmts "⇤"`, with the lexer emitting tokens spelled that way — needs
*nothing* in the tool and was how the whole thing was rehearsed before any code
was written. It was declined. The premise is that a quoted thing is foreign text
**you can find in the file**; an indent has no spelling, so such a string quotes
text the source does not contain, and the file must pick a marker its language
never uses with nothing checking that it did. That keeps the letter of the one
rule and spends its meaning, and it cannot be taken back: once a quoted word can
name a token nobody wrote, quoting has stopped being the thing that tells a
mention from a declaration. A bare word outside the quotes is how this notation
already says *this one is Metaxis's*, and it says it here.

A third spelling — letting a `stmts` hole with no stop word mean *to the dedent*
under a nesting separator — was declined for costing an error: a rule ending in
`b:stmts` because its author forgot the word would be **silently accepted**.
That is the silence `override` was built to remove, one directive over.

**Both declines are recorded here rather than on the roadmap**, because the item
they belong to is settled and has moved. [notation.md](notation.md)'s "The one
exception, and what buying it cost" argues the first at length and is the page
to read before anyone proposes it again.

And here is the rehearsal, whole, because an argument against something is worth
nothing without the version that **works**. This ran before any of the above was
built, on the tool exactly as it then stood, and produced correct nested C on its
first go — the markers standing where an indent-aware lexer would later put the
two tokens it emits:

```
@comment "#" eol
@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"

@separator "\n" => ";\n"

@syntax a "<" b   40 left      => "{a} < {b}"
@syntax a "+" b   50 left      => "{a} + {b}"
@syntax n:name "=" v  5 right  => "{n} = {v}"
@syntax f "(" [ x ]* sep "," join ", " ")"  95 => "{f}({x})"

@syntax "if" c ":" "⇥" b:stmts "⇤"
    => "if ({c}) {{\n{b}\n}}"  terminated
@syntax "while" c ":" "⇥" b:stmts "⇤"
    => "while ({c}) {{\n{b}\n}}"  terminated
@end
x = 0
while x < 10: ⇥ y = x + 1
print(y)
if y < 5: ⇥ print(0)
⇤
⇤
x = 99
```

Nesting, `terminated`, and a dedent standing alone are all already correct
there, on a tool that knew nothing about indentation. **That is what makes it an
argument rather than an assertion**, and it is why it is checked in here instead
of in a scratch directory that no clone has.

**Four things fell out and none had to be built.**

- **Nesting.** An inner block consumes its own `dedent` before an enclosing hole
  can see it, because the hole owns both delimiters.
- **A word after a block.** `b:block "else"` works, and a `block` hole is
  therefore not greedy — it ends itself, so `two holes in a row` does not apply
  to what follows it. This is the shape a `stmts` hole could never have had.
- **A blank or comment-only line closes nothing**, because every newline
  restarts the column count and the indentation measured is always that of the
  line carrying the next token.
- **A statement after a block needs no separator**, because *a separator is not
  wanted after one that ended in a word* already covers it — `}` and `end` and a
  `dedent` are one case. The output side needed nothing invented at all:
  `terminated(b)` answers for a block's last statement exactly as it does for
  `begin … end`, which is what stops the `}` closing over an unterminated one.

**And a test that does something no other one here does.** The body of
`examples/python.mx` is Python — not Python-shaped, Python, which `python3`
runs. So `tests/python.sh` compiles the C *and* runs the source, and compares
the two answers. A translation that is wrong the same way on both sides of an
operator passes a diff and passes `tests/pascal.sh`; it fails this. Both halves
print `40 80 50`.

**What the example does not do, in its own closing note.** `elif` is one rule
per arm count, which is the shape [ROADMAP.md](ROADMAP.md) 6 declines to build
for. A wrapped call is not read, which is ROADMAP 2 and was found by running the
thing rather than reading it. C's types come off Python's annotations or nowhere,
which is the stage-1 wall in its honest form. And the example says `twice`
because a Python function called `double` translates to a C function called
`double`: **a rewriter that moves tokens cannot see that the word it just copied
is a keyword in the language it is writing**, and the only honest response was
not to do it.

*Verified at 13 examples, 69 error cases, `tests/hygiene.sh`, `tests/pascal.sh`,
`tests/asm.sh` and `tests/python.sh`; `make check` clean in 3.4s, no warnings.*

## A suite that can report a hang

```
FAILED  examples/x.mx: did not finish in 10s, and was killed.
        A hang is the one failure a recorded .out cannot show,
        so it is reported here rather than waited on.
```

Every check here compares an output to one recorded beside it, and two of them
compile that output and run it. **None of them can express *did not
terminate*.** There is no `.out` for a hang: `make check` in front of one did
not go red, it stopped.

`tests/limit.sh` runs a command under a wall-clock limit and exits 124 if it has
to kill it, which is what GNU `timeout` uses. macOS has no `timeout(1)` and this
tree takes no dependencies, so it is the portable shape: start the command, race
it against a sleeper, and let whichever finishes first decide. **Every place the
suite runs `mx` goes through it** — the Makefile's `check` and `record` loops and
one or two calls in each test script. `make check LIMIT=30` raises it for a
loaded machine.

*This paragraph said "all six places" when it was written and there were seven,
and the close-out that day did not catch it; there are eight now. The count has
been taken out rather than corrected, because a number counted by hand in prose
is a claim nothing checks — the check that would is now in `tests/hygiene.sh`,
and [POSTMORTEM.md](POSTMORTEM.md) 16 is what it cost.*

**It costs nothing when nothing hangs.** The full run is 2.3 seconds, and the
limit is not a performance budget: every example expands in milliseconds and 10
seconds is three orders of magnitude of headroom.

**Two mistakes while building it, both kept in the file's comments** because
both are easy to make again. The watchdog inherited stdout, so under
`x=$(limit.sh …)` the command substitution waited for the *sleeper* to release
the pipe and a 62-case run took over ten minutes — a background process holds a
pipe whether or not it writes to it. And `kill -9` on the child orphaned the
child's own children, which is the precise failure this file exists because of;
the command now runs under `set -m` and the process *group* is killed.

**Verified against a stub that never returns**, through the real Makefile
recipe: each example is killed at the limit and reported, `make check` exits
nonzero instead of waiting, `errors.sh` says so in its own words, and no
process is left behind.

See [POSTMORTEM.md](POSTMORTEM.md) 14 for what this cost a day, and why an
accurate observation in a commit message closed nothing.

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
come back apart in the output. So `examples/pascal.mx` emits `int k` for a
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

`examples/pascal.mx` and `examples/code.mx` each wrote that parameter list
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

`examples/asm.mx` had written that test out at every operand — eight times,
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
`examples/asm.mx` went from 2458 bytes to 1473 — a little under half — and
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

`examples/asm.mx` reads a C subset and emits arm64 that `tests/asm.sh`
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
same limitation `examples/pascal.mx` meets when it cannot translate a string
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

`examples/calc.mx` is the first file here with no target language:

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
this would make Metaxis an interpreter generator. It does not:

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
group ever saw it — which is what `examples/pascal.mx` still does for its `case`
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
one on the page. `examples/code.mx` writes the arms as the grammar describes
them; `examples/pascal.mx` declares an infix `a ":" s` meaning *case arm*, which
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
apart. `examples/pascal.mx` brackets unconditionally and `while (!(1))` is what
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
`examples/code.mx` loops over the list, and `examples/pascal.mx` gets *identical*
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
this, `examples/code.mx` braced every branch unconditionally — correct either
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
builtin exactly as `level` and `group` are. `examples/pascal.mx` still braces
every branch, and the recorded diff between the two files now shows three
differences rather than two — parentheses, punctuation, and the literal — one
for each thing a template can ask that a string cannot.

*Verified at 10 examples, 36 error cases, `tests/hygiene.sh` and
`tests/pascal.sh`; `make check` clean.*

## Stage 1 begins: Pascal declares its variables, and a compiler checks the C

`examples/pascal.mx` and `examples/code.mx` now read `program`, a `var` section
with comma-separated declarations, `integer` and `boolean`, and an outer
`begin … end.` that becomes `main`. What comes out of `code.mx` is a whole C
program, `#include` and all, and `tests/pascal.sh` compiles it, runs it, and
checks the number it prints.

```
ok      pascal.sh: the C compiles, runs, and computes 39
            pascal.mx still cannot spell C's quotes -- as recorded
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

**The half that stays wrong is pinned too.** `pascal.mx` cannot translate
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
@use "../lib/arith.mx"
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
candidate mechanism, and `if`/`if…else` and `examples/poem.mx`'s `-`/`--`/`---`
depend on it.

The rule check runs at seal, like the class-kind check below it, so directive
order cannot let one through and a rule that arrived through `@use` is named at
the line that wrote it. A class and a separator are checked where they are
written, because a later one replaces the earlier in place.

`examples/use.mx` is the recorded evidence: it uses `lib/arith.mx` and a new
`lib/vector.mx` that uses `lib/arith.mx` too — a diamond — and overrides
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

`[ … ]`, `[ … ]*` and `[ … ]+` work in `@mode text`. `examples/poem.mx` writes a
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
`examples/poem.mx`, in 60ms, correct. The budget is per attempted match and does
not accumulate across a document.

*Verified at 9 examples, 27 error cases, `tests/hygiene.sh`; `make check` clean.*

## `terminated`: a rule that says its output ends a statement

The output separator used to be joined between every pair of statements,
including after one that ended in a word, so `examples/hygiene.out` carried a
`};` at file scope where C wants none. A rule may now be declared `terminated`,
and nothing is joined after it.

**The cheap answer would have been wrong, and wrong in both directions.**
Looking at the last character emitted and skipping the separator after a `}`
fails for `examples/clike.mx`, which reads C's braces and emits Solveig — where
a `.` is wanted between two statements however the one before ended — and it
fails the other way for C's own `struct { … };`. `examples/groups.mx` reads the
same braces as clike.mx and emits JavaScript, where the brace does end a
statement. So the input rule and the output rule are about two different
languages and had to stay two rules. Guessing is what this tool declines to do
about precedence and about scopes, and it declined here for the same reason.

The word sits **after the template**, which is the one place in a rule where a
bare word cannot be anything else, since a hole only appears in the pattern. So
it reserves nothing: `examples/reserved.mx` has a rule whose hole is called
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
And it keeps the one rule rather than bending it: the language is Metaxis's
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

`examples/code.mx` is `examples/pascal.mx` with every rule rewritten in the new
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

The brackets are Metaxis's own vocabulary and live outside the strings, which
is what makes them safe: a file that wants `[` and `]` in its own language
quotes them. `examples/clike.mx` declares `a "[" i "]"` for an index in the same
tool that reads `[ x ]* sep ","`.

Proto declined repetition and optional parts three times, on the grounds that no
program had asked. A language-agnostic tool has argument lists everywhere and
asks on the first file: `examples/groups.mx` writes one call rule covering every
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

[REFERENCE.md](REFERENCE.md) states what every part of a `.mx` file means —
directives, patterns, groups, kinds, levels, templates, both modes, the command
line, every error message the tool can produce, and the limits. It states and
does not argue; [notation.md](notation.md) argues.

Writing it meant checking every claim against the code rather than against
memory, and two of them were wrong. See [POSTMORTEM.md](POSTMORTEM.md) 2.

## The tool

C11 and `make`, plus POSIX `<regex.h>` for `@token` — the only thing here that
Proto does not also need, and the price of letting a file say what a literal is.

`metaxis/src/header.c` is the fixed half: the directive grammar, which no file
can reach. `metaxis/src/lex.c` is the lexer the header wrote.
`metaxis/src/expand.c` is Pratt with backtracking, templates and text mode.

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
emits — is inside a string, and everything outside a string is Metaxis's own
fixed vocabulary.

Two things fell out of it rather than being designed in. Four rule directives
collapsed to one, because once words are quoted a pattern's shape says whether
it is prefix, infix, postfix, circumfix or mixfix — and a pattern beginning with
a hole is a led rule while one beginning with a word is a nud rule, which is the
Pratt distinction read off instead of declared. And the lexer stopped being a
fixed budget, because the header is read before the body.

`examples/clike.mx` writes all six things Proto's `lib/clike.pro` lists as
impossible.
