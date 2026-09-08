#!/bin/sh
# mini.sh -- one grammar, two languages out, and neither named in the grammar.
#
# lib/mini.mx is the grammar of a small imperative language and names no
# target: every rule emits by calling one procedure. lib/mini-c.mx and
# lib/mini-python.mx are those procedures, twice over, and a backend is
# chosen where the file is read:
#
#     mx -u lib/mini.mx -u lib/mini-c.mx      -i prog.mini
#     mx -u lib/mini.mx -u lib/mini-python.mx -i prog.mini
#
# That shape needed nothing built. `-u` may be given more than once, a rule may
# call a template that arrived in another file, and a parameter carries its
# hole's level so each backend brackets its own way (REFERENCE §9, §3.8, §8.3).
# What it still lacks is docs/ROADMAP.md 15, and this script is that item's
# customer: without it the shape is reachable by a reader of the reference and
# pinned by nothing.
#
# The oracle is that **both programs run and agree**. examples/mini.mx is the
# body with the C backend used from inside it, and make check already diffs
# what it expands to against examples/mini.out; a diff cannot say whether the
# C is C. So the C is compiled and run, the same body is read out to Python
# and run by python3, and the two must print the same three numbers. Numbers
# they both get wrong are caught by WANT below, which is what the program is
# worked out on paper to print: 1, 4, 9, 16 and 25 summed to 55, with the two
# squares over 9 printed as they are reached.
#
# Four other things are checked here because each is a way the demonstration
# could be hollow rather than wrong:
#
#   the two forms agree     the body read with -u -i must give the C that the
#                           whole file gives, byte for byte, which is
#                           tests/cpp.sh's claim for the split form and is
#                           what lets one body stand for both runs.
#   the targets differ      a backend that quietly emitted the other's text
#                           would pass every check above. So the C must have a
#                           main and the Python must not, and the Python must
#                           carry no semicolon.
#   the grammar is bare     `mx -g -u lib/mini.mx` must fail, naming a
#                           procedure it has no definition for. A grammar that
#                           can be read on its own has a target in it.
#   the backends match      both must define the same eleven procedure names,
#                           which is the contract nothing in the tool states.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"

TMP="${TMPDIR:-/tmp}/mx-mini.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

WANT='16
25
55'

# ----------------------------------------------------------- the whole file,
# ----------------------------------------------------------- expanded to C.
if ! sh tests/limit.sh "$LIMIT" "$MX" examples/mini.mx > "$TMP/whole.c" 2> "$TMP/err"; then
    echo "FAILED  mini.sh: examples/mini.mx did not expand"
    cat "$TMP/err"
    exit 1
fi

if ! "$CC" -std=c11 -o "$TMP/a" "$TMP/whole.c" 2> "$TMP/cc.err"; then
    echo "FAILED  mini.sh: the C from examples/mini.mx does not compile"
    cat "$TMP/cc.err"
    exit 1
fi
got=$("$TMP/a")
if [ "$got" != "$WANT" ]; then
    echo "FAILED  mini.sh: the C compiled and computed the wrong thing."
    echo "        wanted: $(echo "$WANT" | tr '\n' '/')"
    echo "        got:    $(echo "$got"  | tr '\n' '/')"
    exit 1
fi

# ------------------------------------------------------------ the same body,
# ------------------------------------------------------------ as two files.
awk 'f { print } /^@end/ { f = 1 }' examples/mini.mx > "$TMP/prog.mini"

if ! sh tests/limit.sh "$LIMIT" "$MX" -u lib/mini.mx -u lib/mini-c.mx \
        -i "$TMP/prog.mini" > "$TMP/split.c" 2> "$TMP/err"; then
    echo "FAILED  mini.sh: mx -u lib/mini.mx -u lib/mini-c.mx -i prog.mini did not expand"
    cat "$TMP/err"
    exit 1
