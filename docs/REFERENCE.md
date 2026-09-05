# Reference

*What every part of a `.pt` file means, exhaustively and in one place. This
file states behaviour and does not argue for it — the reasons and the costs are
[notation.md](notation.md)'s job and what is not built is
[ROADMAP.md](ROADMAP.md)'s, and where any of the three disagrees with the code
the code is right and the page is wrong.*

Section headings are stable. The examples deliberately read and write
**several different languages**, because a page about a language-agnostic tool
that only ever showed one would be arguing the opposite of what it says. The one
constraint on them is that **the output is always a language the reader already
has** — C, Pascal, JavaScript, HTML — so that reading an example costs nothing
but the notation it is there to explain.

§1 is a whole file and is `examples/first.pt`, run by `make check` against the
output printed beside it; the rest are fragments written for this page, and
where one names a file it is lifted from that file.

---

## 1 · A first file

`examples/first.pt`, whole:

```
@token  name   "[A-Za-z_][A-Za-z0-9_]*"
@token  number "[0-9]+"
@comment "#" eol
@separator ";" => ";\n"

@syntax a "=" b   10 right  => "{a} = {b}"
@syntax a "+" b   60        => "add({a}, {b})"
@syntax "twice" e           => "({e} * 2)"
@end
x = 1;              # nothing here is built in
twice x + 2;
```

```
$ pt examples/first.pt
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
    => "{init};\nwhile {c} do begin {b}; {step} end"
```

That reads C's `for` and writes Pascal's `while`.

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
@syntax a "@" b  50  => "{a}[{b}]"    ; the directive sigil
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
            | "@token"     name string [ "override" ]
            | "@comment"   string ( string | "eol" )
            | "@separator" string [ "=>" string ] [ "override" ]
            | "@syntax"    pattern [ level ] "=>" template { "terminated" | "override" }
            | "@template"  name "(" [ name { "," name } ] ")" code [ "override" ]
            | "@fragment"  name [ "override" ] "=" pattern
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
- **Redeclaring a name is refused** unless the second declaration says
  `override`, which replaces the pattern; rules already written against the
  class keep working. §3.10.
- A class name may be used as a hole's kind (§4.3).
- A bad pattern is `bad pattern for 'name': …` with the regex library's own
  words after it.
- **Nothing may follow but `override`.** Anything else is `trailing text after
  @token`, as it has always been after a template.

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
@separator ";" => ";\n"
@separator "\n"
```

A separator that contains a newline makes **newline a token** instead of
whitespace: a run of blank lines is one separator, and no separator is produced
before the first token or after the last.

**Declaring it twice is refused** unless the second says `override`. §3.10.
Nothing may follow but `override`; anything else is `trailing text after
@separator`.

Without `@separator` the body is a single expression and a `stmts` hole takes
one expression.

### 3.4 `@syntax pattern [level] => template [terminated] [override]`

The only rule-making directive. §4, §5 and §6 are about the pattern; §8 about
the template, of which there are two kinds.

```
@syntax a "+" b            60           => "({a} + {b})"
@syntax "if" c "then" t                 => "if ({c}) {t}"
@syntax "swap" "(" a "," b ")"
    => "{{ int {~t} = {a}; {a} = {b}; {b} = {~t}; }}"

@syntax a "*" b            70           => { emit group(a, 70) + " * " + group(b, 71) }
@syntax "fn" f:name "(" [ p:name ]* sep "," ")"
    => {
        emit "int " + f + "("
        if count(p) == 0 { emit "void" }
        for x in p sep ", " { emit "int " + x }
        emit ")"
    }
