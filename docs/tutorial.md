# Metaxis, one idea at a time

*This is the **tutorial**: the way in for a reader who has not seen a tool
shaped like this before. It teaches one concept per section, each with a small
file under `docs/tutorial/` that you can run, and shows what the tool prints
when you do. It explains and it does not argue: [REFERENCE.md](REFERENCE.md)
states what every part of a file means, [notation.md](notation.md) argues for
why it is shaped that way, and where this page and either of those disagree,
they are right and this page is wrong. A word this page uses without
explaining — nud, led, Pratt, attribute grammar — is in
[glossary.md](glossary.md). **Every transcript here is run by
`make check`** — `tests/docs.sh` finds each `$ mx …` line in `docs/`, runs it,
and compares — so nothing on this page can drift from the tool without the
suite going red.*

Run the files from the tree root, after `make`:

```
$ mx docs/tutorial/01-first.mx
add(x, 1);
(add(x, 1) * 2);
(add(x, 1) * 2)
```

---

## 0 · The idea, before any syntax

Most tools that read a language know the language before they see the
program. A C compiler knows C; a Markdown renderer knows Markdown. If you want
a new language, you write a new tool, or you learn a parser generator and
write a grammar file that a second tool turns into the first.

Metaxis reads a file that **declares its own language in its first half and
uses it in its second half**. The first half is a list of directives, each
beginning with `@`. The second half is text in whatever language the
directives described. The tool reads the directives, builds a reader for that
language on the spot, reads the second half with it, and prints what the
directives said to print instead.

So a `.mx` file is not a program in Metaxis. It is a program in a language of
its own choosing, with a preface that says what that language is and what to
turn it into. The tool knows nothing in advance — not `+`, not `if`, not that
a number is a number. Every one of those is declared, in the file, or it is
not there.

Three things are worth holding onto through everything below.

**Quotes mean "the text says this."** Inside a directive, anything in double
quotes is a piece of foreign text: a word the body will contain, or a piece
of output to write. Anything outside quotes is Metaxis's own vocabulary — a
directive name, a hole, a number, a bracket. This one rule is what lets a
directive *mention* a word like `if` without *being* an `if` statement, and it
is the whole reason the notation works.

**A hole is a name for something the body will fill in.** In a pattern, a
bare name like `a` or `body` is a hole. The body's text goes into it, and the
template splices it back out by the same name.

**Nothing is built in.** This is a cost, paid once per file, and it is also
the point: a file that declares `+` is a file that could have declared
something else, and the tool will never quietly prefer the language it was
written in.

---

## 1 · A first file

`docs/tutorial/01-first.mx`:

```
@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@separator ";" => ";\n"

@syntax "(" e ")"     => "{e}"
@syntax a "+" b   60  => "add({a}, {b})"
@syntax "double" e    => "({e} * 2)"
@end
x + 1;
double x + 1;
double (x + 1)
```

Read it top to bottom.

- **`@token number "[0-9]+"`** declares a *class* of token: a run of digits is
  a `number`. The quoted part is a regular expression. Without this line the
  tool would not know that `1` is anything.
- **`@token name …`** declares another class, for identifiers.
- **`@separator ";" => ";\n"`** says the body is a sequence of statements
  separated by `;`, and that in the output they should be separated by `;`
  and a newline. The part before `=>` is about the input; the part after is
  about the output.
- **`@syntax "(" e ")" => "{e}"`** is a rule. Its **pattern** is `"(" e ")"`:
  a literal `(`, then a hole called `e`, then a literal `)`. Its **template**
  is `"{e}"`: whatever `e` held, and nothing else. So brackets are read and
  then dropped.
- **`@syntax a "+" b 60 => "add({a}, {b})"`** is the rule for addition. The
  pattern begins with a hole, so this is an *infix* rule — something before
  the `+` and something after. The `60` is its **level**, how tightly it
  binds; §3 is about that. The template writes a function call around the two
  holes.
- **`@syntax "double" e => "({e} * 2)"`** begins with a word, so it is a
  *prefix* rule, and needs no level.
- **`@end`** ends the header. Everything after it is the body.

Now the output:

```
$ mx docs/tutorial/01-first.mx
add(x, 1);
(add(x, 1) * 2);
(add(x, 1) * 2)
```

Three statements in, three out, each rewritten by the rules that matched it.
Notice that the last two are the same: `double x + 1` and `double (x + 1)`
mean the same thing here, because `double` with no level takes everything up
to the end of the statement, and the brackets are read and dropped.

What happens if the body uses something the header did not declare?
`docs/tutorial/01-undeclared.mx` declares `+` and nothing else, and its body
says `1 + 2 * 3`:

```
$ mx docs/tutorial/01-undeclared.mx
mx: docs/tutorial/01-undeclared.mx:4: nothing here is anything this file declared: '* 3
'
```

The tool quotes the rest of the line from the character it could not read,
newline included, which is why the closing quote sits on a line of its own.
That message is the shape of the whole tool: it does not know `*`, and it
will not guess.

---

## 2 · Words and holes, and why nothing is reserved

`docs/tutorial/02-words.mx`:

