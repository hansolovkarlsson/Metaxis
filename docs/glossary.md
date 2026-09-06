# Glossary, and the concepts behind the words

*This is the **glossary**: every term of art the other documents lean on,
explained for a reader who does not already know it. The first half is a
short account of the concepts in the order they depend on each other —
what a grammar is, how a file is turned into tokens, how tokens are turned
into structure, and what this tool does with that structure — because the
words only make sense against that picture. The second half is the words,
alphabetically, each with a plain definition and one line on how it shows up
in Metaxis. This page explains and does not define behaviour:
[REFERENCE.md](REFERENCE.md) does that, [tutorial.md](tutorial.md) teaches
it with runnable files, and where this page and either of those disagree,
they are right.*

---

## Part one · The concepts, in the order they depend on each other

### A language is a set of strings, and a grammar is a finite way to say which

Take every possible piece of text. Some are Pascal programs and most are not.
The set that are is "the Pascal language", and it is infinite, so nobody can
list it; a **grammar** is a finite set of rules that generates exactly that
set. `an expression is a term, or an expression followed by "+" and a term`
is such a rule. Written in a notation like BNF or EBNF, a few dozen of them
describe a whole language.

Rules like that have two kinds of symbol in them. A **terminal** is a piece
of actual text — `+`, `if`, a number. A **nonterminal** is a name for a
category — *expression*, *statement* — that the rules define in terms of
other categories and terminals. A grammar's job is to say how nonterminals
are built from terminals, and reading a text with a grammar means working out
which rules produced it.

Metaxis does not have a separate grammar file. A `.mx` file's header *is* its
grammar, written as `@syntax` rules, and each rule's pattern is one way an
expression can be spelled. The nonterminals are not named: there is one
category, *expression*, and every hole is one.

### Lexing: text becomes tokens

Before structure is looked for, text is cut into **tokens**: the smallest
pieces that mean something on their own. `total := total + 1` is five
tokens, not seventeen characters. The part of a tool that does this is the
**lexer** (also *scanner* or *tokenizer*). It usually works by trying a set
of patterns at the current position and taking the longest one that matches
— **leftmost-longest**, the rule POSIX regular expressions follow, which is
why `0x0c` is read as one number and not as `0` followed by junk.

Some tokens are members of a **class** — every identifier is a *name*, every
run of digits a *number* — and some are individual **words** like `+` or
`if`. Most languages hardcode which is which and call the words *keywords*.
Metaxis makes every class a `@token` declaration and every word a quoted
string in some rule, and when a piece of text could be either, **the class
wins**; so `if` is a name token whose text happens to be a word, and no word
is ever reserved (REFERENCE §6.1).

Some languages carry structure in whitespace. Python's blocks are marked by
indentation — the **off-side rule** — and a lexer for it has to notice when a
line is deeper or shallower than the one before and produce **indent** and
**dedent** tokens that no character in the file spells. Metaxis does this
under `@separator "\n" indent`, and a `block` hole is what reads them.

### Parsing: tokens become structure

A **parser** takes the token stream and finds the structure the grammar says
is there. The structure is a tree: `1 + 2 * 3` is a `+` node whose right
child is a `*` node. That tree is usually called an **abstract syntax tree**,
or AST — *abstract* because it drops the tokens that only served to show
structure, like brackets.

There are families of parser, and the words for them come up in
[prior-art.md](prior-art.md) and [direction.md](direction.md):

- **Top-down** parsers start from the whole (*a program*) and predict what
  must come next. **Recursive descent** is the hand-written form: one
  function per nonterminal. **Bottom-up** parsers start from the tokens and
  build upward, recognising a rule once all of its pieces have been seen.
  Metaxis is bottom-up in spirit: a rule's output is computed from its holes'
  outputs, which were computed first.
- **CFG** means *context-free grammar*: the classic kind, where a rule says
  what a nonterminal can be regardless of what surrounds it. **LR**, **LALR**
  and tools like *yacc* and *bison* take a CFG and generate a table-driven
  bottom-up parser from it. Most textbook compilers work this way. Such a
  grammar can be **ambiguous** — allow two different trees for one text — and
  the generator will complain about it.
- **PEG** means *parsing expression grammar*: it looks like a CFG but its
  choice operator is **ordered** — try the first alternative, and only if it
  fails try the second — so there is never ambiguity, only a first match.
  PEGs are usually run by **packrat** parsers, which remember every partial
  result to stay fast. Metaxis is PEG-shaped in one respect: when several
  rules could apply, they are tried in an order (longest pattern first) and
  the first to succeed wins.
