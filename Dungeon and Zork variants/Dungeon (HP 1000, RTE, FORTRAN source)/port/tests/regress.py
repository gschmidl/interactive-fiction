#!/usr/bin/env python3
"""Scripted checks of dungeon.exe, each in a scratch copy with its own copy
of the data base (never the real saves\\).  The clock is held (--time,
--date), so the dice are the same on every run.

    regress.py [exe]

  opening    the field, the mailbox, the leaflet (modified for HP1000 by
             Tom Hutchinson), VERSION, TIME, QUIT.
  save       SAVE under a name in one run, RESTORE it in the next: the
             leaflet is still carried.  (SAVE and RESTORE are segment E,
             which segment F reaches with DLIN2.)
  gdt        the game's debugger: the password, its commands, and the
             death of an impostor.
  win        the debugger sets the score to 490; the kitchen's 10 points,
             scored by the game, make the full 500 and start the end game
             herald, who comes 15 moves later.
  init       without @DUNGT and @DUNGI the game builds them again from
             @DUNGN, and plays.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))
CLOCK = ['--time', '10:00', '--date', '1982-11-18']
GDT = ['GUARDIAN', 'CHRIS,QUETZAL,27']
HOUSE = ['N', 'E', 'OPEN WINDOW', 'ENTER HOUSE']


def run(work, lines, args=()):
    p = subprocess.run([os.path.join(work, 'dungeon.exe')] + CLOCK +
                       list(args), input='\n'.join(lines) + '\n',
                       capture_output=True, text=True, encoding='latin-1',
                       timeout=60, cwd=work)
    if p.returncode != 0 or p.stderr.strip():
        raise AssertionError('exit %d: %s' % (p.returncode, p.stderr))
    return '\n'.join(l.rstrip() for l in p.stdout.split('\n'))


def need(out, *texts):
    for t in texts:
        if t not in out:
            raise AssertionError('missing: %r' % t)


def t_opening(work):
    out = run(work, ['OPEN MAILBOX', 'TAKE LEAFLET', 'READ LEAFLET',
                     'VERSION', 'TIME', 'QUIT', 'Y'])
    need(out, 'Welcome to Dungeon.                   This version created '
              '18-NOV-82.',
         'You are in an open field west of a big white house with a '
         'boarded',
         'Opening the mailbox reveals:', 'A leaflet.',
         'modified for HP1000 by Tom Hutchinson.', ' V3.0A',
         ' Elapsed playing time =  0 hours &   0 minutes.',
         'Do you wish to leave the game?')


def t_save(work):
    out = run(work, ['OPEN MAILBOX', 'TAKE LEAFLET', 'SAVE', 'GAME1'])
    need(out, 'ENTER FILE NAME: GAME1', 'Saved.')
    if not os.path.exists(os.path.join(work, 'saves', 'GAME1')):
        raise AssertionError('no saves\\GAME1')
    out = run(work, ['RESTORE', 'GAME1', 'INVENTORY'])
    need(out, 'Restored.', 'You are carrying:\nA leaflet.')
    out = run(work, ['RESTORE', 'NOSUCH', 'INVENTORY'])
    need(out, 'You are empty handed.')


def t_gdt(work):
    out = run(work, GDT + ['HE', 'DA', '1,1', 'EX', 'LOOK'])
    need(out, 'State your name, cat, and serial number.',
         ' At your service.', '   TK - Take.',
         ' AD#   ROOM  SCORE  VEHIC OBJECT ACTION  STREN  FLAGS')
    out = run(work, ['GUARDIAN', 'FRED', 'LOOK'])
    if 'At your service' in out:
        raise AssertionError('an impostor was let in')


def t_win(work):
    out = run(work, GDT + ['AA', '1,2', '490', 'EX'] + HOUSE +
              ['SCORE'] + ['LOOK'] * 16 + ['QUIT', 'Y'])
    need(out, ' Old=      0      New= 490',
         '     500 [total of   500 points]',
         'I welcome you to the ranks of the chosen of Zork.',
         'A complete modern Dungeon endgame')


def t_init(work):
    for f in ('@DUNGT', '@DUNGI'):
        os.remove(os.path.join(work, f))
    out = run(work, ['LOOK'])
    need(out, " CREATING NEW '@DUNGT'", " CREATING NEW '@DUNGI'",
         'There is a small mailbox here.')
    out = run(work, ['LOOK'])
    if 'CREATING' in out:
        raise AssertionError('built the data base twice')


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(PORT,
                                                             'dungeon.exe')
    bad = 0
    for name, fn in [('opening', t_opening), ('save', t_save),
                     ('gdt', t_gdt), ('win', t_win), ('init', t_init)]:
        work = tempfile.mkdtemp(prefix='dngtest')
        shutil.copy(exe, os.path.join(work, 'dungeon.exe'))
        for f in ('@DUNGN', '@DUNGT', '@DUNGI'):
            shutil.copy(os.path.join(os.path.dirname(exe), f),
                        os.path.join(work, f))
        try:
            fn(work)
            print('%-8s ok' % name)
        except (AssertionError, subprocess.TimeoutExpired) as e:
            bad += 1
            print('%-8s FAILED: %s' % (name, e))
        shutil.rmtree(work, ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
