#!/usr/bin/env python3
"""Random play: GAMES games of TURNS random sentences each (words of the
game's own vocabulary, now and then SAVE and RESTORE), in a scratch copy of
dungeon.exe and its data base.  A game fails if the program does not end
with exit 0 at the end of its input, writes anything on stderr, or prints a
Fortran run-time error or the game's own PROGRAM ERROR (BUG).

    python fuzz.py [GAMES [TURNS [SEED]]]      (defaults 50, 200, 1)
"""
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))
FILES = ['dungeon.exe', 'ZORK_DBTXT', 'ZORK_PTXT', 'ZORK_PINDX']

DIRS = 'N S E W NE NW SE SW UP DOWN IN OUT LAND CROSS'.split()
VERBS = ('TAKE DROP OPEN CLOSE READ EXAMINE LOOK INVENTORY KILL ATTACK THROW PUT GIVE MOVE PUSH '
         'PULL TURN LIGHT EXTINGUISH EAT DRINK FILL POUR WAVE RUB TIE UNTIE CLIMB BOARD '
         'DISEMBARK INFLATE DEFLATE DIG BURN BREAK KICK KNOCK LISTEN SMELL WAIT SCORE TIME '
         'DIAGNOSE BRIEF VERBOSE SUPERBRIEF UNBRIEF UNSUPER JUMP PRAY HELLO TREASURE TEMPLE '
         'ECHO SPIN RING WIND LOCK UNLOCK SWIM TELL SAY YES NO HELP INFO').split()
OBJS = ('LAMP LANTERN SWORD KNIFE ROPE SACK GARLIC FOOD BOTTLE WATER LEAFLET MAILBOX WINDOW DOOR '
        'RUG TRAPDOOR CASE TROLL AXE TORCH COFFIN BOOK BELL CANDLES MATCHES BOAT PUMP WRENCH '
        'SCREWDRIVER COAL MACHINE DIAMOND EMERALD SHOVEL GUANO BUOY ROBOT CAKE THIEF BAG '
        'PAINTING TRUNK SCEPTRE CHALICE BAR KEYS GRATE LEAVES PAPER NEWSPAPER TUBE PUTTY '
        'CYCLOPS VAMPIRE BAT BRACELET IVORY JADE SKULL BASKET CHAIN RAILING POLE BUTTON').split()
PREPS = 'WITH IN ON AT TO FROM UNDER'.split()


def sentence(rng):
    r = rng.random()
    if r < 0.30:
        return rng.choice(DIRS)
    if r < 0.40:
        return rng.choice(VERBS)
    if r < 0.80:
        return '%s %s' % (rng.choice(VERBS), rng.choice(OBJS))
    if r < 0.97:
        return '%s %s %s %s' % (rng.choice(VERBS), rng.choice(OBJS), rng.choice(PREPS),
                                rng.choice(OBJS))
    return rng.choice(['SAVE', 'RESTORE', 'QUIT', 'Y', 'N', 'G', 'AGAIN', ''])


def main():
    games = int(sys.argv[1]) if len(sys.argv) > 1 else 50
    turns = int(sys.argv[2]) if len(sys.argv) > 2 else 200
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)
    work = tempfile.mkdtemp(prefix='bdung-fuzz-')
    for f in FILES:
        shutil.copy(os.path.join(PORT, f), work)
    failed = 0
    try:
        for g in range(games):
            lines = [sentence(rng) for _ in range(turns)]
            p = subprocess.run([os.path.join(work, 'dungeon.exe'), '--fixed-clock',
                                '--saves=' + os.path.join(work, 'saves')],
                               input=''.join(l + '\n' for l in lines), capture_output=True,
                               text=True, encoding='latin-1', timeout=300, cwd=work)
            why = None
            if p.returncode != 0:
                why = 'exit %d' % p.returncode
            elif p.stderr.strip():
                why = 'stderr: ' + p.stderr.strip()[-300:]
            elif 'Fortran runtime error' in p.stdout or ' PROGRAM ERROR ' in p.stdout:
                why = 'error: ' + p.stdout[-600:]
            if why:
                failed += 1
                path = os.path.join(work, 'failed-%d.in' % g)
                with open(path, 'w', encoding='latin-1', newline='\n') as f:
                    f.write('\n'.join(lines) + '\n')
                print('FAIL  game %d: %s (input kept in %s)' % (g, why, path))
            if (g + 1) % 10 == 0:
                print('ok    %d games' % (g + 1))
    finally:
        if not failed:
            shutil.rmtree(work, ignore_errors=True)
    print('%d of %d games failed' % (failed, games) if failed else 'all %d games passed' % games)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
