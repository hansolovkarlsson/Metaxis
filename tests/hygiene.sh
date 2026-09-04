#!/bin/sh
# hygiene.sh -- the defect, run rather than argued.
#
# A template that is a string cannot ask for a name nobody else has, and cannot
# say *the outer one*. docs/notation.md said so under "Not done" and admitted
# the gap had been reasoned about and not tested. This tests it: it expands
# examples/hygiene.pt, compiles the C that comes out, runs it, and pins the two
# wrong numbers.
#
# **This test passing means the defect is still here.** It is written this way
# on purpose. A test that merely failed would be turned off; one that pins the
# wrong answer has to be edited by whoever fixes it, in the same commit, which
# is the only way the record and the code stay in step.
#
# The numbers are Proto's. `Proto/examples/forms.pro` demonstrates the same two
# failures against the same expansion, and its comments record #105 and #0 for
# the second one -- the same pair this prints.

PT="${1:-./bin/pt}"
CC="${CC:-cc}"
SRC="${SRC:-examples/hygiene.pt}"

TMP="${TMPDIR:-/tmp}/pt-hygiene.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# What it does. Both lines are wrong, and neither is an error.
CAPTURED='swap: 2 2
bump: 105 0'

# What it would do if a template could name its own temporary and reach past a
# caller's shadow. `swap` would leave 2 and 1; `bump` would leave the caller's
# local at 100 and add its 5 to the file-scope `total`.
HYGIENIC='swap: 2 1
bump: 100 5'

if ! "$PT" "$SRC" > "$TMP/body.c" 2> "$TMP/err"; then
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

if [ "$got" = "$CAPTURED" ]; then
    echo "ok      hygiene.sh: the capture is still here"
    echo "            swap: 2 2      hygiene would give  swap: 2 1"
    echo "            bump: 105 0    hygiene would give  bump: 100 5"
    exit 0
fi

if [ "$got" = "$HYGIENIC" ]; then
    echo "FAILED  hygiene.sh: hygiene appears to work."
    echo "        That is the good news and this file is now wrong. Rewrite it,"
    echo "        and the 'Not done' entry in docs/notation.md, in the commit"
    echo "        that did it."
    exit 1
fi

echo "FAILED  hygiene.sh: neither the capture nor hygiene."
echo "        wanted: $(echo "$CAPTURED" | tr '\n' '/')"
echo "        got:    $(echo "$got" | tr '\n' '/')"
echo "        The expansion changed. Work out which of the two it is heading"
echo "        for before touching the numbers above."
exit 1
