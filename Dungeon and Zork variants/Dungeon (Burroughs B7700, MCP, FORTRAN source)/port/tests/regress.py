#!/usr/bin/env python3
"""Scripted checks of dungeon.exe, each in a scratch copy with its own copy
of the data base and its own saves folder (never the real ones).  The clock
is held (--fixed-clock), so the dice are the same on every run.

    python regress.py            (from anywhere)
    python regress.py --record   rewrite reference/walk.out from this build

  options    --help, and GNU-style refusals of bad options (exit 2)
  walk       reference/walk.in gives reference/walk.out, twice
  save       SAVE in one run, RESTORE in the next: the lamp is still carried
             and the player still in the cellar
  gdt        the game's debugger: the password, HE, and the impostor's end
  init       without ZORK_PTXT and ZORK_PINDX the game builds them again
             from ZORK_DBTXT, and plays
  eof        the end of the input ends the game (exit 0)
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))
REF = os.path.join(HERE, 'reference')
FILES = ['dungeon.exe', 'ZORK_DBTXT', 'ZORK_PTXT', 'ZORK_PINDX']
failed = 0


def scratch():
    work = tempfile.mkdtemp(prefix='bdung-')
    for f in FILES:
        shutil.copy(os.path.join(PORT, f), work)
    return work


def run(work, lines, args=('--fixed-clock',), code=0):
    p = subprocess.run([os.path.join(work, 'dungeon.exe'), '--saves=' + os.path.join(work, 'saves')]
                       + list(args), input=''.join(l + '\n' for l in lines),
                       capture_output=True, text=True, encoding='latin-1', timeout=120, cwd=work)
    if p.returncode != code:
        raise AssertionError('exit %d (not %d): %s' % (p.returncode, code, p.stderr[-400:]))
    return '\n'.join(l.rstrip() for l in p.stdout.split('\n')), p.stderr


def check(ok, what):
    global failed
    print('%-6s %s' % ('ok' if ok else 'FAIL', what))
    if not ok:
        failed += 1


def need(out, *texts):
    missing = [t for t in texts if t not in out]
    if missing:
        raise AssertionError('missing: %r' % missing)


def t_options(work):
    out, err = run(work, [], ['--help'])
    check('--fixed-clock' in out and '--saves=DIR' in out, '--help prints the options and exits 0')
    for args, msg in ((['--bogus'], "unrecognized option '--bogus'"),
                      (['-x'], "invalid option -- 'x'"),
                      (['stray'], "unexpected argument 'stray'"),
                      (['--saves='], "option '--saves' requires a directory")):
        out, err = run(work, [], args, code=2)
        check(msg in err, '%s: exit 2, "%s"' % (' '.join(args), msg))


def t_walk(work, record):
    walk = open(os.path.join(REF, 'walk.in'), encoding='latin-1').read().split('\n')
    walk = [l for l in walk if l.strip()]
    out, err = run(work, walk)
    if record:
        with open(os.path.join(REF, 'walk.out'), 'w', encoding='latin-1', newline='\n') as f:
            f.write(out)
        print('wrote reference/walk.out (%d lines)' % out.count('\n'))
        return
    want = open(os.path.join(REF, 'walk.out'), encoding='latin-1', newline='\n').read()
    check(out == want, 'the walk gives reference/walk.out')
    out2, err = run(scratch(), walk)
    check(out2 == out, 'the walk run again gives the same transcript')


def t_save(work):
    house = ['NORTH', 'EAST', 'OPEN WINDOW', 'WEST', 'WEST', 'TAKE LAMP', 'MOVE RUG',
             'OPEN TRAP DOOR', 'TURN ON LAMP', 'DOWN', 'SAVE']
    out, err = run(work, house)
    need(out, 'The door crashes shut, and you hear someone barring it.', 'Saved.')
    check(os.path.exists(os.path.join(work, 'saves', 'ZORK_SAVDATA')), 'SAVE writes saves/ZORK_SAVDATA')
    out, err = run(work, ['RESTORE', 'INVENTORY', 'LOOK'])
    need(out, 'Restored.', 'A lamp.', 'You are in a dark and damp cellar')
    check(True, 'RESTORE in the next run: the lamp is carried, the player is in the cellar')


def t_gdt(work):
    out, err = run(work, ['GUARDIAN', 'CHRIS,QUETZAL,27', 'HE', 'EX', 'LOOK'])
    need(out, 'At your service.', 'AA- Alter ADVS', 'TK- Take.', 'You are in an open field')
    check(True, 'GUARDIAN with the password: the debugger, its commands, back to the game')
    out, err = run(work, ['GUARDIAN', 'SOMEONE,ELSE', 'N'])
    need(out, 'wrong, cretin!', 'Do you wish me to try to patch you?')
    check(True, 'GUARDIAN with a wrong password: the impostor turns to dust')


def t_init(work):
    os.remove(os.path.join(work, 'ZORK_PTXT'))
    os.remove(os.path.join(work, 'ZORK_PINDX'))
    out, err = run(work, ['OPEN MAILBOX'])
    need(out, "CREATING NEW 'ZORK/PTXT'", 'INITIALIZING SECTION #  14', "CREATING NEW 'ZORK/PINDX'",
         'Welcome to Dungeon.', 'Opening the mailbox reveals:')
    check(os.path.exists(os.path.join(work, 'ZORK_PINDX')),
          'without its data base the game builds it again from ZORK_DBTXT, and plays')


def t_eof(work):
    out, err = run(work, ['LOOK'])
    check('You are in an open field' in out, 'the end of the input ends the game (exit 0)')


def main():
    record = '--record' in sys.argv[1:]
    tests = [('walk', lambda w: t_walk(w, record))]
    if not record:
        tests = [('options', t_options)] + tests + [('save', t_save), ('gdt', t_gdt),
                                                    ('init', t_init), ('eof', t_eof)]
    for name, fn in tests:
        work = scratch()
        try:
            fn(work)
        except AssertionError as e:
            check(False, '%s: %s' % (name, e))
        finally:
            shutil.rmtree(work, ignore_errors=True)
    print('%d failed' % failed if failed else 'all passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
