"""Replay the reference sessions into dod.exe and compare the output.

usage: python tests\\run.py [NAME ...]

Each NAME.raw is what SINTRAN III (RetroCore) sent to the terminal while
tools\\reffuzz.py typed NAME.keys:

  tests\\ref\\            gameNN: DODF-RNN, the port's program (data\\)
  tests\\ref-original\\   gameNN: DOD-RNN, the program as recovered (build\\original\\)
  tests\\walk\\           scripted sessions (reffuzz.py --script) of DODF-REF, or of
                         DOD-REF when named original-*
  tests\\sintran-only\\   sessions of the program as recovered that took the way
                         out, line 3120, which is lost: its GOTO runs into what
                         lies below the program, SINTRAN's leftovers there (a
                         question for the terminal type), zeros on the port (the
                         program again from the start).  Compared up to there.

The -R programs are the programs with RANDOM seeded with NN, not the uptime
(the -REF ones with 0: tools\\refprog.py), and the port runs the same image,
made again from the program here.  It gets the same bytes on standard input,
with --raw (untranslated output), --no-hold and -Z, in an empty scratch
folder.  A session the player stopped answering ends there on the port too
(the end of its input).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from refprog import ref_of  # noqa: E402

EXE = os.path.join(PORT, 'dod.exe')
FIXED = os.path.join(PORT, 'data', 'DOD-ENB.PROG')
ORIGINAL = os.path.join(PORT, 'build', 'original', 'DOD-ENB.PROG')
SETS = [('ref', FIXED), ('walk', FIXED), ('ref-original', ORIGINAL), ('sintran-only', ORIGINAL)]


def body_of_reference(raw):
    """What the program sent: after the echo of its name, less SINTRAN's prompt."""
    raw = bytes(b & 0x7f for b in raw)
    i = raw.find(b'\r\n')
    raw = raw[i + 2:] if i >= 0 else raw
    for end in (b'@ ', b'@'):
        if raw.endswith(end):
            raw = raw[:-len(end)]
            break
    return raw


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
        for name in names:
            total += 1
            scratch = tempfile.mkdtemp(prefix='dod-test-')
            try:
                keys = open(os.path.join(ref, name + '.keys'), 'rb').read()
                want = body_of_reference(open(os.path.join(ref, name + '.raw'), 'rb').read())
                m = re.search(r'game(\d+)$', name)
                seed = int(m.group(1)) if m else 0
                image = os.path.join(scratch, 'REF.PROG')
                src = ORIGINAL if name.startswith('original-') else prog
                open(image, 'wb').write(ref_of(open(src, 'rb').read(), seed))
                p = subprocess.run([EXE, '--data', scratch, '--prog', image, '--raw', '--no-hold', '-Z', '1'],
                                   input=keys, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=600)
            finally:
                shutil.rmtree(scratch, ignore_errors=True)
            got = bytes(b & 0x7f for b in p.stdout)
            label = '%s/%s' % (sub, name)
            if sub == 'sintran-only':
                want = want[:want.find(b'\r\nTerminal types are:')]
                got = got[:len(want)]
            if got == want:
                size += len(got)
                print('ok    %-30s %7d bytes%s' % (label, len(got), ', to the way out' if sub == 'sintran-only' else ''))
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
