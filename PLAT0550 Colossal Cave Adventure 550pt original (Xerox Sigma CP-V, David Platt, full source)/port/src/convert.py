#!/usr/bin/env python3
"""Build the port's sources from the LADC_0012 tape.

Reads archive_original/LADC_0012/ladc012_ers.tap.gz itself (the LADC zip's
ASCII conversion drops every line that crosses a tape block - see
cpvtape.py) and writes, under .build/:

  cave/COMPILE_CAVE.kyd   the munger's M:SI input: COMPILE_CAVE's !DATA deck
  cave/D_*.kyd            the cave's D: files, as keyed files, line keys kept
  src/munge.f             MUNGESI + MUNGE:C, the database translator
  src/adv.f               ADVSI + ADV:C, the interpreter

The FORTRAN is Xerox Sigma ANS FORTRAN (CP-V).  What gfortran needs done
to it, beyond the I/O and helper-routine edits listed in EDITS_*:

  * INCLUDE is expanded; its IMPLICIT and PARAMETER statements go straight
    after each unit's first line (ANSF took them anywhere), and
    PARAMETER X=V gets its parentheses;
  * identifiers may begin with $ (IMPLICIT CHARACTER*6 ($) in the
    interpreter, *12 in the munger): $NAME becomes S_NAME, declared
    CHARACTER*n explicitly where the IMPLICIT gave it its type;
  * a FORMAT literal may be delimited by $...$ inside an apostrophe
    string: '($ Ok.$)' is FORMAT (' Ok.');
  * REPEAT n, WHILE (c) is DO n, WHILE (c); .EOR. is .NEQV.;
    PRINT (u, f) is WRITE (u, f); &n in an argument list is *n;
  * IF (c) s1; s2 makes both statements conditional (the Prime version of
    the munger writes the one in BUFFWRITE as two IFs) - the two such
    lines become IF blocks;
  * names count to 8 characters: REHASHVALUE is REHASHVALUES, FASTWRITE
    is the assembler routine FASTWRIT;
  * a 'Z' in column 1 is a debugging line, left out;
  * columns 73-80 are the card's sequence field (one FORMAT has a comma
    there, which ANSF never saw).

Character comparisons that order names (the munger's heap sort and option
search, the interpreter's binary search of the vocabulary) are made in
EBCDIC order through PECMP, so the vocabulary sorts as it did on the Sigma
- which decides, among other things, which synonym NAME prints.

Every edit asserts how often it matches.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..', '..'))
TAPE = os.path.join(GAME, 'archive_original', 'LADC_0012',
                    'ladc012_ers.tap.gz')
OUT = os.path.join(HERE, '..', '.build')

sys.path.insert(0, HERE)
import cpvtape  # noqa: E402

CAVE_MEMBERS = ['D:NULLS', 'D:BITS', 'D:PLACE', 'D:OBJECTS', 'D:OBJSYN',
                'D:VERBS', 'D:TEXT', 'D:VARS', 'D:LABELS', 'D:MOVES',
                'D:DEFINE', 'D:INIT', 'D:REPEAT', 'D:ACTION']


# ----------------------------------------------------------------------
# edits: (old line(s), new lines, expected count), applied to the source
# as it is on the tape (columns 1-72)
# ----------------------------------------------------------------------

EDITS_ADV = [
    # the main program becomes a subroutine behind the port's PROGRAM
    ('      INCLUDE ADV:C\n      WRITE (OUTUNIT, \'(1X)\')',
     ['      SUBROUTINE ADVMAIN', '      INCLUDE ADV:C',
      "      WRITE (OUTUNIT, '(1X)')"], 1),

    # FIND has an alternate return: a function cannot, in gfortran
    ('      INTEGER FUNCTION FIND($ID, *)',
     ['      INTEGER FUNCTION FIND($ID, IALT)'], 1),
    ("      $IDE = $ID // BLANKS(1: 140 - LEN($ID))",
     ['      IALT = 0', "      $IDE = $ID // BLANKS(1: 140 - LEN($ID))"], 1),
    ('      IF (LOW .GE. HIGH) $BADVAR=$IDE; RETURN 1',
     ['      IF (LOW .GE. HIGH) THEN', '      $BADVAR=$IDE', '      IALT = 1',
      '      RETURN', '      END IF'], 1),
    ('      LINEVAL = FIND($LEX, &50)',
     ['      LINEVAL = FIND($LEX, IALT)', '      IF (IALT .NE. 0) GOTO 50'], 1),

    # the terminal
    ("5     READ (INUNIT, '(A139)', END=999) $LINE",
     ['5     CALL PTIN($LINE, 139, &999)'], 1),
    ("      READ (INUNIT, '(A1)', END=7120, ERR=7120) $RESP",
     ['      CALL PTIN($RESP, 1, &7120)'], 1),

    # READTEXT: READ (A140) by key, INQUIRE RECSIZE
    ('      READ (UNIT=DBT, FMT=10, KEY=KEY, ERR=100) $TEXT\n'
     '      INQUIRE(UNIT=DBT,RECSIZE=RSIZE)',
     ['      CALL KRTEXT(DBT, KEY, $TEXT, RSIZE, &100)'], 1),
    ('10    FORMAT (A140)', ['C10   FORMAT (A140)'], 1),
    # READBUFF: (1024R2) by key; CACHEOK's subscript is a REAL; the cache
    ('      READ (UNIT=DBI, FMT=20, KEY=KEY, ERR=199) RSIZE,\n'
     '     + (BUFFER(I), I=1, MIN(RSIZE, BUFFSIZE-1))',
     ['      CALL KRHALF(DBI, KEY, RSIZE, BUFFER, BUFFSIZE-1, &199)'], 1),
    ('181   BUFFER(I) = ISA(ISL(BUFFER(I), 16), -16)',
     ['181   BUFFER(I) = ISA(ISL(BUFFER(I), 16), -16)',
      '      CALL PPATCH(KEY, BUFFER, RSIZE)'], 1),
    ('      IF (.NOT. CACHEOK(KEY/1E6)) GOTO 200',
     ['      IF (.NOT. CACHEOK(KEY/1000000)) GOTO 200'], 1),
    ('20    FORMAT (1024R2)', ['C20   FORMAT (1024R2)'], 1),
    ("999   FORMAT (' Glitch! Buffer too small: ',2I)",
     ["999   FORMAT (' Glitch! Buffer too small: ',2I12)"], 1),
    ("      CALL GET$(&18, KEY, BUFFER)", ["      CALL CGET(&18, KEY, BUFFER)"], 1),
    ("      CALL ADD$(&19, KEY, BUFFER)", ["      CALL CADD(&19, KEY, BUFFER)"], 1),
    ("      CALL START$(&80)", ["      CALL CSTART(&80)"], 1),
    ("      CALL START$(&410)", ["      CALL CSTART(&410)"], 1),
    ("      CALL CLEAR$", ["      CALL CCLEAR"], 1),

    # WEBSTER: the vocabulary, (R4) and (200(A6,R2)) records
    ('      READ (UNIT=DBI, FMT=100, KEY=9000*1000) SYMCNT',
     ['      CALL KPRDI(DBI, 9000*1000, IFND)', '      CALL KPGET4(SYMCNT)'], 1),
    ('      READ (UNIT=DBI, FMT=200, KEY=KEY*1000)\n'
     '     + ($WORD(J), FILEKEY(J), J=I, MIN(I+199, SYMCNT))',
     ['      CALL KPRDI(DBI, KEY*1000, IFND)',
      '      DO 90010 J=I, MIN(I+199, SYMCNT)',
      '      CALL KPGETC($WORD(J), 6)',
      '90010 CALL KPGET(FILEKEY(J))'], 1),
    ('100   FORMAT (R4)', ['C100  FORMAT (R4)'], 1),
    ('200   FORMAT (200(A6,R2))', ['C200  FORMAT (200(A6,R2))'], 1),

    # OPENDB
    ("      OPEN (UNIT=DBI, NAME='ADVI',ACCOUNT=$ACCT,\n"
     "     + USAGE='INPUT', STATUS='OLD', RECL=2048,\n"
     "     + FORM='FORMATTED', ACCESS='KEYED', ERR=100)",
     ["      CALL KOPENN(DBI, 'ADVI', 1, IERR)",
      '      IF (IERR .NE. 0) GOTO 100'], 1),
    ("      OPEN (UNIT=DBT, NAME='ADVT',ACCOUNT=$ACCT,\n"
     "     + USAGE='INPUT', STATUS='OLD', RECL=2048,\n"
     "     + FORM='FORMATTED', ACCESS='KEYED', ERR=100)",
     ["      CALL KOPENN(DBT, 'ADVT', 1, IERR)",
      '      IF (IERR .NE. 0) GOTO 100'], 1),
    ('100   INQUIRE (UNIT=BADUNIT, ERRCODE=OOPS)',
     ['100   OOPS = KERR(BADUNIT)'], 1),

    # EXECUTIVE: SAVE / RESTORE / DELETE in the keyed save file
    ("      OPEN (UNIT=FREEZER, NAME='*ADVFREEZE', ACCOUNT=$ACCT,\n"
     "     + USAGE = 'UPDATE', ACCESS='KEYED', STATUS='OLD', ERR=150,\n"
     "     + RECL=MAXREC, FORM='FORMATTED')",
     ["      CALL KOPENN(FREEZER, '*ADVFREEZE', 3, IERR)",
      '      IF (IERR .NE. 0) GOTO 150'], 1),
    ('      WRITE (UNIT=FREEZER, FMT=999, KEY=$KEY) CHECKSUM,\n'
     '     + (OBJLOC(I), OBJVAL(I), OBJBIT(I), I=0, NOBJ),\n'
     '     + (VARVAL(I), VARBIT(I), I=0, NVARS),\n'
     '     + (PLACEBIT(I), I=0, NPLACE)',
     ['      CALL KPBEG',
      '      CALL KPPUT(CHECKSUM)',
      '      DO 90020 I=0, NOBJ',
      '      CALL KPPUT(OBJLOC(I))',
      '      CALL KPPUT(OBJVAL(I))',
      '90020 CALL KPPUT(OBJBIT(I))',
      '      DO 90021 I=0, NVARS',
      '      CALL KPPUT(VARVAL(I))',
      '90021 CALL KPPUT(VARBIT(I))',
      '      DO 90022 I=0, NPLACE',
      '90022 CALL KPPUT(PLACEBIT(I))',
      '      CALL KPWRC(FREEZER, $KEY)'], 1),
    ("140   CLOSE (UNIT=FREEZER,STATUS='KEEP')",
     ['140   CALL KCLOSE(FREEZER)'], 1),
    ('      INQUIRE (UNIT=FREEZER, ERRCODE=I)', ['      I = KERR(FREEZER)'], 1),
    ('      IF (I .NE. 4Z0300 .OR. $ACCT .NE. $MYACCT) GOTO 9999',
     ['      IF (I .NE. 768 .OR. $ACCT .NE. $MYACCT) GOTO 9999'], 1),
    ("      OPEN (UNIT=FREEZER, NAME='*ADVFREEZE',\n"
     "     + ACCESS='KEYED', USAGE='OUTPUT', STATUS='NEW', KEYM=12,\n"
     "     + FORM='FORMATTED', RECL=MAXREC)\n"
     "      CLOSE (UNIT=FREEZER, STATUS='KEEP')",
     ["      CALL KOPENN(FREEZER, '*ADVFREEZE', 2, IERR)",
      '      CALL KCLOSE(FREEZER)'], 1),
    ('999   FORMAT(5000R2)', ['C999  FORMAT(5000R2)'], 1),
    ("      OPEN (UNIT=FREEZER, NAME='*ADVFREEZE', ACCOUNT=$ACCT,\n"
     "     + USAGE = 'INPUT',  ACCESS='KEYED', STATUS='OLD', ERR=9999,\n"
     "     + RECL=MAXREC)",
     ["      CALL KOPENN(FREEZER, '*ADVFREEZE', 1, IERR)",
      '      IF (IERR .NE. 0) GOTO 9999'], 1),
    ('      READ (UNIT=FREEZER, FMT=999, KEY=$KEY, ERR=220) CHECK',
     ['      CALL KPRDC(FREEZER, $KEY, IFND)',
      '      IF (IFND .EQ. 0) GOTO 220',
      '      CALL KPGET(CHECK)'], 1),
    ('      READ  (UNIT=FREEZER, FMT=999, KEY=$KEY) CHECKSUM,\n'
     '     + (OBJLOC(I), OBJVAL(I), OBJBIT(I), I=0, NOBJ),\n'
     '     + (VARVAL(I), VARBIT(I), I=0, NVARS),\n'
     '     + (PLACEBIT(I), I=0, NPLACE)',
     ['      CALL KPRDC(FREEZER, $KEY, IFND)',
      '      CALL KPGET(CHECKSUM)',
      '      DO 90030 I=0, NOBJ',
      '      CALL KPGET(OBJLOC(I))',
      '      CALL KPGET(OBJVAL(I))',
      '90030 CALL KPGET(OBJBIT(I))',
      '      DO 90031 I=0, NVARS',
      '      CALL KPGET(VARVAL(I))',
      '90031 CALL KPGET(VARBIT(I))',
      '      DO 90032 I=0, NPLACE',
      '90032 CALL KPGET(PLACEBIT(I))'], 1),
    ("      OPEN (UNIT=FREEZER, NAME='*ADVFREEZE', ACCOUNT=$ACCT,\n"
     "     + USAGE = 'UPDATE', ACCESS='KEYED', STATUS='OLD', ERR=9999,\n"
     "     + RECL=MAXREC, FORM='FORMATTED')",
     ["      CALL KOPENN(FREEZER, '*ADVFREEZE', 3, IERR)",
      '      IF (IERR .NE. 0) GOTO 9999'], 1),
    ("      WRITE (UNIT=FREEZER, KEY=$KEY, FMT='()')",
     ['      CALL KDELC(FREEZER, $KEY)'], 1),

    # STOP: the port's, which flushes the terminal and says what CP-V said
    ('      STOP 0', ["      CALL PSTOP('0')"], 1),
    ("999   STOP 'Eof'", ["999   CALL PSTOP('Eof')"], 1),
    ("      STOP 'Help!'", ["      CALL PSTOP('Help!')"], 1),
    ("      IF (NOBJ .GT. OBJECTS) STOP 'Too many objects'",
     ["      IF (NOBJ .GT. OBJECTS) CALL PSTOP('Too many objects')"], 1),
    ("      IF (NPLACE .GT. PLACES) STOP 'Too many places'",
     ["      IF (NPLACE .GT. PLACES) CALL PSTOP('Too many places')"], 1),
    ("      IF (NVARS .GT. VARS) STOP 'Too many vars'",
     ["      IF (NVARS .GT. VARS) CALL PSTOP('Too many vars')"], 1),
    ("      STOP ' '", ["      CALL PSTOP(' ')"], 1),
    ("      STOP 'Gaah!'", ["      CALL PSTOP('Gaah!')"], 1),

    # EBCDIC order for the vocabulary's binary search
    ('      IF ($IDE .LT. $WORD(FIND)) GOTO 20',
     ['      IF (PECMP($IDE, $WORD(FIND)) .LT. 0) GOTO 20'], 1),
]

# the twelve FIND(..., &9999) calls in WEBSTER
for _v, _w in [('HERE', 'HERE'), ('THERE', 'THERE'), ('STATUS', 'STATUS'),
               ('ARG1', 'ARG1'), ('ARG2', 'ARG2'), ('NOBJ', 'NOBJ'),
               ('NPLACE', 'NPLACE'), ('NREP', 'NREP'), ('NINIT', 'NINIT'),
               ('NVARS', 'NVARS'), ('SAYXX', 'SAY')]:
    EDITS_ADV.append(("      %s = FIND('%s', &9999)" % (_v, _w),
                      ["      %s = FIND('%s', IALT)" % (_v, _w),
                       '      IF (IALT .NE. 0) GOTO 9999'], 1))
EDITS_ADV.append(("      EXPLORE = FIND ('EXPLORE', &9999)",
                  ["      EXPLORE = FIND ('EXPLORE', IALT)",
                   '      IF (IALT .NE. 0) GOTO 9999'], 1))

EDITS_MUNGE = [
    ('      INCLUDE MUNGE:C\n      CHARACTER*11 $DB, $FILENAME',
     ['      SUBROUTINE MUNGEMAIN', '      INCLUDE MUNGE:C',
      '      CHARACTER*11 $DB, $FILENAME'], 1),
    # the keyed output files
    ("      OPEN (UNIT=TEXTFILE,STATUS='UNKNOWN',ACCESS='KEYED',NAME='ADVT',\n"
     "     + FORM='FORMATTED',KEYM=3,USAGE='OUTPUT',RECL=2048)",
     ["      CALL KOPENN(TEXTFILE, 'ADVT', 2, IERR)"], 1),
    ("      OPEN (UNIT=INSTFILE,STATUS='UNKNOWN',ACCESS='KEYED',NAME='ADVI',\n"
     "     + FORM='FORMATTED',KEYM=3,USAGE='OUTPUT',RECL=2048)",
     ["      CALL KOPENN(INSTFILE, 'ADVI', 2, IERR)"], 1),
    # READIN's alternate returns are ASSIGNed labels
    ('      ASSIGN 300 TO CONTINUE', ['      ASSIGN 300 TO KCONT'], 2),
    ('      ASSIGN 400 TO NEW', ['      ASSIGN 400 TO KNEW'], 2),
    ('1000  ASSIGN 1300 TO CONTINUE', ['1000  ASSIGN 1300 TO KCONT'], 1),
    ('2950  ASSIGN 3000 TO CONTINUE', ['2950  ASSIGN 3000 TO KCONT'], 1),
    ('      ASSIGN 3100 TO NEW', ['      ASSIGN 3100 TO KNEW'], 1),
    ('200   CALL READIN(CONTINUE, NEW)',
     ['200   CALL READIN(&90001, &90002)',
      '90001 GOTO KCONT', '90002 GOTO KNEW'], 1),
    # the INCLUDE file
    ('      CLOSE(UNIT=ININCLUDE)', ['      CALL FRCLOS(ININCLUDE)'], 1),
    ("      OPEN (UNIT=ININCLUDE,NAME=$FID,ERR=4100,USAGE='INPUT',\n"
     "     + ACCESS='SEQUENTIAL')",
     ['      CALL FROPEN(ININCLUDE, $FID, IERR)',
      '      IF (IERR .NE. 0) GOTO 4100'], 1),
    ('      REWIND 104', ['      CALL FRREW(104)'], 1),
    # the vocabulary: (R4) and (200(A6,R2)) records
    ('      WRITE (UNIT=INSTFILE, FMT=9001, KEY = 9000 * 1000) SYMCNT + 1',
     ['      CALL KPBEG', '      CALL KPPUT4(SYMCNT + 1)',
      '      CALL KPWRI(INSTFILE, 9000 * 1000)'], 1),
    ('9001  FORMAT (R4)', ['C9001 FORMAT (R4)'], 1),
    ('      WRITE (UNIT=INSTFILE, FMT=9100, KEY = KEY * 1000)\n'
     '     + ($NAME(K), KEYS(K), K = I1, J)',
     ['      CALL KPBEG', '      DO 90010 K = I1, J',
      '      CALL KPPUTC($NAME(K), 6)', '90010 CALL KPPUT(KEYS(K))',
      '      CALL KPWRI(INSTFILE, KEY * 1000)'], 1),
    ('9100  FORMAT(200(A6,R2))', ['C9100 FORMAT(200(A6,R2))'], 1),
    ('      CLOSE (UNIT=TEXTFILE)', ['      CALL KCLOSE(TEXTFILE)'], 1),
    ('      CLOSE (UNIT=INSTFILE)', ['      CALL KCLOSE(INSTFILE)'], 1),
    # the listing's I without a width
    ("9200  FORMAT ('1'/' Maximum buffer length',T30,I//\n"
     "     +  ' Vocabulary size',T30,I//\n"
     "     +  ' Directory entries',T30,I/' Directory pages',T30,I//\n"
     "     +  ' Data entries',T30,I/' Data pages',T30,I)",
     ["9200  FORMAT ('1'/' Maximum buffer length',T30,I12//",
      "     +  ' Vocabulary size',T30,I12//",
      "     +  ' Directory entries',T30,I12/' Directory pages',T30,I12//",
      "     +  ' Data entries',T30,I12/' Data pages',T30,I12)"], 1),
    # SWAP assigns to a variable of its own name
    ('      SWAP = KEYS(K)', ['      KSWAP = KEYS(K)'], 1),
    ('      KEYS(I) = SWAP', ['      KEYS(I) = KSWAP'], 1),
    # instruction records: (1024R2)
    ('10    WRITE (UNIT=INSTFILE, FMT=20, KEY=KEY) LEN, (BUFFER(I), I=1, LEN)',
     ['10    CALL KPBEG', '      CALL KPPUT(LEN)', '      DO 90020 I=1, LEN',
      '90020 CALL KPPUT(BUFFER(I))', '      CALL KPWRI(INSTFILE, KEY)'], 1),
    ('20    FORMAT(1024R2)', ['C20   FORMAT(1024R2)'], 1),
    ('      IF (KEY/1000000 .GT. 0) CACHEHW = CACHEHW + LEN + 1;\n'
     '     +                        CACHEDW = CACHEDW + 1',
     ['      IF (KEY/1000000 .GT. 0) THEN',
      '      CACHEHW = CACHEHW + LEN + 1', '      CACHEDW = CACHEDW + 1',
      '      END IF'], 1),
    # the tab character
    ("      WRITE ($TAB, '(R1)') 5", ['      $TAB = CHAR(9)'], 1),
    # the subroutine WRITE; FASTWRIT
    ('1350  CALL WRITE(KEY, $LINE(LINEX:LINEND))',
     ['1350  CALL WRTEXT(KEY, $LINE(LINEX:LINEND))'], 1),
    ('      SUBROUTINE WRITE(IOKEY, $TEXT)',
     ['      SUBROUTINE WRTEXT(IOKEY, $TEXT)'], 1),
    # the names are significant to 8 characters
    ('      REHASH = REHASHVALUE(MOD(FIND, REHASHSIZE))',
     ['      REHASH = REHASHVALUES(MOD(FIND, REHASHSIZE))'], 1),
    ('      REHASH = REHASHVALUE(MOD(I, REHASHN))',
     ['      REHASH = REHASHVALUES(MOD(I, REHASHN))'], 1),
    ('50    IF (.NOT. RPRIME(I, REHASHVALUE(J))) GOTO 100',
     ['50    IF (.NOT. RPRIME(I, REHASHVALUES(J))) GOTO 100'], 1),
    ('      REHASHVALUE(REHASHN) = I', ['      REHASHVALUES(REHASHN) = I'], 1),
    # the terminal is unit 6 here
    ('     + /116, 117, 105, 104, 108/', ['     + /116, 117, 105, 104, 6/'], 1),
    # EBCDIC order: the symbol table's heap sort and the option table
    ('      IF ($NAME(K) .LT. $NAME(J)) K = J',
     ['      IF (PECMP($NAME(K), $NAME(J)) .LT. 0) K = J'], 1),
    ('      IF ($LEX(1:4) .GT. $OPT(ISAM)) GOTO 3007',
     ['      IF (PECMP($LEX(1:4), $OPT(ISAM)) .GT. 0) GOTO 3007'], 1),
    ('14    IF ($OPT(J) .LT. $OPT(K)) K=J',
     ['14    IF (PECMP($OPT(J), $OPT(K)) .LT. 0) K=J'], 1),
]


def edit(lines, edits):
    text = '\n'.join(lines) + '\n'
    for old, new, count in edits:
        pat = old + '\n'
        n = 0
        start = 0
        while True:
            i = text.find(pat, start)
            if i < 0:
                break
            if i == 0 or text[i - 1] == '\n':
                n += 1
            start = i + 1
        if n != count:
            raise SystemExit('edit matched %d times, expected %d:\n%s'
                             % (n, count, old))
        out = []
        pos = 0
        while True:
            i = text.find(pat, pos)
            if i < 0:
                out.append(text[pos:])
                break
            if i == 0 or text[i - 1] == '\n':
                out.append(text[pos:i])
                out.append('\n'.join(new) + '\n')
                pos = i + len(pat)
            else:
                out.append(text[pos:i + 1])
                pos = i + 1
        text = ''.join(out)
    return text.rstrip('\n').split('\n')


# ----------------------------------------------------------------------
# fixed-form helpers
# ----------------------------------------------------------------------

def is_comment(l):
    return l[:1] in ('C', 'c', '*') or l.strip() == ''


def is_cont(l):
    return (not is_comment(l) and len(l) > 5 and l[:5].strip() == ''
            and l[5] not in (' ', '0'))


STR = re.compile(r"'(?:[^']|'')*'")


def segments(l):
    """Split a statement line's text into (is_string, text) pieces."""
    out = []
    pos = 0
    for m in STR.finditer(l):
        out.append((False, l[pos:m.start()]))
        out.append((True, m.group(0)))
        pos = m.end()
    rest = l[pos:]
    if "'" in rest:
        raise SystemExit('unbalanced quote: ' + l)
    out.append((False, rest))
    return out


