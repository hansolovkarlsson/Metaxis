# Reference

*What every part of a `.pt` file means, exhaustively and in one place. This
file states behaviour and does not argue for it — the reasons and the costs are
[notation.md](notation.md)'s job and what is not built is
[ROADMAP.md](ROADMAP.md)'s, and where any of the three disagrees with the code
the code is right and the page is wrong.*

Everything here is checked by `make check`. Section headings are stable; the
examples in them are lifted from `examples/`.

---

## 1 · A first file

```
@token  name   "[A-Za-z_][A-Za-z0-9_]*"
@token  number "[0-9]+"
@comment ";" eol
@separator ";" => ";\n"

@syntax a "+" b   60   => "add({a}, {b})"
@syntax "twice" e      => "({e} * 2)"
@end
x = 1;
twice x + 2;
```

```
$ pt first.pt
x = 1;
(add(x, 2) * 2)
```

A `.pt` file is a **header** of directives and a **body** of foreign text. The
header says what the body's syntax is; nothing is built in. The body is read
with that syntax and rewritten into whatever the templates say.

The one rule everything below follows: **anything a directive says about the
body's text, or about the output's, is inside a string.** Outside the strings a
directive is written in Prototype's own fixed vocabulary, which no file can
change, which is why a directive can never be read as the thing it declares.

---

## 2 · The shape of a file

### 2.1 The header

A directive begins with `@` at the start of a line and **ends at the end of that
line**, continuing onto any following line that begins with a space or a tab:

```
@syntax "for" "(" init ";" c ";" step ")" "{" b:stmts "}"
    => "{init}.\n{{ {c} }}:whileTrue({{ {b}. {step} }})"
```

Blank lines between directives are ignored.

### 2.2 Where the header ends

At **`@end`**, or at the first line that does not begin a directive — whichever
comes first. It is one-way: below the boundary nothing is a directive, so a body
line may begin with `@` and mean it. `@end` exists for the file that declared
`@` as an operator and wants to open its body with one.

The body begins at the start of the line after `@end`, or at the first line that
was not a directive.

### 2.3 Comments in the header

`;` to the end of the line is Prototype's own and **always works**, in every
file, whatever the file declares. A `@comment` opener **joins** it from the line
after the declaration rather than replacing it, so this is legal and does what
it looks like:

```
@comment "//" eol
// From here down the header may use C's comments too.
; and this still works
```

`;` also ends a **directive's own text**, so a note may follow one on its line:

```
@syntax a "@" b  50  => "{a}:at({b})"    ; the directive sigil
```

It can afford to, because everything a directive says about foreign text is
inside a string and this rule applies outside the strings.

### 2.4 Prototype strings

`"…"`, with exactly five escapes: `\"` `\\` `\n` `\t` `\r`. Any other backslash
is an error (`unknown escape`); a newline inside one is an error
(`unterminated string`). **This never varies** — not with `@token string`, not
in a `@use`d file, not anywhere. It is the boundary the whole notation rests on.

---

## 3 · Directives

```ebnf
directive   = "@use"       string
            | "@mode"      ( "expression" | "text" )
            | "@token"     name string
            | "@comment"   string ( string | "eol" )
            | "@separator" string [ "=>" string ]
            | "@syntax"    pattern [ level ] "=>" template
            | "@end" .
```

### 3.1 `@token name "regex"`

Declares a lexical class. The regex is a **POSIX extended** regular expression,
matched anchored at the current position, and POSIX's leftmost-longest rule
applies — so `"0x[0-9a-fA-F]+|[0-9]+"` takes all of `0x0c` and not just the `0`.

```
@token number "0x[0-9a-fA-F]+|[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@token string "'([^']|'')*'"
```

- Matching is case sensitive.
- `.` matches a newline, so a class **may span lines**. That is how a multi-line
  string literal would be declared, and how one is declared by accident.
- Redeclaring a name replaces the pattern. Rules already written against the
  class keep working.
- A class name may be used as a hole's kind (§4.3).
- A bad pattern is `bad pattern for 'name': …` with the regex library's own
  words after it.

### 3.2 `@comment "open" eol` · `@comment "open" "close"`

Adds a comment form. `eol` runs to the end of the line; otherwise the second
string closes it. Several may be declared; each is added, never replacing.

```
@comment "//" eol
@comment "/*" "*/"
@comment "{" "}"
```

An unclosed block comment runs to the end of the file and is not an error.

**Comments are looked for before words** (§6.1), so a file whose comment opener
is also an operator gets the comment.

