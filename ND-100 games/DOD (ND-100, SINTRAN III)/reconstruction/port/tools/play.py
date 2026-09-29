"""A random player for DOD, shared by reffuzz.py (on SINTRAN) and portfuzz.py
(on the port).

Player(seed, commands).answer(tail) gives (keys, why) for whatever the
program is asking at the end of its output, or None when it asks nothing it
knows (or has ended).  The output is 7-bit, CR removed.  A seed always plays
the same game for the same output.

DOD asks every question with INPUT (a ? after it) or LINPUT, and wants one
of F H V (forward, right, left), A (attack), M (magic lightning), FLY (flee),
TA (take), J N (the instructions), A E (the river: all at once, or one by
one).  The player answers with those, now and then something else.  After
COMMANDS answers it stops answering (the game has no command to end it).
"""
import random
import re

WORDS = ['F', 'H', 'V', 'A', 'M', 'FLY', 'TA']


class Player:
    def __init__(self, seed, commands=150):
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0

    def answer(self, tail):
        t = tail.rstrip('\0')
        r = self.rnd
        if self.done >= self.commands:
            return None
        if re.search(r'TRYCK RETURN F\\R ATT F\\RS\\KA D\\DA DEMONEN$', t):
            return '\r', 'the demon'
        t = t.rstrip('\n')                # INPUT waits on, silently, after an empty line
        if not re.search(r'\? *$', t):
            return None
        self.done += 1
        if re.search(r'INSTRUKTIONER\(J/N\) *\? *$', t):
            return r.choice(['J', 'N', 'N', 'JA']) + '\r', 'instructions?'
        if re.search(r'ALLA P\] EN G\]NG\(A/E\) *\? *$', t):
            return r.choice(['A', 'E', 'E', 'X']) + '\r', 'the river'
        x = r.random()
        if x < 0.85:
            return r.choice(WORDS) + '\r', 'command %d' % self.done
        return r.choice(['', 'B', 'FRAM', 'X', 'f', 'fly']) + '\r', 'something else %d' % self.done
