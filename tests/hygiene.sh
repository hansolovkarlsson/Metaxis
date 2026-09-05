#!/bin/sh
# hygiene.sh -- the two checks that read the tree instead of running it.
#
# The second one is the file's original job and most of what is below. The
# first is the limit guard, added because it is the same kind of check -- a
# property nothing else here can express -- and a sixth script for one grep
# would have been one too many. See docs/ROADMAP.md 5, which this closes.
#
# ---------------------------------------------------------------------------
# 1 - the limit guard
#
# tests/limit.sh exists so that a hang is reported rather than waited on, and it
# only works where it is actually used. Nothing enforced that. A test script
# added next month that runs the tool directly is a hole in the one guard this
# suite has against the failure no recorded .out can express, and it would pass
# every check here on the day it was written.
#
# This is not hypothetical, which is why it is a check and not a note.
# docs/COMPLETED.md once claimed "all six places the suite runs the tool" go
# through limit.sh. There were seven that morning and eight by the evening, and
# a close-out read the sentence and did not count. POSTMORTEM 16: **a number in
# prose is not a check.** So this counts nothing -- it states the property.
#
# The property: the tool is never *run* except through tests/limit.sh. Three
# shapes name the binary without running it, and each is exempted by name
# rather than by a pattern broad enough to hide a fourth:
#
#   BIN = ... / MX=...     where the path is stored
#   a make rule header     `all: $(BIN)`, `$(BIN): $(OBJ)`, `check: $(BIN)`
#   sh tests/<x>.sh ...    handing the path to a script this same check covers
#
# The third is the loosest of the three -- it would also forgive a line that
# both handed the path over and ran it -- and it is written this way because
# the alternative is a shell parser. If that shape ever appears, tighten it.
#
# ---------------------------------------------------------------------------
# 2 - hygiene: one half fixed, one half charged, run rather than argued.
#
# It expands examples/hygiene.mx, compiles the C that comes out and runs it.
# Three outcomes are named below and each says something different, because
# there are two failures here and they moved at different times.
#
#   swap    a template that INTRODUCES a name. `{~t}` closed this one: the
#           template asks for a name nobody else has, the caller's `t` is not
#           it, and the second call site does not get the first's temporary.
#
#   bump    a template that REACHES OUT for a name the caller shadowed. Not
#           closed, and not closable this way: there is nothing to invent. The
#           template means the binding in scope where the rule was written, and
#           a string has no way to see a scope. Proto closes it because its
#           expander works on trees in a language whose scopes it knows.
#
# **This test passing means `bump` is still wrong.** It is written that way on
# purpose. A test that merely failed would be turned off; one that pins the
# wrong answer has to be edited by whoever fixes it, in the same commit, which
# is the only way the record and the code stay in step. That is what happened
# to the previous version of this file: it pinned `swap: 2 2`, and `{~t}` made
# it wrong, and this is the edit.
#
# The numbers are Proto's. `Proto/examples/forms.pro` demonstrates the same two
# failures against the same expansion, and its comments record #105 and #0 for
# the second -- the same pair the last line here still prints.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"
SRC="${SRC:-examples/hygiene.mx}"

if [ ! -f tests/limit.sh ]; then
    echo "FAILED  hygiene.sh: tests/limit.sh is gone, and everything below"
    echo "        assumes it is there. See the note at the head of this file."
    exit 1
fi

