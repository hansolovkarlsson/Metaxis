#!/bin/sh
# hygiene.sh -- the five checks that read the tree instead of running it.
#
# The last one is the file's original job and most of what is below. The
# first is the limit guard, added because it is the same kind of check -- a
# property nothing else here can express -- and a sixth script for one grep
# would have been one too many; it closed a roadmap item and is
# docs/COMPLETED.md's "The limit guard" now. The second is the roadmap's
# numbers, added on the same reasoning, for docs/POSTMORTEM.md 20. The third
# is the prose rule in CLAUDE.md, added so that a sweep of the documents had a
# finish line the suite could see, and so that new writing has a guard. The
# fourth is the errors page against the source, for docs/POSTMORTEM.md 24.
#
# ---------------------------------------------------------------------------
# 1 - the limit guard
#
# tests/limit.sh exists so that a hang is reported rather than waited on, and it
# only works where it is actually used. Nothing enforced that. A test script
# added next month that runs the tool directly is a hole in the one guard this
# suite has against the failure no recorded .out can express, and it would pass
# every check here on the day it was written.
#
# This is not hypothetical, which is why it is a check and not a note.
# docs/COMPLETED.md once claimed "all six places the suite runs the tool" go
# through limit.sh. There were seven that morning and eight by the evening, and
# a close-out read the sentence and did not count. POSTMORTEM 16: **a number in
# prose is not a check.** So this counts nothing -- it states the property.
#
# The property: the tool is never *run* except through tests/limit.sh. Three
# shapes name the binary without running it, and each is exempted by name
# rather than by a pattern broad enough to hide a fourth:
#
#   BIN = ... / MX=...     where the path is stored
#   a make rule header     `all: $(BIN)`, `$(BIN): $(OBJ)`, `check: $(BIN)`
#   sh tests/<x>.sh ...    handing the path to a script this same check covers
#
# The third is the loosest of the three -- it would also forgive a line that
# both handed the path over and ran it -- and it is written this way because
# the alternative is a shell parser. If that shape ever appears, tighten it.
#
# ---------------------------------------------------------------------------
# 2 - the roadmap's numbers: a citation resolves, and an item does not vanish.
#
# A roadmap item keeps its number for life -- one that lands is moved to
# docs/COMPLETED.md and its number is retired, never reused -- so a document
# may cite one as `ROADMAP.md N` and be right for as long as the item is open.
# Two things go wrong with that, and both have happened (docs/POSTMORTEM.md 20):
#
#   stale   an item lands and a citation goes on naming its number. A retired
#           number resolves to nothing, and that is caught here. A number that
#           was *reused*, before the rule above was the rule, resolves to the
#           wrong item, and no check by number can see that; three of those
#           were found by hand the day this was written. Once an item has
#           landed, cite docs/COMPLETED.md by its heading.
#
#   lost    an edit addressed by *the next heading I remember* cuts an item
#           out, in a file whose items are not in numeric order. Every
#           `## N ·` heading at HEAD must still be in the tree, unless the
#           commit being prepared is the one that moves it -- say so with
#           SETTLED='N' (numbers, space-separated) on the make line.
#
#   reused  a number given twice (docs/POSTMORTEM.md 29). The page's gaps
#           cannot show what was retired, because an item settled the same
#           afternoon leaves no gap in any commit, so the roadmap's opening
#           note carries the list as one sentence, `Retired so far: 4, 7
#           and 9.`, and this reads it. No `## N ·` heading may have an N
#           on that list, and every N that was a heading at HEAD and is not
#           one now must be on it, which is what keeps the list complete.
#           A note the sentence cannot be read from is refused, the way a
#           page with no headings is.
#
# A citation is one of the four spellings the tree uses -- ROADMAP N,
# ROADMAP.md N, docs/ROADMAP.md N, and the link [ROADMAP.md](ROADMAP.md) N --
# and nothing looser: "item 4 of the roadmap as it then stood" is prose about
# the past and is meant not to match. The dated accounts (POSTMORTEM.md,
# CHANGELOG.md, the journal) are not scanned, for docs.sh's reason: they say
# what was true on a day. Everything else in the tree is a claim about now.


