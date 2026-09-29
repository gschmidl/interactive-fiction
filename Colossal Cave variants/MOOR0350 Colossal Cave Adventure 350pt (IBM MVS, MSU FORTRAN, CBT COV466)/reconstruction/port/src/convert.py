#!/usr/bin/env python3
"""Turn the CBT COV466 file 119 PDS members into sources gfortran will take.

Every member is copied through untouched except for the edits in FIXUPS below,
and each edit states how many times it must match: if a member ever changes,
the build stops instead of quietly producing a different game.

The edits fall into five groups, all described in ..\\README.md:
  1. bitwise AND/OR/XOR built out of LOGICAL*4 EQUIVALENCE on the 370,
  2. characters in integers - Hollerith and 'x' constants become the byte
     value, and every "A1" *read* becomes PGETLN + PUNPK,
  3. the one assembler module (GETDTM) and the DD cards (IOINIT),
  4. command line options (POPTS), which the original had no use for,
  5. one fix, in ADVENT2 only, switched off by --no-fixes.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PDS = os.path.join(HERE, '..', '..', 'src_original', 'CBT.COV466.FILE119.PDS')
OUT = os.path.join(HERE, '..', '.build', 'src')

# The members that make up the program (the $ members are JCL, ADVTAPE is the
# job that wrote the tape, ADVTDATA is the database).
MEMBERS = """A5TOA1 AND AT BITSET BUG CARRY CODE1 CODE2 CVLTUC CVSTB DARK
DATIME DCODE1 DROP DSTROY FORCED GET12 GETIN HERE HOURS HOURSX IOINIT JUGGLE
LIQ LIQ2 LIQLOC MAINT MOTD MOVE MSPEAK NEWHRS NEWHRX OR PCT POOF PSPEAK PUT
RAN RSPEAK SCRMBL SHIFT SPEAK START TOTING VOCAB WIZARD XOR YES YESM YESX
ADVENT ADVENT2 ADVWIZ""".split()
# GETDTM is assembler; the port supplies its own (src\port\getdtm.f).


def load(member):
    """One member as text, CRs and trailing blanks off, latin-1 so that the
    two characters the EBCDIC->ASCII conversion could not place survive."""
    raw = open(os.path.join(PDS, member + '.txt'), 'rb').read()
    lines = raw.decode('latin-1').split('\r\n')
    return '\n'.join(l.rstrip() for l in lines)


def data_list(name, values, first='      DATA ', cont='     1     '):
    """A fixed-form DATA statement of integers, wrapped inside column 72."""
    out = []
    line = first + name + '/'
    for i, v in enumerate(values):
        item = str(v) + ('/' if i == len(values) - 1 else ',')
        if len(line) + len(item) > 71:
            out.append(line)
            line = cont
        line += item
    out.append(line)
    return '\n'.join(out)


def chrset_of(text):
    """The 64 character SIXBIT table exactly as the member spells it."""
    m = re.search(r'DATA CHRSET/(.*?)/\n', text, re.S)
    chars = re.findall(r'1H(.)', m.group(1))
    if len(chars) != 64:
        sys.exit('convert: CHRSET has %d entries, expected 64' % len(chars))
    return [ord(c) for c in chars]


def pack(s):
    """Characters in one word, first character in the low-order byte - the
    order gfortran's A2/A4 output uses.  See PUNPK in src\\port\\pchar.f."""
    v = 0
    for i, c in enumerate(s):
        v |= ord(c) << (8 * i)
    return v


# ---------------------------------------------------------------- the edits

PORT = 'C  PORT: '

FIXUPS = {}


def fix(member, count, old, new):
    FIXUPS.setdefault(member, []).append((count, old, new))


# 1. Bitwise operations.  On the 370 a LOGICAL*4 .AND. over an EQUIVALENCE is
#    a 32 bit bitwise and; gfortran normalises logicals to 0/1 first, so the
#    bodies have to say what they mean.
fix('AND', 1, """      LOGICAL AL,BL,CL
      EQUIVALENCE (AL,A1),(BL,B1),(CL,C1)
C
      A1=A
      B1=B
      CL=AL.AND.BL
      AND=C1""", PORT + """LOGICAL*4 .AND. OVER AN EQUIVALENCE IS A BITWISE
C  AND ON THE 370.  GFORTRAN NORMALISES LOGICALS TO 0/1, SO SAY IAND.
      AND=IAND(A,B)""")

