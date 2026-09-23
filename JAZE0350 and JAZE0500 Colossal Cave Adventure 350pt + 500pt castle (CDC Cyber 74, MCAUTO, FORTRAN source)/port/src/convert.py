#!/usr/bin/env python3
"""Turn the MCAUTO Cyber 74 source into source gfortran will take, and the
two databases into the files the port reads.

`ADVENT.txt` is Blackett's Adventure with Gary Palter's wizard and prime-time
machinery, converted for the MCAUTO Cyber 74 by Tony Jarrett and Paul Zemlin in
December 1978.  Every edit below is a whole line and asserts how often it
matches, so a source that is not the one this was written for fails loudly.

Three things need doing:

1. **The characters.**  The files are a transcription in which three
   characters of the site's set came out as something else:

       %   is a colon           READS%          -> READS:
       \\   is a question mark   QUIT NOW\\       -> QUIT NOW?
       ^*  is an exclamation    CAUTION^*       -> CAUTION!

   The substitution is applied inside string constants and comments only, and
   to the databases.  A fourth character cannot be undone: `"` stands for both
   the apostrophe and the double quote (WON"T and SAYS "XYZZY"), so the
   original could not tell them apart either and neither can the port.

2. **The dialect.**  Display code words, `nH` and quoted Hollerith constants,
   the A and R edit descriptors, SHIFT/MASK/OR, octal constants, the random
   access message file (OPENMS/READMS/WRITMS), PFGET, BUFFER IN/OUT and the
   clock all come from src/port/pjaze.f.

3. **Ones' complement.**  `MASKC = -MASKC` in A5TOA1 means "complement the
   mask" on a CDC 6000; here it has to say so.

The wizard's parameter file (=AMAINT on the Cyber) is lost, so this script
writes `amaint.dat` from the defaults POOF itself carries as comments.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original')
OUT = os.path.join(HERE, '..', '.build')

# ------------------------------------------------------------ display code
DCTAB = (':ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/()$= ,.#[]%"_!&'
         "'?<>@" + chr(92) + '^;')
assert len(DCTAB) == 64


def dcl(s):
    """A quoted or nH constant: left justified, blank filled to ten."""
    s = (s + ' ' * 10)[:10]
    v = 0
    for c in s:
        v = v * 64 + DCTAB.index(c)
    return v


#  What the site's character set did to three characters, undone.
SUBS = [('^*', '!'), (chr(92), '?'), ('%', ':')]
#  How often each appears in each file, as a check that nothing else changed.
COUNTS = {'ADVENT.txt': (23, 8, 78),
          '001.1.txt': (141, 41, 17),
          '001.2.txt': (97, 41, 15)}

# ------------------------------------------------------------------ the edits
EDITS = [
    ('      PROGRAM ADVENT(INPUT,OUTPUT,TAPE1,TAPE2,TAPE3)',
     ['      SUBROUTINE ADVENT'], 1),
    # ---- the two databases and the wizard's parameter file
    (' 2000 CALL PFGET(1,"=DATABS1","UN=XSY913","CT=PU","M=R")',
     [" 2000 CALL PPFGET(1,'databs1.txt')"], 1),
    (' 3000 CALL PFGET(1,"=DATABS2","UN=XSY913","CT=PU","M=R")',
     [" 3000 CALL PPFGET(1,'databs2.txt')"], 1),
    (' 5000 CALL PFGET(3,"=AMAINT","UN=XSY913","CT=PU","M=W")',
     [" 5000 CALL PPFGET(3,'amaint.dat')"], 1),
    # ---- the database reader
    (' 1004 READ(1,1005) LOC,(LINES (K),K=1,8)',
     [' 1004 CALL PRDTXT(LOC,LINES)'], 1),
    (' 1005 FORMAT(I3,X,8A10)', ['C1005 FORMAT(I3,X,8A10)'], 1),
    (' 1043 READ(1,1041)KTAB(TABNDX),ATAB(TABNDX)',
     [' 1043 CALL PRDVOC(KTAB(TABNDX),ATAB(TABNDX))'], 1),
    (' 1041 FORMAT(I4,1X,A4,1X)', ['C1041 FORMAT(I4,1X,A4,1X)'], 1),
    #  Two list directed writes: on the Cyber they start at column 1.
    #  (the text has already had ^* turned into ! by then)
    ('      IF(ASCVAR.GT.1040)PRINT*,"IT WILL BLOW!!!"',
     ['      IF(ASCVAR.GT.1040)PRINT 9991',
      '9991  FORMAT(" IT WILL BLOW!!!")'], 1),
    (' 1011 IF(LOC .GT. RTXSIZ) PRINT*,LOC,RTXSIZ',
     [' 1011 IF(LOC .GT. RTXSIZ) PRINT 9992,LOC,RTXSIZ',
      '9992  FORMAT(" ",I3," ",I3," ")'], 1),
    # ---- the message file, printed a word at a time
    ('    5 PRINT 2,(LINES(I),I=1,L)', ['    5 CALL PRTL8(LINES,L)'], 1),
    ('    2 FORMAT(" ",8A10)',
     ['C     was FORMAT(" ",8A10); PRINT 2 alone prints the blank line',
      '    2 FORMAT(" ")'], 1),
    # ---- words printed with A editing
    ('      PRINT 5015,A5OUT', ['      PRINT 5015,PDCA10(A5OUT)'], 1),
    ('      PRINT 5199,A5OUT', ['      PRINT 5199,PDCA10(A5OUT)'], 1),
    ('      PRINT 8002,A5OUT', ['      PRINT 8002,PDCA10(A5OUT)'], 1),
    ('      PRINT 9032,A5OUT', ['      PRINT 9032,PDCA10(A5OUT)'], 1),
    ('      PRINT 100,HASH', ['      PRINT 100,PDCA4(HASH)'], 1),
    ('      PRINT 7785,K,KK', ['      PRINT 7785,K,PDCA2(KK)'], 1),
    ('      PRINT 18,WORD', ['      PRINT 18,PDCA5(WORD)'], 1),
    ('      PRINT 5,(HNAME(JK),JK=1,2)',
     ['      PRINT 5,PDCA10(HNAME(1)),PDCA10(HNAME(2))'], 1),
    ('      PRINT 15,D,T,(HNAME(JK),JK=1,2)',
     ['      PRINT 15,D,PDCA4(T),PDCA10(HNAME(1)),PDCA10(HNAME(2))'], 1),
    ('      PRINT 2,DAY1,DAY2',
     ['      PRINT 2,PDCA4(DAY1),PDCA4(DAY2)'], 1),
    ('      PRINT 1,DAY1,DAY2',
     ['      PRINT 1,PDCA4(DAY1),PDCA4(DAY2)'], 1),
    ('      IF(FIRST)PRINT 16,DAY1,DAY2,FROM,TILL',
     ['      IF(FIRST)PRINT 16,PDCA4(DAY1),PDCA4(DAY2),FROM,TILL'], 1),
    ('   20 IF(FIRST)PRINT 22,DAY1,DAY2',
     ['   20 IF(FIRST)PRINT 22,PDCA4(DAY1),PDCA4(DAY2)'], 1),
    # ---- the terminal
    (' 1    READ 100,FRST', [' 1    CALL PRDR1(FRST,20)'], 1),
    (' 100  FORMAT(20R1)', ['C100  FORMAT(20R1)'], 1),
    ('      READ 1,HBEGIN', ['      CALL PRDINT(HBEGIN,2)'], 1),
    ('      READ 1,HEND', ['      CALL PRDINT(HEND,2)'], 1),
    ('      READ 1,X', ['      CALL PRDINT(X,2)'], 2),
    ('      READ 2,(HNAME(JK),JK=1,2)', ['      CALL PRDA10(HNAME,2)'], 1),
    ('      READ 3,FROM', ['      CALL PRDINT(FROM,2)'], 1),
    ('      READ 3,TILL', ['      CALL PRDINT(TILL,2)'], 1),
    # ---- the wizard's parameter file, read and written a word at a time
    ('      REWIND 3', ['C     REWIND 3'], 2),
    ('      BUFFER OUT(3,1)(JJ(1),JJ(11))', ['      CALL PBUFOT(3,JJ,11)'], 1),
    ('      BUFFER IN(3,1)(JJ(1),JJ(11))', ['      CALL PBUFIN(3,JJ,11)'], 1),
    ('      IF (UNIT(3)) 11,13,13', ['      IF (PUNIT(3)) 11,13,13'], 1),
    ('      IF(UNIT(3)) 5,10,10', ['      IF(PUNIT(3)) 5,10,10'], 1),
    #  FTN never short circuits, so PCT and RAN - which draw from the
    #  generator - are called even when the rest of the condition has already
    #  decided the answer.  That was measured on NOS 1.3 (tests\cyber\
    #  probe3.job in the ACCA port); gfortran short circuits at -O2 and not
    #  at -O0, so each draw is hoisted out into a statement of its own and
    #  always happens, as it did on the Cyber.
    ('      IF(LOC.LT.15.OR.PCT(95))GOTO 2000',
     ['      KPCT=0', '      IF(PCT(95))KPCT=1',
      '      IF(LOC.LT.15.OR.KPCT.NE.0)GOTO 2000'], 1),
    #  6001 ends a DO loop (and nothing jumps to it), so the label stays
    #  on the last statement; where a label is jumped to, as 14 is, it goes
    #  on the first, so that the jump still draws.
    (' 6001 IF(PCT(50).AND.SAVED.EQ.-1)DLOC(J)=0',
     ['      KPCT=0', '      IF(PCT(50))KPCT=1',
      ' 6001 IF(KPCT.NE.0.AND.SAVED.EQ.-1)DLOC(J)=0'], 1),
    ('      IF(ODLOC(6).NE.DLOC(6).AND.PCT(20))CALL RSPEAK(127)',
     ['      KPCT=0', '      IF(PCT(20))KPCT=1',
      '      IF(ODLOC(6).NE.DLOC(6).AND.KPCT.NE.0)CALL RSPEAK(127)'], 1),
    ('      IF(WZDARK.AND.PCT(35))GOTO 90',
     ['      KPCT=0', '      IF(PCT(35))KPCT=1',
      '      IF(WZDARK.AND.KPCT.NE.0)GOTO 90'], 1),
    ('      IF(LOC.EQ.33.AND.PCT(25).AND..NOT.CLOSNG)CALL RSPEAK(8)',
     ['      KPCT=0', '      IF(PCT(25))KPCT=1',
      '      IF(LOC.EQ.33.AND.KPCT.NE.0.AND..NOT.CLOSNG)CALL RSPEAK(8)'], 1),
    ('      IF(.NOT.(LOC.EQ.36.AND.PCT(30).AND.STFLAG.EQ.0.AND.DATABS.GT.1))',
     ['      KPCT=0', '      IF(PCT(30))KPCT=1',
      '      IF(.NOT.(LOC.EQ.36.AND.KPCT.NE.0.AND.STFLAG.EQ.0',
      '     1.AND.DATABS.GT.1))'], 1),
    ('   14 IF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC))GOTO 12',
     ['   14 KPCT=0', '      IF(PCT(NEWLOC))KPCT=1',
      '      IF(NEWLOC.NE.0.AND.KPCT.EQ.0)GOTO 12'], 1),
    ('      IF(RAN(3).EQ.0.OR.SAVED.NE.-1)GOTO 9175',
     ['      KPCT=RAN(3)',
      '      IF(KPCT.EQ.0.OR.SAVED.NE.-1)GOTO 9175'], 1),
    # ---- .AND. and .OR. used on words rather than on logicals.  gfortran
    #      rejects those, so the compiler finds every one of them.
    ('      BITSET(L,N)=(COND(L).AND.SHIFT(1,N)).NE.0',
     ['      BITSET(L,N)=IAND(COND(L),SHIFT(1,N)).NE.0'], 2),
    ('      DD = AA.AND.BB', ['      DD = IAND(AA,BB)'], 1),
    ('      MASKC = MASKC.AND.MASK1', ['      MASKC = IAND(MASKC,MASK1)'], 1),
    ('      EE = MASKC.AND.DD', ['      EE = IAND(MASKC,DD)'], 1),
    ('      CHARS = (MASKC.AND.CC).OR.DD',
     ['      CHARS = IOR(IAND(MASKC,CC),DD)'], 1),
    ('      PTIME=(PRIMTM.AND.SHIFT(1,T/60)).NE.0',
     ['      PTIME=IAND(PRIMTM,SHIFT(1,T/60)).NE.0'], 1),
    ('      IF((H.AND.SHIFT(1,FROM)).NE.0)GOTO 10',
     ['      IF(IAND(H,SHIFT(1,FROM)).NE.0)GOTO 10'], 1),
    ('      IF((H.AND.SHIFT(1,TILL)).EQ.0.AND.TILL.NE.24)GOTO 14',
     ['      IF(IAND(H,SHIFT(1,TILL)).EQ.0.AND.TILL.NE.24)GOTO 14'], 1),
    ('    5 NEWHRX=(NEWHRX.OR.SHIFT(1,I))',
     ['    5 NEWHRX=IOR(NEWHRX,SHIFT(1,I))'], 1),
    ('      I=SHIFT(J,42).AND.77777700000000000000B',
     ['      I=IAND(SHIFT(J,42),77777700000000000000B)'], 1),
    ('      I=SHIFT(J,6).AND.MASK1', ['      I=IAND(SHIFT(J,6),MASK1)'], 1),
    ('      I=SHIFT(J,24).AND.MASK1', ['      I=IAND(SHIFT(J,24),MASK1)'], 1),
    ('      I=SHIFT(J,42).AND.MASK1', ['      I=IAND(SHIFT(J,42),MASK1)'], 1),
    (' 10   WORD1=(SHIFT(WORD1,6).AND.MASK).OR.FRST(K1+I-1)',
     [' 10   WORD1=IOR(IAND(SHIFT(WORD1,6),MASK),FRST(K1+I-1))'], 1),
    (' 15   WORD1X=(SHIFT(WORD1X,6).AND.MASK).OR.FRST(4+K1+I-1)',
     [' 15   WORD1X=IOR(IAND(SHIFT(WORD1X,6),MASK),FRST(4+K1+I-1))'], 1),
    # ---- DECODE picks the date and time apart, character by character
    ('      DECODE(3,1,I)D', ['      CALL PDECI(I,3,D)'], 1),
    ('      DECODE(2,2,I)HOURS', ['      CALL PDECI(I,2,HOURS)'], 1),
    ('      DECODE(2,2,I)T', ['      CALL PDECI(I,2,T)'], 1),
    ('      DECODE(2,2,I)SECOND', ['      CALL PDECI(I,2,SECOND)'], 1),
    # ---- ones' complement
    ('      MASKC = -MASKC',
     ['C     MASKC = -MASKC - unary minus complements a word on a CDC 6000',
      '      MASKC = PNOT(MASKC)'], 1),
]

#  Declarations the port's own functions need, after a unit's IMPLICIT.
DECLS = {
    'INIT': ['      EXTERNAL RAN'],
    'MAIN': ['      CHARACTER*10 PDCA10', '      CHARACTER*2 PDCA2',
             '      EXTERNAL RAN'],
    'VOCAB': ['      CHARACTER*4 PDCA4'],
    'WIZARD': ['      CHARACTER*5 PDCA5'],
    'HOURS': ['      CHARACTER*4 PDCA4', '      CHARACTER*10 PDCA10'],
    'HOURSX': ['      CHARACTER*4 PDCA4'],
    'NEWHRX': ['      CHARACTER*4 PDCA4'],
}

HEADER = re.compile(r'^ {6,}(?:(?:INTEGER|LOGICAL|REAL) +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION) +([A-Z][A-Z0-9]*)')
HOLL = re.compile(r'(?<![A-Z0-9])(\d+)([RH])')
STRING = re.compile(r'"[^"]*"')
NHOLL = 0                       # counted below and reported


def statement(line):
    return line[:1] not in ('C', 'c', '*', '$') and line.strip() != ''


def subst(text, name):
    """Undo the site's character set inside strings and comments."""
    got = tuple(text.count(a) for a, b in SUBS)
    if name in COUNTS and got != COUNTS[name]:
        sys.exit('convert: %s has %s of ^* \\ %%, expected %s'
                 % (name, got, COUNTS[name]))
    out = []
    for line in text.split('\n'):
        if not statement(line):
            for a, b in SUBS:
                line = line.replace(a, b)
        else:
            def fix(m):
                s = m.group(0)
                for a, b in SUBS:
                    s = s.replace(a, b)
                return s
            line = STRING.sub(fix, line)
        out.append(line)
    return '\n'.join(out)


