"""Play a command list against the original on PRIMOS and save the transcript.

The original is the tape's own EXECUTIVE.SEG with the four ADVINIT files,
restored from pulse_library.tap into <SYS23K>GUEST23>PULSE>ADVENTURE4 of a
scratch copy of the p50em pack (restore.py 3: MAGRST, logical tape 3).
Start the emulator there first (em.exe -map ring0.map ring3.map -tport 8720).

The game prompts with "? " or asks a question and waits; this waits for the
output to go quiet before typing the next line.  PRIMOS echoes what is
typed, so the transcript reads like the terminal did.

usage: python primeplay.py commands.txt out.txt
"""
import sys
import time

from primesh import Prime


def quiet(p, rounds):
    out = ''
    n = 0
    while n < rounds:
        t = p.read()
        out += t
        n = 0 if t else n + 1
        time.sleep(0.4)
    return out


def play(p, cmds):
    p.line('SEG EXECUTIVE')
    out = ''
    for cmd in cmds:
        out += quiet(p, 3)
        if out.rstrip().endswith('OK,'):
            break                       # the game has ended
        p.line(cmd)
    out += quiet(p, 6)
    return out


def main():
    cmds = [l.rstrip('\n') for l in
            open(sys.argv[1], encoding='latin-1').read().split('\n')]
    while cmds and not cmds[-1]:
        cmds.pop()
    p = Prime(log='primeplay.log')
    try:
        p.login()
        p.cmd('A *>PULSE>ADVENTURE4', timeout=30)
        p.buf = ''
        out = play(p, cmds)
    finally:
        try:
            p.line('')
        except Exception:
            pass
        p.close()
    open(sys.argv[2], 'w', encoding='latin-1', newline='\n').write(out)
    print('%d commands, %d lines' % (len(cmds), out.count('\n')))


if __name__ == '__main__':
    main()
