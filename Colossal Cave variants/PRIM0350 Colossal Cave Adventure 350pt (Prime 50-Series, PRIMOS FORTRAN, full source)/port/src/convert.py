#!/usr/bin/env python3
"""Turn the PRIMOS FORTRAN source into sources gfortran will take, one file
per program unit.

`ADVENTURE.FTN` is Gary Palter's portable Adventure with the Prime's own
touches (mixed-case output through ULCASE, a wizard's password built from the
day of the week); `ADVSUB.FTN` is the site-supplied half the conversion guide
asks for. The units listed in REPLACED are not compiled from the source at
all - `src\\port\\` has the port's versions - and everything else is copied
through with the edits below, each of which asserts how many times it matches.

Nothing here is guesswork about the Prime: the emulator still runs PRIMOS
23.4, so RAND$A, MCHR$A and the clock were measured on it (see ..\\README.md).
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original', 'ADVENTURE.UFD')
OUT = os.path.join(HERE, '..', '.build', 'src')

# The routines that talked to PRIMOS.  ULCASE, WIZARD, NEWHRX, HOURSX, SHIFT
# and RAN stay - they are the game's own behaviour - with the system calls
# inside them replaced.
REPLACED = 'DATIME IOINIT ADDR SIZE LDCOMN SVCOMN LEGAL GETTO PACK'.split()

HEADER = re.compile(r'^ {6,}(?:(?:INTEGER|LOGICAL|REAL)(?:\s*\*\s*\d)? +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION) +([A-Z][A-Z0-9$]*)')


def source(name):
    """One source file, with the compiler's listing directives dropped."""
    raw = open(os.path.join(ORIG, name), 'rb').read().decode('latin-1')
    out = []
    for line in raw.split('\n'):
        line = line.rstrip()
        if re.match(r'^\$[0-9]', line):          # $1: start a listing page
            continue
        if line.startswith('$INSERT'):           # SYSCOM>KEYS.F etc.
            continue
        # Columns 73 on are not statement text, and the source has trailing
        # /* comments out there that the compiler never saw.  What is left of
        # a /* comment becomes gfortran's own trailing comment form.
        if line[:1] not in ('C', 'c', '*'):
            line = line[:72].rstrip().replace('/*', '!')
        out.append(line)
    return out


def units():
    """(name, lines) for every program unit in the two source files."""
    got, cur, name = [], None, None
    for f in ('ADVENTURE.FTN.txt', 'ADVSUB.FTN.txt'):
        for line in source(f):
            m = HEADER.match(line)
            if m:
                if cur is not None:
                    sys.exit('convert: %s has no END' % name)
                cur, name = [line], m.group(1)
                continue
            if cur is None:
                continue
            cur.append(line)
            if line.strip() == 'END':
                got.append((name, cur))
                cur = None
        if cur is not None:
            sys.exit('convert: %s has no END' % name)
    return got


def data_list(name, values, cont='     1     '):
    out, line = [], '      DATA ' + name + '/'
    for i, v in enumerate(values):
        item = str(v) + ('/' if i == len(values) - 1 else ',')
        if len(line) + len(item) > 71:
            out.append(line)
            line = cont
        line += item
    out.append(line)
    return '\n'.join(out)


PORT = 'C  PORT: '
FIXUPS = {}


def fix(unit, count, old, new):
    FIXUPS.setdefault(unit, []).append((count, old, new))


def chrset(text):
    """The 64-character table as this source spells it: mostly 1Hx, but the
    comma and the backslash as quoted strings, and both of the last two slots
    an underbar."""
    m = re.search(r'      DATA CHRSET/.*?\n     8\s*/\n', text, re.S)
    if not m:
        sys.exit('convert: cannot find a CHRSET table')
    body = m.group(0)
    vals = [ord(a or b) for a, b in re.findall(r"1H(.)|'(.)'", body)]
    if len(vals) != 64:
        sys.exit('convert: CHRSET has %d entries' % len(vals))
    return body, data_list('CHRSET', vals) + '\n'


