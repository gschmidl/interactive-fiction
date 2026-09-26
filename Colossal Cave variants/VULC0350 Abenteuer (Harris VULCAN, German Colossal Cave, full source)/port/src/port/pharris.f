C  pharris.f - what ABENTEUER got from the Harris: the assembler routines
C  at the end of its job stream (SHIFT, LOWSIX, ABORT, ATTACH, ASSIGN,
C  GENRAT, IO), the library's JDATE, IRANP and IRAN, and the three site
C  routines that opened files by their Harris names (IOINIT, LDCOMN,
C  SVCOMN).  pharrisc.c has what Fortran cannot do.
C
C  ------------------------------------------------------------------
C  SHIFT(VAL,N): "DOUBLE PRECISION INTEGER*6 ROTATE" says the assembler
C  source, but it shifts (LAD and RAD, arithmetic double shifts): left
C  for N >= 0, right for N < 0.
C  ------------------------------------------------------------------
      INTEGER*8 FUNCTION SHIFT(VAL, N)
      INTEGER*8 VAL, N
      IF (N .GE. 0) THEN
        SHIFT = ISHFT(VAL, INT(N))
      ELSE
        SHIFT = SHIFTA(VAL, INT(-N))
      END IF
      RETURN
      END

C  LOWSIX(V): the low six bits (DMA ='77)
      INTEGER*8 FUNCTION LOWSIX(V)
      INTEGER*8 V
      LOWSIX = IAND(V, 63_8)
      RETURN
      END

C  ADDR(X,A): the address of X.  The main program cannot say LOC(X)
C  itself - its LOC is the player's location (COMMON /PLACOM/) - so
C  src\convert.py makes each call PADDR(X).
      INTEGER*8 FUNCTION PADDR(X)
      INTEGER*8 X
      PADDR = LOC(X)
      RETURN
      END

C  ABORT: the job's end, after BUG has said why
      SUBROUTINE PABORT
      FLUSH (6)
      CALL EXIT(1)
      END

C  ------------------------------------------------------------------
C  The clock.  JDATE(J): J(1) is the year times 4096 plus the day of
C  the year, J(2) the time of day in tenths of seconds - what DATIME
C  takes apart (it counts days from a day in 1976 so that D mod 7 is 0
C  on a Saturday, and minutes).  --date and --time hold it still.
C  ------------------------------------------------------------------
      SUBROUTINE JDATE(J)
      INTEGER*4 J(2)
      INTEGER*4 V(8), DOY, M, MDAYS(12)
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      DATA MDAYS/31,28,31,30,31,30,31,31,30,31,30,31/
      CALL DATE_AND_TIME(VALUES=V)
      IF (KDATE(1) .NE. 0) THEN
        V(1) = KDATE(1)
        V(2) = KDATE(2)
        V(3) = KDATE(3)
      END IF
      IF (KTIME .GE. 0) THEN
        V(5) = KTIME / 60
        V(6) = MOD(KTIME, 60)
        V(7) = 0
        V(8) = 0
      END IF
      DOY = V(3)
      DO 10 M = 1, V(2) - 1
      DOY = DOY + MDAYS(M)
      IF (M .EQ. 2 .AND. MOD(V(1), 4) .EQ. 0 .AND.
     +    (MOD(V(1), 100) .NE. 0 .OR. MOD(V(1), 400) .EQ. 0))
     +  DOY = DOY + 1
10    CONTINUE
      J(1) = V(1) * 4096 + DOY
      J(2) = ((V(5) * 60 + V(6)) * 60 + V(7)) * 10 + V(8) / 100
      RETURN
      END

C  ------------------------------------------------------------------
C  IRANP(N) seeded and IRAN(LO,HI) drew from the Harris library's
C  generator, which is not known: a stand-in (Lehmer's minimal
C  standard).  RAN seeds it once a game with the minute of the day;
C  --seed puts a number of the player's in its place.
C  ------------------------------------------------------------------
      SUBROUTINE IRANP(N)
      INTEGER*4 N
      INTEGER*8 S
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      COMMON /PRAND/ S
      S = N
      IF (KSEEDF .NE. 0) S = KSEED
      S = MOD(ABS(S), 2147483646_8) + 1
      RETURN
      END

      INTEGER*4 FUNCTION IRAN(LO, HI)
      INTEGER*4 LO, HI
      INTEGER*8 S
      COMMON /PRAND/ S
      IF (S .LE. 0) S = 1
      S = MOD(16807_8 * S, 2147483647_8)
      IRAN = LO + INT(MOD(S, INT(HI - LO + 1, 8)), 4)
      RETURN
      END

C  ------------------------------------------------------------------
C  IOINIT: the terminal was logical units 0 (in) and 3 (out), the
C  database the file ADV.DATA attached as unit 10.  gfortran's units 5
C  and 6 are the terminal; ADV.DATA is beside the program (PINIT went
C  there).  --fresh=1980 attaches ADV1980.DATA instead: the database as
C  it was when the site's NEUSPIEL was set up (src\edition1980.py).
C  ------------------------------------------------------------------
      SUBROUTINE IOINIT(DUMMY)
      IMPLICIT INTEGER*8(A-Z)
      INTEGER*4 TTYI,TTYO,DBFI
      LOGICAL BLKLIN
      COMMON /IOSCOM/ TTYI,TTYO,BLKLIN,DBFI
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      CHARACTER*12 DBNAME
      TTYI = 5
      TTYO = 6
      DBFI = 10
      DBNAME = 'ADV.DATA'
      IF (KFRESH .EQ. 2) DBNAME = 'ADV1980.DATA'
      OPEN (DBFI, FILE=TRIM(DBNAME), STATUS='OLD', ACTION='READ',
     +  ERR=90)
      RETURN
90    WRITE (0, '(3A)') 'abenteuer: ', TRIM(DBNAME),
     +  ' is missing (it belongs beside abenteuer.exe)'
      CALL EXIT(1)
      END

C  ------------------------------------------------------------------
C  LDCOMN and SVCOMN read and wrote the eleven stretches of COMMON the
C  main program lists (CMADRS, CMSZES) with the Harris IO routine:
C  L true, the file *NEUSPIEL - the game as the site set it up; L
C  false, the player's own saved game, named by the word after SICHR
C  or BRING - and if that could not be attached, *NEUSPIEL ("THE USER
C  WILL BE LEFT IN A FRESH GAME").  A wizard's SVCOMN(.TRUE.) wrote
C  NEUSPIEL, which the site then made *NEUSPIEL.  Here: NEUSPIEL.DAT
C  beside the program (made by src\neuspiel.py from the site's own
C  file on the tape; a wizard's maintenance rewrites it), and
C  saves\<name>.SAV.  --fresh is as if there were no NEUSPIEL: the
C  game sets itself up from ADV.DATA, as it did when there was none.
C  ------------------------------------------------------------------
      SUBROUTINE LDCOMN(L, FNAME, CMADDR, CMSIZE)
      IMPLICIT INTEGER*8(A-Z)
      LOGICAL L
      DIMENSION FNAME(8), CMADDR(4,11), CMSIZE(11)
      CHARACTER*80 NAME
      INTEGER*4 IERR
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      IF (.NOT. L) THEN
        CALL PFNAME(FNAME, NAME)
        IF (NAME .NE. ' ') THEN
          CALL PCLOAD(TRIM(NAME), CMADDR, CMSIZE, IERR)
          IF (IERR .EQ. 0) RETURN
        END IF
      END IF
      IF (KFRESH .NE. 0) RETURN
      CALL PCLOAD('NEUSPIEL.DAT', CMADDR, CMSIZE, IERR)
      RETURN
      END

      SUBROUTINE SVCOMN(L, FNAME, CMADDR, CMSIZE)
      IMPLICIT INTEGER*8(A-Z)
      LOGICAL L
      DIMENSION FNAME(8), CMADDR(4,11), CMSIZE(11)
      CHARACTER*80 NAME
      INTEGER*4 IERR
      IF (L) THEN
        NAME = 'NEUSPIEL.DAT'
      ELSE
        CALL PFNAME(FNAME, NAME)
        IF (NAME .EQ. ' ') GOTO 900
      END IF
      CALL PCSAVE(TRIM(NAME), CMADDR, CMSIZE, IERR)
      IF (IERR .NE. 0) GOTO 900
      RETURN
900   WRITE (6, 903)
903   FORMAT('TUT MIR LEID, ABER DEINEN FILE KANN ICH WEDER GENERIEREN'
     1,' NOCH FINDEN.')
      RETURN
      END

C  the file a player's game is saved in: saves\ and the eight characters
C  of the name (codes, one to a word), up to the first blank
      SUBROUTINE PFNAME(FNAME, NAME)
      INTEGER*8 FNAME(8)
      CHARACTER*(*) NAME
      INTEGER*4 I, K
      NAME = ' '
      K = 0
      DO 10 I = 1, 8
      IF (FNAME(I) .EQ. 32 .OR. FNAME(I) .LE. 0) GOTO 20
      IF (FNAME(I) .GT. 126) GOTO 20
      K = K + 1
10    CONTINUE
20    IF (K .EQ. 0) RETURN
      NAME = 'saves/'
      DO 30 I = 1, K
30    NAME(6+I:6+I) = CHAR(FNAME(I))
      NAME(7+K:) = '.SAV'
      RETURN
      END

C  ------------------------------------------------------------------
C  Lines of text.  PGETLN reads one; from the terminal it is masked to
C  seven bits, NULs dropped, and the end of piped input ends the
C  program.  PUNPK puts the characters' codes one to a word, which is
C  how the program holds them (A1) - see src\convert.py.
C  ------------------------------------------------------------------
      SUBROUTINE PGETLN(UNIT, BUF)
      INTEGER*4 UNIT
      CHARACTER*(*) BUF
      CHARACTER*256 LINE
      INTEGER*4 I, K, C
      READ (UNIT, '(A)', END=90) LINE
      BUF = ' '
      K = 0
      DO 10 I = 1, LEN(LINE)
      C = IAND(ICHAR(LINE(I:I)), 127)
      IF (C .EQ. 0) GOTO 10
      K = K + 1
      IF (K .LE. LEN(BUF)) BUF(K:K) = CHAR(C)
10    CONTINUE
      RETURN
90    IF (UNIT .NE. 5) THEN
        WRITE (0, '(A)') 'abenteuer: ADV.DATA ends too soon'
        CALL EXIT(1)
      END IF
      FLUSH (6)
      CALL EXIT(0)
      END

      SUBROUTINE PUNPK(BUF, ARR, N)
      CHARACTER*(*) BUF
      INTEGER*8 ARR(*)
      INTEGER*4 N, I
      DO 10 I = 1, N
      IF (I .LE. LEN(BUF)) THEN
        ARR(I) = ICHAR(BUF(I:I))
      ELSE
        ARR(I) = 32
      END IF
10    CONTINUE
      RETURN
      END

C  ------------------------------------------------------------------
C  Options (GNU style).  KUNLIM() is -u: no prime time, and a restored
C  game need not wait LATNCY minutes.
C  ------------------------------------------------------------------
      SUBROUTINE POPTS
      CHARACTER*64 ARG
      INTEGER*4 I, N, IOS, H, M
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      INTEGER*4 TTYI, TTYO, DBFI
      LOGICAL BLKLIN
      COMMON /IOSCOM/ TTYI, TTYO, BLKLIN, DBFI
C  Until IOINIT sets them, TTYI and TTYO are 0: logical unit 0, which
C  on the Harris was the terminal (INITIALIZING... is written then).
C  Here the terminal is units 5 and 6 (gfortran's 0 is stderr).
      TTYI = 5
      TTYO = 6
      KTIME = -1
      N = COMMAND_ARGUMENT_COUNT()
      I = 1
10    IF (I .GT. N) GOTO 90
      CALL GET_COMMAND_ARGUMENT(I, ARG)
      IF (ARG .EQ. '-u' .OR. ARG .EQ. '--unlimited') THEN
        KUNL = 1
      ELSE IF (ARG .EQ. '--fresh' .OR. ARG .EQ. '--fresh=tape') THEN
        KFRESH = 1
      ELSE IF (ARG .EQ. '--fresh=1980') THEN
        KFRESH = 2
      ELSE IF (ARG .EQ. '--no-fixes') THEN
        KNOFX = 1
      ELSE IF (ARG .EQ. '--seed' .AND. I .LT. N) THEN
        I = I + 1
        CALL GET_COMMAND_ARGUMENT(I, ARG)
        READ (ARG, *, IOSTAT=IOS) KSEED
        IF (IOS .NE. 0) GOTO 80
        KSEEDF = 1
      ELSE IF (ARG .EQ. '--date' .AND. I .LT. N) THEN
        I = I + 1
        CALL GET_COMMAND_ARGUMENT(I, ARG)
        READ (ARG, '(I4,1X,I2,1X,I2)', IOSTAT=IOS) KDATE
        IF (IOS .NE. 0 .OR. KDATE(2) .LT. 1 .OR. KDATE(2) .GT. 12)
     +    GOTO 80
      ELSE IF (ARG .EQ. '--time' .AND. I .LT. N) THEN
        I = I + 1
        CALL GET_COMMAND_ARGUMENT(I, ARG)
        READ (ARG, '(I2,1X,I2)', IOSTAT=IOS) H, M
        IF (IOS .NE. 0 .OR. H .GT. 23 .OR. M .GT. 59) GOTO 80
        KTIME = H * 60 + M
      ELSE IF (ARG .EQ. '-h' .OR. ARG .EQ. '--help') THEN
        CALL PUSAGE
        CALL EXIT(0)
      ELSE
        GOTO 80
      END IF
      I = I + 1
      GOTO 10
80    WRITE (0, '(3A)') 'abenteuer: unknown option ', TRIM(ARG),
     +  ' (abenteuer --help lists them)'
      CALL EXIT(2)
90    CALL PINIT
      RETURN
      END

      SUBROUTINE PUSAGE
      WRITE (6, 1)
1     FORMAT('usage: abenteuer [-u] [--seed N] [--fresh[=1980]]'
     +  /'        [--no-fixes] [--date YYYY-MM-DD] [--time HH:MM]'//
     2  'ABENTEUER, the German Colossal Cave of a Harris VULCAN'/
     3  'site (Palter''s portable Adventure, HCSD 1977), started'/
     4  'from the site''s own NEUSPIEL.'//
     5  '  -u, --unlimited  no prime time (weekdays 8-18 the cave'/
     6  '                   is closed), and no wait before a'/
     7  '                   saved game goes on'/
     8  '  --seed N         other dice'/
     9  '  --fresh          set up from ADV.DATA, as when there'/
     1  '                   was no NEUSPIEL: the texts as the'/
     2  '                   tape has them (a later edition)'/
     3  '  --fresh=1980     the same from ADV1980.DATA, the'/
     4  '                   edition of 1980 that NEUSPIEL was'/
     5  '                   set up from'/
     6  '  --no-fixes       the site''s game as it was: BRING'/
     7  '                   (restore) and MAGIE MODUS not taken'/
     8  '  --date, --time   hold the clock still'/
     9  '  -h, --help       this')
      RETURN
      END

      INTEGER*8 FUNCTION KUNLIM()
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      KUNLIM = KUNL
      RETURN
      END

C  KFIXES() is 0 with --no-fixes (src\convert.py marks each fix)
      INTEGER*8 FUNCTION KFIXES()
      INTEGER*4 KUNL, KSEEDF, KSEED, KFRESH, KDATE(3), KTIME, KNOFX
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KFRESH, KDATE, KTIME, KNOFX
      KFIXES = 1 - KNOFX
      RETURN
      END
