# Changelog

*The one page that says **when**, for a reader who is not reading the source.
Only what shipped and is visible from outside: a directive that appeared, a
behaviour that changed, a message somebody will now see. Internal reworking, and
the reasoning behind any of it, belongs in [the journal](work-journal/) instead.*

Prototype has not been released, so there are no version numbers yet. Entries
are grouped by the day the work happened, newest first.

## 2026-09-05

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
