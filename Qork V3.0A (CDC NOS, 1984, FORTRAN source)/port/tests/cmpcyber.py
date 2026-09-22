#!/usr/bin/env python3
"""Compare the port with Qork V3.0A running on a Cyber under NOS 2.8.7.

    python tests\\cmpcyber.py [N ...]

tests\\cmdsN.txt are the commands of a session and tests\\cyberN.txt the
print file the job came back with (tests\\cyber\\mkdeck.py and job287.js
made it; the deck compiles qork.src with FTN5 exactly as it is, builds the
database with sense switch 1 off and plays with it on).  The game's
output is cut out of the print file - from "RESTORED." to the page line
that ends the job - and compared with the port playing the same commands
in a scratch copy of itself.

Two things are the printer's, not the program's, and are taken off the
Cyber's side: the blank NOS adds to a line of odd length (records are
kept in pairs of characters), and the 6/12 codes - ^C is the lower case c
an ASCII terminal showed, which the port prints.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)


def cyber_game(path):
    lines = open(path, 'rb').read().decode('latin-1').replace('\r\n', '\n').split('\n')
    start = next(i for i, l in enumerate(lines) if l.rstrip() == ' RESTORED.')
    end = next(i for i in range(start, len(lines)) if lines[i].startswith('1 '))
    out = []
    for l in lines[start:end]:
        l = re.sub(r'\^([A-Z])', lambda m: m.group(1).lower(), l)
        out.append(l.rstrip())
    while out and not out[-1]:
        out.pop()
    return out


def port_game(cmds):
    tmp = tempfile.mkdtemp(prefix='qork-')
    try:
        for f in ('qork.exe', 'qork.dat', 'qork.ini'):
            shutil.copy(os.path.join(PORT, f), tmp)
        r = subprocess.run([os.path.join(tmp, 'qork.exe')], input=cmds,
                           capture_output=True, timeout=120)
        out = r.stdout.decode('latin-1').replace('\r\n', '\n').split('\n')
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    out = [l.rstrip() for l in out]
    while out and not out[-1]:
        out.pop()
    return out


def main():
    names = sys.argv[1:] or sorted(
        f[4:-4] for f in os.listdir(HERE) if re.match(r'cmds\w+\.txt$', f))
    bad = 0
    for n in names:
        ref = cyber_game(os.path.join(HERE, 'cyber%s.txt' % n))
        got = port_game(open(os.path.join(HERE, 'cmds%s.txt' % n), 'rb').read())
        if ref == got:
            print('%-8s IDENTICAL (%d lines)' % (n, len(ref)))
            continue
        bad += 1
        k = next((i for i in range(min(len(ref), len(got))) if ref[i] != got[i]),
                 min(len(ref), len(got)))
        print('%-8s DIFFERS at line %d of %d/%d' % (n, k + 1, len(ref), len(got)))
        for i in range(max(0, k - 2), min(k + 3, max(len(ref), len(got)))):
            print('   cyber %s' % (ref[i] if i < len(ref) else '<end>'))
            print('   port  %s' % (got[i] if i < len(got) else '<end>'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
