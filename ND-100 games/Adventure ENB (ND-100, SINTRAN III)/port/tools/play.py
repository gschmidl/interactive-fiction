"""A random player for ADVENTURE-ENB, shared by reffuzz.py (on SINTRAN) and
portfuzz.py (on the port).

Player(seed, commands).answer(tail) gives (keys, why) for whatever the
program is asking at the end of its output, or None when it asks nothing it
knows (or has ended).  The output is 7-bit, CR removed.  A seed always plays
the same game for the same output.

The commands are the game's keys (ORDER:): the arrow keys (ESC A, B, C, D)
and Home (ESC H, the map), and F T D L K G H V ? N U S.  They are weighted by
where the player is, so that games get somewhere: it takes what lies there,
asks the old women and dragons the way to the cave and heads for it, rests
when it has walked a while, and in the caves looks for the key and the
princess and goes back up with her.  @ (a SINTRAN command) is never typed.
When its commands are used up it ends the game with S, or in the caves
stops resting.
"""
import random
import re

ARROW = {'N': '\x1bA', 'S': '\x1bB', 'E': '\x1bC', 'W': '\x1bD'}
MOVED = re.compile(r'ORDER:(Norrut|S\|derut|[\\|]sterut|V\{sterut)\n(H\{r \{r|Du \{r vid)')
WORD = {'nord': 'N', 'syd': 'S', '|st': 'E', 'v{st': 'W'}
ITEM = re.compile(r'\n(.+?) (?:upplockad|medtaget|uppk\|pt)\n')


