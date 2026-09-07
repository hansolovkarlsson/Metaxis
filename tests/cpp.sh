#!/bin/sh
# cpp.sh -- stage 6: C in, C out, and the oracle is the C compiler's own
# preprocessor.
#
# lib/cpp.mx is a preprocessor written as text-mode rules over the store
# (REFERENCE §8.5), and examples/cpp.mx is a C program that @uses them. This
# runs that body through it and compiles what comes out, then hands the same
# body to the C compiler directly, whose preprocessor reads the same
# directives, and the two programs must print the same lines. That comparison
# is the point: a wrong expansion that still compiles is caught by the compiler
# disagreeing, the way tests/python.sh catches a translation that is wrong the
# same way on both sides of an operator by running the Python too.
#
# Since 2026-09-07 it also runs the body the *second* way REFERENCE §9 allows,
# `mx -u lib/cpp.mx -i prog.c`, where the rules and the file they are pointed
# at are two files and the input carries no header at all. The two runs must
# give the same bytes. That is the whole claim of the split form: the grammar
# a file is read with may come from outside it without changing what it reads
# to, so a `.mx` file can be a stage in a toolchain over sources that say
# nothing about any grammar.
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
       + $(idents DEBUG) + $(idents LEVEL) + $(idents MODE) + $(idents NOTDEFINED) \
       + $(idents FROM_HEADER) + $(idents STR) + $(idents GLUE) ))
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

# The same body, preprocessed by the compiler itself. The body includes
# "cpp.h", which stands beside examples/cpp.mx, so the compiler is told where
# to look; the preprocessed C above has already read it in and needs nothing.
awk 'f { print } /^@end/ { f = 1 }' "$SRC" > "$TMP/prog.c"
if ! "$CC" -std=c11 -I"$(dirname "$SRC")" -o "$TMP/b" "$TMP/prog.c" 2> "$TMP/cc.err"; then
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
2 nested
103
5 hello world 10'

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

# The second form: the same rules over the same body, given as two files.
# $TMP/prog.c above is exactly the body of $SRC with the header gone, so this
# is the identical text read with the identical rules, and it must expand to
# the identical bytes. cpp.h is copied next to it because `read(path)` resolves
# beside the file being expanded (REFERENCE §8.3), which under -i is the input
# and not the rules. That is the one thing the two forms do differently, and
# the reason the copy is here rather than a search path of some kind.
cp "$(dirname "$SRC")/cpp.h" "$TMP/cpp.h"
if ! sh tests/limit.sh "$LIMIT" "$MX" -u lib/cpp.mx -i "$TMP/prog.c" > "$TMP/split.c" 2> "$TMP/err"; then
    echo "FAILED  cpp.sh: mx -u lib/cpp.mx -i prog.c did not expand"
    cat "$TMP/err"
    exit 1
fi
if ! diff -u "$TMP/out.c" "$TMP/split.c" > "$TMP/diff"; then
    echo "FAILED  cpp.sh: the two forms of the same run disagree"
    echo "        left: mx $SRC     right: mx -u lib/cpp.mx -i prog.c"
    cat "$TMP/diff"
    exit 1
fi

# And it is C, not merely the same text: compiled and run, it prints the same.
if ! "$CC" -std=c11 -o "$TMP/c" "$TMP/split.c" 2> "$TMP/cc.err"; then
    echo "FAILED  cpp.sh: the C from the split form does not compile"
    cat "$TMP/cc.err"
    exit 1
fi
split=$("$TMP/c")
if [ "$split" != "$expected" ]; then
    echo "FAILED  cpp.sh: the program from the split form prints something else"
    echo "        got:      $(echo "$split"   | tr '\n' '/')"
    echo "        expected: $(echo "$expected" | tr '\n' '/')"
    exit 1
fi

# And the path base, stated as a failure so it names what it tried. `read` in
# the `#include` rule looks beside the file being expanded, which under -i is
# the input; a run whose rules live in lib/ and whose input lives in $TMP must
# look for the header in $TMP. That is the one thing the two forms do
# differently and it is the thing a toolchain would be bitten by, so it is
# pinned rather than described.
printf 'int a = 1;\n#include "no-such-header.h"\nint b = 2;\n' > "$TMP/bad.c"
msg=$(sh tests/limit.sh "$LIMIT" "$MX" -u lib/cpp.mx -i "$TMP/bad.c" 2>&1)
case "$msg" in
    *"$TMP/no-such-header.h"*) ;;
    *)  echo "FAILED  cpp.sh: 'read' under -i did not look beside the input file"
        echo "        got: $msg"
        exit 1 ;;
esac

# `#elif` is refused, and this is the one thing these rules refuse rather than
# pass through. It stands inside a conditional they do read, so the arm holding
# it is taken or dropped whole: before 2026-09-07 the file below expanded to
# `int before;` and `int after;` with `int a;` gone, no message and status 0,
# where cc keeps `int a;`. That is a wrong answer and not an unfinished one,
# which is why it became a refusal. Both arms are checked, and so is a `#elif`
# standing outside any conditional.
printf '#define A 1\nint before;\n#ifdef B\nint b;\n#elif defined(A)\nint a;\n#endif\nint after;\n' > "$TMP/elif.c"
printf 'int a;\n#elif whatever\n' > "$TMP/stray.c"
for f in elif stray; do
    msg=$(sh tests/limit.sh "$LIMIT" "$MX" -u lib/cpp.mx -i "$TMP/$f.c" 2>&1)
    rc=$?
    case "$rc:$msg" in
        1:*"'#elif' is not a directive these rules read"*) ;;
        *)  echo "FAILED  cpp.sh: $f.c was not refused"
            echo "        status: $rc"
            echo "        got:    $msg"
            exit 1 ;;
    esac
done

echo "ok      cpp.sh: macros, conditionals, an include, # and ##, and cc's own preprocessor agrees"
echo "            no directive left but the system include, LIMIT kept inside its string,"
echo "            and both programs print the same nine lines"
echo "            and mx -u lib/cpp.mx -i prog.c gives the same bytes, compiles and prints them too,"
echo "            with #elif refused in an arm and outside one rather than dropped in silence"
exit 0