```
@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@comment "#" eol
@separator ";" => ";\n"

@syntax a "div" b       70  => "({a} / {b})"
@syntax "if" c "then" t     => "when({c}, {t})"
@syntax a "=" b   10 right  => "{a} := {b}"
@end
10 div 2;             # `div` is a word here because a rule quoted it
divisor = 5;          # and `divisor` is a name, because it is longer
then = 3;             # `then` is a name here...
if then then then;    # ...and a word here, and a name again, by position
```

```
$ mx docs/tutorial/02-words.mx
(10 / 2);
divisor := 5;
then := 3;
when(then, then)
```

Two things are happening, and both come from the same rule about quotes.

**A quoted string in a pattern is a word the body must contain.** `"div"`,
`"if"`, `"then"` and `"="` are words because a rule quoted them. A word does
not have to look like punctuation; `div` is a word made of letters.

**But declaring a word does not reserve it.** The line `then = 3` uses `then`
as a variable name, and `if then then then` uses it as a name, a word, and a
name again — and the tool gets every one of them right, without being told.
It can, because a rule matches a word *by position*: the `if` rule wants the
word `then` after its first hole, so whatever sits there is checked against
the text `then`. Anywhere else, `then` is just a token of the class `name`.

The mechanism behind it is one sentence from the reference: at every
position, the tool takes the longest match from every token class and the
longest match from every declared word, and **the class wins a tie**. So
`div` is a `name` token whose text happens to be `div`; the `div` rule
compares text and matches. `divisor` is a longer `name` than the word `div`,
so it is never split into `div` and `isor`. This is what lets a `.mx` file
read a language with keywords without ever having a keyword list.

`@comment "#" eol` declares a comment form for the body: `#` to the end of
the line. The header has its own comment character, `;`, which always works;
the body has whatever the file declares. Comments are removed before anything
else is looked at.

---

## 3 · Fixity and precedence

The pattern says the **shape** of a rule; the level says how **tightly** it
binds. There is no separate directive for infix, prefix, or anything else,
because the shape is visible in the pattern:

| pattern | what it is |
| --- | --- |
| `a "+" b` | infix: something, a word, something |
| `"-" a` | prefix: a word, then something |
| `a "!"` | postfix: something, then a word |
| `"\|" a "\|"` | circumfix: a word on each side |
| `"if" c "then" t "else" f` | mixfix: words and holes interleaved |

A rule that **begins with a hole** continues something already read — it
needs a level, so the tool knows when to apply it. A rule that **begins with
a word** starts something new and does not need one.

`docs/tutorial/03-levels.mx` writes its output in fully bracketed prefix
form, so that the grouping the parser chose is visible:

```
@syntax "(" e ")"               => "{e}"
@syntax a "=" b   10 right      => "(set {a} {b})"
@syntax a "+" b   60            => "(+ {a} {b})"
@syntax a "-" b   60            => "(- {a} {b})"
@syntax a "*" b   70            => "(* {a} {b})"
@syntax a "^" b   80 right      => "(^ {a} {b})"
@syntax "-" a     90            => "(neg {a})"
@syntax a "!"     95            => "(fact {a})"
@syntax "|" a "|"               => "(abs {a})"
@syntax "if" c "then" t         => "(when {c} {t})"
@syntax "if" c "then" t "else" f => "(if {c} {t} {f})"
@end
1 + 2 * 3;
(1 + 2) * 3;
1 - 2 - 3;
2 ^ 3 ^ 2;
a = b = 5;
-x ^ 2;
3! + 1;
|1 - 5| * 2;
if a then if b then 1 else 2
```

```
$ mx docs/tutorial/03-levels.mx
(+ 1 (* 2 3));
(* (+ 1 2) 3);
(- (- 1 2) 3);
(^ 2 (^ 3 2));
(set a (set b 5));
(^ (neg x) 2);
(+ (fact 3) 1);
(* (abs (- 1 5)) 2);
(when a (if b 1 2))
```

Line by line:

- **`1 + 2 * 3`** groups as `1 + (2 * 3)` because `*` at 70 binds tighter
  than `+` at 60. Higher binds tighter. That is the whole of precedence: a
  number per rule, and nothing else.
- **`1 - 2 - 3`** groups to the left, `(1 - 2) - 3`, because `left` is the
  default. **`2 ^ 3 ^ 2`** groups to the right because the rule says `right`.
  So does `=`, which is why `a = b = 5` assigns `5` to `b` and then to `a`.
- **`-x ^ 2`** came out as `(^ (neg x) 2)`. The prefix `-` has level 90, and
  for a prefix rule the level says how tightly its hole is read: it takes
  `x` and stops before the `^`. Had you wanted `-(x ^ 2)`, you would give
  `-` a lower level than `^`. There is no right answer; there is only what the
  file says.
- **`3! + 1`**: a postfix rule, `a "!"`, is an infix rule with nothing after
  the word.
- **`|1 - 5|`** shows a circumfix: the same character opens and closes, and
  the tool does not mind, because it is matching a word at a position, not
  balancing brackets.
