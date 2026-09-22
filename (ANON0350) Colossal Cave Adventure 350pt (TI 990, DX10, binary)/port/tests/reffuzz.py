#!/usr/bin/env python3
"""Random sessions on the reference system, kept as cross-check sessions.

    python reffuzz.py N [SEED]

Needs the reference system running and set up: sim990 3.3.0 with DX10 3.7,
the console on telnet 2099 at SCI's prompt, GAMES assigned to .GAMES.GAMES and
the GAMES procedure library in use (see reference/README.md and
..\\..\\..\\..\\_TI990_work\\README.md).

Each session walks into the cave (so that dwarves and other random events
come), types 40 random commands, and quits.  It is recorded as
reference/fuzzK.in and fuzzK.raw: the answers are typed one prompt at a time,
and typing stops as soon as the game ends, so that no stray line reaches SCI.
Then crossref.py's clock search finds the start second that replays it, and
the session joins crossref.py's set (reference/clocks.txt).  Any session the
port does not replay is an error in the port: the reference runs the same
program.
"""
import datetime
import os
import random
import re
import socket
import sys
import time

import crossref
from common import REF

WALK = ['NO', 'NO', 'IN', 'GET LAMP', 'GET KEYS', 'OUT', 'S', 'S', 'S', 'UNLOCK GRATE', 'D', 'W',
        'ON', 'W', 'GET CAGE', 'W', 'W']
WORDS = ('N S E W NE NW SE SW U D IN OUT BACK LOOK INVENTORY SCORE GET TAKE DROP THROW OPEN '
         'CLOSE ON OFF LAMP KEYS FOOD BOTTLE WATER CAGE BIRD ROD WAVE XYZZY PLUGH FEED KILL '
         'ATTACK DWARF SNAKE AXE GOLD NUGGET DIAMONDS SILVER COINS JEWELRY FILL POUR DRINK EAT '
         'READ FIND WHERE HELP BRIEF HOURS PLANT CLIMB JUMP CROSS').split()
# Saturday 2026-09-19 - which the program itself counts as a weekday: its hours
# ("Sat - Sun: Open all day") do not apply, and it is closed from 8:00 to 12:00
# and from 13:00 to 17:00.  The sessions start in the evening hours, 17-24.
DAY = (2026, 9, 19)


class Console:
    def __init__(self, port=2099):
        self.s = socket.create_connection(('localhost', port))
        self.s.settimeout(0.2)
        self.raw = b''
        self.last = time.time()

    def pump(self, secs):
        end = time.time() + secs
        while time.time() < end:
            try:
                d = self.s.recv(65536)
            except socket.timeout:
                continue
            if not d:
                raise EOFError('console closed')
            self.raw += re.sub(rb'\xff[\xfb-\xfe].', b'', d)
            self.last = time.time()

    def quiet(self, secs=1.0, limit=120):
        end = time.time() + limit
        while time.time() < end:
            self.pump(0.2)
            if time.time() - self.last >= secs:
                return
        raise TimeoutError('the console did not settle')

    def type(self, line):
        time.sleep(0.6)                         # a key typed as a prompt appears is lost
        for ch in line:
            self.s.sendall(ch.encode('latin-1'))
            time.sleep(0.08)
        self.s.sendall(b'\r')


def session_lines(rng):
    lines = list(WALK)
    for _ in range(40):
        k = rng.random()
        if k < 0.6:
            lines.append(rng.choice(WORDS[:11]))                    # a move
        elif k < 0.9:
            lines.append(' '.join(rng.choice(WORDS) for _ in range(rng.choice((1, 2)))))
        else:
            lines.append(rng.choice(['YES', 'NO', 'Y', 'N']))
    return lines


def record(name, lines, hour, minute):
    c = Console()
    c.quiet(1.0)
    c.type('IDT')
    for v in (DAY[0], DAY[1], DAY[2], hour, minute):
        c.quiet(0.8)
        c.type(str(v))
    c.quiet(1.0)
    mark = len(c.raw)
    c.type('ADVENTUR')
    typed = []

    def over():
        return b'NORMAL PROGRAM COMPLETION' in c.raw[mark:]
    for line in lines + ['QUIT', 'YES'] * 3:
        c.quiet(1.0)
        if over():
            break
        c.type(line)
        typed.append(line)
    c.quiet(3.0, limit=300)
    if not over():
        raise RuntimeError('%s: the game did not end' % name)
    open(os.path.join(REF, name + '.in'), 'w').write(''.join(ln + '\n' for ln in typed))
    open(os.path.join(REF, name + '.raw'), 'wb').write(c.raw[mark:])
    c.s.close()
    return typed


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    rng = random.Random(seed)
    have = crossref.clocks()
    k = 1
    bad = 0
    crossref.SCRATCH = __import__('tempfile').mkdtemp(prefix='adv990f-')
    for _ in range(n):
        while 'fuzz%d' % k in have:
            k += 1
        name = 'fuzz%d' % k
        hour, minute = 17 + k % 7, 10 + (7 * k) % 40       # open hours, never minute 0
        lines = session_lines(rng)
        typed = record(name, lines, hour, minute)
        start = datetime.datetime(*DAY, hour, minute) - datetime.timedelta(minutes=1)
        found = None
        for s in range(6 * 60):
            clock = (start + datetime.timedelta(seconds=s)).strftime('%Y-%m-%d %H:%M:%S')
            if crossref.compare(name, clock, show=False):
                found = clock
                break
        if found:
            with open(os.path.join(REF, 'clocks.txt'), 'a') as f:
                f.write('%s %s\n' % (name, found))
            print('%s: %d lines, replayed by the port at %s' % (name, len(typed), found))
        else:
            bad += 1
            print('%s: %d lines - NO clock replays it; a difference:' % (name, len(typed)))
            crossref.compare(name, start.strftime('%Y-%m-%d %H:%M:%S'))
        have[name] = found
        k += 1
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
