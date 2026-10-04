#!/usr/bin/env python3
"""Random commands through a pipe: dunnet.exe must neither crash nor hang.

    python tests\\fuzz.py [runs] [commands per run]

Each run starts at the console (so the TOPS-20 part is reached) and then
types random verbs, objects, directions, TOPS-20 words, punctuation, ESC and
?.  A run passes when the program ends by itself at the end of its input
with exit code 0, within the time limit.  Lisp errors are counted (the
original has some; they only end the command)."""
import os
import random
import subprocess
import sys

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(PORT, 'dunnet.exe')

WALK = ['take lamp', 'nw', 'nw', 'a special phrase', 'se', 'se', 's', 'take chaos', 'n',
        'ne', 'ne', 'n']
WORDS = ('n s e w ne se nw sw u d up down north south east west l look score take get i '
         'invent inventory drop throw a special phrase consol console off superb brief on put '
         'light exting extinguish dig read examine examin wave eat lamp mirror shovel gold food '
         'chaos net connection silicon chip symbol floor beam all in nugget battery quit '
         'information directory telnet tn ftp return connect exit arpanet chaosnet dungeon '
         'dragon endgame mit-sally login luser resul send bye lamp.obj ~c').split()
NOISE = ['?', '\x1b', '.', ',', '(', ')', "'", '"', '/', '|', ';', '\x08', '\x15', '123', '7.',
         'LAMP', 'TaKe', '  ']


def command(rng):
    n = rng.choice([1, 1, 2, 2, 3, 4])
    parts = []
    for _ in range(n):
        parts.append(rng.choice(NOISE) if rng.random() < 0.15 else rng.choice(WORDS))
    sep = '' if rng.random() < 0.1 else ' '
    return sep.join(parts)


def main():
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    per = int(sys.argv[2]) if len(sys.argv) > 2 else 150
    bad = 0
    errors = 0
    for r in range(runs):
        rng = random.Random(r)
        lines = WALK + ['console'] + [command(rng) for _ in range(per)]
        for opts in ([], ['--no-fixes']):
            data = ('\n'.join(lines) + '\n').encode('latin-1')
            try:
                p = subprocess.run([EXE] + opts, input=data, capture_output=True, timeout=120)
            except subprocess.TimeoutExpired:
                print('run %d %s: HANG' % (r, opts))
                bad += 1
                continue
            out = p.stdout.decode('latin-1')
            errors += out.count('\n;')
            if p.returncode != 0 or 'PDL OVERFLOW' in out:
                print('run %d %s: exit %d%s' % (r, opts, p.returncode,
                                                ' PDL OVERFLOW' if 'PDL OVERFLOW' in out else ''))
                bad += 1
    print('%d runs, %d bad, %d Lisp errors reported' % (runs * 2, bad, errors))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