- **`if a then if b then 1 else 2`** is the dangling `else`, and it went to
  the inner `if`. The rule is: **when several rules begin with the same word,
  the longest pattern is tried first.** So `if c then t else f` is tried
  before `if c then t`, at both levels of nesting, and the inner `if` gets
  the `else` because it is asked first. Nothing had to be written to make
  that happen.

**How the parser actually works**, in one paragraph, for when a grammar does
something you did not expect. It is a *Pratt parser*, with backtracking. It
reads one thing (a token, or a rule that begins with a word), then looks at
the next token: if some rule that begins with a hole has that token as its
word, and its level is higher than the level the parser is currently reading
at, the rule is applied with what was just read as its first hole. Then it
looks again. When a rule fails part-way through, the tool puts the cursor
back and tries the next candidate. `mx -t file.mx` prints every one of those
attempts; §14 shows it.

---

## 4 · What a hole takes

A hole is a bare name, and by default it takes **one expression** — as much
as the rules can read, up to the next word the pattern is waiting for or the
end of the statement. That is usually what you want, and sometimes it is
exactly wrong.

`docs/tutorial/04-kinds.mx`:

```
@syntax a "=" b   10 right   => "{a} := {b}"
@syntax a "+" b   60         => "{a} + {b}"
@syntax "for" i:name "=" a "to" b "do" s  => "for {i} in {a}..{b}: {s}"
@syntax "print" x            => "print({x})"
@end
for i = 1 to 3 do print i + 1;
total = 0
```

```
$ mx docs/tutorial/04-kinds.mx
for i in 1..3: print(i + 1);
total := 0
```

The `for` rule writes `i:name`: a hole with a **kind**. `name` is one of the
token classes the header declared, and a hole of a class kind takes **exactly
one token of that class** and stops. That is what lets the `=` after it be
this rule's word.

Consider what happens without the kind. `docs/tutorial/04-greedy.mx` is the
same file with `for i "=" …`, a plain hole:

```
$ mx docs/tutorial/04-greedy.mx
mx: docs/tutorial/04-greedy.mx:6: no rule reads 'to' here
```

The plain hole `i` took an expression, and `i = 1` *is* an expression — the
assignment rule made it one. So the hole swallowed `i = 1`, the pattern then
wanted a `=` and found `to`, and the rule failed. **This is the one thing
quoting does not settle by itself**: whether a hole should read an expression
or stop after one token is a decision the pattern has to state.

The kinds:

| kind | takes |
| --- | --- |
| `expr` | one expression. The default. |
| *a class name* | exactly one token of that class, as its source text |
| `stmts` | a sequence of statements, up to the pattern's next word — §5 |
| `block` | an indented run of statements — §11 |
| `text` | raw text, in text mode only — §12 |

One rule about holes follows from how they read. Two plain holes in a row
are refused, because the first would take everything the second wants.
`docs/tutorial/04-adjacent.mx` writes `"pair" a b`:

```
$ mx docs/tutorial/04-adjacent.mx
mx: docs/tutorial/04-adjacent.mx:2: two holes in a row: the first would take everything the second wants
```

Two *class-kind* holes in a row are fine — `"pair" a:number b:number` — since
each takes exactly one token.

---

## 5 · Statements, separators, and `terminated`

`@separator` makes the body a sequence of statements. It has two halves, and
they are about two different languages.

`docs/tutorial/05-statements.mx`:

```
@separator ";" => ";\n"

@syntax a "=" b   10 right               => "{a} = {b}"
@syntax a "+" b   60                     => "{a} + {b}"
@syntax "print" x                        => "printf(\"%d\\n\", {x})"
@syntax "block" b:stmts "end"            => "{{\n{b};\n}}" terminated
@syntax "while" c "do" b:stmts "end"     => "while ({c}) {{\n{b};\n}}" terminated
@end
x = 1;
block
  x = x + 1;
  print x
end
while x do
  x = 0
end
print x
```

```
$ mx docs/tutorial/05-statements.mx
x = 1;
{
x = x + 1;
printf("%d\n", x);
}
while (x) {
x = 0;
}
printf("%d\n", x)
```

**On the way in**, `;` separates statements. A `stmts` hole reads statements
up to the word the pattern is waiting for — here `end` — so `block … end`
holds two of them. And notice that `end` is followed by `while` with no `;`
between: a separator is wanted between two statements, **but not after one
that ended in a word**. That is what lets `end`, or C's `}`, stand on its own
line.

**On the way out**, statements are joined with `;` and a newline. The last
statement of a `stmts` hole is joined after nothing, so the `block` template
writes `{b};` — the `;` its last statement needs — by hand.

**`terminated`** after a template says: *what this rule emits already ends a
statement, so do not put the separator after it.* Both block rules say it,
which is why there is no `;` after the closing braces. This is a statement
about the language being *written*, and it is separate from the rule about
the language being *read*, on purpose: a `}` needs no `;` after it in the C
being read, and might need one in some other language being written. The
tool is not entitled to assume the two agree.

A note on the template strings: `{{` and `}}` are how a string template
writes a literal brace, because a single `{` opens a splice. And `\"` and
`\n` are the only escapes there are, along with `\\`, `\t` and `\r`; the
`printf` template writes a C string by escaping its quotes. That never varies
anywhere in the tool.

---

## 6 · Groups: a part that repeats, or need not be there

