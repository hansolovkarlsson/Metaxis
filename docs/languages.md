# Languages

*What this tool can be pointed at and what it cannot, language by language,
for a reader who has one in hand and wants to know before writing a header.
Two lists, fit and unfit, and in front of them the seven properties of the
tool that every verdict rests on, so that a language not listed can be scored
the same way. This page is a survey in the sense [prior-art.md](prior-art.md)
is, and like that page it outranks nothing: [REFERENCE.md](REFERENCE.md)
states the properties, [direction.md](direction.md) argues for why the walls
stay where they are, and where this page and the code disagree the code is
right. Written 2026-09-06. Every verdict names the property it rests on, and
the files that decided them are kept in `docs/languages/` and run by the suite:
the last section, *Run, not reasoned*, shows each with its transcript.*

---

## What decides it

A language is **fit** when the programs people actually write in it can be
read whole, or when the subset people would want is readable and the rest is
a wall the file can name in its own closing note, the way `examples/python.mx`
does. It is **unfit** when the ordinary program meets a wall: when reading it
would mean, in [direction.md](direction.md)'s phrase, *becoming a different
tool*. Unfit is a statement about the whole language. A subset of nearly
anything reads, and the fit list says where the useful subsets stop.

Seven properties decide it. Each is a fact about the tool, stated where the
reference states it, and each has a list of languages it settles.

1. **A rule sees its own pattern and nothing else.** No symbol table, no
   types, no scope. That is the property [direction.md](direction.md) is
   built around, and it is the wall at `x * y` in C, at `Banner;` in Pascal,
   at the first assignment in Python that C wants declared, and at any
   output that needs a type the source did not spell. Collections
   (REFERENCE §8.4) reach the one case that needs no reading back: a head
   that is the aggregate of its body.

2. **The lexer is regexes and quoted words, and its only state is the
   indent stack.** REFERENCE §6.1. At each position the longest class match
   and the longest word are taken, the class wins a tie, and nothing that
   came before changes what a character means. Four things follow, and the
   first three are run below, in `case.mx`, `nest.mx` and `tail.mx`:
   - Matching is **case-sensitive**, so `BEGIN` and `begin` are two words
     and a case-insensitive language reads only in the case its header
     spelled.
   - A block comment **does not nest**: `/* a /* b */ c */` closes at the
     first `*/`.
   - A class that could match **the rest of a line** swallows the keyword in
     front of it, since it is the longer match, so no rule can read a line
     whose tail is another language: a Dockerfile's `RUN`, a Makefile's
     recipe.
   - Anything the lexer would have to *decide* is out: whether `/` begins a
     regex or divides, where a heredoc ends, what `${` nests inside a
     string, what column a token is in, whether the text after `<` is a
     tag name or the words of a paragraph. `xml-text.mx` below is the last
     of those.

3. **A list is separated by a word, or it is not read.** A repeated group
   whose turns are expressions needs a `sep` word to know where one turn
   stops, and whitespace is never a word. A rule that begins with a hole
   must have a word second, so a led rule cannot be two holes. Between them
   this rules out **juxtaposition**: `f x y` as application, `(f x (g y))`
   as a call, `a b c` as a command line, `ab` as concatenation. It is the
   property that surprised this page, and it puts the smallest grammar on
   it in the unfit list. `lisp.mx` and `atoms.mx` below are its two sides.

4. **A newline is whitespace or a separator, and no bracket suspends it.**
   [ROADMAP.md](ROADMAP.md) 2. A language whose statements end at a line
   break reads under `@separator "\n"`, and then a call wrapped onto a
   second line does not, because the lexer cannot know what a bracket is.
   A language that ends statements with a word does not care. `toml.mx` and
   `toml-wrapped.mx` below are the two sides of the line.

5. **A block is braces, a pair of words, or one shape of indentation.**
   `@separator … indent` and the `block` kind, REFERENCE §4.3, are Python's
   shape: a deeper line opens a block and a shallower one closes it. A
   layout rule that opens a block at the column of the next token, or a
   language where the tab is not eight columns, is not that shape.

