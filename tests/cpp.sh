#!/bin/sh
# cpp.sh -- stage 6: C in, C out, and the oracle is the C compiler's own
# preprocessor.
#
# examples/cpp.mx is a preprocessor written as three text-mode rules over the
# store (REFERENCE §8.5). This runs its body through it and compiles what comes
# out, then hands the same body to the C compiler directly, whose preprocessor
# reads the same directives, and the two programs must print the same lines.
# That comparison is the point: a wrong expansion that still compiles is caught
# by the compiler disagreeing, the way tests/python.sh catches a translation
# that is wrong the same way on both sides of an operator by running the
# Python too.
#
# It is not enough on its own, because the compiler behind us has a
# preprocessor: a `#define` the rules left in place would be expanded by cc and
# the two programs would agree. So the output is also read: no directive may
# survive but the include, and no macro name may stand as an identifier except
# the one inside a string, which the string class keeps from the rules.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"
SRC="${SRC:-examples/cpp.mx}"

TMP="${TMPDIR:-/tmp}/mx-cpp.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

if ! sh tests/limit.sh "$LIMIT" "$MX" "$SRC" > "$TMP/out.c" 2> "$TMP/err"; then
    echo "FAILED  cpp.sh: $SRC did not expand"
    cat "$TMP/err"
    exit 1
fi

# What the rules did, read off the output. Every `#define` and `#undef` is
# gone and the `#include` is the one directive left; LIMIT stands once, in the
# string that says it is not a macro there; the other three names are gone.
# Identifiers are counted by splitting the file into them first, for the
# reason tests/island.sh gives: BSD grep -ow misses the second match on a line.
idents() { tr -c 'A-Za-z0-9_' '\n' < "$TMP/out.c" | grep -cx "$1"; }
dirs=$(grep -c '^#' "$TMP/out.c")
inc=$(grep -c '^#include <stdio.h>$' "$TMP/out.c")
lim=$(idents LIMIT)
gone=$(( $(idents TOTAL) + $(idents GREETING) + $(idents A) + $(idents B) \
       + $(idents DOUBLE) + $(idents ADD) + $(idents SEVEN) + $(idents TWICE) \
       + $(idents DEBUG) + $(idents LEVEL) + $(idents MODE) + $(idents NOTDEFINED) ))
step=$(idents STEP)
self=$(idents SELF)
if [ "$dirs" != 1 ] || [ "$inc" != 1 ] || [ "$lim" != 1 ] || [ "$gone" != 0 ] || [ "$step" != 4 ] || [ "$self" != 2 ]; then
    echo "FAILED  cpp.sh: the rules did not do what they do to examples/cpp.mx's body"
    echo "        directives left: $dirs (want 1, the include: $inc)"
    echo "        LIMIT as an identifier: $lim (want 1, inside the string)"
    echo "        TOTAL, GREETING, A, B, the four function-like names and the four conditional ones left: $gone (want 0)"
    echo "        STEP left: $step (want 4: the undefined variable, its use, the comment, and TOTAL's body read after the undef)"
    echo "        SELF left: $self (want 2: the variable and its use, the macro having stopped at itself)"
    exit 1
fi

# The preprocessed C, compiled and run.
if ! "$CC" -std=c11 -o "$TMP/a" "$TMP/out.c" 2> "$TMP/cc.err"; then
    echo "FAILED  cpp.sh: the preprocessed C does not compile"
    cat "$TMP/cc.err"
    exit 1
fi
got=$("$TMP/a")

# The same body, preprocessed by the compiler itself.
awk 'f { print } /^@end/ { f = 1 }' "$SRC" > "$TMP/prog.c"
if ! "$CC" -std=c11 -o "$TMP/b" "$TMP/prog.c" 2> "$TMP/cc.err"; then
    echo "FAILED  cpp.sh: the body of $SRC does not compile on its own"
    cat "$TMP/cc.err"
    exit 1
fi
want=$("$TMP/b")

# And what both must say, so that the two agreeing on a wrong answer is caught
# too.
expected='limit=10 step=3 total=30
LIMIT is not a macro inside a string
17
70
5 4
20 5 7 9
2 nested'

if [ "$got" != "$want" ]; then
    echo "FAILED  cpp.sh: the preprocessed program and the compiler's own disagree"
    echo "        ours:  $(echo "$got"  | tr '\n' '/')"
    echo "        cc's:  $(echo "$want" | tr '\n' '/')"
    exit 1
fi
if [ "$got" != "$expected" ]; then
    echo "FAILED  cpp.sh: both programs print something other than what is pinned here"
    echo "        got:      $(echo "$got"      | tr '\n' '/')"
    echo "        expected: $(echo "$expected" | tr '\n' '/')"
    exit 1
fi

echo "ok      cpp.sh: object-like and function-like macros and nested conditionals, and cc's own preprocessor agrees"
echo "            no directive left but the include, LIMIT kept inside its string,"
echo "            and both programs print the same seven lines"
exit 0