`[ … ]` is Metaxis's own bracket. It is outside the strings, so it can never
be confused with a bracket the body writes — one of those would be quoted.

`docs/tutorial/06-groups.mx`:

```
@syntax a "+" b   60                                  => "({a} + {b})"
@syntax f "(" [ x ]* sep "," join ", " ")"   95        => "{f}({x})"
@syntax "let" [ n:name ]+ sep "," join ", "           => "let {n}"
@syntax "get" k:name [ "or" d ]                      => "get({k}, [{d}])"
@end
f();
f(1, 2 + 3, g(4));
let a, b, c;
get x or 5;
get y
```

```
$ mx docs/tutorial/06-groups.mx
f();
f(1, (2 + 3), g(4));
let a, b, c;
get(x, [5]);
get(y, [])
```

Three forms:

| form | matches |
| --- | --- |
| `[ … ]` | the elements inside, once or not at all |
| `[ … ]*` | zero or more times |
| `[ … ]+` | one or more times |

A repeated group can say what separates its turns on the way in (`sep ","`)
and what joins them on the way out (`join ", "`). `join` defaults to whatever
`sep` was.

**The call rule is one rule for every arity.** `f()` has zero arguments and
`f(1, 2 + 3, g(4))` has three, and the same `[ x ]* sep ","` matched both.
Before groups this was a rule per arity, or it was not written.

**Every hole a pattern declares is always bound.** `get y` did not have an
`or` part, and `d` came out as the empty string — `get(y, [])`. A string
template never has to ask whether a part was there, and cannot; a code
template can, with `matched(d)`, and §7 is about that.

---

## 7 · The two kinds of template

Everything so far has used a **string template**: `=> "…"` with `{hole}`
splices. A string template can splice and do nothing else. That is enough
surprisingly often, and when it is not, the file says `=> { … }` instead and
gets a small language of its own.

The difference is easiest to see on one grammar written both ways.
`docs/tutorial/07-string.mx` and `docs/tutorial/07-code.mx` have the same
body:

```
x = 1 + 2 * 3;
y = (1 + 2) * 3;
if x < y then y = 0 else begin x = 0; y = 1 end
```

The string version must bracket every operand, because it cannot know whether
the operand needs it, and must brace every branch, because it cannot know
whether the branch already ends a statement:

```
$ mx docs/tutorial/07-string.mx
x = (1 + (2 * 3));
y = (((1 + 2)) * 3);
if (x < y) { y = 0; } else { {
x = 0;
y = 1;
}; }
```

Correct, and ugly. The code version asks:

```
@syntax a "+" b   60              => { emit group(a, 60) + " + " + group(b, 61) }
@syntax a "*" b   70              => { emit group(a, 70) + " * " + group(b, 71) }
@syntax "begin" b:stmts "end"     => { emit "{\n" + indent(b, 4) + ";\n}" } terminated
@syntax "if" c "then" t "else" f
    => {
        emit "if (" + c + ") " + t
        if not terminated(t) { emit ";" }
        emit " else " + f
    }
```

```
$ mx docs/tutorial/07-code.mx
x = 1 + 2 * 3;
y = (1 + 2) * 3;
if (x < y) y = 0; else {
    x = 0;
    y = 1;
}
```

How to read a code template:

- **`emit`** is the only way out. What the rule produces is everything it
  emitted, in order. `+` joins text.
- **`group(h, n)`** is the hole `h`, bracketed only if the rule that produced
  it binds looser than `n`. `2 * 3` inside a `+` was produced at level 70,
  which is not below 60, so no brackets; `1 + 2` inside a `*` was produced at
  60, which is below 70, so brackets. The right operand asks for one more than
  the left — `group(b, 61)` — which is what keeps `a - (b - c)` bracketed and
  `a - b - c` not.
- **`terminated(h)`** asks whether what filled `h` already ends a statement,
  which is exactly what a rule declares about itself with `terminated`. The
  `if` rule uses it to decide whether the branch needs a `;` before `else`.
- **`indent(s, n)`** moves every line of `s` right by `n` spaces.
- **`if … { … } else { … }`** and **`for … in … { … }`** are the two control
  forms, and there are no others.

Strings inside a code template are ordinary Metaxis strings: `"{"` is a
brace, not a hole. The doubling rule belongs to string templates only.

### 7.1 Lists

A hole inside a repeated group is a **list** in a code template. A string
template only ever sees the turns joined; the code template can walk them,
and — the case that matters — can walk two lists in step.

`docs/tutorial/07-lists.mx`:

```
@syntax "fn" f:name "(" [ p:name ":" t:name ]* sep "," ")"
    => {
        emit f + "("
        if count(p) == 0 { emit "void" }
        for i, x in p sep ", " { emit at(t, i) + " " + x }
        emit ")"
    }
@syntax "sum" [ x ]+ sep ","
    => { emit "0"; for v in x { emit " + " + v } }
@syntax "calc" a "*" b
    => { emit num(a) * num(b) }
@end
fn main();
fn scale(n: int, k: double);
sum 1, 2, 3;
calc 6 * 7
```

```
$ mx docs/tutorial/07-lists.mx
main(void);
scale(int n, double k);
0 + 1 + 2 + 3;
42
```

