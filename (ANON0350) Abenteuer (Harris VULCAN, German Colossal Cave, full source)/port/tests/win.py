#!/usr/bin/env python3
"""Can the game be won - 350 of 350 points?

    python tests\\win.py [-v]

Not a walkthrough played against the dice: the state is set, in a saved
game made from NEUSPIEL.DAT (tests\\state.py, with the program's own MOVE
and CARRY), to what the long middle of the game leaves - every treasure
found and in the building, the magazine left at Witt's End, the dwarves
and the pirate gone, the lamp lit and carried, the player in the Hall of
Mists, one turn left on CLOCK1 and two on CLOCK2 - and then the program
does the rest itself: BRING (with -u, no wait), the cave closes, the
repository is set up, the marked rod is carried from the SW end to the
NE end, and SPRENG from the SW end is the best way out (BONUS 133).

The ending is checked: 350 of 350 points and the top rank.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import state  # noqa: E402
import transcript  # noqa: E402

COMMANDS = ['bring win',
            'bestand',          # CLOCK1 runs out: the cave is closing
            'bestand',          # CLOCK2 1
            'bestand',          # CLOCK2 0: the cave is closed
            'sw', 'nimm stab', 'no', 'leg stab', 'sw', 'spreng']

EXPECT = ['DU ERZIELTEST 350 VON MOEGLICHEN 350 PUNKTEN',
          'DIE GESAMTE ABENTEURERSCHAFT HULDIGT DIR, ALTMEISTER DER ABENTEURER!']


def endgame(s):
    building, hall, witts_end = 3, 15, 108
    for i in range(50, s['MAXTRS'] + 1):
        if s['PTEXT', i] == 0:
            continue
        if s['FIXED', i] > 0:               # the rug's second place
            s.carry(i + 100, s['FIXED', i])
        s['FIXED', i] = 0
        s.move(i, building)
        s['PROP', i] = 0
    s['TALLY'] = 0
    s.move(s['MAGZIN'], witts_end)
    s['DFLAG'] = 2
    for i in range(1, 7):
        s['DLOC', i] = 0
    s.carry(s['LAMP'], s['PLACE', s['LAMP']])
    s['PROP', s['LAMP']] = 1
    for v in ('LOC', 'OLDLOC', 'OLDLC2', 'NEWLOC'):
        s[v] = hall
    s['CLOCK1'] = 1
    s['CLOCK2'] = 2


def main():
    where = transcript.scratch()
    try:
        s = state.State(os.path.join(where, 'NEUSPIEL.DAT'))
        endgame(s)
        os.makedirs(os.path.join(where, 'saves'), exist_ok=True)
        s.save(os.path.join(where, 'saves', 'WIN.SAV'))
        text, code = transcript.run(COMMANDS, ['-u'], where)
    finally:
        shutil.rmtree(where, ignore_errors=True)
    if '-v' in sys.argv[1:]:
        print(text)
    missing = [e for e in EXPECT if e not in text]
    for e in missing:
        print('win: missing: ' + e)
    print('win: %s (exit %d)' % ('won, 350 of 350' if not missing and code == 0
                                 else 'FAILED', code))
    return 1 if missing or code else 0


if __name__ == '__main__':
    sys.exit(main())
