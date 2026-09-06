# Changelog

*The one page that says **when**, for a reader who is not reading the source.
Only what shipped and is visible from outside: a directive that appeared, a
behaviour that changed, a message somebody will now see. Internal reworking, and
the reasoning behind any of it, belongs in [the journal](work-journal/) instead.*

Metaxis has not been released, so there are no version numbers yet. Entries
are grouped by the day the work happened, newest first.

## 2026-09-05

**`indent(s, n)` in a code template**, and `examples/code.mx` now emits indented
C. Every line moves right, the first included, and an empty line stays empty;
nesting composes, because an inner block is already indented when the outer one
indents it. The C that `examples/code.out` records was previously flat — braces
opened and nothing moved — and `tests/pascal.sh` still compiles it and gets the
same `4 44 80 7 42`, so what changed is the reading and not the meaning. The
string template has no equivalent and is not getting one until something asks.

**`mx -t`** — the parse, traced to standard error: one line per candidate tried,
indented by depth, saying which token it could not get past, and totals at the
end. Expression mode only.

**Expansion is no longer quadratic in the size of the input.** A generated
4985-line Pascal program took **67 seconds** and now takes **174ms**; 16000
statements expand in 524ms. `regexec` measures the whole remaining file on every
call, so each token cost O(rest of file) across every declared class; class
patterns are compiled anchored, so matching now happens against a bounded window
that grows only when a match reaches its edge. Two smaller quadratics went with
it — the per-token line count and the token array's growth. Nothing about what
is emitted changed. One consequence worth knowing: a `@token` pattern that
anchors its end with `$` now sees the window rather than the whole file.

**`as <name>` on a template, and `mx -b <name>`** — a rule may now carry more
than one template, so one grammar can be read out to more than one target. The
untagged template is the default and the fallback, so a second target costs only
the rules that actually differ and a one-target file is unchanged. `terminated`
moved from the rule to the template, because one target may brace a branch where
another does not. `mx -g` lists the tags and no longer needs `-b`;
`make check` and `make record` run every declared backend against
`<name>-<backend>.out`. New: `examples/backends.mx`. New messages:
`this rule already emits 'x'`, `this rule already has an untagged template`,
`expected a name after 'as'`, `no rule emits 'x'`,
`every template here is tagged, so there is no default`, and
`this rule emits nothing for 'x'`.

**`@mode` declared twice is refused**, unless the second says `override`, as
every other repeatable declaration already was. `@mode` also **refuses trailing
text** now, which `@token` and `@separator` began doing earlier the same day and
this one was missed by. Together those closed a third silence nobody had listed:
`@mode expression override` parsed and meant nothing, because the word after the
mode was ignored. New messages: `the mode is already declared at f:n -- write
'override' to mean it`, `'override', but no mode was declared before it`, and
`trailing text after @mode`.

**`@separator "…" indent`, and the `block` kind** — a language whose blocks are
an indentation can now be read. `indent` makes the lexer keep a stack of columns
and emit two tokens no file spells, an *indent* where a line is deeper than the
one before it and one *dedent* per level closed where it is shallower; a hole
written `b:block` reads them, taking the indented run of statements and both of
its delimiters. It needs a separator with a newline in it. Blank lines and
comment-only lines close nothing, a tab is 8 columns, and a `dedent` counts as
ending a statement in a word, so nothing has to separate a block from the line
after it. Unlike a `stmts` hole a `block` needs no word to stop at and may be
followed by one, which is what lets `"if" c ":" b:block "else" ":" e:block` be a
rule. `examples/python.pt` reads Python into C, and `tests/python.sh` compiles
that C, runs it, and **also runs the same text under `python3`** and compares
the two answers.

New messages: `'indent' needs a separator with a newline in it`,
`'b:block' wants a block, and nothing here opens one`,
`this line is indented and no rule opened a block here`,
`this line ends a block but lines up with nothing that opened one`, and a
`block` hole is refused in text mode. `@token block` joins `expr`, `stmts` and
`text` as a class name that could never be used.

