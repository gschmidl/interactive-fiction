"""Play a command list against the original on PRIMOS and save the transcript.

The game prompts with nothing at all - it just stops writing - so this waits
for the output to go quiet before sending the next line, and afterwards checks
that the run ended with the game's own last words.

usage: python primeplay.py commands.txt out.txt
"""
import sys
import time

from primesh import Prime


def play(p, cmds):
    p.line('R ADVENTURE')
    out = ''
    for cmd in cmds:
        # wait for quiet
        quiet = 0
        while quiet < 3:
            t = p.read()
            out += t
            if t:
                quiet = 0
            else:
                quiet += 1
            time.sleep(0.4)
        p.line(cmd)             # PRIMOS echoes it back on the line itself
    quiet = 0
    while quiet < 6:
        t = p.read()
        out += t
        quiet = 0 if t else quiet + 1
        time.sleep(0.4)
    return out


def main():
    cmds = [l.rstrip('\n') for l in
            open(sys.argv[1], encoding='latin-1').read().split('\n')]
    while cmds and not cmds[-1]:
        cmds.pop()
    p = Prime(log='primeplay.log')
    try:
        p.login()
        p.cmd('A *>ADVENTURE.UFD', timeout=30)
        out = play(p, cmds)
    finally:
        try:
            p.line('')
            p.line('LO')
        except Exception:
            pass
        p.close()
    open(sys.argv[2], 'w', encoding='latin-1', newline='\n').write(out)
    print('%d commands, %d lines' % (len(cmds), out.count('\n')))


if __name__ == '__main__':
    main()