# ---------------------------------------------------------------------------
# 3 - the prose rule: no em dash in general prose.
#
# CLAUDE.md's style guide says the em dash is not used in prose, and names
# where it stays: a fact (a date, a range), a quotation reproduced as written,
# a name or title that has one, and anything that is code, an example, a
# transcript or recorded output. Each exemption here is one of those, by name:
#
#   a fenced block       code, a transcript, recorded output
#   an inline code span  `like this`
#   a blockquote line    reproduced as written
#   <!-- as written -->  on the line: a quotation, a title, a fact. The marker
#                        is an HTML comment, so a rendered page does not show
#                        it, and it says why the dash is there to whoever
#                        reads the source.
#
# What is scanned is every tracked Markdown file except the journal, which is
# a diary of closed days by its own charter and is left as written. The other
# two dated accounts, POSTMORTEM.md and CHANGELOG.md, were swept on 2026-09-06
# and are scanned like the rest; what they quote keeps its dash by the marker.
# Source comments, .mx files and .out files had none the day this was written
# and are not scanned: a dash in a .mx body or a .out is the tool's data, and
# the rule is about prose.
#
# ---------------------------------------------------------------------------
# 4 - the errors page: every message it quotes is one the source prints.
#
# docs/REFERENCE.md 10 lists every message in backticks, and a backticked
# message is a claim that the tool says this. Twenty-five of them once showed
# an em dash where the tool prints two hyphens, typeset into the page and
# compared to nothing (docs/POSTMORTEM.md 24). tests/errors.sh pins a message
# by a fragment, which is a different claim: that the tool says it *there*.
# This one is that the page says it *right*, and it reads the two files
# rather than running anything.
#
# A message on the page is fixed text around placeholders, and the check knows
# five shapes of placeholder: a quoted span opened at the start or after a
# space ('x', 'f:n', '{~name}'), an ellipsis (… or ...), `f:n` and `file:line`
# bare, a run of digits, and a directive name after a space (`@mode`), which
# the source fills in from a variable. The fixed pieces between them must
# stand in one string literal of metaxis/src or metaxis/cmd, in order; a
# quoted span must be in that literal verbatim unless the literal has a `%`
# where a value goes. Adjacent literals are joined the way the C compiler
# joins them, since a long message is written across two lines. Cells that
# list several messages with `·` between them are several messages.
#
# The other pages quote messages too, in sentences rather than cells, with
# nothing marking which backticks are messages (docs/COMPLETED.md, "The
# messages the pages quote outside the errors page"). So outside §10 a
# backticked span is taken as a message when two words of letters in a row
# from one of its fixed pieces stand in a source literal, and is then held
# to the rule above, whole. The window is looser than the rule on purpose:
# a message with one word gone stale is still recognised by the words beside
# it. What that cannot see is a message rewritten past recognition, and a
# code span two of whose words happen to be in a literal is held to a rule
# it did not sign up for, which has not happened. The pages read are the
# ones the citation check reads, the dated accounts excepted, and a span
# that wraps across a line break is one span.
#
# What it cannot see: a word the source chooses at run time. `'at' was given
# 9 and there are 2` is printed from `... and there %s %d`, so the page ends
# that one with an ellipsis rather than the check learning English. It is run
# under LC_ALL=C so that both awks read bytes, the ellipsis being three.

# ---------------------------------------------------------------------------
# 5 - hygiene: one half fixed, one half charged, run rather than argued.
#
# It expands examples/hygiene.mx, compiles the C that comes out and runs it.
# Three outcomes are named below and each says something different, because
# there are two failures here and they moved at different times.
#
#   swap    a template that INTRODUCES a name. `{~t}` closed this one: the
#           template asks for a name nobody else has, the caller's `t` is not
#           it, and the second call site does not get the first's temporary.
#
#   bump    a template that REACHES OUT for a name the caller shadowed. Not
#           closed, and not closable this way: there is nothing to invent. The
#           template means the binding in scope where the rule was written, and
#           a string has no way to see a scope. Proto closes it because its
#           expander works on trees in a language whose scopes it knows.
#
# **This test passing means `bump` is still wrong.** It is written that way on
# purpose. A test that merely failed would be turned off; one that pins the
# wrong answer has to be edited by whoever fixes it, in the same commit, which
# is the only way the record and the code stay in step. That is what happened
# to the previous version of this file: it pinned `swap: 2 2`, and `{~t}` made
# it wrong, and this is the edit.
#
# The numbers are Proto's. `Proto/examples/forms.pro` demonstrates the same two
# failures against the same expansion, and its comments record #105 and #0 for
# the second -- the same pair the last line here still prints.