**`@fragment name = pattern`** — a piece of *pattern* with a name, spliced into
a rule with `@name`. It brings its own holes, so a rule that splices it can
write `{p}` without declaring `p`; it takes no arguments and has no scope,
because it is spliced at declaration rather than called at expansion. It must be
declared before it is spliced, which makes a cycle inexpressible and the order a
file is written in unable to change the answer. `override` sits before the `=`,
which is the opposite of every other directive and is forced — a fragment's
pattern runs to the end of the directive, so a trailing `override` would be read
as a hole of that name. `examples/pascal.pt` and `examples/code.pt` each wrote
one parameter list twice and now write it once; both expand byte-identically to
what they did before. `examples/code.pt` also shares the body the two rules had
in common through a `@template`, so the two ways of naming a fragment now meet
in one file.

**`make check` now reports a hang instead of waiting on one.** Every place the
suite runs `pt` goes through `tests/limit.sh`, which kills a command that
outlives a wall-clock limit and reports it as its own kind of failure —
previously a `.pt` file that never terminated stopped the suite rather than
failing it, which no recorded output could express. `make check LIMIT=30` raises
the limit, which defaults to 10 seconds; the full run takes 2.3.

**`@token expr`, `@token stmts` and `@token text` are now refused** —
`'expr' is a kind, so a class called that could never be used`. A hole written
`x:expr` resolves as the *kind*, so a class of that name was never consulted:
`expr` said nothing at all, and `text` reached `a 'text' hole belongs to @mode
text`, an error about the wrong thing.

**A `for` inside a `@template` body no longer crashes.** It read through a null
rule at seal and segfaulted, so no template that looped had ever run; the check
it crashed in — a loop variable shadowing a hole — does not apply to a template,
which has no holes to shadow. [POSTMORTEM.md](POSTMORTEM.md) 12.

**A pattern that declares one hole name twice is now refused** —
`two holes called 'p': a template splices a hole by name, so only one of them
could ever be reached`. It was always a mistake and nothing had ever written
one; splicing a fragment twice into a rule makes it easy to make by accident,
which is what asked for the check.

**`@template name(x) { … }`** — a piece of template with a name, called as a
statement from a code template and emitting into whatever called it. Its body
sees its parameters and its own loop variables and nothing else, so it can be
read on its own; calls resolve once the header has finished, so a rule may call
one declared after it or brought in by `@use`; a template may call a template,
64 deep. At most 8 parameters, and a duplicate name is refused unless it says
`override`. `examples/asm.pt` has one `load` against eight call sites.

A statement may now be a call, so `expected 'emit', 'if' or 'for'` is now
`expected 'emit', 'if', 'for' or a template call`.

**`fresh(label)` in a code template now gives one name per label per
application**, which is what `{~label}` in a string template has always done and
what this page and the reference had both said it did. It returned a new name on
every call, so a template could not put a label at a branch and at the place the
branch jumps to. Exhausting the name space is now an error rather than a crash.

**`examples/asm.pt`** — C in, arm64 assembly out, assembled and run by
`tests/asm.sh`.

**Arithmetic in a code template, and `num(h)`.** `-`, `*`, `/` and `%` are new;
`*` `/` `%` bind tighter than `+` `-`. They want two numbers and are an error
otherwise, and `num(h)` reads a hole's text as one — the whole text or none of
it. **`+` changes**: it adds when both sides are already numbers and joins when
they are not, which is the rule comparison has always used. `count(a) + count(b)`
therefore writes `3` where it used to write `12`; nothing in `examples/` did
that. Division or remainder by zero is an error.

`examples/calc.pt` is the first file here that does not translate its language
but **runs** it.

**`for i, x in h`, and `at(h, n)`.** A `for` in a code template may name the
position as well as the turn — first name is the index, counting from 0 — and
`at(h, n)` takes the turn at a position. Together they walk **two holes of one
repeated group in step**, which is the only way to write `[ v ":" s ]*` and emit
the pairs; a position past the end of a list is an error rather than an empty
string, because two groups of different lengths is the mistake worth catching.
`examples/code.pt`'s `case` is the customer.

**`repeat … until` and `case … of` in the Pascal examples**, inverting the
condition for C's `do … while` and giving every `case` arm the `break` Pascal
does not need. No change to the tool.

