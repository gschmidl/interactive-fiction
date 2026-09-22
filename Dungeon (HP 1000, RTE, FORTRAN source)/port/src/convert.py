#!/usr/bin/env python3
"""Build gfortran source for Dungeon V3.0a (HP 1000 RTE, FTN4X) from the tape.

Reads Tom Hutchinson's HP 1000 version of Dungeon - contribution F042 of the
INTEREX CSL/1000 release 2240 (archive_original/CSL-1000_Rev-2240.zip, see
hptape.py) - and writes .build/src/dung?.f, one file per source file: the
main program with its BLOCK DATA and output routines (#DUNGA), the five
segments (#DUNGB initialisation, #DUNGC parser, #DUNGD debugger, #DUNGE
save/restore, #DUNGF actions) and the subroutine library (#DUNGL).  It also
writes the messages file @DUNGN, as the game reads it, to .build/@DUNGN.

What gfortran needs done to FTN4X (every edit asserts how often it matches):

  * columns 73-80 are not read; a card is padded to 72 columns, which is
    where the blanks of a Hollerith at the end of a card come from; `!`
    starts a comment (outside literals and Holleriths); the file begins
    FTN4X,L and ends with a `$` card;
  * HP INTEGER and LOGICAL are one 16-bit word, and the program counts on
    it - SAVE writes stretches of COMMON by their length in words, and the
    radix-50 vocabulary is meant to overflow 16 bits - so every INTEGER is
    INTEGER*2 and every LOGICAL LOGICAL*2 (INTEGER*4 stays);
  * two characters to a word, the first in the high byte.  Here the first
    is first in memory, which is what gfortran's Hollerith constants and
    byte offsets give; a Hollerith or quoted constant in an expression or a
    DATA statement becomes the number with its bytes in that order (four to
    an INTEGER*4 element); one handed to a CALL stays as it is;
  * octal constants nB; IABS, MIN0, MAX0 of 16-bit arguments;
  * PROGRAM DUNGN (4,90) and the segments PROGRAM DUNGB(5) ... become
    subroutines; a ,(TWH) date after a FUNCTION statement goes;
  * DECODE of A1/A2 fields and the READs become calls that copy characters
    (gfortran's A editing into an INTEGER stops at a comma);
  * EXEC 6 (end) and EXEC 11 (the time), the FMP calls and REIO are the
    port's, as are DLINK/DLIN2/RETRN, the segment linker (pdngc.c).
"""

import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..', '..'))
ZIP = os.path.join(GAME, 'archive_original', 'CSL-1000_Rev-2240.zip')
BUILD = os.path.join(HERE, '..', '.build')
OUT = os.path.join(BUILD, 'src')

sys.path.insert(0, HERE)
import hptape  # noqa: E402

SQ, DQ = chr(39), chr(34)

# the source files of contribution F042, in load order
FILES = [('f04202', 'dunga', 748), ('f04203', 'dungb', 1234),
         ('f04204', 'dungc', 974), ('f04205', 'dungd', 560),
         ('f04206', 'dunge', 252), ('f04207', 'dungf', 1657),
         ('f04208', 'dungl', 5030)]
MESSAGES = ('f04213', 3460)


def tape():
    z = zipfile.ZipFile(ZIP)
    name = [n for n in z.namelist() if n.endswith('.tf.tape')][0]
    tmp = os.path.join(BUILD, '_tape.tf')
    with open(tmp, 'wb') as f:
        f.write(z.read(name))
    files = hptape.tf_files(tmp)
    os.remove(tmp)
    return {n.split('/')[-1]: d for n, h, d in files}


# ----------------------------------------------------------------------
# cards
# ----------------------------------------------------------------------

def is_comment(l):
    return l[:1] in ('C', 'c', '*') or l.strip() == ''


HOLL = re.compile(r'(?<![0-9A-Za-z_' + SQ + DQ + r'])(\d+)H')


