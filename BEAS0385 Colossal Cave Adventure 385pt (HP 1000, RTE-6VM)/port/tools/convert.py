"""Regenerate port/src/*.f from the sources recovered off the disc.

Two stages:

  1. `hp2gfortran` does the mechanical HP FTN4 -> gfortran-legacy dialect work
     (compiler-control lines, Hollerith, octal, IMPLICIT INTEGER widths, ...).

  2. The FIXUPS table below applies the edits that need judgement rather than
     pattern matching.  Every entry must match the expected number of times,
     so the pipeline fails loudly if the inputs change underneath it.

`src/hprte.f` is *not* generated: it is new code standing in for the RTE
system library, the HP string package and the FMP file interface.

Run this from the port directory:  python tools/convert.py
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
DISC = os.path.join(PORT, os.pardir, 'src_original', 'disc')
SRC = os.path.join(PORT, 'src')

# ADV03 and ADVX2 lost their tails on the disc; see the README.  The
# reconstructions live beside the originals.
SOURCES = [('ADV01.FTN', 'adv01.f'),
           ('ADV03.FTN.recovered', 'adv03.f'),
           ('ADV05.FTN', 'adv05.f'),
           ('ADVF4.FTN', 'advf4.f'),
           ('ADVX2.FTN.recovered', 'advx2.f'),
           ('ADVY2.FTN', 'advy2.f')]

# HP FTN4 uses .AND. and .OR. as bitwise operators on integers as well as
# logical ones on LOGICALs.  Only these five sites are arithmetic.
BITWISE = [
 ('adv01.f', "      I= I.OR.K\n      J= J.OR.K",
             "      I=IOR(I,K)\n      J=IOR(J,K)"),
 ('adv05.f', "      I= I.OR.K\n      J= J.OR.K",
             "      I=IOR(I,K)\n      J=IOR(J,K)"),
]

# Statement-function dummies.  gfortran type-checks their arguments strictly
# and its integer literals are 32-bit, while IMPLICIT INTEGER*2 makes every
# variable here 16-bit.  Give the dummies that only ever receive literals a
# 32-bit type, and rename BITST's and PCT's so they stop colliding with the
# real variables L and N.
STMTFN = [
 ("      IMPLICIT INTEGER*2 (A-Z)\n      LOGICAL*2 BLKLIN,NOINPT,FORCED,PCT",
  "      IMPLICIT INTEGER*2 (A-Z)\n"
  "C     Statement-function dummies handed literal constants; gfortran's\n"
  "C     integer literals are 32-bit.  (Port change.)\n"
  "      INTEGER*4 DUMMY,PBOTL,NPCT,NBIT\n"
  "      INTEGER*2 LBIT\n"
  "      LOGICAL*2 BLKLIN,NOINPT,FORCED,PCT"),
 ("      BITST(L,N)=IAND(COND(L),ISHFT(1,N)).NE.0",
  "      BITST(LBIT,NBIT)=IAND(COND(LBIT),ISHFT(1_2,NBIT)).NE.0"),
 ("      BITST(L,N)=(COND(L).AND.ISHFT(1,N)).NE.0",
  "      BITST(LBIT,NBIT)=IAND(COND(LBIT),ISHFT(1_2,NBIT)).NE.0"),
 ("      PCT(N)=(RNDM(100).LT.N)",
  "      PCT(NPCT)=(RNDM(100).LT.NPCT)"),
 ("      PCT(N)=RNDM(100).LT.N",
  "      PCT(NPCT)=RNDM(100).LT.NPCT"),
]

FIXUPS = {

 'adv01.f': [
  # RTE runs the three overlays as disc-resident segments of one program,
  # sharing the outer block's COMMON.  Here they are ordinary subroutines.
  ("      CALL SEGLD(NAM(1,1),IERR)",
   "      CALL ADV1"),
  ("20    CONTINUE\n"
   "      CALL SEGLD(NAM(1,2),IERR)\n"
   "      CALL SEGLD(NAM(1,3),IERR)",
   "20    CONTINUE\n"
   "      CALL ADV2\n"
   "      CALL ADV3"),
 ],

 'adv03.f': [
  ("      PROGRAM ADV1", "      SUBROUTINE ADV1"),
  # Internal (memory) I/O.  HP arms it with CALL CODE and then reads from an
  # INTEGER array; gfortran reads from a CHARACTER variable overlaying it.
  ("      DIMENSION IBUF(44),IDCB1(144),NAM1(3),NAM2(3)",
   "      CHARACTER*88 CIBUF\n"
   "      EQUIVALENCE (IBUF,CIBUF)\n"
   "      DIMENSION IBUF(44),IDCB1(144),NAM1(3),NAM2(3)"),
  # The fast-start cache #ADVYY is a raw image of the COMMON area, written
  # from ABB for a length taken off the RTE load map.  It depends on the
  # linker laying every COMMON block out contiguously in one particular
  # order -- which the source itself warns about at length -- so the port
  # drops it and always reads the database.  That is milliseconds here.
  ("      CALL FMPOPEN(IDCB1, IERR, CNAM3, 'RO', 1)   !DWH REPL FMGR OPEN",
   "C     Fast-start cache disabled in the port; see tools/convert.py.\n"
   "      IERR = -1"),
  ("      CALL FMPOPEN(IDCB1,IERR,CNAM3,'WCO',1)",
   "      IERR = -1"),
  ("      LEN=FMPWRITE(IDCB1,IERR,ABB,5626*2) !DWH-SIZE FROM ADVEN.MAP",
   "C     LEN=FMPWRITE(IDCB1,IERR,ABB,5626*2)"),
  # Same statement-function widening as in ADVX2/ADVY2: the shift has to
  # produce a 16-bit result for IAND, and the bit number arrives as a
  # 32-bit literal.
  ("      BITSET(L,N)=IAND(COND(L),ISHFT(1,N)).NE.0",
   "      BITSET(LBIT,NBIT)=IAND(COND(LBIT),ISHFT(1_2,NBIT)).NE.0"),
  # ADV1 and MORE share this declaration block; the extra names are simply
  # unused in MORE.
  ("      LOGICAL*2 BITSET,LMWARN,CLOSNG,PANIC,",
   "C     Statement-function dummies handed literal constants.\n"
   "      INTEGER*4 NBIT\n"
   "      INTEGER*2 LBIT\n"
   "      LOGICAL*2 BITSET,LMWARN,CLOSNG,PANIC,", 2),
 ],

 'adv05.f': [
  # ADVEN.MAP shows the loader satisfying VOCAB, BUG and RAND from ADV01 and
  # taking only the data-structure routines out of ADV05.  Keeping ADV05's
  # copies here would be a duplicate definition.
  ("      SUBROUTINE VOCAB(ID1,ID2,INIT,V)", "@@CUT@@"),
  ("      INTEGER*2 FUNCTION RNDM(RANGE)", "@@CUT@@"),
  ("      SUBROUTINE BUG(LU,NUM)", "@@CUT@@"),
 ],

 'advf4.f': [
  ("      DIMENSION RTEXT(215),LINES(40),IBUF(44)",
   "      CHARACTER*88 CIBUF\n"
   "      EQUIVALENCE (IBUF,CIBUF)\n"
   "      DIMENSION RTEXT(215),LINES(40),IBUF(44)"),
  ("      DIMENSION RTEXT(215),LINES(40),PTEXT(100),IBUF(44)",
   "      CHARACTER*88 CIBUF\n"
   "      EQUIVALENCE (IBUF,CIBUF)\n"
   "      DIMENSION RTEXT(215),LINES(40),PTEXT(100),IBUF(44)"),
  ("      EQUIVALENCE (FRST,QFRST(2))",
   "      CHARACTER*40 CFRST\n"
   "      EQUIVALENCE (FRST,QFRST(2)),(FRST,CFRST)"),
  # An RTE terminal never reports end of file; a redirected standard input
  # does.  Treat it as the player walking away.
  # The HP console runs in upper case (SIMH's "set TTY0 UC"), so the game
  # never folds what it reads.  A pipe does not.
  ("      READ(LU,3) FRST",
   "      READ(5,3,END=9000) FRST\n"
   "      CALL CASEUP(CFRST)"),
  ("100   RETURN\nC\n      END",
   "100   RETURN\nC\n"
   " 9000 WRITE(LU,'(/\"[end of input]\")')\n"
   "      CALL EXIT\n"
   "      END"),
 ],

 'advx2.f': [
  ("      PROGRAM ADV2", "      SUBROUTINE ADV2"),
  # The two statement-function calls that pass a variable rather than a
  # literal now need the widening spelt out.
  ("      IF(.NOT.BITST(LOC,HINT))HINTLC(HINT)=-1",
   "      IF(.NOT.BITST(LOC,INT(HINT)))HINTLC(HINT)=-1"),
  ("14    IF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC))GOTO 12",
   "14    IF(NEWLOC.NE.0.AND..NOT.PCT(INT(NEWLOC)))GOTO 12"),
 ],

 'advy2.f': [
  ("      PROGRAM ADV3", "      SUBROUTINE ADV3"),
 ],
}

# A-format reads into INTEGER destinations.  gfortran reads A into a
# non-CHARACTER item the way it reads a numeric field, so a comma inside the
# text ends the field and the rest is blank filled; location 3's "a
# building, a well house" loses its comma.  A2GET in hprte.f does the plain
# character copy the HP compiler does.  The number always sits in columns
# 1-5 with its separator in column 6 -- the CLRQ/IPOSQ/NSRTQ loop just above
# each of these shifts the record until that is true.
A2READ = [
 ("      READ(CIBUF,1005) LOC,LINES",
  "      READ(CIBUF(1:5),'(I5)') LOC\n"
  "      CALL A2GET(CIBUF,7,LINES,40)", 1),
 ("      READ(CIBUF,1041)KTAB(TABNDX),ATAB(TABNDX),A2TAB(TABNDX)",
  "      READ(CIBUF(1:5),'(I5)') KTAB(TABNDX)\n"
  "      CALL A2GET(CIBUF,7,ATAB(TABNDX),1)\n"
  "      CALL A2GET(CIBUF,9,A2TAB(TABNDX),1)", 1),
 ("      READ(CIBUF,1000) LOC,LINES",
  "      READ(CIBUF(1:5),'(I5)') LOC\n"
  "      CALL A2GET(CIBUF,7,LINES,40)", 3),
 ("      READ(CFRST,99) WORD1,WORD1A,WORD1X",
  "      CALL A2GET(CFRST,1,WORD1,1)\n"
  "      CALL A2GET(CFRST,3,WORD1A,1)\n"
  "      CALL A2GET(CFRST,5,WORD1X,1)", 1),
 ("      READ(CFRST,99) WORD2,WORD2A,WORD2X",
  "      CALL A2GET(CFRST,1,WORD2,1)\n"
  "      CALL A2GET(CFRST,3,WORD2A,1)\n"
  "      CALL A2GET(CFRST,5,WORD2X,1)", 1),
]

# Internal reads: the unit is the CHARACTER overlay, not the INTEGER array.
# `CALL CODE` is how HP FTN4 arms the next READ/WRITE for memory I/O; in
# standard Fortran the unit being a CHARACTER variable says the same thing.
INTREAD = [(re.compile(r'READ\s*\(\s*IBUF\s*,'), 'READ(CIBUF,'),
           (re.compile(r'READ\s*\(\s*FRST\s*,'), 'READ(CFRST,'),
           (re.compile(r'^      CALL CODE([ \t].*)?$', re.M),
            lambda m: 'C     CALL CODE' + (m.group(1) or '')),
           (re.compile(r'^(\d+)[ \t]+CALL CODE([ \t].*)?$', re.M),
            lambda m: m.group(1).ljust(6) + 'CONTINUE'),
           # ISHFT is an intrinsic here, so it must not be declared EXTERNAL.
           (re.compile(r'^      EXTERNAL ISHFT\s*$', re.M),
            'C     EXTERNAL ISHFT'),
           # Addresses taken for debug WRITEs that are themselves commented
           # out.  LCO has no portable meaning, and nothing reads the result.
           (re.compile(r'^      LOCAT\d=LCO\(.*$', re.M),
            lambda m: 'C' + m.group(0)[1:])]

# A trailing underscore in an HP FORTRAN format suppresses the newline --
# the ">" prompt, and phrases the code continues with an object name.
# gfortran spells that as the $ edit descriptor.
NOADV = re.compile(r'_"\)')

# RTE drives one terminal LU in both directions.  gfortran preconnects
# standard input and standard output as separate units, so writes keep LU
# (which RMPAR reports as 6) and reads move to unit 5.
READLU = re.compile(r'READ\s*\(\s*LU\s*,', re.I)


def run(src, dst):
    subprocess.check_call([sys.executable,
                           os.path.join(HERE, 'hp2gfortran.py'), src, dst])


def cut_units(text):
    """Delete the program units marked @@CUT@@, up to their END."""
    out = []
    skipping = False
    for line in text.split(chr(10)):
        if line.strip() == '@@CUT@@':
            skipping = True
            out.append('C     --- program unit removed by the port; see '
                       'tools/convert.py ---')
            continue
        if skipping:
            out.append('C' + line[1:] if line[:1] not in ('', 'C') else line)
            if line.strip().upper() == 'END':
                skipping = False
            continue
        out.append(line)
    return chr(10).join(out)


def fixup(path):
    name = os.path.basename(path)
    s = open(path, encoding='latin1').read()
    s = NOADV.sub('",$)', s)
    s = s.replace(',"",$)', ',$)')
    for f, old, new in BITWISE:
        if f == name:
            if s.count(old) != 1:
                raise SystemExit('%s: bitwise fixup did not match' % name)
            s = s.replace(old, new, 1)
    if name in ('advx2.f', 'advy2.f'):
        for old, new in STMTFN:
            if old in s:
                s = s.replace(old, new, 1)
    for entry in FIXUPS.get(name, []):
        old, new = entry[0], entry[1]
        want = entry[2] if len(entry) > 2 else 1
        n = s.count(old)
        if n != want:
            raise SystemExit('%s: fixup matched %d times, expected %d:\n%r'
                             % (name, n, want, old[:70]))
        s = s.replace(old, new, want)
    if '@@CUT@@' in s:
        s = cut_units(s)
    for rx, rep in INTREAD:
        s = rx.sub(rep, s)
    for old, new, want in A2READ:
        n = s.count(old)
        if n:
            if n != want:
                raise SystemExit('%s: A2 read matched %d times, expected %d:'
                                 '\n%r' % (name, n, want, old))
            s = s.replace(old, new)
    s = READLU.sub('READ(5,', s)
    open(path, 'w', encoding='latin1', newline=chr(10)).write(s)


def main():
    for src, dst in SOURCES:
        out = os.path.join(SRC, dst)
        run(os.path.join(DISC, src), out)
        fixup(out)
    print('converted %d program files into src/' % len(SOURCES))


if __name__ == '__main__':
    main()
