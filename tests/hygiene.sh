#!/bin/sh
# hygiene.sh -- one half fixed, one half charged, run rather than argued.
#
# It expands examples/hygiene.pt, compiles the C that comes out and runs it.
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

PT="${1:-./bin/pt}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"
SRC="${SRC:-examples/hygiene.pt}"

TMP="${TMPDIR:-/tmp}/pt-hygiene.$$"
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

if ! sh tests/limit.sh "$LIMIT" "$PT" "$SRC" > "$TMP/body.c" 2> "$TMP/err"; then
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