6. **Text mode is a scan, and what it knows of a language is what the file
   declares.** REFERENCE §7 and §3.11. A rule fires where its words match and
   everything else passes through, which is the island grammar every tool
   that works on real code has, and since 2026-09-06 it has the three things
   those tools know: where a `@token` class matches the scan moves by the
   token, so a rename over `err` leaves `stderr` alone and a string or a
   comment declared as a class is passed over whole, and a hole stops only
   where the `@bracket` pairs balance, so a hole over `f(x, g(y))` is all of
   it. What it still lacks is a column: a construct defined by its
   indentation has nothing to measure it. So text mode is for markup and for
   a rewrite over a real file, and a formatter would want the columns it does
   not have.

7. **Output is text, built upward, and any language can be written.** The
   question on the output side is never the grammar, since there is none to
   satisfy; it is whether the source carries what the output wants. BASIC's
   sigils carry a type and Python's annotations carry one, and Pascal's
   `writeln` carries nothing, which is why stage 1 stopped there. A code
   template has `indent(s, n)` for an output whose blocks are indentation.

**Two readings of the same list.** The properties are limits of the tool as
it stands, and most of them are limits by decision: 1 is what keeps a rule
composable and `@use`-able, and 2, 3 and 4 are what keep the lexer something
a header can fully declare. The ones that are merely unbuilt say so:
[ROADMAP.md](ROADMAP.md) 2 would move a verdict or two below, and nothing
else on the roadmap would.

---

## Fit

*The input side. Each row says how far the tool reads and which property
stops it, and a row that says **whole** means an ordinary program in that
language reads without a wall.*

