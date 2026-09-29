#!/usr/bin/env python3
"""Random commands at both programs: they must neither crash nor hang.

    python tests\\fuzz.py [SESSIONS] [--moves N]

Each session runs in a scratch folder (SUSPEND writes NAME.adv to the
current folder), every other one with --no-fixes, on Pohl's and on
Daimler's program.  The words are the game's own (advword.h).  When a
session ends by SUSPEND, the saved game is restored with --restore and
played on with more random commands.  A run passes if the program ends by
itself, exit 0, nothing on stderr, within the time limit, and a restore
finds its saved game.  Daimler's LOG with an object ends his program
with "Fatal error number 39", exit 1 (Fix 4): with --no-fixes that is
what it should do, and it is not counted.
"""
import os
import random
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common as C  # noqa: E402


def one(variant, lines, args, cwd):
    try:
        out, err, rc = C.run(variant, lines, args, cwd=cwd, timeout=60)
    except subprocess.TimeoutExpired:
        return None, 'timed out', 'hang'
    return out, err, rc


def main():
    args = sys.argv[1:]
    sessions, moves = 100, 300
    while args:
        a = args.pop(0)
        if a == '--moves':
            moves = int(args.pop(0))
        else:
            sessions = int(a)
    bad = restored = 0
    tmp = tempfile.mkdtemp(prefix='advent-fuzz-')
    try:
        for variant in C.VARIANTS:
            words = C.vocab(variant)
            for k in range(sessions):
                rng = random.Random(k * 7 + (variant == 'daimler'))
                extra = ['--no-fixes'] if k % 2 else []
                cwd = tempfile.mkdtemp(dir=tmp)
                out, err, rc = one(variant, C.session(rng, words, moves), extra, cwd)
                if rc == 1 and extra and out and out.rstrip().endswith('Fatal error number 39'):
                    continue
                if rc != 0 or err:
                    bad += 1
                    print('%s session %d %s: exit %s %s' % (variant, k, ' '.join(extra), rc,
                                                            err.strip()[:200]))
                    continue
                saves = [n for n in os.listdir(cwd) if n.lower().endswith('.adv')]
                if saves:
                    name = saves[0][:-4]
                    more = [name] + C.session(rng, words, moves // 2)[1:]
                    out, err, rc = one(variant, more, extra + ['--restore'], cwd)
                    restored += 1
                    if rc == 1 and extra and out and out.rstrip().endswith('Fatal error number 39'):
                        continue
                    if rc != 0 or err or "can't open the file" in (out or '') or \
                            'Read error' in (out or ''):
                        bad += 1
                        print('%s session %d restore %s: exit %s %s %s' % (
                            variant, k, name, rc, err.strip()[:200], (out or '')[:120]))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('fuzz: %d sessions of %d commands per program (%d restored from SUSPEND), %d failed'
          % (sessions, moves, restored, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
