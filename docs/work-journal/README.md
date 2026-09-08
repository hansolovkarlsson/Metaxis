# Work journal

*Why, in the order it happened. One file per working day, named for the date; a
day gets a new file rather than an append to a single page. This is where the
reasoning goes that the other records have no room for — what was tried, what
the first framing got wrong, why a decision went the way it did. Wrong turns are
recorded **as** wrong turns, because a retreat is only informed if the map shows
the dead end. Prose, not bullets, and written for a stranger who has only this
to go on.*

The other records answer narrower questions and are kept separately: what is
left ([ROADMAP.md](../ROADMAP.md)), what was built
([COMPLETED.md](../COMPLETED.md)), what a mistake taught
([POSTMORTEM.md](../POSTMORTEM.md)), and when something shipped
([CHANGELOG.md](../CHANGELOG.md)). Seven more sit beside them and are not
records of the work but of the thing: [tutorial.md](../tutorial.md) teaches it,
[glossary.md](../glossary.md) explains its terms of art,
[REFERENCE.md](../REFERENCE.md) states what a `.mx` file means,
[notation.md](../notation.md) argues for why it is shaped that way,
[direction.md](../direction.md) argues for where it could go,
[prior-art.md](../prior-art.md) surveys the tools that do something like this,
and [languages.md](../languages.md) says which languages it can be pointed at
and which it cannot. If a paragraph would fit in one of those, it belongs there
and not here.

**Entries written before 2026-09-05 call the tool `Prototype` and its files
`.pt`.** That was its name at the time; it was renamed to **Metaxis** on
2026-09-05, and these pages are dated accounts rather than descriptions of now,
so they were left as written. Rewriting them would make an entry claim a name
that did not exist when it was written.
[direction.md](../direction.md)'s "What to call it" has the decision.
[CHANGELOG.md](../CHANGELOG.md) and [POSTMORTEM.md](../POSTMORTEM.md) were left
alone for the same reason.

| Day | |
|---|---|
| [2026-09-08](2026-09-08.md) | A question about the code template, which has procedures and not functions, and the thing that question turned out to be about: `-u` may be given more than once and a call resolves after the header is sealed, so a grammar whose every template is one call can be read with a backend file of procedures and the target is chosen on the command line. Tried in a scratch directory, where it worked first time because a parameter carries its hole's level and a backend can therefore bracket its own way. Then `prior-art.md` read against the tree, which found its second-most-recent verdicts describing built features as wanted, three shortlist rows included. Then the shape built rather than described: `lib/mini.mx` with two backends, C and Python, compiled and run against each other. The seam the roadmap item was written around moved when it met real files, and the counterexample that replaced the deleted scratch one was in the tree already. Then a question about global variables and a flag in place of a bracket, whose answer is that the store has been that flag since the day before and that what a flag cannot reach is text mode's pass-through, and whose by-product is the thing that was actually missing: a conditional's arms are a repeated group, so `lib/cpp.mx` reads `#elif defined(NAME)`, nested, with nothing new in the tool. |
| [2026-09-07](2026-09-07.md) | Six sessions. An audit of the day before, whose first boundary was read off a truncated listing, and what it found once the base was right: five sentences that counted things and had been overtaken, a splice pass that took the first mark whose name was a prefix of another's, a site builder that hung on an indented item and numbered seven properties 1, two hygiene pipelines whose failure read as a pass in the file that warns about exactly that, and a caption regex kept twice. Then a morning of fixing each, every fix proved on the broken code or a broken copy first. Two guards for the records, and a newline between a bracket and its match. Then stage 6, over three sessions: the store, a rule led by a class hole, `expand`, a `raw` hole, function-like macros, nested conditionals over a shared close, `read` with the cost written before the code, and `#` and `##`, which read item 11's whole list and retired it. Then three sessions that were about what a finished thing still owes. `-u` and `-i`, so the rules and the body may be two files, asked for by a preprocessor being a stage in somebody else's toolchain and answered by a seam that had been in the code for days without a name; the question *is it done, no open task* answered by running the file's own list of what it cannot do, which found the one of seven gaps that failed silently and cost the tool `refuse`; and two pages counting the test scripts wrongly for the second time in three weeks, corrected by hand the first time and checked this time. |
| [2026-09-06](2026-09-06.md) | A check for the documents' own transcripts, which found a trailing space and a stale name on its first run. Then the roadmap's rule applied deliberately for the first time: a fourth translator picked for the mechanic it would ask for — BASIC, which declares nothing — and a fifth that turned out to need nothing built, the tool rewriting its own front end in text mode. Collections faked with markers and awk before a line of C, then built in the shape the fake settled. A roadmap item lost for one commit by a slice that reached one heading too far. A tutorial written as runnable files, which found a sentence in the reference no example had ever exercised; a glossary; a website generated from the documents the suite checks; a logo redrawn twice and then taken from its author; and a day's layout bugs found by reading every page. In the evening, a
conformance suite scoped and declined, with its four decisions written into
the reference instead; two checks for the records; a style rule with a check
built before the sweep that made it true, whose first run passed on no input;
and eleven hundred dashes rewritten across every live document. Then a
seventh document about the thing: which languages the tool is fit for and
which it is not, every verdict on a stated property, and two verdicts
overturned by running them: Lisp and XML. |
| [2026-09-05](2026-09-05.md) | Two roadmap items, a reference audit that found its own first example never ran, and a change of method: one translator at a time. Stage 1 of Pascal→C, checked by a compiler, which paid on the first run and asked for two builtins. Then an afternoon in which three predictions met files and failed, and a third stretch that built `@fragment`, refuted the spelling the plan had written down for it, and found a crash `@template` had shipped with a day earlier. Then stage 3 — a block that is an indentation — and, last, the first look outward: a survey of the tools that do something like this, which falsified a claim in `direction.md` that nothing in the tree could have checked. |
| [2026-09-04](2026-09-04.md) | The whole project in one day: the premise, the parser, hygiene split in two, both kinds of template, groups, and four wrong turns — including a rule that was wrong while doing exactly what it said. |
