# -*- coding: latin-1 -*-
r"""The exact-line replacements convert.py applies to ADVENT.FOR.

Every key below is a line of ../src_original/ADVENT.FOR, quoted
character for character (the file is written with DEC tabs, so the keys
contain tabs).  convert.py fails the build if any key stops matching, so
this list cannot silently drift away from the original.

Nothing here touches game logic.  Everything is one of:

  * an I/O statement the compiler cannot do -- the A edit descriptor
    over 36-bit packed words, the DEC free-format G descriptor, ACCEPT,
    and the DEC OPEN;
  * a routine that was machine-specific -- SHIFT, CALL DATE/CALL TIME;
  * the save mechanism, which on TOPS-10 was "the monitor dumps the core
    image" and here has to be a file;
  * the three places -u lifts a restriction;
  * two declarations (see A(6) and COMMON /RANCOM/ below).

For the reasoning behind each group see README.md.
"""

NL = '\n'

# ---------------------------------------------------------------------
# Whole routines superseded by src/runtime.f
# ---------------------------------------------------------------------
# SHIFT is 36-bit arithmetic that re-inserts bit 35 by hand.  Its right
# shift still works in 64 bits, but its left shift leaves the result
# un-sign-extended, so a word whose first character is >= 100 octal would
# stop comparing equal to a packed literal.  runtime.f does the same
# bits, sign-extended.
DROP_ROUTINES = ['INTEGER FUNCTION SHIFT(VAL,DIST)']

# ---------------------------------------------------------------------
# Where the generated COMMON /ADVSTA/ is inserted: after the last
# DIMENSION of the main program.  Every variable of the main program
# goes into it, so that SUSPEND can write the whole state out the way
# TOPS-10 wrote the whole core image.  convert.py works the list out
# from the declarations rather than from a list kept by hand.
# ---------------------------------------------------------------------
STATE_ANCHOR = '\tDIMENSION TK(20),DSEEN(6),DLOC(6),ODLOC(6),HNAME(4)'

# a few things the state capture must contain, as a sanity check on the
# automatic list: the dwarves, the clock, the lamp, the score
STATE_MUST_HAVE = ['LOC', 'OLDLOC', 'OLDLC2', 'NEWLOC', 'TURNS', 'LIMIT',
                   'DFLAG', 'DLOC', 'ODLOC', 'DSEEN', 'DKILL', 'TALLY',
                   'TALLY2', 'CLOCK1', 'CLOCK2', 'PROP', 'PLAC', 'FIXD',
                   'TRAVEL', 'KEY', 'COND', 'LTEXT', 'STEXT', 'HINTS',
                   'HINTED', 'HINTLC', 'CTEXT', 'CVAL', 'ACTSPK', 'WD1',
                   'WD2', 'VERB', 'OBJ', 'CLOSED', 'CLOSNG', 'PANIC',
                   'GAVEUP', 'SCORNG', 'DEMO', 'WZDARK', 'LMWARN',
                   'ABBNUM', 'MAXDIE', 'NUMDIE', 'FOOBAR', 'BONUS',
                   'KNFLOC', 'DETAIL', 'IWEST', 'CHLOC', 'CHLOC2',
                   'DALTLC', 'MAXTRS', 'HNTMAX', 'LINUSE', 'TRVS',
                   'TABNDX', 'CLSSES', 'LAMP', 'BOTTLE', 'WATER', 'OIL']

# ---------------------------------------------------------------------
# The replacements
# ---------------------------------------------------------------------
LINES = {}

# --- type declarations DEC did not insist on -------------------------
# FORCED and PCT are statement functions whose value is a logical, but
# they are not in the program's LOGICAL statement, so they are INTEGER
# by IMPLICIT.  FORTRAN-10 tested such a value as a logical anyway (the
# sign bit); gfortran wants the declaration.  Likewise WIZARD, which is
# a LOGICAL FUNCTION but is not declared in the two routines that call
# it.  Six words added to three declarations; no value changes.
LINES['\tLOGICAL TOTING,HERE,AT,BITSET,DARK,WZDARK,LMWARN,CLOSNG,PANIC,'] = [
  '\tLOGICAL TOTING,HERE,AT,BITSET,DARK,FORCED,PCT,WZDARK,LMWARN,CLOSNG,PANIC,']
