#!/usr/bin/env python3
"""Play random commands and report any run that ends badly: a non-zero
exit, anything on stderr (the emulated machine stopped: an instruction the
emulator lacks, a device fault), the game's FATAL ERROR or its attempt to
rebuild the database ("INITIALIZING..."), or a run that never ends.

Each game is up to three runs on one scratch pack, so that a SUSPENDed game
is resumed and a finished one is followed by a new one.

    python fuzz.py [GAMES] [TURNS] [SEED] [--real-clock]

--real-clock runs on the real 60 Hz clock, as run.bat does (not repeatable:
the random numbers then start from the time).
"""
import os
import random
import shutil
import subprocess
import sys
import tempfile

from common import EXE

WORDS = ('n s e w ne nw se sw u d in out enter back look inventory score quit '
         'get take drop throw open close unlock lock on off light extinguish '
         'lamp keys food bottle water oil cage bird rod wave zyxxy clunk xyzzy plugh '
         'plover fee fie foe foo feed kill attack dwarf snake troll bear chain axe gold '
         'diamonds silver jewelry coins chest eggs trident vase emerald pyramid '
         'pearl rug spices nugget clam oyster magazine pillow grate bridge fill '
         'pour drink eat say read find where info help yes no hours suspend '
         'building road forest valley stream cave slab plant beanstalk climb '
         'jump cross swim break blast brief magic mode wizard').split()


def random_lines(rng, turns):
    lines = [rng.choice(['NO', 'YES'])]
    for _ in range(turns):
        k = rng.random()
        if k < 0.75:
            lines.append(' '.join(rng.choice(WORDS) for _ in range(rng.choice((1, 1, 2)))))
        elif k < 0.9:
            lines.append(rng.choice(['yes', 'no', 'y', 'n']))
        else:
            lines.append(''.join(rng.choice('abcdefghijklmnopqrstuvwxyz ') for _ in
                                 range(rng.randint(0, 30))))
    return lines


def one_run(lines, pack, real=False):
    data = ''.join(ln + '\n' for ln in lines).encode()
    clock = [] if real else ['--fixed-clock']
    try:
        p = subprocess.run([EXE] + clock + ['-u', '--pack=' + pack], input=data,
                           capture_output=True, timeout=120)
    except subprocess.TimeoutExpired:
        return 'timed out', ''
    out = p.stdout.decode('latin-1')
    err = p.stderr.decode('latin-1')
    if p.returncode != 0 or err or 'FATAL ERROR' in out or 'INITIALIZING' in out:
        return 'rc %d\n%s' % (p.returncode, out[-700:] + err), out
    return None, out


def main():
    real = '--real-clock' in sys.argv[1:]
    argv = [a for a in sys.argv[1:] if a != '--real-clock']
    games = int(argv[0]) if len(argv) > 0 else 100
    turns = int(argv[1]) if len(argv) > 1 else 300
    seed = int(argv[2]) if len(argv) > 2 else 1
    rng = random.Random(seed)
    bad = runs = 0
    for i in range(games):
        tmp = tempfile.mkdtemp(prefix='advp7f-')
        pack = os.path.join(tmp, 'fuzz.pack')
        try:
            for k in range(3):
                lines = random_lines(rng, turns)
                err, out = one_run(lines, pack, real)
                runs += 1
                if err:
                    bad += 1
                    print('game %d run %d: %s' % (i, k, err))
                    open('fuzz-fail-%d-%d.in' % (i, k), 'w').write('\n'.join(lines) + '\n')
                    break
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    print('%d games, %d runs, %d bad' % (games, runs, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