def code_sub(lines, pat, rep, count, what):
    """re.sub on the code (non-string) part of statement lines."""
    n = 0
    out = []
    for l in lines:
        if is_comment(l):
            out.append(l)
            continue
        segs = []
        for s, t in segments(l):
            if not s:
                t, k = re.subn(pat, rep, t)
                n += k
            segs.append(t)
        out.append(''.join(segs))
    if count is not None and n != count:
        raise SystemExit('%s: %d replacements, expected %d' % (what, n, count))
    return out


def convert_format_strings(lines, count):
    """'($ Ok.$)' -> '('' Ok.'')': $...$ is a literal inside the format."""
    n = 0
    out = []
    for l in lines:
        if is_comment(l):
            out.append(l)
            continue
        segs = []
        for s, t in segments(l):
            if s and t.startswith("'(") and '$' in t:
                v = t[1:-1].replace("''", "'")
                v2 = re.sub(r'\$([^$]*)\$',
                            lambda m: "'" + m.group(1).replace("'", "''") + "'",
                            v)
                assert '$' not in v2, t
                t = "'" + v2.replace("'", "''") + "'"
                n += 1
            segs.append(t)
        out.append(''.join(segs))
    if n != count:
        raise SystemExit('format strings: %d, expected %d' % (n, count))
    return out