| language | reads | why, and where it stops |
| --- | --- | --- |
| **A notation of your own** | whole | The job the tool exists for, and the one no other tool on the prior-art page can do: the grammar is declared in the file that uses it, so no wall arrives that the author did not choose. `examples/tour.mx` and `examples/first.mx` are languages that exist nowhere else. |
| **Expression languages**: calculators, spreadsheet formulas, filter and query expressions | whole | A Pratt grammar is exactly this shape: literals by `@token`, infix by level, calls by a led `"("`. A cell range like `A1:B9` is one token class. `examples/calc.mx` evaluates instead of translating, and is the closest thing here to an interpreter (property 7 and [direction.md](direction.md)). |
| **Pascal** | a subset, and a large one | Stage 1. Every construct in a Wirth-style program reads: `program`, `var`, `procedure`, `function`, `begin … end`, `if`, `while`, `for`, `repeat`, `case`, over the types the header names. Property 1 stops it in three places [ROADMAP.md](ROADMAP.md) 1 records: assigning to the function's own name, calling a parameterless procedure, and `writeln` over mixed types. Property 2 adds one: the language is case-insensitive and the header is not, so a source that writes `Begin` and `BEGIN` on different lines needs a rule per spelling. |
| **BASIC**, line-numbered | whole | Stage 4, and the language that asked for collections: it declares nothing, C wants every variable declared first, and a rule `contribute`s the declaration for each name it meets. The type is on the sigil, `A$`, which is property 7 answered by the source. `examples/basic.mx` reads it and `tests/basic.sh` runs the C. |
| **Python** | a subset, and it says so | Stage 3, and the reason property 5 exists: `@separator "\n" indent` gives the lexer a stack of columns and a `block` hole reads what it emits. Nesting, `else` after a block, and blank lines all fell out. What stops it is 4, a call wrapped onto a second line ([ROADMAP.md](ROADMAP.md) 2), and 1, the first assignment C wants declared and the types C wants everywhere, which `examples/python.mx` takes off the source's own annotations. Its closing note lists the rest. |
| **Lua** | a subset, near whole | The smallest grammar of the general-purpose languages people ship code in: `end`-delimited blocks, `do`, `then`, `function`, a dozen operators, and no statement separator, which property 4 forgives because a statement that ends in `)` or `end` ends in a word and one that ends in a name has a newline after it. What stops it is a long string with a level, `[==[ … ]==]`, which POSIX ERE cannot count (property 2), and the ambiguity Lua's own manual records, a line beginning with `(` after one that ended in a name. `lua.mx` below reads the core. |
| **SQL** | a subset, one per dialect | Keyword-led mixfix with comma lists is the pattern language's native shape: `"select" [ c ]+ sep "," "from" t:name [ "where" w ] [ "order" "by" o ]` reads a query, and each optional clause is an optional group: `sql.mx`, below. Property 2 is the cost: `SELECT` and `select` are two words. The dialects are the other cost, and are not the tool's. |
| **Prolog** | a subset | An operator-precedence syntax is a Pratt grammar, and `op/3` is `@syntax` with a level: [prior-art.md](prior-art.md) § 2 counts Prolog among the ancestors for that reason. Terms, lists, clauses and `:-` all read. What stops it is a program that declares an operator in its own body with `:- op(700, xfx, ===)` and uses it on the next line: this tool reads the header first and the body second, and a directive in the body is a term (property 2). |
| **Assembly** | whole | Line-oriented, mnemonic-led, comma-separated operands, labels by a token class: nothing in it touches a property. Stage 2 *writes* it, `examples/asm.mx`, and `tests/asm.sh` runs the result on a CPU. Reading one stops only at a macro assembler's macros, which change what later lines mean (property 1). |
| **JSON** | whole | Two rules: an object is `"{" [ k:string ":" v ]* sep "," "}"` and an array is `"[" [ v ]* sep "," "]"`. The body is a single expression, so no separator is declared and property 4 never arises: `json.mx`, below. It is the smallest useful input, and a `.mx` that reads JSON and writes a C initialiser, a table, or another config format is a page long. |
| **Fixed-shape formats**: CSV, INI, TOML, Graphviz DOT, Protocol Buffers and GraphQL schemas | whole, when a statement stays on its line | Brackets, keywords and comma- or newline-separated lists, which is what the pattern language was made from. Each declares `@separator "\n"`, and property 4 then says what stops it: a TOML array or a DOT attribute list that wraps onto the next line is [ROADMAP.md](ROADMAP.md) 2. That is how the files are usually written, and a file that wraps is refused rather than misread: `toml.mx` and `toml-wrapped.mx`, below. |
| **CSS** | a subset | Rules, declarations and the common selectors read: `s "{" [ p:name ":" v ]* sep ";" "}"`. What stops it is property 2: an unquoted `url(a.png)` is a token that exists only after `url(`, and `calc()` has arithmetic the rest of the value language does not. `css.mx` below reads a rule. |
| **Markdown, wiki markup, lightweight markup** | whole, for what is inline and per line | Text mode's home, and `examples/poem.mx` is the demonstration: emphasis, links, headings, rules, lists, with the hole's text expanded in its turn so that constructs nest. What stops it is property 6: a construct defined by its column, an indented code block or a nested list, since text mode has no columns to measure. |

**And two rows for reading a real file rather than one written for a
grammar**, since that is the question a reader with a codebase is asking:

| language | reads | why |
| --- | --- | --- |
| **C, as an island** | a rewrite, and a rename | Stage 5. `lib/island.mx` rewrites `metaxis/cmd/mx.c`, the tool's own front end, and `tests/island.sh` compiles and runs the result. The file declares C's identifiers, strings, character literals and comments as classes and its three bracket pairs, which is all of property 6: the rename `out` to `res` leaves `outpath`, the usage string and the first comment alone, and a hole over `f(x, g(y))` is the whole call. On the day the stage landed a template that reused that hole wrote `log(f(x, g(y); complain(f(x, g(y)))`; `examples/island.mx` keeps the line and says what changed. |
| **Any language, in text mode** | a rewrite of the same shape | The rule is the same for all of them, which is why Comby works on thirty languages: declare the language's identifiers, strings and comments as classes and its brackets as `@bracket`, and write the rewrite. Comby ships those declarations for thirty languages; here they are the first six lines of a header. |

---

## Unfit

*The whole language, as its programs are written. Each row names the
property and the construct that meets it, and where a subset or a text-mode
rewrite is still worth having, the row says so.*

