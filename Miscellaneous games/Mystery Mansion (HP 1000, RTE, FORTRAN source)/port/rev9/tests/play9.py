"""Revision 9 regression: boot the site's system three times on a scratch disc
(the build's disc0.img; the player's .build\\disc.img is kept aside):

  A  --fixed-clock: the gate, the note, SUSPEND onto cartridge 2 as TEST, QUIT
  B  --fixed-clock: RESTORE TEST from cartridge 2, the note again, QUIT
  C  this computer's clock (the SYTM path): the banner, QUIT

and check the game's words, the exit status, and that no simulator is left."""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..'))
BUILD = os.path.join(ROOT, '.build')
TERM = os.path.join(ROOT, 'src', 'term.py')
SESSIONS = [
    ('A', ['--fixed-clock'], 'LOOK\nTAKE NOTE\nREAD NOTE\nSUSPEND\n2\nTEST\nQUIT\nYES\n',
     ['WELCOME TO MYSTERY MANSION.  MYSTERY   2    REVISION 9', 'COMPLIMENTS OF BILL WOLPERT',
      'YOU ARE AT THE MAIN GATE', 'YOUR BOOTY NOW CONTAINS THE NOTE', 'I CAN HELP YOU OPEN THE GATE',
      'GOOD FOR ANY REVISION 9', 'PURGE THE FILE NAMED MMTEST', 'SO FAR YOU HAVE SCORED  48 POINTS',
      'GOOD BYE GUY!']),
    ('B', ['--fixed-clock'], 'RESTORE\n2\nTEST\nSCORE\nREAD NOTE\nQUIT\nYES\n',
     ['YOU ARE NOW IN THE SAME SITUATION', 'SO FAR YOU HAVE SCORED  48 POINTS',
      'THE NOTE SAYS THE SAME AS IT DID BEFORE',          # read before the SUSPEND
      'GOOD BYE GUY!']),
    ('C', [], 'QUIT\nYES\n',
     ['REVISION 9', 'COMPLIMENTS OF BILL WOLPERT', 'GOOD BYE GUY!']),
]


def main():
    fails = 0
    r = subprocess.run([sys.executable, TERM, '--bogus'], capture_output=True, text=True)
    if r.returncode != 2:
        print('FAIL unknown option: exit', r.returncode)
        fails += 1
    disc = os.path.join(BUILD, 'disc.img')
    keep = None
    if os.path.exists(disc):
        keep = tempfile.mktemp(suffix='.img', dir=BUILD)
        os.replace(disc, keep)
    try:
        for name, opts, moves, want in SESSIONS:
            r = subprocess.run([sys.executable, TERM] + opts, input=moves,
                               capture_output=True, text=True, timeout=300)
            for w in want:
                if w not in r.stdout:
                    print('FAIL %s missing: %s' % (name, w))
                    fails += 1
            m = re.search(r'MYSTERY +(\d+) +REVISION', r.stdout)
            print('session %s: exit %d, mystery %s' % (name, r.returncode, m.group(1) if m else '?'))
            if r.returncode != 0:
                print('FAIL %s exit %d' % (name, r.returncode))
                fails += 1
            left = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq hp2100.exe'], capture_output=True).stdout
            if b'hp2100.exe' in left:
                print('FAIL %s: the simulator is still running' % name)
                fails += 1
    finally:
        if os.path.exists(disc):
            os.remove(disc)
        if keep:
            os.replace(keep, disc)
    print('play9: %s' % ('FAILED %d' % fails if fails else 'ok'))
    sys.exit(1 if fails else 0)


if __name__ == '__main__':
    main()