LINES['\tLOGICAL PTIME,SOON,YESM'] = ['\tLOGICAL PTIME,SOON,YESM,WIZARD']
LINES['\tLOGICAL YESM,BLKLIN'] = ['\tLOGICAL YESM,BLKLIN,WIZARD']

# --- main program: startup ------------------------------------------
# Entered where a restarted TOPS-10 core image came back in: BOOT reads
# the command line and, if a saved game was named, restores the state,
# after which SETUP is what it was when the game was suspended.
'\tIF(SETUP.NE.0)GOTO 1100'
LINES['\tIF(SETUP.NE.0)GOTO 1100'] = [
  '\tCALL BOOT',
  '\tIF(SETUP.NE.0)GOTO 1100',
]

# The database is read at every start, because there is no core image to
# keep it in.  On the -10 that happened once, before the game was ever
# played; the report and the PAUSE that follow it belong to that
# one-off, so they are behind -v here.
LINES['\tTYPE 1000'] = ['\tIF(VERBOS().NE.0)PRINT 1000']
LINES['\tTYPE 1999,LINUSE,LINSIZ,TRVS,TRVSIZ,TABNDX,TABSIZ,KK'] = [
  '\tIF(VERBOS().NE.0)PRINT 1999,LINUSE,LINSIZ,TRVS,TRVSIZ,TABNDX,TABSIZ,KK']
# PAUSE stopped the program so the user could type SAVE ADVENT.
LINES["\tPAUSE 'INIT Done'"] = ['\tCALL INITD']

# NAME= and ACCESS='SEQIN' are DEC's; DBOPEN finds ADVENT.DAT in the
# working directory or beside the executable.
LINES["\tOPEN(UNIT=1,NAME='ADVENT',ACCESS='SEQIN')"] = ['\tCALL DBOPEN(1)']

# --- main program: reading the database ------------------------------
# FORMAT(G) and FORMAT(99G) are DEC free-format input: integers from one
# record, the rest of the list left zero.  RDFRE does exactly that; the
# zero fill matters, section 3 records carry a variable number of verbs.
LINES['1002\tREAD(1,1003)SECT'] = [
  '1002\tCALL RDFRE(1,DBBUF,1)',
  '\tSECT=DBBUF(1)',
]
LINES['1003\tFORMAT(G)'] = ['C1003\tFORMAT(G)\t\t-- DEC free format, see RDFRE']

# FORMAT(1G,15A5): a location number, the tab that ended it, then 70
# characters of text in 14 words and a 15th word that must be blank.
LINES['1004\tREAD(1,1005)LOC,(LINES(J),J=LINUSE+1,LINUSE+14),KK'] = [
  '1004\tCALL RDMSG(1,LOC,LINES(LINUSE+1),KK)',
]
LINES['1005\tFORMAT(1G,15A5)'] = [
  'C1005\tFORMAT(1G,15A5)\t\t-- packed text, see RDMSG']

LINES['1030\tREAD(1,1031)LOC,NEWLOC,TK'] = [
  '1030\tCALL RDFRE(1,DBBUF,22)',
  '\tLOC=DBBUF(1)',
  '\tNEWLOC=DBBUF(2)',
  '\tDO 1032 IDB=1,20',
  '1032\tTK(IDB)=DBBUF(IDB+2)',
]
LINES['1031\tFORMAT(99G)'] = [
  'C1031\tFORMAT(99G)\t\t-- DEC free format, see RDFRE']
LINES['1050\tREAD(1,1031)OBJ,J,K'] = [
  '1050\tCALL RDFRE(1,DBBUF,3)',
  '\tOBJ=DBBUF(1)',
  '\tJ=DBBUF(2)',
  '\tK=DBBUF(3)',
]
LINES['1060\tREAD(1,1031)VERB,J'] = [
  '1060\tCALL RDFRE(1,DBBUF,2)',
  '\tVERB=DBBUF(1)',
  '\tJ=DBBUF(2)',
]
LINES['1070\tREAD(1,1031)K,TK'] = [
  '1070\tCALL RDFRE(1,DBBUF,21)',
  '\tK=DBBUF(1)',
  '\tDO 1072 IDB=1,20',
  '1072\tTK(IDB)=DBBUF(IDB+1)',
]
LINES['1081\tREAD(1,1031)K,TK'] = [
  '1081\tCALL RDFRE(1,DBBUF,21)',
  '\tK=DBBUF(1)',
  '\tDO 1082 IDB=1,20',
  '1082\tTK(IDB)=DBBUF(IDB+1)',
]
# FORMAT(G,A5): number, tab, five letters.
LINES['1043\tREAD(1,1041)KTAB(TABNDX),ATAB(TABNDX)'] = [
  '1043\tCALL RDVOC(1,KTAB(TABNDX),ATAB(TABNDX))']
