"""Replay the recorded SINTRAN III sessions and compare byte for byte.

    python tests/run.py [NAME ...]

Every tests/ref/NAME.in is typed into svha.exe --raw --no-hold --no-fixes;
the output must equal tests/ref/NAME.ref, which is what SVHA-ADVENTURE sent
on SINTRAN III L (RetroCore, user GAMES, @SVH) for the same keys.  SVHA seeds
its random numbers from the day and minute it starts, so a session whose play
depends on them carries NAME.clock, the -Z value for that minute.

The sessions are the game as it was, bugs and all, so they run without the
port's bug fixes; tests/fixtest.py checks where the fixes part from them.

The game ends by itself (QUIT) -- SINTRAN's CR LF follows -- or because the
input ran out while it waited, when the port adds CR LF too.  The port runs
in a throwaway directory.
"""
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, '..', 'svha.exe')
DEFAULT_CLOCK = '1789632000'


def main():
    names = sys.argv[1:] or sorted(os.path.basename(p)[:-3] for p in glob.glob(os.path.join(HERE, 'ref', '*.in')))
    work = tempfile.mkdtemp(prefix='svhatest')
    bad = 0
    try:
        for n in names:
            inp = open(os.path.join(HERE, 'ref', n + '.in'), 'rb').read()
            ref = open(os.path.join(HERE, 'ref', n + '.ref'), 'rb').read()
            cf = os.path.join(HERE, 'ref', n + '.clock')
            clock = open(cf).read().strip() if os.path.exists(cf) else DEFAULT_CLOCK
            p = subprocess.run([EXE, '--raw', '--no-hold', '--no-fixes', '-Z', clock], input=inp, capture_output=True,
                               cwd=work, timeout=300)
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
