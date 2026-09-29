#!/usr/bin/env python3
"""Compare the port with the original running under NOS 1.3.

cyber1.txt and its friends are the print files of jobs that compiled
`ADVENT.txt` with FTN 4.7 on a Cyber 173 under NOS 1.3 (DtCyber) and played the
commands in the matching cmds file - see cyber\\mkdeck.py for exactly what the
deck contains.  The game's own output is cut out of the print file and diffed
against the port playing the same commands.

Two things are done to the Cyber's side first:

* The site's character set.  The Cyber prints the three characters the way
  DtCyber's table shows them - `^*`, `\\` and `%` - and the port prints what
  they meant - `!`, `?` and `:`.  The same substitution src/convert.py makes
  is made here, so what is compared is the text.
* The clock.  The generator is seeded from DATIME, the day of the year and the
  minute, when the program first draws from it.  The print file records the
  minute the game was started in its dayfile, so the port is run at that
  minute and the next one (the seed rounds on the seconds), with the day of
  year the machine's JDATE gave (DtCyber's NOS 1.3 reports 26/09/21 as day
  233, which tests\\cyber\\probe4.txt shows).  The one that matches is
  reported; if neither does, the session DIFFERS.

usage: python cmpcyber.py [cmds-file ...]
"""
import difflib
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))
START = ' THE BLACK WIZARD OF THE HIGH EAST TOWER WISHES TO KNOW'
SUBS = [('^*', '!'), (chr(92), '?'), ('%', ':')]
DAY = 233


def game(path):
    raw = open(path, encoding='latin-1').read().replace('\r\n', '\n')
    out, on = [], False
    for line in raw.split('\n'):
        line = line.rstrip()
        if not on:
            on = line == START
            if not on:
                continue
        if line.startswith('1 ') and 'COMPUTING CENTER' in line:
            break
        for a, b in SUBS:
            line = line.replace(a, b)
        out.append(line)
    while out and not out[-1]:
        out.pop()
    m = re.search(r'^ (\d\d)\.(\d\d)\.(\d\d)\.\$EXECUTE\.', raw, re.M)
    when = (int(m.group(1)), int(m.group(2))) if m else None
    return out, when


def port(cmds, day, hhmm):
    p = subprocess.run([os.path.join(PORT, 'advent.exe'), '--day', str(day),
                        '--time', '%04d' % hhmm],
                       input=cmds.encode('latin-1'), stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, cwd=PORT, timeout=120)
    got = [l.rstrip() for l in
           p.stdout.decode('latin-1').replace('\r\n', '\n').split('\n')]
    while got and not got[-1]:
        got.pop()
    return got


def main():
    sessions = sys.argv[1:] or sorted(glob.glob(os.path.join(HERE,
                                                             'cmds*.txt')))
    rc = 0
    for cmdfile in sessions:
        name = os.path.basename(cmdfile)[4:-4]
        ref = os.path.join(HERE, 'cyber%s.txt' % name)
        if not os.path.exists(ref):
            print('%-8s no reference print file' % name)
            continue
        cmds = open(cmdfile, encoding='latin-1').read()
        want, when = game(ref)
        if when is None:
            print('%-8s no $EXECUTE time in the dayfile' % name)
            rc = 1
            continue
        h, m = when
        tries = []
        for add in (0, 1):
            t = h * 60 + m + add
            tries.append((t // 60) * 100 + t % 60)
        best = None
        for hhmm in tries:
            got = port(cmds, DAY, hhmm)
            if got == want:
                best = hhmm
                break
        if best is not None:
            print('%-8s IDENTICAL (%d lines, day %d, %04d)'
                  % (name, len(want), DAY, best))
            continue
        got = port(cmds, DAY, tries[0])
        d = list(difflib.unified_diff(want, got, 'cyber', 'port',
                                      lineterm='', n=1))
        print('%-8s DIFFERS (%d cyber lines, %d port lines, day %d, %04d)'
              % (name, len(want), len(got), DAY, tries[0]))
        print('\n'.join(d[:60]))
        rc = 1
    return rc


if __name__ == '__main__':
    sys.exit(main())
