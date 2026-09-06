# Languages

*What this tool can be pointed at and what it cannot, language by language,
for a reader who has one in hand and wants to know before writing a header.
Two lists, fit and unfit, and in front of them the seven properties of the
tool that every verdict rests on, so that a language not listed can be scored
the same way. This page is a survey in the sense [prior-art.md](prior-art.md)
is, and like that page it outranks nothing: [REFERENCE.md](REFERENCE.md)
states the properties, [direction.md](direction.md) argues for why the walls
stay where they are, and where this page and the code disagree the code is
right. Written 2026-09-06. Every property below was run against the tool that
day, and every verdict names the property it rests on.*

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
   came before changes what a character means. Four things follow, each run
   on the day this was written:
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
     tag name or the words of a paragraph.

3. **A list is separated by a word, or it is not read.** A repeated group
   whose turns are expressions needs a `sep` word to know where one turn
   stops, and whitespace is never a word. A rule that begins with a hole
   must have a word second, so a led rule cannot be two holes. Between them
   this rules out **juxtaposition**: `f x y` as application, `(f x (g y))`
   as a call, `a b c` as a command line, `ab` as concatenation. It is the
   property that surprised this page, and it puts the smallest grammar on
   it in the unfit list.

4. **A newline is whitespace or a separator, and no bracket suspends it.**
   [ROADMAP.md](ROADMAP.md) 2. A language whose statements end at a line
   break reads under `@separator "\n"`, and then a call wrapped onto a
   second line does not, because the lexer cannot know what a bracket is.
   A language that ends statements with a word does not care.

5. **A block is braces, a pair of words, or one shape of indentation.**
   `@separator … indent` and the `block` kind, REFERENCE §4.3, are Python's
   shape: a deeper line opens a block and a shallower one closes it. A
   layout rule that opens a block at the column of the next token, or a
   language where the tab is not eight columns, is not that shape.

6. **Text mode is a scan, and a declared class is the only token it has.**
   REFERENCE §7 and [ROADMAP.md](ROADMAP.md) 7. A rule fires where its words
   match and everything else passes through, which is the island grammar
   every tool that works on real code has. Of the three things those tools
   know it has two, since 2026-09-06: where a `@token` class matches, the
   scan moves by the token, so a rename over `err` leaves `stderr` alone and
   a string or a comment declared as a class is passed over whole. What it
   lacks is the third: a hole over `f(x, g(y))` stops at the first `)`. So
   text mode is for markup, for a rewrite shaped like `lib/island.mx`, and
   for a rename; a formatter would want the brackets and the columns it does
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
[ROADMAP.md](ROADMAP.md) 2 and 7 would move a verdict or two below, and
nothing else on the roadmap would.

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
| **Lua** | a subset, near whole | The smallest grammar of the general-purpose languages people ship code in: `end`-delimited blocks, `do`, `then`, `function`, a dozen operators, and no statement separator, which property 4 forgives because a statement that ends in `)` or `end` ends in a word and one that ends in a name has a newline after it. What stops it is a long string with a level, `[==[ … ]==]`, which POSIX ERE cannot count (property 2), and the ambiguity Lua's own manual records, a line beginning with `(` after one that ended in a name. |
| **SQL** | a subset, one per dialect | Keyword-led mixfix with comma lists is the pattern language's native shape: `"select" [ c ]+ sep "," "from" t:name [ "where" w ] [ "order" "by" o ]` reads a query, and each optional clause is an optional group. Run on the day this was written. Property 2 is the cost: `SELECT` and `select` are two words. The dialects are the other cost, and are not the tool's. |
| **Prolog** | a subset | An operator-precedence syntax is a Pratt grammar, and `op/3` is `@syntax` with a level: [prior-art.md](prior-art.md) § 2 counts Prolog among the ancestors for that reason. Terms, lists, clauses and `:-` all read. What stops it is a program that declares an operator in its own body with `:- op(700, xfx, ===)` and uses it on the next line: this tool reads the header first and the body second, and a directive in the body is a term (property 2). |
| **Assembly** | whole | Line-oriented, mnemonic-led, comma-separated operands, labels by a token class: nothing in it touches a property. Stage 2 *writes* it, `examples/asm.mx`, and `tests/asm.sh` runs the result on a CPU. Reading one stops only at a macro assembler's macros, which change what later lines mean (property 1). |
| **JSON** | whole | Two rules: an object is `"{" [ k:string ":" v ]* sep "," "}"` and an array is `"[" [ v ]* sep "," "]"`. The body is a single expression, so no separator is declared and property 4 never arises. It is the smallest useful input, and a `.mx` that reads JSON and writes a C initialiser, a table, or another config format is a page long. |
| **Fixed-shape formats**: CSV, INI, TOML, Graphviz DOT, Protocol Buffers and GraphQL schemas | whole, when a statement stays on its line | Brackets, keywords and comma- or newline-separated lists, which is what the pattern language was made from. Each declares `@separator "\n"`, and property 4 then says what stops it: a TOML array or a DOT attribute list that wraps onto the next line is [ROADMAP.md](ROADMAP.md) 2. That is how the files are usually written, and a file that wraps is refused rather than misread. |
| **CSS** | a subset | Rules, declarations and the common selectors read: `s "{" [ p:name ":" v ]* sep ";" "}"`. What stops it is property 2: an unquoted `url(a.png)` is a token that exists only after `url(`, and `calc()` has arithmetic the rest of the value language does not. |
| **Markdown, wiki markup, lightweight markup** | whole, for what is inline and per line | Text mode's home, and `examples/poem.mx` is the demonstration: emphasis, links, headings, rules, lists, with the hole's text expanded in its turn so that constructs nest. What stops it is property 6: a construct defined by its column, an indented code block or a nested list, since text mode has no columns to measure. |

