#!/usr/bin/env python3
"""Can the game be won - 600 of 600 points and "GOODBYE, IMPLEMENTER!"?

    python tests\\win.py [-v]

Qork's ending is S. O. Lidie's own, not the DECUS Dungeon's: when the
score reaches the most there is (550), clock event 15 counts down fifteen
turns; then a wraith welcomes the player to the ranks of the chosen,
ENDGAME moves him to the west of the house, adds 50 to the most there is
(25 for the Tomb of the Unknown Implementer, 25 for the crypt), puts a
mylar net on the clifftop, and sets two kangaroos bouncing behind the
house.  A net thrown at the light coloured one while it is down brings
its pouch, with a lens in it; and taking the rusty knife in the tomb
with the lens in hand ends the game.

tests\\cmdswin.txt plays that, with the implementers' own GUARDIAN
(password EXPLURIBUSONION TKMG) for the long middle of the game:

  AA 1 2 540     the score set to 540 - the last ten points are earned,
                 by climbing in at the kitchen window, so the real
                 SCRUPD starts the endgame clock
  WAIT x4        the wraith (the fourth WAIT: see CLOCKD in convert.py)
  the net, the kangaroo, the pouch and the lens - played, not set
  TK 15, TK 24   the lamp and the rusty knife (from the maze)
  AH 94          the Land of the Living Dead (past the exorcism); E to
                 the tomb earns its 25 points
  DROP / TAKE RUSTY KNIFE   the laser beam: 25 more, and the end

The Cyber played the same commands (tests\\cyberwin.txt, compared by
cmpcyber.py).  Checked here: the ending, 600 of 600, the rank, and that
the program ended there (the LOOK after it is never read), exit 0.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)

EXPECT = ['SUDDENLY A SINISTER, WRAITHLIKE FIGURE',
          'A SUDDEN RUSTLING OF THE UNDERBRUSH',
          'AMAZING!  YOU HAVE JUST ENTRAPPED THE LIGHT COLORED KANGAROO.',
          'OPENING THE KANGAROO\'S POUCH REVEALS:',
          'YOU ARE IN THE TOMB OF THE UNKNOWN IMPLEMENTER.',
          'IT BEGINS TO LASE!',
          'GOODBYE, IMPLEMENTER!',
          '600  [TOTAL OF 600  POINTS], IN    25  MOVES.',
          'THIS GIVES YOU THE RANK OF IMPLEMENTER (OR IS IT CHEATER?)!']


def main():
    tmp = tempfile.mkdtemp(prefix='qork-win-')
    try:
        for f in ('qork.exe', 'qork.dat', 'qork.ini'):
            shutil.copy(os.path.join(PORT, f), tmp)
        cmds = open(os.path.join(HERE, 'cmdswin.txt'), 'rb').read()
        r = subprocess.run([os.path.join(tmp, 'qork.exe')], input=cmds,
                           capture_output=True, timeout=60)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    out = r.stdout.decode('latin-1').replace('\r\n', '\n')
    at, miss = 0, None
    for e in EXPECT:
        i = out.find(e, at)
        if i < 0:
            miss = e
            break
        at = i + len(e)
    ended = out.rstrip().endswith(EXPECT[-1])
    ok = miss is None and ended and r.returncode == 0
    if '-v' in sys.argv[1:] or not ok:
        print(out)
        if miss:
            print('[missing: %s]' % miss)
        if not ended:
            print('[the game did not end at the ending]')
    print('%-50s %s' % ('win: 600 of 600, GOODBYE, IMPLEMENTER!', 'ok' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
