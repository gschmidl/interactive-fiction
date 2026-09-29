"""Replay the reference sessions into myworld.exe and compare the output.

usage: python tests\\run.py [NAME ...]

Each NAME.raw is what SINTRAN III (RetroCore) sent to the terminal while
tools\\reffuzz.py typed NAME.keys:

  tests\\ref\\            AMJF-REF: the port's program (data\\)
  tests\\ref-original\\   AMJ-REF: the program as recovered (build\\original\\)
  tests\\walk\\           scripted sessions (reffuzz.py --script), of the port's
                         program, or of the recovered one when named original-*

The -REF programs are the programs with RAN not stirring in the uptime, so
the port gets -Z (its uptime stays 0) to meet the same dice.  It gets the
same bytes on standard input, with --raw (untranslated output) and
--no-hold, in a scratch folder with the world (MY-DATA-FILE-MJ:ADV) and
ND-Pascal's error texts.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'myworld.exe')
DATA = os.path.join(PORT, 'data')
FIXED = os.path.join(DATA, 'ADVENTURE-MJ.PROG')
ORIGINAL = os.path.join(PORT, 'build', 'original', 'ADVENTURE-MJ.PROG')
SETS = [('ref', FIXED), ('walk', FIXED), ('ref-original', ORIGINAL)]


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
            scratch = tempfile.mkdtemp(prefix='myworld-test-')
            try:
                for f in ('MY-DATA-FILE-MJ.ADV', 'PASCAL-ERR.SYMB'):
                    shutil.copy(os.path.join(DATA, f), scratch)
                keys = open(os.path.join(ref, name + '.keys'), 'rb').read()
                want = body_of_reference(open(os.path.join(ref, name + '.raw'), 'rb').read())
                p = subprocess.run([EXE, '--data', scratch, '--prog',
                                    ORIGINAL if name.startswith('original-') else prog,
                                    '--raw', '--no-hold', '-Z', '1'],
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