LINES['1041\tFORMAT(G,A5)'] = [
  'C1041\tFORMAT(G,A5)\t\t-- packed word, see RDVOC']

# --- main program: -u ------------------------------------------------
LINES['\tIF(DEMO.AND.TURNS.GE.SHORT)GOTO 13000'] = [
  '\tIF(DEMO.AND.TURNS.GE.SHORT.AND.UNLIM().EQ.0)GOTO 13000']

# --- main program: 20A1 and A2 ---------------------------------------
# A5TOA1 hands back one character per word, left-justified, so an A1
# field printed the first character of each.  WSTR unpacks the same
# characters; the text is the same character for character.
LINES['\tTYPE 5015,(TK(I),I=1,K)'] = ['\tPRINT 5015,TRIM(WSTR(TK,K,1))']
LINES["5015\tFORMAT(/' What do you want to do with the ',20A1)"] = [
  "5015\tFORMAT(/' What do you want to do with the ',A)"]
LINES['\tTYPE 5199,(TK(I),I=1,K)'] = ['\tPRINT 5199,TRIM(WSTR(TK,K,1))']
LINES["5199\tFORMAT(/' I see no ',20A1)"] = ["5199\tFORMAT(/' I see no ',A)"]
LINES['\tTYPE 8002,(TK(I),I=1,K)'] = ['\tPRINT 8002,TRIM(WSTR(TK,K,1))']
LINES["8002\tFORMAT(/' ',20A1)"] = ["8002\tFORMAT(/' ',A)"]
LINES['\tTYPE 9032,(TK(I),I=1,K)'] = ['\tPRINT 9032,TRIM(WSTR(TK,K,1))']
LINES['9032\tFORMAT(/\' Okay, "\',20A1)'] = ['9032\tFORMAT(/\' Okay, "\',A)']
# KK is 's.' or '. ' in a word; A2 over a string prints the same two
# characters, so only the argument changes.
LINES['\tTYPE 20212,K,KK'] = ['\tPRINT 20212,K,WSTR(KK,1,2)']

# --- SPEAK -----------------------------------------------------------
LINES['\tDIMENSION RTEXT(205),LINES(9650)'] = [
  '\tDIMENSION RTEXT(205),LINES(9650)',
  '\tCHARACTER*160 WSTR',
  '\tCHARACTER*200 TABX',
]
# TABX expands tabs to the terminal's eight-column stops, which is what
# the -10's terminal service did with the 577 lines of ADVENT.DAT that
# use a tab to separate sentences.
LINES['\tTYPE 2,(LINES(I),I=K,L)'] = [
  '\tPRINT 2,TRIM(TABX(WSTR(LINES(K),L-K+1,5)))']
LINES["2\tFORMAT(' ',14A5)"] = ["2\tFORMAT(' ',A)"]

# --- GETIN -----------------------------------------------------------
# A(6) is read when a second word starts in the fourth input word
# (WORD2X=...SHIFT(A(J+2),...) with J=4).  On the -10 that read the word
# after A, which was MASKS(1); here it reads a zero.  Only the echo of a
# >15-character command in an error message can see the difference.
LINES['\tDIMENSION A(5),MASKS(6)'] = ['\tDIMENSION A(6),MASKS(6)']
# ACCEPT 4A5: 20 characters of one record, blank-filled into 4 words.
# Everything after it -- the case folding, the word splitting across the
# word boundary -- is the original's own bit arithmetic, untouched.
LINES['2\tACCEPT 3,(A(I),I=1,4)'] = ['2\tCALL RDA5(A,4)']
LINES['3\tFORMAT(4A5)'] = ['C3\tFORMAT(4A5)\t\t-- packed input, see RDA5']