```

A directive ends at a newline, so a code template spanning lines works the way
anything else spanning lines does — except that a directive never ends while a
brace is open, which is what lets the closing `}` sit at the left margin.

**`terminated`** says the rule's output already ends a statement, so no
separator is joined after it (§6.3). A code template can also **read** it off a
hole — `terminated(h)` in §8.3 — which is how a rule decides whether what filled
a hole needs punctuating. **`override`** says the rule means to displace an
earlier one with the same pattern (§3.10). Either, both, in either order.

Both go **after the template**, which is the one place in a rule where a bare
word cannot be anything else — a hole only appears in the pattern — so they
reserve nothing and a hole may still be called `terminated` or `override`.
`examples/reserved.pt` has a rule that is both.

### 3.5 `@use "path"`

Reads another file's directives into this header. The path is taken relative to
the **directory of the file that used it**, or used as-is if absolute. There is
no search path and no environment variable.

A used file holds **directives and nothing else**; a statement in one is
`a used file holds directives and nothing else`. Nesting is limited to 64.

**A file is read once**, however many times it is reached. So a diamond — two
files that both use a third, and one file that uses both — costs nothing and
cannot make a declaration collide with itself, and a cycle ends rather than
failing. Identity is the resolved path, so two spellings of one file are one
file. `lib/vector.pt` uses `lib/arith.pt`, and `examples/use.pt` uses both.

What it brings is what it declared. `examples/use.pt` takes its arithmetic from
`lib/arith.pt` and declares its own `@comment` and `@separator`, because those
belong to the file being written and not to the arithmetic in it.

Two used files that declare one thing are refused, and a file says which it
meant. §3.10.

### 3.6 `@mode expression` · `@mode text`

`expression` is the default and parses the whole body with the declared grammar,
calling anything unmatched an error. `text` scans the body, fires a rule where
one matches, and copies everything else through. §7.

### 3.7 `@end`

Ends the header. §2.2.

### 3.8 `@template name(x, y) { … }`

Names a piece of template so it can be called from more than one rule.
`@fragment` (§3.9) does the same for a piece of *pattern*, and those two are the
only things besides a rule that can be named.

```
@template load(x) {
    if level(x) == 1000 { emit "\tmov x0, #" + x + "\n" } else { emit x }
}

@syntax a "+" b 60 => { load(a) load(b) emit "\tadd x0, x0, x1\n" }
```

- **It is called as a statement**, on a line of its own, and **emits into
  whatever called it** — `emit` already writes to one place, so there is nothing
  to return. Using one where a value is wanted is
  `'load' is a template — it is called as a statement…`, and calling a *builtin*
  as a statement is the same mistake the other way round.
- **Its body sees its parameters and its own loop variables, and nothing else.**
  Not the caller's holes: `'y' is not one of this template's parameters`. That
  is what lets it be read on its own, and checked without knowing who calls it.
- **Calls are resolved once the header has finished**, so a rule may call a
  template declared after it, or one that arrived through `@use`, and the order
  a file writes its directives in does not change the answer (§3.5, §3.8).
- **A template may call a template**, itself included; 64 deep is
  `templates called more than 64 deep`.
- **Fresh names are shared with the caller.** `fresh("L")` inside a template is
  the same name as `fresh("L")` in the rule that called it, which is what lets a
  template finish a construct the rule started (§8.2).
- At most **8 parameters**. Declaring the same name twice is refused unless the
  second says `override`, as everything else is (§3.10).
- `examples/asm.pt` is the customer: one `load` against eight call sites.

### 3.9 `@fragment name = pattern`

Names a piece of **pattern** so it can be spliced into more than one rule.
`@template` (§3.8) is the same idea one side over; they are two mechanics and
not one, for the reasons below.

```
@fragment params = "(" [ p:name ":" "integer" ]* sep ";" ")"

@syntax "procedure" f:name @params ";" b        => { … }
@syntax "function"  f:name @params ":" "integer" ";" b  => { … }
```

- **It is spliced where it is named**, with `@name`, at the point the rule is
  declared. Its elements are copied in, and from there on nothing can tell them
  from elements written out by hand: the rule is checked, sealed, matched and
  clashed (§3.10) as one pattern. `@` can be nothing else in a pattern — an
  element is a quoted word, a hole, a group or a level — so this reserves no
  name.
