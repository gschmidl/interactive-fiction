#!/usr/bin/env python3
"""Build gfortran source for the Burroughs B7700 Dungeon from the tape.

Reads contribution A072 of the INTEREX CSL/1000 release 2213
(archive_original/CSL-1000_Rev-2213.zip, see tftape.py): the Burroughs FORTRAN
source &DUNGN (file a07202) and its text file DUNTXT (a07203).  Writes
.build/dungeon.f and the text file as the game names it, .build/ZORK_DBTXT.

What gfortran needs done to B7700 FORTRAN (every edit asserts how often it
matches, so a changed source fails loudly):

  * `$` cards (compiler options) and the three FILE declarations become
    comments; `%` starts a comment outside a literal;
  * a B7700 word is 48 bits, six characters to a word, the first on the
    left; the program keeps characters one to a word (A1), vocabulary words
    in two halves of three letters, file titles in arrays.  Here every
    INTEGER, REAL and LOGICAL is 8 bytes (-fdefault-integer-8 and
    -fdefault-real-8) and a character constant becomes the number whose
    bytes are the characters, blank-filled - what an A1 read gives (the first
    character first in memory); a longer one in a DATA list fills as many
    words as it needs, and a DATA list that is short is filled with zeros,
    as the B7700 left the rest;
  * octal constants O+nnn (only in DATA statements) become decimal;
  * CONCAT (bit-field insert), COMPL and EQUIV (bit-wise) are rewritten where
    they occur: a mask of 36 ones, NOT, and COMPL(EQUIV(A,B)) = IEOR(A,B);
    .IS. (the same word) is .EQ.;
  * R50CNV and G50, which take a word apart by its bit positions, get two
    helpers (KSTR: is this word characters; KCHR: its n-th character);
    ITIME (hour, minute, second out of TIME(7)) calls BTIME;
  * the file statements CHANGE (TITLE=, MYUSE=), INQUIRE(n,PRESENT=v) and
    CLOSE(n,DISP=) become calls of the port's run-time (brt.f); READ(u=r) and
    WRITE(u=r) are direct-access REC=; DATA=label is ERR=; RESULT=v (the
    break key's report) goes, and nothing breaks;
  * the terminal read becomes CALL BREAD, the terminal is unit 6 for output;
    TIME(1) and RANDOM(seed) in RND are BTICK and BRAND (a stand-in).
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..', '..'))
ZIP = os.path.join(GAME, 'archive_original', 'CSL-1000_Rev-2213.zip')
BUILD = os.path.join(HERE, '..', '.build')

sys.path.insert(0, HERE)
import tftape  # noqa: E402

DQ = '"'
counts = {}


def count(key, n=1):
    counts[key] = counts.get(key, 0) + n


def lit(s):
    """A character constant as the number with those bytes (8, blank-filled)."""
    b = s.encode('latin-1')
    assert len(b) <= 8, s
    return str(int.from_bytes(b.ljust(8, b' '), 'little', signed=True))


def chunks(s):
    b = s.encode('latin-1')
    return [b[i:i + 8].decode('latin-1') for i in range(0, len(b), 8)] or ['']


# ----------------------------------------------------------------------
# cards and statements
# ----------------------------------------------------------------------

def is_comment(l):
    return l[:1] in ('C', 'c', '*', '$') or l.strip() == ''


def strip_pct(text, q):
    """Cut a % comment (outside literals) off a card's statement field.  Q:
    a literal from the card before is still open (it runs on to column 72,
    blanks and all, and goes on in column 7 of the next card)."""
    for i, c in enumerate(text):
        if c == DQ:
            q = not q
        elif c == '%' and not q:
            count('% comment')
            return text[:i], q
    if q:
        count('literal running on to the next card')
    return text, q


def statements(lines):
    """[(label, text, first card number)] and comments as (None, card)."""
    out = []
    q = False
    for n, l in enumerate(lines):
        if is_comment(l):
            assert not q, 'a literal runs into a comment at %d' % n
            if l.startswith('$'):
                count('$ card')
                l = 'C' + l
            out.append((None, l, n))
            continue
        if l.startswith('FILE '):
            count('FILE declaration')
            out.append((None, 'C' + l, n))
            continue
        l = l.ljust(72)
        cont = l[5] not in ' 0'
        assert cont or not q, 'a literal is never closed before card %d' % n
        text, q = strip_pct(l[6:72], q)
        if cont:
            assert out and out[-1][0] is not None, 'continuation of nothing at %d' % n
            lab, prev, first = out[-1]
            out[-1] = (lab, prev + text, first)
            continue
        out.append((l[:5], text, n))
    assert not q
    return out


def split_literals(text):
    """[(is_literal, piece)]: literals with their quotes."""
    parts = []
    i = 0
    while i < len(text):
        j = text.find(DQ, i)
        if j < 0:
            parts.append((False, text[i:]))
            break
        k = text.find(DQ, j + 1)
        assert k > 0, text
        if j > i:
            parts.append((False, text[i:j]))
        parts.append((True, text[j:k + 1]))
        i = k + 1
    return parts


def code_sub(text, pattern, repl, key, flags=0):
    """re.sub outside literals, counted."""
    out = []
    for is_lit, p in split_literals(text):
        if is_lit:
            out.append(p)
            continue
        p, n = re.subn(pattern, repl, p, flags=flags)
        if n:
            count(key, n)
        out.append(p)
    return ''.join(out)


# ----------------------------------------------------------------------
# declarations: the size of every array in a program unit
# ----------------------------------------------------------------------

DECL = re.compile(r'^\s*(?:INTEGER|REAL|LOGICAL|DIMENSION|COMMON)\b', re.I)


def array_sizes(unit):
    sizes = {}
    for lab, text, n in unit:
        if lab is None or not DECL.match(text):
            continue
        body = re.sub(r'/\w*/', ',', text)            # COMMON block names
        body = re.sub(r'/[^/]*/', '', body)           # old-style initial values
        for m in re.finditer(r'\b([A-Z][A-Z0-9]*)\s*\(\s*(\d+)\s*\)', body):
            sizes[m.group(1)] = int(m.group(2))
    return sizes


# ----------------------------------------------------------------------
# DATA lists
# ----------------------------------------------------------------------

def split_top(s):
    """Split at commas that are not inside parentheses or literals."""
    out, depth, cur, q = [], 0, '', False
    for c in s:
        if c == DQ:
            q = not q
        if not q:
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == ',' and depth == 0:
                out.append(cur)
                cur = ''
                continue
        cur += c
    out.append(cur)
    return out


def data_value(v):
    """One DATA value (maybe r*v): as a list of converted values."""
    v = v.strip()
    m = re.match(r'^(\d+)\*(.*)$', v)
    rep = ''
    if m:
        rep, v = m.group(1) + '*', m.group(2).strip()
    if v.startswith(DQ):
        s = v[1:-1]
        if len(s.encode('latin-1')) > 8:
            assert not rep, v
            count('long literal in DATA')
            return [lit(c) for c in chunks(s)]
        count('literal')
        return [rep + lit(s)]
    m = re.match(r'^O\+([0-7]+)$', v)
    if m:
        count('octal O+')
        return [rep + str(int(m.group(1), 8))]
    return [rep + v]


def nvalues(vals):
    n = 0
    for v in vals:
        m = re.match(r'^(\d+)\*', v)
        n += int(m.group(1)) if m else 1
    return n


def convert_data_pairs(body, sizes):
    """'names/values/, names/values/' of a DATA statement, converted, with a
    short list for one whole array filled with zeros."""
    out = []
    i = 0
    while i < len(body):
        j = body.index('/', i)
        k = j + 1
        q = False
        while q or body[k] != '/':
            if body[k] == DQ:
                q = not q
            k += 1
        names = body[i:j].strip().lstrip(',').strip()
        vals = []
        for v in split_top(body[j + 1:k]):
            vals += data_value(v)
        if re.match(r'^[A-Z][A-Z0-9]*$', names) and names in sizes:
            short = sizes[names] - nvalues(vals)
            assert short >= 0, (names, sizes[names], nvalues(vals))
            if short:
                count('DATA list filled with zeros')
                vals.append('%d*0' % short)
        out.append('%s/%s/' % (names, ','.join(vals)))
        i = k + 1
        while i < len(body) and body[i] in ' ,':
            i += 1
    return ','.join(out)


def old_style_init(text, sizes):
    """INTEGER A/"x"/,B(n)/.../  ->  the same with converted values."""
    def rep(m):
        name, dims, vals = m.group(1), m.group(2) or '', m.group(3)
        conv = []
        for v in split_top(vals):
            conv += data_value(v)
        if dims:
            n = int(dims.strip('()'))
            short = n - nvalues(conv)
            if short:
                count('DATA list filled with zeros')
                conv.append('%d*0' % short)
        count('initial value in a type statement')
        return '%s%s/%s/' % (name, dims, ','.join(conv))
    return re.sub(r'([A-Z][A-Z0-9]*)\s*(\(\s*\d+\s*\))?\s*/((?:[^/"]|"[^"]*")*)/', rep, text)


# ----------------------------------------------------------------------
# statements
# ----------------------------------------------------------------------

CHANGE = re.compile(r'^\s*CHANGE\s*\(\s*(\w+)\s*,\s*TITLE\s*=\s*(\w+)\s*,(.*)\)\s*$')
INQ = re.compile(r'^\s*INQUIRE\s*\(\s*(\w+)\s*,\s*PRESENT\s*=\s*(\w+)\s*\)\s*$')
CLOSE = re.compile(r'^\s*CLOSE\s*\(\s*(\w+)\s*,\s*DISP\s*=\s*(\w+)\s*\)\s*$')


def format_commas(t):
    """A FORMAT's literals stay literals, but gfortran reads two literals
    with only blanks between them (a literal on the next card) as one with
    a quote inside: put commas where the B7700 did without."""
    parts = split_literals(t)
    out = []
    for i, (is_lit, p) in enumerate(parts):
        if is_lit and out:
            prev = ''.join(out).rstrip()
            if prev and prev[-1] not in ',/(:':
                count('comma before a literal in a FORMAT')
                out.append(',')
        if not is_lit and i > 0 and parts[i - 1][0]:
            s = p.lstrip()
            if s and s[0] not in ',/):':
                count('comma after a literal in a FORMAT')
                p = ',' + s
        out.append(p)
    return ''.join(out)


def compl_equiv(t):
    """COMPL(EQUIV(A,B)) -> IEOR(A,B): EQUIV is bit-wise equivalence."""
    while True:
        m = re.search(r'\bCOMPL\s*\(\s*EQUIV\s*\(', t)
        if not m:
            return t
        i, depth = m.end(), 1
        while depth:
            depth += {'(': 1, ')': -1}.get(t[i], 0)
            i += 1
        inner = t[m.end():i - 1]                   # A,B
        j = i
        while t[j] == ' ':
            j += 1
        assert t[j] == ')', t
        count('COMPL(EQUIV')
        t = t[:m.start()] + 'IEOR(' + inner + ')' + t[j + 1:]


def convert_statement(text, sizes, unit_name):
    t = text
    if re.match(r'^\s*FORMAT\s*\(', t):
        t = format_commas(t)                        # literals stay literals
        t, n = re.subn(r',\s*\)\s*$', ',$)', t)     # the line goes on (DEC's $)
        count('FORMAT ending in a comma', n)
        return t
    if unit_name == 'BLOCKDATA' and re.match(r'^\s*DATA\s+R50MIN\s*/', t):
        count('BLOCK DATA R50MIN')                  # a local nobody can see
        return None
    # the debugger reads the terminal with READ n,list: the line comes from
    # the run-time, and the program's FORMAT reads it
    m = re.match(r'^\s*READ\s+(\d+)\s*,\s*(.+?)\s*$', t)
    if m:
        assert unit_name == 'GUARD', t
        count('GUARD READ n,list')
        # but gfortran's A2 into an INTEGER ends at a comma ("S," of the
        # password): FORMAT 110 (10A2) and 210 (A2) are copied by the run-time
        if (m.group(1), m.group(2)) == ('110', 'LINE'):
            return ' CALL BREADA(LINE,10,2)'
        if (m.group(1), m.group(2)) == ('210', 'CMD'):
            return ' CALL BREADW(CMD,2)'
        return [' CALL BLINE(BRDBUF)', ' READ(BRDBUF,%s) %s' % (m.group(1), m.group(2))]
    if unit_name == 'GUARD' and re.match(r'^\s*IMPLICIT\s+INTEGER\s*\(\s*A-Z\s*\)\s*$', t):
        count('GUARD line buffer')
        return [t, ' CHARACTER*256 BRDBUF']
    t = code_sub(t, r'^\s*LOCK\s*\(\s*20\s*\)\s*$', ' CALL BCLOSE(20,0)', 'LOCK')
    t = code_sub(t, r'\bITIME\b', 'ITIMEB', 'ITIME renamed')   # gfortran has an ITIME
    # free-field input and output: READ /,list from the terminal
    m = re.match(r'^\s*READ\s*/\s*,\s*(\w+)\s*(?:,\s*(\w+)\s*)?$', t)
    if m:
        count('READ /')
        if m.group(2):
            return ' CALL BRDN2(%s,%s)' % (m.group(1), m.group(2))
        return ' CALL BRDN1(%s)' % m.group(1)
    t = code_sub(t, r'^(\s*PRINT\s*)\*\s*/\s*,', r'\1*,', 'PRINT */')
    m = CHANGE.match(t)
    if m:
        use = re.search(r'MYUSE\s*=\s*(IN|OUT|IO)\b', m.group(3))
        assert use, t
        count('CHANGE')
        return ' CALL BCHANG(%s,%s,%d)' % (m.group(1), m.group(2),
                                           {'IN': 1, 'OUT': 2, 'IO': 3}[use.group(1)])
    m = INQ.match(t)
    if m:
        count('INQUIRE PRESENT')
        return ' CALL BPRES(%s,%s)' % (m.group(1), m.group(2))
    m = CLOSE.match(t)
    if m:
        disp = m.group(2)
        assert disp in ('KEEP', 'CRUNCH', 'DELETE'), t
        count('CLOSE DISP')
        return ' CALL BCLOSE(%s,%d)' % (m.group(1), 1 if disp == 'DELETE' else 0)
    if re.match(r'^\s*DATA\b', t):
        body = re.sub(r'^\s*DATA\s*', '', t)
        return ' DATA ' + convert_data_pairs(body, sizes)
    if re.match(r'^\s*REAL\s+GTITLE\s*\(', t):
        count('REAL GTITLE')                        # a title: its bytes must stay
        t = re.sub(r'^(\s*)REAL', r'\1INTEGER', t)
    if re.match(r'^\s*(INTEGER|REAL|LOGICAL)\b.*/', t):
        t = old_style_init(t, sizes)
    # the terminal and the data base
    t = code_sub(t, r'^\s*READ\s*\(\s*INPCH\s*,\s*100\s*\)\s*INBUF\s*$', ' CALL BREAD(INBUF)',
                 'terminal READ')
    t = code_sub(t, r'^\s*READ\s*\(\s*INPCH\s*,\s*110\s*\)\s*ANS\s*$', ' CALL BREADW(ANS,2)',
                 'terminal READ of an answer')
    t = code_sub(t, r'\b(READ|WRITE)\s*\(\s*(\w+)\s*=\s*(\w+)', r'\1(\2,REC=\3', 'direct access')
    t = code_sub(t, r'\bDATA\s*=\s*(\d+)', r'ERR=\1', 'DATA= label')
    t = code_sub(t, r'\s*,\s*RESULT\s*=\s*RSLT', '', 'RESULT=')
    t = code_sub(t, r'\bAND\s*\(\s*RSLT\s*,\s*1\s*\)', '0', 'break test')
    t = code_sub(t, r'^\s*OUTCH\s*=\s*5\s*$', ' OUTCH=6', 'terminal output unit')
    # bits
    t = code_sub(t, r'\bCONCAT\s*\(\s*0\s*,\s*COMPL\s*\(\s*0\s*\)\s*,\s*35\s*,\s*35\s*,\s*36\s*\)',
                 str((1 << 36) - 1), 'CONCAT 36 ones')
    t = compl_equiv(t)
    t = code_sub(t, r'\bCOMPL\s*\(', 'NOT(', 'COMPL')
    t = code_sub(t, r'\.IS\.', '.EQ.', '.IS.')
    if unit_name == 'G50':
        t = code_sub(t, r'^\s*INTEGER\s+CSET\(39\)\s*$', ' INTEGER CSET(39),CHAR', 'G50 CHAR')
    if unit_name == 'R50CNV':
        t = code_sub(t, r'\bCONCAT\s*\(\s*0\s*,\s*A\(I\)\s*,\s*6\s*,\s*45\s*,\s*7\s*\)', 'KSTR(A(I))',
                     'R50CNV KSTR')
        t, n = re.subn(r'\bCONCAT\s*\(\s*" "\s*,\s*OLD\s*,\s*47\s*,\s*48-J\s*,\s*8\s*\)',
                       'KCHR(OLD,J)', t)
        count('R50CNV KCHR', n)
        t = code_sub(t, r'^(\s*INTEGER\s+A\(1\),L,I,J,K,OLD,NEW,G50)\s*$', r'\1,C,KSTR,KCHR',
                     'R50CNV declarations')
    t = code_sub(t, r'\bG50\s*\(\s*FLOAT\s*\(\s*J\s*\)\s*\)', 'G50(J)', 'G50(FLOAT')
    if unit_name == 'RND':
        t = code_sub(t, r'^\s*FOO\s*=\s*TIME\s*\(\s*1\s*\)\s*$', ' FOO=BTICK()', 'TIME(1)')
        t = code_sub(t, r'\bRANDOM\s*\(\s*FOO\s*\)', 'BRAND(FOO)', 'RANDOM')
        t = code_sub(t, r'^\s*INTEGER\s+X\s*$', ' INTEGER X,BTICK', 'RND declarations')
    # literals in code: the numbers of their bytes
    out = []
    for is_lit, p in split_literals(t):
        if is_lit:
            s = p[1:-1]
            assert len(s.encode('latin-1')) <= 8, text
            count('literal')
            out.append(lit(s))
        else:
            out.append(p)
    return ''.join(out)


def unit_name_of(text):
    m = re.match(r'^\s*(?:(?:INTEGER|LOGICAL|REAL)\s+)?(?:FUNCTION|SUBROUTINE)\s+(\w+)', text)
    if m:
        return m.group(1)
    if re.match(r'^\s*BLOCK\s*DATA', text):
        return 'BLOCKDATA'
    return None


ITIME_BODY = [' IMPLICIT INTEGER(A-Z)', ' CALL BTIME(H,M,S)', ' RETURN', ' END']


def convert(lines):
    stmts = statements(lines)
    # program units
    units, cur = [], []
    for s in stmts:
        cur.append(s)
        if s[0] is not None and re.match(r'^\s*END\s*$', s[1]):
            units.append(cur)
            cur = []
    assert not [s for s in cur if s[0] is not None], 'text after the last END'
    tail = cur
    out = []
    for i, unit in enumerate(units):
        name = None
        for lab, text, n in unit:
            if lab is not None:
                name = unit_name_of(text) or ('MAIN' if i == 0 else None)
                break
        sizes = array_sizes(unit)
        if name == 'ITIME':
            count('ITIME body')
            first = [s for s in unit if s[0] is not None][0]
            out.append((first[0], first[1].replace('ITIME', 'ITIMEB')))
            out += [('     ', b) for b in ITIME_BODY]
            continue
        inserted = False
        for lab, text, n in unit:
            if lab is None:
                out.append((None, text))
                continue
            t = convert_statement(text, sizes, name)
            if t is None:
                out.append((None, 'C(port) ' + text.strip()))
                continue
            if isinstance(t, list):
                out.append((lab, t[0]))
                out += [('     ', s) for s in t[1:]]
                continue
            if name == 'MAIN' and not inserted and re.match(r'^\s*IF\s*\(\s*PROTCT', text):
                count('BRTINI')
                out.append(('     ', ' CALL BRTINI'))
                inserted = True
            if name == 'RND' and re.match(r'^\s*END\s*$', text):
                count('RND declarations')
                out.append(('     ', ' RETURN'))
            out.append((lab, t))
            if name == 'RND' and re.match(r'^\s*INTEGER\s+X', text):
                out.append(('     ', ' REAL BRAND'))
    out += [(None, l) for lab, l, n in tail]
    return out


# ----------------------------------------------------------------------
# cards out
# ----------------------------------------------------------------------

def cut_points(text):
    """Places where a line may end: after a comma or before an operator,
    outside literals."""
    pts, q = [], False
    for i, c in enumerate(text):
        if c == DQ:
            q = not q
        elif not q and c in ',(':
            pts.append(i + 1)
    return pts


def cards(label, text, width=1000):
    """A statement as lines of fixed form (compiled -ffixed-line-length-none),
    cut only outside literals."""
    text = text.rstrip()
    out = []
    first = True
    while True:
        room = width - 6
        if len(text) <= room:
            out.append((label if first else '     1') + text)
            break
        pts = [p for p in cut_points(text) if p <= room]
        assert pts, text[:80]
        p = pts[-1]
        out.append((label if first else '     1') + text[:p])
        text = text[p:]
        first = False
    return out


def main():
    files = tftape.from_zip(ZIP)
    src = [r.decode('latin-1').rstrip() for r in tftape.fmp_records(files['a07202'])]
    txt = [r.decode('latin-1').rstrip() for r in tftape.fmp_records(files['a07203'])]
    assert len(src) == 10360 and len(txt) == 3449, (len(src), len(txt))
    os.makedirs(BUILD, exist_ok=True)
    out = []
    for lab, text in convert(src):
        if lab is None:
            out.append(text)
        else:
            out += cards(lab.ljust(5)[:5] + ' ', text)
    with open(os.path.join(BUILD, 'dungeon.f'), 'w', encoding='latin-1', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    with open(os.path.join(BUILD, 'ZORK_DBTXT'), 'w', encoding='latin-1', newline='\n') as f:
        f.write('\n'.join(txt) + '\n')
    expect = {
        '$ card': 4, 'FILE declaration': 3, 'CHANGE': 7, 'INQUIRE PRESENT': 4, 'CLOSE DISP': 9,
        'terminal READ': 1, 'terminal READ of an answer': 1, 'terminal output unit': 1, 'RESULT=': 14, 'CONCAT 36 ones': 6,
        'COMPL(EQUIV': 3, '.IS.': 1, 'G50 CHAR': 1, 'G50(FLOAT': 1, 'R50CNV KSTR': 1,
        'R50CNV KCHR': 1, 'R50CNV declarations': 1, 'ITIME body': 1, 'TIME(1)': 1, 'RANDOM': 1,
        'RND declarations': 2, 'BRTINI': 1, 'REAL GTITLE': 2, 'LOCK': 1,
        'FORMAT ending in a comma': 2, 'BLOCK DATA R50MIN': 1, 'ITIME renamed': 2,
        'READ /': 3, 'PRINT */': 3, 'GUARD READ n,list': 14, 'GUARD line buffer': 1,
    }
    for k, v in sorted(counts.items()):
        print('%-34s %6d' % (k, v))
    bad = {k: (counts.get(k, 0), v) for k, v in expect.items() if counts.get(k, 0) != v}
    assert not bad, bad


if __name__ == '__main__':
    main()