fix('OR', 1, """      LOGICAL AL,BL,CL
      EQUIVALENCE (AL,A1),(BL,B1),(CL,C1)
C
      A1=A
      B1=B
      CL=AL.OR.BL
      OR=C1""", PORT + """AS IN "AND" - A BITWISE OR ON THE 370.
      OR=IOR(A,B)""")

fix('XOR', 1, """      LOGICAL AL,BL,CL,NAL,NBL
      EQUIVALENCE (AL,A1),(BL,B1),(CL,C1),(NAL,NA1),(NBL,NB1)
C
      A1=A
      B1=B
      NA1=-A1-1
      NB1=-B1-1
      CL=(AL.AND.NBL).OR.(NAL.AND.BL)
      XOR=C1""", PORT + """AS IN "AND".  -A-1 IS THE COMPLEMENT OF A, SO
C  (A AND NOT B) OR (NOT A AND B) IS AN EXCLUSIVE OR EXACTLY.
      XOR=IEOR(A,B)""")

fix('SHIFT', 1, "     1     MAXNEG/Z80000000/,",
    "     1     MAXNEG/Z'80000000'/,")

# 2. Characters in integers.
for member in ('A5TOA1', 'ADVWIZ', 'SPEAK'):
    fix(member, 1, "      DATA BLANK/' '/", "      DATA BLANK/32/")
fix('GETIN', 1, "      DATA BLNK/' '/", "      DATA BLNK/32/")

for member in ('ADVENT', 'ADVENT2'):
    fix(member, 1, "      DATA BLANK/' '/,SDOT/'S.'/,DOTBLK/'. '/",
        "      DATA BLANK/32/,SDOT/%d/,DOTBLK/%d/" % (pack('S.'), pack('. ')))

fix('CVSTB', 1, "      DATA DIGITS/1H0,1H1,1H2,1H3,1H4,1H5,1H6,1H7,1H8,1H9/",
    "      DATA DIGITS/48,49,50,51,52,53,54,55,56,57/")
fix('CVSTB', 1, "      DATA BLANK,MINUS,PLUS/' ','-','+'/",
    "      DATA BLANK,MINUS,PLUS/32,45,43/")

fix('HOURS', 1, "      DATA SCOM/'S,'/,COMBLK/', '/",
    "      DATA SCOM/%d/,COMBLK/%d/" % (pack('S,'), pack(', ')))

fix('MAINT', 1, "      DATA LTTRT/'T'/,LTTRF/'F'/",
    "      DATA LTTRT/84/,LTTRF/70/")

fix('MOTD', 1, "      DATA BLANK/' '/,BLANKS/'    '/,PERIOD/'.'/",
    "      DATA BLANK/32/,BLANKS/%d/,PERIOD/46/" % pack('    '))

fix('GET12', 1, """      DATA BLNK/' '/,CHARA/'A'/,CHARZ/'Z'/,CHAR$/'$'/,CHARPD/'#'/,
     1     CHARAT/'@'/,CHARCL/':'/,CHAR0/'0'/,CHAR9/'9'/""",
    PORT + """IN EBCDIC A-Z IS NOT CONTIGUOUS, SO THE RANGE TESTS BELOW
C  ALSO LET THROUGH THE CODES BETWEEN I AND J AND BETWEEN R AND S.  IN
C  ASCII THEY ARE EXACTLY THE LETTERS, WHICH IS ALL A KEYBOARD CAN SEND
C  ANYWAY.
      DATA BLNK/32/,CHARA/65/,CHARZ/90/,CHAR$/36/,CHARPD/35/,
     1     CHARAT/64/,CHARCL/58/,CHAR0/48/,CHAR9/57/""")

# The three headings HOURSX and NEWHRX print, read down the columns:
# "MON-FRI: ", "SAT&SUN: ", "HOLIDAYS:".
for member in ('HOURSX', 'NEWHRX'):
    text = load(member)
    old = re.search(r'      DATA TYPE\n(?:.*?)/\n', text, re.S).group(0)[:-1]
    chars = re.findall(r"'(.)'", old)
    if len(chars) != 30:
        sys.exit('convert: %s TYPE has %d entries' % (member, len(chars)))
    rows = [', '.join(str(ord(c)) for c in chars[i:i + 3])
            for i in range(0, 30, 3)]
    new = [PORT + 'THE HEADINGS, ONE CHARACTER PER WORD (SEE PUNPK).',
           '      DATA TYPE']
    for i, row in enumerate(rows):
        mark = '123456789A'[i]
        sep = '/' if i == 0 else ' '
        end = '/' if i == 9 else ','
        new.append('     %s          %s%s%s' % (mark, sep, row, end))
    fix(member, 1, old, '\n'.join(new))