### 3.3 `@separator "in"` · `@separator "in" => "out"`

Declares what separates statements in the body, and what to join them with in
the output. Without the second half the input text is reused.

```
@separator ";" => ".\n"
@separator "\n"
```

A separator that contains a newline makes **newline a token** instead of
whitespace: a run of blank lines is one separator, and no separator is produced
before the first token or after the last.

Without `@separator` the body is a single expression and a `stmts` hole takes
one expression.

### 3.4 `@syntax pattern [level] => "template"`

The only rule-making directive. §4, §5 and §6 are all about it.

```
@syntax a "+" b            60           => "{a}:add({b})"
@syntax "if" c "then" t                 => "if ({c}) {t}"
@syntax "swap" "(" a "," b ")"
    => "{{ int {~t} = {a}; {a} = {b}; {b} = {~t}; }}"
```

### 3.5 `@use "path"`

Reads another file's directives into this header. The path is taken relative to
the **directory of the file that used it**, or used as-is if absolute. There is
no search path and no environment variable.

A used file holds **directives and nothing else**; a statement in one is
`a used file holds directives and nothing else`. Nesting is limited to 64.

What it brings is what it declared. `examples/use.pt` takes its arithmetic from
`lib/arith.pt` and declares its own `@comment` and `@separator`, because those
belong to the file being written and not to the arithmetic in it.

Two used files that declare the same word do not collide loudly: **the later
declaration wins, silently.**

### 3.6 `@mode expression` · `@mode text`

`expression` is the default and parses the whole body with the declared grammar,
calling anything unmatched an error. `text` scans the body, fires a rule where
one matches, and copies everything else through. §7.

### 3.7 `@end`

Ends the header. §2.2.

---

## 4 · Patterns

```ebnf
pattern     = element { element } .
element     = string | hole | group .
hole        = name [ ":" kind ] .
kind        = "expr" | "stmts" | "text" | a class named by @token .
group       = "[" element { element } "]" [ rep ] .
rep         = ( "*" | "+" ) [ "sep" string ] [ "join" string ] .
```

**A quoted string is a literal word of the input. A bare name is a hole.** That
distinction is what the whole notation is for, and it is why there are no
angle brackets and no separate `@infix`.

### 4.1 What shape a pattern is

Read off the elements, not declared:

| pattern | shape |
| --- | --- |
| `a "+" b` | infix |
| `"-" a` | prefix |
| `a "++"` | postfix |
| `"(" e ")"` | circumfix |
| `"~"` | a word alone |
| `"if" c "then" t "else" f` | mixfix |

A pattern that **begins with a hole** is a *led* rule: it continues an
expression already parsed, and needs a level. One that **begins with a word** is
a *nud* rule: it starts one, and does not.

### 4.2 What a pattern may not be

| refused | message |
| --- | --- |
| empty | `a rule needs a pattern` |
| an empty word `""` | `an empty word matches nothing` |
| begins with a group | `a rule is found by its first word, so it cannot begin with a group` |
| begins with a hole, no level | `…is infix or postfix and needs a level` |
| begins with a hole, second element not a word | `…must have a word after it` |
| a greedy hole immediately before another hole | `two holes in a row: the first would take everything the second wants` |
| a `stmts` hole with no word after it | `a 'stmts' hole needs a word after it to stop at` |
| an empty group | `a group needs something in it` |
| a repeated group that could not tell one turn from the next | `…needs a 'sep' to know where one turn stops` |

Two holes may not be adjacent when the first is **greedy** — `expr` or `stmts`,
the kinds that read up to a word — because it would take everything the second
wants: given `a b` and input `f x + y`, the first hole takes the sum and the
second finds nothing. A class-kind hole takes exactly one token and cannot be
greedy, so `"f" a:name b:name` is allowed. The check looks through a group's
brackets: a group's last element is adjacent to whatever follows the group.

Where a body is wanted, a rule takes its own delimiters —
`"if" "(" c ")" "{" t:stmts "}"` — rather than needing a kind that means *a
block*.

### 4.3 Kinds

| kind | takes |
| --- | --- |
| `expr` | one expression, at a binding power set by §5. **The default.** |
| `stmts` | statements separated by the declared separator, up to the pattern's next word. Expanded, and joined with the separator's output form. |
| `text` | raw source text up to the pattern's next word. Text mode only (§7); in expression mode it is `a 'text' hole belongs to @mode text`. |
| *a class name* | exactly one token of that class, spliced as its source text. |

A class-kind hole is how a hole says *stop here*:

