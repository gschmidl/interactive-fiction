"""SAVE and RESTORE, which the port adds to SVHA Adventure.

    python tests/savetest.py

Everything runs in a throwaway directory:

  1. SAVE writes the game; the player walks off; RESTORE puts the player back
     in the building with the same things and the same number of turns.
  2. The saved file resumes from the command line.
  3. SAVE answered with Enter saves nothing and the game goes on.
  4. RESTORE of a file that is not there says so and the game goes on.
  5. SAVE as the answer to a yes/no question is the game's business.
  6. A session with no SAVE or RESTORE in it is not changed by any of this
     (tests/run.py checks that byte for byte).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.abspath(os.path.join(HERE, '..', 'svha.exe'))
DATA = os.path.abspath(os.path.join(HERE, '..', 'data'))


def play(work, keys, *args):
    p = subprocess.run([EXE, '--raw', '--no-hold', '-Z', '1789632000', '--data', DATA] + list(args),
                       input=keys, capture_output=True, cwd=work, timeout=120)
    return p.stdout.decode('latin-1').replace('\r', '')


def turns(text):
    return re.findall(r'using +(\d+) turns', text)[-1]


def check(cond, what, text=''):
    if not cond:
        print('FAIL  ' + what)
        print(text[-1500:])
        sys.exit(1)
    print('ok    ' + what)


def main():
    work = tempfile.mkdtemp(prefix='svhasave')
    try:
        out = play(work, b'NO\rIN\rTAKE LAMP\rTAKE KEYS\rSAVE\rcave one\n'
                         b'OUT\rSOUTH\rINVENTORY\rRESTORE\rcave one\nLOOK\rINVENTORY\rQUIT\r')
        check('Saved as cave one.SAV' in out and os.path.exists(os.path.join(work, 'cave one.SAV')),
              'SAVE writes the game', out)
        after = out[out.index('Restored.'):]
        check('You are inside a building' in after and 'Set of keys' in after and 'Brass lantern' in after,
              'RESTORE puts the player back in the building with the lamp and keys', out)
        stayed = play(work, b'NO\rIN\rTAKE LAMP\rTAKE KEYS\rLOOK\rINVENTORY\rQUIT\r')
        saved_turns = turns(stayed)
        check(turns(out) == saved_turns,
              'the turns taken after the save are undone (%s, as if never gone)' % saved_turns, out)
        check("I don't know how" not in out, 'the game never sees SAVE or RESTORE', out)

        out = play(work, b'LOOK\rQUIT\r', 'cave one')
        check('You are inside a building' in out and turns(out) == saved_turns,
              'the saved game resumes from the command line', out)

        out = play(work, b'NO\rIN\rSAVE\r\nTAKE LAMP\rQUIT\r')
        check('Not saved.' in out and 'OK' in out.split('Not saved.')[1], 'SAVE cancelled with Enter', out)

        out = play(work, b'NO\rIN\rRESTORE\rnowhere\nTAKE LAMP\rQUIT\r')
        check('There is no file nowhere.SAV.' in out and 'OK' in out.split('nowhere.SAV.')[1],
              'RESTORE of a missing file', out)

        out = play(work, b'SAVE\rNO\rQUIT\r')
        check('Please answer the question.' in out and 'Save the game' not in out,
              'SAVE at a yes/no question is left to the game', out)
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
