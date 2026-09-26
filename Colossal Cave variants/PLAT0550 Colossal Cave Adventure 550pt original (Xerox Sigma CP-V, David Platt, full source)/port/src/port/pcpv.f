C     ==================================================================
C     pcpv.f - the FORTRAN half of the CP-V Adventure port's run-time.
C
C     Stands in for the Sigma assembler routines the two programs were
C     linked with - CACHESI, SVARSI, DECSI/ENCSI, PRIMESI, GETACCTSI,
C     BREAKSI, FASTREADSI, MASHSI, OUTSWPSI - each written from its AP
C     source on the LADC tape, and for what the ANS FORTRAN run-time
C     did on CP-V: keyed files, the terminal, STOP.  The C half is
C     pcpvc.c.
C     ==================================================================
C
      BLOCK DATA PCPVBD
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      CHARACTER*260 PBASE, PSAVED
      COMMON /PCPVCH/ PBASE, PSAVED
      INTEGER CKEY, CLOC, NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      INTEGER*2 CDATA
      COMMON /PCACHE/ CKEY(1024), CLOC(1024), CDATA(16384),
     +                NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      DATA PUNLIM/0/, PSSW/8*0/, PEOF/0/, PNOFIX/0/, PVERB/0/
      DATA PBASE/'.'/, PSAVED/'saves'/
      DATA NENT/0/, CLOCN/0/, CSIZE/0/, MAXPG/16/, PCT/0/, CSTRT/0/
      END
C
C     ------------------------------------------------------------------
C     Options of adv.exe
C     ------------------------------------------------------------------
      SUBROUTINE POPTS
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      CHARACTER*260 PBASE, PSAVED
      COMMON /PCPVCH/ PBASE, PSAVED
      CHARACTER*80 A, B
      INTEGER N, I, K, V(4), NV
      CALL PGMDIR(PBASE, K)
      PSAVED = PBASE(1:K) // '\saves'
      N = COMMAND_ARGUMENT_COUNT()
      I = 0
   10 I = I + 1
      IF (I .GT. N) RETURN
      CALL GET_COMMAND_ARGUMENT(I, A)
      IF (A .EQ. '-u' .OR. A .EQ. '--unlimited') THEN
         PUNLIM = 1
      ELSE IF (A .EQ. '--no-fixes') THEN
         PNOFIX = 1
      ELSE IF (A .EQ. '-h' .OR. A .EQ. '--help') THEN
         CALL PUSAGE
         STOP
      ELSE IF (A .EQ. '--time' .OR. A .EQ. '--date' .OR.
     +         A .EQ. '--sense-switch') THEN
         IF (I .EQ. N) THEN
            WRITE (*, '(A,A,A)') 'adv: ', TRIM(A), ' needs a value'
            STOP 2
         END IF
         I = I + 1
         CALL GET_COMMAND_ARGUMENT(I, B)
         CALL PNUMS(B, V, NV)
         IF (A .EQ. '--time') THEN
            IF (NV .LT. 2 .OR. V(1) .GT. 23 .OR. V(2) .GT. 59) GOTO 90
            IF (NV .LT. 3) V(3) = 0
            IF (NV .LT. 4) V(4) = 0
            CALL PCLKSET(2, V(1), V(2), V(3), V(4))
         ELSE IF (A .EQ. '--date') THEN
            IF (NV .NE. 3 .OR. V(1) .LT. 1901 .OR. V(1) .GT. 2099
     +          .OR. V(2) .LT. 1 .OR. V(2) .GT. 12 .OR. V(3) .LT. 1
     +          .OR. V(3) .GT. 31) GOTO 90
            CALL PCLKSET(1, V(1), V(2), V(3), 0)
         ELSE
            IF (NV .NE. 1 .OR. V(1) .LT. 1 .OR. V(1) .GT. 6) GOTO 90
            PSSW(V(1)) = 1
         END IF
      ELSE
         WRITE (*, '(A,A,A)') 'adv: unknown option ', TRIM(A),
     +      ' (try --help)'
         STOP 2
      END IF
      GOTO 10
   90 WRITE (*, '(A,A,A,A)') 'adv: bad value for ', TRIM(A), ': ',
     +   TRIM(B)
      STOP 2
      END
