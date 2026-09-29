C  prime.f - the PRIMOS side of the A-code executive (EXECUTIVE.F77),
C  for gfortran.  primec.c has what Fortran cannot do (files beside the
C  program, the clock, the login name, deleting a file).
C
C  All terminal output goes through unit 6, as the executive's own
C  WRITEs do, so the two cannot overtake each other.
C
C  ------------------------------------------------------------------
C  OPENDB: the four ADVINIT files, as PRIMOS wrote them - halfwords,
C  high byte first, characters with the top bit set.
C    ADVINIT1  the A-code                       -> BUFFER
C    ADVINIT2  the text, every halfword negated -> TEXTBUFFER
C    ADVINIT3  pairs of INTEGER*4: key, position -> KEYSBF, NUMKEYS
C    ADVINIT4  SYMCNT, SYMCNT words of 12 characters, their keys
C                                           -> Z$WORD, FILEKEY
C  ------------------------------------------------------------------
      SUBROUTINE OPENDB
      INCLUDE 'executive.ins'
      INTEGER*1 B(200000)
      INTEGER*4 N, I4, K, J
      INTEGER*2 PWORD
      INTEGER*4 PLONG
      CALL PLOAD('ADVINIT1.DAT', B, N, 200000)
      IF (N/2 .GT. BUFFSIZE) STOP 'ADVINIT1.DAT is too big'
      DO 10 I4 = 1, N/2
10    BUFFER(I4) = PWORD(B, 2*I4-1)
      CALL PLOAD('ADVINIT2.DAT', B, N, 200000)
      IF (N/2 .GT. TEXTSIZE) STOP 'ADVINIT2.DAT is too big'
      DO 20 I4 = 1, N/2
20    TEXTBUFFER(I4) = -PWORD(B, 2*I4-1)
      CALL PLOAD('ADVINIT3.DAT', B, N, 200000)
      NUMKEYS = N/8
      IF (NUMKEYS .GT. KEYSIZ) STOP 'ADVINIT3.DAT is too big'
      DO 30 K = 1, NUMKEYS
      KEYSBF(1,K) = PLONG(B, 8*K-7)
30    KEYSBF(2,K) = PLONG(B, 8*K-3)
      CALL PLOAD('ADVINIT4.DAT', B, N, 200000)
      SYMCNT = PWORD(B, 1)
      IF (SYMCNT .GT. VOCAB .OR. N .LT. 2+14*SYMCNT)
     +  STOP 'ADVINIT4.DAT is not a vocabulary'
      DO 40 K = 1, SYMCNT
      DO 40 J = 1, 12
40    Z$WORD(K)(J:J) = CHAR(IAND(INT(B(2+12*(K-1)+J)), 127))
      DO 50 K = 1, SYMCNT
50    FILEKEY(K) = PWORD(B, 2+12*SYMCNT+2*K-1)
      RETURN
      END

C  a halfword, high byte first, from B(I) and B(I+1)
      INTEGER*2 FUNCTION PWORD(B, I)
      INTEGER*1 B(*)
      INTEGER*4 I, W
      W = IOR(ISHFT(IAND(INT(B(I)), 255), 8), IAND(INT(B(I+1)), 255))
      IF (W .GE. 32768) W = W - 65536
      PWORD = INT(W, 2)
      RETURN
      END

C  a long word, high byte first, from B(I) .. B(I+3)
      INTEGER*4 FUNCTION PLONG(B, I)
      INTEGER*1 B(*)
      INTEGER*4 I, K
      PLONG = 0
      DO 10 K = 0, 3
10    PLONG = IOR(ISHFT(PLONG, 8), IAND(INT(B(I+K)), 255))
      RETURN
      END

C  ------------------------------------------------------------------
C  MOVE (EXECUTIVE.PMA) copied N halfwords of a text record into a
C  CHARACTER*140; here each halfword gives its two characters, the top
C  bit taken off.
C  ------------------------------------------------------------------
      SUBROUTINE PMOVE(SRC, DST, N)
      INTEGER*2 SRC(*), N
      CHARACTER*(*) DST
      INTEGER*4 I, W
      DO 10 I = 1, MIN(INT(N), LEN(DST)/2)
      W = IAND(INT(SRC(I)), 65535)
      DST(2*I-1:2*I-1) = CHAR(IAND(ISHFT(W, -8), 127))
