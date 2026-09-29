#!/usr/bin/env python3
"""Turn Qork V3.0A's CDC FTN5 source into source gfortran will take.

    convert.py      (reads ..\\..\\src_original\\qork.src, writes ..\\.build\\src)

qork.src is one deck: the FORTRAN (lines 1-10031 and 10151 to the end, the
second part being PRS, the initialisation, with its readers) and two small
COMPASS subprograms, RIO (the random database file) and CONCAT.  The COMPASS
is replaced by the port's own src\\port\\pqorkc.c and pqork.f; the FORTRAN is
edited here, and every edit asserts how often it matches.

The port keeps the Cyber's 60-bit words: every integer is INTEGER*8 holding
the 60-bit pattern, characters are display code, and what the program does
with bits - packing words six characters at a time, testing and clearing
flags - it does here unchanged.  What has to change:

  * FTN5 constants.  "TEXT" used as a number is Hollerith: its display code,
    left justified and blank filled to ten characters; R"X" is right
    justified and zero filled; O"777" is octal; 10H is ten blanks.  They
    become decimal integers (a DATA statement cannot call a function).
    Inside FORMAT statements quoted text stays text.
  * FORLIB's word functions AND, OR, XOR, COMPL (a 60-bit complement) and
    SHIFT (circular within 60 bits), RANF, TIME, EOF, and the program's
    externals SSWTCH, GOTOER and CONCAT come from the port (P... names).
  * RIO's OPEN, CLOSE, WRR, WRI, RDR and RNL become the port's PRIO...
  * The A and R edit descriptors: every READ and WRITE that moves characters
    becomes a call that converts display code, by NOS's 6/12 ASCII rules -
    lower case arrives as ^ and the letter, which is what the program strips
    and what YESNO looks for.  GUARD's numeric reads read a line (PLINE) and
    then use their own FORMAT on it; PLINE and the calls' end-of-file flag
    PEOFX are declared in each unit that uses them.
  * PROGRAM QORK becomes SUBROUTINE QORKMN (the port's main program reads
    the options first); SUBROUTINE PRS, whose name is also a COMMON block's,
    becomes PRSINI.
  * FTN5's own rules, measured on NOS 2.8.7: every operand of .AND. and
    .OR. was evaluated, so each random draw in a compound condition is made
    before it (PRBH); a function kept its value from call to call, which
    CLOCKD relies on; and GUARD's O and list-directed output is written as
    FTN5 wrote it.

Compile with -fdefault-integer-8 -fdefault-real-8 -ffixed-line-length-none:
lines are padded to column 72 before they are edited, so quoted text that
runs on over a continuation keeps its blanks, and may then grow past it.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', 'src_original')
OUT = os.path.join(HERE, '..', '.build', 'src')

# The 64 character set: display code 00 is ':', 55 octal is blank.
DCTAB = (':ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/()$= ,.#[]%"_!&'
         "'?<>@" + chr(92) + '^;')
assert len(DCTAB) == 64
COUNTS = []


def check(what, n, want):
    if n != want:
        sys.exit('convert: %s: %d, expected %d' % (what, n, want))
    COUNTS.append((what, n))


def dc_left(s):
    """Hollerith: display code, left justified, blank filled to ten"""
    if len(s) > 10:
        sys.exit('convert: constant longer than a word: %r' % s)
    v = 0
    for c in s.ljust(10):
        v = v * 64 + DCTAB.index(c)
    return v


def dc_right(s):
    """R"...": right justified, zero filled"""
    v = 0
    for c in s:
        v = v * 64 + DCTAB.index(c)
    return v


# ------------------------------------------------------------------ source
def source():
    raw = open(os.path.join(ORIG, 'qork.src'), 'rb').read().decode('latin-1')
    lines = raw.replace('\r\n', '\n').split('\n')
    a = next(i for i, l in enumerate(lines) if l[:72].split()[:2] == ['IDENT', 'RIO'])
    b = next(i for i, l in enumerate(lines) if l[:72].split()[:2] == ['IDENT', 'CONCAT'])
    c = next(i for i in range(b, len(lines)) if lines[i][:72].split() == ['END'])
    if (a, c) != (10031, 10149):
        sys.exit('convert: the COMPASS is at lines %d-%d, not 10032-10150'
                 % (a + 1, c + 1))
    # columns 73-80 are sequence numbers, which the compiler never read
    return [l.rstrip() if is_comment(l) else l[:72].rstrip()
            for l in lines[:a] + lines[c + 1:]]


def is_comment(line):
    return line[:1] in ('C', 'c', '*') or not line.strip()


# ------------------------------------------------------------ whole statements
#  (old physical lines, new lines, how often)
EDITS = [
    # the main program: the options come first (src\port\pmain.f)
    (['      PROGRAM QORK (INPUT=64/200, OUTPUT=64/200, TAPE5=INPUT,',
      '     +TAPE6=OUTPUT, DATBAS=64/200, TAPE1=DATBAS, TAPE3=512)'],
     ['      SUBROUTINE QORKMN',
      'C  PORT: THE FILES OF PROGRAM QORK(...) ARE OPENED BY POPTS'], 1),
    # PRS is also a COMMON block's name
    (['  100 CALL PRS'], ['  100 CALL PRSINI'], 1),
    (['      SUBROUTINE PRS'], ['      SUBROUTINE PRSINI'], 1),
    # after the initial state is restored, SAVE and RESTORE use the
    # player's own file (on the Cyber a session's local TAPE3)
    ([' 1600 CONTINUE', '      CALL RSTRGM'],
     [' 1600 CONTINUE', '      CALL RSTRGM',
      'C  PORT: FROM NOW ON UNIT 3 IS SAVES\\QORK.SAV (SEE PSAVUN)',
      '      CALL PSAVUN'], 1),
    # RANF is REAL whatever IMPLICIT says
    (['        RND = INT(FLOAT(X) * RANF())'],
     ['        RND = INT(FLOAT(X) * PRANF())'], 1),
    # ---- terminal input
    (['   90 READ ( INPCH, 100 ,END=10000) INBUF0'],
     ['   90 CALL PRDA1(INPCH,INBUF0,170,PEOFX)',
      '      IF (PEOFX .NE. 0) GO TO 10000'], 1),
    (['        READ(INPCH,110,END=10000) ANS'],
     ['        CALL PRDAN(INPCH,ANS,10,PEOFX)',
      '        IF (PEOFX .NE. 0) GO TO 10000'], 1),
    (['        READ (*,100, END=10000)WORD'],
     ['        CALL PRDA1(5,WORD,6,PEOFX)',
      '        IF (PEOFX .NE. 0) GO TO 10000'], 1),
    # the EOF message, as a 6/12 terminal showed it (^C is c)
    (["  401 FORMAT ( 1X,' I ^C^A^N^N^O^T ^H^E^A^R ^Y^O^U!' )"],
     ['C  PORT: THE TEXT AS AN ASCII TERMINAL SHOWED ITS 6/12 CODES',
      "  401 FORMAT ( 1X,' I cannot hear you!' )"], 1),
    # ---- GUARD, the game debugging tool
    (['        READ (*,110,END=10001)LINE'],
     ['        CALL PRDAW(5,LINE,10,2,PEOFX)',
      '        IF (PEOFX .NE. 0) GO TO 10001'], 1),
    (['        READ (*,210,END=10002)CMD'],
     ['        CALL PRDAW(5,CMD,1,2,PEOFX)',
      '        IF (PEOFX .NE. 0) GO TO 10002'], 1),
    # ---- output
    (['      WRITE(OUTCH,650)(B1(IFROG),IFROG=1,I)'],
     ['      CALL PWRR1(OUTCH,B1,I)'], 1),
    (['1400    WRITE(OUTCH,1410) (INLINE(J),J=1,INLNT)'],
     ['1400    CALL PWRA1(OUTCH,INLINE,INLNT)'], 1),
    (['5000    WRITE(OUTCH,5010) VMAJ,VMIN,VEDIT'],
     ['5000    CALL PWRVER(OUTCH,VMAJ,VMIN,VEDIT)'], 1),
    # ---- the FORMATs of those reads and writes: gfortran has no R edit
    #      descriptor, and nothing refers to them any more
    (['  650 FORMAT ( 1X,170R1 )'],
     ['C 650 FORMAT ( 1X,170R1 ) - PORT: SEE PWRR1'], 1),
    (['475     FORMAT(BZ,170R1)'],
     ['C475    FORMAT(BZ,170R1) - PORT: SEE PRDR1'], 1),
    (['99    FORMAT(BZ,172R1)'],
     ['C99   FORMAT(BZ,172R1) - PORT: SEE PRDR1'], 6),
    (['99    FORMAT(BZ,A2,170R1)'],
     ['C99   FORMAT(BZ,A2,170R1) - PORT: SEE PRDEX'], 1),
    # FTN5 takes a comma before the closing parenthesis; gfortran does not
    (["100     FORMAT(' YOUR SCORE WOULD BE ',)"],
     ["100     FORMAT(' YOUR SCORE WOULD BE ')"], 1),
    (["110     FORMAT('  YOUR SCORE IS ',)"],
     ["110     FORMAT('  YOUR SCORE IS ')"], 1),
    # ---- LEX packs a word with the Boolean operators on integers
    (['      OUTBUF(K) = OUTBUF(K) .AND. (.NOT. SHIFT(O"77",(60-CN)))',
      '      J = SHIFT(J,6) .AND. O"77"',
      '      OUTBUF(K) = OUTBUF(K) .OR. SHIFT(J,(60-CN))'],
     ['C  PORT: .AND. .OR. .NOT. ON INTEGERS ARE BIT OPERATIONS',
      '      OUTBUF(K) = IAND(OUTBUF(K), NOT(SHIFT(O"77",(60-CN))))',
      '      J = IAND(SHIFT(J,6), O"77")',
      '      OUTBUF(K) = IOR(OUTBUF(K), SHIFT(J,(60-CN)))'], 1),
    (['      XTYPE = ( SHIFT(I,-8) .AND. XFMASK ) + 1'],
     ['      XTYPE = IAND( SHIFT(I,-8), XFMASK ) + 1'], 1),
    # FTN5 kept a function's value in a word of its own, which a call
    # that does not set it hands back again (measured on NOS 2.8.7).
    # CLOCKD sets its value only when an event fires, so WAIT, which runs
    # it three times unless it says one fired, waits three turns until
    # the first event of the game and one turn after that.
    (['        DO 100 I=1,CLNT'],
     ['        LOGICAL PSVCKD',
      '        SAVE PSVCKD',
      '        DATA PSVCKD/.FALSE./',
      'C  PORT: THE VALUE CLOCKD HAD LAST TIME, AS ON THE CYBER',
      '        CLOCKD=PSVCKD',
      '        DO 100 I=1,CLNT'], 1),
    (['          CLOCKD=.TRUE.'],
     ['          CLOCKD=.TRUE.',
      '          PSVCKD=.TRUE.'], 1),
] + [
    # FTN5 evaluated every operand of .AND. and .OR., so a PROB (a draw of
    # RND) in a condition was drawn whatever the rest said - the thief's
    # loop over the objects draws once for each, every turn.  gfortran
    # stops at the first operand that settles it (at -O1 and above), so
    # the draw goes into PRBH first; a label goes with it.
    (old, ['C  PORT: THE DRAW FIRST, AS FTN5 MADE IT WHATEVER THE REST SAID']
     + new, 1) for old, new in [
        # ROBRM
        (['          IF((OTVAL(I).LE.0).OR.(AND(OFLAG2(I),SCRDBT).NE.0).OR.',
          '     1       (AND(OFLAG1(I),VISIBT).EQ.0).OR.(.NOT.PROB(PR)))',
          '     2       GO TO 50'],
         ['          PRBH=PROB(PR)',
          '          IF((OTVAL(I).LE.0).OR.(AND(OFLAG2(I),SCRDBT).NE.0).OR.',
          '     1       (AND(OFLAG1(I),VISIBT).EQ.0).OR.(.NOT.PRBH))',
          '     2       GO TO 50']),
        # RMINFO
        (['        IF(.NOT.FULL .AND. (SUPERF.OR.((AND(RFLAG(HERE),RSEEN).NE.0)',
          '     1       .AND. (BRIEFF.OR.PROB(80))))) GO TO 400'],
         ['        PRBH=PROB(80)',
          '        IF(.NOT.FULL .AND. (SUPERF.OR.((AND(RFLAG(HERE),RSEEN).NE.0)',
          '     1       .AND. (BRIEFF.OR.PRBH)))) GO TO 400']),
        # RAPPLI
        (['        IF(PROB(50).OR.(OADV(CANDL).NE.WINNER).OR.',
          '     1       .NOT.QON(CANDL)) RETURN'],
         ['        PRBH=PROB(50)',
          '        IF(PRBH.OR.(OADV(CANDL).NE.WINNER).OR.',
          '     1       .NOT.QON(CANDL)) RETURN']),
        # THIEFD
        (['        IF((RHERE.NE.0).OR.PROB(70)) GO TO 1150'],
         ['        PRBH=PROB(70)',
          '        IF((RHERE.NE.0).OR.PRBH) GO TO 1150']),
        (['1200    IF((RHERE.EQ.0).OR.PROB(70)) GO TO 1250'],
         ['1200    PRBH=PROB(70)',
          '        IF((RHERE.EQ.0).OR.PRBH) GO TO 1250']),
        (['          IF(.NOT.QHERE(I,THFPOS).OR.PROB(60).OR.',
          '     1       (AND(OFLAG1(I),(VISIBT+TAKEBT)).NE.(VISIBT+TAKEBT)))',
          '     2       GOTO 1450'],
         ['          PRBH=PROB(60)',
          '          IF(.NOT.QHERE(I,THFPOS).OR.PRBH.OR.',
          '     1       (AND(OFLAG1(I),(VISIBT+TAKEBT)).NE.(VISIBT+TAKEBT)))',
          '     2       GOTO 1450']),
        (['          IF(.NOT.QHERE(I,THFPOS).OR.(OTVAL(I).NE.0).OR.PROB(80).OR.',
          '     1       (AND(OFLAG1(I),(VISIBT+TAKEBT)).NE.(VISIBT+TAKEBT)))',
          '     2       GOTO 1550'],
         ['          PRBH=PROB(80)',
          '          IF(.NOT.QHERE(I,THFPOS).OR.(OTVAL(I).NE.0).OR.PRBH.OR.',
          '     1       (AND(OFLAG1(I),(VISIBT+TAKEBT)).NE.(VISIBT+TAKEBT)))',
          '     2       GOTO 1550']),
        (['          IF((OADV(I).NE.-THIEF).OR.PROB(70).OR.',
          '     1       (OTVAL(I).GT.0)) GO TO 1850'],
         ['          PRBH=PROB(70)',
          '          IF((OADV(I).NE.-THIEF).OR.PRBH.OR.',
          '     1       (OTVAL(I).GT.0)) GO TO 1850']),
        # FIGHTD (PROB(0) draws too)
        (['          IF((VPROB(I).EQ.0).OR..NOT.PROB(VPROB(I)))',
          '     1       GO TO 2025'],
         ['          PRBH=PROB(VPROB(I))',
          '          IF((VPROB(I).EQ.0).OR..NOT.PRBH)',
          '     1       GO TO 2025']),
        # BLOW
        (['2600    IF((RES.EQ.RSTAG).AND.(DWEAP.NE.0).AND.(RND(100).LT.25))',
          '     1       RES=RLOSE'],
         ['2600    PRBH=(RND(100).LT.25)',
          '        IF((RES.EQ.RSTAG).AND.(DWEAP.NE.0).AND.PRBH)',
          '     1       RES=RLOSE']),
        # WALK
        (['        IF((WINNER.NE.PLAYER).OR.LIT(HERE).OR.PROB(25))',
          '     1       GO TO 500'],
         ['        PRBH=PROB(25)',
          '        IF((WINNER.NE.PLAYER).OR.LIT(HERE).OR.PRBH)',
          '     1       GO TO 500']),
    ]
] + [
    # FTN5 let a function be called as a subroutine
    (['      CALL MOVETO (2)'],
     ['C  PORT: MOVETO IS A LOGICAL FUNCTION; PMOVTO CALLS IT',
      '      CALL PMOVTO (2)'], 1),
    # ---- the database, DATBAS: 172R1 card images
    (['      READ(1,99,END=10000)IN'],
     ['      CALL PRDR1(1,IN,172,PEOFX)',
      '      IF (PEOFX .NE. 0) GO TO 10000'], 6),
    (['      READ(1,99,END=10000)DIR,(IN(I),I=1,170)'],
     ['      CALL PRDEX(1,DIR,IN,PEOFX)',
      '      IF (PEOFX .NE. 0) GO TO 10000'], 1),
    (['470     READ(1,475,END=  478) LINE'],
     ['470   CALL PRDR1(1,LINE,170,PEOFX)',
      '      IF (PEOFX .NE. 0) GO TO 478'], 1),
]

#  GUARD's own numeric and logical reads: read the line, then the original
#  FORMAT (BZ,L1 / BZ,I6 / BZ,O6) on it - a short line is padded with
#  blanks, which BZ makes zeros, as on the Cyber (AH 7 is room 700000).
#  The list-directed READ (*,*) J,K and READ (*,*) J go to PRLD2 and
#  PRLD1, which read on over more lines for values still missing.
GUARD_READ = re.compile(r'^(\s*\d*\s+)READ \(\*,(\d+|\*),END=\s*(\d+)\s*\)\s*(.+)$')


def guard_reads(lines):
    out, n = [], 0
    for line in lines:
        m = GUARD_READ.match(line[:72].rstrip())
        if m and not is_comment(line):
            head, fmt, lbl, items = m.groups()
            label = head.strip()
            lead = label.ljust(5) + ' ' if label else '      '
            if fmt == '*':
                args = items.replace(' ', '')
                out.append(lead + 'CALL PRLD%d(5,%s,PEOFX)' % (args.count(',') + 1, args))
                out.append('      IF (PEOFX .NE. 0) GO TO %s' % lbl)
            else:
                out.append(lead + 'CALL PRLIN(5,PLINE,PEOFX)')
                out.append('      IF (PEOFX .NE. 0) GO TO %s' % lbl)
                out.append('      READ (PLINE,%s,END=%s,ERR=%s) %s' % (fmt, lbl, lbl, items))
            n += 1
        else:
            out.append(line)
    return out, n


#  GUARD's displays as FTN5 wrote them.  Whole statements, after the cards
#  are joined; the key is the statement with its blanks squeezed.
#
#  FTN5's Ow output is the rightmost w of the word's 20 octal digits -
#  062000 where gfortran writes  62000 - so the six displays with an O
#  field print it as text from POCT (src\port\pqork.f).  DS's list-directed
#  PRINT *: FTN5 wrote each value in as few columns as it takes, one blank
#  apart (" 8 0 0 T 0"), where gfortran gives an INTEGER*8 21 columns.
JOINED_EDITS = [
    ('PRINT 310,I,(EQR(I,L),L=1,7)',
     '          PRINT 310,I,(EQR(I,L),L=1,5),POCT(EQR(I,6),6),EQR(I,7)'),
    ('310 FORMAT(1X,I3,5(1X,I6),1X,O6,1X,I6)',
     '310     FORMAT(1X,I3,5(1X,I6),1X,A6,1X,I6)'),
    ('PRINT 330,I,(EQO(I,L),L=1,15)',
     '          PRINT 330,I,(EQO(I,L),L=1,4),POCT(EQO(I,5),7),'
     'POCT(EQO(I,6),7),(EQO(I,L),L=7,15)'),
    ('330 FORMAT(1X,I3,3I6,I4,2O7,2I4,2I6,1X,3I4,2I6)',
     '330     FORMAT(1X,I3,3I6,I4,2A7,2I4,2I6,1X,3I4,2I6)'),
    ('PRINT 350,I,(EQA(I,L),L=1,7)',
     '          PRINT 350,I,(EQA(I,L),L=1,6),POCT(EQA(I,7),6)'),
    ('350 FORMAT(1X,I3,6(1X,I6),1X,O6)',
     '350     FORMAT(1X,I3,6(1X,I6),1X,A6)'),
    ('PRINT 610,TRAVEL(J)',
     '        PRINT 610,POCT(TRAVEL(J),6)'),
    ("610 FORMAT(' OLD= ',O6,6X,'NEW= ')",
     "610     FORMAT(' OLD= ',A6,6X,'NEW= ')"),
    ('44000 PRINT 660,ORP,LASTIT,PVEC,SYN',
     '44000   PRINT 660,ORP(1),POCT(ORP(2),7),(ORP(L),L=3,5),LASTIT,'
     'POCT(PVEC(1),7),(PVEC(L),L=2,5),(POCT(SYN(L),7),L=1,11)'),
    ("660 FORMAT(' ORPHS= ',I7,O7,4I7/ ' PV= ',O7,4I7/' SYN= ',6O7/15X,5O7)",
     "660     FORMAT(' ORPHS= ',I7,A7,4I7/' PV=    ',A7,4I7/' SYN=   ',"
     "6A7/15X,5A7)"),
    ('45000 PRINT 610,PRSFLG',
     '45000   PRINT 610,POCT(PRSFLG,6)'),
    ('19000 PRINT *,PRSA,PRSO,PRSI,PRSWON,PRSCON',
     "19000   PRINT '(1X,I0,1X,I0,1X,I0,1X,L1,1X,I0)',"
     "PRSA,PRSO,PRSI,PRSWON,PRSCON"),
    ('PRINT *,WINNER,HERE,TELFLG',
     "        PRINT '(1X,I0,1X,I0,1X,L1)',WINNER,HERE,TELFLG"),
    ('PRINT *,MOVES,DEATHS,RWSCOR,MXSCOR,MXLOAD,LTSHFT,BLOC, MUNGRM,HS',
     "        PRINT '(1X,I0,8(1X,I0))',MOVES,DEATHS,RWSCOR,MXSCOR,"
     "MXLOAD,LTSHFT,BLOC,MUNGRM,HS"),
]


def joined_edits(lines):
    want = dict(JOINED_EDITS)
    done = {}
    out = []
    for line in lines:
        key = re.sub(r'\s+', ' ', line.strip())
        if not is_comment(line) and key in want:
            done[key] = done.get(key, 0) + 1
            line = want[key]
        out.append(line)
    for key, _ in JOINED_EDITS:
        check('edit ' + key[:30], done.get(key, 0), 1)
    return out


def apply_edits(lines):
    text = '\n'.join(lines) + '\n'
    for old, new, want in EDITS:
        pat = '\n'.join(old) + '\n'
        got = text.count('\n' + pat) + (1 if text.startswith(pat) else 0)
        check('edit ' + old[0].strip()[:40], got, want)
        text = text.replace(pat, '\n'.join(new) + '\n')
    return text.rstrip('\n').split('\n')


# -------------------------------------------------------- constants and names
RENAMES = [
    (re.compile(r'(?<![.\w])AND\s*\('), 'IAND('),
    (re.compile(r'(?<![.\w])XOR\s*\('), 'IEOR('),
    (re.compile(r'(?<![.\w])OR\s*\('), 'IOR('),
    (re.compile(r'\bCOMPL\s*\('), 'PCOMPL('),
    (re.compile(r'\bSHIFT\s*\('), 'PSHIFT('),
    (re.compile(r'\bTIME\s*\('), 'PTIME('),
    (re.compile(r'\bEOF\s*\('), 'PEOF('),
    (re.compile(r'\bCONCAT\s*\('), 'PCNCAT('),
    (re.compile(r'\bCALL\s+SSWTCH\b'), 'CALL PSSWT'),
    (re.compile(r'\bCALL\s+GOTOER\b'), 'CALL PGOTOE'),
    (re.compile(r'\bCALL\s+OPEN\s*\('), 'CALL PRIOOP('),
    (re.compile(r'\bCALL\s+CLOSE\s*\('), 'CALL PRIOCL('),
    (re.compile(r'\bCALL\s+WRR\s*\('), 'CALL PRIOWR('),
    (re.compile(r'\bCALL\s+WRI\s*\('), 'CALL PRIOWI('),
    (re.compile(r'\bCALL\s+RDR\s*\('), 'CALL PRIORD('),
    (re.compile(r'\bCALL\s+RNL\s*\('), 'CALL PRIORN('),
]
HOLL10 = re.compile(r'(?<![\w.])10H$')      # 10H at the end of a line: ten blanks


def statements(lines):
    """join each statement's continuation cards (columns 7-72 of each, the
    cards padded to column 72 as the compiler saw them) into one line;
    comment cards stay where they are"""
    out = []
    cur = None
    for line in lines:
        if is_comment(line):
            if cur is not None:
                out.append(cur)
                cur = None
            out.append(line)
            continue
        card = line[:72].ljust(72)
        if card[5] not in (' ', '0') and cur is not None:
            cur += card[6:]
        else:
            if cur is not None:
                out.append(cur)
            cur = card
    if cur is not None:
        out.append(cur)
    return out


def convert_code(lines):
    """constants and names, outside quoted text; FORMAT text left alone"""
    out = []
    stats = {'hollerith': 0, 'right': 0, 'octal': 0, '10H': 0, 'renames': 0}

    def names(s):
        for pat, rep in RENAMES:
            s, n = pat.subn(rep, s)
            stats['renames'] += n
        return s
    for stmt in statements(lines):
        if is_comment(stmt):
            out.append(stmt)
            continue
        head, body = stmt[:6], stmt[6:]
        in_format = bool(re.match(r'\s*\d+\s*FORMAT\s*\(', stmt))
        res, seg, i = '', '', 0
        while i < len(body):
            ch = body[i]
            if ch in ('"', "'"):
                j = i + 1
                while True:                 # the closing quote; '' is a quote
                    j = body.find(ch, j)
                    if j < 0:
                        sys.exit('convert: unclosed quote: %r' % stmt)
                    if j + 1 < len(body) and body[j + 1] == ch:
                        j += 2
                        continue
                    break
                text = body[i + 1:j]
                if in_format:
                    res += names(seg) + body[i:j + 1]
                    seg = ''
                    i = j + 1
                    continue
                # a prefix letter right before a double quote: O"..", R".."
                prefix = ''
                if ch == '"' and seg and seg[-1] in 'ORL' and \
                        (len(seg) < 2 or not (seg[-2].isalnum() or seg[-2] == '_')):
                    prefix = seg[-1]
                    seg = seg[:-1]
                res += names(seg)
                seg = ''
                if ch == "'":
                    sys.exit("convert: a '...' constant outside a FORMAT: %r" % stmt)
                if prefix == 'O':
                    res += str(int(text, 8))
                    stats['octal'] += 1
                elif prefix == 'R':
                    res += str(dc_right(text))
                    stats['right'] += 1
                elif prefix == 'L':
                    sys.exit('convert: an L"..." constant: %r' % stmt)
                else:
                    res += str(dc_left(text))
                    stats['hollerith'] += 1
                i = j + 1
                continue
            seg += ch
            i += 1
        res += names(seg)
        # 10H: the ten characters after the H are the constant (OUTBUF(I)=10H
        # and .EQ.10H, blanks both)
        m = re.search(r'(?<!\w)10H(.{10})', res)
        if m and not in_format:
            res = res[:m.start()] + str(dc_left(m.group(1))) + res[m.end():]
            stats['10H'] += 1
        out.append(head + res.rstrip())
    return out, stats


UNIT_HEAD = re.compile(r'\s*(?:(?:LOGICAL|INTEGER|REAL)\s+)?'
                       r'(?:FUNCTION|SUBROUTINE|PROGRAM|BLOCK\s*DATA)\b')
PORT_VARS = [('PEOFX', '      INTEGER PEOFX'),
             ('PLINE', '      CHARACTER*80 PLINE'),
             ('POCT', '      CHARACTER*20 POCT'),
             ('PRBH', '      LOGICAL PRBH')]


def declare(lines):
    """the port's own variables, in each program unit that uses them: PEOFX,
    the end-of-file flag of the calls that replaced READ ... END=, is an
    INTEGER (some of those units have no IMPLICIT INTEGER), and PLINE,
    GUARD's line, is text.  The declaration goes after the unit's IMPLICIT
    statement, or its first statement."""
    heads = [i for i, l in enumerate(lines)
             if not is_comment(l) and UNIT_HEAD.match(l[6:])]
    inserts = []
    used = {}
    for k, s in enumerate(heads):
        e = heads[k + 1] if k + 1 < len(heads) else len(lines)
        code = [lines[i] for i in range(s, e) if not is_comment(lines[i])]
        decls = [d for name, d in PORT_VARS
                 if any(re.search(r'\b%s\b' % name, c[6:]) for c in code)]
        if not decls:
            continue
        at = s
        for i in range(s + 1, e):
            if not is_comment(lines[i]) and re.match(r'\s*IMPLICIT\b', lines[i][6:]):
                at = i
                break
        inserts.append((at, decls))
        for d in decls:
            used[d] = used.get(d, 0) + 1
    for at, decls in reversed(inserts):
        lines[at + 1:at + 1] = decls
    return lines, used


def main():
    os.makedirs(OUT, exist_ok=True)
    lines = source()
    lines = apply_edits(lines)
    lines, n = guard_reads(lines)
    check('GUARD numeric reads', n, 15)
    lines, st = convert_code(lines)
    lines = joined_edits(lines)
    lines, used = declare(lines)
    # RDLINE YESNO PROTCT GUARD PRSINI and PRS's seven readers; GUARD
    check('units declaring PEOFX', used.get(PORT_VARS[0][1], 0), 12)
    check('units declaring PLINE', used.get(PORT_VARS[1][1], 0), 1)
    check('units declaring POCT', used.get(PORT_VARS[2][1], 0), 1)
    # ROBRM RMINFO RAPPLI THIEFD FIGHTD BLOW WALK
    check('units declaring PRBH', used.get(PORT_VARS[3][1], 0), 7)
    # counted independently of this scanner; "R" in GRDSTR's DATA is a
    # word of its own, not an R prefix
    check('O"..." constants', st['octal'], 312)
    check('R"..." constants', st['right'], 22)
    check('"..." constants', st['hollerith'], 666)
    check('10H constants', st['10H'], 2)
    COUNTS.append(('renamed calls', st['renames']))
    open(os.path.join(OUT, 'qork.f'), 'w', encoding='latin-1', newline='\n').write(
        '\n'.join(lines) + '\n')
    print('convert: %d lines -> %s' % (len(lines), os.path.join(OUT, 'qork.f')))
    print('convert: ' + ', '.join('%s %d' % c for c in COUNTS if not c[0].startswith('edit')))


if __name__ == '__main__':
    main()
