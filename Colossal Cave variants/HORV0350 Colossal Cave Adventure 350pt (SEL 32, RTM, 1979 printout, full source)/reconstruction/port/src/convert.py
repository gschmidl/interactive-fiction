#!/usr/bin/env python3
"""Turn O'Dwyer's transcription of the 1979 SEL 32 printout into sources
gfortran will take, one file per program unit.

The printout is four card decks - `$JOB AD2COM` (the program), `$JOB DATIME`
and `$JOB LINES` (two members recompiled and put back in the library with
LIBED, so *they* are the ones the finished program used), and `$JOB RNDIO NED`
(assembler) - with the curatorial markings and the printer's running header
in between.  This script drops all of that, takes the later DATIME and SVCOMN
over the earlier ones, and then applies the edits gfortran needs.  Every edit
states how many times it must match, so if the transcription ever changes the
build stops instead of quietly producing a different game.

What the port supplies instead of the SEL 32 run-time is in src\\port\\; the
units listed in REPLACED below are not compiled from the printout at all.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original', 'HORV0350')
OUT = os.path.join(HERE, '..', '.build', 'src')

# Units the port replaces: everything that talked to RTM.
REPLACED = """IOINIT LDCOMN SVCOMN DATIME ADDR SIZE ABORT ATTACH GENRAT IO
SHUTDOWN LINES""".split()

HEADER = re.compile(r'^ {6,}(?:(?:INTEGER|LOGICAL|REAL)(?:\*\d)? +)?'
                    r'(?:PROGRAM|SUBROUTINE|FUNCTION) +([A-Z][A-Z0-9]*)')


def source():
    """The printout with the markings, the running header and the job control
    taken out, as a list of (job number, line)."""
    raw = open(os.path.join(ORIG, 'transcribed-code.txt'),
               encoding='latin-1').read()
    out = []
    job, kind, ended = 0, None, False
    for line in raw.split('\n'):
        line = line.rstrip()
        if line.startswith('==p') or line.startswith('   21MAR79'):
            continue
        if line.startswith('$'):
            if line.startswith('$JOB'):
                job, kind = job + 1, None
            elif line.startswith('$EXECUTE'):
                kind = line.split()[1]
            elif line.startswith('$$'):
                ended = True
            continue
        if ended:
            break               # O'Dwyer's own note at the end of the file
        if kind == 'FORTRAN':
            out.append((job, line))
    return out


def units():
    """(name, job, lines) for every FORTRAN program unit, in printout order."""
    got, cur, name, job = [], None, None, None
    for j, line in source():
        m = HEADER.match(line)
        if m:
            if cur is not None:
                sys.exit('convert: %s has no END' % name)
            cur, name, job = [line], m.group(1), j
            continue
        if cur is None:
            continue            # comments between units
        cur.append(line)
        if line.strip() == 'END':
            got.append((name, job, cur))
            cur = None
    if cur is not None:
        sys.exit('convert: %s has no END' % name)
    return got


def pack(s):
    """A character in a word.  The SEL kept it in the high-order byte with
    blanks after it; the port keeps the code in the low-order byte, which is
    the byte gfortran's A1 writes out (see PUNPK in src\\port\\pchar.f)."""
    return ord(s)


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


# ---------------------------------------------------------------- the edits

PORT = 'C  PORT: '
FIXUPS = {}


def fix(unit, count, old, new):
    FIXUPS.setdefault(unit, []).append((count, old, new))


# 1. Characters in words.  The three SIXBIT tables are spelled out from the
#    transcription itself so that the two characters it gives as hex - 5C, a
#    backslash, and 07, a bell - keep their values.
def chrset(text):
    m = re.search(r'      DATA CHRSET/.*?/\n', text, re.S)
    body = m.group(0)
    vals = []
    for tok in re.findall(r"1H(.)|8Z([0-9A-F]{8})", body):
        vals.append(ord(tok[0]) if tok[0] else int(tok[1][:2], 16))
    if len(vals) != 64:
        sys.exit('convert: CHRSET has %d entries' % len(vals))
    return body[:-1], data_list('CHRSET', vals)