10    DST(2*I:2*I) = CHAR(IAND(W, 127))
      RETURN
      END

C  ------------------------------------------------------------------
C  The terminal.
C    IOA$(CTL, N)  writes the control string, up to N characters; no
C                  new line of its own
C    TNOU(BUF, N)  N characters and a new line (TNOU(0,0): a new line)
C    TONL          a new line
C  The buffers are CHARACTER*1 arrays: TNOU(0,0) passes a number.
C
C  IOA$'s % sequences, measured on PRIMOS 23.4 (tests\prime\ioaprobe*
C  and the sessions cmds4-6): %$ ends the string, %% is %, %/ a new
C  line, %X a blank, %Y nothing, any other character (or a % at the
C  end) prints ??.  The texts have one: Ralph's "GET OUT!$!!!%!"
C  (4386015) comes out as "GET OUT!$!!!??".
C
C  A word the player typed reaches IOA$ too, through SAY's #.  There
C  F77 hands IOA$ a third argument, the text's descriptor - halfwords
C  '5000 and 140 - which the first directive that wants an argument
C  prints: %D 2560, %O %W 5000, %H A00, %F 320, %E 3E 02, %L T, %P
C  5000(0)/214 (D O H right-justified to a width, %6D); %A and %C end
C  the line; %R and %Z print ?? and swallow the next character; a
C  second such directive ends the line.  %V printed 2560 characters of
C  the executive's own memory (tests\prime\ioa_percent_v.txt); here it
C  ends the line.
C  ------------------------------------------------------------------
      SUBROUTINE PIOA(CTL, N)
      CHARACTER*1 CTL(*)
      INTEGER*2 N
      CHARACTER*300 OUT
      CHARACTER*16 V
      CHARACTER*1 D
      INTEGER*4 I, K, W, L
      LOGICAL USED
      K = 0
      I = 1
      USED = .FALSE.
10    IF (I .GT. N .OR. K .GT. 280) GOTO 90
      IF (CTL(I) .NE. '%') THEN
        K = K + 1
        OUT(K:K) = CTL(I)
        I = I + 1
        GOTO 10
      END IF
C     % [width] letter
      I = I + 1
      W = 0
20    IF (I .GT. N) GOTO 80
      IF (CTL(I) .GE. '0' .AND. CTL(I) .LE. '9') THEN
        W = 10*W + ICHAR(CTL(I)) - ICHAR('0')
        I = I + 1
        GOTO 20
      END IF
      D = CTL(I)
      I = I + 1
      IF (D .GE. 'a' .AND. D .LE. 'z') D = CHAR(ICHAR(D) - 32)
      IF (D .EQ. '$') GOTO 90
      IF (D .EQ. '%') THEN
        K = K + 1
        OUT(K:K) = '%'
      ELSE IF (D .EQ. 'X') THEN
        DO 30 L = 1, MAX(W, 1)
        K = K + 1
30      OUT(K:K) = ' '
      ELSE IF (D .EQ. '/') THEN
        IF (K .GT. 0) WRITE (6, '(A)', ADVANCE='NO') OUT(1:K)
        CALL TONL
        K = 0
      ELSE IF (D .EQ. 'Y') THEN
        CONTINUE
      ELSE IF (D .EQ. 'R' .OR. D .EQ. 'Z') THEN
        OUT(K+1:K+2) = '??'
        K = K + 2
        I = I + 1
      ELSE IF (INDEX('ACDEFHLOPVW', D) .GT. 0) THEN
        IF (USED .OR. INDEX('ACV', D) .GT. 0) GOTO 90
        USED = .TRUE.
        IF (D .EQ. 'D') V = '2560'
        IF (D .EQ. 'O' .OR. D .EQ. 'W') V = '5000'
        IF (D .EQ. 'H') V = 'A00'
        IF (D .EQ. 'F') V = '320'
        IF (D .EQ. 'E') V = '3E 02'
        IF (D .EQ. 'L') V = 'T'
        IF (D .EQ. 'P') V = '5000(0)/214'
        L = LEN_TRIM(V)
        IF (INDEX('DOH', D) .GT. 0 .AND. W .GT. L) THEN
          OUT(K+1:K+W-L) = ' '
          K = K + W - L
        END IF
        OUT(K+1:K+L) = V(1:L)
        K = K + L
      ELSE
        OUT(K+1:K+2) = '??'
        K = K + 2
      END IF
      GOTO 10