HEADER = re.compile(r'^ {6}\s*((INTEGER|LOGICAL|REAL|CHARACTER\S*|DOUBLE PRECISION)'
                    r'\s+)?(SUBROUTINE|FUNCTION|BLOCK DATA|PROGRAM)\b')


def split_units(lines):
    units = []
    cur = []
    for l in lines:
        cur.append(l)
        if not is_comment(l) and re.match(r'^ {6}\s*END\s*$', l):
            units.append(cur)
            cur = []
    if any(not is_comment(l) for l in cur):
        raise SystemExit('text after the last END')
    return units


def expand_includes(lines, inc, incname):
    """INCLUDE -> the include's lines; its IMPLICIT and PARAMETER
    statements move up to just after the unit's header."""
    part1 = []
    params = []
    part2 = []
    for l in inc:
        if is_comment(l):
            continue
        if re.match(r'^ {6}\s*IMPLICIT\b', l):
            part1.append(l)
        elif re.match(r'^ {6}\s*PARAMETER\b', l):
            # after the IMPLICITs: ANSF typed a constant by the IMPLICIT
            # wherever it stood (VOCAB is an integer), gfortran by the
            # rules in force at the PARAMETER statement
            params.append(l)
        else:
            part2.append(l)
    part1 += params
    out = []
    ninc = 0
    for u in split_units(lines):
        if not any(re.match(r'^ {6}\s*INCLUDE\s+' + re.escape(incname)
                            + r'(,LIST)?\s*$', l) for l in u):
            out += u
            continue
        hdr = next(i for i, l in enumerate(u) if not is_comment(l))
        if not HEADER.match(u[hdr]):
            raise SystemExit('unit without a header: ' + u[hdr])
        v = u[:hdr + 1] + part1 + ['C     (%s)' % incname]
        for l in u[hdr + 1:]:
            if re.match(r'^ {6}\s*INCLUDE\s+', l):
                v += part2
                ninc += 1
            else:
                v.append(l)
        out += v
    return out, ninc


