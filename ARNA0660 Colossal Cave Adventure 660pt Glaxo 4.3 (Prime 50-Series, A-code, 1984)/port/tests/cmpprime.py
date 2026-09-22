#!/usr/bin/env python3
"""Compare the port with the original running on PRIMOS 23.4.

primeN.txt beside this script is what the original (the tape's own
EXECUTIVE.SEG) printed on the real machine for the commands in cmdsN.txt,
captured over a telnet line by prime\\primeplay.py.  PRIMOS echoes what is
typed, so the port is run with --echo, which writes each line it reads
back the same way; the transcript's own "SEG EXECUTIVE" line before and
PRIMOS's "OK," after are taken off, and the rest is diffed line for line.

The sessions keep to what does not depend on the dice: the port's RND is a
stand-in until PRIMOS's RND$ is known (see ..\\README.md).

usage: python cmpprime.py [cmds-file ...]    (default: every cmds*.txt here)
"""
import difflib
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))


def prime_lines(text):
    lines = text.replace('\r', '').split('\n')
    k = next(i for i, l in enumerate(lines) if l.strip() == 'SEG EXECUTIVE')
    lines = lines[k + 1:]
    while lines and lines[-1].strip() in ('', 'OK,'):
        lines.pop()
    return [l.rstrip() for l in lines]


def port_lines(cmds, work):
    p = subprocess.run([os.path.join(work, 'adventure4.exe'), '--echo'],
                       input=('\n'.join(cmds) + '\n').encode('latin-1'),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       timeout=120)
    lines = p.stdout.decode('latin-1').replace('\r', '').split('\n')
    while lines and not lines[-1].strip():
        lines.pop()
    return [l.rstrip() for l in lines]


def scratch():
    """a copy of the port to run in: saved games go to its saves\\"""
    work = tempfile.mkdtemp(prefix='adv4cmp')
    for f in ('adventure4.exe', 'ADVINIT1.DAT', 'ADVINIT2.DAT',
              'ADVINIT3.DAT', 'ADVINIT4.DAT'):
        shutil.copy(os.path.join(PORT, f), work)
    return work


def main():
    sessions = sys.argv[1:] or sorted(glob.glob(os.path.join(HERE, 'cmds*.txt')))
    rc = 0
    work = scratch()
    try:
        for cmdfile in sessions:
            name = os.path.basename(cmdfile)[4:-4]
            ref = os.path.join(HERE, 'prime%s.txt' % name)
            cmds = open(cmdfile, encoding='latin-1').read().split('\n')
            while cmds and not cmds[-1]:
                cmds.pop()
            want = prime_lines(open(ref, encoding='latin-1').read())
            got = port_lines(cmds, work)
            if want == got:
                print('session %s: %d lines, identical' % (name, len(want)))
                continue
            rc = 1
            print('session %s: DIFFERS' % name)
            for l in difflib.unified_diff(want, got, 'prime', 'port',
                                          lineterm='', n=2):
                print('  ' + l)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return rc


if __name__ == '__main__':
    sys.exit(main())
