#!/bin/sh
# scale.sh -- one input large enough for a quadratic to show.
#
# Every other check here is small on purpose. `make check` runs in three
# seconds, thirteen examples of a few dozen lines each, and **a suite like that
# cannot see an O(n squared) by construction.** It did not, for two days: on
# 2026-09-05 expansion of a 4985-line program took 67 seconds while the whole
# suite stayed green in three. See docs/POSTMORTEM.md 18.
#
# So this generates a program too big to hide in, and the check is that it
# finishes. That is deliberately not a stopwatch: a wall-clock threshold on a
# shared runner is a flaky test, and what this is guarding against is not a
# machine being 20% slower, it is a cost that squares. The old lexer took ~39
# seconds on the 4000 statements below and would be killed by the limit; the
# fixed one takes about a fifth of a second. Thirty seconds is not a budget, it
# is the gap between those two answers.
#
# The times either side of it are printed rather than asserted, because a
# number that is reported gets read and a number that is asserted gets tuned.

MX="${1:-./bin/mx}"
LIMIT="${LIMIT:-10}"
BIG="${BIG:-4000}"
SMALL=$((BIG / 4))

TMP="${TMPDIR:-/tmp}/mx-scale.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# The grammar is examples/pascal.mx's, so this measures a dialect the tree
# actually declares rather than one invented to be fast.
sed -n '1,/^@end$/p' examples/pascal.mx > "$TMP/head.mx"

gen() {
    n=$1
    { cat "$TMP/head.mx"
      printf '\nprogram Scale;\n\nvar\n  a, b, c: integer;\n\nbegin\n'
      awk -v n="$n" 'BEGIN { for (i = 1; i <= n; i++)
                                 printf "  a := (b + %d) * (c - %d) mod 7;\n", i, i }'
      printf 'end.\n'; }
}

# `date +%s` is whole seconds and this finishes in a fifth of one, so the
# reported numbers would all be 0. python3 is used where it is there and the
# report is dropped where it is not -- the check itself is `did it finish`, and
# nothing here depends on being able to time anything.
now_ms() {
    if [ -n "$HAVE_PY" ]; then python3 -c 'import time; print(int(time.time()*1000))'
    else echo ""; fi
}
command -v python3 >/dev/null 2>&1 && HAVE_PY=1 || HAVE_PY=

run() {
    n=$1; out=$2
    gen "$n" > "$TMP/in.mx"
    start=$(now_ms)
    sh tests/limit.sh "$LIMIT" "$MX" "$TMP/in.mx" > "$out" 2> "$TMP/err"
    rc=$?
    stop=$(now_ms)
    ms=""
    [ -n "$HAVE_PY" ] && ms=$((stop - start))
    if [ $rc -eq 124 ]; then
        echo "FAILED  scale.sh: $n statements did not finish in ${LIMIT}s."
        echo "        This is the shape a quadratic makes: the examples stay"
        echo "        green and one large input stops finishing. Profile it --"
        echo "        docs/POSTMORTEM.md 18 is the last time, and reading the"
        echo "        code produced two plausible causes that were both wrong."
        return 1
    fi
    if [ $rc -ne 0 ]; then
        echo "FAILED  scale.sh: $n statements did not expand"
        cat "$TMP/err"
        return 1
    fi
    return 0
}

run "$SMALL" "$TMP/small.out" || exit 1
small=$ms
run "$BIG" "$TMP/big.out" || exit 1
big=$ms

# Cheap and real: pascal.mx ends every statement with ";\n", so one Pascal
# statement is one line of C. The two programs share a preamble character for
# character, so the *difference* in output lines must be the difference in
# statements exactly -- which needs no assumption about how many lines a
# `program` and a `var` section turn into. A lexer that got fast by dropping
# tokens would pass a stopwatch and fails this.
lines=$(wc -l < "$TMP/big.out" | tr -d ' ')
few=$(wc -l < "$TMP/small.out" | tr -d ' ')
got=$((lines - few))
want=$((BIG - SMALL))
if [ "$got" -ne "$want" ]; then
    echo "FAILED  scale.sh: $SMALL statements gave $few lines and $BIG gave $lines,"
    echo "        a difference of $got where $want statements were added."
    echo "        Finishing quickly is not the same as finishing correctly."
    exit 1
fi

echo "ok      scale.sh: $BIG statements expand inside ${LIMIT}s, $lines lines out"
if [ -n "$HAVE_PY" ]; then
    echo "            ${SMALL} in ${small}ms, ${BIG} in ${big}ms -- reported, not asserted"
    echo "            (four times the statements. When this was quadratic, the"
    echo "             second number was 38767ms -- see docs/POSTMORTEM.md 18)"
fi
exit 0