# 2. The reads.  gfortran accepts "A" editing on an integer but reads it as a
#    numeric field - a comma would end it - so every such read goes through
#    PGETLN and PUNPK.  Output is untouched: gfortran's A1 writes the
#    low-order byte, which is where PUNPK puts the character.
fix('ADVENTUR', 1, """ 1004 READ(DBFI,1005)LOC,TEXT,KK
 1005 FORMAT(1I8,70A1,A1)""",
    """ 1004 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1005)LOC
 1005 FORMAT(I8)
      CALL PUNPK(PBUF(9:78),TEXT,70)
      KK=ICHAR(PBUF(79:79))""")

fix('ADVENTUR', 1, """ 1043 READ(DBFI,1041)KTAB(TABNDX),(TEXT(I),I=1,5)
 1041 FORMAT(I8,5A1)""",
    """ 1043 CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1041)KTAB(TABNDX)
 1041 FORMAT(I8)
      CALL PUNPK(PBUF(9:13),TEXT,5)""")

fix('ADVENTUR', 1, "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),FDUMMY(10)",
    "      DIMENSION CMADRS(4,11),CMSZES(11),TEXT(70),FNAME(10),FDUMMY(10)\n"
    + PORT + "SEE PGETLN\n      CHARACTER*132 PBUF")

