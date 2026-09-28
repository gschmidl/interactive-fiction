"""Checks for the Windows port of Explore 5.3.

    python tests/check.py            the sessions, then the comparison
    python tests/check.py --quick    the sessions only

1. Sessions: short games piped through explore.exe, each in a scratch
   folder (--var), checked for the text they must print: the control
   arguments and options, SAVE and RESTORE, abbrevs, the sorcerer changing
   the hours, the shell escape, SEND, start_up.explore, the read-only
   fallback, and the end of input.

2. Comparison: random games (the game's own commands, objects, creatures,
   room names and magic words), and tours that visit every room, kill every
   creature and win with 500 points, played by drive.pl with a fixed seed
   and clock, twice each: on this port's Perl with its lib, and on Linux
   Perl in WSL with the author's unchanged MBasic and Explore::Builtins
   (../src_original).  Every transcript must be the same on both.
"""
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ORIG = os.path.join(os.path.dirname(PORT), 'src_original')
EXE = os.path.join(PORT, 'explore.exe')
PERL = os.path.join(PORT, 'perl', 'bin', 'perl.exe')

failures = []


def run(args, text, var=None):
    """explore.exe ARGS with TEXT as its input, in a scratch --var folder"""
    own = var is None
    if own:
        var = tempfile.mkdtemp(prefix='explore-')
    try:
        p = subprocess.run([EXE, '--var', var] + args, input=text.encode('latin-1'),
                           capture_output=True, timeout=120)
        out = p.stdout.decode('latin-1').replace('\r\n', '\n')
        err = p.stderr.decode('latin-1').replace('\r\n', '\n')
        return p.returncode, out, err, sorted(os.listdir(var)) if os.path.isdir(var) else []
    finally:
        if own:
            shutil.rmtree(var, ignore_errors=True)


