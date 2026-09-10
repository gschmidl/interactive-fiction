# -*- coding: latin-1 -*-
"""Source-level patches applied after the mechanical conversion.

Everything here exists for one reason: gfortran's A edit descriptor packs
eight-bit bytes, while this program's words are five seven-bit characters.
The A-format reads and writes therefore go through C2W/WSTR in runtime.f.
Game logic is untouched -- these are I/O statements, plus the four
routines that were machine-specific (SHIFT, RAN, DATIME, GETIN).
"""

# routines superseded by runtime.f, dropped from adv4su
DROP_ROUTINES = ['INTEGER FUNCTION SHIFT(VAL,DIST)',
                 'INTEGER FUNCTION RAN10(RANGE)',
                 'SUBROUTINE DATIME(D,T)',
                 'SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)']

NEW_GETIN = r"""        SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)
C
C  Rewritten for the port.  The original did ACCEPT 4A5 and then walked
C  the four words a 7-bit field at a time, blanking WORD1 at the first
C  space and sliding WORD2 out across a word boundary.  Reading the line
C  as characters and packing the two tokens with C2W gives the same words:
C  WORD1/WORD1X are characters 1-5 and 6-10 of the first token, WORD2/
C  WORD2X the same for the second.  As before only the first 20 columns
C  are significant, and case is folded up (the original cleared the 0100
C  bit arithmetically).
C
        IMPLICIT INTEGER(A-Z)
        LOGICAL BLKLIN
        COMMON /BLKCOM/ BLKLIN
        CHARACTER*20 LINE
        CHARACTER*10 T1,T2
        IF(BLKLIN)PRINT 1
1       FORMAT()
2       LINE=' '
        READ(5,3,END=90)LINE
3       FORMAT(A20)
        DO 4 I=1,20
        C=ICHAR(LINE(I:I))
        IF(C.GE.97.AND.C.LE.122)LINE(I:I)=CHAR(C-32)
4       CONTINUE
        IF(BLKLIN.AND.LINE.EQ.' ')GOTO 2
        T1=' '
        T2=' '
        I=1
C  .AND. is not short-circuit in Fortran, so the index test and the
C  character test have to be separate or LINE(21:21) gets evaluated.
5       IF(I.GT.20)GOTO 10
        IF(LINE(I:I).NE.' ')GOTO 10
        I=I+1
        GOTO 5
10      J=0
11      IF(I.GT.20)GOTO 20
        IF(LINE(I:I).EQ.' ')GOTO 20
        J=J+1
        IF(J.LE.10)T1(J:J)=LINE(I:I)
        I=I+1
        GOTO 11
20      IF(I.GT.20)GOTO 30
        IF(LINE(I:I).NE.' ')GOTO 30
        I=I+1
        GOTO 20
30      J=0
31      IF(I.GT.20)GOTO 40
        IF(LINE(I:I).EQ.' ')GOTO 40
        J=J+1
        IF(J.LE.10)T2(J:J)=LINE(I:I)
        I=I+1
        GOTO 31
40      CONTINUE
        CALL C2W(T1(1:5),5,WORD1)
        CALL C2W(T1(6:10),5,WORD1X)
        WORD2=0
        WORD2X=0
        IF(T2.NE.' ')THEN
           CALL C2W(T2(1:5),5,WORD2)
           CALL C2W(T2(6:10),5,WORD2X)
        ENDIF
        RETURN
90      CALL EXIT
        END
"""