fi
if ! diff -u "$TMP/whole.c" "$TMP/split.c" > "$TMP/diff"; then
    echo "FAILED  mini.sh: the two forms of the same run disagree"
    echo "        left: mx examples/mini.mx     right: mx -u … -i prog.mini"
    cat "$TMP/diff"
    exit 1
fi

if ! sh tests/limit.sh "$LIMIT" "$MX" -u lib/mini.mx -u lib/mini-python.mx \
        -i "$TMP/prog.mini" > "$TMP/prog.py" 2> "$TMP/err"; then
    echo "FAILED  mini.sh: mx -u lib/mini.mx -u lib/mini-python.mx -i prog.mini did not expand"
    cat "$TMP/err"
    exit 1
fi

# ------------------------------------------------------ two targets, and the
# ------------------------------------------------------ ways of being hollow.
cmain=$(grep -c '^int main(void)$' "$TMP/whole.c")
pmain=$(grep -c 'int main' "$TMP/prog.py")
psemi=$(grep -c ';' "$TMP/prog.py")
if [ "$cmain" != 1 ] || [ "$pmain" != 0 ] || [ "$psemi" != 0 ]; then
    echo "FAILED  mini.sh: the two backends did not write two different languages"
    echo "        main in the C: $cmain (want 1)"
    echo "        main in the Python: $pmain (want 0), semicolons: $psemi (want 0)"
    exit 1
fi

if sh tests/limit.sh "$LIMIT" "$MX" -g -u lib/mini.mx > "$TMP/g.out" 2> "$TMP/g.err"; then
    echo "FAILED  mini.sh: lib/mini.mx was read on its own, so it names a target"
    echo "        A grammar whose every template is a call cannot be sealed"
    echo "        without a backend, and that is the property being claimed."
    exit 1
fi
if ! grep -q "no template called" "$TMP/g.err"; then
    echo "FAILED  mini.sh: lib/mini.mx failed on its own for the wrong reason"
    cat "$TMP/g.err"
    exit 1
fi

names() { sed -n 's/^@template \([a-z_]*\)(.*/\1/p' "$1" | sort; }
names lib/mini-c.mx > "$TMP/c.names"
names lib/mini-python.mx > "$TMP/py.names"
count=$(wc -l < "$TMP/c.names" | tr -d ' ')
if ! diff -u "$TMP/c.names" "$TMP/py.names" > "$TMP/names.diff" || [ "$count" != 11 ]; then
    echo "FAILED  mini.sh: the two backends do not define the same procedures"
    echo "        C defines $count (want 11), and against the Python:"
    cat "$TMP/names.diff"
    exit 1
fi

# ------------------------------------------------------------- and the other
# ------------------------------------------------------------- language, run.
if ! command -v python3 > /dev/null 2>&1; then
    echo "ok      mini.sh: the C compiles, runs, and prints 16 25 55"
    echo "            and the same body read with lib/mini-python.mx gives"
    echo "            Python, which was not run: python3 is not installed,"
    echo "            so the half that compares the two targets was skipped."
    exit 0
fi

pygot=$(sh tests/limit.sh "$LIMIT" python3 "$TMP/prog.py" 2> "$TMP/py.err")
if [ $? -ne 0 ]; then
    echo "FAILED  mini.sh: what lib/mini-python.mx wrote is not valid Python"
    cat "$TMP/py.err"
    exit 1
fi
if [ "$pygot" != "$got" ]; then
    echo "FAILED  mini.sh: the two targets disagree about the same program"
    echo "        the C says:      $(echo "$got"   | tr '\n' '/')"
    echo "        the Python says: $(echo "$pygot" | tr '\n' '/')"
    echo "        Both ran. One of the two backends is wrong, which is the"
    echo "        failure a recorded .out cannot have."
    exit 1
fi

echo "ok      mini.sh: one grammar, two backends: the C compiles and prints 16 25 55"
echo "            python3 runs the other target's text to the same three numbers,"
echo "            the whole file and the two-file form give the same C, and"
echo "            lib/mini.mx on its own is refused for naming no target"
exit 0
