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
([CHANGELOG.md](../CHANGELOG.md)). Three more sit beside them and are not
records of the work but of the thing: [REFERENCE.md](../REFERENCE.md) states
what a `.mx` file means, [notation.md](../notation.md) argues for why it is
shaped that way, and [direction.md](../direction.md) argues for where it could
go. If a paragraph would fit in one of those, it belongs there and not here.

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
| [2026-09-05](2026-09-05.md) | Two roadmap items, a reference audit that found its own first example never ran, and a change of method: one translator at a time. Stage 1 of Pascal→C, checked by a compiler, which paid on the first run and asked for two builtins. Then an afternoon in which three predictions met files and failed, and a third stretch that built `@fragment`, refuted the spelling the plan had written down for it, and found a crash `@template` had shipped with a day earlier. Then stage 3 — a block that is an indentation — and, last, the first look outward: a survey of the tools that do something like this, which falsified a claim in `direction.md` that nothing in the tree could have checked. |
| [2026-09-04](2026-09-04.md) | The whole project in one day: the premise, the parser, hygiene split in two, both kinds of template, groups, and four wrong turns — including a rule that was wrong while doing exactly what it said. |