# gfortran has a g77-compatibility intrinsic named RAN - a REAL function
# which *writes back* to its argument - so without this the program's own
# RAN is never called and RAN(100) tries to modify a literal.
LOGLINE = ("      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,"
           "PANIC,\n     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT")
for member, anchor in (
        ('ADVENT', LOGLINE),
        ('ADVENT2', LOGLINE),
        ('PCT', "      IMPLICIT INTEGER(A-Z)"),
        ('WIZARD', """      LOGICAL YESM
      DIMENSION HNAME(20),XD(10)""")):
    fix(member, 1, anchor, anchor + '\n' + PORT +
        'GFORTRAN HAS AN INTRINSIC OF ITS OWN CALLED RAN\n      EXTERNAL RAN')

# CVLTUC's two alphabets.
fix('CVLTUC', 1, """      DATA UPPER/1HA,1HB,1HC,1HD,1HE,1HF,1HG,1HH,1HI,1HJ,1HK,1HL,1HM,
     1           1HN,1HO,1HP,1HQ,1HR,1HS,1HT,1HU,1HV,1HW,1HX,1HY,1HZ/,
     2     LOWER/1Ha,1Hb,1Hc,1Hd,1He,1Hf,1Hg,1Hh,1Hi,1Hj,1Hk,1Hl,1Hm,
     3           1Hn,1Ho,1Hp,1Hq,1Hr,1Hs,1Ht,1Hu,1Hv,1Hw,1Hx,1Hy,1Hz/""",
    data_list('UPPER', range(65, 91)) + '\n' +
    data_list('LOWER', range(97, 123)))

# The SIXBIT table, three copies of it, spelled as byte values.
for member in ('CODE1', 'CODE2', 'DCODE1'):
    text = load(member)
    old = re.search(r'      DATA CHRSET/.*?/\n', text, re.S).group(0)[:-1]
    fix(member, 1, old, data_list('CHRSET', chrset_of(text)))

# CODE1 took its five characters out of an integer array packed four to a
# word; every caller in the program passes a literal instead.
fix('CODE1', 1, """      IMPLICIT INTEGER(A-Z)
      DIMENSION WORDS(2)
      DATA NWORDS/2/,NCHARS/4/,CHRSIZ/8/,CHRMSK/ZFF000000/""",
    PORT + """THE ORIGINAL PICKED THE CHARACTERS OUT OF AN INTEGER ARRAY
C  HOLDING THEM FOUR TO A WORD ("A4" FORMAT) WITH AND AND SHIFT.  EVERY
C  CALL IN THE PROGRAM PASSES A FIVE CHARACTER LITERAL, SO TAKE THE
C  LITERAL AS A STRING AND THE BYTE ORDER OF THE HOST STOPS MATTERING.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) WORDS
      CHARACTER*5 W""")

fix('CODE1', 1, """      RESULT=0
      COUNT=0
      DO 10 I=1,NWORDS
         WORD=WORDS(I)
         DO 5 J=1,NCHARS
            COUNT=COUNT+1
            IF(COUNT.GT.5) GO TO 20
            CHAR=AND(WORD,CHRMSK)
            WORD=SHIFT(WORD,CHRSIZ)
            DO 1 CHRIDX=1,64
               IF(CHAR.EQ.AND (CHRSET(CHRIDX),CHRMSK)) GO TO 2
1           CONTINUE
            CHRIDX=15
2           RESULT=SHIFT(RESULT,6)+CHRIDX-1
5        CONTINUE
10    CONTINUE
20    CODE1=RESULT""", """      W=WORDS
      RESULT=0
      DO 10 I=1,5
         CHAR=ICHAR(W(I:I))
         DO 1 CHRIDX=1,64
            IF(CHAR.EQ.CHRSET(CHRIDX)) GO TO 2
1        CONTINUE
         CHRIDX=15
2        RESULT=SHIFT(RESULT,6)+CHRIDX-1
10    CONTINUE
20    CODE1=RESULT""")