fix('ADVENTUR', 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")

#    Two lines of the transcription do not compile at all - and could not
#    have compiled on the SEL either, so they are typed wrongly rather than
#    printed wrongly.  Reported to Arthur O'Dwyer.
fix('ADVENTUR', 1, "      INVENT=VOCAB(0+'INVEN'),2)",
    PORT + """TRANSCRIPTION: READ AS VOCAB(CODE1('INVEN'),2), LIKE THE
C  TWENTY-ODD LINES AROUND IT.  THE PRINTOUT AS TYPED HAS "0+'INVEN')".
      INVENT=VOCAB(CODE1('INVEN'),2)""")
fix('ADVENTUR', 1, """     1CTEXT,CVAL,HINTLC,HINTED,HINTS,DSEEN,DLOC,CLSSES,HNTMAX
     2PLAC,FIXD,MAXTRS,TALLY,TALLY2,""",
    PORT + """TRANSCRIPTION: THE COMMA AFTER HNTMAX IS MISSING, WHICH MAKES
C  THE NEXT CONTINUATION READ AS ONE NAME "HNTMAXPLAC" AND LEAVES PLAC OUT
C  OF THE COMMON BLOCK - A LOCAL ARRAY FULL OF WHATEVER WAS ON THE STACK.
     1CTEXT,CVAL,HINTLC,HINTED,HINTS,DSEEN,DLOC,CLSSES,HNTMAX,
     2PLAC,FIXD,MAXTRS,TALLY,TALLY2,""")

fix('ADVENTUR', 1,
    "      IF(TURNS.EQ.0.AND.WD1.EQ.CODE1('RESID'))GOTO 8400",
    PORT + """TRANSCRIPTION: READ AS 'RESTO', THE FIRST FIVE LETTERS OF
C  RESTORE, WHICH IS WHAT MESSAGE 142 TELLS THE PLAYER TO TYPE.  'RESID'
C  IS NOT A WORD AND NOTHING CAN EVER MATCH IT.
      IF(TURNS.EQ.0.AND.WD1.EQ.CODE1('RESTO'))GOTO 8400""")

fix('ADVENTUR', 1, "      .GAVEUP.=TRUE",
    PORT + """TRANSCRIPTION: THE DOTS HAVE MOVED; READ AS GAVEUP=.TRUE.
      GAVEUP=.TRUE.""")

#    SIZE is a gfortran intrinsic; here it is one of the three routines the
#    conversion guide asks the site to supply (ADDR, SIZE and the save file).
fix('ADVENTUR', 1, """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT""",
    """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT
""" + PORT + """SIZE AND LINES ARE GFORTRAN INTRINSICS
      EXTERNAL SIZE,LINES
      INTEGER*4 SIZE,LINES""")

#    SHIFT was a SEL intrinsic, and .AND. between two integers was a bitwise
#    and there; gfortran has neither.
fix('ADVENTUR', 1, "      BITSET(L,N)=(COND(L).AND.SHIFT(1,N)).NE.0",
    PORT + """SHIFT WAS A SEL INTRINSIC AND .AND. ON TWO INTEGERS WAS A
C  BITWISE AND THERE.  THE REST OF THE PROGRAM ALREADY SAYS IAND/ISHFT.
      BITSET(L,N)=IAND(COND(L),ISHFT(1,N)).NE.0""")

#    Options, which the original had no use for.
fix('ADVENTUR', 1, """      CALL IOINIT(0)
      CALL LDCOMN(.TRUE.,FDUMMY,CMADRS,CMSZES)""",
    PORT + """COMMAND LINE OPTIONS.
      CALL POPTS
      CALL IOINIT(0)
      CALL LDCOMN(.TRUE.,FDUMMY,CMADRS,CMSZES)""")

#    Two characters in one word, for the "POINT" / "POINTS" ending.  The SEL
#    put the first character in the high-order byte; here it goes in the low
#    one, which is the order gfortran's A2 writes out again.
fix('ADVENTUR', 1, """      KK='S.'
      IF(K.EQ.1)KK='. '""",
    PORT + """TWO CHARACTERS IN A WORD, LOW-ORDER BYTE FIRST (SEE PUNPK).
      KK=%d
      IF(K.EQ.1)KK=%d""" % (ord('S') | (ord('.') << 8),
                            ord('.') | (ord(' ') << 8)))

# 3. " was the escape character inside a SEL character literal.
fix('ADVENTUR', 2, """CODE1('"".   ')""", """CODE1('".   ')""")
fix('ADVENTUR', 1, """ 9032 FORMAT(/,' OKAY, ""',20A1)""",
    """ 9032 FORMAT(/,' OKAY, "',20A1)""")

# 4. CODE1 took its five characters out of an integer array packed four to a
#    word; every caller in the program passes a literal instead.
fix('CODE1', 1, """      DIMENSION WORDS(2)
      DIMENSION CHRSET(64)

      DATA NWORDS/2/,NCHARS/4/,CHRSIZ/8/,CHRMSK/8ZFF000000/""",
    PORT + """THE ORIGINAL PICKED THE CHARACTERS OUT OF AN INTEGER ARRAY
C  HOLDING THEM FOUR TO A WORD, HIGH-ORDER BYTE FIRST.  EVERY CALL IN THE
C  PROGRAM PASSES A FIVE CHARACTER LITERAL, SO TAKE THE LITERAL AS A
C  STRING AND THE BYTE ORDER OF THE HOST STOPS MATTERING.
      CHARACTER*(*) WORDS
      CHARACTER*5 W
      DIMENSION CHRSET(64)""")

fix('CODE1', 1, """      RESULT=0
      COUNT=0

      DO 10 I=1,NWORDS
         WORD=WORDS(I)

         DO 5 J=1,NCHARS
            COUNT=COUNT+1
            IF(COUNT.GT.5)GOTO 20
            CHAR=IAND(WORD,CHRMSK)
            WORD=ISHFT(WORD,CHRSIZ)
            DO 1 CHRIDX=1,64
               IF(CHAR.EQ.IAND(CHRSET(CHRIDX),CHRMSK))GOTO 2
    1       CONTINUE
            CHRIDX=15
    2       RESULT=ISHFT(RESULT,6)+CHRIDX-1
    5    CONTINUE
   10 CONTINUE

   20 CODE1=RESULT""",
    """      W=WORDS
      RESULT=0
      DO 10 I=1,5
         CHAR=ICHAR(W(I:I))
         DO 1 CHRIDX=1,64
            IF(CHAR.EQ.CHRSET(CHRIDX))GOTO 2
    1    CONTINUE
         CHRIDX=15
    2    RESULT=ISHFT(RESULT,6)+CHRIDX-1
   10 CONTINUE
   20 CODE1=RESULT""")

# CVLTUC's lower case alphabet is 26 blanks in the printout, which would turn
# every space in the player's line into an "A" and make the game unusable.
fix('CVLTUC', 1, """      DATA UPPER/1HA,1HB,1HC,1HD,1HE,1HF,1HG,1HH,1HI,1HJ,1HK,1HL,1HM,
     1           1HN,1HO,1HP,1HQ,1HR,1HS,1HT,1HU,1HV,1HW,1HX,1HY,1HZ/,
     2     LOWER/1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,
     3           1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H ,1H /""",
    PORT + """FIX: THE LOWER CASE ALPHABET IS 26 BLANKS IN THE PRINTOUT, SO
C  THE LOOP BELOW WOULD TURN EVERY SPACE IN THE PLAYER'S LINE INTO AN "A".
C  IT IS RESTORED HERE; --no-fixes PUTS THE BLANKS BACK (SEE PLOW).
""" + data_list('UPPER', range(65, 91)) + '\n' + data_list('LOWER',
                                                           range(97, 123)))
fix('CVLTUC', 1, """      IMPLICIT INTEGER*4(A-Z)
      DIMENSION TEXT(70)""",
    """      IMPLICIT INTEGER*4(A-Z)
      DIMENSION TEXT(70)
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ""")
fix('CVLTUC', 1, """         DO 5 J=1,26
            IF(CHR.NE.LOWER(J))GOTO 5""",
    """         DO 5 J=1,26
            PLOW=LOWER(J)
            IF(NOFIX.NE.0)PLOW=32
            IF(CHR.NE.PLOW)GOTO 5""")

fix('CVSTB', 1, "      DATA DIGITS/1H0,1H1,1H2,1H3,1H4,1H5,1H6,1H7,1H8,1H9/",
    "      DATA DIGITS/48,49,50,51,52,53,54,55,56,57/")
fix('CVSTB', 1, "      DATA BLANK,MINUS,PLUS/' ','-','+'/",
    "      DATA BLANK,MINUS,PLUS/32,45,43/")

for unit in ('SPEAK', 'A5TOA1'):
    fix(unit, 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")

fix('SPEAK', 1, "      IF LINES(K).GE.0)GOTO 10",
    PORT + """TRANSCRIPTION: THE OPENING PARENTHESIS IS MISSING.
      IF(LINES(K).GE.0)GOTO 10""")

fix('HOURS', 1, """      T='S,'
      IF(D.EQ.1)T=', '""",
    PORT + """TWO CHARACTERS IN A WORD, LOW-ORDER BYTE FIRST (SEE PUNPK).
      T=%d
      IF(D.EQ.1)T=%d""" % (ord('S') | (ord(',') << 8),
                           ord(',') | (ord(' ') << 8)))

# 5. GETIN: NULLOK is missing from the dummy argument list, although the
#    comment above it describes it and all fourteen callers pass it.
fix('GETIN', 1, "      SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)",
    PORT + """FIX: NULLOK IS MISSING FROM THE ARGUMENT LIST.  THE COMMENT
C  BELOW DESCRIBES IT, THE BODY TESTS IT, AND ALL FOURTEEN CALLS PASS IT -
C  SO WITHOUT THIS THE BLANK LINE TEST READS AN UNSET VARIABLE.  LIKELY A
C  TRANSCRIPTION SLIP: WOOD0350, WHICH THE TRANSCRIPTION STARTED FROM, HAS
C  NO NULLOK AT ALL.  --no-fixes LEAVES IT AS TRANSCRIBED (SEE PNULOK).
      SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X,NULLOK)""")
fix('GETIN', 1, """      LOGICAL NULLOK,BLKLIN,NULL,LGWORD""",
    """      LOGICAL NULLOK,BLKLIN,NULL,LGWORD
      LOGICAL PNULOK
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ""")
fix('GETIN', 1, """      IF(NULL.AND..NOT.NULLOK)GOTO 2
      IF(NULL.AND.NULLOK)RETURN""",
    """      PNULOK=NULLOK
      IF(NOFIX.NE.0)PNULOK=.FALSE.
      IF(NULL.AND..NOT.PNULOK)GOTO 2
      IF(NULL.AND.PNULOK)RETURN""")
fix('GETIN', 1, "      DATA BLANK/' '/, NEWLINE/8Z0D202020/",
    "      DATA BLANK/32/, NEWLINE/13/")
#    The scan has to resume at the blank the word ended on.  Setting WDST to 1
#    sends it back to the start of the line, so the second word comes out as
#    nothing and "TAKE KEYS" answers "TAKE WHAT?".  A one against a letter I.
fix('GETIN', 1, """ 1010 WDST=1
      LGWORD=.FALSE.""",
    PORT + """TRANSCRIPTION: READ AS WDST=I, THE POSITION THE WORD ENDED AT.
C  WITH WDST=1 THE SCAN GOES BACK TO THE START OF THE LINE AND THE SECOND
C  WORD IS NEVER FOUND.
 1010 WDST=I
      LGWORD=.FALSE.""")
fix('GETIN', 1, "      DIMENSION LINE(70),CHARS(5)",
    "      DIMENSION LINE(70),CHARS(5)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('GETIN', 1, """    2 READ(TTYI,3)LINE
    3 FORMAT(70A1)""",
    """    2 CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,LINE,70)
C   3 FORMAT(70A1)""")

fix('MOTD', 1, "      CVLTUC(TEXT,K)",
    PORT + """TRANSCRIPTION: "CALL" IS MISSING.
      CALL CVLTUC(TEXT,K)""")

# 6. MOTD and MAINT read characters too.
fix('MOTD', 1, "      DATA BLANK/' '/,PERIOD/'.'/,NEWLINE/8Z0D202020/",
    "      DATA BLANK/32/,PERIOD/46/,NEWLINE/13/\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAINT', 1, "      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI",
    "      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI\n" + PORT +
    "SEE PGETLN\n      CHARACTER*132 PBUF")
fix('MAINT', 1, """      READ(TTYI,2)HNAME
    2 FORMAT(20A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,HNAME,20)
C   2 FORMAT(20A1)""")

# 7. The headings HOURSX and NEWHRX print, and the wizard's test.
for unit in ('HOURSX', 'NEWHRX'):
    fix(unit, 1, """      DATA ((TYPE(I,J),J=1,10),I=1,3)
     1     /1HM,1HO,1HN,1H ,1H-,1H ,1HF,1HR,1HI,1H:,
     2      1HS,1HA,1HT,1H ,1H&,1H ,1HS,1HU,1HN,1H:,
     3      1HH,1HO,1HL,1HI,1HD,1HA,1HY,1HS,1H:,1H /""",
        PORT + """THE HEADINGS, ONE CHARACTER PER WORD (SEE PUNPK).
      DATA ((TYPE(I,J),J=1,10),I=1,3)
     1     /77,79,78,32,45,32,70,82,73,58,
     2      83,65,84,32,38,32,83,85,78,58,
     3      72,79,76,73,68,65,89,83,58,32/""")

# 7b. gfortran has intrinsics of its own called RAN (a REAL function that
#     writes back to its argument), XOR, OR, ABORT, SIZE and LINES, so every
#     unit that calls the program's own has to say so.
EXTRA = {'ADVENTUR': 'RAN', 'WIZARD': 'RAN,XOR', 'NEWHRX': 'OR',
         'SPEAK': 'LINES', 'PSPEAK': 'LINES', 'RSPEAK': 'LINES',
         'BUG': 'ABORT'}
for unit, names in EXTRA.items():
    fix(unit, 1, "      IMPLICIT INTEGER*4(A-Z)",
        "      IMPLICIT INTEGER*4(A-Z)\n" + PORT +
        "GFORTRAN HAS INTRINSICS OF THESE NAMES\n      EXTERNAL " + names)

#     --auto walks past the wizard's guessing game, which is how build.sh
#     gets the game to save itself.  The real test is still here and still
#     works (tests\wizard.py plays it).
fix('WIZARD', 1, """      WIZARD=YESM(16,0,7)
      IF(.NOT.WIZARD)RETURN""",
    PORT + """--auto SKIPS THE WIZARD'S TEST
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ
      IF(AUTOWZ.NE.0)GOTO 50
      WIZARD=YESM(16,0,7)
      IF(.NOT.WIZARD)RETURN""")
fix('WIZARD', 1, """   99 CALL MSPEAK(20)""",
    """   50 WIZARD=.TRUE.
      RETURN
C
   99 CALL MSPEAK(20)""")

#     -u (/PUNCOM/, set by POPTS): no prime time, so no demonstration game
#     either, and no wait before a suspended game may be restored.
#     Without -u, START is as transcribed.
fix('START', 1, """      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI""",
    """      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI
      COMMON /PUNCOM/ PUNL""")
fix('START', 1, """      PTIME=IAND(PRIMTM,ISHFT(1,T/60)).NE.0
      SOON=.FALSE.
      IF(SETUP.GE.0)GOTO 20""",
    """      PTIME=IAND(PRIMTM,ISHFT(1,T/60)).NE.0
""" + PORT + """-u: NO PRIME TIME AND NO WAIT BEFORE A RESTORED GAME
      IF(PUNL.NE.0)PTIME=.FALSE.
      SOON=.FALSE.
      IF(SETUP.GE.0.OR.PUNL.NE.0)GOTO 20""")

# 8. Two masks that were hex constants in the SEL's own spelling.
#    RAN branches to a label 1 that the transcription does not have; it can
#    only be the line that advances the seed, which is where every other port
#    in this family puts it.  Reported to Arthur O'Dwyer.
fix('RAN', 1, """      R = R * 16807     !16807 = 7**5 - LONG PERIOD FOR 32 BIT INT
      RAN = IAND (8Z0000FFFF, ISHFT(R,-8))""",
    PORT + """TRANSCRIPTION: THE LABEL 1 THAT "IF (R.NE.0) GOTO 1" NEEDS IS
C  MISSING FROM THE NEXT LINE.
1     R = R * 16807     !16807 = 7**5 - LONG PERIOD FOR 32 BIT INT
      RAN = IAND (Z'0000FFFF', ISHFT(R,-8))""")
fix('LOWSIX', 1, "      LOWSIX = IAND (A, 8Z0000003F)",
    "      LOWSIX = IAND (A, Z'0000003F')")


def commas(name, text):
    """A continued COMMON, DIMENSION, DATA or EQUIVALENCE list must have a
    separator at the break.  Without one, two names run into a single name -
    which is how PLAC fell out of the COMMON block in the transcription."""
    lines = text.split('\n')
    stmt = None
    for i, l in enumerate(lines):
        if not l.strip() or l[:1] in ('C', 'c', '*'):
            continue
        if len(l) > 5 and l[5] not in (' ', '0') and stmt is not None:
            prev = lines[stmt].rstrip()
            body = l[6:].lstrip()
            kind = re.match(r'\s*(COMMON|DIMENSION|DATA|EQUIVALENCE|INTEGER'
                            r'|LOGICAL)\b', lines[stmt0][6:])
            if (kind and prev and prev[-1] not in ',/(+-*='
                    and body[:1].isalpha()):
                sys.exit('convert: %s line %d: no separator between %r and %r'
                         % (name, i + 1, prev[-20:], body[:20]))
        else:
            stmt0 = i
        stmt = i


def labels(name, text):
    """Every statement label a unit branches to must exist in it.  This is a
    transcription of a printout, so a label that has gone missing is exactly
    the kind of damage to look for - and it found one, in RAN."""
    lines = [l for l in text.split('\n') if l[:1] not in ('C', 'c', '*')]
    defined = set()
    for l in lines:
        lab = l[:5].strip()
        if lab.isdigit():
            defined.add(int(lab))
    used = set()
    for l in lines:
        body = l[6:]
        for m in re.finditer(r'\b(?:GO ?TO|GOTO)\s*\(?([\d, ]+)\)?', body):
            used.update(int(x) for x in re.findall(r'\d+', m.group(1)))
        for m in re.finditer(r'\b(?:READ|WRITE)\s*\(\s*[A-Z0-9]+\s*,\s*(\d+)',
                             body):
            used.add(int(m.group(1)))
        for m in re.finditer(r'\b(?:END|ERR)\s*=\s*(\d+)', body):
            used.add(int(m.group(1)))
        m = re.match(r'\s*DO\s+(\d+)', body)
        if m:
            used.add(int(m.group(1)))
        m = re.match(r'\s*IF\s*\(.*\)\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*$',
                     body)
        if m:
            used.update(int(g) for g in m.groups())
    missing = sorted(used - defined)
    if missing:
        sys.exit('convert: %s branches to labels it does not define: %s'
                 % (name, missing))


def main():
    if not os.path.isdir(OUT):
        os.makedirs(OUT)
    seen = {}
    for name, job, lines in units():
        if name in REPLACED:
            continue
        text = '\n'.join(lines)
        if name in seen:
            # DATIME and SVCOMN were recompiled in a later job and put back
            # in the library, so the later one is the one that ran.  Both are
            # in REPLACED, so this is only a guard.
            sys.exit('convert: %s defined twice (jobs %s and %s)'
                     % (name, seen[name], job))
        seen[name] = job
        for count, old, new in FIXUPS.get(name, ()):
            n = text.count(old)
            if n != count:
                sys.exit('convert: %s: pattern found %d times, expected %d:'
                         '\n%s' % (name, n, count, old[:70]))
            text = text.replace(old, new)
        if 'DATA CHRSET/' in text:
            old, new = chrset(text)
            text = text.replace(old, new)
        for i, l in enumerate(text.split('\n')):
            if len(l) > 72 and l[:1] not in ('C', 'c', '*'):
                sys.exit('convert: %s line %d past column 72:\n%s'
                         % (name, i + 1, l))
        labels(name, text)
        commas(name, text)
        open(os.path.join(OUT, name.lower() + '.f'), 'w', encoding='latin-1',
             newline='\n').write(text + '\n')
    print('convert: %d units -> %s' % (len(seen), os.path.normpath(OUT)))

    # The database, with the markings and the running header taken out.
    raw = open(os.path.join(ORIG, 'transcribed-data.txt'),
               encoding='latin-1').read()
    data = [l.rstrip() for l in raw.split('\n')
            if not l.startswith('==p') and not l.startswith('   21MAR79')]
    while data and not data[-1]:
        data.pop()
    # One typo in the text, found by comparing this database with the MSU
    # one (tests\cmpdata.py).  Reported to Arthur O'Dwyer with the others.
    n = 0
    for i, l in enumerate(data):
        if l[8:].startswith('DDIGGING WITHOUT A SHOVEL'):
            data[i] = l[:8] + l[9:]
            n += 1
    if n != 1:
        sys.exit('convert: expected one DDIGGING, found %d' % n)
    out = os.path.join(OUT, '..', 'adv.data')
    open(out, 'w', encoding='latin-1', newline='\n').write(
        '\n'.join(data) + '\n')
    print('convert: %d database lines -> %s'
          % (len(data), os.path.normpath(out)))


if __name__ == '__main__':
    main()
