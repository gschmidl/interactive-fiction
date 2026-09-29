#!/usr/bin/env python3
"""fuzz.py - throw random commands at the range-checked debug build.

Every run gets its own scratch save directory (never port/save) and a frozen
clock, so a failure is reproducible from the seed alone:

    python test/fuzz.py [first-seed [count [commands-per-run]]] [--deep]

--deep also uses the --debug test bench to teleport, grab objects, move
dwarves and run the closing clocks down, so the far end of the cave, the troll,
the bear and the repository get the same treatment as the well house.

A run fails if the game dies with a run-time error instead of ending or
reaching end of input.  Failing inputs are kept in test/out/fuzz-<seed>.txt.
"""
import os, random, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'build', 'dbg', 'adventure-dbg.exe')
DATA = os.path.join(PORT, 'data')
OUT = os.path.join(HERE, 'out')

def vocabulary():
    words, sec = [], 1
    for ln in open(os.path.join(DATA, 'ADVENTURE.DAT'), encoding='latin-1'):
        ln = ln.rstrip('\r\n')
        if ln.strip() == '-1':
            sec += 1
            continue
        if sec == 7:
            w = ln.lstrip('0123456789')
            if w: words.append(w.strip())
    return words

def script(rng, words, n, deep=False):
    motion = ['n', 's', 'e', 'w', 'ne', 'nw', 'se', 'sw', 'u', 'd', 'in', 'out', 'xyzzy', 'plugh', 'plover']
    lines = ['no', rng.choice(['no', 'no', 'yes'])]
    lines += ['enter', 'take lamp', 'take keys', 'xyzzy', 'on', 'take rod', 'e', 'take cage', 'w', 'w', 'w', 'drop rod',
              'take bird', 'take rod', 'w', 'd', 'd'] if rng.random() < 0.7 else []
    for _ in range(n):
        r = rng.random()
        if deep and rng.random() < 0.12:
            lines.append(rng.choice([
                '#goto %d' % rng.randint(1, 140), '#goto %d' % rng.randint(1, 140),
                '#take %d' % rng.randint(1, 64), '#take %d' % rng.randint(1, 64),
                '#put %d %d' % (rng.randint(1, 64), rng.randint(0, 140)),
                '#dwarf %d %d' % (rng.randint(1, 6), rng.randint(0, 140)),
                '#set DFLAG %d' % rng.randint(0, 4),
                '#set CLOCK1 %d' % rng.randint(0, 3), '#set CLOCK2 %d' % rng.randint(0, 3),
                '#set LIMIT %d' % rng.randint(0, 40), '#set TALLY %d' % rng.randint(0, 2)]))
            continue
        if r < 0.45: lines.append(rng.choice(motion))
        elif r < 0.55: lines.append(rng.choice(['yes', 'no']))
        elif r < 0.80: lines.append(rng.choice(words) + ' ' + rng.choice(words))
        elif r < 0.95: lines.append(rng.choice(words))
        else: lines.append(rng.choice(['', ' ', 'x' * 80, 'take ' + 'y' * 70, '1234', '?', 'say xyzzy', 'fee', 'fie', 'foe', 'foo']))
    return lines

def main():
    deep = '--deep' in sys.argv
    argv = [a for a in sys.argv if a != '--deep']
    first = int(argv[1]) if len(argv) > 1 else 1
    count = int(argv[2]) if len(argv) > 2 else 50
    n = int(argv[3]) if len(argv) > 3 else 400
    words = vocabulary()
    os.makedirs(OUT, exist_ok=True)
    bad = 0
    for seed in range(first, first + count):
        rng = random.Random(seed)
        lines = script(rng, words, n, deep)
        save = tempfile.mkdtemp(prefix='advpas-fuzz-')
        try:
            env = dict(os.environ, ADVPAS_DATA=DATA, ADVPAS_SAVE=save)
            t = '%02d%02d%02d' % (rng.randrange(24), rng.randrange(60), rng.randrange(60))
            p = subprocess.run([EXE, '-u', '-d', '28-OCT-1980', '-t', t] + (['--debug'] if deep else []), input='\n'.join(lines) + '\n',
                               capture_output=True, text=True, encoding='latin-1', env=env, timeout=120)
        except subprocess.TimeoutExpired:
            p = None
        finally:
            shutil.rmtree(save, ignore_errors=True)
        ok = p is not None and p.returncode == 0 and 'Runtime error' not in p.stderr and 'exception' not in p.stderr
        if not ok:
            bad += 1
            with open(os.path.join(OUT, 'fuzz-%s%d.txt' % ('deep-' if deep else '', seed)), 'w', encoding='latin-1') as fh:
                fh.write('\n'.join(lines) + '\n')
            print('seed %d%s (-t %s): %s' % (seed, ' --deep' if deep else '', t, 'TIMEOUT' if p is None else 'exit %d' % p.returncode))
            if p is not None:
                print('   ' + '\n   '.join(p.stderr.strip().splitlines()[:8]))
    print('%d runs, %d failed' % (count, bad))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