# --- START: -u -------------------------------------------------------
LINES['\tPTIME=(PRIMTM.AND.SHIFT(1,T/60)).NE.0'] = [
  '\tPTIME=(PRIMTM.AND.SHIFT(1,T/60)).NE.0.AND.UNLIM().EQ.0']
LINES['\tIF(DELAY.GE.LATNCY)GOTO 20'] = [
  '\tIF(DELAY.GE.LATNCY.OR.UNLIM().NE.0)GOTO 20']

# --- MAINT -----------------------------------------------------------
LINES['\tDIMENSION HNAME(4),ABB(150)'] = [
  '\tDIMENSION HNAME(4),ABB(150)',
  '\tDIMENSION MBUF(1)',
]
LINES['\tACCEPT 1,HBEGIN'] = ['\tCALL RDFRE(5,MBUF,1)', '\tHBEGIN=MBUF(1)']
LINES['\tACCEPT 1,HEND'] = ['\tCALL RDFRE(5,MBUF,1)', '\tHEND=MBUF(1)']
LINES['1\tFORMAT(G)'] = ['C1\tFORMAT(G)\t\t-- DEC free format, see RDFRE']
LINES['\tACCEPT 2,HNAME'] = ['\tCALL RDA5(HNAME,4)']
LINES['2\tFORMAT(4A5)'] = ['C2\tFORMAT(4A5)\t\t-- packed input, see RDA5']
LINES['\tACCEPT 1,X'] = ['\tCALL RDFRE(5,MBUF,1)', '\tX=MBUF(1)']

# --- WIZARD ----------------------------------------------------------
LINES['\tTYPE 18,WORD'] = ['\tPRINT 18,WSTR(WORD,1,5)']

# --- HOURS -----------------------------------------------------------
LINES['\tTYPE 5,HNAME'] = ['\tPRINT 5,WSTR(HNAME,4,5)']
LINES["5\tFORMAT(/' Today is a holiday, namely ',4A5)"] = [
  "5\tFORMAT(/' Today is a holiday, namely ',A20)"]
LINES['\tTYPE 15,D,T,HNAME'] = ['\tPRINT 15,D,WSTR(T,1,5),WSTR(HNAME,4,5)']
LINES["15\tFORMAT(/' The next holiday will be in',I3,' ',A5,' namely ',4A5)"] = [
  "15\tFORMAT(/' The next holiday will be in',I3,' ',A5,' namely ',A20)"]

# --- HOURSX ----------------------------------------------------------
# DAY1 and DAY2 arrive as separate arguments; WSTR wants them adjacent.
LINES['\tFIRST=.TRUE.'] = [
  '\tDAYS(1)=DAY1',
  '\tDAYS(2)=DAY2',
  '\tFIRST=.TRUE.',
]
LINES['\tTYPE 2,DAY1,DAY2'] = ['\tPRINT 2,WSTR(DAYS,2,5)']
LINES["2\tFORMAT(10X,2A5,'  Open all day')"] = [
  "2\tFORMAT(10X,A10,'  Open all day')"]
LINES['\tIF(FIRST)TYPE 16,DAY1,DAY2,FROM,TILL'] = [
  '\tIF(FIRST)PRINT 16,WSTR(DAYS,2,5),FROM,TILL']
LINES["16\tFORMAT(10X,2A5,I4,':00 to',I3,':00')"] = [
  "16\tFORMAT(10X,A10,I4,':00 to',I3,':00')"]
LINES['20\tIF(FIRST)TYPE 22,DAY1,DAY2'] = [
  '20\tIF(FIRST)PRINT 22,WSTR(DAYS,2,5)']
LINES["22\tFORMAT(10X,2A5,'  Closed all day')"] = [
  "22\tFORMAT(10X,A10,'  Closed all day')"]

# --- NEWHRX ----------------------------------------------------------
LINES['\tNEWHRX=0'] = [
  '\tDAYS(1)=DAY1',
  '\tDAYS(2)=DAY2',
  '\tNEWHRX=0',
]
LINES['\tTYPE 1,DAY1,DAY2'] = ['\tPRINT 1,WSTR(DAYS,2,5)']
LINES["1\tFORMAT(' Prime time on ',2A5)"] = [
  "1\tFORMAT(' Prime time on ',A10)"]
