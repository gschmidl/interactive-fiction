"""Replay the recorded SINTRAN III sessions and compare byte for byte.

    python tests/run.py [NAME ...]

Every tests/ref/NAME.in is typed into skattejakt.exe --raw; the output must
equal tests/ref/NAME.ref, which is what the original program sent on
SINTRAN III L (RetroCore, user GAMES, @SKAT) for the same keys.  The game
ends either by itself (SLUTT) -- then SINTRAN's CR LF follows -- or because
the input ran out while it waited, when the port adds CR LF too.

The port runs in a throwaway directory, so nothing is written next to it.
"""
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, '..', 'skattejakt.exe')


def main():
    names = sys.argv[1:] or sorted(os.path.basename(p)[:-3] for p in glob.glob(os.path.join(HERE, 'ref', '*.in')))
    work = tempfile.mkdtemp(prefix='skattest')
    bad = 0
    try:
        for n in names:
            inp = open(os.path.join(HERE, 'ref', n + '.in'), 'rb').read()
            ref = open(os.path.join(HERE, 'ref', n + '.ref'), 'rb').read()
            p = subprocess.run([EXE, '--raw', '-Z', '1789632000'], input=inp, capture_output=True,
                               cwd=work, timeout=120)
            out = p.stdout
            if out == ref or out == ref + b'\r\n':
                print('ok    %-16s %6d bytes' % (n, len(out)))
                continue
            k = 0
            while k < min(len(ref), len(out)) and ref[k] == out[k]:
                k += 1
            print('FAIL  %-16s first difference at byte %d' % (n, k))
            print('      expected %r' % ref[max(0, k - 60):k + 60])
            print('      got      %r' % out[max(0, k - 60):k + 60])
            bad += 1
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('%d of %d sessions identical' % (len(names) - bad, len(names)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