| language | why |
| --- | --- |
| **C** | Property 1 at its sharpest: `x * y` is a product or a declaration depending on whether `x` is a typedef, and no grammar decides it. The preprocessor is a second language interleaved by line, whose `#define` changes what the lines after it mean, and whose `#include` names text the tool cannot see. Declarators, `int (*f)(int)`, are readable and unpleasant. **What is fit**: an expression-and-statement subset over types the header names, which is what `examples/clike.mx` and stage 2 read, and text-mode rewrites over a real file, the row above. [direction.md](direction.md) says why a front end is declined. |
| **C++** | Everything C has, and then property 2: `a < b > c` is a template or two comparisons depending on what `a` is, and `>>` closes two templates or shifts. The most vexing parse is property 1 again. |
| **Java, C#, Kotlin, Swift** | The grammars run to hundreds of rules, which is cost and not a wall. The wall is generics against `<` (properties 1 and 2), interpolated strings that nest an expression inside a token (`$"…"`, `"${…}"`, `"\(…)"`, property 2), and that any translation worth doing, overload by overload, needs the types (property 1). A subset for a demonstration reads, as Pascal's did, and stops in the same places. |
| **Go** | Its lexer inserts a semicolon at a line break after a name, a literal or a closing bracket, which is nearly this tool's own rule, and the expression core reads. What stops it is property 4: `gofmt` puts every long argument list on several lines with a trailing comma, and each of those is [ROADMAP.md](ROADMAP.md) 2. Past the syntax, property 1 for the types. |
| **Rust** | Property 2 three times: `'a` is a lifetime or the start of `'a'`, block comments nest, and a generic `<` is a comparison until the name before it is known to be a type. And `macro_rules!` declares syntax the body then uses, which is what a `.mx` header does, except that it happens in the body. |
| **JavaScript, TypeScript** | Property 2: `/` divides after an operand and opens a regex literal after an operator, and a template literal's `${ … }` nests an expression inside a token, which no regex lexes. Automatic semicolon insertion is decided by the *next* token. JSX is a second grammar inside the first. **As an output** it is fit, and `examples/groups.mx` writes it. |
| **Ruby** | The lexer is the wall. `foo -1` and `foo - 1` differ, and which one `foo -1` is depends on whether `foo` is a local (properties 1 and 2 together); heredocs, `%w[…]` literals with a chosen delimiter, regex literals as in JavaScript, and `do … end` against `{ … }` chosen by the call. |
| **Perl** | The reference case for property 1: what a line means depends on prototypes declared earlier in the file and sometimes on what ran before it. `/`, `{` and `<` each mean two things by context (property 2). Nobody parses Perl statically and this tool is not the exception. |
| **PHP** | Two languages interleaved: HTML with `<?php … ?>` islands, which is text mode's shape, so a small rewrite over a PHP file is fit. Reading the PHP whole meets heredocs and interpolated strings (property 2). |
| **XML, HTML** | An element-only document reads, `"<" n:name [ a:name "=" v:string ]* ">" c:stmts "</" m:name ">"`, `xml.mx` below. Text between the tags does not: character data is a token only by where it stands, after a `>` and before a `<`, and a class declared for it matches after `<` as well. In a bare tag it ties with the tag name and the earlier declaration wins, so `<p>` alone reads by luck, and the first attribute makes it the longer match, where it swallows the tag name (property 2, and `xml-text.mx` below). Content that mixes text and elements with nothing between them is property 3 besides. HTML adds optional end tags, which need the tag table to know what a `</div>` closes (property 1). **Text mode** rewrites either in place, which is how they are usually touched. |
| **Haskell, OCaml, Standard ML, F#** | Property 3: application is juxtaposition, `f x y`, and there is no rule shape for it. Then property 5 for Haskell's layout rule, which opens a block at the column of the token after `where` or `do`; property 2 for `{- -}` and `(* *)`, which nest; and fixity declared in the body, as in Prolog. The expression core with every call parenthesised would read, and nobody writes it that way. |
| **Lisp, Scheme, Clojure** | The surprise, and the smallest grammar on the page. `(f x (g y) z)` is an argument list separated by nothing, and property 3 says a repeated group whose turns are expressions needs a `sep` word: `"(" f:name [ x ]* ")"` is refused at the header, and a `sep` of a space is a word the lexer never produces, since whitespace is skipped before a word is looked for. A list of atoms only, `[ x:name ]*`, reads; a nested one does not: `lisp.mx` and `atoms.mx`, below. |
| **Forth, and the concatenative languages** | Property 3, words separated by whitespace, and property 1 in its strongest form: a Forth program redefines its own reader as it runs. |
| **Shell**: sh, bash, zsh | A command line is words separated by whitespace (property 3), and what a word means is decided by quoting, by `$( … )`, by a heredoc's chosen terminator, and by position: `if` is a keyword only where a command may begin (property 2). Text mode edits a script; nothing reads one. |
| **Make, Dockerfile** | A line whose tail is another language, property 2: a recipe line is shell, and a `RUN` line is shell, and a class that could match the tail swallows the keyword. Make adds a tab that means *recipe* and a space that does not, where the indent stack counts a tab as eight spaces (property 5). Text mode edits either. |
| **YAML** | The indentation reads, since it is Python's shape. What stops it is a block scalar, `key: \|` followed by indented lines that are *text*, which is a lexer state beyond the indent stack (property 2); an anchor and its alias, `&a` and `*a`, which is a lookup (property 1); and flow style wrapped across lines (property 4). The subset of maps, lists and one-line scalars reads, and most configuration files are outside it somewhere. |
| **Fortran** (fixed form), **COBOL** | Column-significant: a label in columns 1 to 5, a continuation mark in 6, code from 7 to 72. The lexer counts columns for indentation and for nothing else (property 2). Fortran adds insignificant spaces, so `DO 10 I = 1.10` is an assignment, and COBOL adds a period whose scope depends on position and a `PICTURE` clause that is a language of its own. |
| **TeX, LaTeX** | `\catcode` reprograms the lexer from inside the document, which is property 2 with nothing left of it, and a macro takes arguments delimited by whatever it declared. **Text mode** rewrites a LaTeX document in place, `\emph{…}` to `\textit{…}`, and `@bracket "{" "}"` reads a brace nested inside the argument whole. |
| **Regular expressions, EBNF** | Both put sequence by juxtaposition, `ab` and `a b`, at the centre of the notation, which is property 3. The cleanest statement of the property is that this tool cannot read the EBNF its own reference is written in, and reads its own token patterns only by handing them to `regcomp`. |