def strip_bang(cards):
    """Cut `!` comments off cards, keeping track of literals and Holleriths
    from card to card (a literal may go on to the next card)."""
    out = []
    q = None            # open quote character
    hl = 0              # Hollerith characters still to come
    for l in cards:
        if is_comment(l):
            out.append(l)
            continue
        text = l[6:]
        i = 0
        cut = None
        while i < len(text):
            c = text[i]
            if hl:
                hl -= 1
                i += 1
                continue
            if q:
                if c == q:
                    q = None
                i += 1
                continue
            if c in (SQ, DQ):
                q = c
                i += 1
                continue
            if c == '!':
                cut = i
                break
            m = HOLL.match(text, i)
            if m and (i == 0 or not text[i - 1].isalnum()):
                hl = int(m.group(1))
                i = m.end()
                continue
            i += 1
        if cut is not None:
            text = text[:cut]
        out.append((l[:6] + text).ljust(72))
    return out


def source(data, expect):
    """A source file as cards: columns 1-72, blank-padded; comments as they
    are.  The first card (FTN4X,L) and the closing $ card are dropped."""
    recs, _ = hptape.fmp_records(data)
    assert len(recs) == expect, (len(recs), expect)
    lines = [r.decode('latin-1').rstrip() for r in recs]
    assert lines[0].startswith('FTN4X'), lines[0]
    lines = lines[1:]
    assert lines[-1] == '$', lines[-1]
    lines = lines[:-1]
    assert '$' not in lines
    cards = []
    for l in lines:
        if is_comment(l):
            cards.append(l)
        else:
            cards.append(l[:72].ljust(72))
    return strip_bang(cards)


def is_cont(l):
    return (not is_comment(l) and l[:5].strip() == ''
            and l[5] not in (' ', '0'))


def statements(cards):
    """[(comments before, [cards of the statement])]"""
    out = []
    pend = []
    cur = None
    for l in cards:
        if is_comment(l):
            pend.append(l)
            continue
        if is_cont(l):
            assert cur is not None, l
            cur[1].extend(l2 for l2 in pend)
            pend = []
            cur[1].append(l)
            continue
        if cur is not None:
            out.append(cur)
        cur = (pend, [l])
        pend = []
    if cur is not None:
        out.append(cur)
    if pend:
        out.append((pend, []))
    return out


def joined(stmt):
    label = stmt[0][:5].strip()
    text = ''.join(l[6:72] for l in stmt if not is_comment(l))
    return label, text


def safe_cut(text, room):
    """Where to end a card: after a comma, parenthesis, blank or slash
    outside a literal or Hollerith - or at exactly column 72, where a
    literal goes on to the next card."""
    q = None
    hl = 0
    best = None
    i = 0
    while i < room and i < len(text):
        c = text[i]
        if hl:
            hl -= 1
            i += 1
            continue
        if q:
            if c == q:
                q = None
                best = i + 1
            i += 1
            continue
        if c in (SQ, DQ):
            q = c
            i += 1
            continue
        m = HOLL.match(text, i)
        if m and (i == 0 or not text[i - 1].isalnum()):
            hl = int(m.group(1))
            i = m.end()
            continue
        if c in ',( /':
            best = i + 1
        i += 1
    return best or room


def cards(label, text):
    out = []
    first = True
    text = text.rstrip() if not text.rstrip().endswith('H') else text
    while text or first:
        if len(text) <= 66:
            piece, text = text, ''
        else:
            cut = safe_cut(text, 66)
            piece, text = text[:cut], text[cut:]
        if first:
            out.append(label.ljust(5) + ' ' + piece)
            first = False
        else:
            out.append('     &' + piece)
    return out


# ----------------------------------------------------------------------
# constants
# ----------------------------------------------------------------------

def pack(chars, width=2):
    """Characters -> integer values of `width` bytes, first character first
    in memory (little-endian), blank-padded."""
    if len(chars) % width:
        chars += ' ' * (width - len(chars) % width)
    out = []
    for i in range(0, len(chars), width):
        v = 0
        for k in range(width):
            v |= ord(chars[i + k]) << (8 * k)
        if v >= 1 << (8 * width - 1):
            v -= 1 << (8 * width)
        out.append(str(v))
    return out


OCTAL = re.compile(r'(?<![\w' + SQ + DQ + r'])([0-7]+)B(?![\w.])')


