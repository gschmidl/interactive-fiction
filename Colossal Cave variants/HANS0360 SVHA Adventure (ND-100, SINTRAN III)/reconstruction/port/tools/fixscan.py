"""Where do the bug fixes change the game?  A broad differential scan.

usage: python tools/fixscan.py [-j JOBS]

Types every word of the vocabulary, alone and with each of eight objects,
followed by TAKE, OPEN and LOOK, into svha.exe with and without --no-fixes,
from five places reached from a fresh game (with the --debug pokes that
tests/fixtest.py uses):

  building   in the well house, lamp on
  grate      at the grate with the keys
  crypt      in the crypt wearing the ring
  metal      in the crypt with the coffin opened (LIFT LID) and the metal piece
  pit        in the ice pit with the cauldron of soup

and lists every command whose output differs.  A difference is expected where
a fix is: OPEN or UNLOCK COFFIN, POUR in the ice pit, and a command that takes
no turn given an object (the next command no longer inherits it).  Anything
else is printed as UNEXPECTED and the exit status is 1.  The vault fix shows
only on a second OPEN VAULT, which tests/fixtest.py plays.

QUIT, SAVE, SUSPEND and PAUSE are left out.  Runs in a throwaway directory.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.abspath(os.path.join(HERE, '..', 'svha.exe'))
DATA = os.path.abspath(os.path.join(HERE, '..', 'data'))

START = b'NO\rIN\rTAKE LAMP\rON\r'
CRYPT = START + b'#poke 177776 21.\rTAKE RING\rPUT ON RING\r#poke 177776 31.\r'
PLACES = [
    ('building', START),
    ('grate', b'NO\rIN\rTAKE LAMP\rTAKE KEYS\rOUT\rS\rS\rS\r'),
    ('crypt', CRYPT),
    ('metal', CRYPT + b'LIFT LID\rTAKE METAL\r'),
    ('pit', START + b'#poke 177775 24.\r#poke 177776 195.\r'),
]
OBJECTS = ['', 'RING', 'LAMP', 'COFFIN', 'VAULT', 'SOUP', 'KEYS', 'GRATE']
FOLLOW = b'TAKE\rOPEN\rLOOK\r'
LEFT_OUT = {'QUIT', 'SAVE', 'SUSPEN', 'PAUSE'}
TURNLESS = set('LOOK EXAMIN DESCRI INVENT GOTO MOVE GO WALK ONWARD RUN TRAVEL PROCEE CONTIN FOLLOW '
               'WASH BOW HELP ? INFO INFORM BLAST DETONA IGNITE BLOWUP CALM TAME WAKE DISTUR DIG EXCAVA '
               'SCORE HOURS LOST FUCK REMOVE JUJU'.split())


def vocabulary():
    d = open(os.path.join(DATA, 'SVHA-DATAFIL.SYMB'), 'rb').read()
    words = []
    for x in range(33104, 35900, 6):
        w = d[x:x + 6].decode('latin-1').strip()
        if w and w != 'AAAAAA' and w not in words and w not in LEFT_OUT:
            words.append(w)
    return words


def expected(place, word, obj):
    return ((word in ('OPEN', 'UNLOCK') and obj == 'COFFIN') or (place == 'pit' and word == 'POUR')
            or (word in TURNLESS and obj != ''))


def main():
    jobs = 12
    if len(sys.argv) == 3 and sys.argv[1] in ('-j', '--jobs'):
        jobs = int(sys.argv[2])
    work = tempfile.mkdtemp(prefix='svhascan')
    try:
        shutil.copytree(DATA, os.path.join(work, 'data'))

        def run(job):
            place, prefix, word, obj = job
            cmd = ('%s %s' % (word, obj)).strip().encode('latin-1')
            outs = []
            for flag in ([], ['--no-fixes']):
                try:
                    p = subprocess.run([EXE, '--raw', '--no-hold', '--debug', '-Z', '1789632000',
                                        '--data', os.path.join(work, 'data')] + flag,
                                       input=prefix + cmd + b'\r' + FOLLOW, capture_output=True,
                                       cwd=work, timeout=120)
                    outs.append(p.stdout)
                except subprocess.TimeoutExpired:
                    outs.append(None)
            return job, outs[0] is None or outs[0] != outs[1]

        todo = [(pl, pre, w, o) for pl, pre in PLACES for w in vocabulary() for o in OBJECTS]
        diff = defaultdict(lambda: defaultdict(list))
        bad = []
        with ThreadPoolExecutor(jobs) as ex:
            for (place, _, word, obj), differs in ex.map(run, todo):
                if differs:
                    diff[place][word].append(obj or '-')
                    if not expected(place, word, obj):
                        bad.append('%s: %s %s' % (place, word, obj))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('%d commands, each with and without the fixes' % len(todo))
    for place, _ in PLACES:
        print('== %s: %d differ' % (place, sum(len(v) for v in diff[place].values())))
        for word, objs in diff[place].items():
            print('   %-7s %s' % (word, ' '.join(objs)))
    for b in bad:
        print('UNEXPECTED  ' + b)
    print('%d unexpected' % len(bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