C
      SUBROUTINE PUSAGE
      WRITE (*, '(A)')
     + 'usage: adv [-u] [--time HH:MM[:SS[.mmm]]] [--date YYYY-MM-DD]',
     + '           [--sense-switch N] [--no-fixes] [-h]',
     + ' ',
     + 'CP-V Adventure (David Platt, Honeywell LADC, 1979).',
     + ' ',
     + '  -u, --unlimited    open outside the posted hours, no 600-move',
     + '                     limit, no 30-minute wait before RESTORE',
     + '  --time T           hold the clock at T (the dice are drawn',
     + '                     from the clock: this makes a game repeat)',
     + '  --date D           run as on date D',
     + '  --sense-switch N   set CP-V sense switch N (1-6)',
     + '  --no-fixes         accepted; the port changes nothing'
      END
C
C     Numbers separated by anything else: '09:30:00.250' -> 9,30,0,250
      SUBROUTINE PNUMS(S, V, NV)
      CHARACTER*(*) S
      INTEGER V(4), NV, I, C, INUM
      NV = 0
      INUM = 0
      DO 10 I = 1, LEN_TRIM(S)
         C = ICHAR(S(I:I)) - 48
         IF (C .GE. 0 .AND. C .LE. 9) THEN
            IF (INUM .EQ. 0) THEN
               IF (NV .EQ. 4) THEN
                  NV = 5
                  RETURN
               END IF
               NV = NV + 1
               V(NV) = 0
               INUM = 1
            END IF
            IF (V(NV) .LT. 100000) V(NV) = V(NV) * 10 + C
         ELSE
            INUM = 0
         END IF
   10 CONTINUE
      END