def scan(text, fn_code, fn_lit=None, fn_holl=None):
    """Walk text: fn_code(piece) for code outside literals and Holleriths,
    fn_lit(quote, body) for a literal, fn_holl(n, chars, pos) for nH."""
    out = []
    i = 0
    buf = ''
    while i < len(text):
        c = text[i]
        if c in (SQ, DQ):
            j = text.find(c, i + 1)
            while j != -1 and text[j + 1:j + 2] == c:     # doubled quote
                j = text.find(c, j + 2)
            assert j != -1, text
            out.append(fn_code(buf))
            buf = ''
            body = text[i + 1:j]
            out.append(fn_lit(c, body) if fn_lit else text[i:j + 1])
            i = j + 1
            continue
        m = HOLL.match(text, i)
        if m and (i == 0 or not text[i - 1].isalnum()):
            n = int(m.group(1))
            chars = text[m.end():m.end() + n].ljust(n)
            out.append(fn_code(buf))
            buf = ''
            out.append(fn_holl(n, chars, i) if fn_holl
                       else text[i:m.end() + n])
            i = m.end() + n
            continue
        buf += c
        i += 1
    out.append(fn_code(buf))
    return ''.join(out)


def octals(piece, counts, data=False):
    """nB -> decimal.  An octal constant is one 16-bit word: 100000B and up
    are negative (in parentheses in an expression)."""
    def rep(m):
        counts['octal'] += 1
        v = int(m.group(1), 8)
        assert v <= 0o177777, m.group(0)
        if v > 32767:
            v -= 65536
            return str(v) if data else '(%d)' % v
        return str(v)
    return OCTAL.sub(rep, piece)


# ----------------------------------------------------------------------
# DATA statements
# ----------------------------------------------------------------------

def split_top(s, sep=','):
    out = []
    depth = 0
    cur = ''
    q = None
    hl = 0
    i = 0
    while i < len(s):
        c = s[i]
        if hl:
            cur += c
            hl -= 1
            i += 1
            continue
        if q:
            cur += c
            if c == q:
                q = None
            i += 1
            continue
        if c in (SQ, DQ):
            q = c
            cur += c
            i += 1
            continue
        m = HOLL.match(s, i)
        if m and (i == 0 or not s[i - 1].isalnum()):
            hl = int(m.group(1))
            cur += s[i:m.end()]
            i = m.end()
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        if c == sep and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += c
        i += 1
    out.append(cur)
    return out


def data_pairs(body):
    """'A/1,2/,B/3/' -> [('A', '1,2'), ('B', '3')] (outside literals)."""
    pairs = []
    i = 0
    q = None
    hl = 0
    start = 0
    names = None
    while i < len(body):
        c = body[i]
        if hl:
            hl -= 1
            i += 1
            continue
        if q:
            if c == q:
                q = None
            i += 1
            continue
        if c in (SQ, DQ):
            q = c
            i += 1
            continue
        m = HOLL.match(body, i)
        if m and (i == 0 or not body[i - 1].isalnum()) and names is not None:
            hl = int(m.group(1))
            i = m.end()
            continue
        if c == '/':
            if names is None:
                names = body[start:i]
            else:
                pairs.append((names.strip().lstrip(',').strip(),
                              body[start:i]))
                names = None
            start = i + 1
        i += 1
    assert names is None and body[start:].strip() == '', body
    return pairs


def data_value(v, width, counts):
    """One DATA value -> list of values.  Literals and Holleriths are packed
    `width` bytes to an element; r*value repeats."""
    v = v.strip()
    m = re.match(r'^(\d+)\*(.*)$', v)
    rep = ''
    if m:
        rep, v = m.group(1) + '*', m.group(2).strip()
    if v[:1] in (SQ, DQ) and v[-1:] == v[:1]:
        body = v[1:-1].replace(v[0] * 2, v[0])
        counts['literal'] += 1
        vals = pack(body, width)
    else:
        m = re.match(r'^(\d+)H(.*)$', v)
        if m and len(m.group(2)) >= int(m.group(1)) - 0:
            n = int(m.group(1))
            counts['hollerith'] += 1
            vals = pack(m.group(2)[:n].ljust(n), width)
        else:
            vals = [octals(v, counts, data=True)]
    if rep:
        assert len(vals) == 1, v
        return [rep + vals[0]]
    return vals