def rename_dollars(lines, clen):
    """$NAME -> S_NAME; declare CHARACTER*clen what the IMPLICIT typed."""
    out = []
    for u in split_units(lines):
        u = code_sub(u, r'\$([A-Z][A-Z0-9]*)', r'S_\1', None, '$ names')
        used = []
        declared = set()
        stmt = []
        for l in u:
            if is_comment(l):
                continue
            if is_cont(l):
                stmt.append(l)
            else:
                stmt = [l]
            head = stmt[0][6:].lstrip()
            names = re.findall(r'\bS_[A-Z0-9]+\b',
                               ''.join(t for s, t in segments(l) if not s))
            for x in names:
                if x not in used:
                    used.append(x)
                if head.startswith('CHARACTER'):
                    declared.add(x)
        need = [x for x in used if x not in declared]
        if need:
            # after the IMPLICIT/PARAMETER lines that follow the header
            hdr = next(i for i, l in enumerate(u) if not is_comment(l))
            j = hdr + 1
            while j < len(u) and (is_comment(u[j]) or re.match(
                    r'^ {6}\s*(IMPLICIT|PARAMETER)\b', u[j])):
                j += 1
            decl = []
            line = '      CHARACTER*%d ' % clen
            for x in need:
                if len(line) + len(x) + 1 > 70:
                    decl.append(line.rstrip(','))
                    line = '      CHARACTER*%d ' % clen
                line += x + ','
            decl.append(line.rstrip(','))
            u = u[:j] + decl + u[j:]
        out += u
    return out


