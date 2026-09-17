"""A random player for LEGEND, shared by reffuzz.py (on SINTRAN) and
portfuzz.py (on the port).

Player(seed, commands).answer(tail) gives (keys, why) for whatever the game is
asking at the end of its output, or None when it asks nothing it knows (or
has ended).  The output is 7-bit, with NULs removed.  A seed always plays the
same game.

It keeps away from what would make the reference machine differ from the
port for reasons that are not the program's: TID (the clock), @ (SINTRAN
commands), * and PAUS (both end in @LOGOUT), and the SuperUser key codes.
"""
import random
import re

MOVES = ['N', 'S', '\\', 'V', 'U', 'D', 'NORR', 'S\\DER', '\\STER', 'V[STER', 'UPP', 'NED', 'G] N', 'G] S']
COMMANDS = ['TITTA', 'TIT', 'STATUS', 'STA', 'LISTA', 'L', 'VILKA', 'VEM', '[T', 'T[ND', 'HELA',
            'HJ[LP', '?', 'CLS', 'D\\DA', 'BLIXT', 'BLIXT 2', 'PLOCKA', 'TAPPA', 'TA', 'TA SV[RD',
            'SL[PP', 'SL[PP ALLT', 'BYT', 'V[LJ', 'HUBBA', 'XYZZY', 'UTG]NGAR', 'SE',
            'TELEPORT', 'POSITION', 'GRINA', 'FLY', 'POST', '', 'KLAPPA', 'SLUTA SPELA']
RARE = ['SOVA', 'AVLIVA', 'OMSTART', 'SLUTA']
SHOP = ['?', '1', '2', '3', '4', '0', '0', '1', '2', '7', '']
PROMPTS = [
    (r'Din signatur:$', 'signature'),
    (r'Vilket scenario\? \(1-9, 0\):$', 'scenario'),
    (r"'N'ej\)  <N>$", 'instructions?'),
    (r'forts\{tta$|Tryck n\}gon tangent$', 'any key'),
    (r'Tryck RETURN >\x1bFa\x1bG<$', 'return'),
    (r'V\{lj:$', 'menu'),
    (r'\(1-4\):$', 'class'),
    (r'Klanen\?$', 'order'),
    (r'Vad ska spelaren heta\?$', 'player name'),
    (r'Vad heter du\?$', 'your name'),
    (r'Ta bort spelare nummer:$|Status f\|r spelare nr:$|Spelare nr:$', 'which of mine'),
    (r'(\n|\x1b:)\$$', 'command'),
    (r'\n-$', 'shop'),
    (r'M\)at F\)acklor \?$', 'food or torches'),
    (r'Hur m\}nga \(1-\d+\) \?$|Hur m\}nga\? \(1-\d+\)$', 'how many'),
    (r'Vem\? \(1-\d+\)$', 'revive whom'),
    (r'Anfalla vem\?$|Plocka upp vem\?$', 'whom'),
    (r'Blixt grad \(1-5\)\?$', 'grade'),
    (r'\(J\)$|\(N\)$|\(J\) \?$', 'yes/no'),
    (r'Ditt nya efternamn:$', 'surname'),
    (r'V\{lj spelare:$', 'switch to'),
    (r'position \(1-3\):$', 'position'),
    (r'terminal +\d+ ?\x08:$', 'help page'),
    (r'CTRL-L \+ RETURN$', 'letter'),
]
# what follows a prompt on the screen: Facit codes, a backspace, spaces, NULs
TRAILER = re.compile(r'(?:\x1b[^\x1b]|[\x08\x00\s])+$')


class Player:
    def __init__(self, seed, commands=200):
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0
        self.letter = 0
        self.names = ['GURRA', 'MAGDA', 'KNATTE', 'TJATTE', 'FNATTE', 'OLLE-BOLLE', 'SVEN 2']

    def answer(self, tail):
        rnd = self.rnd
        clean = TRAILER.sub('', tail[-300:].replace(chr(13), ''))
        for rx, what in PROMPTS:
            if re.search(rx, clean):
                return self.respond(what, tail), what
        return None

    def respond(self, what, tail):
        rnd = self.rnd
        if what == 'signature':
            return rnd.choice(['GS', 'LU', 'MB', 'XX', 'ABC', 'ola']) + '\r'
        if what == 'scenario':
            return rnd.choice('123456789') + '\r'
        if what == 'instructions?':
            return 'J' if rnd.random() < 0.2 else rnd.choice('NN\rX')
        if what in ('any key', 'help page'):
            return rnd.choice(' \rA')
        if what == 'return':
            return '\r'
        if what == 'menu':
            self.done += 1                      # a full world can leave nothing else to do
            if self.done >= self.commands:
                return 'D'
            if 'Du har inga spelare' in tail[-400:]:
                return rnd.choice('BBBBBBBEX')
            return rnd.choice('CCCCCCCCCCCCBBEEAX')
        if what == 'class':
            return rnd.choice(['1', '2', '3', '4', '4', '5', 'Q']) + '\r'
        if what == 'order':
            return rnd.choice(['1', '2', '2', '3']) + '\r'
        if what == 'player name':
            return rnd.choice(self.names + ['', 'l{ngt namn', '?!']) + '\r'
        if what == 'your name':
            return rnd.choice(['Kalle Anka', 'Musse Pigg', 'Jo', 'Farbror Joakim']) + '\r'
        if what == 'which of mine':
            return rnd.choice(['1', '2', '1', '3']) + '\r'
        if what == 'command':
            self.done += 1
            if self.done > self.commands:
                return 'SLUTA\r'
            r = rnd.random()
            if r < 0.02:
                keys = rnd.choice(RARE)
            elif r < 0.5:
                keys = rnd.choice(MOVES)
            else:
                keys = rnd.choice(COMMANDS)
            if rnd.random() < 0.3:
                keys = keys.lower()
            return keys + '\r'
        if what == 'shop':
            self.done += 1
            return rnd.choice(SHOP) + '\r'
        if what == 'food or torches':
            return rnd.choice(['M', 'F', '', 'X']) + '\r'
        if what == 'how many':
            return rnd.choice(['1', '2', '5', '', '99']) + '\r'
        if what == 'revive whom':
            return rnd.choice(['1', '', '2']) + '\r'
        if what == 'whom':
            m = re.findall(r'\n {7}([^\n(]+?) \(', tail)
            pool = [x.upper() for x in m] + ['EN', 'MIG', 'NISSE']
            return rnd.choice(pool) + '\r'
        if what == 'grade':
            return rnd.choice(['1', '2', '3', '6']) + '\r'
        if what == 'yes/no':
            return rnd.choice(['J', 'N', '']) + '\r'
        if what == 'surname':
            return rnd.choice(['ANKA', 'PIGG']) + '\r'
        if what == 'switch to':
            return rnd.choice(self.names) + '\r'
        if what == 'position':
            return rnd.choice(['1', '2', '3', '4']) + '\r'
        if what == 'letter':
            self.letter = 1
            return 'Hej fr}n slumpen\r\x0c\r'
        return '\r'
