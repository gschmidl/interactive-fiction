"""Make the English Skattejakt: SKATTEJAKT.PROG in, SKATTEJAKT-EN.PROG out.

    python build.py            build ../port/data/SKATTEJAKT-EN.PROG
    python build.py --extract  also rewrite messages.no.txt from the original

Only data changes; no instruction is touched.  What is replaced (octal
addresses in the image, 3 words to an A6 element):

  LINES      071537-   the text: records of a REAL pointer to the
                       next line (negative on a message's first line), then
                       the text XORed with 020040 045440 020040
  STEXT, LTEXT, CTEXT, MTEXT+PTEXT, RTEXT   the tables of pointers into LINES
  KTAB/ATAB  064114/064732   the vocabulary
  banner     036763-   the message of the day, built like LINES
  formats and A6 literals in the code, in place at their own length
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src_original', 'SKATTEJAKT.PROG')
OUT = os.path.join(HERE, '..', 'port', 'data', 'SKATTEJAKT-EN.PROG')

KEY = (0o020040, 0o045440, 0o020040)
LINES = 0o71534             # LINES(I) is at LINES + 3*I
LINES_END = 0o173765        # first word past the array
BLANK = (0o020040, 0o020040, 0o020040)
FIRST_OBJECT = 5103         # message number of the first object's text
TABLES = [(0o61257, 0o61567), (0o62457, 0o62470), (0o62655, 0o63145),
          (0o67216, 0o67443), (0o71145, 0o71531)]
KTAB, ATAB, VOCMAX = 0o64114, 0o64732, 400
BANNER = 0o36763
WIDTH = 72


def real(v):
    """An index as the 48-bit REAL the program stores it in."""
    n = abs(v)
    e = n.bit_length()
    m = n << (32 - e)
    return (0o40000 + e) | (0x8000 if v < 0 else 0), m >> 16, m & 0xffff


def unreal(w, a):
    e, m = w[a], (w[a + 1] << 16) | w[a + 2]
    x = (e & 0x7fff) - 0o40000
    if m == 0 or not 0 < x <= 16 or not m & 0x80000000 or m & ((1 << (32 - x)) - 1):
        return None
    return -(m >> (32 - x)) if e & 0x8000 else m >> (32 - x)


def encode(text, cols):
    """Text as cols elements, blank padded and XORed."""
    text = text.ljust(cols * 6)
    out = []
    for c in range(cols):
        s = text[c * 6:c * 6 + 6]
        for j in range(3):
            out.append(((ord(s[j * 2]) << 8) | ord(s[j * 2 + 1])) ^ KEY[j])
    return out


def decode(w, a, cols):
    s = ''
    for c in range(cols):
        for j in range(3):
            v = w[a + 3 * c + j] ^ KEY[j]
            s += chr(v >> 8) + chr(v & 255)
    return s


def read_messages(w):
    msgs, i = [], 1
    while True:
        v = unreal(w, LINES + 3 * i)
        n = abs(v)
        if n <= i:
            return msgs, i
        text = decode(w, LINES + 3 * (i + 1), n - i - 1).rstrip()
        if v < 0:
            msgs.append((i, []))
        msgs[-1][1].append(text)
        i = n


def read_table(path):
    msgs, cur = [], None
    with open(path, encoding='ascii') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if re.fullmatch(r'#\d+', line):
                cur = (int(line[1:]), [])
                msgs.append(cur)
            else:
                cur[1].append(line)
    return msgs


# ---------------------------------------------------------------- literals

def lit(s):
    """A 6-character A6 element, as stored in the code."""
    s = s.ljust(6)
    return [(ord(s[i]) << 8) | ord(s[i + 1]) for i in (0, 2, 4)]


# Words the parser and the yes/no routine compare with (see README).  The
# Norwegian "LAS OPP" turned the OPP into APNE; English has UNLOCK, so that
# rule is made to do nothing: OPEN into OPEN.
WORDS = {
    0o22230: 'MAGIC', 0o22233: 'MODE', 0o22344: 'MAGIC', 0o22352: 'MODE',
    0o22237: 'OPEN', 0o22242: 'OPEN', 0o22245: 'FEED', 0o22250: 'POUR', 0o22253: 'WEST',
    0o22264: ' here.', 0o22273: 'HIGHER', 0o22276: 'LOWER', 0o22301: 'CLOSER',
    0o22312: ' what?', 0o22320: 'Y', 0o22323: 'YES',
    0o31075: 'YES', 0o31101: 'Y', 0o31104: 'NO', 0o31107: 'N', 0o31113: 'Y',
    0o41317: 'BOW',
    0o35126: 'days, ', 0o35134: ' day, ',
}
# Text longer than an element, at its own address.
TEXTS = {
    0o35100: 'MON - FRI:  SAT - SUN:  HOLIDAY  :  ',
    0o35661: '   weekdays: weekends:  hols: ',
    0o37651: 'The First Treasure Hunt ',
}
# FORMATs: found by their old text, the new one blank-padded to the same length.
FORMATS = [
    ("(' Det er',I2,' fryktingytende sm} dverger her i '                  ,' rommet.')",
     "(' There are',I2,' threatening little dwarves in the room with you.')"),
    ("(1X,I1,' av dem kaster kniver etter deg.')", "(1X,I1,' of them throw knives at you.')"),
    ("(1X,I1,' av dem treffer deg!')", "(1X,I1,' of them hit you!')"),
    ("(' Hva er det du |nsker } gj|re med ',20A1)", "(' What do you want to do with ',20A1)"),
    ("(' jeg kan ikke se noen ',20A1)", "(' I see no ',20A1)"),
    ("(' Hvis du ga opp n}, ville du oppn}',I4,                            ' poeng av',I4,' oppn}elige.')",
     "(' If you gave up now, you would score',I4,' points of a possible',I4,'.')"),
    ("(' Beskjeden sier: Det magiske ordet er : ',A6)", "(' The message says: The magic word is : ',A6)"),
    ("(' Jeg kan bevare hulen for deg, slik at du kan fortsette',          ' senere, men',/,' du er n|dt til } vente minst',              I3,' minutter f|r du kan fortsette.')",
     "(' I can preserve the cave for you, so that you can continue',' later, but',/,' you will have to wait at least',I3,' minutes before continuing.')"),
    ("(' Jeg kan gi deg et hint, men det vil koste deg',                I2,' poeng.')",
     "(' I can give you a hint, but it will cost you',I2,' points.')"),
    ("(/,' Du oppn}dde',I4,' poeng av ',I4,                                  ' oppn}elige,  i l|pet av',I5,' trekk.')",
     "(/,' You scored',I4,' points out of a possible',I4,', in',I5,' moves.')"),
    ("(' Du sprengte alle grenser!!')", "(' You broke all records!!')"),
    ("(' For } f} en h|yere grad, trenger du',I3,                          ' poeng ekstra.')",
     "(' To reach a higher rank you need',I3,' more points.')"),
    ("(' For } oppn} en h|yere grad,  m} du ',                             'v{re litt av en tryllekunstner!',/,' Gratulerer!!')",
     "(' To reach a higher rank you would have to be',' quite a magician!',/,' Congratulations!!')"),
    ("' Brukt lagerplass:'", "' Storage used:'"),
    ("' ord med tekst'", "' words of text'"),
    ("' reiseruter'", "' routes'"),
    ("' ord i vokabularet'", "' vocabulary words'"),
    ("' steder'", "' places'"),
    ("' gjenstander'", "' objects'"),
    ("' handlingsverb'", "' action verbs'"),
    ("' RTEXT beskjeder'", "' RTEXT messages'"),
    ("' klassifiseringer'", "' classifications'"),
    ("' magiske beskjeder'", "' magic messages'"),
    ("(' Vennligst besvar sp|rsm}let.')", "(' Please answer the question.')"),
    ("(' Denne Skattejakten ble utsatt for kun',I3,                      ' minutter siden.')",
     "(' This treasure hunt was suspended only',I3,' minutes ago.')"),
    ("(' Varigheten av demonstrasjonsspillet',                           ' (<CR> for } slutte etter',I3,'): ',/)",
     "(' Length of the demonstration game',' (<CR> to end after',I3,'): ',/)"),
    ("(' Ventetid for fortsettelse',                                     ' (<CR> for } vente',I3,' min.): ',/)",
     "(' Wait before continuing',' (<CR> to wait',I3,' min.): ',/)"),
    ("(' I dag er en hellidag , nemlig ',4A6)", "(' Today is a holiday, namely ',4A6)"),
    ("(' Neste hellidag vil bli om ',I3,1X,A6,' nemlig ',4A6)", "(' Next holiday will be in ',I3,1X,A6,' namely ',4A6)"),
    ("(5X,2A6,']pen hele dagen')", "(5X,2A6,'Open all day')"),
    ("(5X,2A6,'   Stengt ')", "(5X,2A6,'   Closed')"),
    ("(' Lukningstider  ',2A6)", "(' Closing times  ',2A6)"),
    ("(/,'$Fra : ')", "(/,'$From: ')"),
    ("(/,'$Til : ')", "(/,'$To  : ')"),
]
BANNER_LINES = [
    '\x19                            * * N E W S * *',
    '                          * A D V E N T U R E *',
    '                  N O W  I N  E N G L I S H !!!',
    '  Starring: DWARVES,  DRAGONS,  TROLLS, and even a PRINCESS.',
    'If you like solving riddles and puzzles, you will love this game, but',
    'do spare a thought for how much you demand of this poor overworked',
    'computer,  and how much you slow it down.',
    '                 GOOD LUCK !!!     (You\'ll need it!)',
]


def put_bytes(mem, offset, s):
    mem[offset:offset + len(s)] = s


def build(mem):
    w = list(struct.unpack('>65536H', bytes(mem)))
    old, end = read_messages(w)

    # the text
    new = read_table(os.path.join(HERE, 'messages.en.txt'))
    if [m[0] for m in old] != [m[0] for m in new]:
        raise SystemExit('messages.en.txt does not match the original message list')
    where, seen, i = {}, {}, 1
    for idx, lines in new:
        # English is a little longer than the array allows, so rooms with the
        # same words (the mazes) share one copy.  Only rooms: object messages
        # are found by walking from one to the next.
        if idx < FIRST_OBJECT and tuple(lines) in seen:
            where[idx] = seen[tuple(lines)]
            continue
        where[idx] = i
        if idx < FIRST_OBJECT:
            seen[tuple(lines)] = i
        for n, text in enumerate(lines):
            if len(text) > WIDTH and not text.startswith('>$<') and not text.startswith('*'):
                raise SystemExit('#%d: line of %d characters: %s' % (idx, len(text), text))
            if not text.isascii() or re.search(r'[\[\]\\{|}]', text):
                raise SystemExit('#%d: a national character: %s' % (idx, text))
            cols = max(1, (len(text) + 5) // 6)
            nx = i + 1 + cols
            first = n == 0
            w[LINES + 3 * i:LINES + 3 * i + 3] = real(-nx if first else nx)
            w[LINES + 3 * (i + 1):LINES + 3 * nx] = encode(text, cols)
            i = nx
    if LINES + 3 * i + 3 > LINES_END:
        raise SystemExit('text too long: %d elements' % i)
    w[LINES + 3 * i:LINES + 3 * i + 3] = real(-1)      # the end mark
    used = i
    tail = LINES + 3 * (i + 1)
    for a in range(tail, LINES + 3 * (end + 1), 3):
        w[a:a + 3] = BLANK

    # the pointers to it
    for a, b in TABLES:
        for x in range(a, b + 1):
            if w[x] in where:
                w[x] = where[w[x]]

    # the vocabulary
    entries = []
    with open(os.path.join(HERE, 'vocab.en.txt'), encoding='ascii') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            head, words = line.split(':', 1)
            t, m = map(int, head.split())
            for word in words.split():
                entries.append((word, t * 1000 + m))
    if len(entries) >= VOCMAX:
        raise SystemExit('too many words: %d' % len(entries))
    for n, (word, k) in enumerate(entries, 1):
        if len(word) > 6:
            raise SystemExit('word too long: ' + word)
        w[KTAB + n] = k
        w[ATAB + 3 * n:ATAB + 3 * n + 3] = encode(word, 1)
    w[KTAB + len(entries) + 1] = 0xffff

    # the banner, whose records keep their sizes
    i, line = 1, 0
    while True:
        n = abs(unreal(w, BANNER + 3 * i))
        if n <= i:
            break
        cols = n - i - 1
        if len(BANNER_LINES[line]) > cols * 6:
            raise SystemExit('banner line too long: ' + BANNER_LINES[line])
        w[BANNER + 3 * (i + 1):BANNER + 3 * n] = encode(BANNER_LINES[line], cols)
        i, line = n, line + 1

    # words in the code
    for a, s in WORDS.items():
        w[a:a + 3] = lit(s)
    for a, s in TEXTS.items():
        for k in range(0, len(s), 6):
            w[a + k // 2:a + k // 2 + 3] = lit(s[k:k + 6])

    mem[:] = struct.pack('>65536H', *w)

    # the FORMATs, as bytes because they need not be word aligned
    for a, b in FORMATS:
        pa, pb = a.encode('ascii'), b.encode('ascii')
        if len(pb) > len(pa):
            raise SystemExit('format too long by %d: %s' % (len(pb) - len(pa), b))
        at = mem.find(pa)
        if at < 0:
            raise SystemExit('format not found: ' + a)
        if mem.find(pa, at + 1) >= 0:
            raise SystemExit('format twice: ' + a)
        # padded inside the quotes when it is a bare string, else before the bracket
        if pb.endswith(b"'") and pb.startswith(b"'"):
            pad = pb[:-1] + b' ' * (len(pa) - len(pb)) + b"'"
        else:
            pad = pb[:-1] + b' ' * (len(pa) - len(pb)) + pb[-1:]
        put_bytes(mem, at, pad)
    return used, len(entries)


def main():
    raw = open(SRC, 'rb').read()
    head, mem = raw[:0x200], bytearray(raw[0x200:])
    if '--extract' in sys.argv:
        msgs, _ = read_messages(list(struct.unpack('>65536H', bytes(mem))))
        with open(os.path.join(HERE, 'messages.no.txt'), 'w', encoding='ascii', newline='\n') as f:
            for idx, lines in msgs:
                f.write('#%d\n' % idx)
                for text in lines:
                    f.write(text + '\n')
    used, words = build(mem)
    with open(OUT, 'wb') as f:
        f.write(head + bytes(mem))
    print('%s: %d text elements (the original used 11302, the array holds 11314), %d words' % (os.path.basename(OUT), used, words))


if __name__ == '__main__':
    main()