def adv_output(lines, count):
    """WRITE/PRINT (OUTUNIT, f) list -> an internal WRITE and PTOUT."""
    out = []
    n = 0
    pat = re.compile(r"^(.{6})(\s*)(WRITE|PRINT)\s*\(\s*OUTUNIT\s*,\s*"
                     r"('(?:[^']|'')*'|\d+)\s*\)\s*(.*)$")
    for i, l in enumerate(lines):
        m = None if is_comment(l) else pat.match(l)
        if not m:
            out.append(l)
            continue
        if i + 1 < len(lines) and is_cont(lines[i + 1]):
            raise SystemExit('continued output statement: ' + l)
        out.append('%sWRITE (PTBUF, %s) %s' % (m.group(1), m.group(4),
                                               m.group(5)))
        out.append('      CALL PTOUT(PTBUF)')
        n += 1
    if n != count:
        raise SystemExit('output statements: %d, expected %d' % (n, count))
    return out


def tidy(lines):
    """PARAMETER X=V -> PARAMETER (X=V); drop IMPLICIT CHARACTER*n ($)."""
    out = []
    clen = None
    for l in lines:
        m = re.match(r'^( {6}\s*)PARAMETER\s+([A-Z][A-Z0-9]*)\s*=\s*(\S+)\s*$', l)
        if m:
            out.append('%sPARAMETER (%s=%s)' % m.groups())
            continue
        m = re.match(r'^ {6}\s*IMPLICIT\s+CHARACTER\*(\d+)\s*\(\$\)\s*$', l)
        if m:
            clen = int(m.group(1))
            continue
        out.append(l)
    return out, clen