C
C     The munger's options: the directory holding its input
      SUBROUTINE PMOPTS
      CHARACTER*260 PBASE, PSAVED
      COMMON /PCPVCH/ PBASE, PSAVED
      INTEGER IERR
      IF (COMMAND_ARGUMENT_COUNT() .GE. 1)
     +   CALL GET_COMMAND_ARGUMENT(1, PBASE)
      CALL KOPEN(105, TRIM(PBASE) // '\COMPILE_CAVE.kyd', 1, IERR)
      IF (IERR .NE. 0) THEN
         WRITE (*, '(A)') 'munge: cannot open COMPILE_CAVE.kyd'
         STOP 2
      END IF
      END
C
C     ------------------------------------------------------------------
C     The terminal
C     ------------------------------------------------------------------
C
C     One output record, then CR LF.  CP-V printed a FORTRAN record on
C     the terminal as it stood, carriage-control column and all (ANS
C     FORTRAN B08 under CP-V C00: FORMAT (' HELLO') shows " HELLO").
C     Trailing blanks are not sent; the control bytes of the cave's text
C     are what CP-V's COC made of them: CR printed CR LF, NAK nothing,
C     'line feed only' a bare LF (the console is set not to return on
C     one).
      SUBROUTINE PTOUT(LINE)
      CHARACTER*(*) LINE
      CHARACTER*400 OBUF
      INTEGER N, I, K, IC
      N = LEN(LINE)
   10 IF (N .GT. 0) THEN
         IF (LINE(N:N) .EQ. ' ') THEN
            N = N - 1
            GOTO 10
         END IF
      END IF
      K = 0
      DO 20 I = 1, N
         IC = ICHAR(LINE(I:I))
         IF (IC .EQ. 13) THEN
            OBUF(K+1:K+2) = CHAR(13) // CHAR(10)
            K = K + 2
         ELSE IF (IC .NE. 21) THEN
            K = K + 1
            OBUF(K:K) = LINE(I:I)
         END IF
   20 CONTINUE
      OBUF(K+1:K+2) = CHAR(13) // CHAR(10)
      K = K + 2
      CALL PTWRITE(OBUF, K)
      END
C
C     Read a terminal line into S (the first N characters of it).  At
C     the end of piped input the first read takes the END= branch as
C     FORTRAN would; a read after that stops the program quietly (the
C     original's QUERY would ask again forever).
      SUBROUTINE PTIN(S, N, *)
      CHARACTER*(*) S
      INTEGER N, K, PTREAD0
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      CHARACTER*140 BUF
      IF (PEOF .NE. 0) CALL PQUIT
      IF (PTREAD0(BUF, K) .LT. 0) THEN
         PEOF = 1
         RETURN 1
      END IF
      S = BUF(1:N)
      END
C
C     STOP 'text': ANS FORTRAN's run-time printed ' *STOP* text' (seen
C     on CP-V C00).
      SUBROUTINE PSTOP(MSG)
      CHARACTER*(*) MSG
      CALL PTOUT(' *STOP* ' // MSG)
      CALL PTFLUSH
      CALL EXIT(0)
      END
C
C     The port's own quiet stop, at the end of piped input
      SUBROUTINE PQUIT
      CALL PTFLUSH
      CALL EXIT(0)
      END
C
C     ------------------------------------------------------------------
C     ANS FORTRAN's bit functions: Sigma SLS / SAS (a negative count
C     shifts right) and INOT
C     ------------------------------------------------------------------
      INTEGER FUNCTION ISL(I, N)
      INTEGER I, N
      IF (N .GE. 32 .OR. N .LE. -32) THEN
         ISL = 0
      ELSE
         ISL = ISHFT(I, N)
      END IF
      END
C
      INTEGER FUNCTION ISA(I, N)
      INTEGER I, N
      IF (N .GE. 0) THEN
         IF (N .GE. 32) THEN
            ISA = 0
         ELSE
            ISA = ISHFT(I, N)
         END IF
      ELSE
         ISA = SHIFTA(I, MIN(-N, 31))
      END IF
      END
C
      INTEGER FUNCTION INOT(I)
      INTEGER I
      INOT = NOT(I)
      END
C
C     ------------------------------------------------------------------
C     MASHSI: h = rotate-left-6(h) + byte, over the EBCDIC codes; LAW.
C     ------------------------------------------------------------------
      INTEGER FUNCTION MASH(S, N)
      CHARACTER*(*) S
      INTEGER N, H, I, PEBCDC
      H = 0
      DO 10 I = 1, N
         H = ISHFTC(H, 6, 32)
         H = H + PEBCDC(S(I:I))
   10 CONTINUE
      MASH = ABS(H)
      END
C
C     ------------------------------------------------------------------
C     GETACCTSI: the account the database is in, and the player's own.
C     They are the same here, so SAVE may create the save file.
C     ------------------------------------------------------------------
      SUBROUTINE GETACCT(ACCT, MYACCT)
      CHARACTER*(*) ACCT, MYACCT
      ACCT = 'ADVENTUR'
      MYACCT = 'ADVENTUR'
      END
C
C     DECSI/ENCSI set the DCBs' encryption key: the port's files are
C     not encrypted.  BREAKSI (break key) and OUTSWPSI (DCB table) have
C     nothing to do here either.
      SUBROUTINE CIPHER(K)
      INTEGER K
      END
C
      SUBROUTINE BREAKSET
      END
C
      SUBROUTINE OUTSWP
      END
C
C     ------------------------------------------------------------------
C     SVARSI - system variables.  M:TIME gives the time; ETMF, response
C     and users come from M:DISPLAY (a machine to oneself: 1, 1, 1).
C     Type 6 (seconds) never stores its result - a bug in SVARSI.
C     ------------------------------------------------------------------
      SUBROUTINE SVAR(ITYPE, IVAL)
      INTEGER ITYPE, IVAL, IYR, IDOY, IHH, IMI, ISS, IMS
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      IF (ITYPE .LT. 0 .OR. ITYPE .GE. 9) THEN
         IVAL = 0
         RETURN
      END IF
      CALL PCLOCK(IYR, IDOY, IHH, IMI, ISS, IMS)
      GOTO (100, 110, 120, 130, 140, 150, 160, 170, 180), ITYPE + 1
  100 IVAL = 1
      RETURN
  110 IVAL = 1
      RETURN
  120 IVAL = 1
      RETURN
  130 IVAL = IDOY
      RETURN
  140 IVAL = IHH
      RETURN
  150 IVAL = IMI
      RETURN
  160 RETURN
  170 IVAL = IMS
      RETURN
  180 IF (IVAL .GE. 1 .AND. IVAL .LE. 6) THEN
         IVAL = PSSW(IVAL)
      ELSE
         IVAL = 0
      END IF
      END
C
C     ------------------------------------------------------------------
C     PRIMESI.  Day of the week from M:TIME's year (two digits: years
C     since 1900) and day of year as (365y + y/4 + day) mod 7, 0 =
C     Sunday - which counts a leap year's own leap day from 1 January,
C     so in leap years every day is taken for the next one.  The time
C     as BCD hhmm is checked against each day's two closed spans with
C     CLM, both ends inclusive.  1 = prime time, 2 = ETMF above 6,
C     3 = more than 999 users, 0 = open.  (The check that the program
C     was started as ADV. - which logged cheaters off - always passes.)
C     ------------------------------------------------------------------
      INTEGER FUNCTION PRIME()
      INTEGER PUNLIM, PSSW, PEOF, PNOFIX, PVERB
      COMMON /PCPVCM/ PUNLIM, PSSW(8), PEOF, PNOFIX, PVERB
      INTEGER IYR, IDOY, IHH, IMI, ISS, IMS, IDOW, IBCD, J
      INTEGER SPANS(4, 0:6)
      DATA SPANS / 4*9999,
     +  2304, 4400, 4912, 5888,
     +  2304, 4400, 4912, 5888,
     +  2304, 4400, 4912, 5888,
     +  2304, 4400, 4912, 5888,
     +  2304, 4400, 4912, 5888,
     +  4*9999 /
      PRIME = 0
      IF (PUNLIM .NE. 0) RETURN
      CALL PCLOCK(IYR, IDOY, IHH, IMI, ISS, IMS)
      IDOW = MOD(IYR * 365 + IYR / 4 + IDOY, 7)
      J = IHH * 100 + IMI
      IBCD = (J / 1000) * 4096 + MOD(J / 100, 10) * 256
     +     + MOD(J / 10, 10) * 16 + MOD(J, 10)
      IF (IBCD .GE. SPANS(1, IDOW) .AND. IBCD .LE. SPANS(2, IDOW))
     +   PRIME = 1
      IF (IBCD .GE. SPANS(3, IDOW) .AND. IBCD .LE. SPANS(4, IDOW))
     +   PRIME = 1
