"""The second half of convert.py: READs, program units, the main pass.
(Kept in its own file; convert.py imports it.)"""

import os
import re

from convert import (DQ, OUT, cards, code_only, edit, holleriths, is_comment,
                     joined, OCTAL, source, statements)

# ----------------------------------------------------------------------
# the READs: each becomes one CALL (they sit in logical IFs)
# ----------------------------------------------------------------------

READS = [
    ('      IF(ID.EQ.9999) READ(LU,*)ID', '      IF(ID.EQ.9999) CALL PRDI1(ID)'),
    ('      READ(LU,8)NTAG', '      CALL PRDA(NTAG,10)'),
    ('      IF(LA.EQ.0)READ(LU,12)(MSGR(L),L=1,36)',
     '      IF(LA.EQ.0)CALL PRDA(MSGR,36)'),
    # the other LU, a tape unit playing back a game
    ('      IF(LA.NE.0)READ(LA,12)(MSGR(L),L=1,36)',
     '      IF(LA.NE.0)CALL PRDAL(LA,MSGR,36)'),
    ('      IF(ITST(21).NE.LU)READ(LU,80030) IANS',
     '      IF(ITST(21).NE.LU)CALL PRDA(IANS,1)'),
    ('      READ(LU,90074)(ICOMM(L),L=2,37)', '      CALL PRDA(ICOMM(2),36)'),
    (' 1015 READ(LU,1030) IANS', ' 1015 CALL PRDA(IANS,1)'),
    (' 9020 READ(LU,9025)IANS', ' 9020 CALL PRDA(IANS,1)'),
    ('      READ(LU,9020)IANS,IAN,ID', '      CALL PRDAAI(IANS,IAN,ID)'),
    ('      READ(LU,*) IC', '      CALL PRDI1(IC)'),
    ('      READ(LU,11070)(NAMF(I),I=2,3)', '      CALL PRDA(NAMF(2),2)'),
    ('      IF(LA.EQ.0)READ(LU,*)IC', '      IF(LA.EQ.0)CALL PRDI1(IC)'),
    ('      IF(LA.NE.0)READ(LA,14032)IC', '      IF(LA.NE.0)CALL PRDI6L(LA,IC)'),
    ('      READ(LU,*)IANS', '      CALL PRDI1(IANS)'),
    ('      READ(LU,21220)IANS,KEYM', '      CALL PRDAK(IANS,KEYM)'),
    ('      READ(LU,*)LA', '      CALL PRDI1(LA)'),
    ('      READ(LU,*) IANS', '      CALL PRDI1(IANS)'),
    ('      READ(LU,*) INH,NDAT', '      CALL PRDI2(INH,NDAT)'),
    ('      READ(LU,*) ITM,NDAT', '      CALL PRDI2(ITM,NDAT)'),
    ('      READ(LU,*) IRMN,NDAT', '      CALL PRDI2(IRMN,NDAT)'),
    ('      READ(LU,*)IRMN', '      CALL PRDI1(IRMN)'),
    ('      READ(LU,*) IXIT,NDAT', '      CALL PRDI2(IXIT,NDAT)'),
    ('      READ(LU,*) ITS,NDAT', '      CALL PRDI2(ITS,NDAT)'),
    ('      READ(LU,9820)IRM', '      CALL PRDA(IRM,8)'),
    ('      READ(LU,*)ITM,NDAT', '      CALL PRDI2(ITM,NDAT)'),
    # STOP after the configuration error: through the port's STOP
    ('      IF(IERR.LT.0)STOP', '      IF(IERR.LT.0)CALL PSTOP'),
    # a NUL character followed by a blank, in HP order
    ('      IF(MSG(J).EQ.32)MSG(J)=8224',
     '      IF(MSG(J).EQ.CONE(0))MSG(J)=8224'),
    ('      IF(MSG(J+1).EQ.32)MSG(J+1)=8224',
     '      IF(MSG(J+1).EQ.CONE(0))MSG(J+1)=8224'),
]
READ_COUNTS = {'      READ(LU,*) IC': 2,
               '      READ(LU,11070)(NAMF(I),I=2,3)': 2,
               '      READ(LU,*)IANS': 2}


def reads(lines):
    for old, new in READS:
        want = READ_COUNTS.get(old, 1)
        n = lines.count(old)
        if n != want:
            raise SystemExit('read edit matched %d times, expected %d: %s'
                             % (n, want, old))
        lines = [new if l == old else l for l in lines]
    left = [l for l in lines if not is_comment(l) and
            re.search(r'\bREAD\s*\(', l)]
    if left:
        raise SystemExit('READ left: %s' % left[0])
    return lines


# ----------------------------------------------------------------------
# program units
# ----------------------------------------------------------------------

