"""A random player for MORDOR, shared by reffuzz.py (on SINTRAN) and
portfuzz.py (on the port).

Player(seed, commands, original).opening() gives the typing up to the first
command: the answer about instructions and the hero screen, as
(keys, why, wait) with wait a regular expression for the output that follows
once the game has taken the keys.  Player.answer(tail) gives (keys, why) for
whatever the game is asking at the end of tail, or None when the game has
ended.

original=True plays the program compiled from the recovered source as it is,
whose LARGE wants names typed shifted (see NOTES.md); otherwise names and
commands come in a random mix of cases, the Swedish E-acute as E, É or é.
"""
import random
import re

HEROES = ['Frodo', 'Sam', 'Merry', 'Pippin', 'Fredegar', 'Legolas', 'Glorfindel', 'Haldir',
          'Rumil', 'Gildor', 'Gandalf', 'Aragorn', 'Boromir', 'Faramir', '@omer', 'Imrahil',
          'Halbarad', 'Theodr`d', 'Beregond', 'Mablung', 'Damrod', 'Gimli', 'Elrond', 'Erestor',
          'Thranduil', 'Dain', 'Brand', 'Celeborn', 'Erkenbrand', 'Gamling', 'Forlong', 'Denethor']

COMMANDS = ['ON', 'OFF', 'FOLLOW-ROAD', 'KILL', 'ASK', '0', '0', 'THROW', 'HELP', 'REPORT', 'TIE',
            'UNTIE', 'USE', 'MAP', 'FOLLOW', 'REP', 'O', 'XYZZY', '', '0', '0']
MOVES = ['N', 'S', 'E', 'W', 'NE', 'NW', 'SE', 'SW']

# what the game is waiting for, at the very end of its output
PROMPT = re.compile(r"(Command: |\(T/I\): |away\? |fight\? |honor\? |Ringbearer: |Palantir: |"
                    r"Reenter: |<N>: |finished reading: )$")


def large(ch):
    """The original program's LARGE: 'a'..'}' less 30, not 32 (see NOTES.md)."""
    return chr(ord(ch) - 30) if 'a' <= ch <= '}' else ch


def typed_name(name, rnd=None):
    """What to type for INNAME to take this hero: shifted for the original
    program (rnd None); otherwise the name in some case, with the Swedish
    E-acute (@, `) typed as É, é, E or e."""
    if rnd is None:
        return ''.join(large(c) for c in name)
    how = rnd.choice(['upper', 'lower', 'as is', 'as is'])
    out = ''
    for c in name:
        if c in '@`':
            c = rnd.choice(['@', '`', 'E', 'e'])
        elif how == 'upper':
            c = c.upper()
        elif how == 'lower':
            c = c.lower()
        out += c
    return out


class Player:
    def __init__(self, seed, commands=300, original=False):
        self.original = original
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0
        self.fellowship = []

    def opening(self):
        rnd = self.rnd
        out = []
        answer = rnd.choice(['N\r', '\r', 'NO\r', 'Y\r']) if rnd.random() < 0.3 else 'N\r'
        out.append((answer, 'instructions', r"Finished\.\x1b\(|finished reading: $"))
        # the hero screen, moved about as the game moves its cursor I (1..33);
        # nothing may be typed once I reaches Finished (Esc would break at the next prompt)
        pos, chosen = 1, set()
        count = rnd.randint(4, 12)
        targets = sorted(rnd.sample(range(1, 33), count))
        for h in targets:
            while pos != h:
                if h >= pos + 4:
                    k, pos = 'B', pos + 4
                elif h > pos:
                    k, pos = 'C', pos + 1
                elif h <= pos - 4:
                    k, pos = 'A', pos - 4
                else:
                    k, pos = 'D', pos - 1
                out.append(('\x1b' + k, 'cursor', r'\x1bY..$'))
            chosen.add(h)
            out.append(('\x1bH', 'take ' + HEROES[h - 1], re.escape(HEROES[h - 1]) + r'\x1b\(\x1bY..$'))
        while pos < 33:
            if pos < 30 and pos + 4 <= 33:
                k, pos = 'B', pos + 4
            else:
                k, pos = 'C', pos + 1
            out.append(('\x1b' + k, 'cursor', r'Ringbearer: $' if pos == 33 else r'\x1bY..$'))
        self.fellowship = [HEROES[h - 1] for h in sorted(chosen)]
        return out

    def answer(self, tail):
        rnd = self.rnd
        if tail.endswith('Reenter: '):
            # after "Illegal Command." or "Ambigiuos command" it wants a command again
            last = tail[:-len('Reenter: ')]
            if re.search(r'(Illegal Command\.|Ambigiuos command)\r\n$', last):
                return self.command()
            return self.name(tail), 'name again'
        if tail.endswith('Command: '):
            return self.command()
        lower = [] if self.original else ['t\r', 'i\r', 'f\r', 'r\r']
        if tail.endswith('(T/I): '):
            return rnd.choice(['T\r', 'I\r', 'I\r', '\r'] + lower[:2]), 'together/individually'
        if tail.endswith('away? '):
            return rnd.choice(['F\r', 'R\r', 'R\r', '\r'] + lower[2:]), 'fight/run'
        if tail.endswith(('fight? ', 'honor? ', 'Ringbearer: ', 'Palantir: ')):
            return self.name(tail), 'name'
        if tail.endswith('finished reading: '):
            return '\r', 'page'
        if tail.endswith('<N>: '):
            return 'N\r', 'yes/no'
        return None

    def command(self):
        rnd = self.rnd
        if self.done >= self.commands:
            return 'QUIT-GAME\r', 'the end'
        self.done += 1
        r = rnd.random()
        if r < 0.6:
            keys = rnd.choice(MOVES) + rnd.choice(['', '', '1', '2', '3', '+', '+', '9']) + '\r'
        else:
            keys = rnd.choice(COMMANDS) + '\r'
        if not self.original and rnd.random() < 0.3:
            keys = keys.lower()
        return keys, 'command %d' % self.done

    def name(self, tail):
        """Someone the game has just listed as in the fellowship, if it did."""
        rnd = self.rnd
        listed = []
        m = re.search(r'consists of:\r\n((?:[^\r]*\r\n)+)[^\r]*$', tail)
        if m:
            listed = [h for h in HEROES if re.search(r'(^|\n)' + re.escape(h) + r'\r', m.group(1))]
        pool = listed or self.fellowship
        if rnd.random() < 0.05:
            return 'X\r'
        return typed_name(rnd.choice(pool), None if self.original else rnd) + '\r'
