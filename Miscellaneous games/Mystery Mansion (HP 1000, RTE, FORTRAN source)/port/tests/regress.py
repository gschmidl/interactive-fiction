#!/usr/bin/env python3
"""Scripted checks of mmm.exe, each in a scratch copy (never the real
saves\\).  The clock is held (--time, --date), so the mystery, the dice
and the creator's challenge number are the same on every run.

    regress.py [exe]

  solve      Wolpert's own display for the creator (DISPLAY, then YES and
             99 less the number it asks with) puts the murder weapon in
             the booty, the murderer in the murder room and the player
             next door; one step in solves the mystery (+200 points).
  suspend    SUSPEND on cartridge 5, RESTORE in a new run: the booty and
             the hook come back.
  site       --site: outside the playing hours a security code is asked
             for; -u passes one; the name, the player log and the
             comments reach cartridge MM, and the creator's display
             shows them.
  record     RECORD on the player's own terminal (fix 1: it used to write
             its message for ever), RECORD ON CASSETTE and FROM CASSETTE.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))
CLOCK = ['--time', '20:00', '--date', '2026-09-19']


def run(work, lines, args=()):
    p = subprocess.run([os.path.join(work, 'mmm.exe')] + CLOCK + list(args),
                       input='\n'.join(lines) + '\n', capture_output=True,
                       text=True, encoding='latin-1', timeout=30, cwd=work)
    if p.returncode != 0 or p.stderr.strip():
        raise AssertionError('exit %d: %s' % (p.returncode, p.stderr))
    return p.stdout


def need(out, *texts):
    for t in texts:
        if t not in out:
            raise AssertionError('missing: %r' % t)


def t_solve(work):
    out = run(work, [
        'GET LANTERN', 'LIGHT LANTERN', 'DISPLAY', 'YES 60',
        '1', '10,25219', '6,20100', '0',      # the knife and a compass
        '11', '3,902',                        # the butler to room 2
        '13', '1,5', '30,0', '0',             # the player to room 5
        '0', 'LOOK', 'S', 'SCORE', 'QUIT', 'YES'])
    need(out, ' 39 ARE YOU THE CREATOR?',
         'MROOM  MWHO  MWEP', 'YOU ARE IN THE CREEPY CRYPT',
         '     2     3    10   .',
         'CONGRATULATIONS! YOU SOLVED THE MYSTERY BY HAVING THE KNIFE',
         'SO FAR YOU HAVE SCORED 249 POINTS.')


def t_suspend(work):
    run(work, ['GET LANTERN', 'SUSPEND', '5', 'GAM1'])
    if not os.path.exists(os.path.join(work, 'saves', 'CR5', 'MMGAM1')):
        raise AssertionError('no saves\\CR5\\MMGAM1')
    out = run(work, ['RESTORE', '5', 'GAM1', 'LIST', 'LOOK', 'QUIT', 'YES'])
    need(out, 'YOU ARE NOW IN THE SAME SITUATION',
         'YOUR BOOTY CONTAINS:\n A BATTERY LANTERN',
         'THERE IS A EMPTY SHINY BRASS HOOK')


def t_site(work):
    out = run(work, ['LOOK'], ['--site'])
    need(out, 'SECURITY CODE?', 'WRONG SECURITY CODE.')
    out = run(work, ['WILMA', 'QUIT', 'YES', 'FINE GAME', ''],
              ['--site', '-u'])
    need(out, 'WHAT IS YOUR NAME? WILMA', 'HI WILMA', 'GOOD BYE WILMA',
          "PLEASE ENTER ANY ONE LINE COMMENTS")
    for f in ('MMMC', 'MMTLAN', 'MMTLDA'):
        if not os.path.exists(os.path.join(work, 'saves', 'CRMM', f)):
            raise AssertionError('no cartridge MM file ' + f)
    out = run(work, ['FRED', 'DISPLAY', 'YES 47', '14', '1', '0',
                     'QUIT', 'YES', ''], ['--site', '-u'])
    need(out, 'MYSTERY MANSION DATA', '15815  WILMA', '2000  FINE GAME')


def t_record(work):
    out = run(work, ['RECORD', '1', 'LOOK', 'QUIT', 'YES'])
    need(out, 'RECORDED ON LU #  1 FOR THE REST OF THE GAME.',
         ' >LOOK', 'GOOD BYE GUY!')
    run(work, ['RECORD ON CASSETTE', '5', 'GET LANTERN', 'LIST',
               'RECORD', '6', 'LOOK', 'QUIT', 'YES'])
    lu5 = open(os.path.join(work, 'saves', 'LU5.txt'),
               encoding='latin-1').read().split()
    if lu5[:3] != ['GET', 'LANTERN', 'LIST']:
        raise AssertionError('LU5.txt: %r' % lu5[:6])
    lu6 = open(os.path.join(work, 'saves', 'LU6.txt'),
               encoding='latin-1').read()
    need(lu6, 'RECORDED ON LU #  6', 'YOU ARE AT THE MAIN GATE')
    # a tape of two commands; its end hands the game back the terminal
    with open(os.path.join(work, 'saves', 'LU5.txt'), 'w') as f:
        f.write('GET LANTERN\nLIST\n')
    out = run(work, ['RECORD FROM CASSETTE', '5', 'DROP LANTERN', 'LIST',
                     'QUIT', 'YES'])
    need(out, '>GET LANTERN', 'A BATTERY LANTERN', '>**',
         'YOU NO LONGER HAVE THE LANTERN', 'GOOD BYE GUY!')


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(PORT, 'mmm.exe')
    bad = 0
    for name, fn in [('solve', t_solve), ('suspend', t_suspend),
                     ('site', t_site), ('record', t_record)]:
        work = tempfile.mkdtemp(prefix='mmmtest')
        shutil.copy(exe, os.path.join(work, 'mmm.exe'))
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
