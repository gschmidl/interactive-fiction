      SUBROUTINE POPTS(PROG)
C
C  PORT: COMMAND LINE OPTIONS.  NONE OF THIS IS IN THE ORIGINAL, WHICH
C  WAS STARTED FROM JCL AND TOOK ITS FILES FROM DD CARDS.
C
C     PROG = 1 ADVWIZ, 2 ADVENT, 3 ADVENTST (THE SAVE/RESTORE BUILD)
C
      IMPLICIT INTEGER(A-Z)
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ,PROGID,PDONE
      CHARACTER*80 ARG
C
      NOFIX=0
      FDATE=0
      FTIME=0
      AUTOWZ=0
      PROGID=PROG
      PDONE=0
C
      N=COMMAND_ARGUMENT_COUNT()
      I=0
1     I=I+1
      IF(I.GT.N)RETURN
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      IF(ARG.EQ.'--no-fixes')GOTO 10
      IF(ARG.EQ.'--auto'.AND.PROG.EQ.1)GOTO 11
      IF(ARG.EQ.'--date')GOTO 20
      IF(ARG.EQ.'--time')GOTO 30
      IF(ARG.EQ.'-h'.OR.ARG.EQ.'--help')GOTO 100
      WRITE(6,2)ARG(1:MAX(1,LEN_TRIM(ARG)))
2     FORMAT(' unknown option: ',A)
      GOTO 100
C
10    NOFIX=1
      GOTO 1
11    AUTOWZ=1
      GOTO 1
C
C  --date YYDDD AND --time HHMM FREEZE THE CLOCK.  DATIME IS WHAT SEEDS
C  THE RANDOM NUMBER GENERATOR AND WHAT DECIDES PRIME TIME, SO WITH BOTH
C  GIVEN A RUN IS REPRODUCIBLE.
C
20    I=I+1
      IF(I.GT.N)GOTO 100
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      READ(ARG,*,ERR=100)FDATE
      IF(FDATE.LE.0)GOTO 100
      GOTO 1
30    I=I+1
      IF(I.GT.N)GOTO 100
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      READ(ARG,*,ERR=100)K
      IF(K.LT.0.OR.K.GT.2359)GOTO 100
      FTIME=(K/100)*60+MOD(K,100)
      GOTO 1
C
100   IF(PROG.EQ.1)WRITE(6,101)
      IF(PROG.NE.1)WRITE(6,102)
101   FORMAT(
     1' usage: advwiz [options]   (writes advent.ini, then advent',
     2' can be played)',/
     3'   --auto        pass the wizard test without being asked',/
     4'   --date YYDDD  pretend today is Julian date YYDDD',/
     5'   --time HHMM   pretend the time is HH:MM',/
     6'   --no-fixes    leave the port fixes out (see README)',/
     7'   -h, --help    this text')
102   FORMAT(
     1' usage: advent [options]',/
     2'   --date YYDDD  pretend today is Julian date YYDDD',/
     3'   --time HHMM   pretend the time is HH:MM',/
     4'   --no-fixes    leave the port fixes out (see README)',/
     5'   -h, --help    this text')
      STOP
      END