MX="${1:-./bin/mx}"
CC="${CC:-cc}"
LIMIT="${LIMIT:-10}"
SRC="${SRC:-examples/hygiene.mx}"

if [ ! -f tests/limit.sh ]; then
    echo "FAILED  hygiene.sh: tests/limit.sh is gone, and everything below"
    echo "        assumes it is there. See the note at the head of this file."
    exit 1
fi

# The bracket classes below are not decoration. This file is one of the files
# being scanned, so a pattern written the obvious way would match its own text
# and report itself -- `grep -v grep`, one level up. Written like this it still
# matches $(BIN), bin/mx and $MX and does not contain any of them.
# `|| exit` is not belt and braces. The first draft of this had a regex awk
# could not parse; awk exited 2, the substitution came back empty, and the guard
# printed `ok`. **A check that passes when its own machinery breaks is worse
# than no check**, because it also silences the one that would have told you.
unguarded=$(awk '
    /[$][(]BIN[)]|bin\/mx|[$]M[X]/ {
        if ($0 ~ /^[[:space:]]*#/)                    next   # a comment runs nothing
        if ($0 ~ /limit[.]sh/)                        next   # guarded
        if ($0 ~ /^[[:space:]]*BIN[[:space:]]*=/)     next   # stored, not run
        if ($0 ~ /^[[:space:]]*M[X]=/)                next   # stored, not run
        if (FILENAME == "Makefile" && $0 ~ /^[^\t ][^=]*:/) next   # rule header
        if ($0 ~ /sh tests\/[a-z]+[.]sh /)            next   # handed on
        printf "%s:%d:%s\n", FILENAME, FNR, $0
    }
' Makefile tests/*.sh) || {
    echo "FAILED  hygiene.sh: the limit guard did not run -- awk exited $?."
    echo "        Its own failure must not read as a pass. See the note above it."
    exit 1
}

if [ -n "$unguarded" ]; then
    echo "FAILED  hygiene.sh: the tool is run without tests/limit.sh in front of it."
    echo "        A hang there stops the suite instead of failing it, which is"
    echo "        the one outcome no recorded .out can express."
    echo "$unguarded" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every run of the tool goes through tests/limit.sh"

TMP="${TMPDIR:-/tmp}/mx-hygiene.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# --- 2: the roadmap's numbers. What it checks is described at the head of the file.
present=$(sed -n -E 's/^## ([0-9]+) ·.*/\1/p' docs/ROADMAP.md)
if [ -z "$present" ]; then
    echo "FAILED  hygiene.sh: docs/ROADMAP.md has no '## N ·' headings, or has moved."
    echo "        The roadmap check has nothing to resolve against."
    exit 1
fi
ok=" $(printf '%s ' $present)"

# The tree is what git tracks. If git cannot say, this must not read as a pass.
tracked=$(git ls-files 2>/dev/null) || {
    echo "FAILED  hygiene.sh: git ls-files did not answer, so the roadmap check"
    echo "        cannot say which files are the tree's."
    exit 1
}
scanned=$(echo "$tracked" | grep -v -E '^docs/(POSTMORTEM|CHANGELOG)\.md$|^docs/work-journal/')

# xargs cannot say whether grep failed: it exits 123 for a file with no match
# and for a broken regex alike. grep's stderr can, and so can an empty result,
# since the tree cites the roadmap from several pages. Both read as a failure
# of the check, not a pass -- the limit guard's rule, for the same reason.
cited=$(echo "$scanned" | xargs grep -n -I -o -E \
        'ROADMAP(\.md)?(\]\([^)]*\))?[[:space:]]+[0-9]+' 2> "$TMP/grep.err")
if [ -s "$TMP/grep.err" ]; then
    echo "FAILED  hygiene.sh: the citation scan did not run -- grep complained."
    echo "        Its own failure must not read as a pass."
    sed 's/^/            /' "$TMP/grep.err"
    exit 1
fi
if [ -z "$cited" ]; then
    echo "FAILED  hygiene.sh: the citation scan found no 'ROADMAP.md N' anywhere,"
    echo "        which cannot be right; an empty scan must not read as a pass."
    exit 1
fi
stale=$(echo "$cited" | awk -v ok="$ok" '
        { match($0, /[0-9]+$/); n = substr($0, RSTART)
          if (index(ok, " " n " ") == 0) print }')

if [ -n "$stale" ]; then
    echo "FAILED  hygiene.sh: a document cites a roadmap item that is not on the roadmap."
    echo "        A retired number points at nothing. If the item landed, cite"
    echo "        docs/COMPLETED.md by its heading. Items on the roadmap now:$ok"
    echo "$stale" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every roadmap citation in the tree resolves to an item"

headpage=$(git show HEAD:docs/ROADMAP.md 2>/dev/null) || {
    echo "FAILED  hygiene.sh: git show HEAD:docs/ROADMAP.md did not answer, so the"
    echo "        lost-item check has nothing to compare against."
    exit 1
}
if ! echo "$headpage" | grep -q -E '^## [0-9]+ ·'; then
    echo "FAILED  hygiene.sh: docs/ROADMAP.md at HEAD has no '## N ·' headings,"
    echo "        so the lost-item check would pass on nothing."
    exit 1
fi
lost=$(echo "$headpage" |
    awk -v ok="$ok" -v settled=" $SETTLED " '
        /^## [0-9]+ ·/ { n = $2
          if (index(ok, " " n " ") == 0 && index(settled, " " n " ") == 0) print }')

if [ -n "$lost" ]; then
    echo "FAILED  hygiene.sh: a roadmap item that was on the page at HEAD is not in the tree."
    echo "        Restore it -- or, if this is the commit that moves it to"
    echo "        docs/COMPLETED.md, say so: SETTLED='N' make check"
    echo "$lost" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every roadmap item at HEAD is still on the page"

# The retired list, read off the roadmap's opening note. The sentence may
# wrap, so the page is read as one line and the sentence taken up to its
# period, in the one shape it is allowed: numbers, commas, one `and`.
tr '\n' ' ' < docs/ROADMAP.md > "$TMP/roadmap.line" || {
    echo "FAILED  hygiene.sh: docs/ROADMAP.md could not be read as one line, so"
    echo "        the retired list cannot be found."
    exit 1
}
retired=$(sed -n -E \
    's/.*Retired so far: ([0-9]+(, [0-9]+)*( and [0-9]+)?)\..*/\1/p' \
    "$TMP/roadmap.line" | tr -c '0-9\n' ' ')
if [ -z "$retired" ]; then
    echo "FAILED  hygiene.sh: docs/ROADMAP.md's opening note has no sentence of the"
    echo "        shape 'Retired so far: 4, 7 and 9.' that the reuse check can read."
    echo "        Without the list a retired number can be given again in silence."
    exit 1
fi
retired=" $(printf '%s ' $retired)"

reused=$(grep -E '^## [0-9]+ ·' docs/ROADMAP.md |
    awk -v retired="$retired" '{ if (index(retired, " " $2 " ")) print }') || {
    echo "FAILED  hygiene.sh: the reuse scan did not run -- awk exited $?."
    exit 1
}
if [ -n "$reused" ]; then
    echo "FAILED  hygiene.sh: a roadmap heading uses a number the note has retired."
    echo "        A number is for life: the next free one is after the highest"
    echo "        ever used, on the page or on the list. Retired:$retired"
    echo "$reused" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: no roadmap heading reuses a retired number"

unlisted=$(echo "$headpage" |
    awk -v ok="$ok" -v retired="$retired" '
        /^## [0-9]+ ·/ { n = $2
          if (index(ok, " " n " ") == 0 && index(retired, " " n " ") == 0) print }') || {
    echo "FAILED  hygiene.sh: the retired-list scan did not run -- awk exited $?."
    exit 1
}
if [ -n "$unlisted" ]; then
    echo "FAILED  hygiene.sh: a roadmap item left the page and its number is not on"
    echo "        the note's retired list. Add it to 'Retired so far:' in the same"
    echo "        commit, so the number is never given again."
    echo "$unlisted" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every roadmap item that left the page is on the retired list"

# --- 3: the prose rule. Described at the head of the file.
pages=$(echo "$tracked" | grep -E '\.md$' | grep -v -E '^docs/work-journal/')
if [ -z "$pages" ]; then
    echo "FAILED  hygiene.sh: the prose scan found no Markdown to read, which"
    echo "        cannot be right; an empty scan must not read as a pass."
    exit 1
fi
prose=$(echo "$pages" | xargs awk '
        FNR == 1              { fence = 0 }
        /^```/                { fence = !fence; next }
        fence                 { next }
        /^>/                  { next }
        /<!-- as written -->/ { next }
        { l = $0; gsub(/`[^`]*`/, "", l)
          if (l ~ /—/) printf "%s:%d:%s\n", FILENAME, FNR, $0 }') || {
    echo "FAILED  hygiene.sh: the prose scan did not run -- awk exited $?."
    exit 1
}

