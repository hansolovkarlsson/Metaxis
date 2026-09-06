# Postmortem

*The **scoring**. One entry per mistake or prediction that has met evidence: one
that held, one that failed, or one that held and then failed. Each goes under
**Issue**, **Root cause**, **Solution** and **Learnings**. Not a bug log: a
defect belongs here when what it taught outlives it, and a day that scored
nothing adds nothing. The learning is the part that has to be true a year from
now, so it says what would have caught the thing rather than resolving to be
more careful.*

Newest first.

---

## 24 · A reference that typeset the messages it quoted

**Issue.** Twenty-five error messages quoted in backticks in
[REFERENCE.md](REFERENCE.md) §10 and in [COMPLETED.md](COMPLETED.md) and
[CHANGELOG.md](CHANGELOG.md) showed an em dash where the tool prints two
hyphens. Every one was a message the tool really produces, and none was the
message as written.

**Root cause.** The messages were transcribed into the page and then typeset
as prose, so the page's punctuation habit reached into the quotes. Nothing
compared a quoted message to the source: `tests/errors.sh` pins each message
by a fragment, and no fragment crossed the spot where the dash stood, and
`tests/docs.sh` runs transcripts, which these were not.

**What found it.** A style sweep, on 2026-09-06, for a rule about prose. The
check for that rule exempts code spans, so it did not flag them either; an
agent counting what its exemption covered noticed that the spans held a
character the tool never emits. `grep` of the sources for the em dash
confirmed the tool has none.

**Solution.** Each span was checked against the line in `metaxis/src` that
prints it and made verbatim.

**Learnings.** **A quotation in backticks is a claim that the tool says
this, and it is unchecked unless something compares it.** Entry 19 was the
same lesson for a transcript, and a transcript is now run. A quoted message
is the smaller case of the same thing, and what would catch it is small: every
backticked message on the errors page is a substring of a string literal in
the source. That check is not built. Until it is, the errors page is the one
place in the reference that says what the tool prints without running it.

---

## 23 · A check that passed on no input, twice in one file that warns about it

**Issue.** The prose check in `tests/hygiene.sh` printed `ok` on its first
run over a tree holding 679 lines it should have refused. Earlier the same
afternoon the roadmap check had been placed the same way and happened to be
harmless.

**Root cause.** The check's code was inserted at the comment that describes
it, which is in the file's preamble, above the line that computes the list of
tracked files. `xargs awk` with an empty list runs awk on standard input,
which was empty, so the scan found nothing and reported nothing wrong. The
file's own note on the limit guard says a check that passes when its own
machinery breaks is worse than no check; the note was read while the code was
written and did not prevent it.

**What found it.** Reading the output order. The new `ok` line printed before
the limit guard's, which runs first in the file, so the check had run before
its input existed.

**Solution.** The code moved below the line that lists the files, and the scan
now refuses an empty list as a failure. The two plants that proved the roadmap
check, a stale citation and a deleted heading, were repeated for this one.

**Learnings.** **Every input to a check is a thing that can be empty, and the
empty case is a pass unless it is refused by name.** The limit guard learned
this for a regex awk could not parse; this is the same lesson for a file list.
The general form is that a check reports what it scanned as well as what it
found, and the line it prints should make an empty scan visible: a count of
files, or a refusal. A note in a file does not prevent the mistake the note
describes; a line of code that fails does.

---

## 22 · A test's own watchdog wrote into what the test compared

**Issue.** The first deploy of the website went out green while the suite's
macOS row failed beside it, on one transcript out of thirty-five, with the
tool's output correct. The captured output had gained a line:
`tests/limit.sh: line 61: 3332 Terminated: 15 ( sleep "$LIMIT"; … )`.

**Root cause.** `tests/limit.sh` races every command against a watchdog and
kills the watchdog when the command finishes first. The watchdog's own output
had been closed off deliberately since the file was written: the comment above
it says why. But the *announcement* of a killed background job is not the job's
output; it is the shell's, printed on the shell's stderr at the next `wait`, and
`tests/docs.sh` captures stderr into what it compares, because a transcript
shows what a terminal shows. Bash prints that notice sometimes, depending on
timing, and a loaded runner is where the timing lands. Three hundred local runs
never produced it.

**What found it.** CI, once. Not the suite locally, which had run the same
transcript dozens of times that day.

**Solution.** The shell's stderr is parked on a spare descriptor for the two
lines that kill and reap the watchdog, and restored after. Timeouts, errors
and exit codes still pass through; the next push ran clean on both rows.

**Learnings.** **A check that compares captured stderr owns everything that can
reach that descriptor, including what the shell says about the check's own
machinery.** `hygiene.sh` already had the sibling rule, *a check that passes
when its own machinery breaks is worse than no check*, and this is the same rule
from the other side: a check that *fails* because of its own machinery is one
nobody trusts on the next red run. The specific form to keep: anything a test
script backgrounds and kills is a source of stderr that is the shell's, not the
process's, and closing the process's output does not close it.

---

## 21 · A sentence in the reference that no file had ever exercised

**Issue.** REFERENCE §6.3 has said since the first day that a separator is
wanted between two statements *and not after one that ended in a word — that <!-- as written -->
is what lets `}` and `end` stand on their own*. `end` could not. A tutorial
file wrote `block … end` on one line and `while …` on the next, with no `;`
between, and got `no rule reads 'while' here`.

**Root cause.** The check asked whether the last token consumed was
*punctuation*. `}` is; `end` is not: the class wins a tie, so `end` lexes as a
`name` token whose text happens to be a word, and the token's kind cannot say
which of the two the rule made of it. The sentence was true of `}` and false of
every alphabetic closing word, and every example that had one wrote `end;`,
Pascal's own habit, so nothing in `examples/` had ever put the sentence to the
test.

**Solution.** The parser now records the index of the last token a rule
consumed *as a word*, restores it wherever it restores the cursor, and asks
that instead of the token's kind. `docs/tutorial/05-statements.mx` is the
pin: its transcript is run by `tests/docs.sh`.