def fortran(ms, prog, inc, incname, edits, counts, extra_decl=()):
    src = source_lines(ms, prog)
    inc_l = source_lines(ms, inc)
    # 'Z' in column 1: debugging lines
    src = ['C' + l if l[:1] == 'Z' else l for l in src]
    src = edit(src, edits)
    inc_l = list(inc_l) + list(extra_decl)
    src, ninc = expand_includes(src, inc_l, incname)
    if ninc != counts['include']:
        raise SystemExit('%s: %d INCLUDEs, expected %d'
                         % (prog, ninc, counts['include']))
    src, clen = tidy(src)
    assert clen, 'no IMPLICIT CHARACTER ($)'
    src = code_sub(src, r'\bREPEAT\s+(\d+)\s*,\s*WHILE\b', r'DO \1, WHILE',
                   counts['repeat'], 'REPEAT WHILE')
    src = code_sub(src, r'\.EOR\.', '.NEQV.', counts['eor'], '.EOR.')
    src = code_sub(src, r'\bPRINT\s*\(', 'WRITE (', counts['print'], 'PRINT')
    src = code_sub(src, r'&(\d+)', r'*\1', counts['altret'], '&label')
    src = convert_format_strings(src, counts['formats'])
    src = rename_dollars(src, clen)
    return src


