# The notation

An experiment, and nothing here is implemented. `.pt` in, `.out` out. What it
changes is the *mention* of foreign text inside a directive, which is where
Proto's grammar and the grammars it declares keep meeting.

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
- **Right of `=>`**: one string; `{name}` splices a hole; `{{` and `}}` are a
  literal brace.

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
| `"if" c "then" t` | mixfix | the general case |

The Pratt distinction is already in the shape: a pattern that **begins with a
hole** is a led rule and needs a level; one that **begins with a word** is a nud
rule and does not. Nobody declares which — it is read off.

**`<` and `>` stop being needed.** A hole is a bare name because a word is a
quoted string, so a pattern's own punctuation no longer comes out of the pool of
characters the file is busy declaring.

**The lexer stops being a fixed budget.** Proto's operator characters are a
closed set — `. , : | ;` are spoken for and `;` opens a comment — because the
lexer runs before it knows what the file declared. Here the whole header is read
first, and declared punctuation joins the token set by longest match. `;` as a
statement separator, `|` as bitwise or, `.` as field access: all writable,
because none of them is anything until a string says so.

**Literals become declarable too.** `@token number "[0-9]+"` is what lets `42`
be an integer without a sigil — the one thing on Proto's impossible list that is
the lexer's rule about literals rather than a collision. Pascal's `'it''s'` and
C's `0x1f` are the same directive with a different string in it.

**Alphabetic words still are not reserved.** They are matched by position, not
by the lexer, which is Proto's rule and a good one. `then` is a form's word
where a form wants one and a name everywhere else, including in the same file.

**Directives stop needing a terminator.** Proto ends one with `.`, which is also
the statement separator inside the template. A directive here ends at a newline
that is not inside brackets or a string, so nothing has to guess where the
template stopped.

## Grammar

```ebnf
module      = header body .
header      = { directive } [ "@end" ] .

directive   = "@use"       string
            | "@mode"      ( "expression" | "text" )
            | "@token"     name string
            | "@comment"   string ( string | "eol" )
            | "@separator" string [ "=>" template ]
            | "@syntax"    pattern [ level ] "=>" template .

pattern     = element { element } .
element     = string | hole .
hole        = name [ ":" kind ] .
kind        = "expr" | "name" | "number" | "string" | "block" | "line" | "rest"
            | a class named by @token .
level       = integer [ "left" | "right" ] .
template    = string, with "{" name "}" splices and "{{" "}}" for a brace .
```

Every string above is a Prototype string: `"…"` with `\"` `\\` `\n` `\t`. That
never varies, in any file, whatever the file declares.

**The header ends at the first line that does not begin a directive**, or at
`@end`. One-way: below it nothing is a directive, so a body may begin a line
with `@` and mean it. `@end` is there for the file that declares `@` as an
operator and wants to open the body with one.

**Two holes may not sit adjacent unless the second is a `block`** — Proto's
rule, kept, and for Proto's reason: given `a b` and `f x + y` the first hole
takes the sum and the second finds nothing.

**`@separator` has two halves for the same reason a rule does.** `@separator ";" => ";\n"` says the input separates statements with `;` and the output joins them with `;` and a newline. Where the output half is left off, the input text is reused.

**Two modes.** `@mode expression`, the default, parses the whole body with the
declared grammar and calls anything unmatched an error — a programming
language. `@mode text` scans the body, fires a rule where one matches and copies
everything else through verbatim — prose, markup, a poem. Same directives, same
rule about quoting; the difference is only what happens to text no rule claimed.

## What it costs

**Output parenthesisation is the author's problem.** Prototype knows the input
grammar because the file declared it, and knows nothing about the output's
precedence because the output is a string. Splicing `{a}` — which expanded to
`x + y` — into `"{a} * {b}"` gives `x + y * z`. The author writes
`"({a}) * ({b})"`, and that is the price of agnosticism, paid on every
arithmetic rule. Proto does not pay it: its templates are trees in a language it
knows, and it re-prints them with the parentheses they need. Letting a rule
declare its output level would buy it back and has not been tried.

**Verbosity, on the common case.** `@infix + 60 add.` becomes
`@syntax a "+" b 60 => "{a}:add({b})"` — half again as long, on the line a
dialect writes forty times. Sugar should be *derived* from `@syntax` rather than
primitive, so the general form stays the thing that is true.

**The lexer can no longer run alone.** It needs the header, so a file cannot be
tokenised for an editor, a highlighter or an error message before its `@use`
chain has been resolved. Proto can. This is the real price of declared tokens.

**An undeclared character has no good error.** Today `;` is a comment and says
so. Here it is nothing until declared, and "unexpected character" is all that is
left to say.

**Longest match is now the file's business.** Declaring `"<"` and `"<<"` is
fine; declaring `"<"` and then `@use`-ing a dialect that adds `"<<"` re-lexes
every `a < <b` that was already written. Reading the entire header before the
body contains this, but does not remove it.

**Collision between two dialects is unchanged.** Quoting settles *directive
against declaration*. It says nothing about two declarations of `"+"`, which is
the other problem and a different one.

## Unproven

**A rule's own words winning inside its brackets.** Proto has one place where a
context outranks a declaration — `#[k = v]`, where the pair separator shadows a
declared `=` at the top level of a key, and the README admits it is the
exception. If a dictionary is itself a rule, its `","` and `"="` are words in
*its* pattern and the exception becomes ordinary. This needs a `list` kind with
a declared separator, and it has not been tried.

**Hygiene.** Proto's templates are trees, so a name introduced by a template can
be renamed. A template that is a string cannot be — `"{{ | t | t := {a} }}"`
captures whatever the caller called `t`, silently, which is the failure
`Proto/examples/forms.pro` exists to demonstrate. Either the template gets a way
to ask for a fresh name (`{~t}`) or agnosticism costs hygiene. This is the most
serious open question here and the notation currently has no answer.
