C=======================================================================
C  hprte.f -- stand-ins for the HP 1000 RTE-6/VM library routines that
C  "Adventure II" version 2.2 calls.  None of this is translated code:
C  the originals live in the RTE system libraries, which do not exist off
C  the machine, so each one is reimplemented here in portable Fortran.
C
C  Covered here:
C     LOGLU   session terminal LU              -> Fortran stdout unit
C     TRIMLEN length ignoring trailing blanks  -> LEN_TRIM
C     CASEFOLD, CLCUC   fold to upper case
C     SPLITSTRING       peel the first blank-delimited token off a string
C     FPARM             fetch the runstring (command line) parameter
C     FTIME             30-character date/time stamp in RTE's layout
C     SSEED, URAN       RTE's seeded uniform random number generator
C     NFIOB             FMP buffer count (advisory only)
C     EXEC              RTE executive calls 6 (terminate), 7 (suspend),
C                       11 (time request)
C     FMPPURGE          delete a file
C     FMPREPORTERROR    report an FMP status code
C     UserIsSuper       CI "super user" test
C=======================================================================

      INTEGER*2 FUNCTION LOGLU(SES)
C
C  RTE hands back the LU of the session's terminal.  Here the terminal is
C  the process's standard output, which gfortran preconnects to unit 6;
C  standard input is unit 5 and is reached through KBD (see ioccom.fi).
C
      IMPLICIT NONE
      INTEGER*2 SES
      SES = 0
      LOGLU = 6
      RETURN
      END

      INTEGER*2 FUNCTION TRIMLEN(BUFFER)
C
      IMPLICIT NONE
      CHARACTER*(*) BUFFER
      TRIMLEN = LEN_TRIM(BUFFER)
      RETURN
      END

      SUBROUTINE CASEFOLD(BUFFER)
C
C  RTE's CASEFOLD raises a whole string to upper case in place.
C
      IMPLICIT NONE
      CHARACTER*(*) BUFFER
      INTEGER I, C
      DO I = 1, LEN(BUFFER)
        C = ICHAR(BUFFER(I:I))
        IF (C .GE. 97 .AND. C .LE. 122) BUFFER(I:I) = CHAR(C - 32)
      ENDDO
      RETURN
      END

      SUBROUTINE CLCUC(BUF, NWORDS)
C
C  RTE's CLCUC raises NWORDS 16-bit words (two characters each) to upper
C  case in place.  The callers hand it INTEGER*2 arrays, not CHARACTERs.
C
      IMPLICIT NONE
      INTEGER*2 BUF(*), NWORDS
      INTEGER I, N, LO, HI, V
      N = NWORDS
      DO I = 1, N
        V = BUF(I)
        IF (V .LT. 0) V = V + 65536
        LO = MOD(V, 256)
        HI = V / 256
        IF (LO .GE. 97 .AND. LO .LE. 122) LO = LO - 32
        IF (HI .GE. 97 .AND. HI .LE. 122) HI = HI - 32
        V = LO + HI * 256
        IF (V .GT. 32767) V = V - 65536
        BUF(I) = V
      ENDDO
      RETURN
      END

      SUBROUTINE SPLITSTRING(SOURCE, FIRST, REST)
C
C  Peel the first token off SOURCE into FIRST and leave what follows in
C  REST.  Blanks and commas both delimit, which is what both callers need:
C  the data base reader splits "1001,KEYS" and "31,  OH - Opening Hours",
C  while GETIN splits "get lamp".
C
C  Exactly one delimiter is consumed, so leading blanks in the remainder
C  survive -- the MAGIC menu in section 12 is laid out with them.  REST may
C  be the same variable as SOURCE, so the tail is copied out first.
C
      IMPLICIT NONE
      CHARACTER*(*) SOURCE, FIRST, REST
      CHARACTER*256 WORK, TAIL
      INTEGER I, N, B, E
      LOGICAL ISDELIM
      ISDELIM(I) = WORK(I:I) .EQ. ' ' .OR. WORK(I:I) .EQ. ','
