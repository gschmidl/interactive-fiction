#!/usr/bin/env python3
"""Compare the port against the original running on MVS 3.8j.

mvs_advwiz.txt and mvs_probe.txt in this folder are what the originals
printed on the real machine (see ..\\README.md for how they were produced and
mvs\\ for the harness that produced them).  This script runs the same two
programs here and diffs them line for line.  The port's lines carry the
carriage-control blank that the 1403 printer consumed, so one leading blank
comes off, and trailing blanks are ignored on both sides.

usage: python cmpmvs.py [port-directory]
"""
import difflib
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                       else os.path.join(HERE, '..'))
BUILD = os.path.join(PORT, '.build')

WIZIN = 'y\nDWARF\nn\nS!+?9\nn\ny\n99\n99\n99\nn\n\n\n\nn\nF\n'
PROBEIN = 'xyzzy\ntake lamp\nlower case Words\n\nfee fie foe\ns\n'
# The wizard's challenge answer above is only right for this clock: the ten
# octal digits it has to match come from RAN, seeded from --date/--time.
DATE, TIME = '85123', '1000'


def norm(text):
    text = text.replace('\r\n', '\n')
    return [(l[1:] if l[:1] == ' ' else l).rstrip() for l in text.split('\n')]


def run(exe, stdin, cwd, args):
    p = subprocess.run([exe] + args, input=stdin.encode('latin-1'),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       cwd=cwd, timeout=120)
    return p.stdout.decode('latin-1')


def cut(lines, first, last):
    i = lines.index(first)
    return lines[i:lines.index(last, i) + 1]


def compare(what, got, want):
    d = list(difflib.unified_diff(want, got, 'mvs', 'port', lineterm='', n=1))
    if d:
        print('%-8s DIFFERS (%d mvs lines, %d port lines)'
              % (what, len(want), len(got)))
        print('\n'.join(d[:40]))
        return 1
    print('%-8s IDENTICAL (%d lines)' % (what, len(want)))
    return 0


def main():
    rc = 0
    mvs = open(os.path.join(HERE, 'mvs_advwiz.txt'),
               encoding='latin-1').read().split('\n')
    mvs = [l for l in mvs if l or True][:-1] if mvs[-1] == '' else mvs

    # 1. advwiz, answering the wizard's test for real rather than with --auto,
    #    and with the same answers the MVS job gave.  This writes advent.ini
    #    with MTDTXT left as the original clears it, which is what the MVS
    #    file has, so pass --no-fixes here and rebuild afterwards.
    out = run(os.path.join(PORT, 'advwiz.exe'), WIZIN, PORT,
              ['--no-fixes', '--date', DATE, '--time', TIME])
    got = cut(norm(out), 'INITIALIZING...', 'INITIALIZATION COMPLETED.')
    rc |= compare('advwiz', got, cut(mvs, 'INITIALIZING...',
                                     'INITIALIZATION COMPLETED.'))

    # 2. probe, on the file advwiz has just written.
    probe = os.path.join(BUILD, 'probe.exe')
    if not os.path.exists(probe):
        print('probe    SKIPPED (probe.exe not built - run build.sh --tests)')
        return rc
    import shutil
    shutil.copyfile(os.path.join(PORT, 'advent.ini'),
                    os.path.join(BUILD, 'advent.ini'))
    out = run(probe, PROBEIN, BUILD, [])
    mvs = open(os.path.join(HERE, 'mvs_probe.txt'),
               encoding='latin-1').read().split('\n')
    got = cut(norm(out), 'PROBE 1: STATE CHECKSUMS', 'PROBE DONE')
    rc |= compare('probe', got, cut(mvs, 'PROBE 1: STATE CHECKSUMS',
                                    'PROBE DONE'))
    print('\nadvent.ini is now the --no-fixes one; run build.sh to put the'
          ' playable one back.')
    return rc


if __name__ == '__main__':
    sys.exit(main())