- A **Pratt parser** (Vaughan Pratt, 1973, also *top-down operator
  precedence*) is the family Metaxis actually belongs to. It is built for
  expressions with operators, and it has one idea: every operator has a
  **binding power**, a number, and a parse proceeds by repeatedly asking *is
  the next operator strong enough to take what I have just read as its left
  operand?* Two kinds of rule exist. A **nud** (*null denotation*) starts an
  expression from nothing: a literal, a prefix operator, a bracketed group,
  `if`. A **led** (*left denotation*) continues an expression that has
  already been read: an infix or postfix operator. In Metaxis a pattern that
  begins with a word is a nud rule and one that begins with a hole is a led
  rule, and the level after the pattern is its binding power (REFERENCE §4.1,
  §5, §6.2).

Whichever family, a parser sometimes commits to a rule and finds it does not
fit. **Backtracking** is putting the cursor back and trying the next
candidate. Some parsers avoid it with **lookahead** — peeking at the next
token or two before choosing. Metaxis backtracks, and `mx -t` shows every
attempt.

**Precedence** is which operator wins when two compete for an operand:
`1 + 2 * 3` is `1 + (2 * 3)` because `*` has higher precedence.
**Associativity** is which way equal operators group: `1 - 2 - 3` is
`(1 - 2) - 3` because `-` is *left*-associative, and `2 ^ 3 ^ 2` is
`2 ^ (3 ^ 2)` because `^` is *right*. **Fixity** is where an operator sits
relative to its operands: *prefix* before (`-x`), *postfix* after (`n!`),
*infix* between (`a + b`), *circumfix* around (`|x|`), *mixfix* interleaved
(`if c then t else f`). The **dangling else** is the classic ambiguity of
`if a then if b then x else y` — which `if` owns the `else`? — and every
parser needs a rule for it; in Metaxis it falls out of *longest pattern
first*.

### Expansion: structure becomes text

A compiler turns the tree into machine code; an interpreter walks it and does
what it says. Metaxis does a third thing: it turns the tree back into *text*,
a piece at a time, by running each rule's **template** on the text its holes
produced. That is a **rewriter**, or a **transpiler** when the output is
another programming language. The output for a node is computed from the
outputs of its children, so the value flows *upward* through the tree — a
property with a name, below.

Because the hole's output is text, the template cannot always tell what it
was. `group(h, n)` exists so a template can ask *what level was this produced
at* and bracket it only if needed; `terminated(h)` so it can ask *does this
already end a statement*. Those two questions are all a Metaxis rule can ask
about its children, and [notation.md](notation.md) says why that is the
price of not knowing the output language.

### Attribute grammars: values that ride on the tree

An **attribute grammar** is a grammar in which every node can carry values —
*attributes* — computed by rules attached to the grammar's productions. A
**synthesised** attribute is computed from a node's children, so it flows
upward: the value of an expression, the text a rule emits. An **inherited**
attribute is passed down from a node's parent: the current scope, the
expected type. Metaxis is a bottom-up attribute grammar with exactly one
synthesised attribute, whose type is text; it has no inherited attributes at
all, and that is what "it has no context" means when the documents say it.

A **collection attribute** (from JastAdd and Silver) is a third kind: any
node anywhere may *contribute* a value to a named collection, and the
collection's value is the aggregate of every contribution. It is still
synthesised — nothing flows down — which is why Metaxis could add it without
gaining context. `contribute` and `splice` are that (REFERENCE §8.4).

### Context, and the walls

**Context** is anything a rule would need to know that is not in its own
pattern: what type `x` was declared with, whether `Banner` is a procedure or
a variable, whether `x` is a typedef so that `x * y` is a declaration. A
**symbol table** is the data structure a compiler keeps for that — a map from
names to what is known about them — and building one is what makes a
compiler front end for a real language a big job. A **context-sensitive**
language is one that cannot be parsed without it, and C is one. Metaxis has
no symbol table by decision, and ROADMAP item 1 records the three places the
Pascal translator hit that wall.

### Hygiene: whose name is it

A **macro** is a rule that rewrites one piece of code into another before it
is compiled — C's `#define`, Lisp's `defmacro`, and every Metaxis rule. The
classic macro bug: the macro introduces a temporary called `t`, the caller
already has a `t`, and the two collide. A **hygienic** macro system keeps a
macro's names apart from the caller's automatically. Metaxis closes the half
of this where the template *introduces* a name — `{~t}` and `fresh("t")`
give a **fresh name**, one that appears nowhere else — and leaves open the
half where a template *reaches out* for a name the caller has shadowed,
because a text template cannot see a scope. `examples/hygiene.mx` shows both
halves and `tests/hygiene.sh` runs them.