def source_lines(ms, name):
    """A FORTRAN member as text, cut at column 72."""
    out = []
    for key, b in cpvtape.keyed_records(ms[name][1]):
        t = cpvtape.internal(b).decode('latin-1').rstrip()
        if not t[:1] in ('C', '*'):
            t = t[:72].rstrip()
        out.append(t)
    return out


# the port's routines that need the interpreter's common blocks
ADV_EXTRA = r"""
C     ==================================================================
C     PPATCH - the port's -u (see pcpv.f), which needs the interpreter's
C     common blocks.  Two constants in the cave's code are patched as
C     their records are read (and so as they enter the cache):
C
C       the first REPEAT record (6000.000): SET <variable>,600 - SET
C       I,MAX.GAME, the 600-move limit - gets -1: MOVES counts up from
C       0 and never equals it;
C
C       the RESTORE verb's code: IFGT <variable>,30 - the minutes since
C       the SAVE against MINTIME - gets -1, which any time exceeds.
C
C     (A constant in the code runs from -32768 to 999; 1000 up are
C     references.)  Each record must hold exactly one such instruction.
      SUBROUTINE PPATCH(KEY, BUF, N)
      INCLUDE ADV:C
      INTEGER KEY, BUF(*), N, KVERB, IOP, IVAL
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      DATA KVERB/0/
      IF (PUNLIM .EQ. 0) RETURN
      IF (KVERB .EQ. 0) THEN
         KVERB = FIND('RESTORE', IALT)
         IF (IALT .NE. 0) KVERB = -1
      END IF
      IF (KEY .EQ. REPEAT * 1000000) THEN
         IOP = 17
         IVAL = 600
      ELSE IF (KEY / 1000 .EQ. KVERB) THEN
         IOP = 8
         IVAL = 30
      ELSE
         RETURN
      END IF
      J = 0
      DO 10 I = 1, N - 2
      IF (BUF(I) .EQ. IOP .AND. BUF(I+1) / 1000 .EQ. VARIABLE
     +    .AND. BUF(I+2) .EQ. IVAL) THEN
         IF (J .NE. 0) GOTO 90
         J = I + 2
      END IF
10    CONTINUE
      IF (J .EQ. 0) GOTO 90
      BUF(J) = -1
      RETURN
90    CALL PTOUT(' (-u: a limit to lift was not found)')
      RETURN
      END
"""