C
      WORK = SOURCE
      N = LEN_TRIM(WORK)
      B = 0
      DO I = 1, N
        IF (.NOT. ISDELIM(I)) THEN
          B = I
          GO TO 10
        ENDIF
      ENDDO
   10 IF (B .EQ. 0) THEN
        FIRST = ' '
        REST = ' '
        RETURN
      ENDIF
      E = N
      DO I = B, N
        IF (ISDELIM(I)) THEN
          E = I - 1
          GO TO 20
        ENDIF
      ENDDO
   20 TAIL = ' '
      IF (E + 1 .LT. LEN(WORK)) TAIL = WORK(E+2:)
      FIRST = WORK(B:E)
      REST = TAIL
      RETURN
      END

      SUBROUTINE FPARM(BUFFER)
C
C  RTE's FPARM returns the file name given on the runstring.  Here that is
C  the first command-line argument; with none, BUFFER keeps its default.
C
      IMPLICIT NONE
      CHARACTER*(*) BUFFER
      CHARACTER*256 ARG
      INTEGER N
      N = COMMAND_ARGUMENT_COUNT()
      IF (N .LT. 1) RETURN
      CALL GET_COMMAND_ARGUMENT(1, ARG)
      IF (LEN_TRIM(ARG) .GT. 0) BUFFER = ARG
      RETURN
      END

      SUBROUTINE FTIME(BUF)
C
C  RTE's FTIME fills 30 characters with
C
C        HH:MM PM  DAY., DD  MON., YEAR
C        123456789012345678901234567890
C
C  PRIMTIM in asub.f reads that layout back with an explicit FORMAT, so
C  the column positions matter.
C
      IMPLICIT NONE
      INTEGER*2 BUF(15)
      CHARACTER*30 S
      INTEGER*2 IS(15)
      INTEGER V(8)
      INTEGER H, I, IDOW
      CHARACTER*4 MONS(12), DAYS(7)
      CHARACTER*2 AP
      EQUIVALENCE (S, IS)
      DATA MONS /'JAN.','FEB.','MAR.','APR.','MAY ','JUNE',
     &           'JULY','AUG.','SEP.','OCT.','NOV.','DEC.'/
      DATA DAYS /'SUN.','MON.','TUE.','WED.','THU.','FRI.','SAT.'/
C
      CALL DATE_AND_TIME(VALUES=V)
      H = V(5)
      AP = 'AM'
      IF (H .GE. 12) AP = 'PM'
      IF (H .GT. 12) H = H - 12
      IF (H .EQ. 0) H = 12
      S = ' '
      WRITE(S,100) H, V(6), AP, DAYS(IDOW(V(1),V(2),V(3))),
     &             V(3), MONS(V(2)), V(1)
  100 FORMAT(I2.2,':',I2.2,1X,A2,2X,A4,', ',I2.2,2X,A4,', ',I4)
      DO I = 1, 15
        BUF(I) = IS(I)
      ENDDO
      RETURN
      END

      INTEGER FUNCTION IDOW(IY, IM, ID)
C
C  Day of week, 1 = Sunday, by Sakamoto's method.
C
      IMPLICIT NONE
      INTEGER IY, IM, ID, Y, T(12)
      DATA T /0,3,2,5,0,3,5,1,4,6,2,4/
      Y = IY
      IF (IM .LT. 3) Y = Y - 1
      IDOW = MOD(Y + Y/4 - Y/100 + Y/400 + T(IM) + ID, 7) + 1
      RETURN
      END

      SUBROUTINE SSEED(N)
C
      IMPLICIT NONE
      INTEGER*2 N
      INTEGER*4 SEED
      COMMON /HPRND/ SEED
      SEED = IAND(INT(N,4), 32767) * 65539 + 1
      IF (SEED .EQ. 0) SEED = 1
      RETURN
      END

      REAL*4 FUNCTION URAN(IDUMMY)
C
C  A 32-bit linear congruential generator standing in for RTE's URAN.
C  Returns a uniform deviate in [0,1).  The HP generator's exact stream
C  cannot be reproduced off the machine, and the game only ever seeds it
C  from the wall clock, so any decent generator is faithful in substance.
C
      IMPLICIT NONE
      INTEGER*2 IDUMMY
      INTEGER*4 SEED
      COMMON /HPRND/ SEED
      IF (SEED .EQ. 0) SEED = 123459876
      SEED = SEED * 1103515245 + 12345
      URAN = REAL(IAND(ISHFT(SEED, -16), 32767)) / 32768.0
      RETURN
      END

      INTEGER*2 FUNCTION NFIOB(IDUMMY)
