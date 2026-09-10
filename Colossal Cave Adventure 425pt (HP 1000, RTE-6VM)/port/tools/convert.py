"""Regenerate port/src/*.f and *.fi from the sources recovered off the disc.

Two stages:

  1. `hp2gfortran` does the mechanical HP FTN7X -> gfortran-legacy dialect
     work (directives, Hollerith, octal, default integer widths, ...).

  2. The FIXUPS table below applies the handful of edits that need judgement
     rather than pattern matching.  Every entry must match exactly once, so
     the pipeline fails loudly if the inputs ever change underneath it.

`src/hprte.f` is *not* generated: it is new code standing in for the RTE
system library.  Run this from the port directory:  python tools/convert.py
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
DISC = os.path.join(PORT, os.pardir, 'src_original', 'disc')
SRC = os.path.join(PORT, 'src')

INCLUDES = ['ALPHAS', 'ARYCOM', 'IOCCOM', 'LINCOM', 'MAGCOM',
            'MISCOM', 'PLACOM', 'TRVCOM', 'TXTCOM', 'VOCCOM']
PROGRAMS = ['ADVENT', 'AINIT', 'AMAIN', 'ASUB', 'AIOSUB', 'ABUILD']

# ----------------------------------------------------------------------
# Edits that hp2gfortran cannot make on its own.
# ----------------------------------------------------------------------
FIXUPS = {

 'ioccom.fi': [
  # RTE drives one terminal LU for both directions.  gfortran preconnects
  # standard input and standard output as separate units, so the reads move
  # to a second unit, KBD.  IOCCOM is deliberately excluded from SAVE and
  # RESTORE, so extending it does not disturb the data base format.
  ("      INTEGER*2 CRT,LU",
   "      INTEGER*2 CRT,LU,KBD"),
  ("      COMMON /IOCCOM/ CRT,LU,BLKLIN,NOINPT,REVISION,RESTART,WIZARD",
   "      COMMON /IOCCOM/ CRT,LU,BLKLIN,NOINPT,REVISION,RESTART,WIZARD,KBD"),
 ],

 'advent.f': [
  ("      CRT = LOGLU(SES)",
   "      CRT = LOGLU(SES)\n      KBD = 5"),
 ],

 'abuild.f': [
  ("      CRT = LOGLU(SES)",
   "      CRT = LOGLU(SES)\n      KBD = 5"),
  # HP FMP status 506 is "file not found"; gfortran reports its own errno.
  ("  200 IF(STATUS.EQ.506) THEN",
   "  200 IF(NOFILE(FILENAME)) THEN"),
  ("      INTEGER*2 TRIMLEN,NFIOB ",
   "      INTEGER*2 TRIMLEN,NFIOB \n      LOGICAL*2 NOFILE"),
  # RTE's FMP files are typeless, so the revision probe could read a binary
  # word off a unit opened without FORM=.  gfortran has to be told.
  ("     &     ACCESS='SEQUENTIAL',ERR=200) \n"
   "      READ(LU,ERR=200,IOSTAT=STATUS) REV",
   "     &     ACCESS='SEQUENTIAL',FORM='UNFORMATTED',ERR=200) \n"
   "      READ(LU,ERR=200,IOSTAT=STATUS) REV"),
 ],

 'ainit.f': [
  ("  200 IF(STATUS.EQ.506 .AND. FILENAME.EQ.'ADVENTURE.DAT') THEN",
   "  200 IF(NOFILE(FILENAME) .AND. FILENAME.EQ.'ADVENTURE.DAT') THEN"),
  ("      INTEGER*2 TRIMLEN,NFIOB ",
   "      INTEGER*2 TRIMLEN,NFIOB \n      LOGICAL*2 NOFILE"),
  ("     &     ACCESS='SEQUENTIAL',ERR=200) \n"
   "      READ(LU,ERR=200,IOSTAT=STATUS) REV",
   "     &     ACCESS='SEQUENTIAL',FORM='UNFORMATTED',ERR=200) \n"
   "      READ(LU,ERR=200,IOSTAT=STATUS) REV"),
  # The original tells the player to go and run ABUILD; name the port's copy.
  ("  210   FORMAT(/\"Cannot find \",A,\", run ABUILD to create data base.\"/)",
   "  210   FORMAT(/\"Cannot find \",A,"
   "\", run abuild.exe to create data base.\"/)"),
 ],

 'aiosub.f': [
  # SAVE: HP FMP status 502 is "file already exists".
  ("  100 IF(ERROR.EQ.502) THEN",
   "  100 IF(HASFILE(NAME)) THEN"),
  # RESTORE: 506 is "file not found".
  ("  100 IF(ERROR.EQ.506) THEN",
   "  100 IF(NOFILE(NAME)) THEN"),
  # SAVE and RESTORE share this declaration block verbatim.
  ("      INTEGER*2 I,J,N,ERROR,TRIMLEN,NFIOB \n"
   "      INTEGER*2 ALPDAT(12),ARYDAT(750)",
   "      INTEGER*2 I,J,N,ERROR,TRIMLEN,NFIOB \n"
   "      LOGICAL*2 NOFILE,HASFILE\n"
   "      INTEGER*2 ALPDAT(12),ARYDAT(750)", 2),
  # An RTE terminal never reports end of file; a redirected standard input
  # does.  Treat it as the player walking away.
  ("      READ(KBD,300) BUFFER\n  300 FORMAT(A48) ",
   "      READ(KBD,300,END=9000) BUFFER\n  300 FORMAT(A48) "),
  ("D 500 FORMAT(\"GETIN returns: \",8A2,1X,8A2,1X,8A2) \nC \n      RETURN",
   "D 500 FORMAT(\"GETIN returns: \",8A2,1X,8A2,1X,8A2) \nC \n"
   "      RETURN\nC \n"
   " 9000 WRITE(CRT,'(/\"[end of input]\")')\n"
   "      CALL EXIT"),
  # SAVE appends RTE's ":::3:300" file-type-and-size suffix to the name.
  # There is no such thing here, and it would become part of the file name.
  ("      NAME(N+1:N+8)=':::3:300'\n      N=N+8",
   "C     RTE file type and size suffix; meaningless off the machine.\n"
   "C     NAME(N+1:N+8)=':::3:300'\n"
   "C     N=N+8"),
 ],

 'amain.f': [
  # gfortran type-checks statement-function arguments strictly, and its
  # integer literals are 32-bit while every variable here is 16-bit.  Give
  # the dummies that only ever receive literals a 32-bit type, and rename
  # the two that collide with real variables (L, N) so nothing else moves.
  ("      INTEGER*2 KLOD,DUMMY,HINT,ITEMP,I,J,K,INDEX \n",
   "      INTEGER*2 KLOD,HINT,ITEMP,I,J,K,INDEX \n"
   "C     Statement-function dummies handed literal constants.\n"
   "      INTEGER*4 DUMMY,NPCT,NBIT,PBOTL\n"),
  ("      INTEGER*2 ZZ1,ZZ2,ZZ3,ZZ4,OLDLC2,K2,LL,L,N,IQ,JQ,KQ,LQ,MQ \n",
   "      INTEGER*2 ZZ1,ZZ2,ZZ3,ZZ4,OLDLC2,K2,LL,L,N,IQ,JQ,KQ,LQ,MQ \n"
   "      INTEGER*2 LBIT\n"),
  ("      INTEGER*2 ATTACK,DTOTAL,STICK,KENT,PBOTL\n",
   "      INTEGER*2 ATTACK,DTOTAL,STICK,KENT\n"),
  ("      BITST(L,N)=IAND(COND(L),ISHFT(1,N)).NE.0\n",
   "      BITST(LBIT,NBIT)=IAND(COND(LBIT),ISHFT(1_2,NBIT)).NE.0\n"),
  ("      PCT(N)=(RNDM(100).LT.N) \n",
   "      PCT(NPCT)=(RNDM(100).LT.NPCT) \n"),
  ("          IF(.NOT.BITST(LOC,HINT)) HINTLC(HINT)=-1",
   "          IF(.NOT.BITST(LOC,INT(HINT))) HINTLC(HINT)=-1"),
  # HP FORTRAN reads an unsubscripted array in a scalar context as its
  # first element; line 1331 spells the same test out as WD2(1).EQ.0.
  (" 1660 IF((VERB.EQ.FIND.OR.VERB.EQ.INVENT).AND.WD2.EQ.0) GO TO 1590",
   " 1660 IF((VERB.EQ.FIND.OR.VERB.EQ.INVENT).AND.WD2(1).EQ.0) GO TO 1590"),
  (" 1730 IF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC)) GO TO 1710",
   " 1730 IF(NEWLOC.NE.0.AND..NOT.PCT(INT(NEWLOC))) GO TO 1710"),
  # End of file at the SAVE / RESTORE file-name prompts.
  (" 1920 CALL MSPEK(27)\n      READ(KBD,'(A)') FILENAME\n"
   "      CALL SAVE(FILENAME)\n"
   "      IF(.NOT.WIZARD .AND. PRIMTIM(0)) GO TO 2990\n"
   "      GO TO 1310",
   " 1920 CALL MSPEK(27)\n      READ(KBD,'(A)',END=1925) FILENAME\n"
   "      CALL SAVE(FILENAME)\n"
   "      IF(.NOT.WIZARD .AND. PRIMTIM(0)) GO TO 2990\n"
   "      GO TO 1310\n"
   " 1925 CALL EXIT"),
  (" 1930 CALL MSPEK(28)\n      READ(KBD,'(A)') FILENAME\n"
   "      CALL RESTORE(FILENAME)\n"
   "      CALL RSPEK(207)\n"
   "      GO TO 1310",
   " 1930 CALL MSPEK(28)\n      READ(KBD,'(A)',END=1935) FILENAME\n"
   "      CALL RESTORE(FILENAME)\n"
   "      CALL RSPEK(207)\n"
   "      GO TO 1310\n"
   " 1935 CALL EXIT"),
 ],

 'asub.f': [
  ("      INTEGER*2 I,K,DUMMY \n",
   "      INTEGER*2 I,K\n"
   "C     Statement-function dummy, only ever handed a literal.\n"
   "      INTEGER*4 DUMMY\n"),
  # An RTE terminal never reports end of file.  A redirected standard input
  # does, so treat it as a wrong answer rather than an abort.
  ("      READ(KBD,*,ERR=20) PASANS",
   "      READ(KBD,*,ERR=20,END=20) PASANS"),
 ],
}

# A trailing underscore in an HP FORTRAN format suppresses the newline;
# gfortran spells that as the $ edit descriptor.
NOADV = re.compile(r'_"\)')
# RTE's single terminal LU becomes two Fortran units.
READCRT = re.compile(r'READ(\s*)\(\s*CRT\s*,', re.I)
# An RTE terminal never reports end of file, so none of the terminal reads
# carry an END= branch.  A redirected standard input does, and without a
# branch gfortran aborts with a runtime error.  Inside MAGIC every read
# falls back to a RETURN appended at label 9000.
READKBD = re.compile(r'^(\s*(?:\d+\s+)?READ\(KBD,[^)]*?)\)', re.M)


def magic_eof(s):
    """Give every terminal read inside SUBROUTINE MAGIC an END= branch."""
    start = s.index('      SUBROUTINE MAGIC')
    end = s.index('\n      END \n', start)
    body = s[start:end]

    def add(m):
        return m.group(1) + (')' if 'END=' in m.group(1)
                             else ',END=9000)')

    body = READKBD.sub(add, body)
    body += '\nC \n 9000 RETURN'
    return s[:start] + body + s[end:]


def run(src, dst):
    subprocess.check_call([sys.executable,
                           os.path.join(HERE, 'hp2gfortran.py'), src, dst])


def fixup(path):
    name = os.path.basename(path)
    s = open(path, encoding='latin1').read()
    s = NOADV.sub('",$)', s)
    s = READCRT.sub(lambda m: 'READ' + m.group(1) + '(KBD,', s)
    if name == 'aiosub.f':
        s = magic_eof(s)
    for entry in FIXUPS.get(name, []):
        old, new = entry[0], entry[1]
        want = entry[2] if len(entry) > 2 else 1
        n = s.count(old)
        if n != want:
            raise SystemExit('%s: fixup matched %d times, expected %d:\n%r'
                             % (name, n, want, old[:70]))
        s = s.replace(old, new, want)
    open(path, 'w', encoding='latin1', newline=chr(10)).write(s)


def main():
    for f in INCLUDES:
        dst = os.path.join(SRC, f.lower() + '.fi')
        run(os.path.join(DISC, f + '.INCL'), dst)
        fixup(dst)
    for f in PROGRAMS:
        dst = os.path.join(SRC, f.lower() + '.f')
        run(os.path.join(DISC, f + '.FTN'), dst)
        fixup(dst)
    print('converted %d includes and %d program files into src/'
          % (len(INCLUDES), len(PROGRAMS)))


if __name__ == '__main__':
    main()