**`procedure` and `function` in the Pascal examples.** With parameter lists,
calls, and Free Pascal's `Result :=` for the return value. `tests/pascal.sh`
compiles and runs what comes out and now checks four values rather than two.
No change to the tool: the parameter list is a repeated group with `sep ";"`,
the body is a `stmts` hole stopping at `end`, and a call is a led `"(" … ")"`
at 95.

**`terminated(h)` in a code template.** A new builtin beside `level(h)`: whether
the rule that filled a hole was declared `terminated`, that is, whether the
hole's text already ends a statement. For a `stmts` hole it is the last
statement that answers. It is what lets a rule decide whether what came out of a
hole needs a semicolon, and `examples/code.pt` uses it to emit
`if (c) x = 1; else y = 2` and `if (c) { … } else y = 2` from one rule.

**`override`, and two files declaring one thing.** A rule's pattern, a `@token`
class name and `@separator` may each be declared only once; a second is an error
naming both lines. `override` — after the template for a rule, after the
declaration for the other two — says the second means to displace the first, and
then it wins silently. `override` with nothing to displace is also an error.
Two rules that only share a leading word are unaffected. A hole may still be
called `override`.

**`@token` and `@separator` refuse trailing text**, as `@syntax` always has.
`@token name "…" garbage` used to be accepted and the extra word ignored; it is
now `trailing text after @token`. This is what makes the optional `override`
after them unambiguous.

**`@use` reads a file once**, however many times it is reached, so a diamond
costs nothing and a cycle ends rather than hitting the depth guard.

**A class-kind hole is refused in text mode.** `@syntax "[" x:name "]"` under
`@mode text` used to take everything up to the `]` and ignore the kind. It now
fails: `'x:name' asks for one token of a class, and text mode has no tokens —
every hole there is text`. `expr`, `stmts`, `text` and a bare hole are
unaffected, and expression mode is unaffected. The check runs once the whole
header has been read, so the order a file writes `@mode` and `@syntax` in — and
whether the rule came in through `@use` — does not change the answer.

## 2026-09-04

The first day. The notation, the tool, and everything below.

**Groups in text mode.** `[ … ]`, `[ … ]*` and `[ … ]+` work under `@mode text`.
Matching there is now a search with backtracking rather than a single forward
scan, and a hole takes the shortest run that lets the rest of the pattern match
without spanning the word that closes the rule.

**`terminated`.** A word after a template, saying the rule's output already ends
a statement so no separator is joined after it. A hole may still be called
`terminated`.

**Two kinds of template.** `=> "…"` splices. `=> { … }` is a small interpreted
language: `emit`, `if`/`else`, `for … in … sep …`, text with `+`, comparison,
`and`/`or`/`not`, and the builtins `matched`, `count`, `level`, `group`,
`replace`, `drop` and `fresh`. One character after the `=>` says which form it
is. A directive no longer ends at a newline while a brace is open.

**The notation.** A `.pt` file declares its own grammar in a header and is then
read with it. Every mention of foreign text inside a directive is a string —
quoted words on the pattern side, a quoted template with `{hole}` splices on the
output side.

**Directives.** `@syntax`, `@token`, `@comment`, `@separator`, `@use`, `@mode`,
`@end`.

**Patterns.** Infix, prefix, postfix, circumfix and mixfix, told apart by shape
rather than by four directives. Hole kinds `expr`, `stmts`, `text` and any
`@token` class. Levels with `left` and `right`.

**Groups.** `[ … ]`, `[ … ]*` and `[ … ]+`, with `sep` and `join` on a repeated
one — a part that may repeat and a part that need not be there.

**Templates.** `{hole}` splices, `{{` and `}}` for a literal brace, and `{~t}`
for a name nobody else has.

**Two modes.** `@mode expression` parses the whole body; `@mode text` fires a
rule where one matches and copies the rest through.

**The command line.** `pt [-o output] [-g] file.pt`. `-g` prints the grammar the
header declared and stops.

**Errors.** Every splice in a template is checked at the `@syntax` that wrote it
rather than at the first use of the rule. Twenty error cases are pinned in
`tests/errors.sh`.