80    OUT(K+1:K+2) = '??'
      K = K + 2
90    IF (K .GT. 0) WRITE (6, '(A)', ADVANCE='NO') OUT(1:K)
      FLUSH (6)
      RETURN
      END

      SUBROUTINE TNOU(BUF, N)
      CHARACTER*1 BUF(*)
      INTEGER*2 N
      INTEGER*4 I
      IF (N .GT. 0)
     +  WRITE (6, '(*(A1))', ADVANCE='NO') (BUF(I), I=1,N)
      CALL TONL
      RETURN
      END

      SUBROUTINE TONL
      WRITE (6, '(A)')
      FLUSH (6)
      RETURN
      END

C  NLEN$A: the length of the string without its trailing blanks
      INTEGER*2 FUNCTION PNLEN(STR, N)
      CHARACTER*1 STR(*)
      INTEGER*2 N
      PNLEN = N
10    IF (PNLEN .EQ. 0) RETURN
      IF (STR(PNLEN) .NE. ' ') RETURN
      PNLEN = PNLEN - 1
      GOTO 10
      END

C  ------------------------------------------------------------------
C  Prime's shift intrinsics, on halfwords:
C    RS(I,N) logical right shift, LS(I,N) left shift,
C    RT(I,N) right truncate - the low N bits
C  ------------------------------------------------------------------
      INTEGER*2 FUNCTION RS(I, N)
      INTEGER*2 I, N
      RS = ISHFT(I, -N)
      RETURN
      END

      INTEGER*2 FUNCTION LS(I, N)
      INTEGER*2 I, N
      LS = ISHFT(I, N)
      RETURN
      END

      INTEGER*2 FUNCTION RT(I, N)
      INTEGER*2 I, N
      RT = IAND(I, INT(ISHFT(1, N) - 1, 2))
      RETURN
      END