def convert_data(text, int4, counts):
    m = re.match(r'^(\s*DATA\s*)(.*)$', text)
    head, body = m.group(1), m.group(2).rstrip()
    out = []
    for names, values in data_pairs(body):
        first = re.match(r'\s*(\w+)', names).group(1).upper()
        width = 4 if first in int4 else 2
        vals = []
        for v in split_top(values):
            vals.extend(data_value(v, width, counts))
        out.append('%s/%s/' % (names, ','.join(vals)))
    counts['data'] += 1
    return head + ','.join(out)


# ----------------------------------------------------------------------
# statement edits
# ----------------------------------------------------------------------

# whole statements (text after column 6, as joined, trailing blanks off),
# replaced; each must match exactly the number of times given
STMT_EDITS = [
    # the version string " V3.0A": HP puts a word's first character in the
    # high byte; here it is the low one
    ('VERXX(2)=2H0. + VMAJ * 256', 'VERXX(2)=2H0. + VMAJ', 1),
    ('VERXX(3)=30000B + (VMIN*256) + (VEDIT/256)',
     'VERXX(3)=48 + VMIN + 256*IAND(VEDIT,255_2)', 1),
    # R50CV: a vocabulary entry is text if its first byte - the high byte
    # of the INTEGER*4 on the HP - is not zero; here that byte comes last
    ('CALL UMOVE (1, OLD, 0, ITEMP,0)', 'ITEMP = INT(ISHFT(OLD,-24),2)', 1),
    # the library's own AND and OR (renamed: gfortran has AND and OR)
    ('AND = IAND(I1,I2)', 'KAND = IAND(I1,I2)', 1),
    ('OR = IOR(I1,I2)', 'KOR = IOR(I1,I2)', 1),
    # the reads (a character copy each; see pdng.f)
    ('READ (INPCH,110) LINE', 'CALL PRDA(LINE,10)', 1),
    ('READ (INPCH,210) CMD', 'CALL PRDA(CMD,1)', 1),
    ('READ (INPCH,*) J,K', 'CALL PRDI2(J,K)', 2),
    ('READ (INPCH,*) J', 'CALL PRDI1(J)', 1),
    ('READ (INPCH,490) FLAGS(J)', 'CALL PRDL1(FLAGS(J))', 1),
    ('READ (INPCH,600) SWITCH(J-33)', 'CALL PRDI6(SWITCH(J-33))', 1),
    ('READ (INPCH,600) EQR(J,K)', 'CALL PRDI6(EQR(J,K))', 1),
    ('READ (INPCH,600) EQO(J,K)', 'CALL PRDI6(EQO(J,K))', 1),
    ('READ (INPCH,600) EQA(J,K)', 'CALL PRDI6(EQA(J,K))', 1),
    ('READ (INPCH,600) EQC(J,K)', 'CALL PRDI6(EQC(J,K))', 1),
    ('READ (INPCH,490) CFLAG(J)', 'CALL PRDL1(CFLAG(J))', 1),
    ('READ (INPCH,620) TRAVEL(J)', 'CALL PRDO6(TRAVEL(J))', 1),
    ('READ (INPCH,600) EQV(J,K)', 'CALL PRDI6(EQV(J,K))', 1),
    ('READ (INPCH,600) EQN(J)', 'CALL PRDI6(EQN(J))', 1),
    ('READ (INPCH,600) HERE', 'CALL PRDI6(HERE)', 1),
    ('READ (INPCH,620) PRSFLG', 'CALL PRDO6(PRSFLG)', 1),
    ('READ (INPCH,20) FILEN', 'CALL PRDA(FILEN,3)', 2),
    # DECODE of A1 and A2 fields from a record of the messages file
    ('DECODE(80,475,IBUFF) LINE', 'CALL PUNPK1(IBUFF,LINE,78)', 1),
    ('DECODE(80,99,IBUFF) IN', 'CALL PUNPK1(IBUFF,IN,80)', 5),
    ('DECODE (80,99,IBUFF) IN', 'CALL PUNPK1(IBUFF,IN,80)', 1),
    ('DECODE(80,99,IBUFF) DIR,(IN(I),I=1,78)',
     'CALL PUNPK2(IBUFF,DIR,IN)', 1),
]


