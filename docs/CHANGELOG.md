# Changelog

*The one page that says **when**, for a reader who is not reading the source.
Only what shipped and is visible from outside: a directive that appeared, a
behaviour that changed, a message somebody will now see. Internal reworking, and
the reasoning behind any of it, belongs in [the journal](work-journal/) instead.*

Prototype has not been released, so there are no version numbers yet. Entries
are grouped by the day the work happened, newest first.

## 2026-09-04

The first day. The notation, the tool, and everything below.

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
