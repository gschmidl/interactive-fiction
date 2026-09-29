C     ==================================================================
C     pmmm.f - the FORTRAN half of the Mystery Mansion (HP 1000) port's
C     run-time; the C half is pmmmc.c.
C     ==================================================================
C
      PROGRAM PMMM
C     options, the terminal, the run parameters; then the dispatcher
C     runs the main program MMM and every segment it loads
      CALL PIOINI
      CALL POPTS
      CALL PSEGRUN
      END
C
C     ------------------------------------------------------------------
C     Options.  RTE ran the game as RU,MMM,lu,code,lu2,system: the
C     terminal, a security code, a second terminal that watches, and MM
C     for Wolpert's own set-up (cartridge MM: the player log, comments,
C     messages, playing hours and security codes).
C     ------------------------------------------------------------------
      SUBROUTINE POPTS
      CHARACTER*80 A, B
      CHARACTER*260 D
      INTEGER N, I, K, V(4), NV, ICODE, ISYS
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      ICODE = 0
      ISYS = 0
      CALL PGMDIR(D, K)
      CALL PSAVEDIR(D(1:K) // '\saves')
      N = COMMAND_ARGUMENT_COUNT()
      I = 0
   10 I = I + 1
      IF (I .GT. N) GOTO 50
      CALL GET_COMMAND_ARGUMENT(I, A)
      IF (A .EQ. '--site') THEN
         ISYS = 19789
      ELSE IF (A .EQ. '-u' .OR. A .EQ. '--unlimited') THEN
         ICODE = 15815
      ELSE IF (A .EQ. '--no-fixes') THEN
         PNOFIX = 1
      ELSE IF (A .EQ. '-h' .OR. A .EQ. '--help') THEN
         CALL PUSAGE
         STOP
      ELSE IF (A .EQ. '--time' .OR. A .EQ. '--date' .OR.
     +         A .EQ. '--code') THEN
         IF (I .EQ. N) THEN
            WRITE (0, '(A,A,A)') 'mmm: ', TRIM(A), ' needs a value'
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
            IF (NV .NE. 1 .OR. V(1) .GT. 32767) GOTO 90
            ICODE = V(1)
         END IF
      ELSE
         WRITE (0, '(A,A,A)') 'mmm: unknown option ', TRIM(A),
     +      ' (try --help)'
         STOP 2
      END IF
      GOTO 10
   50 IF (ISYS .NE. 0) CALL PMOUNTMM
      CALL PSETPAR(1, ICODE, 0, ISYS, 0)
      RETURN
   90 WRITE (0, '(A,A,A,A)') 'mmm: bad value for ', TRIM(A), ': ',
     +   TRIM(B)
      STOP 2
      END
C
      SUBROUTINE PUSAGE
      WRITE (0, '(A)')
     + 'usage: mmm [--site] [-u] [--code N] [--time HH:MM[:SS[.mmm]]]',
     + '           [--date YYYY-MM-DD] [--no-fixes] [-h]',
     + ' ',
     + 'Mystery Mansion (Bill Wolpert, HP 1000 RTE, 23 July 81).',
     + ' ',
     + '  --site         as on Wolpert''s machine (RU,MMM,1,,,MM):',
     + '                 player log, comments, messages, hours',
     + '  -u             pass a security code, as one could to',
     + '                 play outside those hours (--code 15815)',
     + '  --code N       pass security code N',
     + '  --time T       hold the clock at T (the mystery and the',
     + '                 dice come from the clock: a game repeats)',
     + '  --date D       run as on date D',
     + '  --no-fixes     leave the program''s bugs as they were'
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
C     STOP, or the END of a segment: flush unit 6 through the filter
      SUBROUTINE PSTOP
      FLUSH (6)
      CALL PFINI
      CALL EXIT(0)
      END
C
C     ------------------------------------------------------------------
C     The unit of a WRITE.  LU 1 is the player's terminal and LU 0 RTE's
C     bit bucket.  Any other LU - the line printer (6), a terminal or a
C     cassette the game is recorded on - is a file named after it in
C     saves, LU6.txt, which the game adds to.  (An LU RTE could not have
C     had goes to the bit bucket; RTE aborted the program.)
C     ------------------------------------------------------------------
      INTEGER FUNCTION PU(LU)
      INTEGER*2 LU
      LOGICAL OPENED(0:255)
      CHARACTER*300 P
      INTEGER K
      DATA OPENED/256*.FALSE./
      IF (LU .EQ. 1) THEN
         PU = 6
      ELSE IF (LU .LE. 0 .OR. LU .GT. 255) THEN
         IF (.NOT. OPENED(0)) THEN
            OPEN (98, FILE='NUL', STATUS='UNKNOWN')
            OPENED(0) = .TRUE.
         END IF
         PU = 98
      ELSE
         PU = 100 + LU
         IF (.NOT. OPENED(LU)) THEN
            CALL PLUPATH(INT(LU), P, K)
            OPEN (PU, FILE=P(1:K), STATUS='UNKNOWN', POSITION='APPEND',
     +            ACTION='WRITE')
            OPENED(LU) = .TRUE.
         END IF
      END IF
      END
C
C     1 unless --no-fixes: the port's fixes test it
      INTEGER*2 FUNCTION KFIXES()
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      KFIXES = 1
      IF (PNOFIX .NE. 0) KFIXES = 0
      END
C
C     ------------------------------------------------------------------
C     Words.  On the HP 1000 a word's first character is its high byte,
C     and a few statements take words apart with /256 and *256; here the
C     first character is first in memory - so those statements call these.
C     ------------------------------------------------------------------
      INTEGER*2 FUNCTION CFIRST(IW)
      INTEGER*2 IW
      CFIRST = IAND(INT(IW), 255)
      END
C
      INTEGER*2 FUNCTION CSECND(IW)
      INTEGER*2 IW
      CSECND = IAND(ISHFT(INT(IW), -8), 255)
      END
C
C     the character C followed by a blank
      INTEGER*2 FUNCTION CONE(IC)
      INTEGER*2 IC
      CONE = INT(IAND(INT(IC), 255) + 32 * 256, 2)
      END
C
C     the first characters of two words, in one word
      INTEGER*2 FUNCTION CJOIN(IA, IB)
      INTEGER*2 IA, IB
      INTEGER K
      K = IAND(INT(IA), 255) + 256 * IAND(INT(IB), 255)
      IF (K .GT. 32767) K = K - 65536
      CJOIN = INT(K, 2)
      END
C
C     ------------------------------------------------------------------
C     The READs, each one terminal line.  A2 editing is a plain copy of
C     the characters into words (gfortran's A into an INTEGER would stop
C     at a comma); a short line reads as blanks.
C     ------------------------------------------------------------------
      SUBROUTINE PRDLIN(LINE, N)
      CHARACTER*(*) LINE
      INTEGER N, PREAD
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      FLUSH (6)
      IF (PEOF .NE. 0) CALL PSTOP
      N = PREAD(LINE)
      IF (N .LT. 0) THEN
C        the end of piped input: the game would wait for ever
         PEOF = 1
         CALL PSTOP
      END IF
      END
C
      BLOCK DATA PMMMBD
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      DATA PEOF/0/, PNOFIX/0/, PBRK/0/
      END
C
      SUBROUTINE PRDA(IW, NW)
      INTEGER*2 IW(*)
      INTEGER NW, N, I, K
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      DO 10 I = 1, NW
         K = ICHAR(LINE(2*I-1:2*I-1)) + 256 * ICHAR(LINE(2*I:2*I))
         IF (K .GT. 32767) K = K - 65536
         IW(I) = INT(K, 2)
   10 CONTINUE
      END
C
C     (2A2,I2)
      SUBROUTINE PRDAAI(IA, IB, ID)
      INTEGER*2 IA, IB, ID
      INTEGER N, K
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      IA = INT(ICHAR(LINE(1:1)) + 256 * ICHAR(LINE(2:2)), 2)
      IB = INT(ICHAR(LINE(3:3)) + 256 * ICHAR(LINE(4:4)), 2)
      READ (LINE(5:6), '(BN,I2)', IOSTAT=K) N
      IF (K .EQ. 0) ID = INT(N, 2)
      END
C
C     (10A2) into IANS, KEYM(9)
      SUBROUTINE PRDAK(IA, KEYM)
      INTEGER*2 IA, KEYM(9), W(10)
      INTEGER I
      CALL PRDA(W, 10)
      IA = W(1)
      DO 10 I = 1, 9
         KEYM(I) = W(I+1)
   10 CONTINUE
      END
C
C     A line from LU LA - a cassette the game was recorded on (RECORD ON
C     TAPE), here its file.  At its end the port does what the player
C     did: it strikes the break key, which stops the tape, and hands the
C     game '**', a line it passes over.  N is -1 then.
      SUBROUTINE PRDLU(LA, LINE, N)
      INTEGER*2 LA
      CHARACTER*(*) LINE
      INTEGER N, PLREAD
      LOGICAL OP
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      INQUIRE (UNIT=100+LA, OPENED=OP)
      IF (OP) FLUSH (100+LA)
      N = PLREAD(INT(LA), LINE)
      IF (N .LT. 0) THEN
         LINE = '**'
         PBRK = 1
      END IF
      END
C
C     (36A2) from LU LA: the terminal, or a tape
      SUBROUTINE PRDAL(LA, IW, NW)
      INTEGER*2 LA, IW(*)
      INTEGER NW, N, I, K
      CHARACTER*160 LINE
      IF (LA .EQ. 1 .OR. LA .LE. 0 .OR. LA .GT. 255) THEN
         CALL PRDA(IW, NW)
         RETURN
      END IF
      CALL PRDLU(LA, LINE, N)
      DO 10 I = 1, NW
         K = ICHAR(LINE(2*I-1:2*I-1)) + 256 * ICHAR(LINE(2*I:2*I))
         IF (K .GT. 32767) K = K - 65536
         IW(I) = INT(K, 2)
   10 CONTINUE
      END
C
C     (I6) from LU LA
      SUBROUTINE PRDI6L(LA, IC)
      INTEGER*2 LA, IC
      INTEGER N, K
      CHARACTER*160 LINE
      IF (LA .EQ. 1 .OR. LA .LE. 0 .OR. LA .GT. 255) THEN
         CALL PRDI6(IC)
         RETURN
      END IF
      CALL PRDLU(LA, LINE, N)
      IF (N .LT. 0) RETURN
      READ (LINE(1:6), '(BN,I6)', IOSTAT=K) N
      IF (K .EQ. 0) IC = INT(N, 2)
      END
C
C     (I6)
      SUBROUTINE PRDI6(IC)
      INTEGER*2 IC
      INTEGER N, K
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      READ (LINE(1:6), '(BN,I6)', IOSTAT=K) N
      IF (K .EQ. 0) IC = INT(N, 2)
      END
C
C     READ (LU,*) - free-field numbers; what the line does not give
C     keeps its value
      SUBROUTINE PRDI1(I1)
      INTEGER*2 I1
      INTEGER V(2), NV
      CALL PRDNUM(V, NV)
      IF (NV .GE. 1) I1 = INT(V(1), 2)
      END
C
      SUBROUTINE PRDI2(I1, I2)
      INTEGER*2 I1, I2
      INTEGER V(2), NV
      CALL PRDNUM(V, NV)
      IF (NV .GE. 1) I1 = INT(V(1), 2)
      IF (NV .GE. 2) I2 = INT(V(2), 2)
      END
C
      SUBROUTINE PRDNUM(V, NV)
      INTEGER V(2), NV, N, K, I
      CHARACTER*160 LINE
      CHARACTER*164 BUF
      CALL PRDLIN(LINE, N)
      NV = 0
      V(1) = -999999
      V(2) = -999999
      BUF = LINE(1:MAX(N,1)) // ' /'
      READ (BUF, *, IOSTAT=K) V(1), V(2)
      IF (K .NE. 0) RETURN
      DO 10 I = 1, 2
         IF (V(I) .NE. -999999) NV = I
   10 CONTINUE
      END
C
C     ------------------------------------------------------------------
C     REIO(2,LU,BUF,N): write N words as a record (the prompt '>_')
C     ------------------------------------------------------------------
      SUBROUTINE REIO(ICODE, LU, IBUF, NW)
      INTEGER*2 ICODE, LU, IBUF(*), NW
      CHARACTER*200 S
      INTEGER I, K
      K = 0
      DO 10 I = 1, NW
         S(K+1:K+1) = CHAR(IAND(INT(IBUF(I)), 255))
         S(K+2:K+2) = CHAR(IAND(ISHFT(INT(IBUF(I)), -8), 255))
         K = K + 2
   10 CONTINUE
      S(K+1:K+2) = CHAR(13) // CHAR(10)
      K = K + 2
      FLUSH (6)
      IF (LU .NE. 0) CALL PWRITE(S, K)
      END
C
C     IFBRK: -1 if the break key was struck since the last call.  The
C     console has none; the port strikes it at the end of a tape.
      INTEGER*2 FUNCTION IFBRK(IDMY)
      INTEGER*2 IDMY
      INTEGER PEOF, PNOFIX, PBRK
      COMMON /PMMMCM/ PEOF, PNOFIX, PBRK
      IFBRK = 0
      IF (PBRK .NE. 0) IFBRK = -1
      PBRK = 0
      END
