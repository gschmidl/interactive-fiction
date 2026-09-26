#!/usr/bin/env python3
"""Regression checks for the Philips P7000 ADVENT port (run after build.sh).

    python regress.py              run the checks
    python regress.py --record     write reference/walk.out from this build

The reference walk is this port's own output (there is no other P7000 to
compare with); it pins the emulated machine's behaviour between builds.
"""
import os
import shutil
import subprocess
import sys
import tempfile

from common import EXE, PORT, REF, reference, run, scratch_pack

FAILS = []
SECTOR = 256 * 3                        # bytes of a 256-word sector


def check(what, ok, detail=''):
    print('%-62s %s' % (what, 'ok' if ok else 'FAILED'))
    if not ok:
        FAILS.append(what)
        if detail:
            print('    ' + detail.replace('\n', '\n    ')[:2000])


def options():
    p = subprocess.run([EXE, '--help'], capture_output=True, text=True)
    check('--help prints the usage and exits 0', p.returncode == 0 and 'Usage:' in p.stdout)
    for bad in (['--nope'], ['-x'], ['--pack'], ['--pack='], ['--trace'], ['--fixed-clock=1'],
                ['extra']):
        p = subprocess.run([EXE] + bad, capture_output=True, text=True)
        check('%s is refused (exit 2)' % ' '.join(bad), p.returncode == 2 and 'Try' in p.stderr,
              p.stderr)
    p = subprocess.run([EXE, '--pack=' + os.path.join(PORT, 'no', 'such', 'dir', 'x.pack')],
                       capture_output=True, text=True, input='')
    check('a pack that cannot be made is reported (exit 1)',
          p.returncode == 1 and 'cannot make' in p.stderr, p.stderr)


def walk():
    inputs, expected = reference('walk')
    out, err, rc = run(inputs, ['--fixed-clock', '-u'])
    check('the reference walk replays identically', rc == 0 and not err and out == expected,
          'rc %d %s\n%s' % (rc, err, first_difference(out, expected)))


def first_difference(a, b):
    la, lb = a.split('\n'), b.split('\n')
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else '<end>'
        y = lb[i] if i < len(lb) else '<end>'
        if x != y:
            return 'line %d:\n  got      %r\n  expected %r' % (i + 1, x, y)
    return ''


def changed_sectors(pack):
    """the sectors in which PACK differs from the original"""
    a = open(os.path.join(PORT, 'p7000.pack'), 'rb').read()
    b = open(pack, 'rb').read()
    return [s for s in range(len(a) // SECTOR)
            if a[s * SECTOR:(s + 1) * SECTOR] != b[s * SECTOR:(s + 1) * SECTOR]]


def new_and_old(tmp):
    pack = scratch_pack(tmp, 'cycle.pack')
    out, err, rc = run(['NO', 'IN', 'TAKE LAMP', 'SUSPEND', 'YES', 'LOOK', 'SCORE'],
                       ['--fixed-clock'], pack)
    check('SUSPEND ends the run (the lines after it are not read)',
          rc == 0 and not err and out.rstrip().endswith('==>YES\n\nOK') and 'LOOK' not in out,
          out[-400:] + err)
    out, err, rc = run(['INVENTORY', 'QUIT', 'YES'], ['--fixed-clock'], pack)
    check('the next start goes on with the suspended game',
          out.lstrip().startswith("YOU'RE INSIDE BUILDING.") and 'BRASS LANTERN' in out,
          out[:600] + err)
    news = 0
    for _ in range(6):
        out, err, rc = run(['NO', 'QUIT', 'YES'], ['--fixed-clock'], pack)
        news += (rc == 0 and not err and 'WOULD YOU LIKE INSTRUCTIONS?' in out
                 and 'INITIALIZING' not in out and 'YOU SCORED' in out)
    check('after QUIT every start is a new game (six in a row)', news == 6, out[:600] + err)
    # ADSAVE is sectors 05001-05200 of the original pack; $COMM is 0154
    extra = [s for s in changed_sectors(pack) if not (0o5001 <= s <= 0o5200 or s == 0o154)]
    check('games change only ADSAVE and IDOS\'s $COMM sector', not extra,
          'also changed: ' + ' '.join('%o' % s for s in extra))


def unlimited():
    walk = ['NO', 'HOURS', 'SUSPEND', 'NO', 'QUIT', 'YES']
    out, err, rc = run(walk, ['--fixed-clock'])
    check('HOURS and SUSPEND show the wizard\'s settings',
          'MON - FRI:   0:00 TO  8:00' in out and 'AT LEAST  1 MINUTES' in out, out[:1200])
    out, err, rc = run(walk, ['--fixed-clock', '-u'])
    check('-u: open all day, no wait before a resume',
          'MON - FRI:  OPEN ALL DAY' in out and 'AT LEAST  0 MINUTES' in out, out[:1200])


def ending():
    out, err, rc = run(['NO', 'IN'], ['--fixed-clock'])
    check('the end of the input ends the run quietly', rc == 0 and not err, err)
    out, err, rc = run(['NO', 'QUIT', 'YES'], ['--fixed-clock'])
    check('QUIT ends with the score and the rating', rc == 0 and not err
          and out.rstrip().endswith('TO ACHIEVE THE NEXT HIGHER RATING, YOU NEED  4 MORE POINTS.'),
          out[-300:] + err)
    out, err, rc = run(['NO', 'IN'])
    check('the real-time clock runs too', rc == 0 and not err and 'INSIDE A BUILDING' in out,
          out[-300:] + err)


def record():
    inputs, _ = reference('walk')
    out, err, rc = run(inputs, ['--fixed-clock', '-u'])
    if rc or err:
        sys.exit('the walk failed: rc %d %s' % (rc, err))
    with open(os.path.join(REF, 'walk.out'), 'w', newline='') as f:
        f.write(out)
    print('wrote reference/walk.out (%d lines)' % out.count('\n'))
    return 0


def main():
    if not os.path.exists(EXE):
        sys.exit('build the port first (build.sh)')
    if sys.argv[1:] == ['--record']:
        return record()
    tmp = tempfile.mkdtemp(prefix='advp7r-')
    try:
        options()
        walk()
        new_and_old(tmp)
        unlimited()
        ending()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('%d failed' % len(FAILS) if FAILS else 'all passed')
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