def header(text, counts):
    """Program-unit statements."""
    t = text.strip()
    m = re.match(r'^SUBROUTINE\s+(\w+)(.*)$', t)
    if m:
        return 'SUBROUTINE ' + RENAME.get(m.group(1).upper(), m.group(1)) \
            + m.group(2)
    if re.match(r'^BLOCK\s*DATA\b', t):
        return t
    m = re.match(r'^PROGRAM\s+(\w+)\s*(\(.*\))?$', t)
    if m:
        counts['program'] += 1
        return 'SUBROUTINE ' + m.group(1)
    m = re.match(r'^((?:INTEGER|LOGICAL)\s+)?FUNCTION\s+(\w+)\s*'
                 r'(\([^)]*\))\s*(,\s*\(.*)?$', t)
    if m:
        kind = (m.group(1) or '').strip()
        if kind == 'INTEGER':
            kind = 'INTEGER*2 '
        elif kind == 'LOGICAL':
            kind = 'LOGICAL*2 '
        if m.group(4):
            counts['stamp'] += 1
        name = RENAME.get(m.group(2).upper(), m.group(2))
        return '%sFUNCTION %s%s' % (kind, name, m.group(3))
    return None


UNIT = re.compile(r'^\s*(PROGRAM|SUBROUTINE|BLOCK\s*DATA|'
                  r'(?:INTEGER|LOGICAL)\s+FUNCTION|FUNCTION)\b')


def units(stmts):
    """Split statements into program units at END."""
    out = []
    cur = []
    for comments, cardl in stmts:
        cur.append((comments, cardl))
        if cardl:
            label, text = joined(cardl)
            if re.match(r'^\s*END\s*$', text):
                out.append(cur)
                cur = []
    if any(c for _, c in cur):
        raise SystemExit('text after the last END')
    if cur:
        out[-1].extend(cur)
    return out


def int4_names(unit):
    names = set()
    for _, cardl in unit:
        if not cardl:
            continue
        label, text = joined(cardl)
        m = re.match(r'^\s*INTEGER\*4\s+(.*)$', text)
        if m:
            for part in split_top(m.group(1)):
                names.add(re.match(r'\s*(\w+)', part).group(1).upper())
    return names


FMP = {'OPEN': ('FOPEN', 6), 'CREAT': ('FCREAT', 7), 'CLOSE': ('FCLOSE', 1),
       'READF': ('FREADF', 6), 'WRITF': ('FWRITF', 5),
       'PURGE': ('FPURGE', 5)}


def calls(text, counts):
    """EXEC, the FMP calls.  Returns the new text."""
    m = re.match(r'^(.*?\bCALL\s*)(\w+)\s*\((.*)\)\s*$', text)
    if not m:
        return text
    pre, name, args = m.group(1), m.group(2).upper(), m.group(3)
    a = [x.strip() for x in split_top(args)]
    if name == 'EXEC':
        if a[0] == '6':
            assert len(a) == 1
            counts['exec6'] += 1
            return pre + 'PSTOP'
        if a[0] == '11':
            assert len(a) == 2
            counts['exec11'] += 1
            return pre + 'PTIME(%s)' % a[1]
        raise SystemExit('EXEC ' + a[0])
    if name in FMP:
        new, n = FMP[name]
        assert len(a) <= n, (name, a)
        if name == 'READF' and len(a) == 4:
            a.append('IFMPDM')      # the length read: an output, so a variable
        a += ['0'] * (n - len(a))
        counts['fmp'] += 1
        return pre + '%s(%s)' % (new, ','.join(a))
    return text


def hollerith_spans(text):
    """Spans of the argument lists of CALL statements (a Hollerith there
    stays as it is)."""
    m = re.search(r'\bCALL\s*\w+\s*\(', text)
    if not m:
        return []
    depth = 1
    j = m.end()
    while j < len(text) and depth:
        if text[j] == '(':
            depth += 1
        elif text[j] == ')':
            depth -= 1
        j += 1
    return [(m.end(), j)]