if [ -n "$prose" ]; then
    n=$(echo "$prose" | wc -l | tr -d ' ')
    echo "FAILED  hygiene.sh: an em dash in general prose, on $n lines."
    echo "        CLAUDE.md's style guide: a comma, a period or a colon instead."
    echo "        If it is a quotation, a title or a fact, say so on the line"
    echo "        with <!-- as written -->; if it is code, put it in backticks."
    echo "$prose" | head -20 | sed 's/^/            /'
    [ "$n" -gt 20 ] && echo "            ... and $((n - 20)) more"
    exit 1
fi
echo "ok      hygiene.sh: no em dash in general prose"

# --- 4: the errors page against the source. Described at the head of the file.
# The pages read beside it are the ones the citation check reads: every
# tracked Markdown file but the dated accounts, and the reference is not
# listed twice.
claims=$(echo "$pages" | grep -v -E '^docs/(POSTMORTEM|CHANGELOG|REFERENCE)\.md$')
if [ -z "$claims" ]; then
    echo "FAILED  hygiene.sh: the message scan found no page to read beside the"
    echo "        reference, which cannot be right."
    exit 1
fi
unmatched=$(LC_ALL=C awk -v q="'" '
    FNR == 1 { incomment = 0; pendingline = 0 }
    FILENAME ~ /[.]c$/ {
        line = $0; n = length(line); i = 1
        join = (pendingline && line ~ /^[ \t]*"/); pendingline = 0
        while (i <= n) {
            c = substr(line, i, 1)
            if (incomment) { j = index(substr(line, i), "*/"); if (!j) break; incomment = 0; i += j + 1; continue }
            if (c == "/" && substr(line, i + 1, 1) == "*") { incomment = 1; i += 2; continue }
            if (c == "/" && substr(line, i + 1, 1) == "/") break
            if (c == q) { i += (substr(line, i + 1, 1) == "\\") ? 4 : 3; continue }
            if (c == "\"") {
                if (join) { s = lit[nlit]; join = 0 } else { s = ""; nlit++ }
                i++
                while (i <= n) {
                    c = substr(line, i, 1)
                    if (c == "\\") { d = substr(line, i + 1, 1); s = s ((d == "n") ? "\n" : (d == "t") ? "\t" : d); i += 2; continue }
                    if (c == "\"") { i++; break }
                    s = s c; i++
                }
                lit[nlit] = s
                k = i; while (k <= n && substr(line, k, 1) ~ /[ \t]/) k++
                if (k > n) pendingline = 1
                else if (substr(line, k, 1) == "\"") { join = 1; i = k }
                continue
            }
            i++
        }
        next
    }
    FNR == 1 { insec = 0; fenced = 0; carry = "" }
    /^```/ { fenced = !fenced; next }
    fenced { next }
    FILENAME == "docs/REFERENCE.md" && /^## 10 / { insec = 1; next }
    FILENAME == "docs/REFERENCE.md" && /^## 11 / { insec = 0 }
    insec && /^\| `/ {
        cell = substr($0, 3); j = index(cell, " | "); if (j) cell = substr(cell, 1, j - 1)
        m = split(cell, parts, " · ")
        for (p = 1; p <= m; p++) {
            msg = parts[p]
            if (msg !~ /^`.*`$/) continue
            msg = substr(msg, 2, length(msg) - 2)
            nmsg++
            if (!found(msg)) print msg
        }
        next
    }
    insec { next }
    {
        # Outside the errors page a span is a message when two words in a
        # row of one of its fixed pieces, cut at the same placeholders, stand
        # in a source literal. Then the whole span is held to the same rule.
        # The window is looser than the rule on purpose: a message with one
        # word gone stale is still recognised by the words beside it, and
        # then fails. Two words of letters, long enough together to be nobody
        # else; the quoted string of a directive is not a word of letters,
        # which keeps `@separator "\n" indent` out although a hint prints it.
        # (No apostrophe in these comments: sh would read it as the end of
        # the program.)
        # A span that wraps is carried into the next line with one space
        # where the break was; the end of a paragraph drops it.
        rest = $0
        if (carry != "" && $0 != "") { sub(/^[ \t]+/, "", rest); rest = carry " " rest }
        carry = ""
        while ((a = index(rest, "\140")) > 0) {   # \140 is a backtick, which sh would pair
            rest = substr(rest, a + 1); b = index(rest, "\140")
            if (!b) { carry = "\140" rest; break }
            span = substr(rest, 1, b - 1); rest = substr(rest, b + 1)
            if (span == "" || !quotes(span)) continue
            nout++
            if (!found(span)) print FILENAME ":" FNR ": " span
        }
    }
    function quotes(msg,    nf, j, l, nw, w, pair) {
        nf = cut(msg)
        for (j = 1; j <= nf; j++) {
            nw = split(frag[j], words, " ")
            for (w = 1; w < nw; w++) {
                if (words[w] !~ /^[A-Za-z]+[,.:;]?$/ || words[w + 1] !~ /^[A-Za-z]+[,.:;]?$/) continue
                pair = words[w] " " words[w + 1]
                if (length(pair) < 10) continue
                for (l = 1; l <= nlit; l++) if (index(lit[l], pair)) return 1
            }
        }
        return 0
    }
    function cut(msg,    n, i, c, nf, k, s, prev) {
        n = length(msg); nf = 0; nq = 0; s = ""; prev = " "
        for (i = 1; i <= n; i++) {
            c = substr(msg, i, 1)
            if (c == q && prev == " ") {
                k = index(substr(msg, i + 1), q); if (!k) { s = s c; prev = c; continue }
                frag[++nf] = s; s = ""; quoted[++nq] = substr(msg, i, k + 1); i += k; prev = q; continue
            }
            if (substr(msg, i, 3) == "…" || substr(msg, i, 3) == "...") { frag[++nf] = s; s = ""; i += 2; prev = "."; continue }
            if (substr(msg, i, 3) == "f:n") { frag[++nf] = s; s = ""; i += 2; prev = "n"; continue }
            if (substr(msg, i, 9) == "file:line") { frag[++nf] = s; s = ""; i += 8; prev = "e"; continue }
            if (c ~ /[0-9]/) { frag[++nf] = s; s = ""; while (substr(msg, i + 1, 1) ~ /[0-9]/) i++; prev = "0"; continue }
            if (c == "@" && prev == " " && substr(msg, i + 1, 1) ~ /[a-z]/) { frag[++nf] = s; s = ""; while (substr(msg, i + 1, 1) ~ /[a-z]/) i++; prev = "@"; continue }
            s = s c; prev = c
        }
        frag[++nf] = s
        return nf
    }
    function found(msg,    nf, k, l, pos, j, ok) {
        nf = cut(msg)
        for (l = 1; l <= nlit; l++) {
            pos = 1; ok = 1
            for (j = 1; j <= nf; j++) {
                if (frag[j] == "") continue
                k = index(substr(lit[l], pos), frag[j]); if (!k) { ok = 0; break }
                pos += k - 1 + length(frag[j])
            }
            if (!ok) continue
            for (j = 1; j <= nq; j++) if (!index(lit[l], quoted[j]) && !index(lit[l], "%")) { ok = 0; break }
            if (ok) return 1
        }
        return 0
    }
    END {
        if (nlit == 0) { print "NONE: the scan found no string literal in the source"; exit 1 }
        if (nmsg == 0) { print "NONE: the scan found no message on the errors page"; exit 1 }
        if (nout == 0) { print "NONE: the scan found no message quoted outside the errors page"; exit 1 }
        print "COUNT " nmsg " " nout
    }
' metaxis/src/*.c metaxis/cmd/mx.c docs/REFERENCE.md $claims) || {
    echo "FAILED  hygiene.sh: the errors-page scan did not run -- awk exited $?."
    echo "$unmatched" | sed 's/^/        /'
    exit 1
}
nmsg=$(echo "$unmatched" | sed -n 's/^COUNT \([0-9]*\) .*/\1/p')
nout=$(echo "$unmatched" | sed -n 's/^COUNT [0-9]* //p')
unmatched=$(echo "$unmatched" | grep -v '^COUNT ')
if [ -n "$unmatched" ]; then
    echo "FAILED  hygiene.sh: a page quotes a message the source does not print."
    echo "        Each line below is a backticked message, on the errors page or"
    echo "        elsewhere with its file and line, whose fixed text is in no string"
    echo "        literal of metaxis/. Check the line that prints it and make the"
    echo "        page verbatim, with … where a value goes."
    printf '%s\n' "$unmatched" | sed 's/^/            /'
    exit 1
fi
echo "ok      hygiene.sh: every message the errors page quotes is one the source prints ($nmsg)"
echo "ok      hygiene.sh: every message quoted outside the errors page is one too ($nout)"


# Where it stands. Two lines right, one wrong, and the wrong one is not an error.
HALF='swap: 2 1
again: 4 3
bump: 105 0'

# Before `{~t}`. The form's temporary was a name the caller also had.
CAPTURED='swap: 2 2
again: 4 4
bump: 105 0'

# What closing the other half would look like: `bump` would leave the caller's
# local at 100 and add its 5 to the file-scope `total` it meant.
WHOLE='swap: 2 1
again: 4 3
bump: 100 5'

if ! sh tests/limit.sh "$LIMIT" "$MX" "$SRC" > "$TMP/body.c" 2> "$TMP/err"; then
    echo "FAILED  hygiene.sh: $SRC did not expand"
    cat "$TMP/err"
    exit 1
fi

# The one line the example does not emit, because `#include` is not a rule.
{ echo '#include <stdio.h>'; cat "$TMP/body.c"; } > "$TMP/prog.c"

if ! "$CC" -o "$TMP/prog" "$TMP/prog.c" 2> "$TMP/cc.err"; then
    echo "FAILED  hygiene.sh: the expansion does not compile"
    cat "$TMP/cc.err"
    exit 1
fi

got=$("$TMP/prog")

# Two applications of one rule must not share a temporary. The numbers above
# would catch that anyway; this catches it by name, and says why.
if ! grep -q 't__1' "$TMP/body.c" || ! grep -q 't__2' "$TMP/body.c"; then
    echo "FAILED  hygiene.sh: the two swaps did not get two names."
    echo "        {~t} is one name per expansion, not one per rule."
    grep -n 'int t' "$TMP/body.c"
    exit 1
fi

case "$got" in
"$HALF")
    echo "ok      hygiene.sh: {~t} holds; reaching out still does not"
    echo "            swap: 2 1      the template's own name, twice over"
    echo "            bump: 105 0    would be  bump: 100 5"
    exit 0 ;;
"$CAPTURED")
    echo "FAILED  hygiene.sh: the capture is back."
    echo "        {~t} has stopped giving each expansion its own name."
    exit 1 ;;
"$WHOLE")
    echo "FAILED  hygiene.sh: reaching out works too."
    echo "        That is the good news and this file is now wrong. Rewrite it,"
    echo "        and the hygiene section of docs/notation.md, in the commit"
    echo "        that did it -- and say there what a template learned to see."
    exit 1 ;;
*)
    echo "FAILED  hygiene.sh: none of the three recorded outcomes."
    echo "        wanted: $(echo "$HALF" | tr '\n' '/')"
    echo "        got:    $(echo "$got" | tr '\n' '/')"
    echo "        Work out which one it is heading for before touching the"
    echo "        numbers above."
    exit 1 ;;
esac
