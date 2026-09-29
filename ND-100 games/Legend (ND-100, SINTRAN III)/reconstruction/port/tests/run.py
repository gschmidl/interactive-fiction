"""Replay the reference sessions into legend.exe and compare the output.

usage: python tests\\run.py [NAME ...]

Each NAME.raw is what SINTRAN III (RetroCore) sent to the terminal while a
program played the typing in NAME.keys (see tools\\reffuzz.py):

  tests\\ref\\           LEGENDF-REF:PROG, the game as the port ships it
                        (data\\LEGEND-LU.PROG, with the port's fixes)
  tests\\ref-original\\  LEGEND-REF:PROG, the recovered source as it was
                        (build\\original\\LEGEND-LU.PROG)

The port gets the same bytes on standard input, with --raw (untranslated
output), --no-hold (the program's pauses change nothing on the screen) and
the clock the reference's title showed (NAME.log, first line).  Every session
starts from a fresh scratch copy of data\\, as the reference machine put the
game's files back before each.
"""
import datetime
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'legend.exe')
DATA = os.path.join(PORT, 'data')
SETS = [
    ('ref', os.path.join(DATA, 'LEGEND-LU.PROG')),
    ('ref-original', os.path.join(PORT, 'build', 'original', 'LEGEND-LU.PROG')),
]


def machine_of(logfile):
    """-Z for the time the reference showed (SINTRAN's year is 28 behind the
    host's) and --terminal for its terminal's logical device number"""
    first = open(logfile, encoding='latin-1').readline()
    m = re.match(r'clock (\d+)-(\d+)-(\d+) (\d+):(\d+) terminal (\d+)', first)
    y, mo, d, h, mi, term = (int(x) for x in m.groups())
    return ['-Z', str(int(time.mktime(datetime.datetime(y + 28, mo, d, h, mi, 0).timetuple()))),
            '--terminal', str(term)]


def body_of_reference(raw):
    """From the program's first output (ESC 3) to the end, less SINTRAN's prompt."""
    raw = bytes(b & 0x7f for b in raw)
    i = raw.find(b'\x1b3')
    raw = raw[max(i, 0):]
    if raw.endswith(b'@'):
        raw = raw[:-1]
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
            scratch = tempfile.mkdtemp(prefix='legend-test-')
            try:
                for f in os.listdir(DATA):
                    if f != 'LEGEND-LU.PROG':
                        shutil.copy(os.path.join(DATA, f), scratch)
                keys = open(os.path.join(ref, name + '.keys'), 'rb').read()
                want = body_of_reference(open(os.path.join(ref, name + '.raw'), 'rb').read())
                p = subprocess.run([EXE, '--data', scratch, '--prog', prog, '--raw', '--no-hold'] +
                                   machine_of(os.path.join(ref, name + '.log')),
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
