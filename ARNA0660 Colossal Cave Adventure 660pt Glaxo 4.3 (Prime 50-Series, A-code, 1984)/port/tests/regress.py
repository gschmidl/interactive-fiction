#!/usr/bin/env python3
"""Checks of adventure4.exe, each in a scratch copy of the port (saved games
go to its saves\\, never the real one).

    regress.py

  prime      the six sessions recorded on PRIMOS 23.4 (cmpprime.py):
             identical line for line
  options    --help, an unknown option (exit 2), a missing ADVINIT file
             (exit 1)
  save       SAVE: asked to accept the 30-minute wait, a name; the file
             saves\\A_<login name>__<name>; the game ends
  restore    RESTORE of that game at once: "only a wizard can restart a
             game in less than 30 minutes"; with -u it is restored (the
             lamp in hand, back in the building) and the image kept or
             thrown away as asked; a name never saved
  seed       the same --seed gives the same game, deep in the cave where
             the dice are rolled; the default is always the same (the
             Prime's RND was seeded with the same number in every game)
  input      a 200-character line, the original's odd comma handling, the
             end of the input in the middle of a question (exit 0)
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))
FILES = ('adventure4.exe', 'ADVINIT1.DAT', 'ADVINIT2.DAT', 'ADVINIT3.DAT',
         'ADVINIT4.DAT')


class Scratch:
    def __enter__(self):
        self.dir = tempfile.mkdtemp(prefix='adv4reg')
        for f in FILES:
            shutil.copy(os.path.join(PORT, f), self.dir)
        return self

    def __exit__(self, *a):
        shutil.rmtree(self.dir, ignore_errors=True)

    def run(self, lines, *args, code=0):
        p = subprocess.run([os.path.join(self.dir, 'adventure4.exe'), '--echo']
                           + list(args),
                           input=('\n'.join(lines) + '\n').encode('latin-1'),
                           capture_output=True, timeout=120)
        if code is not None and p.returncode != code:
            raise AssertionError('exit %d (%s)' % (p.returncode,
                                                   p.stderr.decode('latin-1')))
        return p.stdout.decode('latin-1').replace('\r', '')

    def saves(self):
        d = os.path.join(self.dir, 'saves')
        return sorted(os.listdir(d)) if os.path.isdir(d) else []


def need(out, *texts):
    for t in texts:
        if t not in out:
            raise AssertionError('missing: %r' % t)


def refuse(out, *texts):
    for t in texts:
        if t in out:
            raise AssertionError('should not be there: %r' % t)


def t_prime():
    p = subprocess.run([sys.executable, os.path.join(HERE, 'cmpprime.py')],
                       capture_output=True, text=True, timeout=600)
    if p.returncode:
        raise AssertionError(p.stdout[-1500:])


def t_options():
    exe = os.path.join(PORT, 'adventure4.exe')
    p = subprocess.run([exe, '--help'], capture_output=True, text=True)
    if p.returncode or 'usage: adventure4' not in p.stdout:
        raise AssertionError('--help: %d %r' % (p.returncode, p.stdout))
    p = subprocess.run([exe, '--frobnicate'], capture_output=True, text=True)
    if p.returncode != 2 or 'unknown option --frobnicate' not in p.stderr:
        raise AssertionError('unknown option: %d %r' % (p.returncode, p.stderr))
    with Scratch() as s:
        os.remove(os.path.join(s.dir, 'ADVINIT4.DAT'))
        p = subprocess.run([os.path.join(s.dir, 'adventure4.exe')],
                           input=b'', capture_output=True)
        if p.returncode != 1 or b'ADVINIT4.DAT is missing' not in p.stderr:
            raise AssertionError('missing file: %d %r' % (p.returncode,
                                                          p.stderr))


def t_save():
    with Scratch() as s:
        out = s.run(['no', 'in', 'take lamp', 'save', 'maybe', 'yes', 'g1',
                     'look'])
        need(out, 'If you suspend your Adventure now, you will have to wait at '
             'least 30\nminutes before continuing.\nIs this acceptable?\nmaybe\n'
             'Please answer the question.\nIs this acceptable?\nyes\n'
             'Name to save under: g1\nOk.\n')
        refuse(out, '? look')
        names = s.saves()
        if len(names) != 1 or not (names[0].startswith('A_')
                                   and names[0].endswith('__g1')
                                   or names[0].endswith('_g1')):
            raise AssertionError('saves: %r' % names)


def t_restore():
    with Scratch() as s:
        s.run(['no', 'in', 'take lamp', 'save', 'yes', 'g1'])
        out = s.run(['no', 'restore', 'g1', 'look'])
        need(out, 'Name of saved game: g1\nI\'m sorry - only a wizard can '
             'restart a game in less than 30 minutes.')
        refuse(out, '? look')
        out = s.run(['no', 'restore', 'g1', 'yes', 'inventory', 'quit', 'y'],
                    '-u')
        need(out, 'Do you want me to keep the save-image?\nyes\nOk.\n\n'
             "You're inside building.\n",
             'Your posessions currently consist of:\nBrass lantern\n')
        if len(s.saves()) != 1:
            raise AssertionError('the kept image is gone: %r' % s.saves())
        out = s.run(['no', 'restore', 'g1', 'no', 'quit', 'y'], '-u')
        if s.saves():
            raise AssertionError('the image was to go: %r' % s.saves())
        out = s.run(['no', 'restore', 'nosuchgame', 'quit', 'y'])
        need(out, 'Name of saved game: nosuchgame\n'
             "I can't find any saved game for you to restore.\n")


CAVE = ['no', 'in', 'take keys', 'take lamp', 'out', 's', 's', 's',
        'unlock grate', 'open grate', 'd', 'on', 'w', 'take cage', 'w',
        'take rod', 'w', 'w', 'drop rod', 'take bird', 'take rod', 'w', 'd',
        'w', 'wave rod', 'w', 'e', 'd', 's', 'n', 'e', 'w', 'n', 's', 'd',
        'w', 'e', 'n', 'n', 'w', 'e', 'd', 'u', 'score', 'quit', 'y']


def t_seed():
    with Scratch() as s:
        a = s.run(CAVE)
        b = s.run(CAVE)
        c = s.run(CAVE, '--seed', '7')
        d = s.run(CAVE, '--seed', '7')
        if a != b:
            raise AssertionError('two plain runs differ')
        if c != d:
            raise AssertionError('two runs with --seed 7 differ')
        need(a, 'Hall of Mists', 'You have scored')


def t_input():
    with Scratch() as s:
        # the executive reads 139 characters of a line; a word it does not
        # know on its own is "Huh??".  READIN's comma: what follows it is
        # moved behind the line's first blank, so "get lamp, get keys" is
        # "get get keys" (session 3 on the Prime), and after "in," - no
        # blank before the comma - the rest is lost
        out = s.run(['no', 'x' * 200, 'in, take lamp', 'inventory',
                     'get lamp, get keys', 'quit', 'y'])
        need(out, '? ' + 'x' * 200 + '\n\nHuh??\n',
             "You're not carrying anything.",
             '? get lamp, get keys\n\nOk.\n\n\nDon\'t be ridiculous!\n')
        out = s.run(['no', 'quit'])
        need(out, 'Do you really want to quit now?\n')


def main():
    bad = 0
    for name, fn in [('prime', t_prime), ('options', t_options),
                     ('save', t_save), ('restore', t_restore),
                     ('seed', t_seed), ('input', t_input)]:
        try:
            fn()
            print('%-8s ok' % name)
        except (AssertionError, subprocess.TimeoutExpired) as e:
            bad += 1
            print('%-8s FAILED: %s' % (name, e))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
