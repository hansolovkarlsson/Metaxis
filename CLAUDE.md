# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## Start here

`scratch/daily-standup.md` — written at the end of the previous working day to
be read at the start of the next: where the tree was left, what went in, and
what is outstanding. `scratch/` is gitignored and is not part of this
repository, so the file is absent on a fresh clone and on any day that was not
closed out. When it is absent, `git log` and the documents named below are the
way in.

## What this is

A language-agnostic rewriter in the shape of Proto: a `.mx` file declares its
own grammar in its header and is then read with it, and the output is whatever
the templates say. Its one premise is that **every mention of foreign text
inside a directive is a string** — quoted words on the pattern side, a quoted
template with `{hole}` splices on the output side — so that a directive can
never be read as the thing it is declaring.

## Commands

`make`, `make check` (`make test` is the same target), `make record`,
`make clean`. `make check` runs every example against the `.out` recorded beside
it, then the nine scripts in `tests/` — `errors.sh`, `hygiene.sh`, `docs.sh`,
`pascal.sh`, `asm.sh`, `python.sh`, `basic.sh`, `island.sh` and `scale.sh`. Six
of them **run what they produced** rather than diffing it, which is how a wrong
answer is caught and not merely a changed one; `hygiene.sh` also checks properties of the tree,
`docs.sh` runs every `$ mx …` transcript in `docs/` against what it claims to
print, and `scale.sh` expands one input large enough for a quadratic to show —
the suite is otherwise small enough by construction that it cannot see one.
Every invocation goes through `tests/limit.sh`, so a hang is reported instead of
waited on; `make check LIMIT=30` raises the 10s default. `make record`
re-records those `.out` files; read the diff before committing it.

C11 and `make`, plus POSIX `<regex.h>`.

## The website

`site/build.py` renders `docs/*.md`, `site/index.md` and `examples/` into
`site/out/` (gitignored); `.github/workflows/site.yml` publishes that to GitHub
Pages on every push to `main` that passes `make check`. Nothing is written for
the site alone: a page is a document in the tree, rendered.

## Proto is read-only

`../Proto` is another process's working tree. **Read it, do not write to it** —
no edits, no branches, no commits, not even to files that look untouched. What
comes back from there is insight and prior art, nothing else.

## The records

In `docs/`: `work-journal/` (why, in order — **one file per working day**),
`POSTMORTEM.md` (what a mistake or a prediction taught), `COMPLETED.md` and
`ROADMAP.md` (what exists and what does not — an item moves when it is settled,
including settled against), `CHANGELOG.md` (when).

Six more are about the thing rather than the work: `tutorial.md` teaches it
one concept at a time with runnable files under `docs/tutorial/`, `glossary.md`
explains the terms of art the others lean on and the concepts behind them,
`REFERENCE.md` states what every part of a `.mx` file means, `notation.md` argues for why it is
shaped that way and what it costs, `direction.md` argues for where it could go
and which futures are being declined, and `prior-art.md` surveys the tools that
do something like this and scores this one against them. Where any of them
disagrees with the code, the code is right.

Each of those opens with a note stating its own job. That note is the
specification for what belongs in it — follow it over any general instruction.
The journal's note lives once, in `docs/work-journal/README.md`.

## Style Guide & Constraints

- **Punctuation Restriction:** CLAUDE NEVER USES EM DASHES (—).
- When a pause or structural break is required, ALWAYS substitute the em dash
  with clean alternatives:
  - Use **commas** for minor asynchronous pauses or parenthetical pieces of
    information.
  - Use **periods** to break up fragmented or long thoughts into complete
    sentences.
  - Use **colons** to introduce lists or secondary explanations.
- **Exceptions:** the rule is about general prose. An em dash stays where it
  is part of a fact rather than a sentence: in dates and ranges, in a quotation
  reproduced as written, in a name or title that contains one, and in anything
  that is code, an example, a transcript or recorded output.