# exact-line replacements: file -> [(old, [new lines])]
LINES = {
 'adv4su': [
  # SPEAK
  ("      DIMENSION RTEXT (250),LINES(15000),LINE(18)",
   ["      DIMENSION RTEXT (250),LINES(15000),LINE(18)",
    "      CHARACTER*160 WSTR"]),
  ("      WRITE (6,2)(LINE(I),I=1,UPLIM)",
   ["      WRITE (6,2) TRIM(WSTR(LINE,UPLIM,4))"]),
  ("2     FORMAT(' ',18A4)", ["2     FORMAT(' ',A)"]),
  # MAINT: holiday name read, 5A4
  ("      READ (5,2)HNAME", ["      READ (5,3011)HOLNAM",
                             "      CALL C2WS(HOLNAM,5,4,HNAME)"]),
  ("2     FORMAT(5A4)", ["3011  FORMAT(A20)"]),
  # WIZARD: magic word echo, A5
  ("      TYPE 18,WORD", ["      PRINT 18, TRIM(WSTR(WORD,1,5))"]),
  ("18    FORMAT(/1X,A5)", ["18    FORMAT(/1X,A)"]),
  # HOURS: holiday messages
  ("      WRITE (6,5)HNAME", ["      WRITE (6,5) TRIM(WSTR(HNAME,4,4))"]),
  ("5     FORMAT(/' TODAY IS A HOLIDAY, NAMELY ',4A4)",
   ["5     FORMAT(/' TODAY IS A HOLIDAY, NAMELY ',A)"]),
  ("      WRITE(6,20)D,HNAME", ["      WRITE(6,20)D,TRIM(WSTR(HNAME,5,4))"]),
  ("12    WRITE(6,25)D,HNAME", ["12    WRITE(6,25)D,TRIM(WSTR(HNAME,5,4))"]),
  ("20    FORMAT(/' THE NEXT HOLIDAY WILL BE IN',I3,' DAYS, NAMELY ',5A4)",
   ["20    FORMAT(/' THE NEXT HOLIDAY WILL BE IN',I3,' DAYS, NAMELY ',A)"]),
  ("25    FORMAT(/' THE NEXT HOLIDAY WILL BE IN',I3,' DAY, NAMELY ',5A4)",
   ["25    FORMAT(/' THE NEXT HOLIDAY WILL BE IN',I3,' DAY, NAMELY ',A)"]),
  # HOURSX: day names
  ("      WRITE (6,2)DAY1,DAY2,DAY3",
   ["      WRITE (6,2) TRIM(WSTR(DAYS,3,4))"]),
  ("2     FORMAT(10X,3A4,'  OPEN ALL DAY')",
   ["2     FORMAT(10X,A,'  OPEN ALL DAY')"]),
  ("      IF(FIRST)WRITE (6,16) DAY1,DAY2,DAY3,FROM,TILL",
   ["      IF(FIRST)WRITE (6,16) TRIM(WSTR(DAYS,3,4)),FROM,TILL"]),
  ("16    FORMAT(10X,3A4,I4,':00 TO',I3,':00')",
   ["16    FORMAT(10X,A,I4,':00 TO',I3,':00')"]),
  ("20    IF(FIRST)WRITE (6,22) DAY1,DAY2,DAY3",
   ["20    IF(FIRST)WRITE (6,22) TRIM(WSTR(DAYS,3,4))"]),
  ("22    FORMAT(10X,3A4,'  CLOSED ALL DAY')",
   ["22    FORMAT(10X,A,'  CLOSED ALL DAY')"]),
  # NEWHRX
  ("      TYPE 1,DAY1,DAY2,DAY3", ["      PRINT 1, TRIM(WSTR(DAYS,3,4))"]),
  ("1     FORMAT(' PRIME TIME ON ',3A4)",
   ["1     FORMAT(' PRIME TIME ON ',A)"]),
  # MOTD
  ("      WRITE(6,20)(MSG(I),I=LIM1,LIM2)",
   ["      WRITE(6,20) TRIM(WSTR(MSG(LIM1),LIM2-LIM1+1,4))"]),
  ("20    FORMAT(' ',18A4)", ["20    FORMAT(' ',A)"]),
  ("      READ(5,56)(MSG(I),I=LIM1,LIM2),K",
   ["      READ(5,56)MOTDLN",
    "      CALL C2WS(MOTDLN,19,4,MSG(LIM1))",
    "      CALL C2W(MOTDLN(73:76),4,K)"]),
  ("56    FORMAT(19A4)", ["56    FORMAT(A76)"]),
  # LOAD
  ("      WRITE (6,10) WORD", ["      WRITE (6,10) TRIM(WSTR(WORD,1,4))"]),
  ("10    FORMAT (' LOAD OPTION ',A4,' NOT IMPLEMENTED')",
   ["10    FORMAT (' LOAD OPTION ',A,' NOT IMPLEMENTED')"]),
 ],
 'adv4ma': [
  ("      READ(1,1005)LOC,(LINES(J),J=ERA2,ERA3)",
   ["      READ(1,1005)LOC,DBLINE",
    "      CALL C2WS(DBLINE,18,4,LINES(ERA2))"]),
  ("1005  FORMAT(1I8,18A4)", ["1005  FORMAT(1I8,A72)"]),
  ("1043  READ(1,1041)KTAB(TABNDX),ATAB(TABNDX)",
   ["1043  READ(1,1041)KTAB(TABNDX),VOCLIN",
    "      CALL C2W(VOCLIN,5,ATAB(TABNDX))"]),
  ("1041  FORMAT (I7,A5)", ["1041  FORMAT (I7,A5)"]),
 ],
}

