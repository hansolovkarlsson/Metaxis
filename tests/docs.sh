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
# Which documents. Everything in docs/ and README.md except the two dated
# accounts, named rather than patterned: POSTMORTEM.md quotes the invented
# transcript *as* the record of the mistake and must go on quoting it, and
# CHANGELOG.md says what somebody saw on a given day, which a later day may
# rightly change. The work journal is the same kind of page and is not under
# docs/*.md. A transcript in any of the others is a claim about now.

MX="${1:-./bin/mx}"
LIMIT="${LIMIT:-10}"
TMP="${TMPDIR:-/tmp}/mx-docs.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

if [ -z "$DOCS" ]; then
    for f in README.md docs/*.md; do
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
    why=$(awk -v want="$TMP/$i.want" '
        function strip(s) { sub(/[ \t]+$/, "", s); return s }
        FILENAME == want { w[++nw] = strip($0); next }
                         { g[++ng] = strip($0) }
        END {
            j = 1; elide = 0
            for (k = 1; k <= nw; k++) {
                if (w[k] == "…" || w[k] == "...") { elide = 1; continue }
                if (elide) {
                    while (j <= ng && g[j] != w[k]) j++
                    if (j > ng) { print "after the …, no line of the output reads: " w[k]; exit 1 }
                    elide = 0
                } else if (j > ng) {
                    print "the output ends before: " w[k]; exit 1
                } else if (g[j] != w[k]) {
                    print "output line " j " reads: " g[j]; exit 1
                }
                j++
            }
            if (!elide && j <= ng) { print "the output goes on past the transcript: " g[j]; exit 1 }
        }
    ' "$TMP/$i.want" "$TMP/$i.got")
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

if [ $fail -eq 0 ]; then
    echo "ok      docs.sh: $n transcripts"
fi
exit $fail
