C     ==================================================================
C     pdng.f - the FORTRAN half of the Dungeon (HP 1000 RTE) port's
C     run-time; the C half is pdngc.c.
C     ==================================================================
C
      PROGRAM PDNG
C     options and the terminal; then the game's main program, which
C     runs the segments through DLINK
      CALL PIOINI
      CALL POPTS
      CALL DUNGN
      CALL PSTOP
      END
C
C     ------------------------------------------------------------------
C     Options.  The data base (@DUNGN, and the @DUNGT and @DUNGI the
C     game makes from it) lives beside the program; saved games go to
C     saves beside it.
C     ------------------------------------------------------------------
      SUBROUTINE POPTS
      CHARACTER*80 A, B
      CHARACTER*260 D
      INTEGER N, I, K, V(4), NV
      CALL PGMDIR(D, K)
      CALL PDATADIR(D(1:K))
      CALL PSAVEDIR(D(1:K) // '\saves')
      N = COMMAND_ARGUMENT_COUNT()
      I = 0
   10 I = I + 1
      IF (I .GT. N) RETURN
      CALL GET_COMMAND_ARGUMENT(I, A)
      IF (A .EQ. '-u' .OR. A .EQ. '--unlimited' .OR.
     +    A .EQ. '--no-fixes') THEN
C        accepted, as by the other ports: this Dungeon has no hours,
C        no move limit and (yet) no fixes
         CONTINUE
      ELSE IF (A .EQ. '-h' .OR. A .EQ. '--help') THEN
         CALL PUSAGE
         STOP
      ELSE IF (A .EQ. '--time' .OR. A .EQ. '--date') THEN
         IF (I .EQ. N) THEN
            WRITE (0, '(A,A,A)') 'dungeon: ', TRIM(A), ' needs a value'
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
         ELSE
            IF (NV .NE. 3 .OR. V(1) .LT. 1901 .OR. V(1) .GT. 2099
     +          .OR. V(2) .LT. 1 .OR. V(2) .GT. 12 .OR. V(3) .LT. 1
     +          .OR. V(3) .GT. 31) GOTO 90
            CALL PCLKSET(1, V(1), V(2), V(3), 0)
         END IF
      ELSE
         WRITE (0, '(A,A,A)') 'dungeon: unknown option ', TRIM(A),
     +      ' (try --help)'
         STOP 2
      END IF
      GOTO 10
   90 WRITE (0, '(A,A,A,A)') 'dungeon: bad value for ', TRIM(A), ': ',
     +   TRIM(B)
      STOP 2
      END
C
      SUBROUTINE PUSAGE
      WRITE (0, '(A)')
     + 'usage: dungeon [--time HH:MM[:SS[.mmm]]] [--date YYYY-MM-DD]',
     + '               [-h]',
     + ' ',
     + 'Dungeon V3.0a (Tom Hutchinson, HP 1000 RTE, 18 Nov 82).',
     + ' ',
     + '  --time T       hold the clock at T (the dice are seeded',
     + '                 from it: a game repeats)',
     + '  --date D       run as on date D'
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
C     EXEC 6, or the end of the program: unit 6 through the filter, the
C     files still open onto the disc
      SUBROUTINE PSTOP
      FLUSH (6)
      CALL PFMPEND
      CALL PFINI
      CALL EXIT(0)
      END
C
C     The unit of a WRITE: the game writes only to its terminal, OUTCH
      INTEGER FUNCTION PU(LU)
      INTEGER*2 LU
      LOGICAL OPENED
      DATA OPENED/.FALSE./
      IF (LU .EQ. 0) THEN
         IF (.NOT. OPENED) THEN
            OPEN (98, FILE='NUL', STATUS='UNKNOWN')
            OPENED = .TRUE.
         END IF
         PU = 98
      ELSE
         PU = 6
      END IF
      END
C
C     ------------------------------------------------------------------
C     REIO(ICODE, CONTROL, BUFFER, LENGTH): 1 reads a line, 2 writes a
C     record; LENGTH in words, or characters if negative.  The LU and
C     its control bits (400B: echo) do not matter here.  The arguments
C     may be 16-bit variables or 32-bit literals: little-endian, the
C     first 16 bits are the value either way.
C     ------------------------------------------------------------------
      SUBROUTINE REIO(ICODE, ICNWD, IBUF, IL)
      INTEGER*2 ICODE, ICNWD, IBUF(*), IL
      INTEGER N
      N = IL
      IF (N .GT. 0) N = 2 * N
      IF (N .LT. 0) N = -N
      FLUSH (6)
      IF (ICODE .EQ. 2) THEN
         CALL PREIOW(IBUF, N)
      ELSE
         CALL PREIOR(IBUF, N)
      END IF
      END
C
C     ------------------------------------------------------------------
C     The READs of GDT and of SAVE/RESTORE, each one terminal line.  A2
C     editing is a plain copy of the characters into words (gfortran's A
C     into an INTEGER would stop at a comma); a short line reads as
C     blanks.
C     ------------------------------------------------------------------
      SUBROUTINE PRDLIN(LINE, N)
      CHARACTER*(*) LINE
      INTEGER N, PREAD
      FLUSH (6)
      N = PREAD(LINE)
C     the end of piped input: the game would wait for ever
      IF (N .LT. 0) CALL PSTOP
      END
C
C     (nA2)
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
C     (I6), (O6): what the line does not give keeps its value
      SUBROUTINE PRDI6(IV)
      INTEGER*2 IV
      INTEGER N, K
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      READ (LINE(1:6), '(BN,I6)', IOSTAT=K) N
      IF (K .EQ. 0) IV = INT(N, 2)
      END
C
      SUBROUTINE PRDO6(IV)
      INTEGER*2 IV
      INTEGER N, K
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      READ (LINE(1:6), '(BN,O6)', IOSTAT=K) N
      IF (K .EQ. 0) IV = INT(IAND(N, 65535), 2)
      END
C
C     (L1): T or F in column 1
      SUBROUTINE PRDL1(LV)
      LOGICAL*2 LV
      INTEGER N
      CHARACTER*160 LINE
      CALL PRDLIN(LINE, N)
      IF (LINE(1:1) .EQ. 'T') LV = .TRUE.
      IF (LINE(1:1) .EQ. 'F') LV = .FALSE.
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
C     IXOR: the HP library's exclusive or of two words
      INTEGER*2 FUNCTION IXOR(I, J)
      INTEGER*2 I, J
      IXOR = IEOR(I, J)
      END
