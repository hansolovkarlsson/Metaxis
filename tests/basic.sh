#!/bin/sh
# basic.sh -- stage 4, checked by a compiler, with the wall written in by hand.
#
# examples/basic.mx turns BASIC into C, and the C it emits is every statement
# of the program and nothing else: no `main`, no `#include`, and **no
# declarations**. BASIC declares nothing; a variable exists because some line
# mentions it. C wants each one declared before the first statement, and
# which ones is the aggregate of every LET, FOR and PRINT in the program --
# which no rule in that file can see, because a rule sees its own holes and
# nothing else. That is the customer stage 4 was picked to be. See
# docs/ROADMAP.md.
#
# So this script writes the prologue. The four names below are worked out
# from the BASIC by a person, and they are the one thing the translator does
# not do. **They are pinned in both directions**: the program must not run
# without them, and the translator must not have started emitting them. When
# it does -- when a rule can contribute a declaration and the head of the
# output can splice the aggregate -- the grep below fails and this file, and
# the closing note of examples/basic.mx, get rewritten in the commit that did
# it. That is the same pin tests/pascal.sh keeps on the literal, and for the
# same reason: a wrong thing that is written down cannot be quietly fixed and
# cannot be quietly forgotten.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"

TMP="${TMPDIR:-/tmp}/mx-basic.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# The loop is examples/pascal.mx's, spelled with GOTO: 3+6+12+15+18 = 54 for
# the multiples of 3 that are not 9, minus 1 for each of the other fifteen,
# so 39. The jump-loop counts to 4. 39 is not over 100, so A$ keeps its first
# value. NOT (39 > 30) is false, so the jump is not taken, and 40 is printed.
# Worked out from the BASIC, not from the C.
WANT="39
4
it's middling
40"

if ! sh tests/limit.sh "$LIMIT" "$MX" examples/basic.mx > "$TMP/body.c" 2> "$TMP/err"; then
    echo "FAILED  basic.sh: examples/basic.mx did not expand"
    cat "$TMP/err"
    exit 1
fi

# The four declarations, each once, before the first statement. T, I and N
# are the numbers the program mentions and A$ its one string; the translator
# read the BASIC to know that, which is the whole point.
for d in 'int T;' 'int I;' 'int N;' 'const char \*A_s;'; do
    if [ "$(grep -c "^$d\$" "$TMP/body.c")" != 1 ]; then
        echo "FAILED  basic.sh: '$d' is not declared exactly once"
        echo "        A collection keeps one copy of each contribution and goes"
        echo "        first when nothing splices it. One of those has changed."
        grep -n 'int \|const char' "$TMP/body.c"
        exit 1
    fi
done
if [ "$(head -1 "$TMP/body.c")" != 'int T;' ]; then
    echo "FAILED  basic.sh: the declarations are not at the start of the output"
    head -3 "$TMP/body.c"
    exit 1
fi

# Nothing supplied but what is not a rule.
{
    echo '#include <stdio.h>'
    echo 'int main(void) {'
    cat "$TMP/body.c"
    echo '}'
} > "$TMP/prog.c"

if ! "$CC" -o "$TMP/prog" "$TMP/prog.c" 2> "$TMP/cc.err"; then
    echo "FAILED  basic.sh: the C from examples/basic.mx does not compile"
    echo "        Stage 4 is that this compiles with nothing supplied but"
    echo "        main. Nothing else in the suite would have told you."
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/prog")
if [ "$got" != "$WANT" ]; then
    echo "FAILED  basic.sh: the program compiled and computed the wrong thing."
    echo "        wanted: $(echo "$WANT" | tr '\n' '/')"
    echo "        got:    $(echo "$got"  | tr '\n' '/')"
    echo "        The BASIC is right; something translated it wrongly."
    exit 1
fi

echo "ok      basic.sh: the C compiles, runs, and computes 39 4 40"
echo "            T, I, N and A\$ declared by the LET and FOR that met them"
exit 0