# 2b. The "A1" reads.
fix('GETIN', 1, "      DIMENSION LINE(80)",
    "      DIMENSION LINE(80)\n" + PORT + "SEE PGETLN\n      CHARACTER*80 PBUF")
fix('GETIN', 1, """20    READ(TTYI,21) LINE
21    FORMAT (80A1)""",
    """20    CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,LINE,80)
C21   FORMAT (80A1)""")

fix('MAINT', 1, "      DATA LTTRT/84/,LTTRF/70/",
    PORT + "SEE PGETLN\n      CHARACTER*80 PBUF\nC\n"
    "      DATA LTTRT/84/,LTTRF/70/")
fix('MAINT', 1, """      READ(TTYI,2)HNAME
2     FORMAT(20A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,HNAME,20)
C2    FORMAT(20A1)""")
fix('MAINT', 1, """      READ(TTYI,35)IBLKL
35    FORMAT(A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      IBLKL=ICHAR(PBUF(1:1))
C35   FORMAT(A1)""")

fix('MOTD', 1, "      DIMENSION MTDTXT(100),TEXT(70)",
    "      DIMENSION MTDTXT(100),TEXT(70)\n" + PORT +
    "SEE PGETLN\n      CHARACTER*80 PBUF")
fix('MOTD', 1, """55    READ(TTYI,56)TEXT,K
56    FORMAT(70A1,A4)""",
    """55    CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,TEXT,70)
      K=PPACK4(PBUF(71:74))
C56   FORMAT(70A1,A4)""")

fix('WIZARD', 1, """      READ (TTYI,12) XD
12    FORMAT (10A1)""",
    """      CALL PGETLN(TTYI,PBUF)
      CALL PUNPK(PBUF,XD,10)
C12   FORMAT (10A1)""")

for member in ('ADVENT', 'ADVENT2', 'ADVWIZ'):
    fix(member, 1, """1004  READ(DBFI,1005)LOC,TEXT,KK
1005  FORMAT(1I8,70A1,A1)""",
        """1004  CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1005)LOC
1005  FORMAT(I8)
      CALL PUNPK(PBUF(9:78),TEXT,70)
      KK=ICHAR(PBUF(79:79))""")
    fix(member, 1, """1043  READ(DBFI,1041)KTAB(TABNDX),(TEXT(I),I=1,5)
1041  FORMAT(I8,5A1)""",
        """1043  CALL PGETLN(DBFI,PBUF)
      READ(PBUF(1:8),1041)KTAB(TABNDX)
1041  FORMAT(I8)
      CALL PUNPK(PBUF(9:13),TEXT,5)""")
    fix(member, 1, "      DIMENSION TEXT(70),FNAME(10),FDUMMY(10)",
        "      DIMENSION TEXT(70),FNAME(10),FDUMMY(10)\n" + PORT +
        "SEE PGETLN\n      CHARACTER*80 PBUF")