- **It brings its own holes.** The two rules above splice `{p}` without
  declaring `p`, because the fragment declared it. That is the difference
  between a fragment and a hole, and it is why a splice is `@params` rather than
  a kind: a *kind* says what one hole holds, and a fragment says what sequence
  of elements goes here.
- **It takes no arguments and has no scope.** A template is *called* at
  expansion, takes arguments and can recurse; this is nearer to a macro over
  pattern text. One directive covering both would be one word meaning two
  things.
- **It must be declared before it is spliced** — otherwise
  `no fragment called '@params'`, the same thing `@token` asks of a class used
  as a kind (§4.3). Two things come free with that: a fragment cannot splice
  itself, so no cycle is expressible, and there is no order in which a file
  could have meant something else.
- **A fragment may splice a fragment**, since that one is already declared.
- **A level belongs to a rule, not to a fragment**: it says how tightly *one*
  rule binds, and a fragment is spliced into any number of them. Writing one is
  `a level belongs to a rule and not to a fragment`.
- **Its pattern is checked at each splice**, not at the declaration, because
  every check in §4.2 is about a *whole* pattern — what stops a greedy hole is
  the element after it, and a fragment does not know what will follow it.
- `override` sits **before the `=`**, which is the opposite of everywhere else
  and is forced: a fragment's pattern runs to the end of the directive, so there
  is no *after* in which a bare word could not be part of the pattern, and a
  trailing `override` would be read as a hole of that name. Before the `=` it is
  a modifier on the declaration, which is what it always was. Rules already
  spliced from the earlier declaration keep what they copied.
- `examples/pascal.pt` and `examples/code.pt` are the customers: each wrote one
  parameter list twice, once in `procedure` and once in `function`.

### 3.10 `override` — two files declaring one thing

Five things can be declared twice: a rule's pattern, a `@token` class name,
`@separator`, a `@template` name, and a `@fragment` name. **Unmarked, the second is an error naming both lines.** Marked
`override`, the second wins and nothing is said — because it was said in the
source.

```
@use "ops.pt"                             ; which declares  a "/" b  70
@syntax a "/" b 70 => "idiv({a}, {b})" override
```

`examples/use.pt` does exactly this to the `/` it took from `lib/arith.pt`.

```
b.pt:1: this pattern is already declared at a.pt:3
        -- write 'override' after the template to mean it
```

- **`override` with nothing to displace is also an error.** The word stays true
  that way: it cannot quietly become noise when the declaration it was written
  against is renamed, moved, or dropped from the file it came from.
- **What counts as one pattern is what matching can tell apart**: the same
  elements, in the same order, with the same words, the same hole kinds, the
  same group shapes. **Hole names are not part of it** — `a "+" b` and
  `x "+" y` match the same text, so the second is unreachable whatever its
  holes are called. **Nor are levels** — one pattern at two levels is a grammar
  that cannot say which it means, and `override` is how it says.
- **Two rules that merely share a leading word do not collide.** That is the
  candidate mechanism, not an accident: `"if" c "then" t` and
  `"if" c "then" t "else" f` coexist, `"-" a` and `a "-" b` coexist, and
  `examples/poem.pt` declares `-`, `--` and `---`. §4.2, §5.
- **The rule check runs once the header has finished**, so the order a file
  writes `@mode`, `@syntax` and `@use` in does not change the answer, and a
  rule that came in through `@use` is named at the line in the file that wrote
  it. A class and a separator are checked where they are written, because a
  later one replaces the earlier in place.
- **Which one wins is not which one is doing the overriding.** Without
  `override`, nothing wins: it is refused. This is the one question the tool
  declines to answer by position, for the same reason it declines to guess
  precedence.

---

## 4 · Patterns