```
@syntax "for" i:name ":=" a "to" b "do" s
```

`i:name` takes one token and stops, so the `:=` that follows is this pattern's
word. An `expr` hole would have taken `i := 1` with the infix assignment rule and
the pattern would never match. **This is the one thing quoting does not settle
by itself.**

### 4.4 Groups

`[ … ]` is Prototype's own bracket. It lives outside the strings, so it can
never be confused with a bracket the body writes — one of those is quoted, and
this one cannot be.

| form | matches |
| --- | --- |
| `[ … ]` | the elements inside, once or not at all |
| `[ … ]*` | zero or more turns |
| `[ … ]+` | one or more turns |

A repeated group may say how its turns are told apart and how they are put back
together:

| suffix | means |
| --- | --- |
| `sep "s"` | on the way in, one turn is separated from the next by the word `s` |
| `join "j"` | on the way out, a hole's turns are spliced with `j` between them |

`join` defaults to `sep`'s text; `sep` defaults to nothing, in which case turns
are matched by juxtaposition. Both are refused on a group with no `*` or `+`.
`sep` and `join` are keywords only where a string follows, so a hole may still
be named either.

```
@syntax a "(" [ x ]* sep "," join ", " ")"  95   => "{a}({x})"
@syntax "let" [ n:name ]+ sep "," join ", "      => "let {n}"
@syntax "loop" n "times" "{" b:stmts "}" [ "or" "{" e:stmts "}" ]
    => "for (…) {{ {b} }}\nif (!{n}) {{ {e} }}"
```

**Every hole a pattern declares is bound, groups included.** A hole inside a
repeated group holds every turn, spliced with the group's `join`; a hole inside
an optional group that did not match holds the empty string. Nothing is ever
unbound, so a template never has to ask whether a part was there — and *cannot*
ask, which is [ROADMAP.md](ROADMAP.md)'s first item.

**A group is matched at binding power 0** and is delimited by its own words. A
turn that consumes no tokens ends the repetition. A failed optional group, or a
failed turn, restores both the token cursor and every binding made inside it.

Groups may nest, 16 deep. They are refused in text mode (§7).

---

## 5 · Levels and binding

```ebnf
level = integer [ "left" | "right" ] .
```

Higher binds tighter. `left` is the default and may be written for symmetry.
The level may be placed anywhere among the elements; **write it after the
pattern**, which is what every example does.

A **led** rule applies only where its level is greater than the binding power
the parser is currently reading at. Its trailing hole, if it has one, is read at
its own level — or one below it when `right`, which is what makes
`2 ^ 3 ^ 2` group as `2 ^ (3 ^ 2)`.

A **nud** rule's level, if given, is the binding power of its trailing hole.
That is how `"!" a 80` takes only the operand and not the `&&` after it. A nud
rule without a level reads its trailing hole at 0, taking everything up to the
next word or the end of the statement.

A hole that is **not last** is read at 0. It is delimited by the word after it
rather than by precedence. So is every hole inside a group (§4.4), whether or
not the group is last.

A useful ladder, from `lib/arith.pt`:

```
@syntax a "=" b   10 right
@syntax a "+" b   60
@syntax a "*" b   70
@syntax a "^" b   80 right
@syntax "-" a     90
@syntax a ":" m:name 95
```

---

## 6 · How the body is read — expression mode

### 6.1 The lexer

At each position, in this order:

1. **Whitespace** is skipped. A newline is skipped too, unless the separator is
   a newline, in which case one separator token is produced for a run.
2. **Comments** are looked for, and win. This is the only precedence in the
   lexer that the file did not set.
3. **The longest match from every declared token class**, and **the longest
   match from every declared word**, are both taken, and:

> **the class wins a tie.**

That last line is the whole of *an alphabetic word is not reserved*. `div` is a
`name` token whose text happens to be `div`; a rule that wants the word compares
text, so it matches. `divisor` is a longer class match than the word, so it never
splits. Meanwhile `<=` is not a class match at all and is taken as the word,
longest first, so declaring `<=` does not stop `<` existing.

Nothing else is a token. A character that matches no class and begins no word is
`nothing here is anything this file declared: '…'`.

The word set is every word any rule quoted, plus the separator.

### 6.2 The parser

Pratt, with backtracking.

- **A nud rule** is tried when the current token's text is its first word.
- **A led rule** is tried when the current token's text is its second element's
  word and its level beats the current binding power.
- **Candidates under one leading word are tried longest pattern first**, with
  the token cursor restored after a failure.