# ---------------------------------------------------------------- the edits

# 2. MAIN
fix('MAIN', 1, "      CALL GETTO",
    PORT + """GETTO ATTACHED TO THE ADVCOM SUB-DIRECTORY, WHERE THE GAME
C  KEPT ITS DATABASE AND ITS SAVED COMMON BLOCKS; HERE THEY SIT BESIDE THE
C  PROGRAM AND IOINIT OPENS THEM.
C     CALL GETTO""")
fix('MAIN', 1, "25000 CALL ATCH$$(K$HOME,0,0,0,0,CODE)",
    PORT + """ATTACHING BACK TO THE HOME DIRECTORY BEFORE EXIT.
25000 CONTINUE""")
fix('MAIN', 1,
    "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),FDUMMY(10)",
    "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),FDUMMY(10)\n"
    + PORT + "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAIN', 1, """ 1004 READ(DBFI,1005)LOC,TEXT,KK
 1005 FORMAT(1I8,70A1,A1)""",
    """ 1004 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1005)LOC
 1005 FORMAT(I8)
      CALL PUNPK(PBUF(9:78),TEXT,70)
      KK=ICHAR(PBUF(79:79))""")
fix('MAIN', 1, """ 1043 READ(DBFI,1041)KTAB(TABNDX),(TEXT(I),I=1,5)
 1041 FORMAT(I8,5A1)""",
    """ 1043 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1041)KTAB(TABNDX)
 1041 FORMAT(I8)
      CALL PUNPK(PBUF(9:13),TEXT,5)""")
fix('MAIN', 1, "      CALL IOINIT(0)",
    PORT + """COMMAND LINE OPTIONS.
      CALL POPTS
      CALL IOINIT(0)""")

#    gfortran has intrinsics called SIZE, RAN and ABORT; here SIZE is one of
#    the routines the conversion guide asks the site to supply.
fix('MAIN', 1, """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT""",
    """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT
""" + PORT + """SIZE AND RAN ARE GFORTRAN INTRINSICS
      EXTERNAL SIZE,RAN
      INTEGER SIZE,RAN""")
for unit in ('PCT', 'WIZARD'):
    fix(unit, 1, "      IMPLICIT INTEGER(A-Z)",
        "      IMPLICIT INTEGER(A-Z)\n" + PORT +
        "RAN IS A GFORTRAN INTRINSIC\n      EXTERNAL RAN\n      INTEGER RAN")

# 3. CODE1: the mask came out of an INTEGER*2 pair so that it could be
#    written in octal, and the five characters out of an integer array packed
#    four to a word.  Every call passes a literal.
fix('CODE1', 1, """      DIMENSION WORDS(2)
      INTEGER *2 IDUMY$(2)
      EQUIVALENCE (IDUMY$,CHRMSK)    ! SO I CAN FILL IT WITH AN OCTAL C
      DIMENSION CHRSET(64)""",
    PORT + """THE ORIGINAL PICKED THE CHARACTERS OUT OF AN INTEGER ARRAY
C  HOLDING THEM FOUR TO A WORD, HIGH-ORDER BYTE FIRST, WITH A MASK BUILT OUT
C  OF AN INTEGER*2 PAIR SO THAT IT COULD BE WRITTEN IN OCTAL.  EVERY CALL IN
C  THE PROGRAM PASSES A FIVE CHARACTER LITERAL, SO TAKE THE LITERAL AS A
C  STRING AND THE BYTE ORDER OF THE HOST STOPS MATTERING.
      CHARACTER*(*) WORDS
      CHARACTER*5 W
      DIMENSION CHRSET(64)""")
fix('CODE1', 1,
    "      DATA NWORDS/2/,NCHARS/4/,CHRSIZ/8/, IDUMY$/:177400,:000000/",
    "      DATA NWORDS/2/,NCHARS/4/,CHRSIZ/8/")
fix('CODE1', 1, """      RESULT=0
      COUNT=0