class Player:
    def __init__(self, seed, commands=150, cave=None):
        self.rnd = random.Random(seed)
        self.commands = commands
        self.done = 0
        self.have = []                # what it carries, as the game writes it
        self.giving = None
        self.target = cave            # (NS, EW) of the cave, when told (or a dragon told)
        self.heading = []             # directions to the cave, when asked
        self.at = None                # (NS, EW) when it last asked with ?
        self.walked = 0               # moves since it last rested
        self.cave = False
        self.pos = [0, 0]             # in the caves, from where it came down
        self.north_wall = self.west_wall = False
        self.princess = False
        self.key = False
        self.gold, self.strong, self.bought = 100, 20, 0

    # -- what happened since the last ORDER: --------------------------------
    def notice(self, seg):
        for m in ITEM.finditer(seg):
            self.have.append(m.group(1))
        if 'nyckelknippa medtaget' in seg:
            self.key = True
        if '***POOF***' in seg or 'Gumman tog av dig allt' in seg:
            self.have = []
        if self.giving and ('Prinsessan neg' in seg or 'Gumman tog emot' in seg or 'Draken blev' in seg):
            if self.giving in self.have:
                self.have.remove(self.giving)
        self.giving = None
        m = re.search(r'koordinater \{r +(\d+) *, *(\d+)', seg)
        if m and 'Dina' in seg:
            self.at = (int(m.group(1)), int(m.group(2)))
        m = re.search(r'grottans koordinater \{r *(\d+) *, *(\d+)', seg)
        if m:
            self.target = (int(m.group(1)), int(m.group(2)))
        m = re.search(r'grottan ligger \}t (nord|syd)?(\|st|v\{st)?', seg)
        if m:
            self.heading = [WORD[w] for w in m.groups() if w]
        if 'nere i grottorna' in seg:
            self.cave, self.pos = True, [0, 0]
            self.north_wall = self.west_wall = False
        if 'ORDER:Upp' in seg:
            self.cave = False
        if 'Prinsessan f|ljer dig' in seg:
            self.princess = True
        if 'D{r {r en stenv{gg' in seg:
            self.north_wall = True
        if 'h}rd stenv{gg' in seg:
            self.west_wall = True
        m = MOVED.search(seg)
        if m:
            d = m.group(1)[0]
            if self.cave:
                self.pos[0] += {'N': 1, 'S': -1}.get(d, 0)
                self.pos[1] += {'\\': 1, '|': 1, 'V': -1}.get(d, 0)
            else:
                self.walked += 1
                if self.at:                   # (a road's two steps are only seen by the next ?)
                    dn, de = {'N': (1, 0), 'S': (-1, 0), 'V': (0, -1)}.get(d, (0, 1))
                    self.at = (self.at[0] + dn, self.at[1] + de)
        if 'ORDER:Vila' in seg:
            self.walked = max(0, self.walked - 1)

    def place(self, tail):
        m = list(re.finditer(r'(?:Du \{r vid|H\{r \{r) (.*)', tail))
        return m[-1].group(1).strip() if m else ''

    def move(self, towards=None):
        r = self.rnd
        if towards and r.random() < 0.75:
            return ARROW[r.choice(towards)], 'towards %s' % ''.join(towards)
        d = r.choice('NSEW')
        return ARROW[d], 'arrow %s' % d

    # -- the commands ------------------------------------------------------
    def plan(self, seg):
        """from the map (Home) just shown: the way to the nearest thing worth
        going to, away from a monster next to it; None if no map"""
        i = seg.find('ORDER:Karta\n')
        if i < 0:
            return None
        rows = seg[i + 12:].split('\n')[:9]
        if len(rows) < 9 or any(len(row) != 18 for row in rows):
            return None
        cells = [[row[2 * c:2 * c + 2] for c in range(9)] for row in rows]
        want = {'Gr': 1, 'Sv': 2, 'Sk': 2, 'Gs': 3, 'Gu': 4, 'Dr': 4}
        if self.gold >= 30 and self.bought < 3:
            want.update({'St': 1, 'Sl': 1})
        best, danger = None, []
        for r in range(9):
            for c in range(9):
                cell, d = cells[r][c], abs(r - 4) + abs(c - 4)
                if cell in ('Or', 'R|', 'Tr', 'Dv', 'J{') and d <= 3:
                    danger.append((4 - r, c - 4))
                if cell in want and d > 0 and (best is None or (want[cell], d) < best[0]):
                    best = ((want[cell], d), (4 - r, c - 4))
        if danger and self.strong < 80:
            dn, de = danger[0]
            return [d for d, ok in (('S', dn > 0), ('N', dn < 0), ('W', de > 0), ('E', de < 0)) if ok] or None
        if best:
            dn, de = best[1]
            return [d for d, ok in (('N', dn > 0), ('S', dn < 0), ('E', de > 0), ('W', de < 0)) if ok]
        return None

    def outside(self, tail):
        r = self.rnd
        here = self.place(tail)
        seg = tail[tail.rfind('ORDER:', 0, max(0, len(tail) - 6)):]
        m = re.search(r'Du har +(\d+) *gulddubloner', seg)
        if m:
            self.gold = int(m.group(1))
        m = re.search(r'Ditt f\|rsvar: *(-?\d+)[^\n]*\n+Ditt skydd: *(-?\d+)', seg)
        if m:
            self.strong = int(m.group(1)) + int(m.group(2))
        if 'uppk|pt' in seg:
            self.bought += 1
        x = r.random()
        if self.done > self.commands:
            return 'S', 'enough'
        if here == '****GROTTAN****' and x < 0.8:
            return 'N', 'down into the cave'
        if here in ('en gumma', 'en drake') and 'ORDER:Karta' not in seg and not seg.startswith('ORDER:Fr'):
            if x < 0.45:
                return 'F', 'ask the way'
            if x < 0.6 and self.have:
                return 'G', 'give'
            if x < 0.7:
                return 'D', 'kill'
        if here in ('ett sv{rd', 'en sk|ld', 'en s{ck guld', 'en flod') and x < 0.6 \
                and not seg.startswith('ORDER:Ta'):
            return 'T', 'take'
        if here in ('en stad', 'ett slott') and not seg.startswith('ORDER:K') and not seg.startswith('ORDER:L'):
            if x < 0.6 and self.gold >= 30 and self.bought < 4:
                return 'K', 'buy'
            if x < 0.75:
                return 'L', 'hire'
        if self.walked > 6 and x < 0.7:
            return 'V', 'rest'
        way = self.plan(seg)
        if way:
            return self.move(way)
        y = r.random()
        if y < 0.45:
            return '\x1bH', 'map'
        if y < 0.53 or (self.target and y < 0.6):
            return '?', 'look at oneself'
        if y < 0.55:
            return 'H', 'help'
        if y < 0.57:
            return r.choice('ABCEIJMOPQRWXYZ12'), 'no such command'
        if y < 0.61:
            return r.choice('FTDLKGNUV'), 'anything'
        towards = None
        if self.target and self.at:
            towards = [d for d, ok in (('N', self.target[0] > self.at[0]), ('S', self.target[0] < self.at[0]),
                                       ('E', self.target[1] > self.at[1]), ('W', self.target[1] < self.at[1])) if ok]
        elif self.heading:
            towards = self.heading
        return self.move(towards)

    def inside(self, tail):
        r = self.rnd
        here = self.place(tail)
        seg = tail[tail.rfind('ORDER:', 0, max(0, len(tail) - 6)):]
        if self.done > self.commands:               # no more rests: that ends it
            return self.move()
        x = r.random()
        if x < 0.55:
            return 'V', 'rest (15 a move)'
        if 'I ett h|rn st}r' in seg and x < 0.85:
            return 'T', 'take'
        if here == '*PRINSESSAN*' and not self.princess:
            return 'F', 'ask the princess'
        if here == 'en g}ng upp' and (self.princess or x < 0.62):
            return 'U', 'up'
        y = r.random()
        if y < 0.08:
            return 'G', 'give'
        if y < 0.12:
            return r.choice('?DFHKLNSX'), 'no such command here'
        if self.princess:
            back = [d for d, ok in (('N', self.pos[0] < 0), ('S', self.pos[0] > 0),
                                    ('E', self.pos[1] < 0), ('W', self.pos[1] > 0)) if ok]
            return self.move(back)
        if self.key:
            return self.move(['W'] if self.north_wall else ['N'])
        return self.move()

    def answer(self, tail):
        t = tail.rstrip('\0\x7f')           # the program as recovered prompts with DEL
        r = self.rnd
        if re.search(r'Vill du ha instruktioner\?$', t):
            return r.choice(['N', 'N', 'N', 'J', 'NEJ', 'JA']) + '\r', 'instructions?'
        if re.search(r'ORDER:$', t):
            seg = t[t.rfind('ORDER:', 0, max(0, len(t) - 6)):]
            if 'Pl|tsligt st}r' in seg or 'Det kan du inte g|ra nu' in seg:
                x = r.random()
                if x < 0.6:
                    return 'D', 'fight'
                if x < 0.9:
                    return ARROW[r.choice('NSEW')], 'run'
                return r.choice('GFTV'), 'something else'
            self.notice(seg)
            self.done += 1
            k, why = self.inside(t) if self.cave else self.outside(t)
            return k, '%s (command %d)' % (why, self.done)
        if re.search(r"(TRYCK|Tryck) 'RETURN'[^\n]*$", t):
            return '\r', 'return'
        if re.search(r'Skriv vad du vill ge med sm\} bokst\{ver\.\?? *$', t):
            if self.have and r.random() < 0.8:
                self.giving = r.choice(self.have)
                return self.giving + '\r', 'give'
            return r.choice(['', 'en bok', 'ett sv{rd']) + '\r', 'give (not carried)'
        # INPUT waits on, silently, after an empty line: the question is still there
        t = t.rstrip('\n')
        if re.search(r'Vad vill du k\|pa\?? *$', t):
            return r.choice(['MA', 'RU', 'MA', 'RU', 'SV', 'SK', 'SVARD', 'RUSTNING', 'X', '']) + '\r', 'buy what'
        if re.search(r'Hur m\}nga vill du leja \(1-5\):\?? *$', t):
            return r.choice(['1', '1', '2', '3', '5', '0', '7', '']) + '\r', 'hire how many'
        if re.search(r'Vad betalar du till nr +\d+ *\?? *$', t):
            return r.choice(['1', '5', '10', '20', '0']) + '\r', 'pay'
        if re.search(r'Till vem \?\?? *$', t):
            who = 'PR' if self.princess and r.random() < 0.6 else r.choice(['GU', 'DR', 'PR', 'gu', 'pr', 'X', ''])
            return who + '\r', 'to whom'
        return None
