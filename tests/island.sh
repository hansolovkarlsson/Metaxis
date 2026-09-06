#!/bin/sh
# island.sh -- stage 5: the tool rewrites its own front end, and the result runs.
#
# lib/island.mx is a text-mode rewrite of `fprintf(stderr, "mx: %s\n", X)`
# into `complain(X)`, with the definition inserted. This points it at the real
# metaxis/cmd/mx.c -- concatenated at test time, so it can never drift from
# the file in the tree -- compiles what comes out against the tool's own
# objects in build/, and runs the binary that results on an example and on an
# error. A rewrite that broke the C would not compile; one that changed its
# meaning would expand examples/first.mx differently or print a different
# message. Nothing else in the suite runs the tool on a file that was not
# written for it.
#
# Every count below is a claim about metaxis/cmd/mx.c as it is today, and a
# change to that file may rightly change them. When it does, the numbers here
# change in the same commit -- that is a test noticing the tree, not a defect.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"

TMP="${TMPDIR:-/tmp}/mx-island.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# @use resolves against the directory of the file that used it, and this file
# lives in $TMP, so the path has to be absolute.
{ echo "@use \"$PWD/lib/island.mx\""; echo "@end"; cat metaxis/cmd/mx.c; } > "$TMP/in.mx"

if ! sh tests/limit.sh "$LIMIT" "$MX" "$TMP/in.mx" > "$TMP/mx.c" 2> "$TMP/err"; then
    echo "FAILED  island.sh: lib/island.mx did not expand over metaxis/cmd/mx.c"
    cat "$TMP/err"
    exit 1
fi

# What the rewrite did, counted. Six calls become complain(), the definition
# appears once (it is the only remaining fprintf of that shape), and the one
# call with a different format string is left alone.
calls=$(grep -c 'complain(err)' "$TMP/mx.c")
defs=$(grep -c 'static void complain(const char \*e)' "$TMP/mx.c")
left=$(grep -c 'fprintf(stderr, "mx: %s\\n", err)' "$TMP/mx.c")
other=$(grep -c 'fprintf(stderr, "mx: cannot write %s\\n", outpath)' "$TMP/mx.c")
if [ "$calls" != 6 ] || [ "$defs" != 1 ] || [ "$left" != 0 ] || [ "$other" != 1 ]; then
    echo "FAILED  island.sh: the rewrite did not do what it does to today's mx.c"
    echo "        complain(err) calls: $calls (want 6)   definition: $defs (want 1)"
    echo "        old calls left: $left (want 0)   the other fprintf: $other (want 1)"
    echo "        If metaxis/cmd/mx.c changed, update these; if it did not,"
    echo "        text mode changed and this is the first thing that noticed."
    exit 1
fi

# The rewritten front end, linked against the tree's own objects.
objs=$(ls build/*.o | grep -v '/mx\.o$')
if ! "$CC" -std=c11 -Imetaxis/include -D_POSIX_C_SOURCE=200809L -o "$TMP/mx2" "$TMP/mx.c" $objs 2> "$TMP/cc.err"; then
    echo "FAILED  island.sh: the rewritten mx.c does not compile"
    cat "$TMP/cc.err"
    exit 1
fi

# It has to be the same tool. examples/first.mx through the rewritten binary
# must match the recorded output, and an error must come out through complain().
if ! sh tests/limit.sh "$LIMIT" "$TMP/mx2" examples/first.mx > "$TMP/first.out" 2> "$TMP/err"; then
    echo "FAILED  island.sh: the rewritten tool cannot expand examples/first.mx"
    cat "$TMP/err"
    exit 1
fi
if ! diff -u examples/first.out "$TMP/first.out" > "$TMP/diff"; then
    echo "FAILED  island.sh: the rewritten tool expands examples/first.mx differently"
    cat "$TMP/diff"
    exit 1
fi
msg=$(sh tests/limit.sh "$LIMIT" "$TMP/mx2" examples/no-such-file.mx 2>&1)
if [ "$msg" != "mx: cannot open examples/no-such-file.mx" ]; then
    echo "FAILED  island.sh: the rewritten tool's error message changed"
    echo "        got: $msg"
    exit 1
fi

echo "ok      island.sh: mx.c rewritten by its own tool compiles, runs, and says the same"
echo "            6 calls became complain(), 1 left alone, definition inserted"
exit 0