```ebnf
pattern     = element { element } .
element     = string | hole | group | splice .
hole        = name [ ":" kind ] .
kind        = "expr" | "stmts" | "text" | a class named by @token .
group       = "[" element { element } "]" [ rep ] .
splice      = "@" a name declared by @fragment .
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
| two holes with one name | `two holes called 'p': a template splices a hole by name, so only one of them could ever be reached` |

Two holes may not be adjacent when the first is **greedy** — `expr` or `stmts`,
the kinds that read up to a word — because it would take everything the second
wants: given `a b` and input `f x + y`, the first hole takes the sum and the
second finds nothing. A class-kind hole takes exactly one token and cannot be
greedy, so `"f" a:name b:name` is allowed. The check looks through a group's
brackets: a group's last element is adjacent to whatever follows the group.

A pattern may not declare **one hole name twice**. A template splices a hole by
name and the first one wins, so the second could never be reached — silently,
until it was refused. Writing it out by hand was always a mistake and nobody had
made one; splicing a fragment (§3.9) twice into one rule makes it easy to make by
accident, which is what asked for the check.

Where a body is wanted, a rule takes its own delimiters —
`"if" "(" c ")" "{" t:stmts "}"` — rather than needing a kind that means *a
block*.

### 4.3 Kinds

| kind | takes |
| --- | --- |
| `expr` | one expression, at a binding power set by §5. **The default.** |
| `stmts` | statements separated by the declared separator, up to the pattern's next word. Expanded, and joined with the separator's output form. |
| `text` | raw source text up to the pattern's next word. Text mode only (§7); in expression mode it is `a 'text' hole belongs to @mode text`. |
| *a class name* | exactly one token of that class, spliced as its source text. Expression mode only (§7); in text mode it is `'x:name' asks for one token of a class, and text mode has no tokens`. |

**A fragment is not a kind, and does not appear here.** A kind says what *one
hole* holds; a fragment (§3.9) says what *sequence of elements* goes here and
brings its own holes, so it is spliced with `@name` in a namespace of its own.
Naming a `@fragment` and a `@token` class the same thing is therefore not a
collision, and neither can shadow the other.

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
ask — a code template can (§8.3), and a string template cannot.

**A group is matched at binding power 0** and is delimited by its own words. A
turn that consumes no tokens ends the repetition. A failed optional group, or a
failed turn, restores both the token cursor and every binding made inside it.

Groups may nest, 16 deep, in either mode.

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
- **On the way in, a separator is wanted between two statements, and not after
  one that ended in a word.** That is what lets `}` and `end` stand on their
  own, so C's `for (…) { … }` needs no `;` after it.
- **On the way out, a separator is joined between two statements unless the rule
  that produced the first was declared `terminated`** (§3.4), in which case a
  newline is joined instead.

**These are two rules and not one, deliberately.** The first is about the
language being read and the second about the language being written, and this
tool is not entitled to assume they agree.

`examples/clike.pt` and `examples/groups.pt` both read C's braces and disagree
about the output. The first writes a language that wants a separator between two
statements *however the one before ended* — so it declares nothing, and a
separator is joined after a `}` like anywhere else. The second writes
JavaScript, where a `}` has already ended the statement, so its brace rules say
`terminated` and nothing is joined after them.

Working it out from the last character emitted would get one of them wrong, and
would get C's own `struct { … };` wrong in the other direction.

If the body does not parse, the error names the furthest token reached:
`no rule reads 'x' here`, or `the file ends in the middle of something`.

---

## 7 · Text mode

`@mode text`. The body is scanned; where a rule matches it fires; everything
else is copied through unchanged.

- Only **nud** rules apply. A led rule has nothing to continue.
- **Matching is a search, not a scan.** A rule takes an alternative, tries the
  whole remainder of its pattern, and puts the cursor and every binding back if
  it fails. That is what groups need — an optional part may or may not be there,
  so what follows a hole is not known until the rest has been tried — and it is
  bounded by a budget (`this rule has too many ways to match`).
- **Groups work**, `[ … ]`, `[ … ]*` and `[ … ]+` with `sep` and `join`, exactly
  as in expression mode (§4.4). A hole inside a repeated group is a list, which
  a code template can loop over.
- **The longest leading word that matches wins.** Declaration order breaks a tie
  between two of the same length and decides nothing else — `examples/poem.pt`
  declares `-`, `--` and `---` in that order and `---` still wins.
- **Every hole is text** and takes the shortest run that lets the rest of the
  pattern match. A hole with nothing after it takes the rest of the enclosing
  text.
- **A class kind is refused here.** `expr` and `stmts` both mean *read up to the
  word that stops you*, which is what a text-mode hole does anyway, so those
  degrade honestly and are left alone. A class says something else — *one token,
  matching this regex* — and text mode has no tokens, so `"[" x:name "]"` took
  everything up to the `]` and never consulted the kind. It read as if it had
  worked, which is why it is now refused rather than honoured; honouring it is
  [ROADMAP.md](ROADMAP.md)'s job if anybody asks. **The check runs once the
  header has finished**, not in the rule that declared it, because `@mode` is a
  directive like any other and may be written after the rule, or in the file
  that `@use`d it.
- **A hole may not span the word that closes the rule** — the last literal word
  in its pattern, groups looked into. In `"[[" t "|" u "]]"`, a `]]` reached
  before the `|` means the construct has already ended, so the rule fails and
  the next one is tried, rather than `t` swallowing the close and the search
  running on to whatever `|` appears later in the file. `examples/poem.pt` pins
  it, and [POSTMORTEM.md](POSTMORTEM.md) 4 says what it cost to find.
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

**There are two kinds, and one character tells them apart.** After the `=>`, a
`"` begins a string template and a `{` begins a code template. Nothing had to be
reserved for this: a template had always been a string, and a string never
starts with a brace.