UNITHEAD = re.compile(r'^ {6}\s*(SUBROUTINE|PROGRAM|BLOCK DATA)\s+(\w+)')
COMPLEX_TWIN = {'CVRB': 'IVRB', 'CCLS': 'ICLS', 'CRNM': 'IRNM',
                'CRSN': 'IRSN', 'CPRP': 'IPRP', 'CDTN': 'IDTN',
                'CTEM': 'ITEM'}


def units(lines):
    out = []
    cur = []
    for l in lines:
        cur.append(l)
        if not is_comment(l) and re.match(r'^[ 0-9]{5} \s*END\s*$', l):
            out.append(cur)
            cur = []
    if any(not is_comment(l) for l in cur):
        raise SystemExit('text after the last END')
    if cur:
        out[-1].extend(cur)
    return out


def split_args(s):
    out = []
    depth = 0
    cur = ''
    for c in s:
        if c == ',' and depth == 0:
            out.append(cur)
            cur = ''
            continue
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        cur += c
    out.append(cur)
    return out


def exec_calls(text, counts):
    """CALL EXEC(8,NAME[,p...]) -> CALL PSEG(NAME,p1,..,p5), missing
    parameters 0;  CALL EXEC(11,ITIM) -> CALL PTIME(ITIM)."""
    out = []
    i = 0
    while True:
        m = re.search(r'CALL\s*EXEC\s*\(', text[i:])
        if not m:
            out.append(text[i:])
            break
        s = i + m.start()
        a = i + m.end()
        depth = 1
        j = a
        while depth:
            if text[j] == '(':
                depth += 1
            elif text[j] == ')':
                depth -= 1
            j += 1
        args = split_args(text[a:j - 1])
        code = args[0].strip()
        if code == '8':
            p = [x.strip() for x in args[2:]]
            assert len(p) <= 5, args
            p += ['0'] * (5 - len(p))
            rep = 'CALL PSEG(%s,%s)' % (args[1].strip(), ','.join(p))
            counts['exec8'] += 1
        elif code == '11':
            assert len(args) == 2
            rep = 'CALL PTIME(%s)' % args[1].strip()
            counts['exec11'] += 1
        else:
            raise SystemExit('EXEC %s' % code)
        out.append(text[i:s])
        out.append(rep)
        i = j
    return ''.join(out)


# FMP calls take optional trailing arguments; the port's have fixed ones
FMP = {'OPEN': ('FOPEN', 6), 'CREAT': ('FCREAT', 7), 'CLOSE': ('FCLOSE', 1),
       'READF': ('FREADF', 4), 'WRITF': ('FWRITF', 4),
       'POSNT': ('FPOSNT', 3)}


def fmp_calls(text, counts):
    out = []
    i = 0
    while True:
        m = re.search(r'CALL\s*(OPEN|CREAT|CLOSE|READF|WRITF|POSNT)\s*\(',
                      text[i:])
        if not m:
            out.append(text[i:])
            break
        s = i + m.start()
        a = i + m.end()
        depth = 1
        j = a
        while depth:
            if text[j] == '(':
                depth += 1
            elif text[j] == ')':
                depth -= 1
            j += 1
        args = [x.strip() for x in split_args(text[a:j - 1])]
        new, n = FMP[m.group(1)]
        if len(args) > n:
            raise SystemExit('%s with %d arguments' % (m.group(1), len(args)))
        args += ['0'] * (n - len(args))
        out.append(text[i:s])
        out.append('CALL %s(%s)' % (new, ','.join(args)))
        counts['fmp'] += 1
        i = j
    return ''.join(out)


def splitdq(text):
    """Two literals meeting as "..."" ..." - where the comma between them
    stood in column 73, which FTN4 did not read (the relocatable on the
    tape has them meeting so).  HP's formatter took "" as the end of one
    literal and the start of the next; gfortran would print a quote."""
    out = []
    n = 0
    q = False
    i = 0
    while i < len(text):
        c = text[i]
        if c == DQ:
            if q and text[i + 1:i + 2] == DQ:
                out.append(DQ + ',' + DQ)
                n += 1
                i += 2
                continue
            q = not q
        out.append(c)
        i += 1
    return ''.join(out), n


def litrep(text):
    """A repeat count on a literal in a FORMAT, 3"..." -> 3("..."): digits
    outside a literal, straight after , ( / or the end of a literal, and
    straight before one."""
    out = []
    n = 0
    i = 0
    q = False
    while i < len(text):
        c = text[i]
        if q:
            out.append(c)
            if c == DQ:
                q = False
            i += 1
            continue
        if c == DQ:
            q = True
            out.append(c)
            i += 1
            continue
        m = re.match(r'\d+', text[i:])
        if m and text[i + m.end():i + m.end() + 1] == DQ:
            prev = ''.join(out).rstrip()[-1:]
            if prev in (',', '(', '/', DQ):
                j = text.index(DQ, i + m.end() + 1)
                out.append(m.group(0) + '(' + text[i + m.end():j + 1] + ')')
                i = j + 1
                n += 1
                continue
        out.append(c)
        i += 1
    return ''.join(out), n