### Islands: reading only what you declare

An **island grammar** describes just the constructs you care about and treats
everything else as water to pass through. Tools like Comby and Coccinelle
work on real C and thirty other languages this way: they know brackets,
strings and comments, and nothing else about any language, which is exactly
why they work on all of them. TXL calls the same idea **agile parsing**.
Metaxis's text mode is an island grammar, and stage 5 pointed it at the
tool's own source; ROADMAP item 7 is what it would take to give it the three
things Comby knows.

### Two more ideas the direction page uses

A **stack machine** is a computer, real or imagined, whose instructions take
their operands from a stack and push their results back; it is the easiest
target to generate code for, because every expression becomes *push the
operands, then apply*. `examples/asm.mx` compiles to arm64 by treating it as
one.

The **Futamura projections** (Yoshihiko Futamura, 1971) are three
observations about **partial evaluation**, which is specialising a program
to some of its inputs. Specialise an interpreter to a program and you get a
compiled program; specialise the specialiser to the interpreter and you get
a compiler; do it once more and you get a compiler generator.
[direction.md](direction.md) uses them to say what Metaxis is *not*: it is
the kind of definitional interpreter such a process would consume, not the
process.

---

## Part two · The words

**Agile parsing.** TXL's name for extending a base grammar with overrides so
that input the grammar does not explain is passed through instead of
rejected. See *island grammar*. — prior-art.md §3.3.

**Ambiguity.** When a grammar allows more than one tree for one text. CFG
tools report it; PEG and Pratt parsers resolve it by order. In Metaxis, two
rules with the same pattern are refused unless one says `override`, and rules
sharing a leading word are tried longest first. — REFERENCE §3.10, §6.2.

**Anchored.** A regular expression matched only at the current position, not
searched for. Every `@token` pattern is anchored, which is why `$` in one
means *the edge of the window the lexer is looking at*. — REFERENCE §3.1.

**Arity.** How many operands or arguments something takes. A group
`[ x ]* sep ","` is one rule for every arity. — REFERENCE §4.4.

**Associativity.** Which way equal operators group. `left` is the default;
`right` after a level makes `a ^ b ^ c` mean `a ^ (b ^ c)`. — REFERENCE §5.

**AST, parse tree.** The tree structure a parser finds. Metaxis never
materialises one: each rule's output is computed as the rule matches, and
only text flows up. — direction.md.

**Attribute grammar.** A grammar whose nodes carry computed values. Metaxis
is one with a single synthesised attribute, text. — direction.md, Part one.

**Backend.** The half of a translator that writes the output language, as
opposed to the *front end* that reads the input. In Metaxis a rule may have
one template per backend, tagged `as name`, and `mx -b name` picks. —
REFERENCE §3.4, §9.

**Backtracking.** Putting the parse cursor back after a rule fails and
trying the next candidate. Every candidate under one leading word is tried
this way, and `mx -t` shows it. — REFERENCE §6.2.

**Binding power.** A Pratt parser's number for how tightly an operator
holds its operands; the level after a pattern. Higher binds tighter. —
REFERENCE §5.

**Body.** The second half of a `.mx` file: text in the language the header
declared. Begins after `@end`, or at the first line that is not a directive.
— REFERENCE §2.2.

**Bottom-up.** Building structure from the tokens upward, recognising a
rule once its pieces are all present; also, computing a node's value from
its children's. Metaxis is bottom-up in both senses.

**Budget.** A cap on how much work a search may do before it is declared
stuck. Text mode has one — 200,000 match attempts per rule. Expression mode
has a recursion depth cap and no budget; `mx -t` counts what a parse tried,
which is the measurement a budget would be picked from, and the item that
wanted one settled against itself once that count existed. — REFERENCE §7,
§9, §11; notation.md "Not done".

**CFG, context-free grammar.** The classic grammar formalism, where each
rule defines a nonterminal without reference to what surrounds it. Yacc,
bison and ANTLR take one. Metaxis does not use one; its rules are Pratt
rules. — Part one.

**Circumfix.** An operator with a word on each side of its operand, like
`|x|`. Written `"|" a "|"`. — REFERENCE §4.1.

**Class.** A kind of token declared by `@token`: *number*, *name*, *string*.
A hole may ask for exactly one token of a class with `x:name`. — REFERENCE
§3.1, §4.3.

**Code template.** A rule's output written as `=> { … }`: a small language
with `emit`, `if`, `for`, template calls and builtins. — REFERENCE §8.3;
tutorial §7.

**Collection, collection attribute.** A named aggregate that any rule may
add a line to with `contribute`, and that one rule splices with `splice`,
filled in by a second pass. — REFERENCE §8.4; tutorial §13.