---

## As an output

Property 7: any language can be written, and the examples deliberately write
several. What the suite has compiled, assembled or run: **C**, in every test that
compiles what it produced, **arm64 assembly** on a CPU, and **Python**'s own
source through `python3` beside the C made from it. What the examples write and the recorded `.out`
checks: **JavaScript**, **HTML**, **Pascal**, and Proto's **Solveig**.

What decides an output is what the source carries, and the rows above say
where a source stops carrying it. Three shapes of output are worth naming
because each needed a mechanic:

- **An output with a head that its body determines**, C's declarations, an
  include only if something prints: collections, `contribute` and `splice`.
- **An output that is not shaped like its input**, a sequence with labels
  where the input was a tree: fresh names, `{~L}`, and `@template`. Stage 2.
- **An output whose blocks are indentation**, Python or YAML from a
  brace language: `indent(s, n)` in a code template, and only there.
  A string template cannot indent what it splices.

---

## And the jobs, not the languages

The lists above are by language, and the question is often by job. The same
seven properties, read that way:

| job | | why |
| --- | --- | --- |
| **inventing a notation** and running it the same minute | fit | the tool's whole reason; the loop is one command |
| **a small classic language to C**, compiled and run | fit | stages 1, 3 and 4 |
| **a markup to HTML** | fit | text mode, `examples/poem.mx` |
| **a code generator to a machine** | fit | stage 2 |
| **a calculator, an evaluator for a formula language** | fit | `examples/calc.mx`; not an interpreter, and [direction.md](direction.md) says what the difference costs |
| **one grammar, two targets** | fit | `mx -b`, `examples/backends.mx` |
| **a rewrite over a real file** | fit, shaped like `lib/island.mx` | property 6 |
| **a front end for an industrial language** | unfit | property 1; [direction.md](direction.md)'s *what it should not become* |
| **a codemod over a tree** | unfit, by decision | the grammar belongs to the file, not the tool; [prior-art.md](prior-art.md) § 5 |
| **a formatter** | unfit | text mode gives the file back and reads its brackets, but has no columns, and expression mode refuses what it has no rule for |
| **a syntax highlighter** | unfit | the lexer cannot run without the header (notation.md, *What it costs*), and a rule cannot fire on a bare token |
| **a linter** | unfit | one error per run, no source maps, [ROADMAP.md](ROADMAP.md) 5 |

