#!/bin/sh
# errors.sh -- what a file gets told when it is wrong.
#
# An error message is a surface like any other and goes stale the same way, so
# each one here is pinned to the text it is meant to produce. `mx` must exit
# non-zero and say the fragment.

MX="${1:-./bin/mx}"
LIMIT="${LIMIT:-10}"
TMP="${TMPDIR:-/tmp}/mx-errors.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

fail=0
n=0

# Set before a case to pass a flag to that one case, and cleared by `expect`
# afterwards. Two of the messages below are about `-b` and are unreachable
# without it, and a message this file cannot reach is a message nothing pins.
FLAGS=''

expect() {
    want="$1"; shift
    n=$((n + 1))
    f="$TMP/case$n.mx"
    cat > "$f"
    got=$(sh tests/limit.sh "$LIMIT" "$MX" $FLAGS "$f" 2>&1)
    rc=$?
    FLAGS=''
    if [ $rc -eq 124 ]; then
        echo "FAILED  errors.sh case $n: did not finish in ${LIMIT}s -- killed."
        echo "        Refusing a bad file should be immediate; a hang here is a"
        echo "        defect in the check, not a slow machine."
        fail=1
        return
    fi
    if [ $rc -eq 0 ]; then
        echo "FAILED  errors.sh case $n: expected a failure, got output"
        fail=1
        return
    fi
    case "$got" in
        *"$want"*) echo "ok      errors.sh case $n: $want" ;;
        *) echo "FAILED  errors.sh case $n"
           echo "        wanted: $want"
           echo "        got:    $got"
           fail=1 ;;
    esac
}

expect "no directive called '@infix'" <<'EOF'
@infix "+" 60 add
EOF

expect "begins with a hole is infix or postfix and needs a level" <<'EOF'
@syntax a "+" b => "{a}"
EOF

expect "two holes in a row" <<'EOF'
@syntax "f" a b 10 => "{a}{b}"
EOF

expect "the pattern has no such hole" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => "{b}"
@end
f x
EOF

expect "no kind or token class called 'phrase'" <<'EOF'
@syntax "f" a:phrase => "{a}"
EOF

expect "needs a word after it to stop at" <<'EOF'
@token name "[a-z]+"
@syntax "begin" b:stmts => "{b}"
EOF

expect "unterminated string" <<'EOF'
@syntax "f => "x"
EOF

expect "nothing here is anything this file declared" <<'EOF'
@token name "[a-z]+"
@end
a $ b
EOF

expect "no rule reads" <<'EOF'
@token name "[a-z]+"
@syntax a "+" b 60 => "{a}{b}"
@end
a b
EOF

expect "the file ends in the middle of something" <<'EOF'
@token name "[a-z]+"
@syntax a "+" b 60 => "{a}{b}"
@end
a +
EOF

expect "a 'text' hole belongs to @mode text" <<'EOF'
@token name "[a-z]+"
@syntax "f" a:text "g" => "{a}"
@end
f a g
EOF

expect "a fresh name needs a label" <<'EOF'
@syntax "f" a => "{~}{a}"
EOF

expect "cannot share a label" <<'EOF'
@syntax "f" a => "{~a} {a}"
EOF

expect "cannot begin with a group" <<'EOF'
@token name "[a-z]+"
@syntax [ "x" ] a => "{a}"
EOF

expect "a group needs something in it" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ ] a => "{a}"
EOF

expect "belongs to a repeated group" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a ] sep "," => "{a}"
EOF

expect "expected ']'" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a => "{a}"
EOF

expect "needs a 'sep' to know where one turn stops" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a ]* "end" => "{a}"
EOF

expect "the template uses 'b' and the pattern has no such hole" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit b }
EOF

expect "no such thing as 'shout'" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit shout(a) }
EOF

expect "'group' takes 2 and was given 1" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit group(a) }
EOF

expect "the loop variable 'a' is also a hole" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a:name ]+ => { for a in a { emit a } }
EOF

expect "expected 'emit', 'if', 'for' or a template call" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { a }
EOF

expect "is one of this language's own words" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit in }
EOF

expect "a block ends in the middle of something" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit a
EOF

expect "trailing text after the template" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => "{a}" terminated nonsense
EOF

expect "cannot open" <<'EOF'
@use "no-such-file.mx"
EOF

# A block hole in text mode. The mode is named after the rule on purpose: this
# is checked once the header has finished speaking, not by the rule that
# declared it, so the order a file writes its directives in cannot let it
# through. (A class hole was refused here too until 2026-09-06, when text mode
# began consulting the classes a file declares; examples/island.mx has one.)
expect "text mode has no tokens to measure the indentation of" <<'EOF'
@separator "\n" indent
@syntax "if" c ":" b:block => "{c}{b}"
@mode text
EOF