**Comby, Coccinelle, TXL, Stratego, JastAdd, Silver, Nanopass.** Tools the
survey compares Metaxis against: the first two rewrite real code by knowing
only brackets, strings and comments; TXL is a grammar-based transformer with
agile parsing; Stratego a term rewriter with concrete syntax; JastAdd and
Silver are attribute-grammar systems with collection attributes; Nanopass a
framework for compilers written as many small passes. — prior-art.md.

**Context.** Anything a rule would need to know that is not in its own
pattern. Metaxis has none by decision. — Part one; direction.md.

**Dangling else.** Which `if` owns the `else` in `if a then if b then x
else y`. Resolved here by trying the longer pattern first. — REFERENCE §6.2.

**Dedent, indent.** Tokens the lexer produces from indentation under
`@separator … indent`, read by a `block` hole. The only tokens no file
spells. — REFERENCE §3.3, §6.1.

**Directive.** A line beginning with `@` in the header: `@token`,
`@syntax`, and the rest. — REFERENCE §3.

**Expansion.** Running the templates over what the parser found; what
Metaxis calls the step that produces output. — REFERENCE §6, §8.

**Expression mode.** The default mode: the body is fully parsed and
anything no rule reads is an error. — REFERENCE §6.

**Fixity.** Where an operator sits relative to its operands: prefix, infix,
postfix, circumfix, mixfix. Read off the pattern, never declared. —
REFERENCE §4.1.

**Fragment.** A named piece of *pattern*, `@fragment`, spliced into rules
with `@name`. Contrast *template*. — REFERENCE §3.9.

**Fresh name.** A name guaranteed to occur nowhere in the source or in any
template: `{~t}` or `fresh("t")`. — REFERENCE §8.2.

**Front end.** The half of a translator that reads the input: lexer and
parser. `metaxis/cmd/mx.c` is the tool's own front end in the narrower
sense of the program that takes the command line.

**Futamura projections.** Three results about partial evaluation that turn
an interpreter into a compiler. — Part one; direction.md.

**Grammar.** A finite set of rules describing a language. In Metaxis, the
header of the file being read. — Part one.

**Header.** The first half of a `.mx` file: the directives. Ends at `@end`
or at the first non-directive line. — REFERENCE §2.1, §2.2.

**Hole.** A bare name in a pattern, filled by the body's text and spliced
back out by the template. — REFERENCE §4, §4.3.

**Hygiene.** Keeping a macro's names from colliding with its caller's. Half
closed here by fresh names. — Part one; notation.md.

**Infix.** An operator between its operands. A pattern beginning with a
hole, `a "+" b`; a led rule. — REFERENCE §4.1.

**Inherited attribute.** A value passed down the tree from parent to child.
Metaxis has none. — Part one.

**Interpreter generator.** A tool that produces an interpreter from a
language description. direction.md discusses whether Metaxis is one and
concludes it is not, because a rule cannot leave a hole unevaluated. —
COMPLETED.md, direction.md.

**Island grammar.** A grammar for only the constructs you care about, with
everything else passed through. Text mode is one. — Part one; prior-art.md
§3.3; ROADMAP 7.

**Kind.** What a hole takes: `expr`, `stmts`, `text`, `block`, or a class.
— REFERENCE §4.3.

**Led rule.** *Left denotation*: a Pratt rule that continues an expression
already read. A pattern beginning with a hole; needs a level. — REFERENCE
§4.1, §6.2.

**Leftmost-longest.** POSIX's rule for regular expressions: of the matches
starting at the current position, take the longest. — REFERENCE §3.1.

**Level.** The number after a pattern: its binding power. — REFERENCE §5.

**Lexer.** The part that cuts text into tokens. Metaxis builds one from the
`@token` classes and every quoted word in every rule. — Part one; REFERENCE
§6.1.

**Lookahead.** Peeking at coming tokens before choosing a rule. Metaxis
does not; it backtracks instead.

**LR, LALR.** Families of table-driven bottom-up parsers generated from a
CFG; yacc produces LALR parsers. — Part one.

**Macro.** A rule that rewrites code before it is compiled. Every Metaxis
rule is one, over text. — Part one.

**Mixfix.** An operator whose words and operands interleave: `if c then t
else f`. — REFERENCE §4.1.

**Mode.** Expression mode or text mode, `@mode`. — REFERENCE §3.6.

**Nonterminal.** A category name in a grammar — *expression*, *statement*.
Metaxis has one, unnamed: every hole is an expression. — Part one.