def keyed_file(path, recs):
    """Write the port's keyed-file format: records sorted by key."""
    recs = sorted(recs)
    with open(path, 'wb') as f:
        f.write(b'CPVKEYED' + len(recs).to_bytes(4, 'big'))
        for key, data in recs:
            k = key.to_bytes(3, 'big')
            f.write(bytes([3]) + k + len(data).to_bytes(2, 'big') + data)


def main():
    ms = cpvtape.members(TAPE)
    os.makedirs(os.path.join(OUT, 'cave'), exist_ok=True)
    os.makedirs(os.path.join(OUT, 'src'), exist_ok=True)

    # the cave: the D: files, and COMPILE_CAVE's !DATA deck as M:SI
    for m in CAVE_MEMBERS:
        recs = [(k, cpvtape.internal(b))
                for k, b in cpvtape.keyed_records(ms[m][1])]
        keyed_file(os.path.join(OUT, 'cave', m.replace(':', '_') + '.kyd'),
                   recs)
    job = [cpvtape.internal(b).decode('latin-1')
           for k, b in cpvtape.keyed_records(ms['COMPILE_CAVE'][1])]
    assert job[0] == '!DEFAULT OPT=NOLIST', job[0]
    deck = job[job.index('!DATA') + 1:job.index('!EOD')]
    assert deck[-1] == 'OPT' and len(deck) == 15, deck
    deck[-1] = 'NOLIST'            # !DEFAULT OPT=NOLIST
    assert [d.split()[1] for d in deck[:-1]] == CAVE_MEMBERS
    keyed_file(os.path.join(OUT, 'cave', 'COMPILE_CAVE.kyd'),
               [((i + 1) * 1000, d.encode('latin-1'))
                for i, d in enumerate(deck)])

    ptdecl = ['      CHARACTER*256 PTBUF', '      COMMON /PTCOM/ PTBUF']
    adv = fortran(ms, 'ADVSI', 'ADV:C', 'ADV:C', EDITS_ADV,
                  dict(include=15, repeat=2, eor=2, print=2, altret=13,
                       formats=4), ptdecl)
    adv = adv_output(adv, 18)
    extra, _ = expand_includes(ADV_EXTRA.strip('\n').split('\n'),
                               source_lines(ms, 'ADV:C') + ptdecl, 'ADV:C')
    extra, _ = tidy(extra)
    extra = rename_dollars(extra, 6)
    adv += extra

    munge = fortran(ms, 'MUNGESI', 'MUNGE:C', 'MUNGE:C', EDITS_MUNGE,
                    dict(include=14, repeat=7, eor=0, print=2, altret=26,
                         formats=13))

    for name, src in (('adv.f', adv), ('munge.f', munge)):
        for l in src:
            if not is_comment(l):
                if len(l) > 132:
                    raise SystemExit('line too long: ' + l)
                if '$' in ''.join(t for s, t in segments(l) if not s):
                    raise SystemExit('$ left in code: ' + l)
        with open(os.path.join(OUT, 'src', name), 'w', encoding='latin-1',
                  newline='\n') as f:
            f.write('\n'.join(src) + '\n')
    print('convert: %d lines of adv.f, %d of munge.f, %d cave files'
          % (len(adv), len(munge), len(CAVE_MEMBERS)))


if __name__ == '__main__':
    main()
