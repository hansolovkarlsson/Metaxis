#!/bin/sh
# docs.sh -- every `$ mx ...` transcript in the documents, run.
#
# A transcript is a claim with a `$` in front of it, and the `$` makes it look
# checked. On 2026-09-05 one of the two in docs/REFERENCE.md had been written
# instead of run: `mx -g examples/pascal.mx` was shown printing a backend that
# file does not declare. docs/POSTMORTEM.md 19 is the entry and the customer.
# The number of transcripts only goes up, and the tool can check its own
# documentation -- so it does.
#
# The shape: inside a fenced block, a line beginning `$ mx ` is a command and
# the lines under it, to the next `$ ` line or the closing fence, are what it
# is claimed to print. The command is run from the tree root -- so a document
# writes `examples/use.mx`, never a path relative to itself -- with `mx`
# replaced by the binary under tests/limit.sh, and the rest of the line handed
# to the shell as written, redirections included: `2>&1 >/dev/null` in a
# transcript means what it means at a prompt. What the terminal would show is
# what is compared, so stdout and stderr are taken together and the exit status
# is not looked at, since a transcript cannot show one either.
#
# Two decisions, recorded here because neither was obvious:
#
#   elision      a line that is exactly `…` (or `...`) means *skip ahead* -- the
#                next line of the transcript is looked for further down the
#                output, and what lies between is not compared. A trailing `…`
#                matches whatever remains. Transcripts keep eliding, because
#                REFERENCE.md is for a reader and forty lines of grammar would
#                make it worse; the check bends instead.
#
#   whitespace   trailing whitespace is stripped on both sides before comparing.
#                An editor strips it from a document, so a document cannot hold
#                it, and a check that demanded it could never pass. (Nothing here
#                emits any now; mx.c's `show` used to, and this is what noticed.)
#
# Which documents. Everything in docs/, README.md and site/index.md, the one
# page written for the site, except the two dated accounts, named rather
# than patterned: POSTMORTEM.md quotes the invented
# transcript *as* the record of the mistake and must go on quoting it, and
# CHANGELOG.md says what somebody saw on a given day, which a later day may
# rightly change. The work journal is the same kind of page and is not under
# docs/*.md. A transcript in any of the others is a claim about now.
#
# The second thing a page can claim, since 2026-09-06: that a fenced block *is*
# a file. A line reading `docs/tutorial/01-first.mx`: on its own -- the
# regex is site/build.py's, which renders that line as the caption of the
# block under it -- followed by a fence, possibly after blank lines, says the
# block is that file. Twenty-five of those stood on three pages the day this
# was written, and nothing compared a block to its file: the transcript under
# it runs the file on disk, so a block that drifted would sit above a
# transcript true of a file the reader was not looking at, and stay green.
# docs/POSTMORTEM.md 19 and 24 one layer up, and ROADMAP item 10 for a day.
#
# The convention, read off the pages: the block is the file whole, or the file
# minus its leading `;` comment block and the blank lines after it, which is
# how the tutorial quotes a file whose comment repeats the prose beside it.
# Either is accepted; anything else is a drift. Trailing whitespace is
# stripped on both sides, for the reason above, and a line that is exactly `…`
# means *skip ahead* exactly as it does in a transcript: the tutorial's
# customer for that is eight blocks that omit the three header lines an
# earlier section already showed, and one `…` at the top says so to the
# reader as well as to the check. The same awk compares both kinds of claim.

MX="${1:-./bin/mx}"
LIMIT="${LIMIT:-10}"
TMP="${TMPDIR:-/tmp}/mx-docs.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

