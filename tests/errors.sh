#!/bin/sh
# errors.sh -- what a file gets told when it is wrong.
#
# An error message is a surface like any other and goes stale the same way, so
# each one here is pinned to the text it is meant to produce. `pt` must exit
# non-zero and say the fragment.

PT="${1:-./bin/pt}"
TMP="${TMPDIR:-/tmp}/pt-errors.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

fail=0
n=0

expect() {
    want="$1"; shift
    n=$((n + 1))
    f="$TMP/case$n.pt"
    cat > "$f"
    got=$("$PT" "$f" 2>&1)
    if [ $? -eq 0 ]; then
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

expect "no kind or token class called 'block'" <<'EOF'
@syntax "f" a:block => "{a}"
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
@use "no-such-file.pt"
EOF

# The mode is named after the rule on purpose: this is checked once the header
# has finished speaking, not by the rule that declared it, so the order a file
# writes its directives in cannot let it through.
expect "text mode has no tokens" <<'EOF'
@token name "[a-z]+"
@syntax "[" x:name "]" => "<{x}>"
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

if [ $fail -eq 0 ]; then
    echo "ok      errors.sh: $n cases"
fi
exit $fail