`p` and `t` are two holes in one group, so they are two parallel lists —
the names and the types — and `for i, x in p` binds the position as well as
the turn so that `at(t, i)` can fetch the matching type. That is what turns
`scale(n: int, k: double)` into `scale(int n, double k)`, and a string
template cannot write it, because it has no way to interleave two lists.

`count(p)` is how many turns there were, and `num(a)` reads a hole's text as
a number so that `*` can multiply rather than join. The last rule computes
instead of writing: a `.mx` file can be a calculator, and `examples/calc.mx`
is one.

### 7.2 Everything a code template can say

There are five kinds of statement, and they are the only things that can stand
on a line of their own inside `{ … }`:

| statement | does |
| --- | --- |
| `emit expr` | Appends text to the rule's output. The only way anything gets out. |
| `if expr { … } else { … }` | Branches. A list is true when it has turns, a number when it is not zero, a text when it is not empty. |
| `for x in h { … }` | Loops over the turns of a repeated hole. `for i, x in h` also binds the position, counting from 0. `sep expr` after the list emits that text between turns. |
| `name(args)` | Calls a `@template`. It emits into the caller and sees only its own parameters. |
| `contribute(name, text)` | Adds one line to a named collection, once per distinct text. §13. |

An expression joins text with `+`, compares with `==`, `!=`, `<`, `<=`, `>`,
`>=`, combines with `and`, `or` and `not`, and does arithmetic with `-`, `*`,
`/` and `%` on numbers. `+` adds when both sides are already numbers and joins
otherwise; the other four want numbers, which is what `num(h)` is for.

And the builtins, which are the functions an expression can call:

| builtin | gives |
| --- | --- |
| `matched(h)` | Whether the optional group holding `h` matched at all. |
| `count(h)` | How many turns a repeated hole took. |
| `at(h, n)` | The turn at position `n`, counting from 0. Out of range is an error, which is how two lists of different lengths get caught. |
| `num(h)` | The hole's text read as a number. All of the text must be the number: `12abc` is an error, not 12. |
| `level(h)` | The level of the rule that filled `h`, or 1000 for a bare token. |
| `terminated(h)` | Whether what filled `h` already ends a statement. For a `stmts` or `block` hole, the last statement answers. |
| `group(h, n)` | `h`, wrapped in parentheses when its level is below `n`. |
| `replace(s, from, to)` | `s` with every `from` replaced by `to`. |
| `drop(s, front, back)` | `s` with that many characters removed from each end. |
| `indent(s, n)` | Every line of `s` moved right by `n` spaces, the first included, and an empty line left empty. |
| `fresh(label)` | A name nobody else has. The same label gives the same name within one application of a rule. §8. |
| `splice(name)` | A placeholder that the second pass replaces with the collection's aggregate. §13. |

Every name in a code template is checked at the `@syntax` that wrote it: a
misspelled builtin, a wrong number of arguments, a hole the pattern does not
have, or a template called where a value was wanted is refused before any body
is read. The same list, with the exact error texts, is REFERENCE §8.3 and
§8.4.

---

## 8 · Fresh names

A template that introduces a temporary variable has a problem: whatever name
it picks might already be in use by the code around it. `{~t}` in a string
template, and `fresh("t")` in a code template, ask for **a name nobody else
has** — one that occurs nowhere in the source and in no template.

`docs/tutorial/08-fresh.mx`:

```
@syntax "swap" a "," b
    => "{{ int {~t} = {a}; {a} = {b}; {b} = {~t}; }}" terminated
@syntax "loop" n "times" b:stmts "end"
    => { emit "for (int " + fresh("i") + " = 0; " + fresh("i") + " < " + n + "; " + fresh("i") + "++) {\n" + indent(b, 4) + ";\n}" } terminated
@end
swap x, y;
swap t, u;
loop 3 times
  swap p, q
end
```

```
$ mx docs/tutorial/08-fresh.mx
{ int t__1 = x; x = y; y = t__1; }
{ int t__2 = t; t = u; u = t__2; }
for (int i__4 = 0; i__4 < 3; i__4++) {
    { int t__3 = p; p = q; q = t__3; };
}
```

Within one use of a rule, every `{~t}` is the same name; the next use gets a
different one. So the second swap, which swaps a variable actually called
`t`, gets `t__2` and nothing collides. And `fresh("i")` said three times in
one template is one name, `i__4`, which is what a loop needs.

Why `i__4` and not `i__3`: the body of the loop was expanded before the loop's
own template ran — a hole is always filled before the template that uses it —
and the swap inside took `t__3` first. The counter is shared, so no two names
from anywhere in the file can ever be the same.

This closes half of a problem called **hygiene**: a template's own temporary
can never capture a caller's name. The other half — a template that reaches
*out* for a name the caller has shadowed — stays open, and notation.md says
why it is a price rather than a bug.

---

## 9 · Saying things once: `@use`, `override`, `@fragment`, `@template`

`docs/tutorial/09-arith.mx` is a library — directives and no body:

