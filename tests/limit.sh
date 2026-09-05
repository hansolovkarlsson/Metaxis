#!/bin/sh
# limit.sh -- run a command under a wall-clock limit, and say which failure it was.
#
# Every other check here compares what a file expanded to against what is
# recorded beside it, which catches a changed answer and, where the output is
# compiled and run, a wrong one. **None of them can express *did not
# terminate*.** There is no `.out` for a hang: `make check` in front of one does
# not go red, it stops, and a suite that stops is indistinguishable from a slow
# machine until somebody looks at the process list.
#
# That is not hypothetical. On 2026-09-05 a `for` with an index restored the
# environment to the wrong frame, so the second turn linked a frame to itself and
# `lookup` walked the cycle forever. It was found by the suite hanging, and the
# process was still spinning four and a half hours later because nothing ever
# reaped it. See docs/COMPLETED.md, and POSTMORTEM.md 14 for what it cost.
#
# macOS has no timeout(1) and this tree takes no dependencies, so this is the
# portable shape: start the command, race it against a sleeper, and let whichever
# finishes first decide. The sleeper marks the file *before* it kills, so a
# process that died from the watchdog is always distinguishable from one that
# exited on its own -- and the watchdog is killed before the mark is read, so a
# command that finished first cannot be reported as a timeout.
#
# Exits 124 on the limit, which is what GNU timeout uses, and passes the
# command's own status through otherwise.

LIMIT="$1"
[ -n "$LIMIT" ] || { echo "limit.sh: no limit given" >&2; exit 2; }
shift
[ "$#" -gt 0 ] || { echo "limit.sh: no command given" >&2; exit 2; }

mark="${TMPDIR:-/tmp}/pt-limit.$$.$LIMIT"
rm -f "$mark"

# Job control, so that the command becomes a process *group* leader and the
# watchdog can kill the group rather than the one process. `pt` spawns nothing,
# so for the suite as it stands this is belt and braces -- but a killed command
# that leaves children behind is exactly the orphan this file exists because of,
# and a check that leaks the thing it is policing would be a poor joke. `set +m`
# again straight after, so the shell's own reporting stays quiet.
set -m 2>/dev/null
"$@" &
cmd=$!
set +m 2>/dev/null

# The watchdog's output is closed off deliberately, and it is not tidiness. A
# background child inherits this script's stdout, so under `x=$(limit.sh …)` the
# command substitution waits for *every* holder of that pipe to let go -- and the
# watchdog holds it for the whole limit even when the command finished in
# milliseconds. That turned a 62-case run into ten minutes. Detaching it is what
# makes the limit free when nothing hangs.
( sleep "$LIMIT"; : > "$mark"
  kill -9 "-$cmd" 2>/dev/null || kill -9 "$cmd" 2>/dev/null
) >/dev/null 2>&1 &
dog=$!

wait "$cmd" 2>/dev/null
rc=$?

kill "$dog" 2>/dev/null
wait "$dog" 2>/dev/null

if [ -f "$mark" ]; then
    rm -f "$mark"
    exit 124
fi
rm -f "$mark"
exit "$rc"