# 3. The DD cards.
fix('IOINIT', 1, """      IMPLICIT INTEGER(A-Z)
      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE
      LOGICAL BLKLIN
C
      TTYI=5
      TTYO=6
      DBFI=1
      DBINIT=2
      DBSAVE=3
      BLKLIN=.FALSE.
      RETURN""",
    """      IMPLICIT INTEGER(A-Z)
      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE
      LOGICAL BLKLIN
""" + PORT + """THE JCL SUPPLIED FT01F001 (THE DATABASE), FT02F001 (THE
C  INITIALIZATION FILE ADVWIZ WRITES AND ADVENT READS) AND FT03F001 (THE
C  SUSPEND FILE).  HERE THEY SIT BESIDE THE PROGRAM, EXCEPT SAVED GAMES,
C  WHICH GO IN SAVES\\ AS IN THE OTHER PORTS IN THIS COLLECTION.
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ,PROGID,PDONE
      CHARACTER*260 PDIR
C
      TTYI=5
      TTYO=6
      DBFI=1
      DBINIT=2
      DBSAVE=3
      BLKLIN=.FALSE.
      IF(PDONE.NE.0)RETURN
      PDONE=1
      CALL PGMDIR(PDIR,PDLEN)
      IF(PROGID.NE.1)GOTO 20
      OPEN(DBFI,FILE=PDIR(1:PDLEN)//'advtdata.txt',STATUS='OLD',
     1     ERR=91)
      OPEN(DBINIT,FILE=PDIR(1:PDLEN)//'advent.ini',STATUS='UNKNOWN',
     1     FORM='UNFORMATTED',ERR=92)
      RETURN
20    OPEN(DBINIT,FILE=PDIR(1:PDLEN)//'advent.ini',STATUS='OLD',
     1     FORM='UNFORMATTED',ERR=93)
      IF(PROGID.NE.3)RETURN
      OPEN(DBSAVE,FILE=PDIR(1:PDLEN)//'saves/advent.sav',
     1     STATUS='UNKNOWN',FORM='UNFORMATTED',ERR=94)
      RETURN
91    WRITE(TTYO,95)
95    FORMAT(' PORT: CANNOT OPEN ADVTDATA.TXT')
      STOP
92    WRITE(TTYO,96)
96    FORMAT(' PORT: CANNOT WRITE ADVENT.INI')
      STOP
93    WRITE(TTYO,97)
97    FORMAT(' PORT: CANNOT OPEN ADVENT.INI - RUN ADVWIZ FIRST')
      STOP
94    WRITE(TTYO,98)
98    FORMAT(' PORT: CANNOT OPEN SAVES/ADVENT.SAV')
      STOP""")

# 4. Options, and the wizard's test.
fix('ADVWIZ', 1, "      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE",
    "      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE\n"
    "      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ,PROGID,PDONE")

fix('ADVWIZ', 1, """      CALL IOINIT(0)
      WRITE(TTYO,1000)""",
    PORT + """COMMAND LINE OPTIONS; 1 SAYS THIS IS ADVWIZ.
      CALL POPTS(1)
      CALL IOINIT(0)
      WRITE(TTYO,1000)""")

# ADVWIZ clears MTDTXT to zero, but MOTD walks it as a chain that ends at a
# negative word - ADVENT's own copy of this loop sets -1 - so a game set up
# with no message of the day loops for ever in MOTD before printing its
# first line.  The loop's last statement becomes a CONTINUE so that the
# extra assignment is inside it.
fix('ADVWIZ', 1, """5     MTDTXT(I) = 0
      DO 6 I=1,TRVSIZ""",
    PORT + """FIX: MOTD READS MTDTXT AS A CHAIN ENDED BY A NEGATIVE WORD
C  AND ADVENT'S OWN COPY OF THIS LOOP SETS -1 HERE, SO ZERO HANGS THE
C  GAME IN MOTD.  --no-fixes LEAVES IT AS WRITTEN.
      MTDTXT(I) = 0
      IF(NOFIX.EQ.0)MTDTXT(I) = -1
5     CONTINUE
      DO 6 I=1,TRVSIZ""")

fix('WIZARD', 1, """      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE
C
      WIZARD=YESM(16,0,7)""",
    """      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI,DBINIT,DBSAVE
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ,PROGID,PDONE
""" + PORT + """SEE PGETLN
      CHARACTER*80 PBUF
C
""" + PORT + """ADVWIZ --auto WALKS STRAIGHT IN, SO THAT THE BUILD CAN
C  MAKE THE INITIALIZATION FILE WITHOUT PLAYING THE GUESSING GAME.  THE
C  REAL TEST IS STILL HERE AND STILL WORKS (TESTS\\WIZARD.PY PASSES IT).
      IF(AUTOWZ.NE.0)GOTO 50
      WIZARD=YESM(16,0,7)""")

fix('WIZARD', 1, """99    CALL MSPEAK(20)""",
    """50    WIZARD=.TRUE.
      RETURN
C
99    CALL MSPEAK(20)""")

# 5. ADVENT and ADVENT2.
fix('ADVENT', 1, """      CALL IOINIT(0)
C
C  LOAD 'SYSTEM' COMMON BLOCKS.""",
    PORT + """COMMAND LINE OPTIONS; 2 SAYS THIS IS ADVENT.
      CALL POPTS(2)
      CALL IOINIT(0)
C
C  LOAD 'SYSTEM' COMMON BLOCKS.""")