# The bracket classes below are not decoration. This file is one of the files
# being scanned, so a pattern written the obvious way would match its own text
# and report itself -- `grep -v grep`, one level up. Written like this it still
# matches $(BIN), bin/mx and $MX and does not contain any of them.
# `|| exit` is not belt and braces. The first draft of this had a regex awk
# could not parse; awk exited 2, the substitution came back empty, and the guard
# printed `ok`. **A check that passes when its own machinery breaks is worse
# than no check**, because it also silences the one that would have told you.
unguarded=$(awk '
    /[$][(]BIN[)]|bin\/mx|[$]M[X]/ {
        if ($0 ~ /^[[:space:]]*#/)                    next   # a comment runs nothing
        if ($0 ~ /limit[.]sh/)                        next   # guarded
        if ($0 ~ /^[[:space:]]*BIN[[:space:]]*=/)     next   # stored, not run
        if ($0 ~ /^[[:space:]]*M[X]=/)                next   # stored, not run
        if (FILENAME == "Makefile" && $0 ~ /^[^\t ][^=]*:/) next   # rule header
        if ($0 ~ /sh tests\/[a-z]+[.]sh /)            next   # handed on
        printf "%s:%d:%s\n", FILENAME, FNR, $0
    }
' Makefile tests/*.sh) || {
    echo "FAILED  hygiene.sh: the limit guard did not run -- awk exited $?."
    echo "        Its own failure must not read as a pass. See the note above it."
    exit 1
}

if [ -n "$unguarded" ]; then
    echo "FAILED  hygiene.sh: the tool is run without tests/limit.sh in front of it."
    echo "        A hang there stops the suite instead of failing it, which is"
    echo "        the one outcome no recorded .out can express."
    echo "$unguarded" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every run of the tool goes through tests/limit.sh"

TMP="${TMPDIR:-/tmp}/mx-hygiene.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# Where it stands. Two lines right, one wrong, and the wrong one is not an error.
HALF='swap: 2 1
again: 4 3
bump: 105 0'

# Before `{~t}`. The form's temporary was a name the caller also had.
CAPTURED='swap: 2 2
again: 4 4
bump: 105 0'

# What closing the other half would look like: `bump` would leave the caller's
# local at 100 and add its 5 to the file-scope `total` it meant.
WHOLE='swap: 2 1
again: 4 3
bump: 100 5'

if ! sh tests/limit.sh "$LIMIT" "$MX" "$SRC" > "$TMP/body.c" 2> "$TMP/err"; then
    echo "FAILED  hygiene.sh: $SRC did not expand"
    cat "$TMP/err"
    exit 1
fi

# The one line the example does not emit, because `#include` is not a rule.
{ echo '#include <stdio.h>'; cat "$TMP/body.c"; } > "$TMP/prog.c"

if ! "$CC" -o "$TMP/prog" "$TMP/prog.c" 2> "$TMP/cc.err"; then
    echo "FAILED  hygiene.sh: the expansion does not compile"
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/prog")

# Two applications of one rule must not share a temporary. The numbers above
# would catch that anyway; this catches it by name, and says why.
if ! grep -q 't__1' "$TMP/body.c" || ! grep -q 't__2' "$TMP/body.c"; then
    echo "FAILED  hygiene.sh: the two swaps did not get two names."
    echo "        {~t} is one name per expansion, not one per rule."
    grep -n 'int t' "$TMP/body.c"
    exit 1
fi

case "$got" in
"$HALF")
    echo "ok      hygiene.sh: {~t} holds; reaching out still does not"
    echo "            swap: 2 1      the template's own name, twice over"
    echo "            bump: 105 0    would be  bump: 100 5"
    exit 0 ;;
"$CAPTURED")
    echo "FAILED  hygiene.sh: the capture is back."
    echo "        {~t} has stopped giving each expansion its own name."
    exit 1 ;;
"$WHOLE")
    echo "FAILED  hygiene.sh: reaching out works too."
    echo "        That is the good news and this file is now wrong. Rewrite it,"
    echo "        and the hygiene section of docs/notation.md, in the commit"
    echo "        that did it -- and say there what a template learned to see."
    exit 1 ;;
*)
    echo "FAILED  hygiene.sh: none of the three recorded outcomes."
    echo "        wanted: $(echo "$HALF" | tr '\n' '/')"
    echo "        got:    $(echo "$got" | tr '\n' '/')"
    echo "        Work out which one it is heading for before touching the"
    echo "        numbers above."
    exit 1 ;;
esac