```
@token number "[0-9]+"
@token name   "[A-Za-z_][A-Za-z0-9_]*"
@syntax "(" e ")"           => "({e})"
@syntax a "+" b   60        => "({a} + {b})"
@syntax a "*" b   70        => "({a} * {b})"
@syntax a "/" b   70        => "({a} / {b})"
```

`docs/tutorial/09-reuse.mx` takes it, changes one rule, and names two things:

```
@use "09-arith.mx"
@separator ";" => ";\n"

@syntax a "/" b   70   => "div({a}, {b})" override

@fragment args = "(" [ p:name ]* sep "," ")"

@template head(kind, f, p) {
    emit kind + " " + f + "("
    for i, x in p sep ", " { emit "int " + x }
    emit ")"
}

@syntax "proc" f:name @args   => { head("void", f, p) }
@syntax "func" f:name @args   => { head("int", f, p) }
@end
8 / 2 + 1;
proc show(a, b);
func answer()
```

```
$ mx docs/tutorial/09-reuse.mx
(div(8, 2) + 1);
void show(int a, int b);
int answer()
```

- **`@use "path"`** reads another file's directives into this header. The path
  is relative to the file that used it. A file is read once however many
  times it is reached, so two libraries that both use a third do not collide
  with it.
- **`override`** marks a declaration that means to replace an earlier one
  with the same pattern. Without the word, declaring `a "/" b` twice is an
  error naming both lines; with it, the second wins and nothing is said,
  because it was said in the source. `override` with nothing to override is
  also an error, so the word cannot go stale.
- **`@fragment name = pattern`** names a piece of *pattern*, spliced into a
  rule with `@name`. It brings its own holes: both rules above use `p`
  without declaring it, because the fragment did.
- **`@template name(x, y) { … }`** names a piece of *output*. It is called as
  a statement, sees only its parameters, and emits into whoever called it.
  A list — `p` here — goes through a parameter unchanged.

Fragment and template are two mechanics and not one, on purpose: one is
spliced into a pattern at declaration and the other is called at expansion,
and a single word covering both would mean two things.

---

## 10 · One grammar, more than one target

Everything so far has written one output. Suppose the same source is wanted
in two: the same little language, printed once as C and once as Python. The
grammar is the expensive half of a file, and it does not change between those
two — what changes is what a handful of rules *print*. So the tool lets a rule
carry more than one template, and lets the command line say which.

Under a rule, each `=>` is one target's output. A template followed by
`as name` is **tagged**; a template with nothing after it is the **default**.
`mx -b name` picks: for every rule, the template tagged `name` if the rule has
one, and the default if it does not. Plain `mx` picks the default everywhere.
The grammar is read once either way, and the parse is the same; only the
output side listens to `-b`.

`docs/tutorial/10-backends.mx`:

```
@syntax a "=" b   10 right   => "{a} = {b}"
@syntax a "+" b   60         => "{a} + {b}"
@syntax "print" x            => "printf(\"%d\\n\", {x})"
                             => "print({x})" as python
@end
x = 1 + 2;
print x
```

```
$ mx docs/tutorial/10-backends.mx
x = 1 + 2;
printf("%d\n", x)
```

```
$ mx -b python docs/tutorial/10-backends.mx
x = 1 + 2;
print(x)
```

Read the two outputs against the header. The `=` and `+` rules have one
template each and printed the same line both times: `-b python` asked them
for a `python` template, they had none, and the default was used. Only
`print` has a second template, and only `print` changed. **That fallback is
the whole point**: a file that adds a target rewrites the rules that differ
and none of the ones that do not.

Three things around the flag are worth knowing before you lean on it.

**Naming a target nothing emits is an error**, not a quiet run of the
defaults, and the message says what the file does emit:

```
$ mx -b ruby docs/tutorial/10-backends.mx
mx: no rule emits 'ruby' -- this file declares python
```

**A file whose every template is tagged has no default.** Run it without `-b`
and the tool refuses, naming the rule, rather than letting the first tag win
by being written first — position decides nothing in a header, and this is
not the place to start. `docs/tutorial/10-tagged.mx` tags both of its
templates:

```
@syntax "print" x   => "printf(\"%d\\n\", {x})" as c
                    => "print({x})" as python
@end
print 7
```

```
$ mx docs/tutorial/10-tagged.mx
mx: docs/tutorial/10-tagged.mx:8: every template here is tagged, so there is no default -- name one with '-b <name>'. This file emits: c, python
```

```
$ mx -b c docs/tutorial/10-tagged.mx
printf("%d\n", 7)
```

**`terminated` belongs to the template, not the rule** (§5), and two targets
are why. `examples/backends.mx` has an `if` whose default template braces
its branch, so what it prints already ends a statement, and whose `tight`
template prints a bare statement that still needs the separator's semicolon.
One rule, two templates, and only one of them is `terminated`. That example
is the fuller version of this section: one C grammar, one target that
brackets every operand and one that brackets only where precedence needs it,
with the rules the two agree on written once and untagged.

`mx -g` lists what a header declared, tags first, so you can see a file's
targets without reading its rules:

```
$ mx -g docs/tutorial/10-backends.mx
backend    python
mode       expression
separator  declared
token      number   [0-9]+
token      name     [A-Za-z_][A-Za-z0-9_]*
words      'print' '+' ';' '='
infix     a "=" b [10 right]
infix     a "+" b [60]
prefix    "print" x
```