C  ------------------------------------------------------------------
C  PREAD(VAR, W): a line from the terminal into VAR as READ with format
C  (Aw) did - the first W characters, the rest of VAR blank.  The line
C  is masked to 7 bits (the Prime's terminal line) and NULs dropped.
C  At the end of piped input the program ends: the Prime's terminal
C  never had one.  --echo writes the line back, as the terminal did.
C  ------------------------------------------------------------------
      SUBROUTINE PREAD(VAR, W)
      CHARACTER*(*) VAR
      INTEGER*2 W
      CHARACTER*256 LINE, CLEAN
      INTEGER*4 I, K, C
      INTEGER*4 KUNL, KSEEDF, KSEED, KECHO
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KECHO
      READ (5, '(A)', END=90) LINE
      CLEAN = ' '
      K = 0
      DO 10 I = 1, LEN(LINE)
      C = IAND(ICHAR(LINE(I:I)), 127)
      IF (C .EQ. 0) GOTO 10
      K = K + 1
      CLEAN(K:K) = CHAR(C)
10    CONTINUE
      IF (KECHO .NE. 0) THEN
        WRITE (6, '(A)') TRIM(CLEAN)
        FLUSH (6)
      END IF
      VAR = CLEAN(1:MIN(INT(W), LEN(CLEAN)))
      RETURN
90    FLUSH (6)
      CALL EXIT(0)
      END

C  ------------------------------------------------------------------
C  Options (GNU style).  KUNLIM() is -u: EXEC 9 then says every
C  restored game was saved long ago (999 minutes), so a quick restore
C  costs no points.  --seed N starts the dice from N; --echo writes
C  every line read back, as the Prime's terminal did (for tests).
C  ------------------------------------------------------------------
      SUBROUTINE POPTS
      CHARACTER*64 ARG
      INTEGER*4 I, N, IOS
      INTEGER*4 KUNL, KSEEDF, KSEED, KECHO
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KECHO
      N = COMMAND_ARGUMENT_COUNT()
      I = 1
10    IF (I .GT. N) GOTO 90
      CALL GET_COMMAND_ARGUMENT(I, ARG)
      IF (ARG .EQ. '-u' .OR. ARG .EQ. '--unlimited') THEN
        KUNL = 1
      ELSE IF (ARG .EQ. '--echo') THEN
        KECHO = 1
      ELSE IF (ARG .EQ. '--seed' .AND. I .LT. N) THEN
        I = I + 1
        CALL GET_COMMAND_ARGUMENT(I, ARG)
        READ (ARG, *, IOSTAT=IOS) KSEED
        IF (IOS .NE. 0) GOTO 80
        KSEEDF = 1
      ELSE IF (ARG .EQ. '-h' .OR. ARG .EQ. '--help') THEN
        CALL PUSAGE
        CALL EXIT(0)
      ELSE
        GOTO 80
      END IF
      I = I + 1
      GOTO 10
80    WRITE (0, '(3A)') 'adventure4: unknown option ', TRIM(ARG),
     +  ' (adventure4 --help lists them)'
      CALL EXIT(2)
90    CALL PINIT
      RETURN
      END

      SUBROUTINE PUSAGE
      WRITE (6, '(A)') 'usage: adventure4 [-u] [--seed N]'
      WRITE (6, '(A)') ''
      WRITE (6, '(A)') 'ADVENTURE4, Mike Arnautov''s 660-point'
      WRITE (6, '(A)') 'Adventure (Glaxo version 4.3, 26 Jul 1984),'
      WRITE (6, '(A)') 'run by his A-code executive (rev.19.2) as'
      WRITE (6, '(A)') 'on the Prime.'
      WRITE (6, '(A)') ''
      WRITE (6, '(A)') '  -u, --unlimited  a restored game counts as'
      WRITE (6, '(A)') '                   saved long ago: no points'
      WRITE (6, '(A)') '                   lost for a quick restore'
      WRITE (6, '(A)') '  --seed N         other dice: the Prime rolled'
      WRITE (6, '(A)') '                   the same ones in every game'
      WRITE (6, '(A)') '  --echo           write each line read back'
      WRITE (6, '(A)') '  -h, --help       this'
      RETURN
      END

      INTEGER*2 FUNCTION KUNLIM()
      INTEGER*4 KUNL, KSEEDF, KSEED, KECHO
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KECHO
      KUNLIM = INT(KUNL, 2)
      RETURN
      END

C  ------------------------------------------------------------------
C  RND(X): PRIMOS's RND$, reached through a dynamic link.  Measured on
C  PRIMOS 23.4: X other than 0 is a seed - all 32 bits of it, the
C  word after an INTEGER*2 argument too - and RND returns X itself;
C  RND(0) is the next number, a 23-bit fraction (an unnormalised float,
C  exponent 128).  The seed, and so every game, is the same each time:
C  the executive calls RND(I) with I left at 501 by a loop.  The
C  generator itself is not yet known (tests\prime has the measurements
C  and a dump of its code), so this is a stand-in with the same
C  interface: seeded from X (or --seed), 23-bit fractions.
C  ------------------------------------------------------------------
      REAL*4 FUNCTION RND(ISEED)
      INTEGER*2 ISEED
      INTEGER*4 R
      INTEGER*4 KUNL, KSEEDF, KSEED, KECHO
      COMMON /PCOMM/ KUNL, KSEEDF, KSEED, KECHO
      SAVE R
      DATA R /1/
      IF (ISEED .NE. 0) THEN
        R = ISEED
        IF (KSEEDF .NE. 0) R = KSEED
        R = MOD(ABS(R), 2147483646) + 1
        RND = REAL(ISEED)
        RETURN
      END IF
      R = INT(MOD(16807_8 * R, 2147483647_8), 4)
      RND = REAL(ISHFT(R, -8)) / 8388608.0
      RETURN
      END