# A led rule in text mode. Until 2026-09-06 this was accepted and the rule never
# fired: `p->f` came through a text-mode file unchanged, with no message and
# exit 0. Same shape as the block case above -- a rule that reads as if it
# worked -- and refused at the same place, once the header has finished.
expect "text mode has nothing for it to continue" <<'EOF'
@syntax a "->" b 50 => "{a}.{b}"
@mode text
EOF

# Two files declaring one thing. The hole names differ on purpose: what makes
# the second rule unreachable is its pattern, and a pattern does not know what
# its holes were called.
expect "this pattern is already declared" <<'EOF'
@token number "[0-9]+"
@syntax a "+" b 60 => "add({a},{b})"
@syntax x "+" y 60 => "plus({x},{y})"
EOF

expect "nothing with this pattern was declared before it" <<'EOF'
@token number "[0-9]+"
@syntax a "+" b 60 => "add({a},{b})" override
EOF

expect "the class 'name' is already declared" <<'EOF'
@token name "[a-z]+"
@token name "[A-Z]+"
EOF

expect "no class 'name' was declared before it" <<'EOF'
@token name "[a-z]+" override
EOF

expect "the separator is already declared" <<'EOF'
@separator ";"
@separator "."
EOF

expect "no separator was declared before it" <<'EOF'
@separator ";" override
EOF

# `dtake` matches a prefix, so without this 'overridden' would be read as
# 'override' with three characters dropped after it in silence.
expect "trailing text after @token" <<'EOF'
@token name "[a-z]+" overridden
EOF

expect "trailing text after @separator" <<'EOF'
@separator ";" nonsense
EOF

expect "expected a name after ',' in 'for'" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a:name ]* sep "," => { for i, in a { emit i } }
EOF

expect "names the position and the turn the same thing" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a:name ]* sep "," => { for i, i in a { emit i } }
EOF

# Out of range is an error and not an empty string: `at` is for walking two
# groups together, and two groups of different lengths is what that gets wrong.
expect "'at' was given 9 and there are 2" <<'EOF'
@token name "[a-z]+"
@syntax "f" [ a:name ]* sep "," => { for i, x in a { emit at(a, 9) } }
@end
f x, y
EOF

# Arithmetic never reads a number out of text that only looks like one, so an
# operand that is not already a number is an error and not a silent zero.
expect "wants two numbers and was given" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit a * 2 }
@end
f x
EOF

expect "'num' wants a number and was given 'x'" <<'EOF'
@token name "[a-z]+"
@syntax "f" a => { emit num(a) }
@end
f x
EOF

expect "'/' by zero" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { emit num(a) / 0 }
@end
f 5
EOF

# A template is resolved once the header has finished, so a rule may call one
# declared after it. These are the ways that can still be wrong.
expect "no template called 'nope'" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { nope(a) }
EOF

expect "'t' takes 2 and was given 1" <<'EOF'
@token number "[0-9]+"
@template t(x, y) { emit x }
@syntax "f" a => { t(a) }
EOF

expect "is not one of this template's parameters" <<'EOF'
@token number "[0-9]+"
@template t(x) { emit y }
@syntax "f" a => { t(a) }
EOF

expect "'level' is a builtin and gives a value" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { level(a) }
EOF

# The other way round: `contribute` is a statement, as `emit` is, and has no
# value. `splice` is the expression half and is covered by the case above it.
expect "'contribute' is a statement" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { emit contribute("vars", a) }
EOF

expect "'contribute' takes 2" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { contribute(a) }
EOF

expect "'splice' takes 1 and was given 2" <<'EOF'
@token number "[0-9]+"
@syntax "f" a => { emit splice("vars", a) }
EOF

expect "'t' is a template" <<'EOF'
@token number "[0-9]+"
@template t(x) { emit x }
@syntax "f" a => { emit t(a) }
EOF

expect "the template 't' is already declared" <<'EOF'
@token number "[0-9]+"
@template t(x) { emit x }
@template t(x) { emit x }
EOF

expect "no fragment called '@nope'" <<'EOF'
@token name "[A-Za-z_]+"
@syntax "f" @nope => "x"
EOF

expect "no fragment called '@p'" <<'EOF'
@token name "[A-Za-z_]+"
@syntax "f" @p => "x"
@fragment p = "a"
EOF

expect "expected a fragment's name after '@'" <<'EOF'
@token name "[A-Za-z_]+"
@fragment p = "a"
@syntax "f" @ => "x"
EOF

expect "a fragment needs something in it" <<'EOF'
@fragment p =
EOF

