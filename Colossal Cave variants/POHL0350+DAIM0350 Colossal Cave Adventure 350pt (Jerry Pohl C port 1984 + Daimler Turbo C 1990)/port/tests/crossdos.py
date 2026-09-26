#!/usr/bin/env python3
"""Daimler's port against Daimler's own DOS program, run under DOSBox.

    python tests\\crossdos.py [SESSIONS] [--moves N]

A copy of the port is built with the text files his ADVENT.EXE was built
for (the May 1990 ADVENT4.TXT, see tests\\dosref.py), and SESSIONS random
sessions (default 10 of 150 commands) run on both, the port with
--no-fixes.  They must print the same, byte for byte (line ends aside).

Only Daimler: his EXE was built from exactly the source ported here.
Pohl's DOS program (reference\\pohl-dos) is his 1984 build, and the source
ported is his 1990 revision, so the two are not expected to agree.
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
import dosref  # noqa: E402


def build_alt(tmp):
    """the port built with the DOS program's text files; its .exe"""
    texts = os.path.join(tmp, 'texts')
    os.makedirs(texts)
    for name, data in dosref.texts('daimler').items():
        with open(os.path.join(texts, name.lower()), 'wb') as f:
            f.write(data)
    stage = os.path.join(tmp, 'exe')
    env = dict(os.environ, ADVENT_VARIANT='daimler',
               ADVENT_TEXTS=texts.replace('\\', '/'), ADVENT_STAGE=stage.replace('\\', '/'))
    r = subprocess.run(['sh', 'build.sh'], cwd=C.PORT, env=env, capture_output=True)
    if r.returncode != 0:
        raise SystemExit('build.sh failed:\n' + (r.stdout + r.stderr).decode('latin-1'))
    return os.path.join(stage, 'advent.exe')


def first_difference(a, b):
    la, lb = a.split('\n'), b.split('\n')
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else '(end)'
        y = lb[i] if i < len(lb) else '(end)'
        if x != y:
            return 'line %d:\n      dos:  %r\n      port: %r' % (i + 1, x, y)
    return 'none'


def main():
    args = sys.argv[1:]
    sessions, moves = 10, 150
    while args:
        a = args.pop(0)
        if a == '--moves':
            moves = int(args.pop(0))
        else:
            sessions = int(a)
    tmp = tempfile.mkdtemp(prefix='advent-crossdos-')
    bad = 0
    try:
        exe = build_alt(tmp)
        words = C.vocab('daimler')
        for k in range(sessions):
            lines = C.session(random.Random(3000 + k), words, moves)
            dos, _ = dosref.run('daimler', lines, timeout=90)
            cwd = tempfile.mkdtemp(dir=tmp)
            port, err, rc = C.run('daimler', lines, ['--no-fixes'], exe_path=exe, cwd=cwd)
            if dos != port or rc != 0 or err:
                bad += 1
                print('session %d: %s' % (k, first_difference(dos, port)
                                          if dos != port else 'exit %d %s' % (rc, err)))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('crossdos: %d sessions of %d commands against Daimler\'s DOS program, %d failed'
          % (sessions, moves, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