---

## Run, not reasoned

*The files that decided the verdicts above, kept beside this page in
`docs/languages/`, each a header and a few lines of body. Every transcript
here is run by `tests/docs.sh` against what it claims to print, so a change to
the lexer or to groups that moves a verdict turns this page red rather than
stale. [POSTMORTEM.md](POSTMORTEM.md) 25 is why: two verdicts on the first
draft were reasoned from the reference and both were wrong, and the six-line
files that refuted them had been thrown away. Grouped by the property each
one runs, then the fit rows that name a file.*

Property 2, **case**. The header spells `begin` and the body writes `BEGIN`, which is a `name` token and not that word, so the statement it opens has no rule:

`docs/languages/case.mx`:

```
; case.mx -- property 2: matching is case-sensitive. The header spells
; `begin`, and the body's `BEGIN` is not that word.

@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@separator ";" => ";\n"
@syntax "begin" b:stmts "end"  => "{{ {b}; }}"
@syntax "print" x              => "print({x})"
@end
begin print 1 end;
BEGIN print 2 END
```

```
$ mx docs/languages/case.mx
mx: docs/languages/case.mx:11: no rule reads 'print' here
```

Property 2, **a block comment does not nest**. The first `*/` closes it, and `c */ 1` is read as the body:

`docs/languages/nest.mx`:

```
; nest.mx -- property 2: a block comment does not nest. The first `*/`
; closes it, and what follows is read as the body.

@token number "[0-9]+"
@comment "/*" "*/"
@syntax "print" x  => "print({x})"
@end
print /* a /* b */ c */ 1
```

```
$ mx docs/languages/nest.mx
mx: docs/languages/nest.mx:8: nothing here is anything this file declared: 'c */ 1
'
```

Property 2, **a class that can match the rest of a line swallows the keyword** in front of it. `RUN apt-get update` is one `shell` token, longer than the word `RUN`, so the rule never fires and the token stands for itself. This is the Dockerfile, and it is `make`'s recipe line:

`docs/languages/tail.mx`:

```
; tail.mx -- property 2: a class that can match the rest of a line is the
; longest match and swallows the keyword in front of it. The rule never
; fires, and the token stands for itself.

@token shell "[^\n]+"
@separator "\n"
@syntax "RUN" s:shell  => "sh -c '{s}'"
@end
RUN apt-get update
RUN make check
```

```
$ mx docs/languages/tail.mx
RUN apt-get update
RUN make check
```

Property 2, **a token that exists only by position**. XML character data stands between a `>` and a `<`, and a class declared for it matches after `<` as well. In a bare tag the two classes tie on the tag name and the earlier declaration wins, so `<p>` alone reads by luck; the first attribute makes the character class the longer match, and the tag name is gone:

`docs/languages/xml-text.mx`:

```
; xml-text.mx -- property 2: character data is a token only by where it
; stands. A class declared for it matches after `<` as well: in a bare tag
; it ties with the tag name and the earlier declaration wins, so `<p>` alone
; reads by luck, and the first attribute makes it the longer match.

@token name   "[A-Za-z_][A-Za-z0-9_-]*"
@token string "\"[^\"]*\""
@token chars  "[^<>]+"
@syntax "<" n:name [ a:name "=" v:string ]* ">" c:chars "</" m:name ">"
    => "{n}({c})"
@end
<p class="lead">some words</p>
```

```
$ mx docs/languages/xml-text.mx
mx: docs/languages/xml-text.mx:12: no rule reads 'p class="lead"' here
```

Property 3, **a list separated by whitespace is not read**. The one rule Lisp needs is refused at the header, because a turn that is an expression has to be told from the next by a word, and whitespace is skipped before a word is looked for:

`docs/languages/lisp.mx`:

```
; lisp.mx -- property 3: a repeated group whose turns are expressions needs
; a `sep` word, and whitespace is never one. Refused at the header.

@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@syntax "(" f:name [ x ]* ")"  => "{f}({x})"
@end
(f 1 (g 2) 3)
```

```
$ mx docs/languages/lisp.mx
mx: docs/languages/lisp.mx:6: a repeated group that ends in a greedy hole and begins with a hole needs a 'sep' to know where one turn stops
```

Property 3, the other side. A turn that is **one class token** needs no `sep`, since it ends where the token does. A list of atoms reads; a nested list is the file above:

`docs/languages/atoms.mx`:

```
; atoms.mx -- property 3, the other side: a list of class tokens needs no
; `sep`, since each turn is exactly one token. Atoms read; a nested list
; would not, see lisp.mx.

@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@syntax "(" f:name [ x:name ]* join ", " ")"  => "{f}({x})"
@end
(list a b c)
```

```
$ mx docs/languages/atoms.mx
list(a, b, c)
```

Property 4, before the wall. A fixed-shape format under `@separator "
"`, a statement to a line:

`docs/languages/toml.mx`:

```
; toml.mx -- a fixed-shape format under `@separator "\n"`: a statement on
; its line reads whole. Compare toml-wrapped.mx.

@token number "[0-9]+"
@token string "\"[^\"]*\""
@token name   "[A-Za-z_][A-Za-z0-9_-]*"
@separator "\n"
@syntax "[" t:name "]"                        => "section {t}"
@syntax k:name "=" v   10                     => "  {k}: {v}"
@syntax "[" [ x ]* sep "," join ", " "]"      => "list({x})"
@end
[server]
port = 8080
hosts = ["a", "b"]
```

```
$ mx docs/languages/toml.mx
section server
  port: 8080
  hosts: list("a", "b")
```

Property 4, **the wrapped line**. The same header and the array broken after its comma. The newline is a separator, no bracket suspends it, and the file is refused rather than misread, which is [ROADMAP.md](ROADMAP.md) 2:

`docs/languages/toml-wrapped.mx`:

```
; toml-wrapped.mx -- property 4: the same header as toml.mx, and the array
; wrapped onto a second line. The newline is a separator and no bracket
; suspends it, so the file is refused rather than misread. ROADMAP 2.

@token number "[0-9]+"
@token string "\"[^\"]*\""
@token name   "[A-Za-z_][A-Za-z0-9_-]*"
@separator "\n"
@syntax "[" t:name "]"                        => "section {t}"
@syntax k:name "=" v   10                     => "  {k}: {v}"
@syntax "[" [ x ]* sep "," join ", " "]"      => "list({x})"
@end
[server]
hosts = ["a",
         "b"]
```

```
$ mx docs/languages/toml-wrapped.mx
mx: docs/languages/toml-wrapped.mx:15: no rule reads '
' here
```

**SQL.** Keyword-led mixfix with a comma list and two optional clauses. A clause that was not there splices as nothing, which is what an optional group is:

`docs/languages/sql.mx`:

```
; sql.mx -- keyword-led mixfix with a comma list and optional clauses, which
; is the pattern language's native shape.

@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@separator ";" => ";\n"
@syntax "select" [ c ]+ sep "," join ", " "from" t:name [ "where" w ] [ "order" "by" o ]
    => "scan({t}) | filter({w}) | sort({o}) | project({c})"
@syntax a "=" b   40   => "{a} == {b}"
@syntax a ">" b   40   => "{a} > {b}"
@end
select id, total from orders where total > 100 order by id;
select name from users
```

```
$ mx docs/languages/sql.mx
scan(orders) | filter(total > 100) | sort(id) | project(id, total);
scan(users) | filter() | sort() | project(name)
```

**JSON.** Two rules and three words, and no separator, so the newlines are whitespace and the body is one expression. The object's keys and values are two lists in one group, which a code template pairs and a string template cannot:

`docs/languages/json.mx`:

