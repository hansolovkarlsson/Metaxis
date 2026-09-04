# Postmortem

*The **scoring**. One entry per mistake or prediction that has met evidence — one
that held, one that failed, or one that held and then failed — under **Issue**,
**Root cause**, **Solution** and **Learnings**. Not a bug log: a defect belongs
here when what it taught outlives it, and a day that scored nothing adds
nothing. The learning is the part that has to be true a year from now, so it
says what would have caught the thing rather than resolving to be more careful.*

Newest first.

---

## 5 · Two changes that looked right because what they broke was silent

**Issue.** Two on the same afternoon, hours apart.

`terminated` was added and appeared to work. It did not work on code templates:
`code_parse` returned the position after its own lookahead token rather than
after the closing brace, so in `=> { … } terminated` the word was swallowed by
the template's lexer and the flag was never set. The two examples converted
first were string templates, where it was fine.

Then a bound on text-mode holes — *a hole may not span any word still to come* —
was shipped, and the next commit's own example stopped working. In
`"![" alt "](" src [ " " title ] ")"` the group's space is a word still to come
and an alt text may contain spaces, so `![a cat](cat.png)` no longer matched at
all.

**Root cause.** Different bugs, one shape: **the thing they broke fails
silently.** A flag that is not set produces output that is merely unchanged. A
text-mode rule that does not match produces text that is merely copied through,
because that is what text mode does with anything no rule claims. Neither turns
red, neither raises anything, and both look exactly like a feature working on
input it does not apply to.

**Solution.** `code_parse` records the position before its lookahead. The bound
became *a hole may not span the word that closes the rule* — the one word whose
arrival means the construct has ended — which fixes the defect it was written
for without forbidding a space inside an alt text.

**Learnings.** Both were found by probing by hand and reading the output
closely, and neither could have been found by `make check`, because in both
cases the recorded output was *plausible*. **Where a feature's failure mode is
"nothing happens", passing tests are not evidence that it works** — only an
example whose expected output would visibly differ is. The `terminated` bug in
particular survived because the feature applies to two template forms and was
exercised in one; a thing that applies to two shapes needs a case in each, and
the second case is the one nobody writes.

The second is also a warning about the first fix in a pair. The bound was
written to fix `POSTMORTEM.md` 4, shipped in its own commit with its own
regression test, and was still wrong — stricter than the defect required. A fix
that is broader than the evidence that demanded it is a guess wearing a test.

---
## 4 · A rule that was wrong, doing exactly what it said

**Issue.** In text mode a hole stopped at *the first occurrence of the pattern's
next word*, and nothing else. Given two rules for a wiki link — the labelled
`"[[" t "|" u "]]"` declared before the bare `"[[" t "]]"` — this input

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
pattern's next word — the first occurrence, not the last*, and that is precisely
what the code did. **The rule itself was wrong**, which is the harder kind: there
was nothing to notice by reading the code against the documentation, because
they agreed.

It was also nearly invisible. `examples/poem.pt` had no rule that could fail
partway — every pattern there is `"x" hole "x"`, where the terminator is the
only word left to find — so the whole suite passed while the rule was wrong for
any pattern with three words in it.

**Solution.** A hole stops at the earliest of **every** word still to come in
its pattern, and fails if what stops it is not its own terminator. A `]]`
reached before the `|` means the construct has ended. `examples/poem.pt` now
declares both link forms and carries the input above.

**Learnings.** The first thing I told Hans about this was that it was a
correctness bug in the matcher. It was not: the matcher did what it was
documented to do, and it took reading §7 to see that. **A defect found by
staring at output should be checked against the specification before it is
called a bug, because "the code is wrong" and "the rule is wrong" want different
fixes** — one is a patch and the other is a decision, and only the second has to
be written down somewhere a reader will meet it.

The example that was supposed to cover this feature could not have caught it.
Every rule in `poem.pt` had exactly one word after its hole, so the difference
between *the next word* and *every later word* did not exist in the test data. A
feature demonstrated only in its easy shape is untested in its real one.

---
## 3 · An unimplemented feature that was two features

**Issue.** [notation.md](notation.md) recorded hygiene as a single open problem
with a single fix — *either the template gets a way to ask for a fresh name
(`{~t}` ) or agnosticism costs hygiene* — and admitted in the same paragraph
that the gap had been reasoned about and not tested. Building the test showed
the sentence was wrong: there are two failures, and `{~t}` closes one of them
and cannot close the other.

**Root cause.** The two failures look identical from outside — a form's
expansion collides with a caller's name — so one example seemed to cover both.
They are not the same thing. A template that *introduces* a name needs a name
nobody else has, which a template can be handed. A template that *reaches out*
for a name the caller shadowed needs to know what a scope is, which a template
that is a string cannot be handed at all. Reasoning about the symptom found one
mechanism where there were two.

**Solution.** `examples/hygiene.pt` declares both forms; `tests/hygiene.sh`
compiles the C they expand to and runs it. `{~t}` was then built, and the second
half moved out of "Not done" and into "What it costs", because it is the price
of being agnostic and not a feature nobody has written yet.

**Learnings.** *Unsolved* and *untested* are different states, and the gap
between them hid a structural fact rather than a detail. A problem described
only in prose can be described as one problem when it is two, and nothing in the
prose will say so — running it is what splits them. The test is written to pin
the **wrong** answer for the half that is still wrong, so that fixing it forces
an edit to the test in the same commit; a test that merely failed would have
been switched off.

---

## 2 · Two claims that survived a passing suite and were killed by a document

**Issue.** Writing [REFERENCE.md](REFERENCE.md) meant checking every statement
against the code instead of against memory. Two were false.

Text mode tried rules in declaration order, so a file declaring `-` before `---`
turned `a --- b` into three hyphens — the opposite of the maximal munch the
lexer had been doing in expression mode all along, and of what the documentation
said the tool did. And the `@use` limit incremented a counter that was never
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
and no example exercised the difference — which is exactly the shape a document
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
semicolons — which would have compiled, passed, and quietly made the tool unable
to read C. A failure on the first realistic input is evidence about the rule and
not about the input, and the cost of getting that backwards is a tool that only
reads the files written to suit it. Proto's `conventions.md` puts the general
form of this as *a surface does not grow without a customer*; the converse is
that a customer who cannot be served is telling you about the surface.
