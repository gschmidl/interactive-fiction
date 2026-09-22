#!/usr/bin/env python3
"""Random commands at the port: it must neither crash nor hang.

    python tests\\fuzz.py [SESSIONS] [--moves N] [--seed S]
    python tests\\fuzz.py --write FILE [--moves N] [--seed S]

The words are the game's own, taken from the vocabulary DATA statements of
qork.src (verbs, adjectives, objects, prepositions, directions, buzz words).
A command is a direction, a verb, a verb and an object, or a verb, an object,
a preposition and an object, now and then with an adjective or a word of
nonsense; some are in lower case (6/12 input).  Each session is a scratch
copy of the port playing N commands with dice of its own (--seed), then
QUIT and Y; it passes if the program exits 0 within the time limit with no
runtime error.

--write FILE writes one session's commands in upper case, for the Cyber
(its card reader has no lower case): tests\\cmdsfuzz.txt is one, and
cmpcyber.py holds the port to what the Cyber printed for it.
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
sys.path.insert(0, os.path.join(PORT, 'src'))
import convert          # noqa: E402  (its reader of qork.src)


def vocabulary():
    words = {}
    for stmt in convert.statements(convert.source()):
        m = re.match(r'\s*DATA\s+([BPDAVO])VOC\w*\s*/', stmt[6:])
        if m and not convert.is_comment(stmt):
            # "WORD", not the octal O"2000" of a direction's code
            words.setdefault(m.group(1), []).extend(
                w for w in re.findall(r'[,/\s]"([^"]*)"', stmt[6:]) if w.strip())
    return {k: sorted(set(v)) for k, v in words.items()}


def session(rng, voc, moves):
    out = []
    for _ in range(moves):
        r = rng.random()
        if r < 0.25:
            cmd = rng.choice(voc['D'])
        elif r < 0.35:
            cmd = rng.choice(voc['V'])
        elif r < 0.75:
            cmd = '%s %s' % (rng.choice(voc['V']), rng.choice(voc['O']))
        elif r < 0.9:
            cmd = '%s %s %s %s' % (rng.choice(voc['V']), rng.choice(voc['O']),
                                   rng.choice(voc['P']), rng.choice(voc['O']))
        else:
            cmd = ' '.join(rng.choice(voc['A'] + voc['B'] + ['XYZZY', 'FOO'])
                           for _ in range(rng.randint(1, 4)))
        if rng.random() < 0.1:
            cmd = '%s %s' % (rng.choice(voc['A']), cmd)
        out.append(cmd)
    return out + ['QUIT', 'Y']


def play(cmds, seed):
    tmp = tempfile.mkdtemp(prefix='qork-fuzz-')
    try:
        for f in ('qork.exe', 'qork.dat', 'qork.ini'):
            shutil.copy(os.path.join(PORT, f), tmp)
        r = subprocess.run([os.path.join(tmp, 'qork.exe'), '--seed', str(seed)],
                           input='\n'.join(cmds).encode('latin-1') + b'\n',
                           capture_output=True, timeout=60)
        return r.returncode, r.stdout.decode('latin-1'), r.stderr.decode('latin-1')
    except subprocess.TimeoutExpired:
        return None, '', 'timed out'
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    args = sys.argv[1:]
    moves, seed, write, sessions = 300, 1, None, 20
    while args:
        a = args.pop(0)
        if a == '--moves':
            moves = int(args.pop(0))
        elif a == '--seed':
            seed = int(args.pop(0))
        elif a == '--write':
            write = args.pop(0)
        elif a.isdigit():
            sessions = int(a)
        else:
            sys.exit(__doc__)
    voc = vocabulary()
    if write:
        cmds = session(random.Random(seed), voc, moves)
        open(write, 'w', newline='\n').write('\n'.join(cmds) + '\n')
        print('fuzz: %d commands -> %s' % (len(cmds), write))
        return 0
    bad = 0
    for k in range(sessions):
        rng = random.Random(seed * 1000 + k)
        cmds = session(rng, voc, moves)
        cmds = [c.lower() if rng.random() < 0.2 else c for c in cmds]
        dice = rng.randint(1, 2 ** 40)
        code, out, err = play(cmds, dice)
        if code != 0 or 'runtime error' in (out + err).lower() or 'Error termination' in err:
            bad += 1
            print('session %d (--seed %d): exit %s  %s' % (k, dice, code, err.strip()[:200]))
    print('fuzz: %d sessions of %d commands, %d failed' % (sessions, moves, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