if [ -z "$DOCS" ]; then
    for f in README.md site/index.md docs/*.md; do
        case "$f" in
        docs/POSTMORTEM.md|docs/CHANGELOG.md) ;;
        *) DOCS="$DOCS $f" ;;
        esac
    done
fi

# One pair of files per transcript: N.cmd holds `file:line<TAB>command`, N.want
# the lines under it. awk's exit status is checked, as in hygiene.sh: a check
# whose extractor breaks must not read as a pass.
n=$(awk -v tmp="$TMP" '
    FNR == 1  { fence = 0 }
    /^```/    { fence = !fence; open = 0; next }
    !fence    { next }
    /^\$ /    {
        open = 0
        if ($0 ~ /^\$ mx( |$)/) {
            n++; open = 1
            printf "%s:%d\t%s\n", FILENAME, FNR, substr($0, 3) > (tmp "/" n ".cmd")
            printf "" > (tmp "/" n ".want")
        }
        next
    }
    open      { print > (tmp "/" n ".want") }
    END       { print n + 0 }
' $DOCS) || {
    echo "FAILED  docs.sh: the extractor did not run -- awk exited $?."
    exit 1
}

if [ "$n" -eq 0 ]; then
    echo "FAILED  docs.sh: found no transcripts in:$DOCS"
    echo "        There are some, so the extractor is broken, not the documents."
    exit 1
fi

# compare WANT GOT WHAT-WANT-IS WHAT-GOT-IS: the lines of WANT, a document's
# claim, against the lines of GOT, what is really there. Trailing whitespace is
# stripped on both sides; a WANT line that is exactly `…` or `...` skips ahead.
# Prints why on a mismatch and exits 1. Used for a transcript against the tool's
# output and for a quoted file against the file.
compare() {
    awk -v want="$1" -v wn="$3" -v gn="$4" '
        function strip(s) { sub(/[ \t]+$/, "", s); return s }
        FILENAME == want { w[++nw] = strip($0); next }
                         { g[++ng] = strip($0) }
        END {
            j = 1; elide = 0
            for (k = 1; k <= nw; k++) {
                if (w[k] == "…" || w[k] == "...") { elide = 1; continue }
                if (elide) {
                    while (j <= ng && g[j] != w[k]) j++
                    if (j > ng) { print "after the …, no line of the " gn " reads: " w[k]; exit 1 }
                    elide = 0
                } else if (j > ng) {
                    print "the " gn " ends before: " w[k]; exit 1
                } else if (g[j] != w[k]) {
                    print gn " line " j " reads: " g[j]; exit 1
                }
                j++
            }
            if (!elide && j <= ng) { print "the " gn " goes on past the " wn ": " g[j]; exit 1 }
        }
    ' "$1" "$2"
}

fail=0
i=1
while [ "$i" -le "$n" ]; do
    IFS='	' read -r where cmd < "$TMP/$i.cmd"
    run="sh tests/limit.sh $LIMIT $MX ${cmd#mx}"
    sh -c "$run" > "$TMP/$i.got" 2>&1
    if [ $? -eq 124 ]; then
        echo "FAILED  docs.sh: $where  $cmd"
        echo "        did not finish in ${LIMIT}s -- killed."
        fail=1; i=$((i + 1)); continue
    fi
    why=$(compare "$TMP/$i.want" "$TMP/$i.got" transcript output)
    if [ $? -eq 0 ]; then
        echo "ok      docs.sh: $where  $cmd"
    else
        echo "FAILED  docs.sh: $where  $cmd"
        echo "        $why"
        echo "        --- the document, then what the tool printed:"
        sed 's/^/            /' "$TMP/$i.want"
        echo "        ---"
        sed 's/^/            /' "$TMP/$i.got"
        fail=1
    fi
    i=$((i + 1))
done

# Every labelled fence, against the file it names. One pair per block:
# N.path holds `file:line<TAB>path`, N.fence the block's lines.
m=$(awk -v tmp="$TMP" '
    FNR == 1  { fence = 0; label = "" }
    /^```/    {
        fence = !fence
        if (fence && label != "") {
            m++; open = 1
            printf "%s\t%s\n", where, label > (tmp "/" m ".path")
            printf "" > (tmp "/" m ".fence")
        } else open = 0
        label = ""; next
    }
    fence     { if (open) print > (tmp "/" m ".fence"); next }
    /^[ \t]*`[A-Za-z0-9_.\/-]+\.mx`(, whole)?:[ \t]*$/ {
        label = $0; sub(/^[ \t]*`/, "", label); sub(/`.*$/, "", label)
        where = FILENAME ":" FNR; next
    }
    /^[ \t]*$/ { next }
    { label = "" }
    END       { print m + 0 }
' $DOCS) || {
    echo "FAILED  docs.sh: the quoted-file extractor did not run -- awk exited $?."
    exit 1
}

if [ "$m" -eq 0 ]; then
    echo "FAILED  docs.sh: found no labelled fence in:$DOCS"
    echo "        There are some, so the extractor is broken, not the documents."
    exit 1
fi

i=1
while [ "$i" -le "$m" ]; do
    IFS='	' read -r where path < "$TMP/$i.path"
    if [ ! -f "$path" ]; then
        echo "FAILED  docs.sh: $where  labels a block as $path, and there is no such file."
        fail=1; i=$((i + 1)); continue
    fi
    awk 'BEGIN { head = 1 } head && /^;/ { next } head && /^$/ { next } { head = 0; print }' "$path" > "$TMP/$i.body"
    if compare "$TMP/$i.fence" "$path" block file > /dev/null; then
        echo "ok      docs.sh: $where  $path"
    elif why=$(compare "$TMP/$i.fence" "$TMP/$i.body" block file); then
        echo "ok      docs.sh: $where  $path"
    else
        echo "FAILED  docs.sh: $where  quotes $path, and the block is not the file."
        echo "        Neither the file whole nor the file after its leading comment: $why"
        echo "        --- the document, then the file after its comment:"
        sed 's/^/            /' "$TMP/$i.fence"
        echo "        ---"
        sed 's/^/            /' "$TMP/$i.body"
        fail=1
    fi
    i=$((i + 1))
done

if [ $fail -eq 0 ]; then
    echo "ok      docs.sh: $n transcripts, $m quoted files"
fi
exit $fail
