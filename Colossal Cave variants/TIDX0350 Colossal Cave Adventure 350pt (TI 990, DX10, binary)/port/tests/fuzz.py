#!/usr/bin/env python3
"""Play random commands and report any game that ends badly: a non-zero
exit, anything on stderr (a DX10 call the port lacks, a fault of the
emulated processor, a stuck task), or a FORTRAN run-time error message.

    python fuzz.py [GAMES] [TURNS] [SEED]
"""
import random
import subprocess
import sys

from common import EXE

WORDS = ('n s e w ne nw se sw u d in out enter back look inventory score quit '
         'get take drop throw open close unlock lock on off light extinguish '
         'lamp keys food bottle water oil cage bird rod wave xyzzy plugh plover '
         'fee fie foe foo feed kill attack dwarf snake troll bear chain axe gold '
         'diamonds silver jewelry coins chest eggs trident vase emerald pyramid '
         'pearl rug spices nugget clam oyster magazine pillow grate bridge fill '
         'pour drink eat say read find where info help save yes no restore '
         'building road forest valley stream cave slab plant beanstalk climb '
         'jump cross swim break blast hours brief suspend magic mode wizard').split()


def one_game(rng, turns, i):
    lines = ['NO', rng.choice(['NO', 'YES'])]
    for _ in range(turns):
        k = rng.random()
        if k < 0.75:
            lines.append(' '.join(rng.choice(WORDS) for _ in range(rng.choice((1, 1, 2)))))
        elif k < 0.9:
            lines.append(rng.choice(['yes', 'no', 'y', 'n']))
        else:
            lines.append(''.join(rng.choice('abcdefghijklmnopqrstuvwxyz ') for _ in
                                 range(rng.randint(0, 30))))
    clock = '2026-09-%02d %02d:%02d:%02d' % (rng.randint(1, 30), rng.randint(0, 23),
                                            rng.randint(0, 59), rng.randint(0, 59))
    args = ['-u', '--clock=' + clock]
    data = ''.join(ln + '\n' for ln in lines).encode()
    try:
        p = subprocess.run([EXE] + args, input=data, capture_output=True, timeout=120)
    except subprocess.TimeoutExpired:
        return 'game %d (%s): timed out' % (i, clock), lines
    out = p.stdout.decode('latin-1')
    if p.returncode != 0 or p.stderr or ' ERROR AT >' in out:
        tail = out[-700:] + p.stderr.decode('latin-1')
        return 'game %d (%s): rc %d\n%s' % (i, clock, p.returncode, tail), lines
    return None, lines


def main():
    games = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    turns = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)
    bad = 0
    for i in range(games):
        err, lines = one_game(rng, turns, i)
        if err:
            bad += 1
            print(err)
            open('fuzz-fail-%d.in' % i, 'w').write('\n'.join(lines) + '\n')
    print('%d games, %d bad' % (games, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