**Nud rule.** *Null denotation*: a Pratt rule that starts an expression from
nothing. A pattern beginning with a word; needs no level. — REFERENCE §4.1,
§6.2.

**Off-side rule.** Blocks marked by indentation, as in Python. — Part one;
REFERENCE §3.3.

**Override.** The word that says a second declaration of the same thing is
meant to replace the first. — REFERENCE §3.10.

**Packrat.** A PEG parser that memoises every partial result. — Part one.

**Parser.** The part that turns tokens into structure. Metaxis's is a Pratt
parser with backtracking. — Part one; REFERENCE §6.2.

**Partial evaluation.** Specialising a program to some of its inputs. — see
*Futamura projections*.

**Pattern.** The left side of a rule: words, holes, groups, in order. —
REFERENCE §4.

**PEG.** Parsing expression grammar: a grammar with ordered choice, hence
never ambiguous. — Part one.

**Placeholder.** What `splice` leaves in the output for the second pass to
replace; a fresh name, so it can collide with nothing. — REFERENCE §8.4.

**Postfix.** An operator after its operand, `n!`. A led rule with nothing
after its word: `a "!"`. — REFERENCE §4.1.

**Pratt parser.** The parser family Metaxis belongs to: operator-precedence
parsing driven by binding powers, with nud and led rules. — Part one;
REFERENCE §6.2.

**Precedence.** Which operator wins an operand; here, the level. — Part one;
REFERENCE §5.

**Prefix.** An operator before its operand, `-x`. A nud rule with a hole
after its word: `"-" a`. — REFERENCE §4.1.

**Proto.** The sibling project this one is *in the shape of*: a tool that
also reads a file declaring its own grammar, whose working tree at `../Proto`
is read-only from here. REFERENCE §12 lists where the two differ.

**Phoenix.** A sibling project on the other side: an EBNF, top-down
recogniser that makes a compiler. direction.md places Metaxis beside it.

**Quadratic.** Work that grows with the square of the input size — double
the input, four times the time. POSTMORTEM 18 records one, and `tests/scale.sh`
exists to notice another.

**Recursive descent.** Hand-written top-down parsing, one function per
nonterminal. — Part one.

**Regular expression, POSIX ERE.** A pattern language for text; ERE is the
*extended* POSIX dialect, the one `@token` uses, matched anchored and
leftmost-longest. — REFERENCE §3.1.

**Rewriter, transpiler.** A program that turns text in one language into
text in another. What Metaxis is. — Part one.

**Scanner, tokenizer.** Other names for the lexer.

**Self-hosting.** A tool applied to its own source. Stage 5 is Metaxis
rewriting its own front end. — COMPLETED.md.

**Separator.** What separates statements on the way in and joins them on
the way out, `@separator`. — REFERENCE §3.3, §6.3.

**Solveig.** A Smalltalk-shaped language Proto was built to write;
`examples/tour.mx` and `examples/clike.mx` emit it so that a comparison with
Proto's own examples is direct.

**Source map.** A file recording which input line produced each output
line, so a downstream error can point at what somebody wrote. Not built;
ROADMAP 5.

**Stack machine.** A machine whose instructions take operands from a stack.
The model `examples/asm.mx` generates code for. — Part one.

**String template.** A rule's output written as `=> "…"` with `{hole}`
splices. Can splice and do nothing else. — REFERENCE §8.1.

**Symbol table.** A compiler's map from names to what it knows about them.
Metaxis has none. — Part one; ROADMAP 1.

**Synthesised attribute.** A value computed from a node's children, flowing
upward. Metaxis's one attribute. — Part one.

**Template.** The right side of a rule: what it emits. A string template or
a code template. Also `@template`, a *named* piece of output. — REFERENCE
§8, §3.8.

**Terminal.** A piece of actual text in a grammar rule; in Metaxis, a
quoted word or a token class. — Part one.

**Terminated.** A rule's declaration that its output already ends a
statement, so no separator follows it; also the builtin that asks a hole the
same. — REFERENCE §3.4, §6.3, §8.3.

**Text mode.** `@mode text`: the body is scanned, rules fire where they
match, everything else passes through. — REFERENCE §7.

**Token.** The smallest unit of text that means something on its own; what
the lexer produces. — Part one; REFERENCE §6.1.

**Top-down.** Parsing from the whole toward the tokens, predicting what must
come next. — Part one.

**Transcript.** A `$ mx …` line in a document with the output beneath it.
Every one in `docs/` is run by `tests/docs.sh`. — COMPLETED.md.

**Word.** In Metaxis, a quoted string in a pattern: a literal the body must
contain. Never reserved, because the class wins a tie. — REFERENCE §4, §6.1.