C     (ETMF 1 of at most 6, 1 user of at most 999: nothing else stops it)
      END
C
      SUBROUTINE HOURS
      CALL PTOUT('   Monday through Friday:  Midnight to 9 AM')
      CALL PTOUT('                           11:30 AM to 1:30 PM')
      CALL PTOUT('                           5 PM to midnight')
      CALL PTOUT('   Saturday:               All day')
      CALL PTOUT('   Sunday:                 All day')
      CALL PTOUT(' ')
      CALL PTOUT('   The cave is closed at all other times, and also'
     +        // ' whenever the ETMF is')
      CALL PTOUT('   greater than 6.')
      CALL PTOUT(' ')
      END
C
C     ------------------------------------------------------------------
C     CACHESI: a sorted table of up to 1024 keys (4 pages of
C     doublewords) over pages of halfwords - one page to start, up to
C     16 in all - holding instruction records up to and including
C     their -999.  Full: the alternate return, and READBUFF stops
C     adding.  Nothing the player sees depends on it; it is kept
C     because the interpreter's paths through it are the original's.
C     ------------------------------------------------------------------
      SUBROUTINE CSTART(*)
      INTEGER CKEY, CLOC, NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      INTEGER*2 CDATA
      COMMON /PCACHE/ CKEY(1024), CLOC(1024), CDATA(16384),
     +                NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      PCT = 1024
      MAXPG = MAXPG - 1
      CSIZE = 1024
      CSTRT = 1
      END
C
      SUBROUTINE CADD(*, KEY, BUFFER)
      INTEGER KEY, BUFFER(*), I, N, J
      INTEGER CKEY, CLOC, NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      INTEGER*2 CDATA
      COMMON /PCACHE/ CKEY(1024), CLOC(1024), CDATA(16384),
     +                NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      N = 0
   10 N = N + 1
      IF (BUFFER(N) .NE. -999) GOTO 10
      CSIZE = CSIZE - N
      IF (CSIZE .LT. 0) THEN
         MAXPG = MAXPG - 1
         IF (MAXPG .LT. 0) RETURN 1
         CSIZE = CSIZE + 1024
      END IF
      PCT = PCT - 1
      IF (PCT .LT. 0) RETURN 1
C     insertion into the key table
      J = NENT
      NENT = NENT + 1
   20 IF (J .GT. 0) THEN
         IF (KEY .LE. CKEY(J)) THEN
            CKEY(J+1) = CKEY(J)
            CLOC(J+1) = CLOC(J)
            J = J - 1
            GOTO 20
         END IF
      END IF
      CKEY(J+1) = KEY
      CLOC(J+1) = CLOCN
      DO 30 I = 1, N
         CDATA(CLOCN + I) = INT(IAND(BUFFER(I), 65535) -
     +      65536 * (IAND(BUFFER(I), 32768) / 32768), 2)
   30 CONTINUE
      CLOCN = CLOCN + N
      END
C
      SUBROUTINE CGET(*, KEY, BUFFER)
      INTEGER KEY, BUFFER(*), LO, HI, MID, I, L
      INTEGER CKEY, CLOC, NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      INTEGER*2 CDATA
      COMMON /PCACHE/ CKEY(1024), CLOC(1024), CDATA(16384),
     +                NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      IF (NENT .EQ. 0) RETURN 1
      LO = 1
      HI = NENT
   10 IF (LO .GT. HI) RETURN 1
      MID = (LO + HI) / 2
      IF (CKEY(MID) .EQ. KEY) GOTO 20
      IF (CKEY(MID) .LT. KEY) THEN
         LO = MID + 1
      ELSE
         HI = MID - 1
      END IF
      GOTO 10
   20 L = CLOC(MID)
      I = 0
   30 I = I + 1
      BUFFER(I) = CDATA(L + I)
      IF (BUFFER(I) .NE. -999) GOTO 30
      END
C
      SUBROUTINE CCLEAR
      INTEGER CKEY, CLOC, NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      INTEGER*2 CDATA
      COMMON /PCACHE/ CKEY(1024), CLOC(1024), CDATA(16384),
     +                NENT, CLOCN, CSIZE, MAXPG, PCT, CSTRT
      IF (CSTRT .EQ. 0) RETURN
      NENT = 0
      CLOCN = 0
      CSIZE = 0
      PCT = 0
      MAXPG = 16
      CSTRT = 0
      END
C
C     ------------------------------------------------------------------
C     Keyed files by CP-V name
C     ------------------------------------------------------------------
      SUBROUTINE KOPENN(IUNIT, NAME, MODE, IERR)
      INTEGER IUNIT, MODE, IERR, K, PMKDIR
      CHARACTER*(*) NAME
      CHARACTER*260 PBASE, PSAVED
      COMMON /PCPVCH/ PBASE, PSAVED
      IF (NAME .EQ. '*ADVFREEZE') THEN
         IF (MODE .EQ. 2) K = PMKDIR(PSAVED)
         CALL KOPEN(IUNIT, TRIM(PSAVED) // '\advfreeze.dat', MODE, IERR)
      ELSE IF (NAME .EQ. 'ADVI') THEN
         CALL KOPEN(IUNIT, TRIM(PBASE) // '\advi.dat', MODE, IERR)
      ELSE IF (NAME .EQ. 'ADVT') THEN
         CALL KOPEN(IUNIT, TRIM(PBASE) // '\advt.dat', MODE, IERR)
      ELSE
         IERR = 768
      END IF
      END
C
C     READTEXT's READ (A140) and INQUIRE RECSIZE
      SUBROUTINE KRTEXT(IUNIT, KEY, TEXT, RSIZE, *)
      INTEGER IUNIT, KEY, RSIZE, IFND, KPLEN
      CHARACTER*(*) TEXT
      CALL KPRDI(IUNIT, KEY, IFND)
      IF (IFND .EQ. 0) RETURN 1
      RSIZE = KPLEN()
      CALL KPGETC(TEXT, 140)
      END
C
C     READBUFF's READ (1024R2) RSIZE, (BUFFER(I), I=1, MIN(RSIZE, n))
C     R2 fills a word from the right: the halfwords come back unsigned
      SUBROUTINE KRHALF(IUNIT, KEY, RSIZE, BUFFER, NMAX, *)
      INTEGER IUNIT, KEY, RSIZE, BUFFER(*), NMAX, IFND, I
      CALL KPRDI(IUNIT, KEY, IFND)
      IF (IFND .EQ. 0) RETURN 1
      CALL KPGET(RSIZE)
      DO 10 I = 1, MIN(RSIZE, NMAX)
         CALL KPGET(BUFFER(I))
   10 CONTINUE
      END
C
C     ------------------------------------------------------------------
C     FASTREADSI: the next line of M:SI (DCB 105) or of the INCLUDE file
C     (M:UI, DCB 104), blank-filled to 140.  A control character as the
C     last byte of the record becomes a blank.  LINEX is the first
C     nonblank column, LINEND one past the last; FILEKEY is the line's
C     key in a keyed file (the INCLUDE files), 0 for M:SI.
C     ------------------------------------------------------------------
      SUBROUTINE FASTREAD(IDCB, LINE, IFKEY, LINEX, LINEND, *)
      INTEGER IDCB, IFKEY, LINEX, LINEND, IU, NARS, IKEY, KNEXT
      INTEGER IFST, IL, I
      CHARACTER*(*) LINE
      IU = 105
      IF (IDCB .EQ. 104) IU = 104
      IF (KNEXT(IU, LINE, NARS, IKEY) .LT. 0) RETURN 1
      IF (NARS .GE. 1) THEN
         IF (ICHAR(LINE(NARS:NARS)) .LT. 32) LINE(NARS:NARS) = ' '
      END IF
      IFST = NARS - 1
      DO 10 I = 1, LEN(LINE)
         IF (LINE(I:I) .NE. ' ') THEN
            IFST = I - 1
            GOTO 20
         END IF
   10 CONTINUE
   20 IL = NARS - 1
   30 IF (IL .NE. IFST) THEN
         IF (LINE(IL+1:IL+1) .EQ. ' ') THEN
            IL = IL - 1
            GOTO 30
         END IF
      END IF
      LINEX = IFST + 1
      LINEND = IL + 2
      IFKEY = 0
      IF (IU .EQ. 104) IFKEY = IKEY
      END
C
C     FASTWRIT: the text line as a record of the text file (F:117)
      SUBROUTINE FASTWRITE(IOKEY, TEXT)
      INTEGER IOKEY
      CHARACTER*(*) TEXT
      CALL KPBEG
      CALL KPPUTC(TEXT, LEN(TEXT))
      CALL KPWRI(117, IOKEY)
      END
C
C     The munger's INCLUDE file: D:name is the tape member, kept by the
C     build as D_name.kyd beside COMPILE_CAVE.kyd
      SUBROUTINE FROPEN(IUNIT, FID, IERR)
      INTEGER IUNIT, IERR, I
      CHARACTER*(*) FID
      CHARACTER*40 F
      CHARACTER*260 PBASE, PSAVED
      COMMON /PCPVCH/ PBASE, PSAVED
      F = FID
      DO 10 I = 1, LEN_TRIM(F)
         IF (F(I:I) .EQ. ':') F(I:I) = '_'
   10 CONTINUE
      CALL KOPEN(IUNIT, TRIM(PBASE) // '\' // TRIM(F) // '.kyd', 1,
     +           IERR)
      END
C
      SUBROUTINE FRCLOS(IUNIT)
      INTEGER IUNIT
      CALL KCLOSE(IUNIT)
      END
C
      SUBROUTINE FRREW(IUNIT)
      INTEGER IUNIT
      CALL KREW(IUNIT)
      END