# ADVENT2 reads the initialization file before IOINIT has set DBINIT, so it
# reads unit 0.  On MVS that wanted a FT00F001 DD card; here it is an error.
fix('ADVENT2', 1, """      READ(DBINIT)RTEXT,LINES,KTAB,ATAB,TABSIZ,ATLOC,LINK,PLACE,""",
    PORT + """COMMAND LINE OPTIONS; 3 SAYS THIS IS THE TEST BUILD.
      CALL POPTS(3)
""" + PORT + """FIX: ADVENT2 READS THE INITIALIZATION FILE BEFORE IOINIT
C  HAS SET DBINIT, SO IT READS UNIT 0 - THE ONE PLACE THIS MEMBER
C  DIFFERS FROM ADVENT AND IS WRONG.  --no-fixes LEAVES IT AS WRITTEN.
      IF(NOFIX.EQ.0)CALL IOINIT(0)
      READ(DBINIT)RTEXT,LINES,KTAB,ATAB,TABSIZ,ATLOC,LINK,PLACE,""")
fix('ADVENT2', 1, """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT""",
    """      LOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,
     1CLOSED,GAVEUP,SCORNG,DEMO,YEA,FORCED,PCT
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ,PROGID,PDONE""")

# The save file is sequential and never rewound: RESTORE reads its first
# record and leaves the file after it, so a SUSPEND after a RESTORE wrote a
# second record and the next RESTORE read the first one again - the newer
# game was lost.  After REWIND the record written is the file's last one.
fix('ADVENT2', 1, """C     CALL ALLOC
      WRITE(DBSAVE)RTEXT,LINES,KTAB,ATAB,TABSIZ,ATLOC,LINK,PLACE,""",
    """C     CALL ALLOC
""" + PORT + """FIX: THE SAVE FILE WAS NEVER REWOUND, SO A SUSPEND AFTER
C  A RESTORE ADDED A SECOND GAME THAT THE NEXT RESTORE NEVER READ.
C  --no-fixes LEAVES IT AS WRITTEN.
      IF(NOFIX.EQ.0)REWIND DBSAVE
      WRITE(DBSAVE)RTEXT,LINES,KTAB,ATAB,TABSIZ,ATLOC,LINK,PLACE,""")

# 6. CARRY walks the list of things at a location until it finds the
#    object.  Handed a location the object is not at, it reached the end of
#    the list (0) and read LINK(0), outside the array; on MVS the walk went
#    on through whatever lay before LINK in /PLACOM/ (it hung the first
#    probe).  It now stops at the end of the list, and at a location outside
#    ATLOC, whatever the options.
fix('CARRY', 1, """5     IF(ATLOC(WHERE).NE.OBJECT)GOTO 6
      ATLOC(WHERE)=LINK(OBJECT)
      RETURN
6     TEMP=ATLOC(WHERE)
7     IF(LINK(TEMP).EQ.OBJECT)GOTO 8""",
    PORT + """NOT A LOCATION, OR THE OBJECT NOT IN ITS LIST: NOTHING
C  TO TAKE IT OUT OF (THE WALK BELOW WENT PAST THE END OF LINK).
5     IF(WHERE.LT.1.OR.WHERE.GT.150)RETURN
      IF(ATLOC(WHERE).NE.OBJECT)GOTO 6
      ATLOC(WHERE)=LINK(OBJECT)
      RETURN
6     TEMP=ATLOC(WHERE)
7     IF(TEMP.EQ.0)RETURN
      IF(LINK(TEMP).EQ.OBJECT)GOTO 8""")


def main():
    if not os.path.isdir(OUT):
        os.makedirs(OUT)
    for member in MEMBERS:
        text = load(member)
        for count, old, new in FIXUPS.get(member, ()):
            n = text.count(old)
            if n != count:
                sys.exit('convert: %s: pattern found %d times, expected %d:\n%s'
                         % (member, n, count, old[:70]))
            text = text.replace(old, new)
        for i, line in enumerate(text.split('\n')):
            if len(line) > 72 and not line[:1] in ('C', '*', 'c'):
                sys.exit('convert: %s line %d runs past column 72:\n%s'
                         % (member, i + 1, line))
        out = os.path.join(OUT, member.lower() + '.f')
        open(out, 'w', encoding='latin-1', newline='\n').write(text + '\n')
    print('convert: %d members -> %s' % (len(MEMBERS), os.path.normpath(OUT)))


if __name__ == '__main__':
    main()