LINES['adv4ma'] += [
  ("      PRINT 5015,(TK(I),I=1,K)", ["      PRINT 5015, TRIM(WSTR(TK,K,1))"]),
  ("5015  FORMAT(/' WHAT DO YOU WANT TO DO WITH THE ',20A1)",
   ["5015  FORMAT(/' WHAT DO YOU WANT TO DO WITH THE ',A)"]),
  ("      PRINT 5199,(TK(I),I=1,K)", ["      PRINT 5199, TRIM(WSTR(TK,K,1))"]),
  ("5199  FORMAT(/' I SEE NO ',20A1)",
   ["5199  FORMAT(/' I SEE NO ',A)"]),
  ("      PRINT 8002,(TK(I),I=1,K)", ["      PRINT 8002, TRIM(WSTR(TK,K,1))"]),
  ('8002  FORMAT(/' + chr(39) + ' ' + chr(39) + ',20A1)',
   ['8002  FORMAT(/' + chr(39) + ' ' + chr(39) + ',A)']),
  ("      PRINT 9032,(TK(I),I=1,K)", ["      PRINT 9032, TRIM(WSTR(TK,K,1))"]),
  ('9032  FORMAT(/' + chr(39) + ' OKAY, ' + chr(34) + chr(39) + ',20A1)',
   ['9032  FORMAT(/' + chr(39) + ' OKAY, ' + chr(34) + chr(39) + ',A)']),
]

# declarations inserted after a routine's IMPLICIT line
DECLS = {
  'MAINT':  ['      CHARACTER*20 HOLNAM'],
  'WIZARD': ['      CHARACTER*160 WSTR'],
  'HOURS':  ['      CHARACTER*160 WSTR'],
  'HOURSX': ['      CHARACTER*160 WSTR', '      DIMENSION DAYS(3)'],
  'NEWHRX': ['      CHARACTER*160 WSTR', '      DIMENSION DAYS(3)'],
  'MOTD':   ['      CHARACTER*160 WSTR', '      CHARACTER*76 MOTDLN'],
  'LOAD':   ['      CHARACTER*160 WSTR'],
  '@MAIN':  ['      CHARACTER*160 WSTR', '      CHARACTER*72 DBLINE',
             '      CHARACTER*5 VOCLIN'],
}

# DAY1/2/3 arrive as separate arguments; WSTR wants them contiguous
DAYSFIX = [
  ("      FIRST=.TRUE.", ["      DAYS(1)=DAY1", "      DAYS(2)=DAY2",
                          "      DAYS(3)=DAY3", "      FIRST=.TRUE."]),
  ("      NEWHRX=0", ["      DAYS(1)=DAY1", "      DAYS(2)=DAY2",
                      "      DAYS(3)=DAY3", "      NEWHRX=0"]),
]
LINES['adv4su'] += DAYSFIX

# TK1 is DIMENSION 9 and FORMAT(11I7) fills exactly 9, but the travel
# loop scans to 20 and ran off the end.  It survived on the -10 only
# because the word after TK1 happened to be zero.  The author's own
# sibling loop over the same record (1070/1071) uses 1,9, and exactly
# one section-3 record fills all nine fields -- the one that trips it.
# A location's options simply continue on the next record.
LINES['adv4ma'] += [
  ('1035  DO 1037 L=1,20', ['1035  DO 1037 L=1,9']),
]