```ebnf
template = string                      (* §8.1, splicing            *)
         | "{" { stmt } "}" .          (* §8.3, an interpreted one  *)
```

A string template **splices** and does nothing else. A code template can loop
over a repeated hole, ask whether an optional part matched, ask an operand what
level it was parsed at, and build text. Use the first where it is enough, which
is most of the time.

### 8.1 The string template

```ebnf
string template = string, with "{" name "}" splices, "{~" name "}" fresh names,
                  and "{{" "}}" for a literal brace .
```

| in a template | emits |
| --- | --- |
| `{name}` | the hole `name`, expanded — every turn of it if it is in a repeated group (§4.4), the empty string if its optional group did not match |
| `{~label}` | a name nobody else has (§8.2) |
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

### 8.2 Fresh names

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

`fresh("t")` is the same thing in a code template: **one name per label per
application**, so two `fresh("L")` in one template are one name and
`fresh("Lelse")` beside it is another. It draws from the same counter, so a
string template and a code template in one file never collide. Until 2026-09-05
it returned a new name on every call, which is [POSTMORTEM.md](POSTMORTEM.md) 10;
`examples/asm.pt` is what needed a label in two places and found it.

This closes the half of hygiene where a template **introduces** a name. It does
not touch the half where a template **reaches out** for one the caller shadowed:
there is nothing to invent there, and **neither** kind of template can see a
scope. `examples/hygiene.pt` demonstrates both and `tests/hygiene.sh` runs them.

### 8.3 The code template

```ebnf
code   = "{" { stmt } "}" .
stmt   = "emit" expr
       | "if" expr code [ "else" code ]
       | "for" [ name "," ] name "in" expr [ "sep" expr ] code
       | name "(" [ expr { "," expr } ] ")" .    (* a template, §3.8 *)
expr   = expr ( "and" | "or" ) expr
       | expr ( "==" | "!=" | "<" | ">" | "<=" | ">=" ) expr
       | expr ( "+" | "-" ) expr              (* + joins or adds   *)
       | expr ( "*" | "/" | "%" ) expr        (* numbers only      *)
       | "not" expr
       | string | integer | name
       | name "(" [ expr { "," expr } ] ")"
       | "(" expr ")" .
```

