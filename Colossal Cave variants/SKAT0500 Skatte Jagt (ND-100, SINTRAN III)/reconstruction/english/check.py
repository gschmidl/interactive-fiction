"""Check the English image against the sessions recorded on the real SINTRAN.

    python check.py [-v]

Each recorded session (port/tests/ref/NAME.in, NAME.ref) was typed on the
original.  Its commands are put into English word for word, by the meaning each
word has in the original's table, and typed into SKATTEJAKT-EN.PROG.  Then the
recorded Norwegian transcript is translated message by message from
messages.no.txt and messages.en.txt and the two must agree.  The game's random
numbers were dumped with it, so the same meanings must give the same game: the
same rooms, dwarves and knives.  Any text pointer, word or format gone wrong
shows up as a difference.
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.join(HERE, '..', 'port')
EXE = os.path.join(PORT, 'skattejakt.exe')
ORIG = os.path.join(PORT, 'data', 'SKATTEJAKT.PROG')
ENGL = os.path.join(PORT, 'data', 'SKATTEJAKT-EN.PROG')
REF = os.path.join(PORT, 'tests', 'ref')
KEY = (0o020040, 0o045440, 0o020040)
KTAB, ATAB = 0o64114, 0o64732


def vocabulary(path):
    w = struct.unpack('>65536H', open(path, 'rb').read()[0x200:])
    words = []
    for n in range(1, 400):
        k = w[KTAB + n]
        if k == 0xffff:
            break
        s = ''
        for j in range(3):
            v = w[ATAB + 3 * n + j] ^ KEY[j]
            s += chr(v >> 8) + chr(v & 255)
        words.append((s.rstrip(), k))
    return words


def first_meaning(words):
    table = {}
    for word, k in words:
        table.setdefault(word, k)
    return table


def read_messages(path):
    msgs, cur = {}, None
    with open(path, encoding='ascii') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if re.fullmatch(r'#\d+', line):
                cur = int(line[1:])
                msgs[cur] = []
            else:
                msgs[cur].append(line.rstrip())
    return msgs


NO = first_meaning(vocabulary(ORIG))
EN = first_meaning(vocabulary(ENGL))
BY_MEANING = {}
for word, k in EN.items():
    BY_MEANING.setdefault(k, word)
BY_MEANING[45] = 'NORTH'        # NORD must not become the N of yes or no
LITERAL = {'JA': 'YES', 'J': 'Y', 'NEI': 'NO', 'VEST': 'WEST'}


def english_word(word):
    """The English for one typed word; unknown words stand as they are."""
    w6 = word.upper()[:6]
    if w6 in LITERAL:
        return LITERAL[w6]
    if w6 in NO:
        k = NO[w6]
        if EN.get(w6) == k:     # FEE FIE FOE FOO stay themselves
            return w6
        if k in BY_MEANING:
            return BY_MEANING[k]
    return 'Q' + word if word.strip() and not word.isdigit() else word


# Everything in a transcript that is not a whole message: the banner, the
# FORMATs, with the typed word shown as a group.
BANNER = [
    ('* * N Y H E T * *', '* * N E W S * *'), ('*   E V E N T Y R   *', '* A D V E N T U R E *'),
    ('N ]  P ]  N O R S K !!!', 'N O W  I N  E N G L I S H !!!'),
    ('I hovedrollene: DVERGER,  DRAGER,  TROLL, til og med en PRINSESSE.',
     'Starring: DWARVES,  DRAGONS,  TROLLS, and even a PRINCESS.'),
    ('Hvis du liker } l|se g}ter og problemer, vil du elske dette spillet, men',
     'If you like solving riddles and puzzles, you will love this game, but'),
    ('tenk litt p} at du krever en hel del av denne stakkars overarbeidede',
     'do spare a thought for how much you demand of this poor overworked'),
    ('datamaskinen,  og dermed sinker den en hel del.', 'computer,  and how much you slow it down.'),
    ('LYKKE TIL !!!     (Det trenger du!)', "GOOD LUCK !!!     (You'll need it!)"),
]
FORMS = [
    (r'Det er ?(\d+) fryktingytende sm} dverger her i rommet\.', r'There are \1 threatening little dwarves in the room with you.'),
    (r'(\d+) av dem kaster kniver etter deg\.', r'\1 of them throw knives at you.'),
    (r'(\d+) av dem treffer deg!', r'\1 of them hit you!'),
    (r'Hvis du ga opp n}, ville du oppn} *(\d+) poeng av *(\d+) oppn}elige\.', r'If you gave up now, you would score \1 points of a possible \2.'),
    (r'Du oppn}dde *(\d+) poeng av *(\d+) oppn}elige, i l\|pet av *(\d+) trekk\.', r'You scored \1 points out of a possible \2, in \3 moves.'),
    (r'For } f} en h\|yere grad, trenger du *(\d+) poeng ekstra\.', r'To reach a higher rank you need \1 more points.'),
    (r'For } oppn} en h\|yere grad, m} du v\{re litt av en tryllekunstner!', 'To reach a higher rank you would have to be quite a magician!'),
    (r'Gratulerer!!', 'Congratulations!!'),
    (r'Beskjeden sier: Det magiske ordet er : (\S+)', r'The message says: The magic word is : \1'),
    (r'MAN - FRE: ]pen hele dagen', 'MON - FRI: Open all day'),
    (r'L.R - S.N: ]pen hele dagen', 'SAT - SUN: Open all day'),
    (r'HELLIDAG : ]pen hele dagen', 'HOLIDAY : Open all day'),
    (r'Du sprengte alle grenser!!', 'You broke all records!!'),
    (r'Vennligst besvar sp\|rsm}let\.', 'Please answer the question.'),
    (r'Jeg kan gi deg et hint, men det vil koste deg *(\d+) poeng\.', r'I can give you a hint, but it will cost you \1 points.'),
    (r'Jeg kan bevare hulen for deg, slik at du kan fortsette senere, men', 'I can preserve the cave for you, so that you can continue later, but'),
    (r'du er n\|dt til } vente minst *(\d+) minutter f\|r du kan fortsette\.', r'you will have to wait at least \1 minutes before continuing.'),
]
ECHOED = [
    (r'(\S+) hva\?', '%s what?'),
    (r'jeg kan ikke se noen (\S+) her\.', 'I see no %s here.'),
    (r'Hva er det du \|nsker } gj\|re med (\S+) ?\?', 'What do you want to do with %s?'),
    (r'OK, "(\S+) hva\? "\.', 'OK, "%s what?".'),
    (r'OK, "(\S+)"\.', 'OK, "%s".'),
]


def translate_line(line):
    """A line that is no whole message: a banner, a FORMAT, or typed words."""
    for a, b in BANNER:
        if line == norm(a):
            return norm(b)
    for a, b in FORMS:
        if re.fullmatch(a, line):
            return norm(re.sub(a, b, line))
    for a, b in ECHOED:
        m = re.fullmatch(a, line)
        if m:
            return b % english_word(m.group(1))
    return ' '.join(english_word(x) for x in line.split(' '))


def norm(line):
    return re.sub(r' +', ' ', line.strip())


def translate_transcript(lines, no, en):
    """The recorded lines in English; message by message, longest match first."""
    starts = {}
    for idx, text in no.items():
        if text and not text[0].startswith('>$<'):
            starts.setdefault(norm(text[0]), []).append(idx)
    out, i = [], 0
    while i < len(lines):
        line = norm(lines[i])
        best = None
        for idx in starts.get(line, []):
            text = no[idx]
            if all(i + j < len(lines) and norm(lines[i + j]) == norm(t) for j, t in enumerate(text)):
                if best is None or len(text) > len(no[best]):
                    best = idx
        if best is not None:
            out.append(('msg', [norm(t) for t in en[best]]))
            i += len(no[best])
            continue
        out.append(('line', line))
        i += 1
    return out


def lines_of(data):
    text = data.decode('latin-1').replace('\r', '')
    return [x for x in (l.replace('\x19', '').replace('\x04', '').replace('\x07', '') for l in text.split('\n'))
            if x.strip()]


def compare(name, verbose):
    in_no = open(os.path.join(REF, name + '.in'), 'rb').read().decode('latin-1')
    ref = lines_of(open(os.path.join(REF, name + '.ref'), 'rb').read())
    typed = []
    for line in re.split(r'[\r\n]+', in_no):
        typed.append(' '.join(english_word(x) for x in line.split(' ')))
    in_en = '\r'.join(typed).encode('latin-1')
    p = subprocess.run([EXE, '--raw', '-Z', '1789632000', '--prog', ENGL], input=in_en, stdout=subprocess.PIPE,
                       stderr=subprocess.DEVNULL, timeout=120)
    got = lines_of(p.stdout)
    no, en = MSG_NO, MSG_EN
    expect = translate_transcript(ref, no, en)
    a, bad, unknown = 0, 0, []
    known_en = {norm(t) for text in en.values() for t in text}
    for kind, item in expect:
        if kind == 'msg':
            for t in item:
                if a < len(got) and norm(got[a]) == t:
                    a += 1
                else:
                    # the same words may be the same message; report the first
                    bad += 1
                    if verbose or bad <= 3:
                        print('  %s: expected %r, got %r' % (name, t, got[a] if a < len(got) else None))
                    if a < len(got):
                        a += 1
        else:
            # a line that is not a message: an echoed command, banner or format
            t = translate_line(item)
            have = norm(got[a]) if a < len(got) else None
            if have != t and have is not None and have.upper() != t.upper():
                unknown.append((item, have))
                bad += 1
            a += 1
    return bad, len(expect), unknown, len(got) - a


MSG_NO = read_messages(os.path.join(HERE, 'messages.no.txt'))
MSG_EN = read_messages(os.path.join(HERE, 'messages.en.txt'))


def check_tables():
    """Every text pointer in every table must lead to the right English."""
    old = struct.unpack('>65536H', open(ORIG, 'rb').read()[0x200:])
    new = struct.unpack('>65536H', open(ENGL, 'rb').read()[0x200:])
    bad = 0
    n = 0
    for a, b in ((0o61257, 0o61567), (0o62457, 0o62470), (0o62655, 0o63145), (0o67216, 0o67443), (0o71145, 0o71531)):
        for x in range(a, b + 1):
            if old[x] in MSG_EN:
                n += 1
                if read_at(new, new[x]) != [norm(t) for t in MSG_EN[old[x]]]:
                    bad += 1
                    print('  table word %o: message %d is wrong' % (x, old[x]))
    print('%d text pointers checked, %d wrong' % (n, bad))
    return bad


def read_at(w, i):
    lines = []
    base = 0o71534
    while True:
        e, m = w[base + 3 * i], (w[base + 3 * i + 1] << 16) | w[base + 3 * i + 2]
        x = (e & 0x7fff) - 0o40000
        nx = m >> (32 - x)
        s = ''
        for c in range(i + 1, nx):
            for j in range(3):
                v = w[base + 3 * c + j] ^ KEY[j]
                s += chr(v >> 8) + chr(v & 255)
        lines.append(norm(s))
        e2 = w[base + 3 * nx]
        if e2 & 0x8000:
            return lines
        i = nx


def main():
    verbose = '-v' in sys.argv
    total_tables = check_tables()
    # the keys_ sessions are about the terminal, which translation does not touch
    names = sorted(f[:-3] for f in os.listdir(REF) if f.endswith('.in') and not f.startswith('keys_'))
    total = total_tables
    seen = {}
    for name in names:
        bad, n, unknown, extra = compare(name, verbose)
        total += bad + (1 if extra else 0)
        for item in unknown:
            seen[item] = seen.get(item, 0) + 1
        print('%-14s %5d messages and lines, %d different%s' % (name, n, bad, ', %d extra' % extra if extra else ''))
    print('\nLines that differ, recorded => English:')
    for (a, b), c in sorted(seen.items(), key=lambda x: -x[1]):
        print('%5d  %s\n    => %s' % (c, a, b))
    print('\n%s' % ('ALL THE SAME' if total == 0 else '%d DIFFERENCES' % total))
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main())