C
      DO 10 I=1,NWORDS
         WORD=WORDS(I)
C
         DO 5 J=1,NCHARS
            COUNT=COUNT+1
            IF(COUNT.GT.5)GOTO 20
            CHAR=AND(WORD,CHRMSK)
            WORD=SHIFT(WORD,CHRSIZ)
            DO 1 CHRIDX=1,64
               IF(CHAR.EQ.AND(CHRSET(CHRIDX),CHRMSK))GOTO 2
    1       CONTINUE
            CHRIDX=15
    2       RESULT=SHIFT(RESULT,6)+CHRIDX-1
    5    CONTINUE
   10 CONTINUE
C
   20 CODE1=RESULT""",
    """      W=WORDS
      RESULT=0
      DO 10 I=1,5
         CHAR=ICHAR(W(I:I))
         DO 1 CHRIDX=1,64
            IF(CHAR.EQ.CHRSET(CHRIDX))GOTO 2
    1    CONTINUE
         CHRIDX=15
    2    RESULT=SHIFT(RESULT,6)+CHRIDX-1
   10 CONTINUE
   20 CODE1=RESULT""")

# 3b. Characters in words, wherever they are written as literals.
for unit in ('MAIN', 'A5TOA1', 'GETIN', 'SPEAK'):
    fix(unit, 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")
fix('MOTD', 1, "      DATA BLANK/' '/,PERIOD/'.'/",
    "      DATA BLANK/32/,PERIOD/46/")
fix('CVSTB', 1, "      DATA BLANK,MINUS,PLUS/' ','-','+'/",
    "      DATA BLANK,MINUS,PLUS/32,45,43/")
fix('CVSTB', 1, "      DATA DIGITS/1H0,1H1,1H2,1H3,1H4,1H5,1H6,1H7,1H8,1H9/",
    "      DATA DIGITS/48,49,50,51,52,53,54,55,56,57/")
for unit in ('CVLTUC', 'CVUTLC'):
    fix(unit, 1, """      DATA UPPER/1HA,1HB,1HC,1HD,1HE,1HF,1HG,1HH,1HI,1HJ,1HK,1HL,1HM,
     1           1HN,1HO,1HP,1HQ,1HR,1HS,1HT,1HU,1HV,1HW,1HX,1HY,1HZ/,
     2     LOWER/1Ha,1Hb,1Hc,1Hd,1He,1Hf,1Hg,1Hh,1Hi,1Hj,1Hk,1Hl,1Hm,
     3           1Hn,1Ho,1Hp,1Hq,1Hr,1Hs,1Ht,1Hu,1Hv,1Hw,1Hx,1Hy,1Hz/""",
        data_list('UPPER', range(65, 91)) + '\n' +
        data_list('LOWER', range(97, 123)))

# ULCASE holds 26 pairs of upper and lower case letters and compares against
# ' ', 'I', '.', '!' and '?'; every one of those is a character in a word, so
# each single-character literal in it becomes the character's code.  It is the
# only unit where that blanket rule is safe - everywhere else the literals are
# text in FORMAT statements.
LITERAL = re.compile(r"'(.)'")

#     "s." and ". " for the plural of "point", two characters in one word.
fix('MAIN', 1, """      KK='s.'
      IF(K.EQ.1)KK='. '""",
    PORT + """TWO CHARACTERS IN A WORD, LOW-ORDER BYTE FIRST (SEE PUNPK).
      KK=%d
      IF(K.EQ.1)KK=%d""" % (ord('s') | (ord('.') << 8),
                            ord('.') | (ord(' ') << 8)))

#     "s " and "  " for the plural of "day", two characters in one word.
fix('HOURS', 1, """      T='s '
      IF(D.EQ.1)T='  '""",
    PORT + """TWO CHARACTERS IN A WORD, LOW-ORDER BYTE FIRST (SEE PUNPK).
      T=%d
      IF(D.EQ.1)T=%d""" % (ord('s') | (ord(' ') << 8),
                           ord(' ') | (ord(' ') << 8)))

#    START tidied up by attaching back to the home directory before it
#    stopped the program.
fix('START', 2, "      CALL ATCH$$(K$HOME,0,0,0,0,X)",
    PORT + """ATTACHING BACK TO THE HOME DIRECTORY BEFORE EXIT.