`emit` is the only way out: what a rule expands to is everything it emitted, in
order. Statements need no separator; a `;` between them is allowed and ignored.
Strings in a code template are **Prototype strings**, spelled Prototype's way,
exactly as `@syntax`'s own words are — the language is Prototype's, so it lives
outside the strings and the foreign text it emits lives inside them.

**Its own words are `emit`, `if`, `else`, `for`, `in`, `sep`, `not`, `and` and
`or`.** A hole may not be one of them.

**`for i, x in h`** binds the position as well as the turn — the first name is
the index, counting from 0, the way Go and Python's `enumerate` read it. With
one name there is no index. It exists because **two holes in one repeated group
are two parallel lists** and nothing else pairs them:

```
@syntax "case" e "of" [ v ":" s ]* sep ";" "end"
    => {
        emit "switch (" + e + ") {"
        for i, x in v sep "\n" { emit "case " + x + ": " + at(s, i) + "; break;" }
        emit "}"
    }
```

`examples/code.pt` is that rule; `examples/pascal.pt` cannot write it and folds
the pair into one hole with an infix rule instead.

**What is in scope**: every hole the pattern declares, and the loop variables
around the statement. A hole inside a repeated group is a **list**; every other
hole is text. That list is the one thing `join` throws away, and having it is the
whole difference between the two kinds of template.

**Truth**: a list is true when it has turns, an integer when it is not zero, a
text when it is not empty.

**Arithmetic.** `*` `/` `%` bind tighter than `+` `-`, which bind tighter than a
comparison; `not` binds tighter than all of them, where it always did. `-` `*`
`/` `%` want **two numbers** and are an error otherwise — nothing here reads a
number out of text that merely looks like one, and `num(h)` is how a hole says
it meant one. `+` is the exception and follows comparison's rule: it **adds**
when both sides are already numbers and **joins** when they are not. Division or
remainder by zero is an error.

A rule whose template computes rather than writes is what `examples/calc.pt` is:
`=> { emit num(a) * num(b) }` puts a number back into the parse as the value of
that subexpression. **Evaluation is eager** — a hole is filled before the
template runs — so a rule can select between two already-computed values but
cannot leave one uncomputed. See its header for what that rules out.

| builtin | gives |
| --- | --- |
| `matched(h)` | whether `h`'s group matched at all |
| `count(h)` | how many turns a repeated hole took |
| `at(h, n)` | the turn at position `n`, counting from 0. Out of range is an error, not an empty string — two groups of different lengths is the mistake `at` exists to catch |
| `num(h)` | `h`'s text read as a number. The **whole** text or none of it: `'12abc'` is an error, not 12 |
| `level(h)` | the level of the rule that filled `h`; 1000 for an atom |
| `terminated(h)` | whether the rule that filled `h` was declared `terminated` (§3.4) — that is, whether `h`'s text already ends a statement. For a `stmts` hole it is the **last** statement that answers; for a bare token, and for a hole nothing filled, it is false |
| `group(h, n)` | `h`, bracketed in `(` `)` when `level(h) < n` |
| `replace(s, from, to)` | `s` with every `from` replaced |
| `drop(s, front, back)` | `s` with that many characters off each end |
| `fresh(label)` | a name nobody else has — §8.2 |

Everything in a code template is checked at the `@syntax` that wrote it: a name
that is neither a hole nor a loop variable, a builtin nobody has, the wrong
number of arguments, a loop variable that is also a hole.

`examples/code.pt` is `examples/pascal.pt` with every rule rewritten in this
form and the body left alone, so `diff examples/pascal.out examples/code.out` is
what the second form is for:

```
-for (…) { if (((((i % mod) == 0)) && ((i != 9)))) { total = (total + i); } … }
+for (…) if ((i % mod == 0) && (i != 9)) total = total + i; …
-if ((!((total > 100)))) { puts('it''s middling'); } else { puts('big'); }
+if (!(total > 100)) puts("it's middling"); else puts("big")
-if ((total > 30)) { { … }; } else { printf("%d\n", total); }
+if (total > 30) { … } else printf("%d\n", total)
```

Three things, one per builtin. The parentheses come from `group(a, 60)` asking
an operand its level. The braces and the semicolons come from `terminated(t)`
asking whether a branch already ends a statement — a string template has to
brace every branch, because bracing is right either way and it cannot ask. The
literal comes from `replace(drop(x, 1, 1), "''", "'")`.

**And one place it buys nothing**, which is as much use to know. Both files turn
Pascal's `(a: integer; b: integer)` into C's `(int a, int b)` and produce the
same text: the code template loops over the list, and the string one gets there
with `join ", int "`, because every turn wants the same word in front of it. A
loop is the general form and `join` is the special case, and the special case is
common enough to be worth reaching for first.

`tests/pascal.sh` compiles what the second one emits and runs it, and the first
is expected **not** to compile: that literal is the only thing wrong with it,
and no `@syntax` can reach it, because a rule cannot match a bare token.

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
words      '(' ')' '*' '+' ',' '-' '/' ':' ';' '<' '=' '>' '^'
prefix    "(" e ")"
infix     a ":" m [95]
infix     a "^" b [80 right]
prefix    "-" a [90]
…
prefix    "<" x "," y ">"
infix     a "/" b [70]
```

The last two lines are the point of that file: `<x, y>` came in through
`lib/vector.pt`, and `/` is the rule the file declared `override` for, so only
one `/` is listed.

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
| `'x:name' asks for one token of a class, and text mode has no tokens` | §7 |
| `a rule needs a pattern` | `@syntax => "…"` |
| `trailing text after the template` | something after the template that is not `terminated` or `override` |
| `an empty word matches nothing` | `""` as a pattern element |
| `expected '=>'` | a rule with no template |
| `a rule that begins with a hole is infix or postfix and needs a level` | §4.2 |
| `a rule that begins with a hole must have a word after it` | §4.2 |
| `two holes in a row: …` | §4.2 |
| `a 'stmts' hole needs a word after it to stop at` | §4.3 |
| `unclosed '{' in a template` | §8 |
| `the template splices '{x}' and the pattern has no such hole` | §8 |
| `a fresh name needs a label: '{~name}'` | §8.2 |
| `'{~a}' and '{a}' would read as one thing: …` | §8.1 |
| `expected 'emit', 'if' or 'for'` | a code template's statement is none of those — §8.3 |
| `expected a value` · `expected ')'` · `expected '{'` · `expected 'in'` · `expected a name after 'for'` | a code template's own syntax — §8.3 |
| `'in' is one of this language's own words and cannot be a value` | §8.3 |
| `a block ends in the middle of something` | an unclosed code template |
| `nothing in this language is written 'x'` | a character a code template has no use for |
| `the template uses 'x' and the pattern has no such hole` | §8.3 |
| `no such thing as 'x'` | a builtin nobody has — §8.3 |
| `'group' takes 2 and was given 1 — it gives …` | wrong arity for a builtin |
| `the loop variable 'a' is also a hole — one of them has to be called something else` | §8.3 |
| `expected a name after ',' in 'for'` | `for i, in h` — §8.3 |
| `'for i, i' names the position and the turn the same thing` | §8.3 |
| `'at' was given 9 and there are 2` | a position past the end of a list — §8.3 |
| `'*' wants two numbers and was given 'x' and '2' — num(h) reads a hole as one` | §8.3; likewise `-`, `/`, `%` |
| `'num' wants a number and was given 'x'` | §8.3 |
| `'/' by zero` | §8.3; likewise `%` |
| `expected 'emit', 'if', 'for' or a template call` | a statement that is none of those |
| `no template called 'x'` | §3.8 |
| `'t' takes 2 and was given 1 — declared at f:n` | §3.8 |
| `'y' is not one of this template's parameters` | §3.8 — a template cannot see the caller's holes |
| `'level' is a builtin and gives a value — put it in an 'emit'…` | §3.8 |
| `'t' is a template — it is called as a statement…` | §3.8 |
| `the template 't' is already declared at f:n` | §3.10 |
| `'override', but no template 't' was declared before it` | §3.10 |
| `expected a name after '@template'` · `expected '(' after a template's name` · `expected a parameter name` · `a template's body is a block` | §3.8 |
| `a template takes at most 8 parameters` | §11 |
| `templates called more than 64 deep — 't' calls itself without stopping` | §11 |
| `loops nested more than 32 deep` | §11 |
| `cannot open path` | `@use` |
| `a used file holds directives and nothing else` | §3.5 |
| `@use nested more than 64 deep` | §3.5 |
| `this pattern is already declared at f:n — write 'override' after the template to mean it` | §3.10 |
| `'override', but nothing with this pattern was declared before it` | §3.10 |
| `the class 'x' is already declared at f:n — write 'override' to mean it` | §3.10 |
| `'override', but no class 'x' was declared before it` | §3.10 |
| `the separator is already declared at f:n — write 'override' to mean it` | §3.10 |
| `'override', but no separator was declared before it` | §3.10 |
| `no fragment called '@p'` | §3.9 — spliced before it was declared, or never declared |
| `expected a fragment's name after '@'` | a bare `@` in a pattern — §3.9 |
| `a fragment needs something in it` | `@fragment p =` with no pattern — §3.9 |
| `expected a name after '@fragment'` · `expected '=' after a fragment's name` | §3.9 |
| `a level belongs to a rule and not to a fragment` | §3.9 |
| `the fragment 'p' is already declared at f:n — write 'override' to mean it` | §3.10 |
| `'override', but no fragment 'p' was declared before it` | §3.10 |
| `two holes called 'p': …` | §4.2 |
| `trailing text after @token` · `trailing text after @separator` · `trailing text after @fragment` | a word after the directive that is not one of its own |

