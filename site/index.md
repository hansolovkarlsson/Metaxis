# A file that declares its own language, then uses it

Metaxis is a rewriter. A `.mx` file has two halves: a **header** of
directives that say what the second half's syntax is and what to turn it
into, and a **body** written in that syntax. The tool reads the header,
builds a reader for the language it describes, reads the body with it, and
prints what the templates say. Nothing is built in, not `+`, not `if`, not
that a number is a number, so the file that declares `+` could have declared
anything else, and the tool never quietly prefers the language it was written
in.

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
$ mx examples/first.mx
x = 1;
(add(x, 2) * 2)
```

## One rule

**Everything a directive says about foreign text is inside a string.** A
quoted word in a pattern is text the body must contain; a quoted template is
text to write. Everything outside the quotes is Metaxis's own fixed
vocabulary, which no file can change: a directive name, a hole, a level, a
bracket. That is what lets a directive *mention* `if` without *being* an `if`
statement, and it is why a `.mx` file can read a language with keywords
without ever having a keyword list: `then` is a word where a rule quoted it
and a name everywhere else, by position.

## What it has been pointed at

Five translators, each taken far enough to be compiled and run by the test
suite, and each picked because it would ask the tool for something:

- **Pascal → C**, the mechanics, checked by a C compiler and a number.
- **C → arm64 assembly**, a target that is a sequence rather than a tree.
- **Python → C**, a language whose blocks are indentation; the output is run
  under `python3` as well, and the two answers compared.
- **BASIC → C**, a source that declares nothing, so the head of the output is
  the aggregate of its body.
- **C → C**, the tool rewriting its own front end in text mode, with the
  result compiled and run.

## Where to read

- **[Tutorial](tutorial.html)**: one concept per section, each with a file
  you can run and the output it gives. Every transcript is checked by the
  suite.
- **[Glossary](glossary.html)**: the concepts in the order they depend on
  each other, then every term of art, for a reader meeting Pratt parsers,
  attribute grammars or hygiene here first.
- **[Reference](reference.html)**: what every part of a `.mx` file means,
  exhaustively, with a contents table and an index.
- **[Examples](examples.html)**: every example in the tree with its source
  and recorded output side by side.
- **[Notation](notation.html)**: why it is shaped this way and what that
  costs, argued rather than stated.
- **[Direction](direction.html)**: where it could go, and which futures are
  declined.
- **[Prior art](prior-art.html)**: the tools that do something like this,
  and how this one scores against them.
- **[Languages](languages.html)**: which languages the tool can be pointed
  at and which it cannot, each verdict resting on a stated property.
- **[Changelog](changelog.html)** and **[Roadmap](roadmap.html)**: when
  things shipped, and what is not built and why.

## Getting it

```
git clone https://github.com/hansolovkarlsson/Metaxis
cd Metaxis && make && make check
bin/mx examples/first.mx
```

C11 and `make`, plus POSIX `<regex.h>`. Nothing else. The suite runs on
every push, on Linux and macOS.
