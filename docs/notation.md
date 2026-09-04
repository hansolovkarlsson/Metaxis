# The notation

`.pt` in, `.out` out. What this changes, against Proto, is the *mention* of
foreign text inside a directive — which is where Proto's grammar and the
grammars it declares keep meeting.

Everything on this page is implemented and is exercised by `examples/`.
What is not is under [Not done](#not-done), at the bottom.

**This page argues; [REFERENCE.md](REFERENCE.md) states; [ROADMAP.md](ROADMAP.md)
says what to do about what is missing.** Every directive,
kind, level, template form, lexer decision and error message is there,
exhaustively. Read that one to write a `.pt`; read this one to know why it is
shaped the way it is, and what it cost.

## The one rule

> **Everything a directive says about text — the text it recognises and the
> text it emits — is inside a string. Everything outside a string is
> Prototype's own fixed vocabulary.**

A directive mentions foreign text; the body is foreign text. Quoting is what
tells the two apart, and it is the only thing that has to. Strings already have
exactly one escaping rule, understood before anything else in the file, so
there is nowhere left for an ambiguity to live.

It applies to both sides of a rule, which is what makes it a rule and not a
convention:

```
@syntax "if" c "then" t  =>  "if ({c}) {{ {t} }}"
        ~~~~     ~~~~~~      ~~~~~~~~~~~~~~~~~~~
        input, quoted        output, quoted
             c        t          {c}      {t}
             holes, bare         splices, braced
```

- **Left of `=>`**: quoted means a literal word of the input; bare means a hole.
- **Right of `=>`**: one string; `{name}` splices a hole; `{~name}` is a name
  nobody else has; `{{` and `}}` are a literal brace.

The header's own strings are always Prototype's, spelled Prototype's way. The
body's strings are whatever `@token string` said they are. The two never meet.

## What falls out of it

**One directive instead of four.** Proto needs `@infix`, `@infixr`, `@prefix`
and `@syntax` because the directive's *name* is what says which positions are
operands. Once words are quoted, position says it:

| pattern | what it is | why |
| --- | --- | --- |
| `a "+" b` | infix | hole, word, hole |
| `"-" a` | prefix | begins with a word |
| `a "++"` | postfix | begins with a hole, ends with a word |
| `"(" e ")"` | circumfix | words at both ends |
| `"~"` | a word alone | no holes at all |
| `"if" c "then" t` | mixfix | the general case |

The Pratt distinction is already in the shape: a pattern that **begins with a
hole** is a led rule and needs a level; one that **begins with a word** is a nud
rule and does not. Nobody declares which — it is read off, in
`header.c:rule_syntax`, in one line.

**`<` and `>` stop being needed.** A hole is a bare name because a word is a
quoted string, so a pattern's own punctuation no longer comes out of the pool of
characters the file is busy declaring.

**The lexer stops being a fixed budget.** Proto's operator characters are a
closed set — `. , : | ;` are spoken for and `;` opens a comment — because its
lexer runs before it knows what the file declared. Here the header is read
first and `lex.c` runs second, so `;` as a statement separator, `|` as bitwise
or and `.` as field access are all writable: none of them is anything until a
string says so.

**Literals become declarable too.** `@token number "[0-9]+"` is what lets `42`
be an integer without a sigil — the one thing on Proto's impossible list that is
the lexer's rule about literals rather than a collision. Pascal's `'it''s'` and
C's `0x1f` are the same directive with a different string in it.

**Alphabetic words still are not reserved.** See *Ties*, below. `then` is a
form's word where a form wants one and a name everywhere else, including in the
same file — `examples/tour.pt` uses `then` both ways four lines apart.

**Directives stop needing a terminator.** Proto ends one with `.`, which is also
the statement separator inside the template. Here a directive ends at a newline,
and continues only onto a line that is *indented*.

## The directives

```ebnf
module      = header body .
header      = { directive } [ "@end" ] .

directive   = "@use"       string
            | "@mode"      ( "expression" | "text" )
            | "@token"     name string
            | "@comment"   string ( string | "eol" )
            | "@separator" string [ "=>" string ]
            | "@syntax"    pattern [ level ] "=>" template
            | "@end" .

pattern     = element { element } .
element     = string | hole | group .
hole        = name [ ":" kind ] .
kind        = "expr" | "stmts" | "text" | a class named by @token .
group       = "[" element { element } "]" [ rep ] .
rep         = ( "*" | "+" ) [ "sep" string ] [ "join" string ] .
level       = integer [ "left" | "right" ] .
template    = string, with "{" name "}" splices, "{~" name "}" fresh names,
              and "{{" "}}" for a literal brace .
```

Every string above is a Prototype string: `"…"` with `\"` `\\` `\n` `\t` `\r`
and no other escape. That never varies, in any file, whatever the file declares.

**`@token`** names a class and gives a POSIX extended regular expression for it.
Redeclaring a name replaces it.

**`@comment`** adds an opener. It does not replace anything: `;` to end of line
is Prototype's own header comment and always works, and a declared comment joins
it. Below the body, only the declared ones are left.

**`@separator`** gives the input separator and, after `=>`, what to join the
output with. Without the second half the input text is reused. A separator of
`"\n"` makes newline a token instead of whitespace — `examples/reserved.pt`
uses it.

**`@use`** reads another file's directives into this header. It is looked for
beside the file that used it, holds directives and nothing else, and stops at
64 deep. `examples/use.pt` takes its arithmetic from `lib/arith.pt` and keeps
its own comment and separator, which is the division that file is for.

**`@end`** ends the header. Without it the header ends at the first line that
does not begin a directive. Either way it is one-way: below it nothing is a
directive, so a body may begin a line with `@` and mean it — which
`examples/reserved.pt` does, having declared `@` as an operator.

## Fresh names

`{~t}` in a template is **a name nobody else has**. One expansion, one name: two
`{~t}` in a template are the same name, and the next use of the rule is a
different one. `examples/hygiene.pt` calls one `swap` twice and the output has
`t__1` and `t__2`.

A name is taken if it occurs anywhere in the source being expanded or in any
template any rule declared, `@use` included. That is a substring test, so it is
conservative in the safe direction — `t__1` is refused while `t__12` is in the
file — and it costs one scan per name.

A label is not a hole and cannot share a name with one: `{~a}` beside `{a}`
would read as one thing and is refused where it is written, not where it is
used. Every splice in a template is checked at the `@syntax` that wrote it.

**This closes exactly half of the hygiene problem, and the half it does not
close is not a missing feature.** See *A template can name its own temporary and
nothing else*, below.

## The rules the implementation had to settle

Six things quoting does not decide by itself. Each is a decision, and each is
one line to state.

**A hole's kind is how far it reaches.** `expr` parses an expression, `stmts` a
run of statements up to the pattern's next word, `text` raw source, and a class
name exactly one token of that class. `examples/pascal.pt` needs `i:name` in
`"for" i:name ":=" …` so that the `:=` is the `for`'s and not the infix rule's:
a `name` hole takes one token and stops, where an `expr` hole would take the
assignment. This is the one thing quoting does not do by itself.

**Which bracket a group uses is a readability question and not a structural
one, and that it can be is the rule working.** `[`, `(`, `{` and `<` were all
equally free, because foreign text is quoted and a pattern's punctuation can
never collide with it — the same question in Proto would have been painful,
since `<` and `>` are operator characters a file might want, which is what
`<x>` holes cost there. `[ … ]` was taken because ISO EBNF already spells an
optional part that way and a reader arrives knowing it, and because braces were
spoken for by templates and reading a pattern beside its template is the common
act. The suffixes `*` and `+` are regex's rather than EBNF's, which buys one
bracket and three forms in a series instead of two brackets — `[ … ]`,
`[ … ]*`, `[ … ]+`, one thing to learn.

Swapping the pair for `( … )` was offered and declined, and the brackets are
settled. It leaves `( … )` unspent, which is worth something on its own: a
bracket nothing has claimed is cheap to keep and expensive to get back.

**A group needs no new syntax to be safe from the body.** `[ … ]`, `*`, `+`,
`sep` and `join` are Prototype's vocabulary and live *outside* the strings, so a
file that wants `[` and `]` in its own language quotes them and the two never
meet — `examples/clike.pt` declares `a "[" i "]"` for an index in the same tool
that reads `[ x ]* sep ","`. Proto declined repetition and optional parts three
times, in `conventions.md`, on the grounds that no program had asked; a
language-agnostic tool has argument lists everywhere and asks on the first file.

What a group does **not** buy is an output that differs on whether a part
matched. Every hole is bound — to its turns, or to nothing — and a splice is the
only thing a string template can vary, so `examples/groups.pt` still writes
`if (!0) { ; }` where its optional part was absent. That is
the plainest customer for the code template (§ *What it costs*), and the reason
`join` covers only the easy half of per-element output — `examples/code.pt`
writes the other half with a `for … sep` loop.

**Two holes may not sit next to each other.** Proto's rule, kept, and for
Proto's reason: given `a b` and `f x + y` the first hole takes the sum and the
second finds nothing. Proto allows the pair when the second is a `block`; here a
rule takes its own braces instead — `"if" "(" c ")" "{" t:stmts "}"` — so the
exception is not needed and does not exist. The refusal is narrowed to a
**greedy** first hole, since a class-kind hole takes one token and stops, which
is what lets `[ p:name ]*` be a parameter list.

**Ties go to the token class.** At each position the lexer takes the longest
match from a declared class and the longest match from a declared word, and
**the class wins a tie**. That is the whole of *an alphabetic word is not
reserved*: `div` is a `name` token whose text happens to be `div`, a rule that
wants the word compares text, and `divisor` is a longer class match than the
word so it never splits.

**Comments are looked for before words.** A file whose comment opener is also an
operator gets the comment. It is the only precedence in the lexer the file did
not set, and it is stated because it is not derivable.

**Candidates under one leading word are tried longest first**, with the token
cursor restored on failure. That is what makes `if c then t else f` win over
`if c then t`, and it is the whole of the dangling else: the inner `if` takes
the `else` because it is asked first. Proto matches its candidates in lockstep
and needs no backtracking; this backtracks, which is shorter and slower, and the
inputs are files.

**A separator is wanted between two statements, and not after one that ended in
a word.** That is what lets `}` stand on its own — C's `for (…) { … }` takes no
`;` after it, and Pascal's `end` takes none either — without a rule having to
declare itself terminating.

## Two modes

`@mode expression`, the default, parses the whole body with the declared grammar
and calls anything unmatched an error. `@mode text` scans the body, fires a rule
where one matches and copies everything else through verbatim; a hole's text is
expanded in its turn, so `**a //slanted// claim**` nests. Same directives, same
rule about quoting; the difference is only what happens to text no rule claimed.

## Running it

```
make            # bin/pt
make check      # every example against the .out beside it, then tests/errors.sh
make record     # re-record those .out files; read the diff before committing it

bin/pt examples/clike.pt          # to stdout
bin/pt -o out.sol examples/clike.pt
bin/pt -g examples/pascal.pt      # the grammar the header declared, and stop
```

C11 and `make`, plus POSIX `<regex.h>` for `@token` — which is in libc and is
the only thing here Proto does not also need.

## What it costs

**Output parenthesisation is the author's problem, in a string template.**
Prototype knows the input grammar because the file declared it, and a string
template can splice and nothing else. So `examples/pascal.pt` writes
`"({a} + {b})"` on every arithmetic rule, and `examples/pascal.out` is checked
in with the parenthesis noise that produces:

```
if (((((i % mod) == 0)) && ((i != 9)))) total = (total + i) else …
```

**A code template does not pay it.** `examples/code.pt` is the same file with
`=> { emit group(a, 60) + " + " + group(b, 61) }` in place of the string, and
`group(h, n)` asks an operand what level it was parsed at and brackets it only
where it must. Both outputs are recorded, and the diff between them is why the
second form exists. The cost is real and it is now a choice: it is what a string
template costs, and the price of a string template is that it is four times
shorter.

**A template can name its own temporary and nothing else.** `{~t}`, and
`fresh("t")` in a code template, closed the half of hygiene where a template
*introduces* a name. The half where it
*reaches out* for one stays open, and is not an unimplemented feature — it is
the price. `examples/hygiene.pt` declares a `bump` whose template means the
file-scope `total` that was in scope where the rule was written; a caller that
shadows `total` gets the shadow updated and the real one left alone, and both
numbers are wrong and neither is an error:

```
swap: 2 1      the template's own name, twice over -- {~t}
again: 4 3     and the second call site did not get the first's
bump: 105 0    would be  bump: 100 5
```

There is nothing for a fresh name to invent here. What is wanted is a way to say
*the outer one*, and **neither kind of template can see a scope**, let alone
reach past a caller's — the code template was built and this did not move, which
was predicted in the roadmap before it was written and is the one prediction
there that held. Proto can, because its expander works on trees in a
language whose scopes it knows; closing it here would mean Prototype learning
the output language's binding rules, which is the one thing being agnostic gave
up. `tests/hygiene.sh` compiles that output and runs it, so the line stays a
number.

**The output separator was joined between every pair of statements**, including
after one that ended in a word, so `examples/hygiene.out` used to carry a `};`
at file scope where C wants none. **This is settled**: a rule may be declared
`terminated`, and then nothing is joined after it.

What settled it is worth more than the fix. The obvious cheap answer — look at
the last character emitted, and skip the separator after a `}` — would have been
*wrong*, and wrong in both directions at once. `examples/clike.pt` reads C's
braces and emits Solveig, where a `.` is wanted between two statements however
the one before ended; `examples/groups.pt` reads the same braces and emits
JavaScript, where it is not; and C's own `struct { … };` wants the semicolon
after the brace. So the input rule and the output rule are about two different
languages and had to be two rules. Guessing is what this tool declines to do
about precedence and about scopes, and it declines here for the same reason.

**A literal is moved, not understood — again, in a string template.**
`examples/pascal.pt` emits `puts('it''s middling')` into C, because a `string`
hole splices the source text it matched and a splice is all there is.
`examples/code.pt` writes `replace(drop(x, 1, 1), "''", "'")` and gets
`puts("it's middling")`. Both are recorded, for the same reason as the
parentheses above.

**Verbosity, on the common case.** `@infix + 60 add.` becomes
`@syntax a "+" b 60 => "{a}:add({b})"` — half again as long, on the line a
dialect writes forty times. Sugar should be *derived* from `@syntax` rather than
primitive, so the general form stays the thing that is true. None is implemented.

**The lexer cannot run alone.** It needs the header, so a file cannot be
tokenised for an editor, a highlighter or an error message before its `@use`
chain has been resolved. Proto can. This is the real price of declared tokens.

**An undeclared character has no good error.** In Proto `;` is a comment and
says so. Here it is nothing until declared, and *nothing here is anything this
file declared* is all that is left to say.

**Longest match is now the file's business.** Declaring `"<"` and `"<<"` is
fine; declaring `"<"` and then `@use`-ing a file that adds `"<<"` re-lexes every
`a < <b` that was already written. Reading the whole header before the body
contains this, and does not remove it.

**Collision between two used files is unchanged.** Quoting settles *directive
against declaration*. It says nothing about two declarations of `"+"`, which is
the other problem and a different one. Today the later one wins, silently.

## Not done

**Source maps.** The output has no way back to the line that produced it, so an
error in a downstream compiler points into text nobody wrote. Proto emits a
`.map` beside its output.

**A rule's own words winning inside its brackets.** Proto has one place where a
context outranks a declaration — `#[k = v]`, where the pair separator shadows a
declared `=` at the top level of a key. Here a bracketed rule's interior words
are ordinary pattern elements, so the case does not arise in the same shape; but
nothing has been written that tests it, and a `list` kind with a declared
separator is the shape that would.

**Expression-mode backtracking has no budget.** Candidates are retried with the
cursor restored and only a recursion depth of 400 stops it; a pathological
header would be slow and nothing measures it. Text mode is no longer in that
position — its matcher became a search on 2026-09-04 and was given a budget of
200000 attempts per rule at the same time, and it *is* measured: 113KB of
markdown, 2000 lines, 60ms, with the `**` and `[[…]]` rules of
`examples/poem.pt`. The expression side has had no such measurement and no such
budget. See [ROADMAP.md](ROADMAP.md).

**`left` is accepted and does nothing**, since left is the default. It stays
because writing it is sometimes clearer than leaving it out. (The other half of
this entry — that a level on a nud rule sets its trailing hole's binding power
and was documented nowhere but here — stopped being true when
[REFERENCE.md](REFERENCE.md) §5 was written.)
