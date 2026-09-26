"""Replay the reference sessions into cavefun.exe and compare the output.

usage: python tests\\run.py [NAME ...]

Each NAME.raw is what SINTRAN III (RetroCore) sent to the terminal while
tools\\reffuzz.py typed NAME.keys:

  tests\\ref\\           ADV-INTER-CB-MJ:PROG with CAVE-FUN-MJ:ADV, as the port
                        ships them (data\\, with the port's fixes)
  tests\\ref-original\\  the interpreter as recovered (build\\original\\) with the
                        game file as recovered, which the sessions call CAVE-ORIG
  tests\\walk\\          games played by hand on SINTRAN (tools\\refwalk.py)

The port gets the same bytes on standard input, with --raw (untranslated
output) and --no-hold.  Every session starts from a fresh scratch copy of the
game file, as the reference machine had no saved games before each.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'cavefun.exe')
DATA = os.path.join(PORT, 'data')
ORIGINAL = os.path.join(PORT, '..', 'src_original')
SETS = [
    ('ref', os.path.join(DATA, 'ADV-INTER-CB-MJ.PROG'), {'CAVE-FUN-MJ.ADV': os.path.join(DATA, 'CAVE-FUN-MJ.ADV')}),
    ('walk', os.path.join(DATA, 'ADV-INTER-CB-MJ.PROG'), {'CAVE-FUN-MJ.ADV': os.path.join(DATA, 'CAVE-FUN-MJ.ADV')}),
    ('ref-original', os.path.join(PORT, 'build', 'original', 'ADV-INTER-CB-MJ.PROG'),
     {'CAVE-ORIG.ADV': os.path.join(ORIGINAL, 'CAVE-FUN-MJ.ADV')}),
]


def body_of_reference(raw):
    """What the program sent: after the echo of its name, less SINTRAN's prompt."""
    raw = bytes(b & 0x7f for b in raw)
    i = raw.find(b'\r\n')
    raw = raw[i + 2:] if i >= 0 else raw
    if raw.endswith(b'@'):
        raw = raw[:-1]
    return raw


def main():
    wanted = set(sys.argv[1:])
    failed = total = size = 0
    for sub, prog, files in SETS:
        ref = os.path.join(HERE, sub)
        if not os.path.isdir(ref):
            continue
        names = sorted(f[:-5] for f in os.listdir(ref) if f.endswith('.keys'))
        if wanted:
            names = [n for n in names if n in wanted or sub + '/' + n in wanted]
        for name in names:
            total += 1
            scratch = tempfile.mkdtemp(prefix='cavefun-test-')
            try:
                for f, src in files.items():
                    shutil.copy(src, os.path.join(scratch, f))
                keys = open(os.path.join(ref, name + '.keys'), 'rb').read()
                want = body_of_reference(open(os.path.join(ref, name + '.raw'), 'rb').read())
                p = subprocess.run([EXE, '--data', scratch, '--prog', prog, '--raw', '--no-hold'],
                                   input=keys, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=600)
            finally:
                shutil.rmtree(scratch, ignore_errors=True)
            got = bytes(b & 0x7f for b in p.stdout)
            label = '%s/%s' % (sub, name)
            if got == want:
                size += len(got)
                print('ok    %-30s %7d bytes' % (label, len(got)))
                continue
            failed += 1
            n = min(len(got), len(want))
            i = next((k for k in range(n) if got[k] != want[k]), n)
            print('FAIL  %-30s differs at byte %d (port %d, reference %d bytes)' % (label, i, len(got), len(want)))
            print('      reference: %r' % want[max(0, i - 80):i + 60])
            print('      port:      %r' % got[max(0, i - 80):i + 60])
            if p.stderr:
                print('      stderr: %r' % p.stderr[-300:])
    print('%d of %d sessions identical (%d bytes)' % (total - failed, total, size))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
