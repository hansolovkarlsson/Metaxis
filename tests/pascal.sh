#!/bin/sh
# pascal.sh -- stage 1, checked by a compiler rather than by diff.
#
# Every other example is pinned to a recorded `.out`, which catches a change and
# nothing else: an expansion can be wrong in any way that still looks plausible
# and the diff will pass it. This one takes the Pascal in examples/, expands it,
# **compiles the C that comes out and runs it**, and checks a number. That is the
# whole reason the stage-1 target is C -- see docs/ROADMAP.md.
#
# It runs the pair, because they fail in different places and each failure says
# something:
#
#   code.pt    must compile and print what the Pascal computes. If the
#              arithmetic, the precedence, `mod`, the loop or the branches were
#              translated wrongly, the number is wrong and nothing else has to
#              notice. It is also the only place `terminated(h)` is checked
#              against a compiler: the Pascal has one branch that is a
#              `begin … end` and one that is not, so a rule that punctuated
#              either of them wrongly would not build.
#
#   pascal.pt  must NOT compile, and the reason must be the string literal.
#              **That is recorded on purpose**, the way tests/hygiene.sh records
#              `bump: 105 0`: it is the one thing a string template cannot do,
#              it is documented in the file's own closing note, and a test that
#              merely skipped it would let it be quietly fixed or quietly
#              forgotten. When somebody translates the literal, this half fails
#              and has to be edited in the same commit.

PT="${1:-./bin/pt}"
CC="${CC:-cc}"

TMP="${TMPDIR:-/tmp}/pt-pascal.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# 3+6+12+15+18 = 54 for the multiples of 3 that are not 9, minus 1 for each of
# the other fifteen, so 39 after the loop. 39 is not over 100, so the first
# line; it is over 30, so the branch that adds one and prints 40. Then
# Show(Double(40)) is 80 through a function that returns, and Pair(40, 2) is 42
# through a procedure with two parameters. Worked out from the Pascal.
WANT="it's middling
40
80
42"

# ---------------------------------------------------------------- code.pt
if ! "$PT" examples/code.pt > "$TMP/code.c" 2> "$TMP/err"; then
    echo "FAILED  pascal.sh: examples/code.pt did not expand"
    cat "$TMP/err"
    exit 1
fi

if ! "$CC" -o "$TMP/code" "$TMP/code.c" 2> "$TMP/cc.err"; then
    echo "FAILED  pascal.sh: the C from examples/code.pt does not compile"
    echo "        Stage 1 is that this compiles. Nothing else in the suite"
    echo "        would have told you."
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/code")
if [ "$got" != "$WANT" ]; then
    echo "FAILED  pascal.sh: the program compiled and computed the wrong thing."
    echo "        wanted: $(echo "$WANT" | tr '\n' '/')"
    echo "        got:    $(echo "$got"  | tr '\n' '/')"
    echo "        The Pascal is right; something translated it wrongly."
    exit 1
fi

# ---------------------------------------------------------------- pascal.pt
if ! "$PT" examples/pascal.pt > "$TMP/pascal.c" 2> "$TMP/err"; then
    echo "FAILED  pascal.sh: examples/pascal.pt did not expand"
    cat "$TMP/err"
    exit 1
fi

if "$CC" -o "$TMP/pascal" "$TMP/pascal.c" > /dev/null 2>&1; then
    echo "FAILED  pascal.sh: the C from examples/pascal.pt compiles now."
    echo "        That is the good news and this half is now wrong. A string"
    echo "        template had no way to translate 'it''s' into \"it's\", which"
    echo "        is what that file's closing note says and what code.pt exists"
    echo "        to contrast with. Rewrite both, and this test, in the commit"
    echo "        that did it."
    exit 1
fi

if ! grep -q "it''s" "$TMP/pascal.c"; then
    echo "FAILED  pascal.sh: examples/pascal.pt no longer emits the Pascal"
    echo "        literal, so whatever stopped it compiling is something else."
    echo "        Find out what before touching this file."
    exit 1
fi

echo "ok      pascal.sh: the C compiles, runs, and computes 80 and 42"
echo "            pascal.pt still cannot spell C's quotes -- as recorded"
exit 0
