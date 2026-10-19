#!/usr/bin/env python3
"""Turn the CDC FORTRAN Extended source into source gfortran will take.

`adventure.src` is one deck: three overlays (ADVENT, INIT, MAIN) and the
subroutines, columns 1-72 with a sequence number in 73-80.  Nothing here is
rewritten by hand - every edit below is a whole line that has to match the
number of times stated, so a source that is not the one this was written for
fails loudly instead of quietly.

What has to change, and nothing else:

  * the overlays become subroutines of one program;
  * FORTRAN Extended's `nR`, `nL` and `nH` constants and its `R` and `A`
    edit descriptors become calls that pack and unpack display code, because
    a word here is 64 bits and not 60 and gfortran has no display code;
  * `.AND.`/`.OR.`/`.NOT.` used on integers become IAND/IOR/PNOT - the
    compiler finds any that are missed, so there is no risk of taking one
    for the logical operator it also is in this source;
  * octal constants become decimal;
  * the record manager calls (FILEWA/OPENM/GET/PUT/CLOSEM) become the
    port's word addressable files;
  * TIME, DECODE, RANF, RANSET, SECOND and EOF come from src/port/pcdc.f.

The port's own behaviour is in src/port/pcdc.f; this file only edits.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'src_original', 'adventure.src')
DATA = os.path.join(HERE, '..', '..', 'src_original', 'adventure.txt')
OUT = os.path.join(HERE, '..', '.build', 'src')

# ---------------------------------------------------------------- display code
#  The 64 character set; see pcdc.f.  Used here only to work out what the
#  nR and nH constants in DATA statements come to, since a DATA statement
#  cannot call a function.
DCTAB = (':ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/()$= ,.#[]%"_!&'
         "'?<>@" + chr(92) + '^;')
assert len(DCTAB) == 64


def dcw(s):
    """An nR constant: right justified, binary zero fill."""
    v = 0
    for c in s:
        v = v * 64 + DCTAB.index(c)
    return v


# --------------------------------------------------------------------- the edits
#  (old line, new lines, how many times it must match)
EDITS = [
    # ---- ADVENT: the outer overlay becomes the subroutine the port calls
    ('      OVERLAY (ADVENT,0,0)',
     ['C     OVERLAY (ADVENT,0,0) - the port is one program'], 1),
    ('      PROGRAM ADVENT (INPUT=101B,OUTPUT=101B,TAPE1=101B,TAPE5=INPUT)',
     ['      SUBROUTINE ADVENT'], 1),
    ('      DATA T/9R06.00.00., 9R11.30.00., 9R13.30.00.,',
     ['C  The hours the cave is shut, as display code: 9R06.00.00. and so on',
      '      DATA T/%d,%d,%d,' % (dcw('06.00.00.'), dcw('11.30.00.'),
                                  dcw('13.30.00.'))], 1),
    ('     + 9R15.30.00./', ['     + %d/' % dcw('15.30.00.')], 1),
    ('      DATA BLANK/4R    /,EOFM/4R>$< /',
     ['      DATA BLANK/%d/,EOFM/%d/' % (dcw('    '), dcw('>$< '))], 1),
    # ---- ADVENT: the record manager
    ('      CALL FILEWA(FIT2,3LLFN,5LTAPE2,3LWSA,WSA2,3LMRL,370,2LRT,1LU,',
     ['      CALL PFILEW(FIT2,1,370)'], 1),
    ('     +3LFWB,BUF2,3LBFS,514)',
     ['C     +3LFWB,BUF2,3LBFS,514) - the record manager kept its own buffer'],
     1),
    ('      CALL FILEWA(FIT3,3LLFN,5LTAPE3,3LWSA,WSA3,3LMRL,10,2LRT,1LU,',
     ['      CALL PFILEW(FIT3,2,10)'], 1),
    ('     +3LFWB,BUF3,3LBFS,514)',
     ['C     +3LFWB,BUF3,3LBFS,514)'], 1),
    ('      CALL OPENM(FIT2,6LOUTPUT)', ['      CALL POPENM(FIT2,1)'], 1),
    ('      CALL OPENM(FIT3,6LOUTPUT)', ['      CALL POPENM(FIT3,1)'], 1),
    ('      CALL OPENM(FIT2,5LINPUT)', ['      CALL POPENM(FIT2,0)'], 1),
    ('      CALL OPENM(FIT3,5LINPUT)', ['      CALL POPENM(FIT3,0)'], 1),
    ('      CALL CLOSEM(FIT2)', ['      CALL PCLOSM(FIT2)'], 2),
    ('      CALL CLOSEM(FIT3)', ['      CALL PCLOSM(FIT3)'], 2),
    ('      CALL OVERLAY (6HADVENT,1,0)', ['      CALL INIT'], 1),
    ('      CALL OVERLAY (6HADVENT,2,0)', ['      CALL PMAIN'], 1),
    # ---- TIMER: the wizard's question, and the clock
    ('      REAL TIME, WHEN',
     ['      REAL WHEN'], 1),
    ('101   READ (5, 105) FRST', ['101   CALL PRDA(FRST,2)'], 1),
    ('  105 FORMAT(A2)', ['C 105 FORMAT(A2)'], 1),
    ('111   READ (5, 115) WORD', ['111   CALL PRDA(WORD,10)'], 1),
    ('  115 FORMAT(A10)', ['C 115 FORMAT(A10)'], 1),
    ('      IF(EOF(5))101,102',
     ['      IF(EOF(5).NE.0)GOTO 101', '      GOTO 102'], 1),
    ('      IF(EOF(5)) 111,112',
     ['      IF(EOF(5).NE.0)GOTO 111', '      GOTO 112'], 1),
    # -u (PUNLIM): the clock is off, as for the wizard, so neither the
    # hours here nor the watch in MAIN stop the game
    ('   30 WIZSW = .FALSE.',
     ['   30 WIZSW = PUNLIM()', '      IF(WIZSW) RETURN'], 1),
    ('      WHEN = TIME(1.)', ['      NOW = PTIMEW()'], 2),
    ('      DECODE (10,5,WHEN) NOW', ['C     DECODE (10,5,WHEN) NOW'], 1),
    ('    5 FORMAT(A10)', ['C   5 FORMAT(A10)'], 1),
    ('      NOW = NOW .AND. .NOT.MASK(6)',
     ['      NOW = IAND(NOW,PNOT(MASK(6)))'], 2),
    ('   20 PRINT 10, T',
     ['   20 PRINT 10, PDC9(T(1)),PDC9(T(2)),PDC9(T(3)),PDC9(T(4))'], 1),
    ('     +" FOLLOWING HOURS :",/,1XR9," TO ",R9,/,1X,R9," TO ",R9)',
     ['     +" FOLLOWING HOURS :",/,1X,A9," TO ",A9,/,1X,A9," TO ",A9)'], 1),
    # ---- VOCAB
    ('      PRINT 100,ID1', ['      PRINT 100,PDC4(ID1)'], 1),
    ('  100 FORMAT(" KEYWORD = ",R4)',
     ['  100 FORMAT(" KEYWORD = ",A4)'], 1),
    # ---- RND: FORTRAN Extended knew the type of its own library functions
    ('      REAL RAN',
     ['      DOUBLE PRECISION RAN,RANF,SECOND',
      '      EXTERNAL SECOND'], 1),
    # ---- SPEAK, PSPEAK: the message file
    ('      CALL GET(FIT3,WSA3,N,0,0,10)',
     ['      CALL PGET(FIT3,WSA3,N,10)'], 1),
    ('      CALL GET(FIT3,WSA3,N1,0,0,10)',
     ['      CALL PGET(FIT3,WSA3,N1,10)'], 1),
    ('    1 CALL GET(FIT3,WSA3,M,0,0,10)',
     ['    1 CALL PGET(FIT3,WSA3,M,10)'], 1),
    ('      CALL GET(FIT2,WSA2,WRDSUM,0,0,WASIZ10)',
     ['      CALL PGET(FIT2,WSA2,WRDSUM,WASIZ10)'], 3),
    ('      WASIZ10=SHIFT(WINDEX,-30).AND.7777777777B',
     ['      WASIZ10=IAND(SHIFT(WINDEX,-30),1073741823)'], 3),
    ('      WRDSUM=WINDEX.AND.7777777777B',
     ['      WRDSUM=IAND(WINDEX,1073741823)'], 3),
    ('5      PRINT 2,(LINES(I),I=1,L)', ['5     CALL PRTL(LINES,L)'], 1),
    ('2     FORMAT(1X,18R4)',
     ['C     was FORMAT(1X,18R4); PRINT 2 on its own prints the blank line',
      '2     FORMAT(1X)'], 1),
    # ---- GETIN and STUFF: the command line, packed into words
    ('      DATA BLANK/1R /', ['      DATA BLANK/45/'], 1),
    ('    7 FRST(III)=55B', ['    7 FRST(III)=45'], 1),
    ('    2 READ (5,3) FRST', ['    2 CALL PRDR1(FRST,20)'], 1),
    ('3     FORMAT(20R1)', ['C3    FORMAT(20R1)'], 1),
    ('      IF(EOF(5))2,4',
     ['      IF(EOF(5).NE.0)GOTO 2', '      GOTO 4'], 1),
    ('      WORD2=SHIFT((WORDFUL.AND.MASK(24)),24)',
     ['      WORD2=SHIFT(IAND(WORDFUL,MASK(24)),24)'], 1),
    ('      DATA MASK1 /77B/', ['      DATA MASK1 /63/'], 1),
    ('      SOURCE(I)=SOURCE(I).AND.MASK1',
     ['      SOURCE(I)=IAND(SOURCE(I),MASK1)'], 1),
    ('      STUFF=SHIFT(STUFF,6).OR.SOURCE(I)',
     ['      STUFF=IOR(SHIFT(STUFF,6),SOURCE(I))'], 1),
    ('      STUFF=SHIFT(STUFF,6).OR.55B',
     ['      STUFF=IOR(SHIFT(STUFF,6),45)'], 1),
    # ---- YESX
    ('      READ (5,3) REPLY', ['      CALL PRDR1(REPLY,1)'], 1),
    ('    3 FORMAT(R1)', ['C   3 FORMAT(R1)'], 1),
    ('      IF(EOF(5)) 8,7',
     ['      IF(EOF(5).NE.0)GOTO 8', '      GOTO 7'], 1),
    # ---- ISHFT, the program's own (and gfortran's intrinsic name).  It
    #      masked its own argument in place, so ISHFT(1,N) wrote 1 back over
    #      the constant 1 - harmless on the CDC, a segmentation fault here,
    #      so the mask goes into a local instead.
    ('      VAR=VAR.AND.177777B', ['      V=IAND(VAR,65535)'], 1),
    ('      ISHFT=VAR', ['      ISHFT=V'], 1),
    ('      ISHFT=SHIFT(VAR,COUNT)', ['      ISHFT=SHIFT(V,COUNT)'], 1),
    ('    1 TSHFT=SHIFT(VAR,-1).AND.37777777777777777777B',
     ['    1 TSHFT=IAND(SHIFT(V,-1),576460752303423487)'], 1),
    # ---- INIT: the database reader
    ('      OVERLAY (1,0)', ['C     OVERLAY (1,0)'], 1),
    ('      PROGRAM INIT', ['      SUBROUTINE INIT'], 1),
    ('      BITSET(L,N)=(COND(L).AND.ISHFT(1,N)).NE.0',
     ['      BITSET(L,N)=IAND(COND(L),ISHFT(1,N)).NE.0'], 2),
    ('1004  READ(1,1005) LOC,LINES', ['1004  CALL PRDTXT(LOC,LINES)'], 1),
    ('1005  FORMAT(I4,18R4)', ['C1005 FORMAT(I4,18R4)'], 1),
    ('      CALL PUT(FIT2,WSA2,WASIZ10,WRDSUM)',
     ['      CALL PPUT(FIT2,WSA2,WASIZ10,WRDSUM)'], 1),
    ('      WINDEX=SHIFT((WASIZ10.AND.7777777777B),30)',
     ['      WINDEX=SHIFT(IAND(WASIZ10,1073741823),30)'], 1),
    ('      WINDEX=WINDEX.OR.(WRDSUM.AND.7777777777B)',
     ['      WINDEX=IOR(WINDEX,IAND(WRDSUM,1073741823))'], 1),
    ('      CALL PUT(FIT3,WSA3,10,ASCVAR)',
     ['      CALL PPUT(FIT3,WSA3,10,ASCVAR)'], 1),
    (' 1043 READ(1,1041)KTAB(TABNDX),ATAB(TABNDX)',
     [' 1043 CALL PRDVOC(KTAB(TABNDX),ATAB(TABNDX))'], 1),
    (' 1041 FORMAT(I6,R4)', ['C1041 FORMAT(I6,R4)'], 1),
    # ---- MAIN
    ('      OVERLAY (2,0)', ['C     OVERLAY (2,0)'], 1),
    ('      PROGRAM MAIN', ['      SUBROUTINE PMAIN'], 1),
    #  FTN never short circuits: PCT (which draws from the generator) is
    #  called even when the rest of the condition has already decided the
    #  answer - measured on NOS 1.3, all six shapes, in tests\cyber\
    #  probe3.job.  gfortran short circuits at -O2 and not at -O0, so the
    #  draw is hoisted out and always happens, as on the Cyber.  Without
    #  this the dwarves turn up at different times.
    ('      IF(LOC.LT.15.OR.PCT(80))GOTO 2000',
     ['      KPCT=0', '      IF(PCT(80))KPCT=1',
      '      IF(LOC.LT.15.OR.KPCT.NE.0)GOTO 2000'], 1),
    ('      IF(ODLOC(6).NE.DLOC(6).AND.PCT(80))CALL RSPEAK(127)',
     ['      KPCT=0', '      IF(PCT(80))KPCT=1',
      '      IF(ODLOC(6).NE.DLOC(6).AND.KPCT.NE.0)CALL RSPEAK(127)'], 1),
    ('      IF(WZDARK.AND.PCT(35))GOTO 90',
     ['      KPCT=0', '      IF(PCT(35))KPCT=1',
      '      IF(WZDARK.AND.KPCT.NE.0)GOTO 90'], 1),
    ('      IF(LOC.EQ.33.AND.PCT(25).AND..NOT.CLOSNG)CALL RSPEAK(8)',
     ['      KPCT=0', '      IF(PCT(25))KPCT=1',
      '      IF(LOC.EQ.33.AND.KPCT.NE.0.AND..NOT.CLOSNG)CALL RSPEAK(8)'], 1),
    (' 2610 IF(WD1.EQ.4RWEST.AND.PCT(10))',
     [' 2610 KPCT=0', '      IF(PCT(10))KPCT=1',
      '      IF(WD1.EQ.4RWEST.AND.KPCT.NE.0)'], 1),
    ('14    IF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC))GOTO 12',
     ['14    KPCT=0', '      IF(PCT(NEWLOC))KPCT=1',
      '      IF(NEWLOC.NE.0.AND.KPCT.EQ.0)GOTO 12'], 1),
    ('      DECODE (10,2100,WHEN) NOW',
     ['C     DECODE (10,2100,WHEN) NOW'], 1),
    (' 2100 FORMAT(A10)', ['C2100 FORMAT(A10)'], 1),
    (' 2611 PRINT 2110,NOW', [' 2611 PRINT 2110,PDC9(NOW)'], 1),
    ('     +1X,R9," WHEN...")', ['     +1X,A9," WHEN...")'], 1),
    ('      PRINT 5015,WDFULL', ['      PRINT 5015,PDC10(WDFULL)'], 1),
    (' 5015 FORMAT(" WHAT DO YOU WANT TO DO WITH THE ",R10)',
     [' 5015 FORMAT(" WHAT DO YOU WANT TO DO WITH THE ",A10)'], 1),
    ('      PRINT 5199,WDFULL', ['      PRINT 5199,PDC10(WDFULL)'], 1),
    (" 5199 FORMAT(\" I DON'T SEE ANY \",R10)",
     [" 5199 FORMAT(\" I DON'T SEE ANY \",A10)"], 1),
    (' 8000 PRINT 8002,WDFULL', [' 8000 PRINT 8002,PDC10(WDFULL)'], 1),
    (" 8002 FORMAT(\" I DON'T UNDERSTAND \",R10)",
     [" 8002 FORMAT(\" I DON'T UNDERSTAND \",A10)"], 1),
    ('      PRINT 9032,WDFULL', ['      PRINT 9032,PDC10(WDFULL)'], 1),
    (' 9032 FORMAT(" OKAY, ",R10)', [' 9032 FORMAT(" OKAY, ",A10)'], 1),
    ('      PRINT 8315,T',
     ['      PRINT 8315,PDC9(T(1)),PDC9(T(2)),PDC9(T(3)),PDC9(T(4))'], 1),
    (' 8315 FORMAT(1X,R9," TO ",R9,/,1X,R9," TO ",R9)',
     [' 8315 FORMAT(1X,A9," TO ",A9,/,1X,A9," TO ",A9)'], 1),
]

#  Declarations the port's own functions need, inserted after a unit's
#  IMPLICIT statement (or after its header if it has none - EXITADV).
DECLS = {
    'TIMER': ['      CHARACTER*9 PDC9', '      LOGICAL PUNLIM'],
    'VOCAB': ['      CHARACTER*4 PDC4'],
    'EXITADV': ['      IMPLICIT INTEGER (A-Z)'],
    'INIT': ['      EXTERNAL ISHFT'],
    'PMAIN': ['      CHARACTER*9 PDC9', '      CHARACTER*10 PDC10',
             '      EXTERNAL ISHFT'],
}

HEADER = re.compile(r'^ {6,}(?:(?:INTEGER|LOGICAL|REAL) +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION) +([A-Z][A-Z0-9]*)')
HOLL = re.compile(r'(?<![A-Z0-9])(\d+)([RH])')
NHOLL = 62                      # how many are left once the edits are done


def statement(line):
    """True for a line that carries statement text (not a comment)."""
    return line[:1] not in ('C', 'c', '*', '$') and line.strip() != ''


def main():
    raw = open(SRC, encoding='latin-1').read().replace('\r\n', '\n')
    # Columns 73-80 are the sequence number, not statement text.
    lines = [l[:72].rstrip() for l in raw.split('\n')]
    while lines and not lines[-1]:
        lines.pop()
    text = '\n'.join(lines) + '\n'

    for old, new, count in EDITS:
        pat = old + '\n'
        got = text.count(pat)
        if got != count:
            sys.exit('convert: %r matched %d times, expected %d'
                     % (old, got, count))
        text = text.replace(pat, '\n'.join(new) + '\n')

    # The nR and nH constants that are left are all in executable statements:
    # rebuilt right to left so the offsets stay good.
    out, nholl = [], 0
    for line in text.split('\n'):
        if statement(line) and not re.match(r'^ *[0-9 ]* *FORMAT', line):
            for m in reversed(list(HOLL.finditer(line))):
                k = int(m.group(1))
                s = line[m.end():m.end() + k]
                if len(s) != k:
                    sys.exit('convert: short constant in %r' % line)
                fn = 'PDCW' if m.group(2) == 'R' else 'PDCL'
                line = (line[:m.start()] + "%s('%s')" % (fn, s)
                        + line[m.end() + k:])
                nholl += 1
            if len(line) > 72:
                sys.exit('convert: line too long after packing: %r' % line)
        out.append(line)
    if nholl != NHOLL:
        sys.exit('convert: packed %d nR/nH constants, expected %d'
                 % (nholl, NHOLL))
    text = '\n'.join(out)

    # Nothing may be left that gfortran would not understand.
    checks = ((r'(?<![A-Z0-9])[0-7]+B(?![A-Z0-9])', 'octal constant'),
              (r'\bDECODE\b', 'DECODE'), (r'\bENCODE\b', 'ENCODE'),
              (r'\bOVERLAY\b', 'OVERLAY'), (r'\d+[RH]\w', 'nR/nH constant'))
    for i, line in enumerate(text.split('\n'), 1):
        if not statement(line) or re.match(r'^ *[0-9 ]* *FORMAT', line):
            continue
        for bad, what in checks:
            if re.search(bad, line):
                sys.exit('convert: %s left at line %d: %r' % (what, i, line))

    # The declarations the port's own functions need, per unit: after the
    # unit's IMPLICIT statement, or after its header if it has none.
    lines = text.split('\n')
    hasimp = {}
    unit = None
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
            if unit in DECLS and not hasimp[unit]:
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
        sys.exit('convert: no place found for the declarations of %s'
                 % ', '.join(sorted(missing)))
    text = '\n'.join(out)

    if not os.path.isdir(OUT):
        os.makedirs(OUT)
    with open(os.path.join(OUT, 'advent.f'), 'w', newline='\n') as f:
        f.write(text)
    # TAPE1, the database, goes with the game exactly as it is on the tape.
    with open(DATA, 'rb') as f:
        db = f.read()
    with open(os.path.join(OUT, '..', 'adventure.txt'), 'wb') as f:
        f.write(db)
    print('convert: advent.f, %d lines; adventure.txt, %d bytes'
          % (len(text.split('\n')), len(db)))


if __name__ == '__main__':
    main()