### In the body

| message | means |
| --- | --- |
| `nothing here is anything this file declared: '…'` | a character that matches no class and begins no word — §6.1 |
| `no rule reads 'x' here` | the parser stopped; `x` is the furthest token it reached |
| `the file ends in the middle of something` | as above, at end of file |
| `a 'text' hole belongs to @mode text` | §4.3 |
| `the grammar recurses without consuming anything` | 400 deep — §6.2 |
| `a text rule expands into itself` | 64 deep — §7 |
| `this rule has too many ways to match` | a text rule's search ran past its budget — §7 |
| `no fresh name for '{~t}' is free` | 100000 candidates were all taken — §8.1 |

---

## 11 · Limits

| | |
| --- | --- |
| `@use` nesting | 64 (and a file is read once, §3.5) |
| group nesting | 16 |
| loop nesting in a code template | 32 |
| template call depth | 64 |
| parameters of one template | 8 |
| text-mode match attempts per rule | 200000 |
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
| the template | an expression in the target language | a string, or an interpreted block — §8 |
| output precedence | Proto re-prints and parenthesises | `group(h, n)` in a code template; the author's own parentheses in a string one |
| hygiene | the expander renames | `{~t}` for half of it, §8.1 |
| source maps | a `.map` beside the output | none |
| the target | Solveig, one language it was built to write | anything; the examples here write C and Pascal |

`docs/notation.md` argues about which of those are gains and which are the
price. This page only says which are which.