**And two rows for reading a real file rather than one written for a
grammar**, since that is the question a reader with a codebase is asking:

| language | reads | why |
| --- | --- | --- |
| **C, as an island** | a rewrite, with the hole kept last, and a rename | Stage 5. `lib/island.mx` rewrites `metaxis/cmd/mx.c`, the tool's own front end, and `tests/island.sh` compiles and runs the result. What holds it to that shape is property 6: a template that reused the hole wrote `log(f(x, g(y); complain(f(x, g(y)))`, and `examples/island.mx` records the accident on purpose. The rename in the same file, `out` to `res`, leaves `outpath`, the usage string and the first comment alone, because the file declares C's identifiers, strings, character literals and comments as classes. |
| **Any language, in text mode** | a rewrite of the same shape | The rule is the same for all of them, which is why Comby works on thirty languages: declare the language's identifiers, strings and comments as classes, and write a rewrite whose hole does not cross a bracket and whose output keeps the hole last. The bracket is what [ROADMAP.md](ROADMAP.md) 7 still holds, and is what would widen it. |

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
| **XML, HTML** | An element-only document reads, `"<" n:name [ a:name "=" v:string ]* ">" c:stmts "</" m:name ">"`, run on the day this was written. Text between the tags does not: character data is a token only by where it stands, after a `>` and before a `<`, and a class declared for it is the longest match after `<` as well, where it swallows the tag name (property 2). Content that mixes text and elements with nothing between them is property 3 besides. HTML adds optional end tags, which need the tag table to know what a `</div>` closes (property 1). **Text mode** rewrites either in place, which is how they are usually touched. |
| **Haskell, OCaml, Standard ML, F#** | Property 3: application is juxtaposition, `f x y`, and there is no rule shape for it. Then property 5 for Haskell's layout rule, which opens a block at the column of the token after `where` or `do`; property 2 for `{- -}` and `(* *)`, which nest; and fixity declared in the body, as in Prolog. The expression core with every call parenthesised would read, and nobody writes it that way. |
| **Lisp, Scheme, Clojure** | The surprise, and the smallest grammar on the page. `(f x (g y) z)` is an argument list separated by nothing, and property 3 says a repeated group whose turns are expressions needs a `sep` word: `"(" f:name [ x ]* ")"` is refused at the header, and a `sep` of a space is a word the lexer never produces, since whitespace is skipped before a word is looked for. A list of atoms only, `[ x:name ]*`, reads; a nested one does not. Run on the day this was written. |
| **Forth, and the concatenative languages** | Property 3, words separated by whitespace, and property 1 in its strongest form: a Forth program redefines its own reader as it runs. |
| **Shell**: sh, bash, zsh | A command line is words separated by whitespace (property 3), and what a word means is decided by quoting, by `$( … )`, by a heredoc's chosen terminator, and by position: `if` is a keyword only where a command may begin (property 2). Text mode edits a script; nothing reads one. |
| **Make, Dockerfile** | A line whose tail is another language, property 2: a recipe line is shell, and a `RUN` line is shell, and a class that could match the tail swallows the keyword. Make adds a tab that means *recipe* and a space that does not, where the indent stack counts a tab as eight spaces (property 5). Text mode edits either. |
| **YAML** | The indentation reads, since it is Python's shape. What stops it is a block scalar, `key: \|` followed by indented lines that are *text*, which is a lexer state beyond the indent stack (property 2); an anchor and its alias, `&a` and `*a`, which is a lookup (property 1); and flow style wrapped across lines (property 4). The subset of maps, lists and one-line scalars reads, and most configuration files are outside it somewhere. |
| **Fortran** (fixed form), **COBOL** | Column-significant: a label in columns 1 to 5, a continuation mark in 6, code from 7 to 72. The lexer counts columns for indentation and for nothing else (property 2). Fortran adds insignificant spaces, so `DO 10 I = 1.10` is an assignment, and COBOL adds a period whose scope depends on position and a `PICTURE` clause that is a language of its own. |
| **TeX, LaTeX** | `\catcode` reprograms the lexer from inside the document, which is property 2 with nothing left of it, and a macro takes arguments delimited by whatever it declared. **Text mode** rewrites a LaTeX document in place, `\emph{…}` to `\textit{…}`, and a brace nested inside the argument is the one gap left in [ROADMAP.md](ROADMAP.md) 7. |
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
| **a formatter** | unfit | text mode gives the file back but has no brackets and no columns, and expression mode refuses what it has no rule for |
| **a syntax highlighter** | unfit | the lexer cannot run without the header (notation.md, *What it costs*), and a rule cannot fire on a bare token |
| **a linter** | unfit | one error per run, no source maps, [ROADMAP.md](ROADMAP.md) 5 |
