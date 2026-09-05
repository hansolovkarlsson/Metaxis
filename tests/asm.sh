#!/bin/sh
# asm.sh -- stage 2, checked by an assembler and a CPU.
#
# tests/pascal.sh compiles the C that examples/code.pt emits. This does the same
# one level down: it assembles what examples/asm.pt emits, links it against a
# four-line runtime, runs it, and checks the numbers. If a rule emitted its
# operands in the wrong order, popped the wrong register, or reused a label, the
# program would still assemble and would print something else.
#
# **The arch gate is deliberate and is not a skip of the interesting part.** The
# example's recorded .out is diffed by `make check` on every machine, so a change
# to what it emits is caught anywhere. Only the half that needs a CPU of the
# right kind is gated, and it says so rather than passing silently.

PT="${1:-./bin/pt}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"

if [ "$(uname -m)" != "arm64" ]; then
    echo "ok      asm.sh: skipped, examples/asm.pt emits arm64 and this is $(uname -m)"
    echo "            the recorded output is still checked by make check"
    exit 0
fi

TMP="${TMPDIR:-/tmp}/pt-asm.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# 2 + 3*4 with the multiply first; 100-7-3 grouped left; then the two arms of a
# conditional, one each way. Worked out from the C, not from the assembly.
WANT="14
90
10
20"

if ! sh tests/limit.sh "$LIMIT" "$PT" examples/asm.pt > "$TMP/prog.s" 2> "$TMP/err"; then
    echo "FAILED  asm.sh: examples/asm.pt did not expand"
    cat "$TMP/err"
    exit 1
fi

# The runtime the example does not emit, because `#include` is not a rule and
# printf's variadic ABI is not something a stack machine should be writing.
cat > "$TMP/rt.c" <<'C'
#include <stdio.h>
void putn(int n) { printf("%d\n", n); }
C

if ! "$CC" -o "$TMP/prog" "$TMP/prog.s" "$TMP/rt.c" 2> "$TMP/cc.err"; then
    echo "FAILED  asm.sh: the assembly does not assemble"
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/prog")
if [ "$got" != "$WANT" ]; then
    echo "FAILED  asm.sh: it assembled and computed the wrong thing."
    echo "        wanted: $(echo "$WANT" | tr '\n' '/')"
    echo "        got:    $(echo "$got"  | tr '\n' '/')"
    echo "        Operand order, a popped register, or a reused label."
    exit 1
fi

# A label used at the branch and at the place it jumps to must be one name. This
# is the defect that writing the example found, and the numbers above would not
# always catch it -- a duplicated label is an assembler error, but a *shared* one
# across two conditionals is a wrong jump that can still print the right thing.
if [ "$(grep -c 'Lelse__' "$TMP/prog.s")" -ne 4 ]; then
    echo "FAILED  asm.sh: two conditionals should give four Lelse__ lines,"
    echo "        two branches and two labels, with a different name each."
    grep -n 'L.*__' "$TMP/prog.s"
    exit 1
fi

echo "ok      asm.sh: it assembles, runs, and prints 14 90 10 20"
exit 0