**Learnings.** **Writing the tutorial was a test, and it found what the examples
could not.** The examples are translators taken far enough to be compiled and
run, and a translator writes the source language the way its speakers do,
`end;`, so a corner that idiom never visits is never visited. A tutorial file is
written to *show one sentence of the reference*, and that is the input the suite
did not have: the same lesson as [18](#18)'s `scale.sh`, from the other side.
The rule that follows: **when a document claims a behaviour, the smallest file
that would demonstrate it belongs in the tree, run.** That is what
`docs/tutorial/` now is, and the transcript check is what makes it a test rather
than prose.

---

## 20 · A roadmap item that vanished for one commit

**Issue.** [ROADMAP.md](ROADMAP.md) 7, the island rule, was written on
2026-09-06 and placed between items 4 and 5, where the page's own ordering put
it. The next commit rewrote the tail of item 4 by replacing everything from a
sentence inside it *up to the heading of item 5*, and item 7 was in between. It
was gone from the page for one commit, pushed, and noticed only when the commit
after that removed item 4 the same way and the section headings were listed.

**Root cause.** An edit addressed by *the next heading I remember* rather
than *the end of this item*, in a file whose items are not in numeric order.
The slice was right the day the page was linear and wrong the day it was not,
and nothing told it.

**What found it.** Listing the headings after an edit, which is the check
that should have run before it: `grep '^## '` costs nothing and would have
shown 1, 2, 3, 5, 6 with no 7.

**Solution.** Restored from the commit where it last existed, with the two
wording changes the lost edit had meant to make to it, and a line in the item
saying it was lost and for how long.

**Learnings.** **A prose edit that is addressed by position needs the same check
a transcript got yesterday: run something afterwards that would have failed.**
`tests/docs.sh` runs the transcripts and could not see this, because no
transcript was involved. What would see it is small: every `ROADMAP.md N` that
any document names should resolve to a `## N ·` heading, and every heading that
was on the page before an edit should be on it after unless the commit says it
moved. The first is a grep; the second is `git diff | grep '^-## '`, read before
committing. Neither is built yet, and the first would flag two references in
`metaxis/src` that went stale when items were renumbered on 2026-09-05, which is
the argument for building it.

---

## 19 · An example that was written instead of run

**Issue.** [REFERENCE.md](REFERENCE.md) § 9 showed what `-g` prints:

```
$ mx -g examples/pascal.mx
backend    tight
```

`examples/pascal.mx` declares no backends. It cannot print that line and never
could. The file that does is `examples/backends.mx`, and the transcript was
written the same hour `-b` was built, from what the output *would* look like.

**What found it.** The close-out audit, by running the command in the page. The
other transcript in that file, `mx -g examples/use.mx`, was checked at the same
time and is **accurate**, elisions and all, so this was one invented example
among two rather than a habit.

**Root cause.** It was **born false**, and that is what makes it a different
defect from [16](#16). Sixteen was a count that was true when written and rotted
when the tree changed under it; the fix there is to re-check. This one was never
true for a moment. Nothing rotted, nothing drifted: a plausible transcript was
composed to illustrate a feature that had just been built, in the same commit
that built it, while the tool that would have printed the real one sat two
keystrokes away.

**Solution.** Corrected to `examples/backends.mx`, which is the file that has a
backend, and the output pasted from the actual run.

**Learnings.** **A transcript is a claim with a `$` in front of it, and the `$`
makes it look checked.** That is exactly its danger: prose invites doubt and a
command prompt performs having-been-run. The rule that follows is narrow enough
to keep: **never write a console example you did not just execute**, not "verify
examples periodically", which is 16's fix for 16's problem and would not have
helped here, because there was no interval during which this was right.

**And the check is cheap enough to be worth building.** Every `$ mx …` line in
`docs/` could be run and its output compared. There are two today; the point is
not the two, it is that the tool can check its own documentation and does not.
It is on [ROADMAP.md](ROADMAP.md) with this entry as its customer.

---

## 18 · An item that named its suspect, and two fixes for the wrong thing

**Issue.** Expansion was quadratic in the size of the input. 125 statements of
generated Pascal took 78ms, 2000 took 13.3 seconds, and 4985 took **67
seconds**. Nothing in the tree could see it: every example here is a few dozen
lines, and `make check` runs green in three seconds.

**What found it.** `mx -t`, built to answer the roadmap's **budget for
expression-mode backtracking**, which asked for *a large program in one of the
declared dialects, timed*, before a budget was picked. Generating that program
was the whole of the work; the defect announced itself on the first run.

**Root cause, and it is three deep.**

The item **named its suspect in its title**: *a budget for expression-mode
backtracking*. The measurement cleared the suspect completely. The heaviest
example restores 24 candidates; the 4985-line program restores **zero** out of
31,206 tried, at depth 5 against a limit of 400. Backtracking was never
anything.

Then I guessed twice, and shipped both guesses as fixes with comments claiming
they were the cause:

- **`line_at()`** rescans from byte 0 for every token. Genuinely quadratic.
  Fixing it changed nothing measurable.
- **`push()`** reallocated and copied the whole token array per token. Also
  genuinely quadratic: about 12 million `Tok` copies at that size. Took 67
  seconds to 51.

The cause was `regexec()`. **It takes a NUL-terminated string, and the C library
measures it**, so handing it `src + i` costs O(rest of file) on every call,
three classes per token. The match is anchored and short; the measuring is not.
The proof is a file with **the same tokens** and 200KB of trailing comment:
4.1s became 32.6s.

**Solution.** Match against a bounded window that doubles only when a match
reaches its edge, sound because every class pattern is already compiled
anchored. 4985 lines went from **67107ms to 174ms**, and lexing is linear: 16000
statements in 524ms. The two earlier fixes are kept, because they are right and
the second would have become the bottleneck once this one was gone, and their
comments are rewritten to say they were not the cause.

**Learnings.**

**A performance item that names its suspect will find the suspect guilty unless
it measures.** This one named backtracking in its own title and was written by
someone who had read the parser and not run it. What saved it was the clause it
added against itself: *what the item is really asking for is the measurement,
because a budget picked without one is a number somebody made up.* That sentence
is why the instrument got built instead of the budget, and the instrument is
what cleared the suspect.

**And the profiler is four commands, which is fewer than one wrong guess.** I
read the lexer, formed a hypothesis, fixed it, and re-measured: twice, both
times wrong, both times having written a comment asserting the cause. `sample`
on the running process pointed at `regexec` immediately and would have done so
before either. **Reading code produces plausible causes; only measurement
produces the cause**, and this is the same lesson as
[16](#16--a-number-counted-by-hand) one layer down: prose about the tree drifts
from the tree, and a hypothesis about the tree is prose.

**A footnote that is its own small entry.** Writing this up renumbered the
roadmap again, and **both records written in the same hour cited the closed item
by number**, the fourth time that has happened today, an hour after an entry was
added saying a completion record must cite what it closed rather than where it
sat. Knowing a rule and applying it are different acts, and the only thing that
has actually caught this is `grep` after each renumber.

**What would have caught it earlier.** Nothing here, and that is the honest
answer: every example is small on purpose, and a suite that runs in three
seconds cannot see an O(n²). The tree has no large input and now knows it wants
one.

---

## 17 · A claim the page could not check, on the page that says it checks every claim

**Issue.** [direction.md](direction.md) closed with **"One property, and no
other tool here has it: the language definition and the program that uses it
live in the same file and are read in one pass."** Four tools falsify it as
written. Seed7 declares a statement's keywords, holes and priority in the
program and uses it on the next line. Coq's `Notation` takes quoted words, bare
holes, a level and an associativity. Katahdin lets a running program modify its
own grammar. Prolog's `:- op(700, xfx, ===)` has done the smallest version of it
since 1972.

**What found it.** The first survey of tools outside this repository, run
because it was asked for. Nothing in the tree could have found it: the suite has
no opinion about Seed7, and neither has Proto.

**Root cause.** The page's own note says *every claim in it that could be
checked against the code has been*, and that clause is doing far more work than
it reads as doing. **It scopes verification to what this repository contains,
and the one claim on the page that was about the outside world is precisely the
one the scope excludes.** The sentence reads as rigour and is, for every claim
it was written for; the claim it was not written for sat under it for a day
wearing the same authority as the rest.

**Solution.** The section is rewritten rather than appended to, which is what
that page requires of itself. What survives is a conjunction: the definition and
the program in one file, in one pass, **and neither the language read nor the
language written is the tool's own**: the clause that separates a rewriter from
an extensible language, and the reason none of the four can be pointed at Pascal
on Monday and arm64 on Tuesday. [prior-art.md](prior-art.md) § 2 holds the
evidence, and is where the next claim of this kind gets checked.

**Learnings.** **A verification rule is only as wide as the claims it was
written for, and it does not announce the difference.** Every claim about the
tool had been checked; the one claim about the *field* had never been checkable,
and the note covering both did not distinguish them. What would have caught it
is the thing that eventually did: going and looking, once, at what else exists.

And the shape is 16 one step further out. That entry was **prose describing the
tree drifting from the tree**, and its answer was to run a command instead of
re-reading a sentence. This one is prose about the world that was never checked
against the world at all, and its answer is that **some sentences have no
command in this repository**. For those, the only instrument is a survey, and
until yesterday this project had never run one.

The correction is also the argument for doing it. The claim got *smaller* and
*better* in the same edit: what it now says is what the tool is for, where what
it said before was a boast about being first, and the boast was the false half.

**A second instance, found within the hour and worth adding here rather than
opening an entry for.** The GitHub repository's *About* field held the
description `direction.md`'s own table labels **before 2026-09-05**: "a
language-agnostic rewriter", with no mention of the evaluator that landed that
morning. It had been stale all day. It is the same root cause one step further
out again: **repository metadata is a document that nothing in the tree can
read.** `make check` cannot see it, `grep` cannot see it, and no close-out had
ever thought to look, because every instrument this project has stops at the
edge of the working tree. It was fixed by asking `gh` what it said, which is the
survey answer at the smallest possible scale: **the instrument for a claim
outside the repository is to go and fetch it.**

---

## 16 · A number counted by hand, wrong on the day it was written, that a close-out read and did not check

**Issue.** `COMPLETED.md`'s entry for `tests/limit.sh` said **"All six places the
suite runs `pt`"** go through it. There were seven when the sentence was written,
this morning. By the evening there were eight, because `tests/python.sh` added
one. Neither the commit that wrote it nor the close-out four hours later noticed.

**What found it.** The next close-out, doing the audit rather than the writing:
`grep -c` over `Makefile` and `tests/*.sh`, run because a *different* claim
looked stale and the same command answered both.

**Root cause.** The file's own opening note says *each entry ends with the state
it was verified at, so a claim here can be checked rather than taken*. That is
true of the `Verified at 12 examples, 62 error cases` lines, which are dated
snapshots and stay true by being about a moment. It is not true of a count in
running prose describing how the tool is **wired**, which claims to be about now
and rots the first time somebody adds a file. The two look identical on the page
and behave completely differently, and nothing marked which was which.

**Solution.** The count is gone rather than corrected: *every place the suite
runs `pt` goes through it*, with the shape spelled out and no number to go
stale. Three more descriptions of `make check` were stale in the same direction
and by more: `CLAUDE.md`, `README.md` and `notation.md` all still said the suite
ran `errors.sh` and `hygiene.sh`, which stopped being the whole list when
`pascal.sh` landed **eleven commits ago** and was wrong twice over by tonight.
`CLAUDE.md` is the file a session with no memory reads first, so that one was
costing something every morning.

And a grep that fails when a test script runs `$PT`
without `tests/limit.sh` in front of it. That is the claim, enforced.

**Learnings.**

**A number in prose is a claim with no test, and it decays fastest in the
sentences that sound most authoritative.** "All six places" reads like something
somebody verified. It reads that way *because* it is specific, which is exactly
why it was not re-counted: the precision was doing the work that checking should
have done.

**Distinguish a snapshot from a standing claim, and prefer the snapshot.** *As
of this entry, twelve examples* cannot go stale; *the suite has four test
scripts* is stale the moment a fifth arrives. Where a record has to describe how
something is wired now, describe the **shape** and let the count live in a
check, or in nothing.

**And the close-out is where this gets caught, or it does not get caught.**
Entry 14's lesson was that a gap recorded only in prose stays open; this is the
narrower version: **prose that describes the tree drifts from the tree, and the
only thing that reliably notices is somebody running a command instead of
re-reading the sentence.** Today's close-out found four stale claims in the four
minutes it spent grepping and none in the hour it spent writing.

---

## 15 · A prediction that held in every part, and the one measurement that is why

**Issue.** [direction.md](direction.md) closes with a standing lesson from three
estimates taken the same day: *this page was right about every shape and wrong
about every distance. Structure can be reasoned about from the code; distance is
what you find on the way.* Stage 3 was laid out as options before it was built,
and the layout made four claims that were not distance claims at all:

- the pattern side is small and the **lexer side is the whole cost**, and is the
  same cost under every notation considered;
- nesting would fall out, because an inner block consumes its own dedent;
- `b:block "else"` would work, because a block ends itself and so is not greedy;
- a blank or comment-only line would close nothing.

All four held, and the build came in at roughly the size the layout said: about
sixty lines of lexer, thirty of header, twenty of parser.

**Root cause: of the accuracy, which is the part worth explaining.** The layout
was not written from reading. Before it, two files were run: one showing that
Python's expressions already read with no tool change, and one **rehearsing the
whole feature by hand**: a `.pt` whose blocks were marked by two ordinary quoted
words, placed where an indent-aware lexer would later synthesise them. That file
produced correct nested C on its first run. Every claim above except the
blank-line one was *observed in that rehearsal*, not predicted: nesting,
`terminated` after a block, a word following a block, and the separator rule
that lets a dedent stand alone were all already true of the tool as it stood.

So the layout was not a forecast that happened to be right. It was a report on
something that had been run, written in the future tense.

**And what the same rehearsal found that no amount of reading would have.** The
item being planned listed three obstacles to reading Python. There was a fourth:
a call wrapped across two lines is `no rule reads '\n' here`, and it turned up
by typing one into a test file. It is now [ROADMAP.md](ROADMAP.md) 2. Two more
of the same kind arrived later and from the same direction: a code template's
`emit` hands back a repeated group's turns **concatenated with no joiner**,
where a string template splices them with the group's `join`; and a Python
function called `double` becomes a C function called `double`, which a compiler
said and nothing here could have.

**Learnings.**

**Reading the code predicts structure. Running it produces the list.** Both
halves of that showed up within an hour of each other, in the same task, and
neither substitutes for the other. The four structural claims came off the code
and held; all four things the plan was *missing* came off running it, and every
one was cheap to find and would have been expensive to reason toward.

**A plan is worth what was run before it.** The rule this suggests is small and
mechanical: before laying out options for a feature, spend the cheapest thing
that can be executed on the question, and write the options *afterwards*. The
cheapest thing is a hand-written rehearsal, a file that fakes the missing half.
The cost here was two scratch files and about ten minutes, and it is the entire
difference between this entry and the three that direction.md is still
apologising for.

**And keep the rehearsal where a clone can find it.** The first draft of this
entry cited both files in `scratch/`, which is gitignored, so the evidence for
the day's main decision would have been readable by exactly one machine. The one
that fakes the block markers is now inlined in [COMPLETED.md](COMPLETED.md),
beside the decline it argues against, because it is the version that *works* and
that is the only honest way to file an argument against something. **Evidence
cited from outside the repository is a citation, not a record**, the same shape
as entry 14, one directory over.

---

## 14 · The suite could not report the one failure it could not express, and that sat in a commit message for a day

**Issue.** `make check` had no time limit anywhere: not in the Makefile, not in
any of the four test scripts. A `.pt` file that never terminates did not make it
go red; it made it *stop*, which from outside is indistinguishable from a slow
machine. This was known: the defect that proved it was found and fixed on
2026-09-05, `COMPLETED.md` records that it "surfaced as the first test hanging
rather than as a wrong answer, which is the one kind of failure this tree's
recorded outputs cannot express", and that morning's close-out commit is titled
*a suite that cannot report a hang*.

Nothing was done about it for the rest of the day, across eleven further
commits, because the observation lived in a commit title and a paragraph and
never became an item anybody would meet again.

**What found it.** Not a test. A `ps` listing, run for an unrelated reason nine
hours later, showing the original hung process still spinning: 4½ hours elapsed,
205 minutes of CPU, holding a pre-fix binary in memory that no longer existed on
disk. The bug had been fixed hours earlier; only the process outlived it,
because nothing reaps a background job whose session has ended.

**Root cause.** Two, and they are the same shape. The suite compares outputs, so
it can only report failures that *are* outputs: a hang produces none. And the
observation that this was a gap was recorded where nothing would act on it.

**Solution.** `tests/limit.sh`, and every one of the six places the suite runs
`pt` now goes through it. Exit 124 is a hang, reported in its own words rather
than as an empty diff. It costs nothing when nothing hangs: the full run is 2.3
seconds.

**Two things went wrong while building it**, both worth keeping.

The watchdog inherited stdout, so under `x=$(limit.sh …)` the command
substitution waited for the *sleeper* to release the pipe, every case paying the
full limit even after finishing in milliseconds. A 62-case run went from two
seconds to over ten minutes. **A background process holds the pipe whether or
not it ever writes to it.**

And `kill -9` on the child left the child's own children running. That is the
exact orphan this file exists because of, about to be recreated by the thing
policing it, so the command is started under `set -m` and the group is killed,
not the process.

**Learnings.** **A known gap recorded only in prose is a gap that stays open.**
This is POSTMORTEM 13's lesson arriving from the other end on the same day: that
entry was about a warning in a comment that was accurate and did nothing; this
is about an accurate observation in a commit message that did nothing. Both were
right, both were well written, and neither was *downstream* of anything. What
closed this one was not better prose but a check that fails.

The narrower one: **a test suite can only report the failure modes its output
format can express.** Recorded `.out` files catch a changed answer. Compiling
and running catches a wrong one. Neither can say *did not terminate*, and the
gap was invisible precisely because every existing check was passing.

## 13 · A file warned about its own trap, and the trap still had to be stepped around by hand

**Issue.** `examples/pascal.pt` reads a `case` arm with a general `a ":" s`
rule, which is only safe because `a ":" "integer"` is declared above it: both
patterns are three elements long, so declaration *order* is what decides. The
file said so in a comment, and finished the thought:

> Add a second type and it goes above this line too — and nothing enforces that,
> which is the other half of what the workaround costs.

A second type arrived on 2026-09-05. `real` went above that line, and nothing
enforced it. Had it gone below, `k: real` would have been read as a case arm and
the file would have emitted `case k: double; break;`, which compiles about as
well as it reads, but only because this program happens to use `real` in a place
the compiler objects to. A different program gets silently wrong output.

**Root cause.** Nothing here can express *this rule must be tried before that
one*. Rules are found longest-first with declaration order breaking the tie, so
a tie between two three-element patterns is settled by a fact about the file's
layout that no directive states and no check reads.

**Solution.** None, and that is the entry. The comment was updated to record
that its own prediction came due and was still handled by hand.

**Learnings.** **A warning in a comment is not a mechanism, and writing an
accurate one does not discharge the risk.** This is the best case for a comment:
specific, correctly located, correctly predicting both the trigger and the
consequence. It still did nothing except be right. What made it safe was that
somebody read it at the moment they were changing that file.

Worth separating from the other thing this repository does with limits, which
*does* work: `bump: 105 0` and the `it''s` literal are recorded in output and
pinned by a test, so a change trips something. The difference is not
documentation versus code; it is **whether the record is downstream of the thing
it describes**. A pinned output is; a comment beside the rule is not.

The narrow version, for when this is met again: **an ordering constraint between
two declarations is invisible to every check here.** Anything relying on one
should say so at the point of the *later* declaration, which is where somebody
inserting a third will be looking.

## 12 · A `for` inside a template read through a null rule and crashed

**Issue.** `@template` landed on 2026-09-05 with `for` legal in a template body,
and any template that actually wrote one segfaulted, at seal, before a line of
input was read. `examples/asm.pt`, the customer the feature was built for, has
no loop in any of its templates, so nothing found it for a day.

```
@template sub(p) { for x in p sep ", " { emit x } }     ->     SIGSEGV
```

**Root cause.** `check_block` is called with the rule a body belongs to, and
with `NULL` for a template. A template has no rule, which is the whole point: it
sees its parameters and nothing else. `check_expr` was written for that and
guards every use with `r &&`. The `for` branch, added in the same commit, calls
`rule_has_hole(r, nm)` unguarded to refuse a loop variable that shadows a hole.
For a template there are no holes to shadow, so the call is not merely unsafe,
it is asking a question that does not apply.

**Solution.** The same `r &&` guard the neighbouring branch already had, and a
comment saying why a template has nothing to collide with. The check still
applies in full to a rule's own code template, which is where it was earned.

**What found it.** Deduplicating `examples/code.pt`: `procedure` and `function`
had the same four-line body differing only in C's return type, so one
`@template` should take the difference as a parameter, and its body has a loop
over the parameter list. The example now compiles and runs under
`tests/pascal.sh`, so a CPU checks the fix rather than a diff.

**Learnings.** **A feature is only tested to the depth of the one file that
asked for it.** `@template` was built because `asm.pt` wrote two lines eight
times, so the customer exercised calls, parameters, `if`, `emit` and `fresh`,
and never `for`, because the eight sites had nothing to loop over. Every branch
of the new code that the customer did not walk shipped unexecuted, and one of
them was a crash. The tree's habit of building only what a customer asked for is
still right; what it does not do by itself is *cover* what was built.

The narrow version is worth stating separately, because it is mechanical and
will recur: **when a parameter can be `NULL` for one caller, every use in that
function is a site, not just the ones written first.** Two branches of one
function were added in one commit; the older one guarded and the newer one did
not.

## 11 · One page, three predictions, one afternoon: right about the shape, wrong about every distance

**Issue.** `docs/direction.md` was written on 2026-09-05 to say what Prototype
could become. Within hours three of its claims met evidence, and it was a good
day for the page and a bad day for its estimates.

| it said | outcome |
| --- | --- |
| stage 2 will show whether the output side generalises past targets shaped like the input | **held**: it does, and nothing in the tool had to change |
| arithmetic and `num(h)` make this an interpreter generator; the gap is *exactly one roadmap item* | **failed**: it makes a calculator |
| named fragments may be one mechanic or two | **failed as stated**: two, and building one settled it |

The second is the instructive one. Arithmetic landed and read well, which was
the risk the item had named. It was not the risk: a hole is expanded before the
template that uses it runs, so a rule can select between computed values and
cannot leave one uncomputed. `if 1 then 10 else (1 / 0)` divides by zero.
Evaluation is eager, and everything an interpreter needs that a calculator does
not is a thing that must *not* happen.

**Root cause.** The page reasoned correctly about **structure**: that the
attribute flowing up could be a value, that this is a bottom-up attribute
grammar, that the missing thing is naming. Then it guessed at **distance** from
the same armchair. Structure is a property of the code and can be reasoned
about; distance is a property of what you find on the way, and cannot. Every
distance it named was wrong and every shape it named was right.

**Solution.** Each claim was rewritten in place rather than appended to, which
is the rule that page opens with, and each rewrite records which risk was
predicted and which one actually bit. The three artefacts that did the refuting
are `examples/calc.pt`, `examples/asm.pt` and `@template` itself: files, not
arguments.

**Learnings.** **A prediction is worth writing down in proportion to how cheaply
it can be shown wrong, and the way to show it wrong is to build the smallest
thing that tests it.** Not one of the three was settled by thinking about it
harder. The arithmetic item spent a paragraph arguing about whether the notation
would read well, argued the question well, and was arguing about the wrong
thing, which nothing but running it could have revealed.

The second lesson is narrower and is about the shape of a plan document.
**Separate the claim about structure from the claim about distance**, because
they have different epistemic status and the second is nearly always the one
that is wrong. Had the page said *the gap is deferral or arithmetic, we do not
know which* instead of *exactly one roadmap item*, it would have been right, and
it had every fact needed to say so.

---

## 10 · Two spellings called the same thing, one of which nobody had ever used

**Issue.** `{~t}` in a string template and `fresh("t")` in a code template are
documented as the same feature: *a name nobody else has*, one name per label per
application. REFERENCE.md § 8.2 said so in as many words: *`fresh("t")` is the
same thing in a code template*. It was not. `{~t}` was memoised per application,
as documented; `fresh("t")` called the generator afresh every time, so two calls
in one template gave two names.

That makes a whole class of template unwritable, and it is not an obscure one:
anything needing a **label at a branch and at the place the branch jumps to**.
Writing `examples/asm.pt` walked straight into it on the first conditional.

**Root cause.** `fresh()` had **never been used**. `git grep 'fresh('` across
`examples/`, `lib/` and `tests/` at the commit before this one returns nothing;
`{~t}` returns two examples, one of which `tests/hygiene.sh` compiles and runs.
So the exercised half was right and the unexercised half had drifted, and the
sentence claiming they were the same is what stopped anybody looking. It reads
as a fact about the implementation and is in fact a promise about two code paths
that never met.

There was a second, worse thing in the same three lines: `pt_fresh` can return
NULL when the name space is exhausted, `subst` checks it, and the code template
did not: it passed NULL to `v_text`, which dereferences it. Unreachable in
practice, and only because the feature had no users.

**Solution.** `Ev` keeps the same per-application memo `subst` keeps, so the two
spellings now genuinely are one behaviour; exhaustion is an error rather than a
crash; and `examples/asm.pt` uses `fresh` in anger, twice per conditional, with
`tests/asm.sh` checking that two conditionals produce four label lines under two
distinct names.

**Learnings.** **An equivalence claim is a test that has not been written.**
"X is the same as Y" asserts that two implementations agree, and unless something
exercises both, it is exactly as reliable as the one nobody ran. The tell here was
available and cheap: a feature named in the reference and used in no `.pt` file
in the repository is a feature whose documented behaviour is a guess.

The general form is worth more than this instance. This tree's examples are its
test suite, so **a builtin with no example is untested by construction**. The
builtin table in REFERENCE.md § 8.3 is the list to audit against `git grep`, and
doing that is minutes. What made this one findable at all was writing a program
that needed it; what would have found it a day earlier is asking which entries
in that table no file uses.

That audit has now been run: every builtin in § 8.3 is used by at least one
example, `fresh` by `examples/asm.pt`. It took a minute and it is the kind of
thing that should be re-run whenever the table grows.

---

## 9 · A method proposed in the morning and scored the same evening

**Issue.** Hans introduced staging on 2026-09-05: one translator at a time,
taken far enough to be compiled and run, *"and that way we can work on
introducing new mechanics and test them out properly."* That is a prediction,
that pressure from a single finished translator surfaces the right features, and
it met evidence within hours, which almost never happens to a claim about
method.

**It held, and it held in the way that is hard to arrange deliberately.** Two
mechanics were built the same day and both had a customer that asked before the
feature existed. `terminated(h)` came from C wanting a semicolon before `else`
and forbidding one after a block, a distinction that depends on the rule that
filled the hole. `for i, x in h` with `at(h, n)` came from `case` arms being two
holes in one repeated group and therefore two parallel lists. Neither was on the
roadmap that morning. Neither would have been guessed at, and the shape of both
was decided by the thing that needed them rather than by what seemed general.

**And a prediction inside it failed, which is why it worked.** The roadmap had
said stage 1 needed no new mechanics: `procedure`, `function`, `repeat` and
`case` were *"rules nobody has written yet"*. Three of the four were exactly
that. `case` was not: its arms want to be `[ v ":" s ]*` and cannot be, and the
workaround is an infix rule whose correctness depends on the order of
declarations in its own file. The failure is what produced the second mechanic.
A schedule that had only confirmed its own predictions would have produced
nothing.

**Root cause of the prediction being scorable at all.** It was written down as a
claim about the work rather than as an intention: *what is left to write, none
of which needs new mechanics*, naming four items and a reason. A sentence in
that shape can be wrong in public. "We should focus" cannot.

**Learnings.** **A method is a prediction and should be recorded as one, with
the thing it claims will happen written down before it does.** The staging note
in ROADMAP.md names which translator, in which order, and what each is for, so
by the evening it could be checked rather than believed.

The narrower one is about *where* to make predictions falsifiable. Listing four
things and asserting they need nothing new is a bet with four outcomes; three
held, one paid. Compare the alternative sentence, "the rest of Pascal should be
straightforward", which is unfalsifiable and would have taught nothing when
`case` turned out not to be.

---

## 8 · Three defects a diff could not have seen, and one it structurally cannot

**Issue.** Hans's argument for a C target on 2026-09-05 was that *"C as output
is good because it can be tested that the output is correct."* `tests/pascal.sh`
was written to do it: expand, compile, run, check the number. It found three
things the same day.

1. **`if (c) x = 1 else x = 2`.** Not C: a branch that is an expression wants a
   `;` before the `else` and a branch that is a block must not have one. Both
   example files had emitted it since the day they were written.
2. **A block's last statement had no semicolon.** A separator goes *between* two
   statements and never after the last, and `begin … end` closed straight over
   it. Also older than the test.
3. **A hang.** A `for` with an index puts two frames on the environment, and the
   restore pointed at the first frame rather than at what was there before both,
   so the second turn linked a frame to itself and `lookup` walked a cycle.

**Root cause.** The first two are the same cause as POSTMORTEM 5: the output was
*plausible*. Every recorded `.out` in this tree was green through both of them,
because a `.out` pins what the expansion **is** and says nothing about whether
it is **right**. A diff catches change. Only a test that runs the output catches
wrongness, and until 2026-09-05 `tests/hygiene.sh` was the only one here that
did.

The third has a different cause and is the more interesting one. It was not a
wrong answer; it was no answer. **A tree of recorded outputs has no `.out` for
*did not terminate*.** There is no expansion to compare, the harness waits, and
the failure is invisible to the format the entire suite is built on.

**Solution.** `tests/pascal.sh`, on `tests/hygiene.sh`'s model: compile what
`examples/code.pt` emits, run it, and check the numbers the Pascal computes,
worked out from the Pascal and not from the C. The half that must *not* compile,
`examples/pascal.pt`, which cannot spell C's quotes, is pinned too, so that
whoever fixes it has to edit the test in the same commit.

**Learnings.** **A recorded output is a regression test and not a correctness
test, and the difference is invisible until something else checks.** Two of
these three predate the test by days and sat under a green suite the whole time.
Where a target language can be executed, executing it is not a nicety; it is the
only part of the suite that can disagree with the expansion rather than with
yesterday's expansion.

And the narrower one, which cost fifteen minutes of confusion before it was
understood: **a suite whose failure mode is a diff cannot report a hang.** Any
harness built on recorded outputs needs a clock somewhere, or the one bug class
it cannot express is the one that stops the run.

---

## 7 · A page that said it was checked, opening with an example that never ran

**Issue.** REFERENCE.md began *Everything here is checked by `make check`*, and
its § 1, the first thing anybody reads, was a complete `.pt` file with its
output printed under it. Typed out and run, it fails on its second line: nothing
in it declares `=`, so `x = 1` is `nothing here is anything this file declared`.
It also declared `;` as both a comment opener and the statement separator, and
comments are looked for first, so the separator could never have fired either.
The printed output was what the file would have produced if it had worked, which
is why it looked right.

**Root cause.** The sentence was true of the *behaviour* the page describes:
every rule and message in it is exercised by `examples/` and `tests/`. It read
as if it were true of the *examples* in it, which nothing ran. § 1 was the only
whole file on the page and the only one that could have been run, and it was
written by hand rather than lifted from `examples/`, so it was the one snippet
with no file behind it and the one that rotted.

**Solution.** § 1 is now `examples/first.pt`, a real file with a recorded
output, so `make check` runs it and the block on the page is that file. The
promise was rewritten to say what is actually checked and what is a fragment.
The rest of the page's snippets are fragments that could not be run as they
stand; a harness for one file would have been machinery around a single case.

**Learnings.** **A claim that something is verified is itself a claim, and it is
the one nobody thinks to verify.** It reads as provenance rather than as an
assertion, so it is trusted in exactly the place where the reader would
otherwise have been sceptical, and it made this defect *less* likely to be found
than if the page had promised nothing.

The general form is that documentation is checkable only where it is
**executed**, not where it is careful. Every other example on the page is a
fragment and stays unchecked, which is now written down rather than papered
over; the one that can be run is the one that is run. Where a document wants to
be trusted about behaviour, the way to earn it is to make the artifact the
document quotes *be* the artifact the suite runs, which is what `examples/*.out`
already does everywhere else in this tree, and what § 1 was the single exception
to.

---

## 6 · A roadmap item that described its own defect backwards

**Issue.** The roadmap said: *`@use` two files that both declare `"+"` and the
later one wins, silently.* For a **rule** that is the opposite of what happens.
Candidates under one leading word are tried longest-pattern-first with
declaration order breaking a tie, so the **earlier** wins and the later is
unreachable: the file that wrote the second template silently gets the first
one's output. The sentence was true of `@token` and `@separator`, which do
replace in place, and it was written in one breath covering all three.

Had the fix been built to the description, it would have been built to make the
later win, which it already did in two cases out of three and never did in the
one the example was about.

**Root cause.** The item was written from the shape of the code and not from
running it: three declarations, one array, an obvious hazard. Nothing in the
tree contradicted it, because the case had no test and no example: it was on the
roadmap precisely *because* nothing exercised it.

A second thing hid inside the same item. `@use` read a file once per route to
it, so a diamond declared everything in the shared file twice. Any collision
rule would have fired on that first, and it would have looked like the rule
working.

**Solution.** Run the case before believing the description of it: four `.pt`
files, both orders, three directives, ten minutes. The behaviour was the
opposite in one case and undefined-looking in another, and both were then
written into the item's own record before a line of the fix was written.

**Learnings.** **A record of what is broken goes stale exactly like a record of
what works, and has one fewer thing checking it.** REFERENCE.md is read against
the code every time somebody uses the tool; ROADMAP.md describes behaviour
nobody exercises, by construction, so an error in it can survive any amount of
green. What would have caught this is the thing that did: **the first step of
fixing a defect is reproducing it**, and the roadmap entry should have carried
the two-file reproduction that produced it rather than a sentence about it.

The narrower lesson is about *aggregation*. The claim was accurate for two of
the three things it covered, which is how it read as true; a sentence that says
"declarations" when the tool has three kinds with two different resolution
rules is a sentence that cannot be checked without being split first.

---

## 5 · Two changes that looked right because what they broke was silent

**Issue.** Two on the same afternoon, hours apart.

`terminated` was added and appeared to work. It did not work on code templates:
`code_parse` returned the position after its own lookahead token rather than
after the closing brace, so in `=> { … } terminated` the word was swallowed by
the template's lexer and the flag was never set. The two examples converted
first were string templates, where it was fine.

Then a bound on text-mode holes, *a hole may not span any word still to come*,
was shipped, and the next commit's own example stopped working. In `"![" alt
"](" src [ " " title ] ")"` the group's space is a word still to come and an alt
text may contain spaces, so `![a cat](cat.png)` no longer matched at all.

**Root cause.** Different bugs, one shape: **the thing they broke fails
silently.** A flag that is not set produces output that is merely unchanged. A
text-mode rule that does not match produces text that is merely copied through,
because that is what text mode does with anything no rule claims. Neither turns
red, neither raises anything, and both look exactly like a feature working on
input it does not apply to.

**Solution.** `code_parse` records the position before its lookahead. The bound
became *a hole may not span the word that closes the rule*, the one word whose
arrival means the construct has ended, which fixes the defect it was written for
without forbidding a space inside an alt text.

**Learnings.** Both were found by probing by hand and reading the output
closely, and neither could have been found by `make check`, because in both
cases the recorded output was *plausible*. **Where a feature's failure mode is
"nothing happens", passing tests are not evidence that it works**. Only an
example whose expected output would visibly differ is. The `terminated` bug in
particular survived because the feature applies to two template forms and was
exercised in one; a thing that applies to two shapes needs a case in each, and
the second case is the one nobody writes.

The second is also a warning about the first fix in a pair. The bound was
written to fix `POSTMORTEM.md` 4, shipped in its own commit with its own
regression test, and was still wrong: stricter than the defect required. A fix
that is broader than the evidence that demanded it is a guess wearing a test.

---
## 4 · A rule that was wrong, doing exactly what it said

**Issue.** In text mode a hole stopped at *the first occurrence of the pattern's
next word*, and nothing else. Given two rules for a wiki link, the labelled
`"[[" t "|" u "]]"` declared before the bare `"[[" t "]]"`, this input

```
A plain [[here]] and a bar|pipe later.
A labelled [[url|label]] too.
```

came out as

```
A plain <a href="pipe later.
A labelled [[url|label">here</a> too.
```

The labelled rule matched `[[here]]`: `t` went looking for a `|`, did not find
one before the `]]`, kept going, found the one in `bar|pipe` a line later, and
swallowed the close and everything between. Silent, and wrong.

**Root cause.** Not a code defect. `REFERENCE.md` §7 said *stops at the
pattern's next word — the first occurrence, not the last*, and that is precisely <!-- as written -->
what the code did. **The rule itself was wrong**, which is the harder kind: there
was nothing to notice by reading the code against the documentation, because
they agreed.

It was also nearly invisible. `examples/poem.pt` had no rule that could fail
partway: every pattern there is `"x" hole "x"`, where the terminator is the only
word left to find, so the whole suite passed while the rule was wrong for any
pattern with three words in it.

**Solution.** A hole stops at the earliest of **every** word still to come in
its pattern, and fails if what stops it is not its own terminator. A `]]`
reached before the `|` means the construct has ended. `examples/poem.pt` now
declares both link forms and carries the input above.

**Learnings.** The first thing I told Hans about this was that it was a
correctness bug in the matcher. It was not: the matcher did what it was
documented to do, and it took reading §7 to see that. **A defect found by
staring at output should be checked against the specification before it is
called a bug, because "the code is wrong" and "the rule is wrong" want different
fixes**: one is a patch and the other is a decision, and only the second has to
be written down somewhere a reader will meet it.

The example that was supposed to cover this feature could not have caught it.
Every rule in `poem.pt` had exactly one word after its hole, so the difference
between *the next word* and *every later word* did not exist in the test data. A
feature demonstrated only in its easy shape is untested in its real one.

---
## 3 · An unimplemented feature that was two features

**Issue.** [notation.md](notation.md) recorded hygiene as a single open problem
with a single fix, *either the template gets a way to ask for a fresh name
(`{~t}` ) or agnosticism costs hygiene*, and admitted in the same paragraph that
the gap had been reasoned about and not tested. Building the test showed the
sentence was wrong: there are two failures, and `{~t}` closes one of them and
cannot close the other.

**Root cause.** The two failures look identical from outside, a form's expansion
colliding with a caller's name, so one example seemed to cover both. They are
not the same thing. A template that *introduces* a name needs a name nobody else
has, which a template can be handed. A template that *reaches out* for a name
the caller shadowed needs to know what a scope is, which a template that is a
string cannot be handed at all. Reasoning about the symptom found one mechanism
where there were two.

**Solution.** `examples/hygiene.pt` declares both forms; `tests/hygiene.sh`
compiles the C they expand to and runs it. `{~t}` was then built, and the second
half moved out of "Not done" and into "What it costs", because it is the price
of being agnostic and not a feature nobody has written yet.

**Learnings.** *Unsolved* and *untested* are different states, and the gap
between them hid a structural fact rather than a detail. A problem described
only in prose can be described as one problem when it is two, and nothing in the
prose will say so: running it is what splits them. The test is written to pin
the **wrong** answer for the half that is still wrong, so that fixing it forces
an edit to the test in the same commit; a test that merely failed would have
been switched off.

---

## 2 · Two claims that survived a passing suite and were killed by a document

**Issue.** Writing [REFERENCE.md](REFERENCE.md) meant checking every statement
against the code instead of against memory. Two were false.

Text mode tried rules in declaration order, so a file declaring `-` before `---`
turned `a --- b` into three hyphens, the opposite of the maximal munch the lexer
had been doing in expression mode all along, and of what the documentation said
the tool did. And the `@use` limit incremented a counter that was never
decremented, so the real ceiling was 64 *used files in a run* while three
documents said "64 deep".

**Root cause.** Text mode was written as a separate scanner and did not inherit
the rule the lexer had already been given, because nothing forced the two to be
described in one place until a reference existed. The `@use` counter is the
plainer failure: the name `nfiles` said what it counted and the prose said
something else, and no test distinguished them because no test went past one
level.

**Solution.** Longest leading word wins in text mode, declaration order breaking
ties only. `examples/poem.pt` now declares `-`, `--` and `---` in that order and
pins the outcome, so the bug cannot come back quietly. `@use` decrements on the
way out.

**Learnings.** **A reference is a test.** Four commits and a green suite did not
find either of these; writing down every claim and checking it found both in one
pass, and each was a one-line fix once seen. The suite could not have found
them, because both were places where the code and the *intended rule* differed
and no example exercised the difference, which is exactly the shape a document
catches and a test does not, unless somebody first knows to write the test.

---

## 1 · A parse failure that wanted a rule, not a patch

**Issue.** `examples/clike.pt` would not parse. `for (…) { … }` followed by
`total.print;` failed, because the statement loop required a separator between
every pair of statements and C's block statements carry none.

**Root cause.** The separator rule had been taken from Proto, where `.` between
statements is unconditional. It is unconditional there because Solveig has no
self-terminating statement; C and Pascal both do, and a language-agnostic tool
meets one on its first real file.

**Solution.** *A separator is wanted between two statements, and not after one
that ended in a word.* Stated in [REFERENCE.md](REFERENCE.md) §6.3, implemented
in four lines.

**Learnings.** The first instinct was to make `examples/clike.pt` write the
semicolons, which would have compiled, passed, and quietly made the tool unable
to read C. A failure on the first realistic input is evidence about the rule and
not about the input, and the cost of getting that backwards is a tool that only
reads the files written to suit it. Proto's `conventions.md` puts the general
form of this as *a surface does not grow without a customer*; the converse is
that a customer who cannot be served is telling you about the surface.