C     CALL ATCH$$(K$HOME,0,0,0,0,X)""")

# 4. The terminal reads.
fix('GETIN', 1, "      DIMENSION LINE(70),CHARS(5)",
    "      DIMENSION LINE(70),CHARS(5)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('GETIN', 1, """    2 READ(TTYI,3)LINE
    3 FORMAT(70A1)""",
    """    2 CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,LINE,70)
C   3 FORMAT(70A1)""")
fix('MAINT', 1,
    "      DIMENSION HNAME(20),ABB(150),CMADRS(4,11),CMSZES(11),FDUMMY(10)",
    "      DIMENSION HNAME(20),ABB(150),CMADRS(4,11),CMSZES(11),FDUMMY(10)\n"
    + PORT + "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAINT', 1, """      READ(TTYI,2)HNAME
    2 FORMAT(20A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,HNAME,20)
C   2 FORMAT(20A1)""")
fix('MOTD', 1, "      DIMENSION MTDTXT(100),TEXT(70)",
    "      DIMENSION MTDTXT(100),TEXT(70)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MOTD', 1, """   55 READ(TTYI,56)TEXT,K
   56 FORMAT(70A1,A1)""",
    """   55 CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,TEXT,70)
      K=ICHAR(PBUF(71:71))
C  56 FORMAT(70A1,A1)""")

# 5. START turned the terminal's echo off to read the magic word.
for unit, n in (('START', 1), ('WIZARD', 2)):
    fix(unit, n, """      CALL DUPLX$(INTS(:100000))""",
        PORT + """DUPLX$ TURNED THE TERMINAL'S ECHO OFF FOR THE MAGIC WORD.
C     CALL DUPLX$(INTS(:100000))""")
    fix(unit, n, """      CALL DUPLX$(0)""",
        PORT + """AND ON AGAIN.
C     CALL DUPLX$(0)""")
# 6. SHIFT used the Prime's own shift intrinsics.
fix('SHIFT', 1, """      IF( DIST .GE. 0 ) SHIFT = LS( VAL, DIST )
      IF( DIST .LT. 0 ) SHIFT = RS( VAL,-DIST )""",
    PORT + """LS AND RS WERE THE PRIME'S LOGICAL SHIFT INTRINSICS.
      IF( DIST .GE. 0 ) SHIFT = ISHFT( VAL, DIST )
      IF( DIST .LT. 0 ) SHIFT = ISHFT( VAL, DIST )""")

# 7. RAN used PRIMOS's own generator.  It was measured on the emulator:
#    RAND$A(R) sets R = 16807*R mod (2**31-1) and returns R/(2**31-1), the
#    Lehmer minimal standard.  RNDI$A seeded R from the clock.
fix('RAN', 1, """      IMPLICIT INTEGER (A-Z)
      REAL RAND$A
      COMMON /RANCOM/ R
C
      IF( R .EQ. 0 ) CALL RNDI$A(R)
      RAN = (RANGE-1)*RAND$A(R)""",
    PORT + """RAND$A AND RNDI$A WERE PRIMOS LIBRARY ROUTINES.  MEASURED ON
C  THE EMULATOR, RAND$A(R) SETS R = MOD(16807*R, 2**31-1) AND RETURNS
C  R/(2**31-1) - LEHMER'S MINIMAL STANDARD - AND RNDI$A SEEDS R FROM THE
C  CLOCK.  PRAND DOES THE SAME IN 64 BIT ARITHMETIC (SRC\\PORT\\PPRIME.F).
      IMPLICIT INTEGER (A-Z)
      REAL PRAND
      COMMON /RANCOM/ R