```
; json.mx -- two rules read it, and the body is one expression, so no
; separator is declared and a newline is whitespace. The object's keys and
; values are two lists, paired by the code template.

@token number "-?[0-9]+(\\.[0-9]+)?"
@token string "\"[^\"]*\""
@syntax "{" [ k:string ":" v ]* sep "," "}"
    => { emit "map("; for i, x in k sep ", " { emit x + " = " + at(v, i) }; emit ")" }
@syntax "[" [ v ]* sep "," join ", " "]"   => "seq({v})"
@syntax "true"   => "T"
@syntax "false"  => "F"
@syntax "null"   => "nil"
@end
{"name": "mx",
 "tags": ["c", "make"],
 "ok": true, "size": 1.5, "next": null}
```

```
$ mx docs/languages/json.mx
map("name" = "mx", "tags" = seq("c", "make"), "ok" = T, "size" = 1.5, "next" = nil)
```

**CSS.** A rule with its declarations and a class selector. The wall is not in this file: `url(a.png)` is a token that exists only after `url(`, and no class can say so:

`docs/languages/css.mx`:

```
; css.mx -- a rule, its declarations and a selector read. `url(a.png)` is
; the wall: that token exists only after `url(`, and no class here can say
; so. Two holes in one group are two lists, paired by the code template.

@token number "[0-9]+(px|em|%)?"
@token name   "[A-Za-z_-][A-Za-z0-9_-]*"
@token hex    "#[0-9a-fA-F]+"
@syntax s:name "{" [ p:name ":" v ]* sep ";" "}"   50
    => { emit "rule(" + s; for i, x in p { emit "; " + x + " = " + at(v, i) }; emit ")" }
@syntax "." c:name   => "class({c})"
@end
h1 { color: #333; margin: 0 }
```

```
$ mx docs/languages/css.mx
rule(h1; color = #333; margin = 0)
```

**XML, element-only.** A tag with attributes, children, a self-closing tag and a close all read, with the children as a `stmts` hole under `@separator "
"`. What does not read is the text between the tags, `xml-text.mx` above:

`docs/languages/xml.mx`:

```
; xml.mx -- an element-only document reads: a tag, attributes, children, a
; close. Compare xml-text.mx.

@token name   "[A-Za-z_][A-Za-z0-9_-]*"
@token string "\"[^\"]*\""
@separator "\n"
@syntax "<" n:name [ a:name "=" v:string ]* join " " ">" c:stmts "</" m:name ">"
    => "{n}[{a}={v}]({c})"
@syntax "<" n:name [ a:name "=" v:string ]* join " " "/>"
    => "{n}[{a}={v}]"
@end
<doc id="1">
  <item kind="a"/>
  <item kind="b"/>
</doc>
```

```
$ mx docs/languages/xml.mx
doc[id="1"](item[kind="a"]
item[kind="b"])
```

**Lua.** No separator is declared in the language, and property 4 forgives it: a statement that ends in `)` or `end` ends in a word, after which no separator is wanted, and one that ends in a name has a newline after it, which is the separator here:

`docs/languages/lua.mx`:

```
; lua.mx -- no separator is declared in the language, and property 4
; forgives it: a statement that ends in `)` or `end` ends in a word, and one
; that ends in a name has a newline after it, which is the separator here.

@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@separator "\n" => ";\n"
@syntax "local" v:name "=" e                    => "int {v} = {e}"
@syntax v:name "=" e   10                       => "{v} = {e}"
@syntax a "+" b   60                            => "{a} + {b}"
@syntax a "<" b   40                            => "{a} < {b}"
@syntax "if" c "then" b:stmts "end"             => "if ({c}) {{ {b}; }}"
@syntax "while" c "do" b:stmts "end"            => "while ({c}) {{ {b}; }}"
@syntax "print" "(" x ")"                       => "printf(\"%d\\n\", {x})"
@end
local x = 1
while x < 3 do
  x = x + 1
  if x < 3 then print(x) end
end
print(x)
```

```
$ mx docs/languages/lua.mx
int x = 1;
while (x < 3) { x = x + 1;
if (x < 3) { printf("%d\n", x); }; };
printf("%d\n", x)
```