- If no rule matches and the token is a class token, it stands for itself and
  its source text is the result.

Longest-first is the whole of the dangling else: `if c then t else f` is tried
before `if c then t`, so the inner `if` in `if a then if b then x else y` takes
the `else`.

Recursion is capped at 400 (`the grammar recurses without consuming anything`).

### 6.3 Statements

The body is a sequence of statements. So is a `stmts` hole, up to the word that
closes it.

- Leading and repeated separators are skipped, so an empty statement is nothing.
- **A separator is wanted between two statements, and not after one that ended
  in a word.** That is what lets `}` and `end` stand on their own, so C's
  `for (…) { … }` needs no `;` after it.
- The output is joined with the separator's output form **unconditionally**,
  including after a statement that ended in a word. That is why
  `examples/hygiene.out` carries a `};` at file scope.

If the body does not parse, the error names the furthest token reached:
`no rule reads 'x' here`, or `the file ends in the middle of something`.

---

## 7 · Text mode

`@mode text`. The body is scanned; where a rule matches it fires; everything
else is copied through unchanged.

- Only **nud** rules apply. A led rule has nothing to continue.
- **A group is refused**: `a group belongs to @mode expression`, reported before
  anything is scanned. [ROADMAP.md](ROADMAP.md) 2 says what it would take.
- **The longest leading word that matches wins.** Declaration order breaks a tie
  between two of the same length and decides nothing else — `examples/poem.pt`
  declares `-`, `--` and `---` in that order and `---` still wins.
- **Every hole is text**, whatever kind it was given, and stops at the pattern's
  next word — the first occurrence, not the last. A hole with no word after it
  takes the rest of the enclosing text.
- **A hole's text is expanded in its turn**, so `**a //slanted// claim**` nests.
  Depth is capped at 64 (`a text rule expands into itself`).
- If a rule's pattern does not complete, nothing is consumed and the next rule is
  tried; if none matches, one character is copied and the scan moves on.
- A comment is removed. **A comment that is alone on its line takes the line
  with it**, indentation and newline included; one that follows text on a line
  is removed only as far as the end of the line.
- `@separator` is not used.

---

## 8 · Templates

```ebnf
template = string, with "{" name "}" splices, "{~" name "}" fresh names,
           and "{{" "}}" for a literal brace .
```

| in a template | emits |
| --- | --- |
| `{name}` | the hole `name`, expanded — every turn of it if it is in a repeated group (§4.4), the empty string if its optional group did not match |
| `{~label}` | a name nobody else has (§8.1) |
| `{{` | `{` |
| `}}` | `}` |
| `}` alone | `}` |
| `{` alone | opens a splice — there is no way to emit a bare `{` except `{{` |

Every splice is checked **at the `@syntax` that wrote it**, not at the first use
of the rule:

- a `{name}` with no such hole is `the template splices '{name}' and the pattern
  has no such hole`;
- `{~}` is `a fresh name needs a label: '{~name}'`;
- `{~a}` beside a hole named `a` is refused — they would read as one thing.

### 8.1 Fresh names

`{~t}` is **a name nobody else has**. Within one expansion every `{~t}` is the
same name; the next use of the rule gets a different one.

```
@syntax "swap" "(" a "," b ")"
    => "{{ int {~t} = {a}; {a} = {b}; {b} = {~t}; }}"
```

Two calls of that rule produce `t__1` and `t__2`. A candidate is refused if it
occurs anywhere in the source being expanded or in any template any rule
declared, `@use` included; the test is a substring test, so it is conservative
in the safe direction — `t__1` is refused while `t__12` is in the file.

This closes the half of hygiene where a template **introduces** a name. It does
not touch the half where a template **reaches out** for one the caller shadowed:
there is nothing to invent there, and a template that is a string cannot see a
scope. `examples/hygiene.pt` demonstrates both and `tests/hygiene.sh` runs them.

---

## 9 · The command line

```
pt [-o output] [-g] file.pt
```

| | |
| --- | --- |
| *(no flag)* | the expansion, to standard output |
| `-o path` | the expansion, to `path` |
| `-g` | the grammar the header declared, then stop |

A trailing newline is added if the expansion does not end in one. Errors go to
standard error as `pt: file:line: …` and exit 1; a bad command line exits 2.

`-g` is the way to see what a header actually built:

```
$ pt -g examples/use.pt
mode       expression
separator  declared
token      number   [0-9]+
token      name     [A-Za-z_][A-Za-z0-9_]*
comment    # eol
words      '(' ')' '*' '+' '-' '/' ':' ';' '=' '^'
prefix    "(" e ")"
infix     a ":" m [95]
infix     a "^" b [80 right]
prefix    "-" a [90]
…
```