C
      IF( R .EQ. 0 ) CALL PRNDI(R)
      RAN = (RANGE-1)*PRAND(R)""")

# 8. WIZARD's password is the magic word with its second and fourth letters
#    replaced by the two-letter code for the day of the week.
fix('WIZARD', 1, """      CALL MCHR$A(CHARS(2),K1,WEEK(I),K1)
      CALL MCHR$A(CHARS(4),K1,WEEK(I),K2)""",
    PORT + """MCHR$A(A,I,B,J) PUT THE J'TH CHARACTER OF B INTO THE I'TH OF A
C  (MEASURED ON THE EMULATOR).  WEEK IS SEVEN TWO-CHARACTER DAY CODES AND
C  CHARS IS FIVE WORDS OF ONE CHARACTER EACH, SO THIS REPLACES THE SECOND
C  AND FOURTH LETTERS OF THE MAGIC WORD WITH THE DAY'S CODE.
      CHARS(2)=PWEEK(I,1)
      CHARS(4)=PWEEK(I,2)""")
fix('WIZARD', 1, """      INTEGER *2 WEEK(7), K2, K1
      PARAMETER K1=1, K2=2
      INTEGER *4 CHARS(5)
      DATA WEEK/'SASUMOTUWETHFR'/""",
    PORT + """THE DAY CODES, ONE CHARACTER PER WORD (SEE PUNPK).
      INTEGER *4 CHARS(5)
      DIMENSION PWEEK(7,2)
      DATA PWEEK/83,83,77,84,87,84,70,65,85,79,85,69,72,82/""")


def main():
    if not os.path.isdir(OUT):
        os.makedirs(OUT)
    seen = set()
    for name, lines in units():
        if name in REPLACED:
            continue
        if name in seen:
            sys.exit('convert: %s defined twice' % name)
        seen.add(name)
        text = '\n'.join(lines)
        for count, old, new in FIXUPS.get(name, ()):
            # Whole lines only: a pattern that matched the beginning of a
            # longer declaration once spliced a new statement into the middle
            # of it, which compiled and was wrong.
            n = text.count(old + '\n')
            if n != count:
                sys.exit('convert: %s: pattern found %d times, expected %d:'
                         '\n%s' % (name, n, count, old[:70]))
            text = text.replace(old + '\n', new + '\n')
        if 'DATA CHRSET/' in text:
            old, new = chrset(text)
            text = text.replace(old, new)
        if name == 'ULCASE':
            text, n = LITERAL.subn(lambda m: str(ord(m.group(1))), text)
            if n != 64:
                sys.exit('convert: ULCASE has %d literals, expected 64' % n)
        for i, l in enumerate(text.split('\n')):
            if len(l) > 72 and l[:1] not in ('C', 'c', '*'):
                sys.exit('convert: %s line %d past column 72:\n%s'
                         % (name, i + 1, l))
        open(os.path.join(OUT, name.lower() + '.f'), 'w', encoding='latin-1',
             newline='\n').write(text + '\n')
    print('convert: %d units -> %s' % (len(seen), os.path.normpath(OUT)))

    # The database, as the game reads it (the file called COMMON).
    raw = open(os.path.join(ORIG, 'COMMON.txt'), 'rb').read().decode('latin-1')
    data = [l.rstrip() for l in raw.replace('\r\n', '\n').split('\n')]
    while data and not data[-1]:
        data.pop()
    out = os.path.join(OUT, '..', 'common')
    open(out, 'w', encoding='latin-1', newline='\n').write(
        '\n'.join(data) + '\n')
    print('convert: %d database lines -> %s'
          % (len(data), os.path.normpath(out)))


if __name__ == '__main__':
    main()