def main():
    src = open(os.path.join(ORIG, 'ADVENT.txt'),
               encoding='latin-1').read().replace('\r\n', '\n')
    src = subst(src, 'ADVENT.txt')
    #  Only comments run past column 72 in this listing; no statement does,
    #  which is why the generated source can be longer than 72 columns.
    for i, l in enumerate(src.split('\n'), 1):
        if statement(l) and l[72:].strip():
            sys.exit('convert: line %d has code past column 72: %r' % (i, l))
    lines = [l.rstrip() for l in src.split('\n')]
    while lines and not lines[-1]:
        lines.pop()
    #  The first two cards are the file's own name, not FORTRAN.
    while lines and lines[0].strip() == 'ADVENT':
        lines.pop(0)
    text = '\n'.join(lines) + '\n'

    for old, new, count in EDITS:
        pat = old + '\n'
        got = text.count(pat)
        if got != count:
            sys.exit('convert: %r matched %d times, expected %d'
                     % (old, got, count))
        text = text.replace(pat, '\n'.join(new) + '\n')

    #  Quoted and nH constants become calls that pack display code.  A quoted
    #  constant is left justified and blank filled, like nH; nR is right
    #  justified.
    #  A FORMAT keeps its strings, and so do the continuation lines of one;
    #  everywhere else the first six columns (label and continuation mark)
    #  are left alone, or a "2H" continuation would look like a constant.
    out, nholl, informat = [], 0, False
    for line in text.split('\n'):
        if not statement(line):
            out.append(line)
            continue
        cont = len(line) > 5 and line[5] not in (' ', '0')
        if not cont:
            informat = bool(re.match(r'^ *[0-9 ]* *FORMAT', line))
        if not informat:
            head, body = line[:6], line[6:]
            for m in reversed(list(HOLL.finditer(body))):
                k = int(m.group(1))
                s = (body[m.end():] + ' ' * k)[:k]
                fn = 'PDCW' if m.group(2) == 'R' else 'PDCL'
                body = (body[:m.start()] + "%s('%s')" % (fn, s)
                        + body[m.end() + k:])
                nholl += 1
            for m in reversed(list(STRING.finditer(body))):
                s = m.group(0)[1:-1]
                if "'" in s:
                    sys.exit('convert: apostrophe in %r' % line)
                body = body[:m.start()] + "PDCL('%s')" % s + body[m.end():]
                nholl += 1
            line = head + body
            #  Packing a constant into a call makes the line longer, so the
            #  generated source is compiled -ffixed-line-length-132.
            if len(line) > 132:
                sys.exit('convert: line too long: %r' % line)
        out.append(line)
    text = '\n'.join(out)
    print('convert: %d Hollerith constants packed' % nholl)

    #  Octal constants become decimal.
    def octal(m):
        return str(int(m.group(1), 8))
    text, noct = re.subn(r'(?<![A-Z0-9])([0-7]+)B(?![A-Z0-9])', octal, text)
    print('convert: %d octal constants' % noct)

    #  Nothing the Cyber's compiler understood and gfortran does not may be
    #  left outside a FORMAT (which keeps its own text).
    informat = False
    for i, line in enumerate(text.split('\n'), 1):
        if not statement(line):
            continue
        cont = len(line) > 5 and line[5] not in (' ', '0')
        if not cont:
            informat = bool(re.match(r'^ *[0-9 ]* *FORMAT', line))
        if informat:
            continue
        for bad, what in ((r'"', 'quote'), (r'\bBUFFER\b', 'BUFFER'),
                          (r'(?<![A-Z0-9])[0-7]+B(?![A-Z0-9])', 'octal'),
                          (r'\d+[RH]\w', 'nR/nH')):
            if re.search(bad, line[6:]):
                sys.exit('convert: %s left at line %d: %r' % (what, i, line))

    #  Per unit declarations.
    lines = text.split('\n')
    hasimp, unit = {}, None
    for line in lines:
        m = HEADER.match(line)
        if m:
            unit = m.group(1)
            hasimp[unit] = False
        elif unit and line.strip().startswith('IMPLICIT INTEGER'):
            hasimp[unit] = True
    out, unit, done = [], None, set()
    for line in lines:
        m = HEADER.match(line)
        if m:
            unit = m.group(1)
            out.append(line)
            if unit in DECLS and not hasimp.get(unit):
                out.extend(DECLS[unit])
                done.add(unit)
            continue
        out.append(line)
        if (unit in DECLS and unit not in done
                and line.strip().startswith('IMPLICIT INTEGER')):
            out.extend(DECLS[unit])
            done.add(unit)
    missing = set(DECLS) - done
    if missing:
        sys.exit('convert: nowhere to declare %s' % ', '.join(sorted(missing)))
    text = '\n'.join(out)

    outsrc = os.path.join(OUT, 'src')
    if not os.path.isdir(outsrc):
        os.makedirs(outsrc)
    with open(os.path.join(outsrc, 'advent.f'), 'w', newline='\n') as f:
        f.write(text)

    #  The two databases, with the same three characters put back.
    for name, out_name in (('001.2.txt', 'databs1.txt'),
                           ('001.1.txt', 'databs2.txt')):
        d = open(os.path.join(ORIG, name), encoding='latin-1').read()
        d = d.replace('\r\n', '\n')
        got = tuple(d.count(a) for a, b in SUBS)
        if got != COUNTS[name]:
            sys.exit('convert: %s has %s, expected %s'
                     % (name, got, COUNTS[name]))
        for a, b in SUBS:
            d = d.replace(a, b)
        with open(os.path.join(OUT, out_name), 'w', newline='\n',
                  encoding='latin-1') as f:
            f.write(d)

    #  =AMAINT is lost.  POOF carries the values it used to hold as comments:
    #    WKDAY="00777400" WKEND=0 HOLID=0 HBEGIN=0 HEND=-1
    #    SHORT=30 MAGIC="DWAR" MAGNM=11111 LATNCY=90
    #  ("00777400" is octal in the PDP-10 dialect this came from - the other
    #  ports of this program have 261888, the same number.)
    import struct
    jj = [0o777400, 0, 0, 0, -1, 30, dcl('DWAR'), 11111, 90,
          dcl(' '), dcl(' ')]
    with open(os.path.join(OUT, 'amaint.dat'), 'wb') as f:
        for v in jj:
            f.write(struct.pack('<q', v))
    print('convert: advent.f, %d lines; two databases; amaint.dat'
          % len(text.split('\n')))


if __name__ == '__main__':
    main()
