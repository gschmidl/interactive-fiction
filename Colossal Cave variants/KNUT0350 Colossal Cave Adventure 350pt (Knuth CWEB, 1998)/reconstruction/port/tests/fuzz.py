#!/usr/bin/env python3
"""Random commands at the port: it must neither crash nor hang.

    python tests\\fuzz.py [SESSIONS] [--moves N]

The words are the game's own (the new_word calls of advent.w).  A command
is one word, two words, or now and then three, an empty line, a line far
longer than the 72-byte buffer, or y/n for the questions.  Each session
has dice of its own (--seed), and every other one runs with --no-fixes.
A session passes if the program ends by itself, exit 0, within the time
limit.
"""
import os
import random
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common as C  # noqa: E402

WORDS = [w for w in C.VOCAB if w.isalpha()]


def session(rng, moves):
    out = ['no' if rng.random() < 0.7 else 'yes']
    for _ in range(moves):
        r = rng.random()
        if r < 0.35:
            out.append(rng.choice(WORDS))
        elif r < 0.8:
            out.append('%s %s' % (rng.choice(WORDS), rng.choice(WORDS)))
        elif r < 0.85:
            out.append(' '.join(rng.choice(WORDS) for _ in range(3)))
        elif r < 0.9:
            out.append(rng.choice(['', ' ', 'y', 'n', 'yes', 'no']))
        elif r < 0.95:
            out.append(' '.join(rng.choice(WORDS) for _ in range(rng.randint(10, 40))))
        else:
            out.append(rng.choice(WORDS) * rng.randint(10, 30))
    if rng.random() < 0.5:
        out += ['quit', 'y']
    return out


def main():
    args = sys.argv[1:]
    sessions, moves = 100, 400
    while args:
        a = args.pop(0)
        if a == '--moves':
            moves = int(args.pop(0))
        else:
            sessions = int(a)
    bad = 0
    for k in range(sessions):
        rng = random.Random(k)
        cmds = session(rng, moves)
        extra = ['--no-fixes'] if k % 2 else []
        try:
            out, err, rc = C.run(cmds, ['--seed', str(k * 101 + 1)] + extra, timeout=60)
        except subprocess.TimeoutExpired:
            out, err, rc = '', 'timed out', None
        if rc != 0 or err:
            bad += 1
            print('session %d %s: exit %s %s' % (k, ' '.join(extra), rc, err.strip()[:200]))
    print('fuzz: %d sessions of %d commands, %d failed' % (sessions, moves, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