LINES['\tACCEPT 3,FROM'] = ['\tCALL RDFRE(5,NBUF,1)', '\tFROM=NBUF(1)']
LINES['\tACCEPT 3,TILL'] = ['\tCALL RDFRE(5,NBUF,1)', '\tTILL=NBUF(1)']
LINES['3\tFORMAT(G)'] = ['C3\tFORMAT(G)\t\t-- DEC free format, see RDFRE']

# --- MOTD ------------------------------------------------------------
# MSG is the message of the day.  On the -10 it was part of the core
# image the wizard saved; here it is part of the saved state, so it goes
# into a COMMON block where STATIO can reach it.
LINES['\tDIMENSION MSG(100)'] = [
  '\tDIMENSION MSG(100)',
  '\tCOMMON /MOTCOM/ MSG',
  '\tDIMENSION MOTBUF(15)',
  '\tCHARACTER*160 WSTR',
  '\tCHARACTER*200 TABX',
]
LINES['\tTYPE 20,(MSG(I),I=K+1,MSG(K)-1)'] = [
  '\tPRINT 20,TRIM(TABX(WSTR(MSG(K+1),MSG(K)-K-1,5)))']
LINES["20\tFORMAT(' ',14A5)"] = ["20\tFORMAT(' ',A)"]
LINES['55\tACCEPT 56,(MSG(I),I=M+1,M+14),K'] = [
  '55\tCALL RDA5(MOTBUF,15)',
  '\tDO 57 I=1,14',
  '57\tMSG(M+I)=MOTBUF(I)',
  '\tK=MOTBUF(15)',
]
LINES['56\tFORMAT(15A5)'] = ['C56\tFORMAT(15A5)\t\t-- packed input, see RDA5']

# --- RAN -------------------------------------------------------------
# The generator itself is kept exactly as written -- it is not
# machine-specific, and because it is seeded only from DATIME the port
# and the original produce identical random behaviour on the same date
# and time.  Only its seed moves: R has to be part of the saved state,
# and a DATA-initialised local cannot be reached from STATIO.  COMMON is
# zeroed by the loader here just as core was on the -10, so DATA R/0/ is
# not lost.
LINES['\tDATA R/0/'] = ['\tCOMMON /RANCOM/ R']

# --- DATIME ----------------------------------------------------------
# The decoding stays exactly as written, including the leap-year
# arithmetic and BUG(28).  Only the two DEC library calls are replaced,
# by routines that hand back the same ASCII a TOPS-10 FOROTS does.
LINES['\tCALL DATE(DAT)'] = ['\tCALL DCDATE(DAT)']
LINES['\tCALL TIME(TIM)'] = ['\tCALL DCTIME(TIM)']

# --- CIAO ------------------------------------------------------------
# Magic message 32 is "BE SURE TO SAVE YOUR CORE-IMAGE..."  There is no
# core image and no monitor to save it, so the program writes the file
# itself and says so.  Everything else about CIAO, including the STOP,
# is unchanged.
LINES['\tCALL MSPEAK(K)'] = ['\tCALL CIAOSV(K)']

# lines that legitimately occur more than once and get the same
# replacement every time
MULTI = set(['\tACCEPT 1,X'])

# Declarations inserted after a routine's IMPLICIT line.  WSTR is the
# runtime's unpacker, used wherever an A descriptor printed packed
# words; DBBUF holds one record for RDFRE; DAYS puts two separate
# arguments where WSTR can see them as a pair.
DECLS = {
  '@MAIN':  ['\tDIMENSION DBBUF(22)', '\tCHARACTER*160 WSTR'],
  'WIZARD': ['\tCHARACTER*160 WSTR'],
  'HOURS':  ['\tCHARACTER*160 WSTR'],
  'HOURSX': ['\tCHARACTER*160 WSTR', '\tDIMENSION DAYS(2)'],
  'NEWHRX': ['\tCHARACTER*160 WSTR', '\tDIMENSION DAYS(2)', '\tDIMENSION NBUF(1)'],
}

