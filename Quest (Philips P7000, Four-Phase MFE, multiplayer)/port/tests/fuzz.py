"""Random play: many one-player games of random commands, each a scripted run.

    python fuzz.py [GAMES [TURNS [SEED]]] [--real-clock]      (defaults 60, 80, 1)

--real-clock runs each game on the real clock with -u, as play.bat does: at the real
machine's speed, two seconds a line, and not repeatable.

Each game signs on, types TURNS random lines (known words and nonsense, several sentences on
a line, speech, the editing keys' effect is not covered), and QUITs.  Every third game uses
the easy library.  A game fails if the machine stops, the run does not end, the exit is not
0, or the transcript does not end with QUEST's last words for the player.  A failing game's
input is written to fuzz-fail-SEED-N.in in the current folder (run this from a scratch
folder).
"""
import random
import sys

from common import run

WORDS = ('LOOK SEARCH GET DROP TAKE GIVE THROW SWING HIT KILL ATTACK FIGHT EAT DRINK OPEN CLOSE '
         'READ WEAR WIELD PUT LIGHT CLIMB JUMP RUN WALK GO WAIT SLEEP REST WINK SMILE WAVE '
         'INVENTORY HELP AGAIN IT ALL EVERYTHING EVERYONE ME AT TO THE A WITH FROM IN ON '
         'NORTH SOUTH EAST WEST NE NW SE SW UP DOWN N S E W U D '
         'SWORD AXE KNIFE DAGGER COIN GOLD RING STAFF SCEPTER BADGE LAMP TORCH ROPE KEY BOX '
         'PRINCE WIZARD KING BEAR RAT WOLF DEMON TROLL DRAGON HEINO').split()
DIRS = 'NORTH SOUTH EAST WEST NE NW SE SW UP DOWN N S E W'.split()
THINGS = 'SWORD AXE KNIFE DAGGER COIN RING STAFF SCEPTER BADGE LAMP ROPE KEY IT ALL'.split()
BEINGS = 'PRINCE WIZARD BEAR RAT WOLF DEMON TROLL DRAGON EVERYONE ME'.split()
ENDINGS = ('You quit.', 'You die', 'You have died', 'killed', 'You are dead')


def sentence(rnd):
    """play, mostly: walking about, searching, taking things, fighting; and some nonsense"""
    r = rnd.random()
    if r < 0.25:
        return rnd.choice(DIRS)
    if r < 0.30:
        return rnd.choice(['RUN ', 'WALK ']) + rnd.choice(DIRS)
    if r < 0.45:
        return rnd.choice(['LOOK', 'SEARCH', 'INVENTORY', 'LOOK AT ME', 'SEARCH', 'AGAIN',
                           'LOOK AT ' + rnd.choice(BEINGS + THINGS)])
    if r < 0.55:
        return rnd.choice(['GET ', 'TAKE ', 'DROP ', 'GET THE ']) + rnd.choice(THINGS)
    if r < 0.65:
        return rnd.choice(['SWING %s AT %s', 'THROW %s AT %s', 'HIT %s WITH %s',
                           'GIVE %s TO %s']) % ((rnd.choice(THINGS), rnd.choice(BEINGS))
                                                 if rnd.random() < 0.5 else
                                                 (rnd.choice(BEINGS), rnd.choice(THINGS)))
    if r < 0.70:
        return '"' + ' '.join(rnd.choice(WORDS) for _ in range(rnd.randint(1, 5)))
    if r < 0.78:
        return ''.join(rnd.choice('ABCDEFGHIJKLMNOPQRSTUVWXYZ') for _ in range(rnd.randint(1, 8)))
    return ' '.join(rnd.choice(WORDS) for _ in range(rnd.randint(1, 4)))


def game_lines(rnd, turns):
    lines = [rnd.choice(['Heino', 'Anna', 'X', 'Abcdefghijk']), rnd.choice(['yes', 'no', 'maybe'])]
    if lines[-1] == 'maybe':
        lines.append('no')
    for _ in range(turns):
        n = 1 if rnd.random() < 0.8 else rnd.randint(2, 3)
        lines.append('. '.join(sentence(rnd) for _ in range(n)).lower()
                     if rnd.random() < 0.3 else '. '.join(sentence(rnd) for _ in range(n)))
    lines += ['quit', 'yes']
    return lines


def main():
    real = '--real-clock' in sys.argv[1:]
    argv = [a for a in sys.argv[1:] if a != '--real-clock']
    games = int(argv[0]) if len(argv) > 0 else 60
    turns = int(argv[1]) if len(argv) > 1 else 80
    seed = int(argv[2]) if len(argv) > 2 else 1
    rnd = random.Random(seed)
    failed = 0
    for g in range(games):
        lines = game_lines(rnd, turns)
        args = ['--easy'] if g % 3 == 2 else []
        why = None
        try:
            code, out, err = run(lines, args, timeout=600 + (4 * turns if real else 0),
                                 fixed_clock=not real)
            tail = out[-400:]
            if code != 0 or err.strip():
                why = 'exit %d: %s' % (code, err.strip()[:300])
            elif not any(e in tail for e in ENDINGS):
                why = 'the transcript does not end with the player leaving:\n' + tail
        except Exception as e:                  # a hang
            why = 'did not end: %s' % e
        if why:
            failed += 1
            name = 'fuzz-fail-%d-%d.in' % (seed, g)
            with open(name, 'w') as f:
                f.write('\n'.join(lines) + '\n')
            print('FAIL  game %d%s: %s (input in %s)' % (g, ' --easy' if args else '', why, name))
        elif g % 10 == 9:
            print('ok    %d games' % (g + 1))
    print('%d of %d games failed' % (failed, games) if failed else 'all %d games passed' % games)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