C
C  Number of FMP disc buffers.  Advisory only once the BUFSIZ= specifiers
C  are gone; the value is printed by a debug line and nothing else.
C
      IMPLICIT NONE
      INTEGER*2 IDUMMY
      NFIOB = 8
      RETURN
      END

      SUBROUTINE EXEC(ICODE, IA, IB)
C
C  The three RTE executive calls this program makes.
C
C     EXEC(6)            terminate
C     EXEC(7)            suspend until the operator restarts the program
C     EXEC(11,ITIME,IY)  time request: ITIME = (10ms ticks, seconds,
C                        minutes, hours, day-of-year), IY = year
C
      IMPLICIT NONE
      INTEGER*2 ICODE, IA(*), IB
      INTEGER V(8), I, DOY, MD(12)
      CHARACTER*1 ANS
      DATA MD /0,31,59,90,120,151,181,212,243,273,304,334/
C
      IF (ICODE .EQ. 6) THEN
        CALL EXIT
      ELSEIF (ICODE .EQ. 7) THEN
        WRITE(*,'(/"[Game suspended.  Press RETURN to resume.]")')
        READ(5,'(A)',END=900,ERR=900) ANS
  900   RETURN
      ELSEIF (ICODE .EQ. 11) THEN
        CALL DATE_AND_TIME(VALUES=V)
        DOY = MD(V(2)) + V(3)
        IF (V(2) .GT. 2 .AND. MOD(V(1),4) .EQ. 0 .AND.
     &      (MOD(V(1),100) .NE. 0 .OR. MOD(V(1),400) .EQ. 0))
     &      DOY = DOY + 1
        IA(1) = V(8) / 10
        IA(2) = V(7)
        IA(3) = V(6)
        IA(4) = V(5)
        IA(5) = DOY
        IB = V(1)
      ENDIF
      RETURN
      END

      INTEGER*2 FUNCTION FMPPURGE(NAME)
C
      IMPLICIT NONE
      CHARACTER*(*) NAME
      INTEGER U, IOS
      LOGICAL THERE
      U = 47
      FMPPURGE = 0
      INQUIRE(FILE=NAME, EXIST=THERE)
      IF (.NOT. THERE) THEN
        FMPPURGE = -6
        RETURN
      ENDIF
      OPEN(UNIT=U, FILE=NAME, STATUS='OLD', IOSTAT=IOS)
      IF (IOS .NE. 0) THEN
        FMPPURGE = IOS
        RETURN
      ENDIF
      CLOSE(UNIT=U, STATUS='DELETE', IOSTAT=IOS)
      FMPPURGE = IOS
      RETURN
      END

      SUBROUTINE FMPREPORTERROR(IERR, NAME)
C
      IMPLICIT NONE
      INTEGER*2 IERR
      CHARACTER*(*) NAME
      WRITE(*,10) IERR, NAME(1:MAX(1,LEN_TRIM(NAME)))
   10 FORMAT(/"FMP error",I6," on ",A)
      RETURN
      END

      LOGICAL*2 FUNCTION NOFILE(NAME)
C
C  Stands in for the tests the original makes against FMP status 506,
C  "file not found".  gfortran reports its own errno instead, so ask the
C  file system directly.
C
      IMPLICIT NONE
      CHARACTER*(*) NAME
      LOGICAL THERE
      INQUIRE(FILE=NAME(1:MAX(1,LEN_TRIM(NAME))), EXIST=THERE)
      NOFILE = .NOT. THERE
      RETURN
      END

      LOGICAL*2 FUNCTION HASFILE(NAME)
C
C  Likewise for FMP status 502, "file already exists".
C
      IMPLICIT NONE
      CHARACTER*(*) NAME
      LOGICAL THERE
      INQUIRE(FILE=NAME(1:MAX(1,LEN_TRIM(NAME))), EXIST=THERE)
      HASFILE = THERE
      RETURN
      END

      LOGICAL*2 FUNCTION UserIsSuper(IDUMMY)
C
C  CI's "is this a super user" test.  There is no RTE account system here.
C
      IMPLICIT NONE
      INTEGER*2 IDUMMY
      UserIsSuper = .TRUE.
      RETURN
      END
