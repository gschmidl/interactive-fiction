#!/usr/bin/env python3
"""Regression tests for the Qork port.

    python tests\\regress.py [-v]

Every game is played in a scratch copy of the port (qork.exe, qork.dat,
qork.ini), so saves\\ is never touched.  At the end the Cyber comparison
(tests\\cmpcyber.py: seven sessions against NOS 2.8.7's print files), the
win (tests\\win.py) and a short fuzz run (tests\\fuzz.py) are run too.
-v prints every transcript.
"""
import filecmp
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import cmpcyber  # noqa: E402
import fuzz  # noqa: E402
import win  # noqa: E402

VERBOSE = '-v' in sys.argv[1:]
failed = []


def report(name, ok, text=''):
    print('%-50s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failed.append(name)
    if text and (VERBOSE or not ok):
        print('    ' + text.replace('\n', '\n    '))


def scratch(files=('qork.exe', 'qork.dat', 'qork.ini')):
    tmp = tempfile.mkdtemp(prefix='qork-')
    for f in files:
        shutil.copy(os.path.join(PORT, f), tmp)
    return tmp


def play(cmds, args=(), where=None):
    """stdout, stderr and exit code of qork.exe given CMDS (lines) in WHERE,
    a scratch copy made and thrown away here if None"""
    own = where is None
    if own:
        where = scratch()
    try:
        data = ''.join(c + '\n' for c in cmds).encode('latin-1')
        r = subprocess.run([os.path.join(where, 'qork.exe')] + list(args),
                           input=data, capture_output=True, timeout=60)
        return (r.stdout.decode('latin-1').replace('\r\n', '\n'),
                r.stderr.decode('latin-1').replace('\r\n', '\n'), r.returncode)
    finally:
        if own:
            shutil.rmtree(where, ignore_errors=True)


def in_order(text, parts):
    """every part in TEXT, one after another; the first one missing"""
    at = 0
    for p in parts:
        i = text.find(p, at)
        if i < 0:
            return p
        at = i + len(p)
    return None


def game(name, cmds, parts, args=(), where=None, code=0):
    out, err, rc = play(cmds, args, where)
    miss = in_order(out + err, parts)
    report(name, miss is None and rc == code,
           out + err + ('\n[missing: %s]' % miss if miss else '') +
           ('\n[exit %d, not %d]' % (rc, code) if rc != code else ''))
    return out


# ------------------------------------------------------------ options
def t_options():
    out, err, rc = play([], ['--help'])
    report('--help: the usage, exit 0', rc == 0 and out.startswith('usage: qork'), out + err)
    out, err, rc = play([], ['-h'])
    report('-h: the same', rc == 0 and out.startswith('usage: qork'), out + err)
    out, err, rc = play([], ['--frobnicate'])
    report('an unknown option: refused, exit 2',
           rc == 2 and 'unknown option --frobnicate' in err, out + err)
    for bad in (['--seed'], ['--seed', '0'], ['--seed', '12x'],
                ['--seed', '281474976710656']):
        out, err, rc = play([], bad)
        report('%s: refused, exit 2' % ' '.join(bad),
               rc == 2 and '--seed takes a number' in err, out + err)
    game('--no-fixes: accepted, the game runs', ['QUIT', 'Y'],
         ['WELCOME TO QORK.', 'DO YOU WISH TO LEAVE THE GAME?'], ['--no-fixes'])


# ------------------------------------------------------------ input
UPPER = ['OPEN MAILBOX', 'TAKE LEAFLET', 'READ LEAFLET', 'N', 'E',
         'OPEN WINDOW', 'W', 'INVENTORY', 'QUIT', 'Y']


def t_lower_case():
    up = play(UPPER)[0]
    low = play([c.lower() for c in UPPER])[0]
    mixed = play([c.capitalize() for c in UPPER])[0]
    report('lower case (6/12: ^ and the letter) plays as upper',
           up == low == mixed and 'KITCHEN' in up, low)


def t_yesno():
    game('QUIT, n: play on; QUIT, yes: the end',
         ['QUIT', 'n', 'LOOK', 'QUIT', 'yes', 'LOOK'],
         ['DO YOU WISH TO LEAVE THE GAME?', 'WEST OF A BIG WHITE HOUSE',
          'DO YOU WISH TO LEAVE THE GAME?'])
    out = play(['QUIT', 'Y', 'LOOK'])[0]
    report('... and nothing is read after it', out.count('WEST OF A BIG') == 1, out)


def t_end_of_input():
    out = game('end of input: "I cannot hear you!", exit 0', ['LOOK'],
               ['MAILBOX', 'I cannot hear you!'])
    report('... once, and the program ends quietly',
           out.count('I cannot hear you!') == 1 and out.rstrip().endswith('I cannot hear you!'), out)
    game('no input at all: the same', [], ['WELCOME TO QORK.', 'I cannot hear you!'])
    game('end of input at a question: asked again, then the end', ['QUIT'],
         ['DO YOU WISH TO LEAVE THE GAME?', 'PLEASE ANSWER THE QUESTION.',
          'DO YOU WISH TO LEAVE THE GAME?'])


def t_dice():
    cmds = open(os.path.join(HERE, 'cmds2.txt')).read().split('\n')
    a = play(cmds)[0]
    b = play(cmds)[0]
    c = play(cmds, ['--seed', '4242'])[0]
    d = play(cmds, ['--seed', '4242'])[0]
    report('the Cyber\'s dice: every game the same', a == b, a)
    report('--seed N: other dice, the same every time', c == d and c != a, c)


# ------------------------------------------------------------ files
def t_save_restore():
    where = scratch()
    try:
        game('SAVE writes saves\\QORK.SAV', ['N', 'SAVE', 'QUIT', 'Y'], ['SAVED.'],
             where=where)
        report('... which is there', os.path.exists(os.path.join(where, 'saves', 'QORK.SAV')))
        game('RESTORE in the next game brings it back',
             ['RESTORE', 'LOOK', 'QUIT', 'Y'],
             ['RESTORED.', 'RESTORED.', 'NORTH SIDE OF A WHITE HOUSE'], where=where)
    finally:
        shutil.rmtree(where, ignore_errors=True)
    game('RESTORE before any SAVE: the game as it starts',
         ['N', 'RESTORE', 'LOOK', 'QUIT', 'Y'],
         ['NORTH SIDE OF A WHITE HOUSE', 'RESTORED.', 'WEST OF A BIG WHITE HOUSE'])


def t_build():
    where = scratch(('qork.exe',))
    try:
        shutil.copy(os.path.join(PORT, '..', 'src_original', 'qork.txt'), where)
        r = subprocess.run([os.path.join(where, 'qork.exe'), '--build'],
                           capture_output=True, timeout=120)
        out = r.stdout.decode('latin-1')
        same = all(filecmp.cmp(os.path.join(where, f), os.path.join(PORT, f), shallow=False)
                   for f in ('qork.dat', 'qork.ini'))
        report('--build: qork.dat and qork.ini again, the same',
               r.returncode == 0 and "CREATING NEW 'DTEXT.DAT'" in out and same, out)
    finally:
        shutil.rmtree(where, ignore_errors=True)
    where = scratch(('qork.exe', 'qork.ini'))
    try:
        out, err, rc = play(['LOOK'], where=where)
        report('no qork.dat: said so, exit 1', rc == 1 and 'qork.dat is missing' in err, out + err)
    finally:
        shutil.rmtree(where, ignore_errors=True)
    where = scratch(('qork.exe', 'qork.dat'))
    try:
        out, err, rc = play(['LOOK'], where=where)
        report('no qork.ini: said so, exit 1', rc == 1 and 'qork.ini is missing' in err, out + err)
    finally:
        shutil.rmtree(where, ignore_errors=True)


# ------------------------------------------------------------ GUARDIAN
def t_guardian():
    game('GUARDIAN with the password: at your service',
         ['GUARDIAN', 'EXPLURIBUSONION TKMG', 'DA', '1', '1', 'EX', 'QUIT', 'Y'],
         ['WHO SUMMONS THE GUARDIAN', 'AT YOUR SERVICE.', 'LIMITS:',
          '   1      2      0      0    151      0      0 000000', 'GDN>'])
    game('GUARDIAN without it: dust',
         ['GUARDIAN', 'FROBOZZ', 'Y', 'QUIT', 'Y'],
         ['WRONG, CRETIN!', 'DO YOU WISH ME TO TRY TO PATCH YOU?', 'NOW LET ME SEE...'])


def main():
    t_options()
    t_lower_case()
    t_yesno()
    t_end_of_input()
    t_dice()
    t_save_restore()
    t_build()
    t_guardian()
    print()
    sys.argv = ['cmpcyber.py']
    if cmpcyber.main() != 0:
        failed.append('cmpcyber')
    sys.argv = ['win.py']
    if win.main() != 0:
        failed.append('win')
    sys.argv = ['fuzz.py']
    if fuzz.main() != 0:
        failed.append('fuzz')
    print()
    print('%d FAILED: %s' % (len(failed), ', '.join(failed)) if failed else 'all passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
