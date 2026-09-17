"""The bugs in SVHA Adventure that the port fixes (see NOTES.md).

    python tests/fixtest.py

Each game starts fresh in a throwaway directory with a copy of data\ and uses
the port's --debug pokes to walk straight to the place (177776 is the player's
location, 177766-177775 the things carried):

  1. Wearing the ring, OPEN COFFIN opens the coffin; the metal piece in it
     opens the vault.  Without the ring the lid is too heavy.  Opening it again
     does not close it or make a second metal piece.
  2. In the ice pit, POUR SOUP from the cauldron melts the ice and UP climbs
     out; anywhere else the soup is poured away as before.
  3. OPEN VAULT on the open vault says it is already open and leaves it open;
     CLOSE still closes it, and OPEN VAULT elsewhere still finds no vault.
  4. A command that takes no turn (REMOVE RING, INVENTORY RING) does not leave
     its object for the next command; a noun on its own still does.  The
     recorded session fuzz3 shows the bug on the real machine: with the fixes
     the port parts from it exactly there.
  5. With --no-fixes the bugs are back, and a game saved that way gets the
     fixes when it is resumed without --no-fixes.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.abspath(os.path.join(HERE, '..', 'svha.exe'))
DATA = os.path.abspath(os.path.join(HERE, '..', 'data'))

START = b'NO\rIN\rTAKE LAMP\rON\r'
CRYPT = START + b'#poke 177776 21.\rTAKE RING\rPUT ON RING\r#poke 177776 31.\r'
CRYPT_NO_RING = START + b'#poke 177776 31.\r'
PIT = START + b'#poke 177775 24.\r#poke 177776 195.\r'     # carrying the cauldron of soup

METAL = "You don't have the necessary piece of metal"
OPENED = 'As you opened the coffin'


def play(work, keys, *args):
    p = subprocess.run([EXE, '--raw', '--no-hold', '--debug', '-Z', '1789632000',
                        '--data', os.path.join(work, 'data')] + list(args),
                       input=keys, capture_output=True, cwd=work, timeout=120)
    return p.stdout.decode('latin-1').replace('\r', '')


def replies(text):
    """[(command, reply)] from a transcript: the game echoes a command indented"""
    out, cmd, lines = [], None, []
    for line in text.split('\n'):
        if line.startswith(' ' * 30) and line.strip():
            if cmd is not None:
                out.append((cmd, '\n'.join(lines)))
            cmd, lines = line.strip(), []
        elif cmd is not None:
            lines.append(line)
    if cmd is not None:
        out.append((cmd, '\n'.join(lines)))
    return out


def reply(text, cmd, n=1):
    got = [r for c, r in replies(text) if c == cmd]
    return got[n - 1] if len(got) >= n else ''


def check(cond, what, text=''):
    if not cond:
        print('FAIL  ' + what)
        print(text[-2000:])
        sys.exit(1)
    print('ok    ' + what)


def main():
    work = tempfile.mkdtemp(prefix='svhafix')
    try:
        shutil.copytree(DATA, os.path.join(work, 'data'))

        out = play(work, CRYPT + b'OPEN COFFIN\rOPEN COFFIN\rLOOK\rTAKE METAL\rOPEN VAULT\rW\rQUIT\r')
        first = reply(out, 'OPEN COFFIN')
        check(OPENED in first and METAL not in first, 'wearing the ring, OPEN COFFIN opens the coffin', out)
        second = reply(out, 'OPEN COFFIN', 2)
        look = reply(out, 'LOOK')
        check(OPENED not in second and 'The coffin is open' in second and 'The coffin is open' in look
              and look.count('metal piece') == 1,
              'opening it again leaves it open, with one metal piece', out)
        check('treasure room' in reply(out, 'W'), 'the metal piece opens the vault', out)

        out = play(work, CRYPT_NO_RING + b'OPEN COFFIN\rLOOK\rQUIT\r')
        r = reply(out, 'OPEN COFFIN')
        check('without the help of a crane' in r and METAL not in r and 'The coffin is closed' in reply(out, 'LOOK'),
              'without the ring the lid does not move', out)

        out = play(work, CRYPT + b'LIFT LID\rQUIT\r')
        check(OPENED in reply(out, 'LIFT LID'), 'LIFT LID still opens it', out)

        out = play(work, PIT + b'POUR SOUP\rUP\rQUIT\r')
        check('The soup melts the ice' in reply(out, 'POUR SOUP'), 'POUR SOUP in the ice pit melts the ice', out)
        check('You are on top of a pit' in reply(out, 'UP'), 'and the player climbs out', out)

        out = play(work, PIT + b'#poke 177776 193.\rPOUR SOUP\rPOUR SOUP\rQUIT\r')
        check('The soup is all over the ground' in reply(out, 'POUR SOUP')
              and "You don't carry any soup" in reply(out, 'POUR SOUP', 2),
              'anywhere else the soup is poured away, once', out)

        vault = CRYPT + b'OPEN COFFIN\rTAKE METAL\r'
        out = play(work, vault + b'OPEN VAULT\rOPEN VAULT\rUNLOCK VAULT\rW\rE\rCLOSE\rOPEN VAULT\r'
                                 b'DROP METAL\rCLOSE\rOPEN VAULT\rE\rOPEN VAULT\rQUIT\r')
        check(reply(out, 'OPEN VAULT').startswith('OK') and 'It was already open' in reply(out, 'OPEN VAULT', 2)
              and 'It was already open' in reply(out, 'UNLOCK VAULT') and 'treasure room' in reply(out, 'W'),
              'OPEN VAULT on the open vault leaves it open', out)
        check('It is now closed' in reply(out, 'CLOSE') and reply(out, 'OPEN VAULT', 3).startswith('OK'),
              'CLOSE closes it and OPEN VAULT opens it again', out)
        check(METAL in reply(out, 'OPEN VAULT', 4) and "There's no vault here" in reply(out, 'OPEN VAULT', 5),
              'without the metal piece, or away from the crypt, as before', out)
        out = play(work, CRYPT + b'LIFT LID\rTAKE METAL\rOPEN VAULT\rOPEN VAULT\rW\rQUIT\r', '--no-fixes')
        check(reply(out, 'OPEN VAULT', 2).startswith('OK') and 'treasure room' not in reply(out, 'W'),
              '--no-fixes: OPEN VAULT closes the open vault', out)

        left = CRYPT + b'INVENTORY RING\rDROP\rLAMP\rTAKE\rREMOVE RING\rOPEN COFFIN\rQUIT\r'
        out = play(work, left)
        check(reply(out, 'DROP').startswith('OK'), 'INVENTORY RING leaves no object: DROP drops the lamp', out)
        check(reply(out, 'TAKE').startswith('OK'), 'LAMP on its own still keeps it: TAKE takes the lamp', out)
        check("Can't" in reply(out, 'REMOVE RING') and OPENED in reply(out, 'OPEN COFFIN'),
              'after REMOVE RING, OPEN COFFIN opens the coffin', out)
        out = play(work, left, '--no-fixes')
        check("You aren't carrying it" in reply(out, 'DROP') and METAL in reply(out, 'OPEN COFFIN'),
              '--no-fixes: the object is left over (DROP RING, OPEN RING)', out)

        ref = open(os.path.join(HERE, 'ref', 'fuzz3.ref'), 'rb').read()
        p = subprocess.run([EXE, '--raw', '--no-hold', '-Z', open(os.path.join(HERE, 'ref', 'fuzz3.clock')).read().strip()],
                           input=open(os.path.join(HERE, 'ref', 'fuzz3.in'), 'rb').read(),
                           capture_output=True, cwd=work, timeout=300)
        k = 0
        while k < min(len(ref), len(p.stdout)) and ref[k] == p.stdout[k]:
            k += 1
        check(ref[:k].endswith(b'EAT EMERALD\r\n') and b'GO PILL' in ref[k - 400:k]
              and b'INVENTORY LINTEL' in ref[k - 400:k] and p.stdout[k:].startswith(b'I think I just lost my appetite'),
              'fuzz3, recorded on SINTRAN, parts from the fixed game only where GO PILL had left PILL for '
              'INVENTORY LINTEL and EAT EMERALD (byte %d)' % k, p.stdout[max(0, k - 600):k + 200].decode('latin-1'))

        out = play(work, PIT + b'POUR SOUP\rQUIT\r', '--no-fixes')
        check("You don't carry any soup" in reply(out, 'POUR SOUP'), '--no-fixes: the soup bug is back', out)

        out = play(work, CRYPT + b'OPEN COFFIN\rSAVE\rcrypt\nQUIT\r', '--no-fixes')
        check(METAL in reply(out, 'OPEN COFFIN') and os.path.exists(os.path.join(work, 'crypt.SAV')),
              '--no-fixes: the coffin bug is back', out)
        out = play(work, b'OPEN COFFIN\rQUIT\r', 'crypt')
        check(OPENED in reply(out, 'OPEN COFFIN'), 'that saved game has the fixes when resumed', out)
        out = play(work, b'OPEN COFFIN\rQUIT\r', '--no-fixes', 'crypt')
        check(METAL in reply(out, 'OPEN COFFIN'), 'and not with --no-fixes', out)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('all fix checks passed')


if __name__ == '__main__':
    main()
