"""Play the game through to YOU MADE IT! on the port, with the orc switched off.

usage: python tests\\winnable.py [-v]

tests\\walk-debug.txt is a route through every puzzle, collecting all 27
treasures into the house.  Its first line after the adventure's name is a
--debug poke that makes rule 29 (the orc appearing out of the dark) a verb
rule, so that the AUTO rules stop before it: with no orc there is no death,
and the route plays the same every time.  Every command must be understood
(no "I CAN'T DO THAT YET", "YOU CANNOT GO THAT WAY", "I SEE NO ...").
-v prints the game.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), 'tools'))
from runport import run  # noqa: E402

BAD = re.compile(r"I CAN'T DO THAT YET|YOU CANNOT GO THAT WAY|I SEE NO |WHAT IS |YOU DON'T HAVE|"
                 r"TOO HEAVY|RUN ERROR|YOU ARE DEAD|BROKE YOUR NECK")


def main():
    cmds = [l.rstrip('\n') for l in open(os.path.join(HERE, 'walk-debug.txt'))
            if l.strip() and not l.startswith('# ')]
    out = run(''.join(c + '\r' for c in cmds).encode('latin-1'), extra=['--debug'])
    if '-v' in sys.argv:
        print(out)
    trouble = BAD.findall(out)
    won = 'YOU MADE IT!' in out
    score = re.search(r'TOTAL OF (\d+) POINTS\nON A SCALE FROM 0 TO 10 THAT RATES A +(\d+)!\nHOPE', out)
    print('%d commands; %s; %s' % (len(cmds) - 1, 'won' if won else 'NOT WON',
                                   'no trouble' if not trouble else 'TROUBLE: %r' % trouble[:5]))
    if score:
        print('%s points, rated %s of 10' % score.groups())
    return 0 if won and not trouble else 1


if __name__ == '__main__':
    sys.exit(main())