---

## 10 · Errors

Every message the tool can produce, and what it means.

### In the header

| message | means |
| --- | --- |
| `no directive called '@x'` | §3 lists them all |
| `expected a directive` | a line begins with `@` and no name follows |
| `expected a string` | a directive wanted `"…"` and got something else |
| `unterminated string` | a `"` with no closing `"` before the newline |
| `unknown escape` | a `\` in a string other than `\" \\ \n \t \r` |
| `expected 'expression' or 'text'` | `@mode` |
| `expected a class name` | `@token` |
| `bad pattern for 'x': …` | `@token`'s regex, in the regex library's words |
| `expected a quoted word, a hole, a group or a level` | a pattern element that is none of those |
| `expected ']'` | a group with no closing bracket |
| `a group needs something in it` | `[ ]` |
| `'sep' belongs to a repeated group — '[ … ]*' or '[ … ]+'` | §4.4; likewise `join` |
| `groups nested more than 16 deep` | §11 |
| `a rule is found by its first word, so it cannot begin with a group` | §4.2 |
| `a repeated group that ends in a greedy hole and begins with a hole needs a 'sep' to know where one turn stops` | §4.4 |
| `expected a kind after ':'` | a hole wrote `:` and stopped |
| `no kind or token class called 'x'` | §4.3, or a `@token` that has not been declared yet |
| `a rule needs a pattern` | `@syntax => "…"` |
| `an empty word matches nothing` | `""` as a pattern element |
| `expected '=>'` | a rule with no template |
| `trailing text after the template` | something after the closing `"` |
| `a rule that begins with a hole is infix or postfix and needs a level` | §4.2 |
| `a rule that begins with a hole must have a word after it` | §4.2 |
| `two holes in a row: …` | §4.2 |
| `a 'stmts' hole needs a word after it to stop at` | §4.3 |
| `unclosed '{' in a template` | §8 |
| `the template splices '{x}' and the pattern has no such hole` | §8 |
| `a fresh name needs a label: '{~name}'` | §8.1 |
| `'{~a}' and '{a}' would read as one thing: …` | §8.1 |
| `cannot open path` | `@use` |
| `a used file holds directives and nothing else` | §3.5 |
| `@use nested more than 64 deep` | §3.5 |

### In the body

| message | means |
| --- | --- |
| `nothing here is anything this file declared: '…'` | a character that matches no class and begins no word — §6.1 |
| `no rule reads 'x' here` | the parser stopped; `x` is the furthest token it reached |
| `the file ends in the middle of something` | as above, at end of file |
| `a 'text' hole belongs to @mode text` | §4.3 |
| `a group belongs to @mode expression` | §7 |
| `the grammar recurses without consuming anything` | 400 deep — §6.2 |
| `a text rule expands into itself` | 64 deep — §7 |
| `no fresh name for '{~t}' is free` | 100000 candidates were all taken — §8.1 |

---

## 11 · Limits

| | |
| --- | --- |
| `@use` nesting | 64 |
| group nesting | 16 |
| expression recursion | 400 |
| text-mode expansion depth | 64 |
| fresh-name attempts | 100000 |
| everything else | memory |

The tool allocates and never frees. It reads one file and exits.

---

## 12 · Differences from Proto

Both take a file that declares its own grammar. Where they part:

| | Proto | Prototype |
| --- | --- | --- |
| what a directive quotes | nothing — operators and pattern words are bare, holes are `<x>` | every mention of foreign text |
| rule directives | `@infix`, `@infixr`, `@prefix`, `@syntax` | `@syntax` |
| repetition and optional parts | declined three times, no customer | `[ … ]`, `[ … ]*`, `[ … ]+` — §4.4 |
| the shape of a rule | named by the directive | read off the pattern |
| operator characters | a closed set; `. , : ; |` are spoken for | whatever a string says |
| literals | the lexer's, fixed | `@token` |
| comments | `;`, fixed | `@comment` |
| statement separator | `.`, fixed | `@separator` |
| the template | an expression in the target language | a string |
| output precedence | Proto re-prints and parenthesises | the author's parentheses |
| hygiene | the expander renames | `{~t}` for half of it, §8.1 |
| source maps | a `.map` beside the output | none |
| the target | Solveig | anything |

`docs/notation.md` argues about which of those are gains and which are the
price. This page only says which are which.
