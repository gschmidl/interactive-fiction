"""A random player for the adventure interpreter and CAVE-FUN, shared by
reffuzz.py (on SINTRAN) and portfuzz.py (on the port).

Player(seed, commands, name).answer(tail) gives (keys, why) for whatever the
program is asking at the end of its output, or None when it asks nothing it
knows (or has ended).  The output is 7-bit, CR removed.  A seed always plays
the same game.

The commands are drawn from the game's own words, weighted towards moving
and taking what is in sight, so that games get somewhere.  A saved game gets
a new name each time, in quotes (SINTRAN creates a file only when its name
is quoted); LOAD takes back one saved earlier.  The @ commands (SINTRAN) are
never typed.
"""
import random
import re

VERBS = ['GET', 'DROP', 'UNLOCK', 'OPEN', 'LOOK', 'LOCK', 'CLOSE', 'HUGABUGA', 'ON', 'LIGHT', 'OFF',
         'FILL', 'WAVE', 'EMPTY', 'NUMENOR', 'XYZZY', 'OFFICER', 'TIE', 'CLIMB', 'SHAH', 'FRODO',
         'MERRY', 'JUMP', 'HELP', 'MONGOL', 'PLANT', 'READ', 'FEED', 'ATTACK', 'KILL', 'WATER',
         'TAKE', 'THROW', 'L']
NOUNS = ['INVENTORY', 'I', 'SCORE', 'S', 'MATCHES', 'BOX', 'SILVER', 'BAR', 'PLATINUM', 'PYRAMID',
         'MESSAGE', 'MSG', 'LAMP', 'LIGHT', 'WATER', 'ROD', 'BOTTLE', 'HOOK', 'BEANS', 'BEAR',
         'TROLL', 'ORC', 'ALL', 'KEYS', 'KEY', 'SWORD', 'ROPE', 'FOOD', 'XYZZY', 'CART', 'CHAIN']
DIRS = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW', 'U', 'D', 'NORTH', 'SOUTH', 'EAST', 'WEST',
        'UP', 'DOWN', 'GO NORTH', 'GO SOUTH', 'GO EAST', 'GO WEST', 'GO UP', 'GO DOWN', 'GO N', 'GO S']
EXIT_WORDS = {'NORTH': 'N', 'EAST': 'E', 'SOUTH': 'S', 'WEST': 'W', 'NE': 'NE', 'SE': 'SE',
              'SW': 'SW', 'NW': 'NW', 'UP': 'U', 'DOWN': 'D'}


class Player:
    def __init__(self, seed, commands=150, name='CAVE-FUN'):
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0
        self.name = name
        self.saved = []
        self.ended = False

    def command(self, tail):
        r = self.rnd
        # what the last description showed
        items = re.findall(r'VISIBLE ITEMS HERE: \n(.*)\n', tail)
        exits = re.findall(r'OBVIOUS EXITS ARE: (.*)', tail)
        x = r.random()
        if exits and x < 0.45:
            ws = [EXIT_WORDS[w] for w in exits[-1].split() if w in EXIT_WORDS]
            if ws:
                return r.choice(ws)
        if items and x < 0.6:
            words = [w for w in re.split(r'[ ,]+', items[-1]) if len(w) > 2]
            if words:
                return 'GET ' + r.choice(words)
        if x < 0.68:
            return r.choice(DIRS)
        if x < 0.72:
            return r.choice(['GET I', 'GET S', 'GET INVENTORY', 'LOOK', 'DROP ALL', 'GET ALL'])
        if x < 0.735:
            if self.saved and r.random() < 0.5:
                return 'LOAD ' + r.choice(self.saved)
            name = 'SAV%d' % (len(self.saved) + 1)
            self.saved.append(name)
            return 'SAVE "%s"' % name
        if x < 0.9:
            return r.choice(VERBS) + ' ' + r.choice(NOUNS)
        return r.choice(VERBS + DIRS)

    def answer(self, tail):
        t = tail.rstrip('\0')
        if re.search(r'PLAY: $', t):
            return self.name + '\r', 'adventure'
        if re.search(r'\n > $', t) or t.endswith(' > ') and t.count('\n') == 0:
            if self.done >= self.commands:
                return 'END\r', 'enough'
            self.done += 1
            c = self.command(t)
            return c + '\r', 'command %d' % self.done
        if re.search(r'SAVE GAME: $', t):
            return self.rnd.choice(['N', 'NO', 'Y']) + '\r', 'save at the end?'
        if re.search(r'SAVE FILE NAME: $', t):
            name = 'SAV%d' % (len(self.saved) + 1)
            self.saved.append(name)
            return '"%s"\r' % name, 'save file'
        if re.search(r'LOAD FILE NAME: $', t):
            if self.saved:
                return self.saved[0] + '\r', 'load file'
            return 'CAVE-FUN\r', 'load file (nothing saved: the adventure itself)'
        return None


class ScriptPlayer:
    """Types the commands of a script (one a line; # lines and debug pokes
    left out) one at each prompt, then ENDs the game without saving."""

    def __init__(self, lines, name='CAVE-FUN'):
        self.lines = [l for l in lines if l.strip() and not l.startswith('#')]
        if self.lines and self.lines[0] in ('CAVE-FUN', 'CAVE-ORIG'):
            self.lines.pop(0)
        self.name = name
        self.done = 0

    def answer(self, tail):
        t = tail.rstrip('\0')
        if re.search(r'PLAY: $', t):
            return self.name + '\r', 'adventure'
        if re.search(r'\n > $', t):
            if self.done >= len(self.lines):
                return 'END\r', 'end of the script'
            self.done += 1
            return self.lines[self.done - 1] + '\r', 'script %d' % self.done
        if re.search(r'SAVE GAME: $', t):
            return 'N\r', 'save at the end?'
        if re.search(r'(SAVE|LOAD) FILE NAME: $', t):
            return '"SCRIPT"\r', 'file'
        return None
