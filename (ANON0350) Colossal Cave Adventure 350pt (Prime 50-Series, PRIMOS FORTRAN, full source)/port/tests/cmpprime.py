#!/usr/bin/env python3
"""Compare the port with the original running on PRIMOS 23.4.

prime1.txt and its friends in this folder are what the original printed on the
real machine, captured over a telnet line by prime\\primeplay.py (see
..\\README.md).  The transcript carries PRIMOS's own echo of every command and,
at the end, its "**** STOP" and "OK," - this script takes those out, runs the
same commands here, and diffs what is left.

usage: python cmpprime.py [cmds-file ...]
The default is every cmds*.txt beside this script.
"""
import difflib
import glob
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))
# The sessions are played with the clock and the generator held still so that
# a run here is reproducible.  The original is seeded from its own clock, so
# the commands keep out of the cave, where nothing the game prints depends on
# the generator.
ARGS = ['--seed', '1', '--day', '17800', '--time', '1']


def norm(lines, cmds):
    """Drop PRIMOS's echo of the commands, its R ADVENTURE and its exit."""
    out, want = [], list(cmds)
    for l in lines:
        l = l.rstrip()
        if l in ('R ADVENTURE', '**** STOP', 'OK,'):
            continue
        if want and l == want[0]:
            want.pop(0)
            continue
        out.append(l)
    if want:
        print('  (never saw the echo of %r)' % want[0])
    while out and not out[-1]:
        out.pop()
    return out


def main():
    sessions = sys.argv[1:] or sorted(
        glob.glob(os.path.join(HERE, 'cmds*.txt')))
    rc = 0
    for cmdfile in sessions:
        name = os.path.basename(cmdfile)[4:-4]
        ref = os.path.join(HERE, 'prime%s.txt' % name)
        cmds = [l.rstrip() for l in
                open(cmdfile, encoding='latin-1').read().split('\n')]
        while cmds and not cmds[-1]:
            cmds.pop()
        want = norm(open(ref, encoding='latin-1').read().split('\n'), cmds)
        p = subprocess.run([os.path.join(PORT, 'advent.exe')] + ARGS,
                           input=('\n'.join(cmds) + '\n').encode('latin-1'),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           cwd=PORT, timeout=120)
        got = [l.rstrip() for l in
               p.stdout.decode('latin-1').replace('\r\n', '\n').split('\n')]
        while got and not got[-1]:
            got.pop()
        d = list(difflib.unified_diff(want, got, 'prime', 'port',
                                      lineterm='', n=1))
        if d:
            print('%-8s DIFFERS (%d prime lines, %d port lines)'
                  % (name, len(want), len(got)))
            print('\n'.join(d[:60]))
            rc = 1
        else:
            print('%-8s IDENTICAL (%d lines)' % (name, len(want)))
    return rc


if __name__ == '__main__':
    sys.exit(main())
