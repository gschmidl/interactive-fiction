#!/usr/bin/env python3
"""advent.exe against a second build of the same program.

    python tests\\crosscheck.py [SESSIONS] [--moves N]

The second build is the same C, from the same ctangle run (.build/advent.c,
so run build.sh first), compiled by gcc on Linux in WSL Debian with
AddressSanitizer and UndefinedBehaviorSanitizer.  Each session is a list of
random commands as tests\\fuzz.py makes them, with now and then a line of
8-bit bytes or with a NUL in it, run on both builds with the same --seed:

    session 0, 4, 8 ...   the fixes
    session 1, 5, 9 ...   --no-fixes
    session 2, 6, 10 ...  --debug, and random # commands among the others,
                          numbers out of range included
    session 3, 7, 11 ...  --debug --no-fixes, the same

A session passes if the two transcripts are the same (line ends aside), both
programs exit 0, and neither writes to stderr: a sanitizer report fails it.
When a session fails, the scratch folder with both transcripts is kept and
its name printed.
"""
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common as C  # noqa: E402
import fuzz  # noqa: E402

CSRC = os.path.join(C.PORT, '.build', 'advent.c')
WSL = ['wsl', '-d', 'Debian']
GCC = ['gcc', '-std=gnu89', '-funsigned-char', '-O1', '-g', '-fno-omit-frame-pointer',
       '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-w',
       '-o', 'advent-linux', 'advent.c']
# every byte but the line end, carriage return and Ctrl-Z (end of file to a
# Windows program reading text)
ODD = [chr(b) for b in range(256) if b not in (0x0a, 0x0d, 0x1a)]
VARIABLES = ['tally', 'lost', 'dflag', 'clock1', 'clock2', 'deaths', 'bonus', 'limit']
# setarch -R: without address randomisation.  gcc 12's AddressSanitizer
# and the WSL2 kernel's 32 bits of mmap randomness do not get on: now and
# then a run loops for ever printing "AddressSanitizer:DEADLYSIGNAL".
RUNNER = '''\
export ASAN_OPTIONS=detect_leaks=0
export UBSAN_OPTIONS=print_stacktrace=1
while read k args; do
  timeout 60 setarch "$(uname -m)" -R ./advent-linux $args < in/$k.txt > out/$k.txt 2> err/$k.txt
  echo $? > rc/$k.txt
done < sessions.txt
'''


def odd_line(rng):
    if rng.random() < 0.5:
        w = rng.choice(fuzz.WORDS)
        return w + '\0' + rng.choice(fuzz.WORDS)
    return ''.join(rng.choice(ODD) for _ in range(rng.randint(1, 90)))


def debug_line(rng):
    def num(a, b):
        return str(rng.randint(a, b))

    def obj():
        return num(-2, 70) if rng.random() < 0.7 else rng.choice(fuzz.WORDS)
    r = rng.random()
    if r < 0.1:
        return '#show'
    if r < 0.25:
        return '#loc ' + num(-5, 150)
    if r < 0.35:
        return '#where ' + obj()
    if r < 0.55:
        return '#put %s %s' % (obj(), num(-3, 150))
    if r < 0.75:
        return '#prop %s %s' % (obj(), num(-3, 8))
    if r < 0.95:
        return '#set %s %s' % (rng.choice(VARIABLES), num(-5, 40))
    return rng.choice(['#', '#frob', '#set', '#put 2', '#loc', '#set frob 1'])


def session(k, moves):
    rng = random.Random(1000 + k)
    lines = fuzz.session(rng, moves)
    debug = k % 4 >= 2
    out = [lines[0]]
    for line in lines[1:]:
        if debug and rng.random() < 0.15:
            out.append(debug_line(rng))
        out.append(odd_line(rng) if rng.random() < 0.03 else line)
    args = ['--seed', str(k * 7919 + 13)]
    if k % 2:
        args.append('--no-fixes')
    if debug:
        args.append('--debug')
    return out, args


def wsl(args, cwd):
    return subprocess.run(WSL + ['--cd', cwd, '-e'] + args, capture_output=True)


def first_difference(a, b):
    la, lb = a.split('\n'), b.split('\n')
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else '(end)'
        y = lb[i] if i < len(lb) else '(end)'
        if x != y:
            return 'line %d:\n      windows: %r\n      linux:   %r' % (i + 1, x, y)
    return 'none'


def main():
    args = sys.argv[1:]
    sessions, moves = 60, 400
    while args:
        a = args.pop(0)
        if a == '--moves':
            moves = int(args.pop(0))
        else:
            sessions = int(a)
    if not os.path.exists(CSRC):
        print('no %s: run build.sh first' % CSRC)
        return 1
    tmp = tempfile.mkdtemp(prefix='advent-cross-')
    shutil.copy(CSRC, tmp)
    for d in ('in', 'out', 'err', 'rc'):
        os.mkdir(os.path.join(tmp, d))
    r = wsl(GCC, tmp)
    if r.returncode != 0:
        print('gcc in WSL Debian failed:\n' + (r.stdout + r.stderr).decode('utf-8', 'replace'))
        return 1

    runs = [session(k, moves) for k in range(sessions)]
    with open(os.path.join(tmp, 'sessions.txt'), 'w', newline='\n') as f:
        for k, (lines, a) in enumerate(runs):
            with open(os.path.join(tmp, 'in', '%d.txt' % k), 'wb') as g:
                g.write(''.join(l + '\n' for l in lines).encode('latin-1'))
            f.write('%d %s\n' % (k, ' '.join(a)))
    with open(os.path.join(tmp, 'run.sh'), 'w', newline='\n') as f:
        f.write(RUNNER)
    wsl(['sh', 'run.sh'], tmp)

    bad = 0
    for k, (lines, a) in enumerate(runs):
        try:
            out, err, rc = C.run(lines, a, timeout=60)
        except subprocess.TimeoutExpired:
            out, err, rc = '', 'timed out', None
        with open(os.path.join(tmp, 'out', '%d.win.txt' % k), 'w', encoding='latin-1',
                  newline='\n') as f:
            f.write(out)

        def linux(d):
            with open(os.path.join(tmp, d, '%d.txt' % k), 'rb') as f:
                return f.read().decode('latin-1')
        lout, lerr, lrc = linux('out'), linux('err'), linux('rc').strip()
        problems = []
        if rc != 0 or err:
            problems.append('windows: exit %s %s' % (rc, err.strip()[:300]))
        if lrc != '0' or lerr:
            problems.append('linux: exit %s %s' % (lrc, lerr.strip()[:1500]))
        if out != lout:
            problems.append('transcripts differ at ' + first_difference(out, lout))
        if problems:
            bad += 1
            print('session %d (%s):\n    %s' % (k, ' '.join(a), '\n    '.join(problems)))
    print('crosscheck: %d sessions of %d commands, %d failed' % (sessions, moves, bad))
    if bad:
        print('transcripts kept in %s' % tmp)
    else:
        shutil.rmtree(tmp, ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
