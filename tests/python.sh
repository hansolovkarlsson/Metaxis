#!/bin/sh
# python.sh -- stage 3, checked by two compilers rather than by diff.
#
# tests/pascal.sh compiles the C and runs it, which catches a wrong answer
# instead of a changed one. This one does that and then does something the
# suite has not done before: **it runs the source language too.**
#
# The body of examples/python.mx is Python. Not Python-shaped -- Python, which
# python3 will run. So the same text can be executed twice, once by the
# interpreter it was written for and once as the C this tree translated it
# into, and the two answers compared. A translation that is *self-consistently*
# wrong -- a precedence table that is wrong the same way on both sides of an
# operator, a loop that is off by one in the C and recorded as such -- passes a
# diff and passes pascal.sh, and fails here, because the Python is not this
# tree's to be wrong about.
#
# That is the whole argument for the stage-3 example's body being runnable
# Python rather than an illustration of one.
#
# The Python half is skipped, loudly, where python3 is not installed. The C
# half is not skippable: it is the test.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"

TMP="${TMPDIR:-/tmp}/mx-python.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# i runs 1..19. `i % 3 == 0 and i != 9` is true for 3, 6, 12, 15 and 18, which
# add to 54, and false for the other fourteen, each of which takes one away.
# So total is 40, twice(40) is 80, and clamp(80, 50) is 50. Worked out from the
# Python, and then confirmed by running it -- which is the point of this file.
WANT="40
80
50"

if ! sh tests/limit.sh "$LIMIT" "$MX" examples/python.mx > "$TMP/py.c" 2> "$TMP/err"; then
    echo "FAILED  python.sh: examples/python.mx did not expand"
    cat "$TMP/err"
    exit 1
fi

if ! "$CC" -o "$TMP/py" "$TMP/py.c" 2> "$TMP/cc.err"; then
    echo "FAILED  python.sh: the C from examples/python.mx does not compile"
    echo "        Stage 3 is that this compiles. A block that closed in the"
    echo "        wrong place is a brace in the wrong place, and nothing but a"
    echo "        compiler would say so."
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/py")
if [ "$got" != "$WANT" ]; then
    echo "FAILED  python.sh: the C compiled and computed the wrong thing."
    echo "        wanted: $(echo "$WANT" | tr '\n' '/')"
    echo "        got:    $(echo "$got"  | tr '\n' '/')"
    exit 1
fi

# ------------------------------------------------------- and the same text,
# ------------------------------------------------------- run as what it is.
sed -n '/^@end$/,$p' examples/python.mx | tail -n +2 > "$TMP/body.py"

if ! command -v python3 > /dev/null 2>&1; then
    echo "ok      python.sh: the C compiles, runs, and computes 40 80 50"
    echo "            python3 is not installed, so the source half was skipped"
    echo "            -- that half is the one that checks the C against the"
    echo "            language it came from, and it did not run."
    exit 0
fi

pygot=$(sh tests/limit.sh "$LIMIT" python3 "$TMP/body.py" 2> "$TMP/py.err")
pyrc=$?
if [ "$pyrc" -ne 0 ]; then
    echo "FAILED  python.sh: the body of examples/python.mx is not valid Python."
    echo "        It is supposed to be a program, not an illustration of one."
    echo "        If the grammar now reads something Python does not, that is a"
    echo "        dialect and this file should stop claiming otherwise."
    cat "$TMP/py.err"
    exit 1
fi

if [ "$pygot" != "$got" ]; then
    echo "FAILED  python.sh: the Python and its translation disagree."
    echo "        python3 says: $(echo "$pygot" | tr '\n' '/')"
    echo "        the C says:   $(echo "$got"   | tr '\n' '/')"
    echo "        This is the failure a recorded .out cannot have: both halves"
    echo "        ran, and the translation is wrong."
    exit 1
fi

echo "ok      python.sh: the C compiles, runs, and computes 40 80 50"
echo "            and python3 runs the same text to the same three numbers"
exit 0