expect "expected a name after '@fragment'" <<'EOF'
@fragment = "a"
EOF

expect "expected '=' after a fragment's name" <<'EOF'
@fragment p "a"
EOF

expect "a level belongs to a rule and not to a fragment" <<'EOF'
@fragment p = "a" 50
EOF

expect "the fragment 'p' is already declared" <<'EOF'
@fragment p = "a"
@fragment p = "b"
EOF

expect "'override', but no fragment 'p' was declared before it" <<'EOF'
@fragment p override = "a"
EOF

expect "two holes called 'x'" <<'EOF'
@token name "[A-Za-z_]+"
@fragment p = x:name
@syntax "f" @p "," @p => "y"
EOF

expect "two holes called 'a'" <<'EOF'
@token name "[A-Za-z_]+"
@syntax "f" a:name "," a:name => "{a}"
EOF

expect "'expr' is a kind, so a class called that could never be used" <<'EOF'
@token expr "[a-z]+"
EOF

expect "'stmts' is a kind, so a class called that could never be used" <<'EOF'
@token stmts "[a-z]+"
EOF

expect "'text' is a kind, so a class called that could never be used" <<'EOF'
@token text "[a-z]+"
EOF

expect "'block' is a kind, so a class called that could never be used" <<'EOF'
@token block "[a-z]+"
EOF

# ------------------------------------------------------- indentation, and the
# ------------------------------------------------------- block kind it opens.

expect "'indent' needs a separator with a newline in it" <<'EOF'
@token name "[a-z]+"
@separator ";" indent
@end
a
EOF

expect "'b:block' wants a block, and nothing here opens one" <<'EOF'
@token name "[a-z]+"
@separator "\n"
@syntax "if" c ":" b:block => "{c}{b}"
@end
a
EOF

# The separator may be declared after the rule that depends on it, or in a file
# that `@use` pulled in, so the check is at seal and not where it is written.
expect "'b:block' wants a block, and nothing here opens one" <<'EOF'
@token name "[a-z]+"
@syntax "if" c ":" b:block => "{c}{b}"
@separator "\n"
@end
a
EOF

expect "asks for an indented run of statements" <<'EOF'
@mode text
@separator "\n" indent
@syntax "if" c ":" b:block => "{c}{b}"
@end
x
EOF

expect "this line ends a block but lines up with nothing that opened one" <<'EOF'
@token name "[a-z]+"
@separator "\n" indent
@syntax "if" c ":" b:block => "{c}{b}"
@end
if a:
    b
  c
EOF

expect "this line is indented and no rule opened a block here" <<'EOF'
@token name "[a-z]+"
@separator "\n" indent
@syntax "if" c => "{c}"
@end
if a
    b
EOF

# `@mode` was the last global that replaced itself in silence and the last one
# that ignored whatever followed it -- so `@mode expression override` parsed and
# meant nothing. Three cases, because there were three silences and the roadmap
# item had named one.

expect "the mode is already declared at" <<'EOF'
@mode text
@mode expression
@token name "[a-z]+"
@syntax "hi" => "yo"
@end
hi
EOF

expect "trailing text after @mode" <<'EOF'
@mode expression zzz
@token name "[a-z]+"
@syntax "hi" => "yo"
@end
hi
EOF

expect "'override', but no mode was declared before it" <<'EOF'
@mode expression override
@token name "[a-z]+"
@syntax "hi" => "yo"
@end
hi
EOF

# `as`, and the four ways a rule can be wrong about which target it emits for.

expect "this rule already emits 'tight'" <<'EOF'
@token number "[0-9]+"
@syntax "n" => "a" as tight => "b" as tight
@end
n
EOF

expect "this rule already has an untagged template" <<'EOF'
@token number "[0-9]+"
@syntax "n" => "a" => "b"
@end
n
EOF

expect "expected a name after 'as'" <<'EOF'
@token number "[0-9]+"
@syntax "n" => "a" as
@end
n
EOF

expect "every template here is tagged, so there is no default" <<'EOF'
@token number "[0-9]+"
@syntax "n" => "a" as tight
@end
n
EOF

FLAGS="-b nope"
expect "no rule emits 'nope' -- this file declares tight" <<'EOF'
@token number "[0-9]+"
@syntax "n" => "a" => "b" as tight
@end
n
EOF

FLAGS="-b tight"
expect "this rule emits nothing for 'tight', and has no untagged template" <<'EOF'
@token number "[0-9]+"
@separator ";" => ";\n"
@syntax "n" => "a" => "b" as tight
@syntax "m" => "c" as plain
@end
n; m
EOF

if [ $fail -eq 0 ]; then
    echo "ok      errors.sh: $n cases"
fi
exit $fail
