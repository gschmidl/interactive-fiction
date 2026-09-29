#!/usr/bin/env python3
""""*** press <return> twice to continue ***" - fix 1.

    python tests\\returns.py

The instructions read their three pauses with GET SKIP LIST, which on the
Prime skips blank lines until it finds a word (measured on PRIMOS 23.4),
so the Returns alone never went on.  The port's fix 1 lets two blank
lines go on, as the text says, and a word still does; --no-fixes is the
Prime's rule.  Checked:

  1. two Returns at each pause reach "Please input a wierd positive number."
  2. a word at each pause does too (with and without the fix)
  3. with --no-fixes, blank lines alone never do
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(os.path.dirname(HERE), 'dungeon.exe')
SEED = 'Please input a wierd positive number.'


def run(lines, *args):
    r = subprocess.run([EXE] + list(args),
                       input=''.join(l + '\n' for l in lines).encode(),
                       capture_output=True, timeout=30)
    return r.stdout.decode('latin-1')


def main():
    checks = [
        ('two Returns at each pause',
         SEED in run(['yes'] + [''] * 6 + ['42', 'quit', 'yes'])),
        ('one Return does not go on',
         SEED not in run(['yes'] + [''] * 5)),
        ('a word at each pause',
         SEED in run(['yes', 'go', 'go', 'go', '42', 'quit', 'yes'])),
        ('--no-fixes: a word at each pause',
         SEED in run(['yes', 'go', 'go', 'go', '42', 'quit', 'yes'],
                     '--no-fixes')),
        ('--no-fixes: Returns alone never go on',
         SEED not in run(['yes'] + [''] * 30, '--no-fixes')),
    ]
    for name, ok in checks:
        print('%-40s %s' % (name, 'ok' if ok else 'FAILED'))
    bad = [n for n, ok in checks if not ok]
    print('returns: %s' % ('all ok' if not bad else '%d failed' % len(bad)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