def check(name, ok, detail=''):
    print('%-58s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failures.append(name)
        if detail:
            print('    ' + detail.strip().replace('\n', '\n    ')[:3000])


def has(out, *texts):
    return all(t in out for t in texts)


def sessions():
    rc, out, err, _ = run(['--help'], '')
    check('--help', rc == 0 and has(out, 'usage: explore', '-pathname PATH'), out + err)

    rc, out, err, _ = run(['--bogus'], '')
    check('an unknown --option is refused', rc != 0 and 'unknown option --bogus' in err, err)

    rc, out, err, files = run([], 'quit\nyes\n')
    check('a plain start: banner, news, first room', rc == 0 and has(
        out, 'Version 5.3\n', 'This is the resurrected Explore game',
        'Welcome to "Explore"', 'edge of a small clearing', 'You scored  0'), out + err)
    check('the first start seeds the writable folder',
          files == ['explore.rwdir', 'hours.data', 'winners.data'], repr(files))

    rc, out, err, _ = run(['-version', '-brief'], 'quit\nyes\n')
    check('-version and -brief', rc == 0 and 'Explore Version 5.3 of 06 June 1980' in out
          and 'Welcome' not in out and 'resurrected' not in out, out + err)

    rc, out, err, _ = run(['-no_version'], 'quit\nyes\n')
    check('-no_version', rc == 0 and 'Version 5.3' not in out and 'Welcome' in out, out)

    rc, out, err, _ = run(['-pn', 'missing.data'], '')
    check('-pn with a missing database', 'explore: Entry not found. missing.data' in out, out + err)

    rc, out, err, _ = run(['-xyz'], '')
    check('an unknown control argument',
          'not implemented by this command. -xyz' in out, out + err)

    rc, out, err, _ = run([], 'in\nget\non\nwhat\nscore\nturns\nquit\nyes\n')
    check('in, get, on, what, score, turns', rc == 0 and has(
        out, 'small wooden building', 'Okay.', 'Your flashlight is now on.',
        'You are currently carrying:\n\nflashlight', 'You have taken  5  turns.'), out + err)

    var = tempfile.mkdtemp(prefix='explore-')
    try:
        rc, out, err, files = run([], 'in\nget\nsave t1\n', var)
        check('save ends the game and writes NAME.explore',
              rc == 0 and 'Game saved.....' in out and 't1.explore' in files, out + repr(files))
        rc, out, err, files = run([], 'restore t1\nwhat\nquit\nyes\n', var)
        check('restore brings the game back (and deletes the file)', rc == 0 and has(
            out, 'You are currently carrying:\n\nflashlight') and 't1.explore' not in files,
            out + repr(files))
        # 1980's own bug: the command table (6230) takes only "save NAME", so
        # the SAVED_GAME_ default of 8970-9030 cannot be reached
        rc, out, err, files = run([], 'in\nsave\nrestore\nquit\nyes\n', var)
        check('save and restore with no name are unknown words (as in 1980)',
              has(out, 'I don\'t know the word "save".', 'I don\'t know the word "restore".'),
              out + repr(files))

        user = os.environ.get('USERNAME', 'user')
        rc, out, err, files = run([], 'ab\n.a i what\ni\n.l\n.p\nquit\nyes\n', var)
        abbrev = os.path.join(var, user + '.explore_abbrev')
        check('abbrevs: .a defines one, it expands, .l lists it', rc == 0 and has(
            out, "You aren't carrying anything!",
            '>udd>Explore>Player>%s.explore_abbrev' % user)
            and re.search(r'  i +what\n', out) and os.path.isfile(abbrev),
            out + err + repr(files))
        rc, out, err, files = run(['-ab', user], 'i\nquit\nyes\n', var)
        check('-ab uses the saved abbrevs', "You aren't carrying anything!" in out, out + err)
        check('quit deletes the abbrev scratch file',
              user + '.explore_abbrev_edit' not in files, repr(files))

        rc, out, err, _ = run([], 'sorcerer\nhello\nhours\nnew\nno\n\n\n\n'
                              'Winds howl in the cave today.\n.\nquit\nquit\nyes\n', var)
        check('the sorcerer (magic word "hello") changes the hours', rc == 0 and has(
            out, 'Magic word, please:', 'Request?', 'New Sorcerer Word',
            'Do you want to change the hours (yes or no)?', 'New Message Of The Day'), out + err)
        hours = open(os.path.join(var, 'hours.data'), 'rb').read().decode('latin-1')
        check('... and hours.data is rewritten with them',
              hours.replace('\r\n', '\n').startswith('arj\n0000,2359\n')
              and 'Winds howl in the cave today.' in hours, hours)
        rc, out, err, _ = run([], 'sorcerer\nhello\nsorcerer\nnew\nquit\nquit\nyes\n', var)
        check('... the new magic word works, the old one does not',
              has(out, 'Wrong!', 'Winds howl in the cave today.', 'Request?'), out + err)
    finally:
        shutil.rmtree(var, ignore_errors=True)

    rc, out, err, _ = run([], '..echo shell escape works\nm echo so does m\nquit\nyes\n')
    check('the shell escape (.. and m)', rc == 0 and has(
        out, 'shell escape works', 'so does m'), out + err)

    rc, out, err, _ = run([], 'send\nLippard.Scouting\nquit\nyes\n')
    check('send says it cannot', rc == 0 and 'Person.Project?' in out
          and 'cannot send to Lippard.Scouting' in err, out + err)

    var = tempfile.mkdtemp(prefix='explore-')
    try:
        with open(os.path.join(var, 'start_up.explore'), 'w') as f:
            f.write('in\nscore\n')
        rc, out, err, _ = run([], 'what\nquit\nyes\n', var)
        check('start_up.explore runs first', rc == 0 and has(
            out, '? in\n', 'small wooden building', '? score\n', "You aren't carrying"), out + err)
    finally:
        shutil.rmtree(var, ignore_errors=True)

    tmp = tempfile.mkdtemp(prefix='explore-')
    try:
        blocker = os.path.join(tmp, 'afile')
        open(blocker, 'w').close()
        rc, out, err, _ = run([], 'quit\nyes\n', os.path.join(blocker, 'sub'))
        check('an unwritable --var plays read-only', rc == 0 and 'playing read-only' in err
              and 'Welcome' in out and 'You scored' in out, out + err)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    rc, out, err, _ = run([], 'in\nlook\n')
    check('the end of input ends the game',
          rc == 0 and out.count('center of a small wooden building') == 2, out + err)
    rc, out, err, _ = run([], 'sorcerer\n')
    check('... also at the magic word', rc == 0 and 'Magic word, please:' in out, out + err)


# --- the comparison -----------------------------------------------------------

OBJECTS = ['flashlight', 'recharger', 'wand', 'tube', 'nodules', 'knife', 'spear',
           'diamonds', 'gold bars', 'silver bars', 'platinum bars', 'book', 'magazine',
           'weasel', 'disk', 'dust', 'cross', 'statue', 'printout', 'ring', 'dog',
           'sword', 'nest', 'plaque', 'skin', 'tape', 'mudball', 'medallion', 'rock',
           'liquid', 'obelisk', 'plate', 'staff', 'sapphire', 'emeralds', 'pearl', 'ruby',
           'coin', 'sculpture', 'bridge', 'abyss', 'fissure']
CREATURES = ['scylla', 'basilisk', 'dragon', 'cyclops', 'dark', 'scorpion', 'roc',
             'troll', 'coronis', 'dwarf']
ROOMS = ['shack', 'cellar', 'granite', 'forest', 'phosphorescent', 'library', 'mosquito',
         'gold', 'silver', 'power', 'platinum', 'weapon', 'weapon2', 'heated', 'elbow',
         'torches', 'sand', 'elbow2', 'sorcerer', 'lower-cross', 'auxiliary', 'crossroads',
         'abyss', 'brink', 'tunnel1', 'grafitti', 'tunnel2', 'tunnel3', 'tunnel4',
         'tunnel6', 'tunnel5', 'west-hall', 'mid-hall', 'east-hall', 'dark', 'thin',
         'waterfall', 'short-hall', 'elbow3', 'metal', 'weasel', 'terminal', 'Lippard',
         'chamber', 'light', 'twisted', 'experiment', 'basilisk', 'vault', 'finish',
         'outside', 'scroll', 'printout-room', 'edge', 'fissure', 'no-exits', 'deadend']
MOVES = ['n', 's', 'e', 'w', 'u', 'd', 'north', 'south', 'east', 'west', 'up', 'down',
         'in', 'out']
MAGIC = ['winner', 'nilrem', 'xineohp', 'indiana', 'nlocnil', 'erolpxe', 'Mark Davis']
WORDS = ['look', 'what', 'score', 'turns', 'brief', 'full', 'fast', 'on', 'off', 'eat',
         'plug', 'jump', 'read', 'cross', 'wave', 'swing', 'throw', 'get', 'drop',
         'get all', 'drop all', 'help', 'commands', 'info', 'news', 'hours', 'modes',
         'list_help', 'lh', 'help moving', 'help magic', 'help sorcerer', 'help xyz',
         'stm brief', 'stm full,quit', 'set_modes fast', 'stm multip', 'stm bogus',
         'talk', 'multics', 'ab', '.a zz look', '.ab yy score', 'zz', 'yy please', '.l',
         '.la z', '.ls z', '.lx sc', '.s zz', '.d zz', '.q', '.', '.x', 'xyzzy', 'plugh',
         'yes', 'no', 'restore', 'look; score', 'n;n;e', '']


def command(rnd):
    r = rnd.random()
    if r < 0.40:
        return rnd.choice(MOVES)
    if r < 0.55:
        return rnd.choice(['get ', 'drop ', 'throw ', 'wave ', 'swing ']) + rnd.choice(OBJECTS)
    if r < 0.62:
        return rnd.choice(ROOMS)
    if r < 0.66:
        return rnd.choice(OBJECTS + CREATURES)
    if r < 0.67:
        return rnd.choice(MAGIC)
    if r < 0.68:
        # the magic word (from a pipe it is read like any line), then requests
        return 'sorcerer\n' + rnd.choice(['hello', 'nope']) + '\n' + \
            rnd.choice(['whom', 'mail', 'move\n' + str(rnd.randint(1, 58)), 'x']) + '\nquit'
    return rnd.choice(WORDS)


def game(seed, length):
    """a random game; most take the flashlight first and switch it on, or
    they soon fall into a pit in the dark"""
    rnd = random.Random(seed)
    lines = ['in', 'get', 'on'] if seed % 4 else []
    lines += [command(rnd) for _ in range(length)]
    return '\n'.join(lines) + '\nquit\nyes\n'


# the creatures, their rooms and the thing that kills each (explore.data)
KILLS = [('scylla', 17, 'sword'), ('basilisk', 48, 'weasel'), ('dragon', 30, 'wand'),
         ('cyclops', 24, 'spear'), ('dark', 35, 'liquid'), ('scorpion', 18, 'medallion'),
         ('roc', 45, 'skin'), ('troll', 10, 'rock'), ('coronis', 42, 'mudball')]


def tour():
    """The deep end, which random games never reach: the sorcerer's MOVE
    visits every room to take everything, each creature is killed with its
    weapon, the treasures are left in the shack, and LOOK in the tablet
    room (50) should then win with 500 points and write the winner."""
    def move(room):
        return ['sorcerer', 'hello', 'move', str(room), 'quit']
    lines = ['in', 'get', 'on', 'winner']
    for room in range(1, 59):
        lines += move(room) + ['get all']
    for creature, room, weapon in KILLS:
        lines += move(room) + ['look', 'throw ' + weapon, 'get all']
    lines += move(56) + ['nilrem', 'drop all', 'score', 'get flashlight']
    lines += move(50) + ['look', 'Tester', 'yes', 'quit', 'yes']
    return '\n'.join(lines) + '\n'


def wslpath(p):
    p = os.path.abspath(p).replace('\\', '/')
    return '/mnt/' + p[0].lower() + p[2:]


def both(name, text, seed):
    """play TEXT with SEED on both; report a difference; return the
    Windows transcript, or None if they differ"""
    windows_lib = os.path.join(PORT, 'lib')
    linux_lib = wslpath(os.path.join(ORIG, 'MBasic', 'lib')) + ':' + \
        wslpath(os.path.join(ORIG, 'explore', 'perl', 'lib'))
    text = text.encode('latin-1')
    work = tempfile.mkdtemp(prefix='explore-drive-')
    try:
        w = subprocess.run([PERL, os.path.join(PORT, 'tests', 'drive.pl'), windows_lib,
                            os.path.join(PORT, 'share'), work, str(seed)],
                           input=text, capture_output=True, timeout=300)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    l = subprocess.run(['wsl', '-d', 'Debian', '--', 'sh',
                        wslpath(os.path.join(PORT, 'tests', 'drive_wsl.sh')),
                        linux_lib, wslpath(os.path.join(ORIG, 'explore', 'perl', 'share')),
                        str(seed)], input=text, capture_output=True, timeout=300)
    wout = (w.stdout + b'\n--stderr--\n' + w.stderr).decode('latin-1').replace('\r\n', '\n')
    lout = (l.stdout + b'\n--stderr--\n' + l.stderr).decode('latin-1').replace('\r\n', '\n')
    if wout == lout and w.returncode == l.returncode:
        return wout
    a, b = wout.split('\n'), lout.split('\n')
    k = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
    check('%s is the same on both' % name, False,
          'line %d\n  windows: %r\n  linux:   %r\n(status %d / %d)'
          % (k + 1, a[k] if k < len(a) else None, b[k] if k < len(b) else None,
             w.returncode, l.returncode))
    return None


def comparison(games, length, tours):
    same = sum(both('game %d' % (g + 1), game(1000 + g, length), g + 1) is not None
               for g in range(games))
    check('%d random games of %d commands: the same on Windows and Linux' % (games, length),
          same == games, '%d of %d the same' % (same, games))
    outs = [both('tour %d' % (t + 1), tour(), 101 + t) for t in range(tours)]
    won = sum('You have won!' in o and 'Grand Master Explorer' in o for o in outs if o)
    check('%d tours of all 58 rooms (%d won with 500 points): the same on both'
          % (tours, won), all(outs) and won > 0)


if __name__ == '__main__':
    sessions()
    if '--quick' not in sys.argv:
        comparison(60, 150, 4)
    print('%d failed' % len(failures) if failures else 'all passed')
    sys.exit(1 if failures else 0)