# --- MAGIC MODE hints under -u ---------------------------------------
# Two calls added to WIZARD, both no-ops without -u.  They only print;
# they do not touch WORD, VAL, MAGIC or MAGNM, so what WIZARD accepts is
# exactly what it accepted in 1977.
#
# The second one sits after WIZARD's FORMAT 18 rather than after the
# TYPE that uses it.  A FORMAT is not executable, so the call still runs
# between the challenge being printed and GETIN reading the reply --
# and anchoring on the FORMAT keeps the patch off the TYPE line, which
# convert.py rewrites to PRINT.
LINES['\tCALL MSPEAK(17)'] = [
  '\tCALL MSPEAK(17)',
  '\tCALL WIZWRD(MAGIC)',
]
LINES['18\tFORMAT(/1X,A5)'] = [
  '18\tFORMAT(/1X,A5)',
  '\tCALL WIZHNT(VAL,MAGNM)',
]

# --- .AND./.OR. are not short-circuit in FORTRAN-10 ----------------------
# DEC evaluated a whole logical expression; gfortran is free to stop at the
# first operand that settles it, and does.  That is invisible for pure
# operands -- but PCT(N) is RAN(100).LT.N, and RAN has a side effect: it
# advances the generator.  Every PCT that gfortran skips is a draw the -10
# consumed, so the port's random stream slides out of step with the
# original's and every later dwarf, pit and message diverges.
#
# Measured, not guessed: with the clock frozen to the reference run's
# 06-MAY-1989 10:05, the original's replies to four nonsense words are
# 60,13,60,61 and the port's were 60,60,60,13 -- the original's stream sits
# exactly one draw further on at the first divergence.
#
# Each of the seven sites below evaluates its PCT/RAN into a temporary
# first, so the draw is taken whatever the rest of the condition does.  The
# tests are otherwise unchanged, and PCT appears once in each, so forcing
# it cannot change the result of the expression -- only the draw count.
def _pct(cond, call, tail, label='\t'):
    return ['\tLPCTT=0', '\tIF(%s)LPCTT=1' % call, label + cond + tail]

LINES['\tIF(LOC.LT.15.OR.PCT(95))GOTO 2000'] = _pct(
  'IF(LOC.LT.15.OR.LPCTT.NE.0)', 'PCT(95)', 'GOTO 2000')

LINES['6001\tIF(PCT(50).AND.SAVED.EQ.-1)DLOC(J)=0'] = _pct(
  'IF(LPCTT.NE.0.AND.SAVED.EQ.-1)', 'PCT(50)', 'DLOC(J)=0', '6001\t')

LINES['\tIF(ODLOC(6).NE.DLOC(6).AND.PCT(20))CALL RSPEAK(127)'] = _pct(
  'IF(ODLOC(6).NE.DLOC(6).AND.LPCTT.NE.0)', 'PCT(20)', 'CALL RSPEAK(127)')

LINES['\tIF(WZDARK.AND.PCT(35))GOTO 90'] = _pct(
  'IF(WZDARK.AND.LPCTT.NE.0)', 'PCT(35)', 'GOTO 90')

LINES['\tIF(LOC.EQ.33.AND.PCT(25).AND..NOT.CLOSNG)CALL RSPEAK(8)'] = _pct(
  'IF(LOC.EQ.33.AND.LPCTT.NE.0.AND..NOT.CLOSNG)', 'PCT(25)', 'CALL RSPEAK(8)')

# PCT(NEWLOC) is taken even when NEWLOC is 0 -- PCT(0) is always false but
# still draws, and the -10 drew it.
LINES['14\tIF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC))GOTO 12'] = [
  '14\tLPCTT=0',
  '\tIF(PCT(NEWLOC))LPCTT=1',
  '\tIF(NEWLOC.NE.0.AND.LPCTT.EQ.0)GOTO 12']

# RAN itself, not via PCT.  (convert.py renames RAN to RAN10.)
LINES['\tIF(RAN(3).EQ.0.OR.SAVED.NE.-1)GOTO 9175'] = [
  '\tLRANT=RAN10(3)',
  '\tIF(LRANT.EQ.0.OR.SAVED.NE.-1)GOTO 9175']
