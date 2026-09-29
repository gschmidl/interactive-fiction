#!/usr/bin/env python3
"""Compare the port with sessions recorded on the reference system (sim990
3.3.0, DX10 3.7, the GAMES library restored from the original tape; the
system console, a teleprinter).  See reference/README.md.

    python crossref.py                 check each session at its known clock
    python crossref.py --find NAME DATE HH:MM FROM-MINUTES
                                       search the clock for a new session

The game seeds its random numbers from the minute and second of the first
reading of the clock inside RANDOM, so a session only replays when the
port's clock (--clock, which ticks once a reading) is set to match.  The
clocks found are kept in reference/clocks.txt.  Everything up to NORMAL
PROGRAM COMPLETION must be identical; the SDT line after it only shows the
time the session ended.
"""
import datetime
import os
import re
import shutil
import sys
import tempfile

from common import REF, port_lines, reference_session, run

# The sessions share one scratch folder, run in name order, so that a later
# one finds the save file an earlier one wrote.
SCRATCH = None


def pathname_prompt(lines):
    """SCI offers the pathname used last in the same logon (the synonym
    $CAVE) as the initial value; the port has no logon to remember it in"""
    return [re.sub(r'^( SAVE/RESTORE PATHNAME: )\S*  ', r'\1  ', ln) for ln in lines]


def compare(name, clock, show=True):
    inputs, want = reference_session(name)
    text, rc = run(inputs, ['--clock=' + clock], cwd=SCRATCH)
    got = pathname_prompt(port_lines(text))
    want = pathname_prompt(want[:-1])   # without the SDT line
    got = got[:len(want)]
    if got == want:
        return True
    if show:
        for i, (a, b) in enumerate(zip(want, got)):
            if a != b:
                print('  line %d:\n    reference: %r\n    port:      %r' % (i + 1, a, b))
                break
        else:
            print('  lengths differ: reference %d lines, port %d' % (len(want), len(got)))
    return False


def clocks():
    table = {}
    path = os.path.join(REF, 'clocks.txt')
    for line in open(path):
        line = line.split('#')[0].strip()
        if line:
            name, clock = line.split(None, 1)
            table[name] = clock
    return table


def find(name, date, start, minutes):
    t0 = datetime.datetime.strptime(date + ' ' + start, '%Y-%m-%d %H:%M')
    for s in range(minutes * 60):
        t = t0 + datetime.timedelta(seconds=s)
        clock = t.strftime('%Y-%m-%d %H:%M:%S')
        if compare(name, clock, show=False):
            print('%s %s' % (name, clock))
            return 0
    print('%s: no clock from %s %s on reproduces it' % (name, date, start))
    return 1


def main():
    global SCRATCH
    SCRATCH = tempfile.mkdtemp(prefix='adv990-')
    try:
        if len(sys.argv) > 1 and sys.argv[1] == '--find':
            return find(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]))
        bad = 0
        for name, clock in sorted(clocks().items()):
            ok = compare(name, clock)
            print('%-6s %s  %s' % (name, clock, 'identical' if ok else 'DIFFERENT'))
            bad += not ok
        return 1 if bad else 0
    finally:
        shutil.rmtree(SCRATCH, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
