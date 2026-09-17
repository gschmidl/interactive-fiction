"""Replay the reference sessions into mordor.exe and compare the output.

usage: python tests\\run.py [NAME ...]

Each NAME.raw is what SINTRAN III (RetroCore) sent to the terminal while a
program played the typing in NAME.keys (see tools\\reffuzz.py):

  tests\\ref\\           MORDORF-REF:PROG, the game as the port ships it
                        (data\\MORDOR-MJ.PROG, with the port's fixes)
  tests\\ref-original\\  MORDOR-REF:PROG, the recovered source compiled
                        unchanged (build\\original\\MORDOR-MJ.PROG)

The port gets the same bytes on standard input, with --raw (untranslated
output), --unlimited (the DEL typed ahead on the reference) and a fixed clock
(the reference programs' random numbers ignore the uptime; see NOTES.md).

Every session runs in a scratch copy of data\\ (never the real one: it holds
the map of your games).  A session whose name ends in "-keep" uses the map
the previous session in its directory left, as the reference did; any other
starts with none.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'mordor.exe')
DATA = os.path.join(PORT, 'data')
SETS = [
    ('ref', os.path.join(DATA, 'MORDOR-MJ.PROG')),
    ('ref-original', os.path.join(PORT, 'build', 'original', 'MORDOR-MJ.PROG')),
]
CLOCK = '1789668000'   # 2026-09-17 20:00 local; any time will do with --unlimited
GAME_FILES = ['MORDOR-RULES-MJ.DATA', 'PASCAL-ERR.SYMB']


def body_of_reference(raw):
    """From the program's first output (the form feed) to the end, less SINTRAN's prompt."""
    raw = bytes(b & 0x7f for b in raw)
    i = raw.find(b'\x0c')
    if i < 0:
        i = raw.find(b'\r\n') + 2
    raw = raw[i:]
    if raw.endswith(b'@'):
        raw = raw[:-1]
    return raw


def body_of_port(out):
    """The port ends with the new line SINTRAN writes before its @."""
    return bytes(b & 0x7f for b in out)


def main():
    wanted = set(sys.argv[1:])
    failed = total = size = 0
    for sub, prog in SETS:
        ref = os.path.join(HERE, sub)
        if not os.path.isdir(ref):
            continue
        names = sorted(f[:-5] for f in os.listdir(ref) if f.endswith('.keys'))
        if wanted:
            names = [n for n in names if n in wanted or sub + '/' + n in wanted]
        scratch = tempfile.mkdtemp(prefix='mordor-test-')
        try:
            for f in GAME_FILES:
                shutil.copy(os.path.join(DATA, f), scratch)
            for name in names:
                total += 1
                mapfile = os.path.join(scratch, 'MORDOR-MAP-MJ.DATA')
                if not name.endswith('-keep') and os.path.exists(mapfile):
                    os.remove(mapfile)
                keys = open(os.path.join(ref, name + '.keys'), 'rb').read()
                want = body_of_reference(open(os.path.join(ref, name + '.raw'), 'rb').read())
                p = subprocess.run([EXE, '--data', scratch, '--prog', prog, '--raw', '--unlimited',
                                    '-Z', CLOCK], input=keys, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, timeout=600)
                got = body_of_port(p.stdout)
                label = '%s/%s' % (sub, name)
                if got == want:
                    size += len(got)
                    print('ok    %-30s %7d bytes' % (label, len(got)))
                    continue
                failed += 1
                n = min(len(got), len(want))
                i = next((k for k in range(n) if got[k] != want[k]), n)
                print('FAIL  %-30s differs at byte %d (port %d, reference %d bytes)' % (label, i, len(got), len(want)))
                print('      reference: %r' % want[max(0, i - 60):i + 60])
                print('      port:      %r' % got[max(0, i - 60):i + 60])
                if p.stderr:
                    print('      stderr: %r' % p.stderr[-300:])
        finally:
            shutil.rmtree(scratch, ignore_errors=True)
    print('%d of %d sessions identical (%d bytes)' % (total - failed, total, size))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
