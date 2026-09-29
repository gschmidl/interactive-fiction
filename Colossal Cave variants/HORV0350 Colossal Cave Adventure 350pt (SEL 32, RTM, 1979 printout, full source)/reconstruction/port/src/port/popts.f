      SUBROUTINE POPTS
C
C  PORT: COMMAND LINE OPTIONS.  NONE OF THIS IS IN THE ORIGINAL, WHICH WAS
C  STARTED FROM AN RTM JOB DECK.
C
      IMPLICIT INTEGER*4(A-Z)
      COMMON /POPTCM/ NOFIX,FDATE,FTIME,AUTOWZ
      COMMON /PUNCOM/ PUNL
      CHARACTER*80 ARG
C
      NOFIX=0
      FDATE=-1
      FTIME=-1
      AUTOWZ=0
      PUNL=0
C
      N=COMMAND_ARGUMENT_COUNT()
      I=0
1     I=I+1
      IF(I.GT.N)RETURN
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      IF(ARG.EQ.'-u'.OR.ARG.EQ.'--unlimited')GOTO 12
      IF(ARG.EQ.'--no-fixes')GOTO 10
      IF(ARG.EQ.'--auto')GOTO 11
      IF(ARG.EQ.'--day')GOTO 20
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
C  -u: NO PRIME TIME (SO NO DEMONSTRATION GAME) AND NO WAIT BEFORE A
C  SUSPENDED GAME MAY BE RESTORED.  START LOOKS AT PUNL.
C
12    PUNL=1
      GOTO 1
C
C  --day N AND --time HHMM FREEZE THE CLOCK.  DATIME IS WHAT SEEDS THE
C  RANDOM NUMBER GENERATOR AND WHAT DECIDES PRIME TIME, SO WITH BOTH GIVEN
C  A RUN IS REPRODUCIBLE.  DAY 0 IS SATURDAY 1 JULY 1978, AS IN DATIME.
C
20    I=I+1
      IF(I.GT.N)GOTO 100
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      READ(ARG,*,ERR=100)FDATE
      IF(FDATE.LT.0)GOTO 100
      GOTO 1
30    I=I+1
      IF(I.GT.N)GOTO 100
      CALL GET_COMMAND_ARGUMENT(I,ARG)
      READ(ARG,*,ERR=100)K
      IF(K.LT.0.OR.K.GT.2359)GOTO 100
      FTIME=(K/100)*60+MOD(K,100)
      GOTO 1
C
100   WRITE(6,101)
101   FORMAT(
     1' usage: advent [options]',/
     1'   -u, --unlimited  no prime time and no wait before a',
     1' restored game (run.bat passes it)',/
     2'   --auto        pass the wizard test without being asked,',
     3' for setting the game up',/
     4'   --day N       pretend N days have passed since 1 July 1978',/
     5'   --time HHMM   pretend the time is HH:MM',/
     6'   --no-fixes    leave the port fixes out (see README)',/
     7'   -h, --help    this text')
      STOP
      END
