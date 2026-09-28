#!/usr/bin/env python3
"""Random-command runs of adv462 on scratch copies.

Each run plays two sessions with the clock set (ADV462_CLOCK), so a run
repeats exactly: 5 to 80 random commands drawn from the game's own
vocabulary, then SUSPEND; then a new process that RESTOREs that game and
plays on for --moves commands.  A run
fails if a process crashes (exit code not 0), hangs (timeout), or stops with
the game's own "Fatal error" (its BUG routine).

    python fuzz.py [--runs N] [--moves M] [--seed S] [--out DIR]
        generate the runs and play them with ../adv462.exe
    python3 fuzz.py --replay DIR --exe PROGRAM --tag NAME
        play the same runs with another build (tests/linux.sh does this with
        a Linux build of the same source)
    python fuzz.py --compare DIR [--tag NAME]
        compare the transcripts of the two

DIR (default ../.build/fuzz) gets in/NNN.clock, NNN.1, NNN.2 (the input of
the two sessions) and <tag>/NNN.txt (the transcripts; the Windows ones are
tagged win).
"""
import argparse
import os
import random
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)


def vocabulary(path):
    """Section 4 of the database: (code, word) for every word."""
    words, sect, want_sect = [], None, True
    with open(path) as f:
        for line in f:
            line = line.rstrip('\n')
            if want_sect:
                sect, want_sect = int(line), False
                continue
            if line.strip() == '-1':
                want_sect = True
                continue
            if sect == 4:
                # format(i8,5a1): anything after the word is a comment
                words.append((int(line[:8]), line[8:13].strip()))
    return words


