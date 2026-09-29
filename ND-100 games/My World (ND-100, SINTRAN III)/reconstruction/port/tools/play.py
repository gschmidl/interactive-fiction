"""A random player for MY_WORLD, shared by reffuzz.py (on SINTRAN) and
portfuzz.py (on the port).

Player(seed, commands).answer(tail) gives (keys, why) for whatever the
program is asking at the end of its output, or None when it asks nothing it
knows (or has ended).  The output is 7-bit, CR removed.  A seed always plays
the same game for the same output.

The commands are drawn from the game's own words (MY-DATA-FILE-MJ:ADV),
weighted towards moving and taking what is in sight, now and then two or
three to a line with commas, in either case.  After COMMANDS commands it
QUITs.  Asked whether to be patched up after a death, it says yes or no in
any of the ways a player would.
"""
import random
import re

VERBS = ['GET', 'TAKE', 'PICK', 'GO', 'WALK', 'DROP', 'LOOK', 'L', 'XYZZY', 'UNLOCK', 'OPEN', 'LOCK',
         'CLOSE', 'ON', 'LIGHT', 'OFF', 'UNLIGHT', 'JUMP', 'MAKE', 'FREE', 'FILL', 'EMPTY', 'THROW']
DIRS = ['NORTH', 'N', 'NE', 'EAST', 'E', 'SE', 'SOUTH', 'S', 'SW', 'WEST', 'W', 'NW', 'UP', 'U', 'DOWN', 'D']
NOUNS = ['ELEVATOR', 'ALL', 'INVENTORY', 'I', 'SCORE', 'GATE', 'DOOR', 'BRIDGE']
THINGS = ['LAMP', 'LIGHT', 'KEYS', 'SET', 'BOTTLE', 'BOLA', 'BALLS', 'IRON', 'WIRE', 'ORC', 'PLANK', 'BIRD',
          'CAGE', 'SNAKE', 'SILVER', 'BARS', 'WATER', 'POOL', 'OIL', 'FOOD', 'NUGGET', 'GOLD', 'MITHRIL',
          'TROLL', 'CHEST', 'TREASURE', 'PILLOWS', 'PILLOW']


class Player:
    def __init__(self, seed, commands=150):
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0

    def one(self, tail):
        r = self.rnd
        x = r.random()
        here = tail[tail.rfind('Command: '):] if 'Command: ' in tail else tail
        if x < 0.35:
            return r.choice(DIRS if r.random() < 0.7 else ['GO ' + r.choice(DIRS)])
        if x < 0.5:
            words = [w.upper() for w in re.findall(r'[A-Za-z]+', here)]
            seen = [w for w in THINGS if w in words]
            if seen:
                return r.choice(['GET', 'TAKE']) + ' ' + r.choice(seen)
        if x < 0.58:
            return r.choice(['I', 'INVENTORY', 'SCORE', 'LOOK', 'GET ALL', 'DROP ALL', 'ON', 'LIGHT LAMP',
                             'OFF', 'OPEN GATE', 'UNLOCK', 'FILL BOTTLE', 'FILL LAMP', 'EMPTY BOTTLE',
                             'FREE BIRD', 'MAKE BRIDGE', 'XYZZY', 'THROW BOLA', 'GET BIRD', 'GET CAGE'])
        if x < 0.85:
            return r.choice(VERBS) + ' ' + r.choice(THINGS + NOUNS)
        if x < 0.93:
            return r.choice(['THE', 'A', '']) + ' ' + r.choice(VERBS) + ' THE ' + r.choice(THINGS)
        return r.choice(VERBS + THINGS + ['PLUGH', 'HELLO', '', '.', ',', 'GET,', 'N,,S'])

    def answer(self, tail):
        t = tail.rstrip('\0')
        r = self.rnd
        if re.search(r'Do you want me to try and patch you\? $', t):
            return r.choice(['Y', 'YES', 'y', 'yes', 'N', 'NO', 'n', 'Yes']) + '\r', 'patch me?'
        if re.search(r'Command: \n*$', t):         # (an empty line is passed over: it asks on)
            self.done += 1
            if self.done > self.commands:
                return 'QUIT\r', 'enough'
            cmd = self.one(t)
            if r.random() < 0.1:
                cmd += ',' + self.one(t)
            if r.random() < 0.2:
                cmd = cmd.lower()
            return cmd + '\r', 'command %d' % self.done
        return None