# the library's AND, OR and ITIME have the names of gfortran intrinsics
RENAME = {'AND': 'KAND', 'OR': 'KOR', 'ITIME': 'ITIMX'}


def code_fixes(p, counts):
    """Code outside literals: octal constants, FTN4X's bitwise .AND. of an
    integer and a constant, the renamed routines."""
    p = octals(p, counts)
    p, k = re.subn(r'\b([A-Z]\w*)\.AND\.(\d+)\b', r'IAND(\1,\2_2)', p)
    counts['intand'] += k
    p, k = re.subn(r'(?<!\.)\b(AND|OR|ITIME)\s*\(',
                   lambda m: RENAME[m.group(1)] + '(', p)
    counts['rename'] += k
    return p


def sf_literals(body, sfuncs, counts):
    """A statement function's arguments are checked for kind: an integer
    literal handed to one (INTEGER*4 in gfortran) is made INTEGER*2."""
    for name in sfuncs:
        out = ''
        i = 0
        for m in re.finditer(r'\b%s\s*\(' % name, body):
            if m.start() < i:
                continue
            depth = 1
            j = m.end()
            while depth:
                if body[j] == '(':
                    depth += 1
                elif body[j] == ')':
                    depth -= 1
                j += 1
            args = split_top(body[m.end():j - 1])
            new = []
            for a in args:
                if re.match(r'^\s*-?\d+\s*$', a):
                    a = a.strip() + '_2'
                    counts['sf_literal'] += 1
                new.append(a)
            out += body[i:m.end()] + ','.join(new) + ')'
            i = j
        body = out + body[i:]
    return body


def arrays_and_sfuncs(unit):
    """Names with dimensions in the unit, and its statement functions."""
    arrays = set()
    for _, cardl in unit:
        if not cardl:
            continue
        label, text = joined(cardl)
        m = re.match(r'^\s*(DIMENSION|COMMON|INTEGER\*?\d*|LOGICAL\*?\d*|'
                     r'REAL)\b(.*)$', text)
        if not m:
            continue
        rest = re.sub(r'/\s*\w*\s*/', ',', m.group(2))
        for part in split_top(rest):
            mm = re.match(r'\s*(\w+)\s*\(', part)
            if mm:
                arrays.add(mm.group(1).upper())
    sfuncs = set()
    for _, cardl in unit:
        if not cardl:
            continue
        label, text = joined(cardl)
        m = re.match(r'^\s*(\w+)\s*\(\s*\w+(\s*,\s*\w+)*\s*\)\s*=', text)
        if m and m.group(1).upper() not in arrays:
            sfuncs.add(m.group(1).upper())
    return sfuncs


def convert_stmt(text, int4, counts, sfuncs=()):
    body = text.strip()
    # IMPLICIT and the types
    if re.match(r'^IMPLICIT\s+INTEGER\s*\(A-Z\)$', body):
        counts['implicit'] += 1
        return 'IMPLICIT INTEGER*2 (A-Z)'
    m = re.match(r'^INTEGER(\s+)(?!\*|FUNCTION)(.*)$', body)
    if m:
        counts['integer'] += 1
        text = 'INTEGER*2 ' + octals(m.group(2), counts)
        return text
    m = re.match(r'^LOGICAL(\s+)(?!\*|FUNCTION)(.*)$', body)
    if m:
        counts['logical'] += 1
        return 'LOGICAL*2 ' + m.group(2)
    if re.match(r'^DATA\b', body):
        return convert_data(body, int4, counts)
    if re.match(r'^FORMAT\s*\(', body):
        return body
    # constants, outside CALL argument lists where a Hollerith stays
    spans = hollerith_spans(body)

    def holl(n, chars, pos):
        if any(a <= pos < b for a, b in spans):
            counts['holl_arg'] += 1
            return '%dH%s' % (n, chars)
        assert n <= 2, (n, chars, body)
        counts['holl_expr'] += 1
        return pack(chars.ljust(2))[0]

    def lit(q, b):
        raise SystemExit('literal outside DATA/FORMAT: ' + body)

    body = scan(body, lambda p: code_fixes(p, counts), lit, holl)
    if sfuncs:
        body = sf_literals(body, sfuncs, counts)
    # intrinsics of 16-bit arguments
    body, k = re.subn(r'\b(IABS|MIN0|MAX0)\s*\(',
                      lambda m: {'IABS': 'ABS(', 'MIN0': 'MIN(',
                                 'MAX0': 'MAX('}[m.group(1)], body)
    counts['intrinsic'] += k
    # writes to the terminal
    body, k = re.subn(r'\bWRITE\s*\(\s*OUTCH\s*,', 'WRITE(PU(OUTCH),', body)
    counts['write'] += k
    assert not re.search(r'\bWRITE\s*\((?!PU\()', body), body
    body = calls(body, counts)
    assert not re.search(r'\b(READ|DECODE|ENCODE)\s*\(', body), body
    return body