That listing is the first thing to look at when a grammar does not do what
you meant: it shows the words the lexer will look for, and which rules the
tool actually built.

---

## 11 · Blocks that are indentation

Everything so far has delimited a block with words: `begin … end`,
`{ … }`. Python delimits with indentation, and an indent is not text anybody
wrote, so it cannot be quoted. It is the one delimiter in the notation that is
not a string, and it is spelled outside the quotes, the way Metaxis spells its
own vocabulary.

`docs/tutorial/11-blocks.mx`:

```
@separator "\n" => ";\n" indent

@syntax n:name "=" v   5 right     => { emit n + " = " + v }
@syntax a "+" b   60               => { emit a + " + " + b }
@syntax a "<" b   40               => { emit a + " < " + b }
@syntax "print" x                  => { emit "printf(\"%d\\n\", " + x + ")" }

@template braces(b) {
    emit "{\n" + indent(b, 4)
    if not terminated(b) { emit ";" }
    emit "\n}"
}
@syntax "while" c ":" b:block
    => { emit "while (" + c + ") "; braces(b) } terminated
@syntax "if" c ":" b:block "else" ":" e:block
    => { emit "if (" + c + ") "; braces(b); emit " else "; braces(e) } terminated
@end
x = 0
while x < 3:
    x = x + 1
    if x < 2:
        print x
    else:
        print 0
print x
```

```
$ mx docs/tutorial/11-blocks.mx
x = 0;
while (x < 3) {
    x = x + 1;
    if (x < 2) {
        printf("%d\n", x);
    } else {
        printf("%d\n", 0);
    }
}
printf("%d\n", x)
```

Two things make it work. **`@separator "\n" … indent`**: the separator is a
newline, and `indent` tells the lexer to keep a stack of columns and produce
an `indent` token where a line is deeper than the one before, and one
`dedent` per level where it is shallower. **`b:block`** is the hole that reads
them: an `indent`, the statements, and the `dedent` that closes them. It owns
both of its delimiters, which is why `else` can follow it with nothing in
between, and why a nested block is consumed before the enclosing one sees its
`dedent`.

The `braces` template shows a rule that comes up in every block-structured
output: the separator goes *between* statements and never after the last, so
the block's last statement needs its own `;` — unless it is itself a
`terminated` rule, in which case it must not have one. `terminated(b)` asked
of a `stmts` or `block` hole answers for the last statement in it.

---

## 12 · Text mode: rewrite what you know, pass the rest through

Everything above is **expression mode**: the whole body is parsed, and
anything the rules cannot read is an error. `@mode text` is the opposite. The
body is *scanned*; where a rule matches, it fires; everything else is copied
through unchanged.

`docs/tutorial/12-text.mx`:

```
@mode text
@syntax "**" t:text "**"              => "<b>{t}</b>"
@syntax "[[" t:text "|" u:text "]]"   => "<a href=\"{u}\">{t}</a>"
@syntax "[[" t:text "]]"              => "<a href=\"{t}\">{t}</a>"
@syntax "TODO(" who:text "):" note:text "\n"  => "<!-- {who}:{note} -->\n"
@end
Some **bold** words and a [[link|http://x.y]] and a bare [[http://z]].
TODO(hans): read the reference
Nothing else here is touched: {braces}, "quotes", `backticks`.
```

```
$ mx docs/tutorial/12-text.mx
Some <b>bold</b> words and a <a href="http://x.y">link</a> and a bare <a href="http://z">http://z</a>.
<!-- hans: read the reference -->
Nothing else here is touched: {braces}, "quotes", `backticks`.
```

In text mode there are no tokens: a hole is a run of characters, and takes
the *shortest* run that lets the rest of the pattern match. `t:text` in the
`**` rule stops at the next `**`. Only rules that begin with a word apply,
since there is nothing for an infix rule to continue.

Because everything unmatched passes through, text mode can be pointed at a
**real file** and asked to change two things in it. `docs/tutorial/12-island.mx`
rewrites one C construct in a fragment of C and leaves the rest, comments
included:

```
$ mx docs/tutorial/12-island.mx
/* a fragment of a C file */
static void complain(const char *e) { fprintf(stderr, "mx: %s\n", e); }

static void usage(void)
{
    if (!src) { complain(err); return 1; }
    fprintf(stderr, "mx: cannot write %s\n", outpath);
}
```

The `fprintf` with a different format string was left alone, because the
rule's pattern is the whole call up to the argument. Where did the definition
of `complain` come from? That is §13.

---

## 13 · Collections: a head that depends on the whole body

A rule sees its own holes and nothing else. That is what keeps rules
independent, and it has one honest cost: some output has a *head* that
depends on *all* of the body. C wants every variable declared before the
first statement; an `#include` is only needed if something prints; a helper
is only needed if something calls it. No single rule can know.

A **collection** is how a rule says what *it* would add to such a head,
without seeing anyone else's contribution.

`docs/tutorial/13-collections.mx`:

```
@syntax "program" n:name
    => { emit "/* " + n + " */\n" + splice("head") + "\nint main(void) {\n    " + splice("vars") } terminated
@syntax "let" v:name "=" e
    => { contribute("vars", "int " + v + ";"); emit v + " = " + e }
@syntax "print" x
    => { contribute("head", "#include <stdio.h>"); emit "printf(\"%d\\n\", " + x + ")" }
@syntax "end"
    => { emit "return 0;\n}" } terminated
@end
program demo;
let a = 1;
let b = 2;
let a = 3;
print a;
end
```

```
$ mx docs/tutorial/13-collections.mx
/* demo */
#include <stdio.h>
int main(void) {
    int a;
    int b;
a = 1;
b = 2;
a = 3;
printf("%d\n", a);
return 0;
}
```

- **`contribute("vars", text)`** is a statement, beside `emit`. It adds a
  line to the collection called `vars`. A collection keeps **one copy of each
  distinct line, in the order first seen**: `a` was contributed twice and
  declared once.
- **`splice("vars")`** is an expression that leaves a placeholder. The
  `program` rule ran first, before any `let` had contributed anything, so the
  aggregate could not be known yet. Once the whole body has been read, a
  second pass replaces each placeholder with its collection. Lines after the
  first are given the placeholder's indentation, which is why `int b;` is
  indented like `int a;`.
- **The pass reads nothing between the placeholders.** It knows nothing about
  C. That is what keeps the tool agnostic while giving it the one piece of
  context every code generator eventually needs.

What if the source has no head at all? BASIC's first line is a statement;
there is nowhere for a `program` rule to splice. Then a collection nothing
splices **goes at the start of the output**. `docs/tutorial/13-headless.mx`:

```
@syntax "let" v:name "=" e
    => { contribute("vars", "int " + v + ";"); emit v + " = " + e }
@end
let a = 1;
let b = a;
let a = 2
```

```
$ mx docs/tutorial/13-headless.mx
int a;
int b;
a = 1;
b = a;
a = 2
```

`examples/basic.mx` is the real thing: a BASIC program whose C declarations
come from the LET and FOR statements that mention the variables, and
`tests/basic.sh` compiles the result with nothing supplied but `main`.

---

## 14 · When it goes wrong: `-g`, `-t`, and the errors

Every error names the file and line, and says what it wanted in a sentence.
Three you will meet early, from `docs/tutorial/14-*.mx`:

```
$ mx docs/tutorial/14-wrong-level.mx
mx: docs/tutorial/14-wrong-level.mx:2: a rule that begins with a hole is infix or postfix and needs a level
```

```
$ mx docs/tutorial/14-no-hole.mx
mx: docs/tutorial/14-no-hole.mx:2: the template splices '{b}' and the pattern has no such hole
```

Both of those are caught at the `@syntax` that wrote them, before any body is
read. The third is a body error, and it names the furthest token the parser
reached:

```
$ mx docs/tutorial/14-unread.mx
mx: docs/tutorial/14-unread.mx:4: no rule reads '+' here
```

When the header is accepted and the body does something unexpected, two
flags show what the tool actually did.

**`mx -g`** prints the grammar the header built — the token classes, every
word the lexer will look for, and every rule with its shape and level. §10
shows one. If a word you expected is missing from the `words` line, no rule
quoted it; if a rule shows as `prefix` when you meant infix, its pattern
begins with a word.

**`mx -t`** traces the parse to standard error: one line per candidate rule
tried, indented by depth, and `ok` or the token it could not get past. The
expansion still goes to standard output, so the two can be separated:

```
$ mx -t docs/tutorial/03-levels.mx 2>&1 >/dev/null | head -4
   try 1/1  a "+" b  [docs/tutorial/03-levels.mx:11]
     try 1/1  a "*" b  [docs/tutorial/03-levels.mx:13]
      ok
    ok
```

That is `1 + 2 * 3`: the `+` rule was tried, and while reading its right
operand the `*` rule was tried and succeeded, so `2 * 3` became the operand.
When a grammar groups something the way you did not intend, this is where the
answer is.

---

## 15 · Where to go from here

The examples in `examples/` are each a complete translator, and every one is
run by `make check` against a recorded output or, for five of them, compiled
and executed:

- `first.mx` and `tour.mx` — the shape, and every fixity in one file.
- `pascal.mx` and `code.mx` — one Pascal grammar, string templates and code
  templates, with the diff between their outputs as the argument.
- `asm.mx` — C in, arm64 out: a target that is a sequence, not a tree.
- `python.mx` — blocks by indentation, run under `python3` as well as
  compiled as C.
- `basic.mx` — a source that declares nothing, and collections.
- `lib/island.mx` with `tests/island.sh` — the tool rewriting its own front
  end in text mode.
- `poem.mx`, `groups.mx`, `use.mx`, `backends.mx`, `hygiene.mx`, `calc.mx`,
  `reserved.mx`, `clike.mx` — one concept each.

[REFERENCE.md](REFERENCE.md) is the complete statement of what every part
means, and is organised the way this page is. [notation.md](notation.md) is
why it is shaped like this and what that costs, which is the page to read
when something here seems arbitrary; usually it is not, and that page says
what the alternative would have cost.
