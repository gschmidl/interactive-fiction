"""Regression tests for the QUEST port: scripted games with the clock counted in instructions.

    python regress.py            run the tests
    python regress.py --record   write reference/walk.out from the port as it is now

  1. Options: --help; unknown and invalid options, missing and bad values exit 2 with a
     message.
  2. The walk (reference/walk.in: signing on, HELP, looking, walking, talking, QUIT) gives
     reference/walk.out, and the same again when run twice.
  3. The cave is closed on a weekday at 10:00: without -u the port says so and exits 1; with
     -u the operator gives the site's password and the game plays.
  4. --easy: QUEST reads the 'EASY' library, and its world differs from the hard one's.
  5. The end of stdin in the middle of a game ends the run (exit 0).
"""
import os
import subprocess
import sys

from common import EXE, REFERENCE, reference, run

FAILED = []


def check(ok, what, detail=''):
    print('%s  %s' % ('ok   ' if ok else 'FAIL ', what))
    if not ok:
        FAILED.append(what)
        if detail:
            print('      ' + detail.replace('\n', '\n      '))


def options():
    p = subprocess.run([EXE, '--help'], capture_output=True, text=True)
    check(p.returncode == 0 and '--players=N' in p.stdout and '--unlimited' in p.stdout,
          '--help prints the options and exits 0')
    for args, words in ((['--bogus'], "unrecognized option '--bogus'"),
                        (['-x'], "invalid option -- 'x'"),
                        (['--players'], 'option requires an argument'),
                        (['--players=7'], 'the number of players must be 1 to 6'),
                        (['-p', '0'], 'the number of players must be 1 to 6'),
                        (['--port=http'], 'the port must be 1 to 65535'),
                        (['--easy=yes'], 'does not take an argument'),
                        (['stray'], "unexpected argument 'stray'")):
        p = subprocess.run([EXE] + args, capture_output=True, text=True, stdin=subprocess.DEVNULL)
        check(p.returncode == 2 and words in p.stderr and '--help' in p.stderr,
              '%s: exit 2, "%s"' % (' '.join(args), words), 'exit %d: %s' % (p.returncode, p.stderr))


def walk(record):
    with open(os.path.join(REFERENCE, 'walk.in')) as f:
        lines = f.read().splitlines()
    code, out, err = run(lines)
    if record:
        with open(os.path.join(REFERENCE, 'walk.out'), 'w', encoding='latin-1', newline='\n') as f:
            f.write(out)
        print('wrote reference/walk.out (%d lines)' % out.count('\n'))
        return
    want = reference('walk.out')
    check(code == 0 and out == want, 'the walk gives reference/walk.out',
          'exit %d, stderr %r; first difference at line %d' % (
              code, err, next((i for i, (a, b) in enumerate(zip(out.split('\n'), want.split('\n')))
                               if a != b), min(out.count('\n'), want.count('\n'))) + 1))
    code2, out2, _ = run(lines)
    check(code2 == 0 and out2 == out, 'the walk run twice gives the same transcript')
    for words in ('What is your first name?', 'You walk north.', 'You quit.'):
        check(words in out, 'the transcript has "%s"' % words)


def closed_cave():
    lines = ['Heino', 'yes', 'look', 'quit', 'yes']
    env = {'QUEST_TIME': '10,00,240380'}                # a Monday, 10:00
    code, out, err = run(lines, env=env)
    check(code == 1 and 'The cave is closed now' in err and not out.strip(),
          'on a weekday at 10:00 the cave is closed (exit 1)', 'exit %d: %r %r' % (code, err, out))
    code, out, err = run(lines, ['-u'], env=env)
    check(code == 0 and 'You quit.' in out, '-u gives the password, and the game plays',
          'exit %d: %r' % (code, err))
    code, out, err = run(lines, env={'QUEST_TIME': '10,00,220380'})     # a Saturday
    check(code == 0 and 'You quit.' in out, 'on a Saturday at 10:00 the cave is open',
          'exit %d: %r' % (code, err))


def easy():
    lines = ['Heino', 'yes', 'look', 'quit', 'yes']
    code, hard, _ = run(lines)
    code2, easy_out, err = run(lines, ['--easy'])
    first = [l for l in easy_out.split('\n') if l.startswith('You are ')][:1]
    check(code == 0 and code2 == 0 and easy_out != hard and first,
          '--easy: the easy library\'s world is another one (%s)' % (first[0] if first else '-'),
          'exit %d: %r' % (code2, err))


def early_end():
    code, out, err = run(['Heino', 'yes', 'look'])
    check(code == 0 and ' LOOK' in out, 'the end of stdin in a game ends the run (exit 0)',
          'exit %d: %r' % (code, err))


def main():
    if '--record' in sys.argv[1:]:
        walk(True)
        return 0
    options()
    walk(False)
    closed_cave()
    easy()
    early_end()
    print('%d failed' % len(FAILED) if FAILED else 'all passed')
    return 1 if FAILED else 0


if __name__ == '__main__':
    sys.exit(main())
