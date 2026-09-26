#!/usr/bin/env python3
"""Compare the port with the original running under NOS 1.3.

cyber1.txt and its friends are the print files of jobs that compiled
`adventure.src` with FTN 4.7 on a Cyber 173 under NOS 1.3 (DtCyber) and played
the commands in the matching cmds file - see cyber\\mkdeck.py.  Everything
outside the game's own output (the NOS banner, the loader map, the dayfile) is
cut away here, and the rest is diffed against the port playing the same
commands.

The jobs were built with mkdeck.py --seed, which is the one line of difference
from the tape: `CALL RANSET(SECOND(1.))` became `CALL RANSET(0.5)`, so that
the generator starts at a known place instead of at the CPU clock.  The port
is started at the same seed.

usage: python cmpcyber.py [cmds-file ...]
The default is every cmds*.txt beside this script.
"""
import difflib
import glob
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))
#  RANSET(0.5) leaves the seed at 04000000000000001 octal.
ARGS = ['--seed', '140737488355329']

START = ' WELCOME. PERCHANCE, ARE YOU A WIZARD?'


def game(path):
    """The game's own output, cut out of a NOS print file."""
    raw = open(path, encoding='latin-1').read().replace('\r\n', '\n')
    out, on = [], False
    for line in raw.split('\n'):
        line = line.rstrip()
        if not on:
            on = line == START.rstrip()
            if not on:
                continue
        if line.startswith('1 ') and 'COMPUTING CENTER' in line:
            break
        out.append(line)
    while out and not out[-1]:
        out.pop()
    return out


#  What probe.job printed on NOS 1.3 for the game's own RND, seeded so that
#  the generator stood at 04631463146314631 octal.
RND = [(30, 4), (79, 4), (15, 3), (65, 2), (75, 4), (20, 3), (51, 1), (90, 0)]


def rnd():
    """prnd.exe calls the game's RND with the seeds measured on the Cyber."""
    exe = os.path.join(PORT, '.build', 'prnd.exe')
    if not os.path.exists(exe):
        print('%-8s not built' % 'rnd')
        return 0
    out = subprocess.run([exe], stdout=subprocess.PIPE,
                         cwd=PORT, timeout=60).stdout.decode('latin-1')
    got = []
    for line in out.splitlines():
        f = line.split()
        if len(f) == 7 and f[1] == '1':          # seed 1, the measured one
            got.append((int(f[5]), int(f[6])))
    if got == RND:
        print('%-8s IDENTICAL (%d draws)' % ('rnd', 2 * len(got)))
        return 0
    print('%-8s DIFFERS' % 'rnd')
    print('  cyber %s' % (RND,))
    print('  port  %s' % (got,))
    return 1


def main():
    sessions = sys.argv[1:] or sorted(glob.glob(os.path.join(HERE,
                                                             'cmds*.txt')))
    rc = rnd()
    for cmdfile in sessions:
        name = os.path.basename(cmdfile)[4:-4]
        ref = os.path.join(HERE, 'cyber%s.txt' % name)
        if not os.path.exists(ref):
            print('%-8s no reference print file' % name)
            continue
        cmds = open(cmdfile, encoding='latin-1').read()
        want = game(ref)
        p = subprocess.run([os.path.join(PORT, 'advent.exe')] + ARGS,
                           input=cmds.encode('latin-1'),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           cwd=PORT, timeout=120)
        got = [l.rstrip() for l in
               p.stdout.decode('latin-1').replace('\r\n', '\n').split('\n')]
        while got and not got[-1]:
            got.pop()
        d = list(difflib.unified_diff(want, got, 'cyber', 'port',
                                      lineterm='', n=1))
        if d:
            print('%-8s DIFFERS (%d cyber lines, %d port lines)'
                  % (name, len(want), len(got)))
            print('\n'.join(d[:60]))
            rc = 1
        else:
            print('%-8s IDENTICAL (%d lines)' % (name, len(want)))
    return rc


if __name__ == '__main__':
    sys.exit(main())
