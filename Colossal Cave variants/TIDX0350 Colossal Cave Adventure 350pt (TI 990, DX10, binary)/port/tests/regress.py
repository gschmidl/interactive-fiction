#!/usr/bin/env python3
"""Regression checks for the TI 990 Adventure port (run after build.sh).

    python regress.py
"""
import os
import shutil
import subprocess
import sys
import tempfile

import crossref
from common import EXE, run

FAILS = []


def check(what, ok, detail=''):
    print('%-58s %s' % (what, 'ok' if ok else 'FAILED'))
    if not ok:
        FAILS.append(what)
        if detail:
            print('    ' + detail.replace('\n', '\n    ')[:2000])


def options():
    p = subprocess.run([EXE, '--help'], capture_output=True, text=True)
    check('--help prints the usage and exits 0', p.returncode == 0 and 'Usage:' in p.stdout)
    for bad in (['--nope'], ['-x'], ['--clock=25:00:00'], ['--clock'], ['extra']):
        p = subprocess.run([EXE] + bad, capture_output=True, text=True)
        check('%s is refused (exit 2)' % ' '.join(bad), p.returncode == 2 and 'Try' in p.stderr,
              p.stderr)


def sessions():
    check('the three reference sessions replay identically', crossref.main() == 0)


def save_restore(tmp):
    walk = ['YES', 'my.sav', 'NO', 'IN', 'GET LAMP', 'SAVE', 'YES']
    out, rc = run(walk, ['--clock=2026-09-19 20:10:00'], cwd=tmp)
    check('SAVE suspends the game and ends it', rc == 0 and 'at least 90 minutes' in out
          and os.path.getsize(os.path.join(tmp, 'my.sav')) > 0, out[-600:])
    back = ['YES', 'my.sav', 'Y', 'NO', 'RESTORE', 'INVENTORY']
    out, rc = run(back, ['--clock=2026-09-19 20:30:00'], cwd=tmp)
    check('RESTORE 20 minutes later is refused', 'suspended a mere 20 minutes ago' in out, out[-600:])
    out, rc = run(back, ['--clock=2026-09-19 22:30:00'], cwd=tmp)
    check('RESTORE two hours later resumes the game', 'Brass lantern' in out
          and "You're inside Building." in out, out[-600:])
    out, rc = run(back, ['-u', '--clock=2026-09-19 20:11:00'], cwd=tmp)
    check('-u: RESTORE a minute later resumes the game', 'Brass lantern' in out, out[-600:])
    out, rc = run(['YES', 'my.sav', 'N', 'new.sav', 'NO', 'QUIT', 'YES'],
                  ['--clock=2026-09-19 20:12:00'], cwd=tmp)
    check('an existing file can be refused and another name given',
          '(DX10 Error 0026)   RE-ENTER SAVE/RESTORE PATHNAME' in out
          and os.path.exists(os.path.join(tmp, 'new.sav')), out[:800])


def hours():
    monday = ['--clock=2026-09-21 10:00:00']
    out, rc = run(['NO', 'NO', 'NO', 'NO'], monday)
    check('weekday 10:00: the cave is closed', 'Colossal Cave is closed' in out, out[:600])
    out, rc = run(['NO', 'NO', 'IN', 'QUIT', 'YES'], ['-u'] + monday)
    check('-u: weekday 10:00 the cave is open', 'closed' not in out
          and 'inside a building' in out, out[:600])


def ending():
    out, rc = run(['NO', 'NO', 'IN'], ['--clock=2026-09-19 20:10:00'])
    check('the end of the input ends the game quietly', rc == 0 and 'ERROR' not in out, out[-300:])
    out, rc = run(['NO', 'NO', 'QUIT', 'YES'], ['--clock=2026-09-19 20:10:00'])
    check('QUIT: STOP 1, NORMAL PROGRAM COMPLETION and the date', rc == 0
          and out.rstrip().endswith('NORMAL PROGRAM COMPLETION\n20:10:01 SATURDAY, SEP 19, 2026 (262)'),
          out[-300:])


def minute_zero():
    walk = ['NO', 'NO', 'IN', 'GET LAMP', 'OUT', 'S', 'S', 'S', 'UNLOCK GRATE', 'D', 'W',
            'ON', 'W', 'W', 'W', 'D', 'W', 'QUIT', 'YES']
    for flags in ([], ['--no-fixes']):
        out, rc = run(walk, flags + ['--clock=2026-09-19 20:00:00'], timeout=60)
        check('first random number at minute 0 %s' % ' '.join(flags or ['(fixed)']),
              rc == 0 and 'NORMAL PROGRAM COMPLETION' in out, out[-400:])


def main():
    if not os.path.exists(EXE):
        sys.exit('build the port first (build.sh)')
    tmp = tempfile.mkdtemp(prefix='adv990r-')
    try:
        options()
        sessions()
        save_restore(tmp)
        hours()
        ending()
        minute_zero()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('%d failed' % len(FAILS) if FAILS else 'all passed')
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