def commands(rng, words, n, saves=True):
    by_type = {t: [w for c, w in words if c // 1000 == t] for t in range(4)}
    motion, obj, action, special = (by_type[t] for t in range(4))
    out = []
    for _ in range(n):
        r = rng.random()
        if r < 0.30:
            out.append(rng.choice(motion))
        elif r < 0.58:
            out.append(rng.choice(action) + ' ' + rng.choice(obj + ['all']))
        elif r < 0.70:
            out.append(rng.choice(action))
        elif r < 0.78:
            out.append(rng.choice(['yes', 'no', 'y', 'n', '']))
        elif r < 0.83:
            out.append(rng.choice(obj))
        elif r < 0.88:
            out.append(rng.choice(special))
        elif r < 0.91:
            out.append(rng.choice(['fast', 'full', 'listen', 'turns', '.',
                                   'hours', 'score', 'take all', 'drop all']))
        elif r < 0.93:
            if saves:
                out.append(rng.choice(['restore', 'suspend']) + ' ' +
                           rng.choice(['a', 'b', 'fz', 'con', 'x.y', 'c:d']))
        elif r < 0.96:
            n = rng.choice([69, 70, 71, 75, 120])
            out.append(''.join(rng.choice('abcdefghijklmnop qrstuvwxyz.,!?')
                               for _ in range(n)))
        else:
            out.append(' '.join(rng.choice(motion + obj + action)
                                for _ in range(rng.randint(2, 4))))
    return out


def generate(k, seed, moves, words, indir):
    rng = random.Random(seed * 100003 + k)
    clock = '%d %d' % (rng.randint(17000, 18700), rng.randint(0, 1439))
    # The first session is short and makes no SUSPEND or RESTORE of its own,
    # or most would end (death, QUIT, the wizard's questions after a RESTORE
    # that finds nothing) before its SUSPEND, leaving nothing to restore.
    first = [rng.choice(['no', 'no', 'no', 'yes'])]
    first += commands(rng, words, rng.randint(5, 80), saves=False)
    first += ['suspend fz', 'yes']
    second = ['no', 'restore fz'] + commands(rng, words, moves) + ['quit', 'yes']
    for ext, text in (('clock', clock), ('1', '\n'.join(first)),
                      ('2', '\n'.join(second))):
        with open(os.path.join(indir, '%03d.%s' % (k, ext)), 'w', newline='\n') as f:
            f.write(text + '\n')


def play(program, k, d, tag, timeout=60):
    """Play run k of d/in with program; write d/tag/NNN.txt; return problems."""
    name = '%03d' % k
    with open(os.path.join(d, 'in', name + '.clock')) as f:
        clock = f.read().strip()
    saves = os.path.join(d, 'saves-' + tag, name)
    os.makedirs(saves, exist_ok=True)
    env = dict(os.environ, ADV462_CLOCK=clock, ADV462_SAVEDIR=saves)
    text, problems = [], []
    for part in ('1', '2'):
        with open(os.path.join(d, 'in', name + '.' + part), 'rb') as f:
            inp = f.read()
        try:
            p = subprocess.run([program], input=inp, capture_output=True,
                               env=env, cwd=os.path.dirname(program),
                               timeout=timeout)
        except subprocess.TimeoutExpired:
            problems.append('session %s: timeout' % part)
            continue
        out = p.stdout.decode('latin-1').replace('\r\n', '\n')
        text.append('=== session %s (clock %s)\n' % (part, clock) + out)
        if p.returncode != 0:
            problems.append('session %s: exit code %d' % (part, p.returncode))
        if 'Fatal error' in out:
            tail = out[out.index('Fatal error'):].split('\n')
            problems.append('session %s: %s' % (part, ' '.join(tail[:3])))
    with open(os.path.join(d, tag, name + '.txt'), 'w', newline='\n') as f:
        f.write(''.join(text))
    return problems


def replay(d, program, tag):
    os.makedirs(os.path.join(d, tag), exist_ok=True)
    runs = sorted(int(n[:3]) for n in os.listdir(os.path.join(d, 'in'))
                  if n.endswith('.clock'))
    bad = 0
    for k in runs:
        problems = play(program, k, d, tag)
        if problems:
            bad += 1
            print('run %03d: %s' % (k, '; '.join(problems)))
    print('%s: %d runs, %d with problems' % (tag, len(runs), bad))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--runs', type=int, default=100)
    ap.add_argument('--moves', type=int, default=300)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--out', default=os.path.join(PORT, '.build', 'fuzz'))
    ap.add_argument('--replay', metavar='DIR')
    ap.add_argument('--exe', metavar='PROGRAM')
    ap.add_argument('--tag', default='linux')
    ap.add_argument('--compare', metavar='DIR')
    a = ap.parse_args()

    if a.replay:
        if not a.exe:
            ap.error('--replay needs --exe')
        # absolute: the program runs in its own folder
        return 1 if replay(os.path.abspath(a.replay), os.path.abspath(a.exe),
                           a.tag) else 0

    if a.compare:
        names = sorted(os.listdir(os.path.join(a.compare, 'win')))
        diff = []
        for n in names:
            with open(os.path.join(a.compare, 'win', n)) as f1, \
                    open(os.path.join(a.compare, a.tag, n)) as f2:
                if f1.read() != f2.read():
                    diff.append(n)
        print('%d runs compared, %d differ %s' % (len(names), len(diff), diff[:20]))
        return 1 if diff else 0

    # A scratch copy of the program: it changes to its own folder, and the
    # port folder's adventure.newgame must not be touched.
    game = os.path.join(a.out, 'game')
    shutil.rmtree(a.out, ignore_errors=True)
    for sub in ('game', 'in', 'win'):
        os.makedirs(os.path.join(a.out, sub))
    for f in ('adv462.exe', 'adventure.data', 'adventure.newgame'):
        shutil.copy(os.path.join(PORT, f), game)
    words = vocabulary(os.path.join(PORT, 'adventure.data'))
    for k in range(a.runs):
        generate(k, a.seed, a.moves, words, os.path.join(a.out, 'in'))
    bad = replay(a.out, os.path.join(game, 'adv462.exe'), 'win')
    with open(os.path.join(game, 'adventure.newgame'), 'rb') as f1, \
            open(os.path.join(PORT, 'adventure.newgame'), 'rb') as f2:
        if f1.read() != f2.read():
            print('adventure.newgame was changed by a run')
            bad += 1
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