def convert_unit(u, counts):
    first = next(i for i, l in enumerate(u) if not is_comment(l))
    m = UNITHEAD.match(u[first])
    assert m, u[first]
    kind, name = m.group(1), m.group(2)
    seg = kind == 'PROGRAM'
    if seg:
        u = u[:first] + ['      SUBROUTINE %s' % name,
                         'C     PROGRAM %s(5): an RTE segment' % name] + \
            u[first + 1:]
    head = ['      IMPLICIT INTEGER*2 (I-N)']
    if kind != 'BLOCK DATA':
        head += ['      INTEGER PU',
                 '      INTEGER*2 CJOIN,CFIRST,CSECND,CONE']
    out = []
    for comments, stmt in statements(u):
        out.extend(comments)
        if not stmt:
            continue
        label, text = joined(stmt)
        orig = text
        body = text.lstrip()
        if re.match(r'INTEGER\s', body):
            text = text.replace('INTEGER', 'INTEGER*2', 1)
            counts['integer'] += 1
        # MMSB's AND handling writes below IWRD: IWRD(1-6,0) when AND
        # is the first word of a command, and then - once that has left
        # AND in IWRD(6,0) and a lone GET or DROP makes the word count -1
        # - IWRD(1-6,-1), 18 words below.  On the HP that was whatever
        # the loader put below the common block; here it is a pad.
        text, k = re.subn(r'^(\s*)COMMON\s*/MMBC/\s*IWRD\(9,9\)',
                          r'\1COMMON/MMBC/IWRDP(18),IWRD(9,9)', text)
        counts['pad'] += k
        mm = re.match(r'(\s*DATA\s*)(C[A-Z]+)(\s*/)', text)
        if mm and mm.group(2) in COMPLEX_TWIN:
            text = (mm.group(1) + COMPLEX_TWIN[mm.group(2)] + mm.group(3)
                    + text[mm.end():])
            counts['complexdata'] += 1
        text, k = holleriths(text)
        counts['hollerith'] += k
        if re.match(r'\s*FORMAT\s*\(', text):
            text, k = litrep(text)
            counts['litrep'] += k
            text, k = splitdq(text)
            counts['splitdq'] += k

        def wunit(w):
            counts['write'] += 1
            return 'WRITE(PU(%s),' % w.group(1)
        text = code_only(text, lambda s: OCTAL.sub(
            lambda o: str(int(o.group(1), 8)), s))
        text = code_only(text, lambda s: re.sub(
            r'\bWRITE\s*\(\s*([A-Z][A-Z0-9]*|\d+)\s*,', wunit, s))
        text = exec_calls(text, counts)
        text = fmp_calls(text, counts)
        if text == orig:
            out.extend(stmt)
        else:
            out.extend(cards(label, text.strip()))
        if head and UNITHEAD.match(out[-1] if stmt else ''):
            out.extend(head)
            head = []
    return name, seg, out


def main():
    os.makedirs(OUT, exist_ok=True)
    src = reads(edit(source()))
    counts = dict(integer=0, pad=0, complexdata=0, hollerith=0, litrep=0, splitdq=0, write=0,
                  exec8=0, exec11=0, fmp=0)
    root, segs = [], []
    names = []
    for u in units(src):
        name, seg, lines = convert_unit(u, counts)
        names.append(name)
        (segs if seg else root).extend(lines)
    expect = dict(integer=2, pad=16, complexdata=7, litrep=3, splitdq=21, write=906,
                  exec8=499, exec11=6, fmp=43)
    for k, v in expect.items():
        if counts[k] != v:
            raise SystemExit('%s: %d, expected %d' % (k, counts[k], v))
    assert names == ['MMM', 'MMBCBD', 'MMRI', 'MMRL'] + [
        'MMS' + c for c in 'ABCDEFGHIJKL'], names
    for fn, lines in (('mmmroot.f', root), ('mmmseg.f', segs)):
        for l in lines:
            if not is_comment(l) and len(l) > 72:
                raise SystemExit('card too long: ' + l)
        with open(os.path.join(OUT, fn), 'w', encoding='latin-1',
                  newline='\n') as f:
            f.write('\n'.join(lines) + '\n')
    print('convert: %s' % ', '.join('%s %d' % kv for kv in counts.items()))
