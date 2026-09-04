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

A language-agnostic rewriter in the shape of Proto: a `.pt` file declares its
own grammar in its header and is then read with it, and the output is whatever
the templates say. Its one premise is that **every mention of foreign text
inside a directive is a string** — quoted words on the pattern side, a quoted
template with `{hole}` splices on the output side — so that a directive can
never be read as the thing it is declaring.

## Commands

`make`, `make check` (`make test` is the same target), `make record`,
`make clean`. `make check` runs every example against the `.out` recorded beside
it, then `tests/errors.sh` and `tests/hygiene.sh`. `make record` re-records
those `.out` files; read the diff before committing it.

C11 and `make`, plus POSIX `<regex.h>`.

## Proto is read-only

`../Proto` is another process's working tree. **Read it, do not write to it** —
no edits, no branches, no commits, not even to files that look untouched. What
comes back from there is insight and prior art, nothing else.

## The records

In `docs/`: `work-journal/` (why, in order — **one file per working day**),
`POSTMORTEM.md` (what a mistake or a prediction taught), `COMPLETED.md` and
`ROADMAP.md` (what exists and what does not — an item moves when it is settled,
including settled against), `CHANGELOG.md` (when).

Two more are about the thing rather than the work: `REFERENCE.md` states what
every part of a `.pt` file means, and `notation.md` argues for why it is shaped
that way and what it costs. Where either disagrees with the code, the code is
right.

Each of those opens with a note stating its own job. That note is the
specification for what belongs in it — follow it over any general instruction.
The journal's note lives once, in `docs/work-journal/README.md`.
