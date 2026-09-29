#!/usr/bin/env python3
"""Random play of volcano.exe: each run starts somewhere (the slopes, or one
of the author's warps), then types random moves - mostly the game's own
letters, some words and junk, control characters, bytes above 127 - under a
fixed --seed and a timeout; every fifth run types only junk, at the shaft.
A run must end at one of the game's endings or when its moves run out, with
exit code 0 and nothing on stderr.  An exit code 1 is a BASIC error stop
("... ERROR IN LINE n"), 3 the emulator stopping on something it does not
model.

    fuzz.py [runs [moves]] [--exe volcano.exe] [--no-fixes]
"""

import os
import random
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, '..', 'volcano.exe')

ENDINGS = {
    'headhunters': 'EXPENSE.  TOUGH LUCK FOR YOU.....',
    'pit': 'UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK',
    'orcs': 'BEFORE DINING ON YOUR PUNY BODY... TOUGH LUCK.',
    'prison': '.... SOME HUMANOID, SOME INDETERMINATE.',
}
LETTERS = ['U', 'D', 'B', 'R', 'F', 'T', 'L']
WORDS = ['LIFT', 'INVENTORY', 'MAZE1', 'TRAPDOOR', 'PIT', 'SHAFT', 'GO', 'N',
         'u', 'd', 'inventory', '', ' U', 'U ', 'A,B', '"U"', 'U:D']
STARTS = [['U', 'D', 'D'], ['U', 'D', 'D', 'D'], ['MAZE1'], ['TRAPDOOR', ''],
          ['SHAFT'], []]


def junk(r):
    n = r.randint(1, 40)
    s = ''.join(chr(r.randint(32, 126)) for _ in range(n))
    if r.random() < 0.3:            # control characters, rubout, high bytes
        k = r.randint(0, len(s))
        s = s[:k] + r.choice(['\x08', '\x7f', '\x07', '\x09', '\x15', '\x18',
                              '\x1b', '\x00', '\xe7', '\x87', '\xa4',
                              '\x80', '\x01']) + s[k:]
    return s


def move(r):
    x = r.random()
    if x < 0.70:
        return r.choice(LETTERS)
    if x < 0.90:
        return r.choice(WORDS)
    return junk(r)


def one(seed, moves, extra):
    r = random.Random(seed)
    if seed % 5 == 4:
        # every fifth run types only junk at the shaft, where anything but
        # D asks again: HYBASIC's line input takes all of it
        cmds = ['SHAFT'] + [junk(r) for _ in range(moves)]
        cmds = [c for c in cmds if c.strip('\x00').upper() != 'D']
    else:
        cmds = list(r.choice(STARTS)) + [move(r) for _ in range(moves)]
    data = ('\n'.join(cmds) + '\n').encode('latin-1')
    try:
        p = subprocess.run([EXE, '--seed', str(seed % 1000)] + extra,
                           input=data, capture_output=True, timeout=60)
    except subprocess.TimeoutExpired:
        return 'TIMEOUT', 0, cmds
    out = p.stdout.decode('latin-1').replace('\r', '')
    prompts = out.count('MOVE? ')
    if p.returncode or p.stderr.strip():
        return ('exit %d %s' % (p.returncode, p.stderr.decode('latin-1')
                                .strip() or out[-200:])), prompts, cmds
    for name, text in ENDINGS.items():
        if out.rstrip('\n').endswith(text):
            return name, prompts, cmds
    return 'moves ran out', prompts, cmds


def main():
    global EXE
    args = sys.argv[1:]
    extra = []
    if '--no-fixes' in args:
        args.remove('--no-fixes')
        extra.append('--no-fixes')
    if '--exe' in args:
        i = args.index('--exe')
        EXE = args[i + 1]
        del args[i:i + 2]
    runs = int(args[0]) if args else 200
    moves = int(args[1]) if len(args) > 1 else 300
    tally = {}
    prompts = 0
    bad = 0
    for seed in range(runs):
        how, n, cmds = one(seed, moves, extra)
        prompts += n
        tally[how if not how.startswith(('exit', 'TIMEOUT')) else 'FAILED'] = \
            tally.get(how if not how.startswith(('exit', 'TIMEOUT'))
                      else 'FAILED', 0) + 1
        if how.startswith(('exit', 'TIMEOUT')):
            bad += 1
            print('seed %d: %s after %d moves; the moves: %r'
                  % (seed, how, n, cmds[:n + 2]))
    print('%d runs, %d moves read: %s'
          % (runs, prompts, ', '.join('%s %d' % kv
                                      for kv in sorted(tally.items()))))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
