#!/usr/bin/env python3
"""Random commands, many games: no run-time error, no BUG, no hang.

    python tests\\fuzz.py [games] [--commands N]

Each game is N (400) commands of one or two words drawn from the game's
own vocabulary (section 4 of the database), with yes/no answers and junk
mixed in, fed at once to the port in a scratch copy, with its own --seed
and -u; every fifth game is --fresh.

abenteuer.exe must end each game by itself or at the end of its input,
with exit code 0 (BUG, the program's own FATAL ERROR, exits 1), and in
time.  The same game is then played by the bounds-checked build
(.build\\checked, made by build.sh), which stops at the first subscript
out of range: that is trouble unless it is one of the two the original
has itself, reading a neighbouring word of COMMON /PLACOM/ (ATLOC, LINK,
PLACE, FIXED, COND ... in that order, here as on the Harris):

  LINK(0)   8010  IF(ATLOC(LOC).EQ.0.OR.LINK(ATLOC(LOC)).NE.0) - FORTRAN
            does not stop at the .OR.; LINK(0) is ATLOC(150), and the
            condition is true either way
  COND(-1)  FORCED(LOC), BITSET(LOC,N) at the first move of a game set
            up from ADV.DATA: the database was read into LOC line by
            line, so LOC is the -1 that ends the last section; COND(-1)
            is FIXED(99), 0 - no object 99
"""
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
DB = os.path.join(PORT, '..', 'src_original', 'database')
MAIN_F = os.path.join(PORT, '.build', 'src', 'main.f')

# (array, index, text on the line the check stopped at)
ORIGINAL = [('link', 0, 'LINK(ATLOC(LOC))'),
            ('cond', -1, 'FORCED(LOC)=COND(LOC)'),
            ('cond', -1, 'BITSET(L,N)=KAND(COND(L)')]


def vocabulary():
    words = []
    for line in open(os.path.join(DB, 'TAPE4.txt'), encoding='latin-1'):
        w = line[8:].strip()
        if w and w.replace('*', '').isalnum():
            words.append(w.lower())
    return words


JUNK = ['ja', 'nein', 'j', 'n', 'xyzzy', 'plugh', 'hilfe', 'info', 'ende',
        'sichr', 'bring', 'magie modus', '', '12', '-1', '???', 'a b c d',
        'nimm alles', 'leg alles', 'schau', 'bestand', 'punkte']


def commands(rng, n, words):
    out = []
    for _ in range(n):
        r = rng.random()
        if r < 0.15:
            out.append(rng.choice(JUNK))
        elif r < 0.55:
            out.append(rng.choice(words))
        else:
            out.append(rng.choice(words) + ' ' + rng.choice(words))
    return out


def play(exe, opts, cmds):
    tmp = tempfile.mkdtemp(prefix='abenteuer-fuzz-')
    try:
        shutil.copy(exe, os.path.join(tmp, 'abenteuer.exe'))
        for f in ('ADV.DATA', 'NEUSPIEL.DAT'):
            shutil.copy(os.path.join(PORT, f), tmp)
        try:
            r = subprocess.run([os.path.join(tmp, 'abenteuer.exe')] + opts,
                               input=('\n'.join(cmds) + '\n').encode(),
                               capture_output=True, timeout=120)
            return r.returncode, r.stdout.decode('latin-1'), \
                r.stderr.decode('latin-1')
        except subprocess.TimeoutExpired:
            return 'hang', '', ''
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def original_site(err, source):
    m = re.search(r"At line (\d+) of file \S*main\.f\s+Fortran runtime error: "
                  r"Index '(-?\d+)' of dimension 1 of array '(\w+)'", err)
    if not m:
        return None
    line = source[int(m.group(1)) - 1]
    for array, index, text in ORIGINAL:
        if m.group(3) == array and int(m.group(2)) == index and text in line:
            return '%s(%d)' % (array.upper(), index)
    return None


def main():
    args = sys.argv[1:]
    games = int(args[0]) if args and args[0].isdigit() else 40
    n = int(args[args.index('--commands') + 1]) if '--commands' in args else 400
    plain = os.path.join(PORT, 'abenteuer.exe')
    checked = os.path.join(PORT, '.build', 'checked', 'abenteuer.exe')
    source = open(MAIN_F, encoding='latin-1').read().split('\n')
    words = vocabulary()
    bad = 0
    seen = {}
    whole = 0
    for g in range(games):
        rng = random.Random(g)
        opts = ['-u', '--seed', str(g + 1), '--date', '2026-09-20',
                '--time', '%02d:%02d' % (rng.randrange(24), rng.randrange(60))]
        if g % 5 == 4:
            opts.append('--fresh')
        cmds = commands(rng, n, words)
        code, out, err = play(plain, opts, cmds)
        if code != 0 or 'FATAL ERROR' in out or err:
            bad += 1
            print('game %d (%s): exit %s\n%s%s' % (
                g, ' '.join(opts), code,
                '\n'.join(out.rstrip().split('\n')[-10:]), err))
            continue
        code, out, err = play(checked, opts, cmds)
        if code == 0 and not err:
            whole += 1
            continue
        site = original_site(err, source)
        if site:
            seen[site] = seen.get(site, 0) + 1
            continue
        bad += 1
        print('game %d (%s), checked build: exit %s\n%s' % (
            g, ' '.join(opts), code, err))
    print('fuzz: %d games of %d commands, %d in trouble; the checked build '
          'played %d through, stopped at the original\'s own %s'
          % (games, n, bad, whole,
             ', '.join('%s %d times' % kv for kv in sorted(seen.items()))
             or 'nowhere'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