def convert(name, cardl, counts, used):
    stmts = statements(cardl)
    out = []
    for unit in units(stmts):
        int4 = int4_names(unit)
        sfuncs = arrays_and_sfuncs(unit)
        has_implicit = any(
            c and re.match(r'^\s*IMPLICIT\b', joined(c)[1]) for _, c in unit)
        blockdata = False
        first = True
        for comments, c in unit:
            out.extend(comments)
            if not c:
                continue
            label, text = joined(c)
            key = text.strip()
            for i, (old, new, n) in enumerate(STMT_EDITS):
                if key == old:
                    used[i] += 1
                    text = new
                    break
            if first:
                h = header(text, counts)
                if h is None:
                    assert re.match(r'^\s*BLOCK\s*DATA', text), text
                    h = text.strip()
                out.extend(cards(label, h))
                blockdata = h.startswith('BLOCK')
                if not has_implicit:
                    out.append('      IMPLICIT INTEGER*2 (I-N)')
                    counts['implicit_added'] += 1
                    if not blockdata:
                        out.append(PU_DECL)
                        counts['pu'] += 1
                first = False
                continue
            new = convert_stmt(text, int4, counts, sfuncs)
            out.extend(cards(label, new))
            if new.startswith('IMPLICIT') and not blockdata:
                out.append(PU_DECL)
                counts['pu'] += 1
    return out


# the port's unit number for an LU (pdng.f)
PU_DECL = '      INTEGER*4 PU'


def main():
    os.makedirs(OUT, exist_ok=True)
    files = tape()
    counts = {k: 0 for k in (
        'program', 'stamp', 'implicit', 'implicit_added', 'integer',
        'logical', 'data', 'literal', 'hollerith', 'holl_expr', 'holl_arg',
        'octal', 'intrinsic', 'write', 'exec6', 'exec11', 'fmp', 'intand',
        'rename', 'sf_literal', 'pu')}
    used = [0] * len(STMT_EDITS)
    for fid, name, n in FILES:
        cardl = source(files[fid], n)
        lines = convert(name, cardl, counts, used)
        with open(os.path.join(OUT, name + '.f'), 'w', newline='\n') as f:
            f.write('C     %s.f - generated by convert.py from %s on the '
                    'CSL/1000 2240 tape\n' % (name, fid.upper()))
            for l in lines:
                f.write(l.rstrip() + '\n')
    bad = [(STMT_EDITS[i][0], used[i], STMT_EDITS[i][2])
           for i in range(len(STMT_EDITS)) if used[i] != STMT_EDITS[i][2]]
    if bad:
        raise SystemExit('edits matched wrongly: %r' % bad)
    # the messages file, as the game reads it: 80-byte records
    recs, _ = hptape.fmp_records(files[MESSAGES[0]])
    assert len(recs) == MESSAGES[1] and all(len(r) == 80 for r in recs)
    with open(os.path.join(BUILD, '@DUNGN'), 'wb') as f:
        f.write(b'RTEFMP 4 %d\n' % len(recs))
        for r in recs:
            f.write(bytes([40, 0]) + r)
    print('convert: ' + ', '.join('%s %d' % kv for kv in counts.items()))


if __name__ == '__main__':
    main()
