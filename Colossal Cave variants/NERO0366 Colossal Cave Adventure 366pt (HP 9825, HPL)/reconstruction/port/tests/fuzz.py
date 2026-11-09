"""Play many random games on the port and report any HPL error, crash or hang.

usage: fuzz.py [GAMES] [COMMANDS] [--no-fixes]

Each game takes the lamp and keys, goes down through the grate, then types
COMMANDS random words from the game's vocabulary (mostly directions, so it
gets about the cave and meets the dwarves and the pirate).  The seed of
game n is n, so a failure can be replayed with  advent.exe --seed n.
"""
import os
import random
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, '..', 'advent.exe')
VOCAB = os.path.join(HERE, '..', '..', 'data', 'sec4_vocabulary.txt')

PREFIX = ['no', 'in', 'get lamp', 'get keys', 'get food', 'get bottle', 'out', 's', 's', 's',
          'unlock grate', 'd', 'on', 'w', 'get cage', 'w', 'w', 'w', 'get bird', 'w', 'd']


def words():
    motion, verbs, objects = [], [], []
    for line in open(VOCAB, encoding='utf-8'):
        parts = line.split()
        if len(parts) < 2:
            continue
        w, v = parts[-1].strip('"').lower(), int(parts[0])
        if v in (2018, 2024, 2030):         # quit, score, suspend: they end the game
            continue
        if v < 1000:
            motion.append(w)
        elif v < 2000:
            objects.append(w)
        elif v < 3000:
            verbs.append(w)
    return motion, verbs, objects


def game(n, ncmd, motion, verbs, objects, extra):
    rnd = random.Random(n)
    cmds = list(PREFIX)
    for _ in range(ncmd):
        r = rnd.random()
        if r < 0.55:
            cmds.append(rnd.choice(motion))
        elif r < 0.75:
            cmds.append(rnd.choice(verbs) + ' ' + rnd.choice(objects))
        elif r < 0.85:
            cmds.append(rnd.choice(verbs))
        elif r < 0.95:
            cmds.append(rnd.choice(['yes', 'no', 'y', 'n']))
        else:
            cmds.append(rnd.choice(['inventory', 'look', 'xyzzy', 'plugh', 'get all', 'drop all']))
    save = os.path.join(tempfile.gettempdir(), 'nero_fuzz_%d.sav' % n)
    if os.path.exists(save):
        os.remove(save)
    args = [EXE, '--seed', str(n), '--no-pause', '--saves', save] + extra
    try:
        p = subprocess.run(args, input='\n'.join(cmds) + '\n', capture_output=True, text=True,
                           encoding='latin-1', timeout=30)
    except subprocess.TimeoutExpired:
        return 'hang', ''
    finally:
        if os.path.exists(save):
            os.remove(save)
    if p.returncode != 0 or 'HPL error' in p.stderr:
        return 'error %d: %s' % (p.returncode, p.stderr.strip()), p.stdout[-600:]
    return None, p.stdout


def main():
    ngames = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 300
    ncmd = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 500
    extra = ['--no-fixes'] if '--no-fixes' in sys.argv else []
    motion, verbs, objects = words()
    bad = 0
    deepest = 0
    for n in range(1, ngames + 1):
        err, out = game(n, ncmd, motion, verbs, objects, extra)
        if err:
            bad += 1
            print('seed %d: %s' % (n, err))
            print('   ...' + out.replace('\n', '\n   '))
        else:
            for line in out.splitlines():
                if line.startswith('You scored '):
                    deepest = max(deepest, int(line.split()[2]))
    print('%d games, %d failed; best score %d' % (ngames, bad, deepest))
    sys.exit(1 if bad else 0)


main()
