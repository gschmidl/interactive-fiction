C ======================================================================
C  PQORK - what Qork V3.0A had from NOS, FTN5's library and its own
C  COMPASS, written for gfortran.  Every integer is INTEGER*8 holding
C  the Cyber's 60-bit word, and characters are display code, so the
C  program packs and tests them exactly as it did (src\convert.py).
C
C    PRLIN            a line from the terminal (unit 5) or DATBAS (1)
C    PRDA1 PRDAN      terminal input as A1 words / one A10 word, by
C    PRDAW            NOS's 6/12 ASCII rules (lower case is ^ and the
C                     letter); PRDAW is n fields of An (GUARD)
C    PRLD1 PRLD2      GUARD's list-directed reads
C    PRDR1 PRDEX      DATBAS card images as R1 words (PRDEX: A2 first)
C    PWRR1 PWRA1      output of R1 / A1 words, 6/12 codes shown as the
C    PWRVER           terminal showed them; the version line
C    POCT             an Ow output field as FTN5 wrote it
C    PSHIFT PCOMPL    SHIFT (circular within 60 bits) and COMPL
C    PRANF PTIME PEOF FTN5's RANF, TIME and EOF
C    PSSWT PGOTOE     sense switch 1 (off = --build) and GOTOER
C    PCNCAT           the COMPASS CONCAT: a mask of the low 36 bits
C    POPTS PSAVUN     options, files, and the player's save file
C  The random database file (RIO) is in pqorkc.c.
C ======================================================================

      BLOCK DATA PQRKBD
      IMPLICIT INTEGER (A-Z)
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      DATA PBUILD/0/,PSEED/0/,PEOF5/0/,PEOF1/0/,PEOFS/0/,PFIX/1/
      END

C  the 64 character set: display code 00 is ':', 55 octal is blank
      CHARACTER*64 FUNCTION PDCTAB()
      PDCTAB = ':ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/()$= ,.#[]%"_!&'
     +//'''?<>@'//CHAR(92)//'^;'
      RETURN
      END

C ----------------------------------------------------------------------
C  A line of input.  From the terminal it is masked to seven bits and
C  NULs are dropped.  At the end of the input the read takes the
C  program's END= branch - Qork then says "I cannot hear you!" and
C  reads again, as it did on the Cyber.  BACKSPACE lets that read
C  happen (gfortran refuses to read after an end of file): at a console
C  after a Ctrl-Z the player types on; at the end of a pipe or file it
C  meets the end again, and a second end in a row ends the program.
C ----------------------------------------------------------------------
      SUBROUTINE PRLIN(U, LINE, IEOF)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) LINE
      CHARACTER*512 RAW
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      IEOF = 0
      IF (U .EQ. 1) THEN
        READ (1, '(A)', END=10) RAW
        PEOF1 = 0
      ELSE
        FLUSH (6)
        READ (5, '(A)', IOSTAT=IOS) RAW
        IF (IOS .NE. 0) GOTO 20
        PEOF5 = 0
        PEOFS = 0
      END IF
      LINE = ' '
      K = 0
      DO 5 I = 1, LEN_TRIM(RAW)
      C = IAND(ICHAR(RAW(I:I)), 127)
      IF (C .EQ. 0 .OR. C .EQ. 13) GOTO 5
      K = K + 1
      IF (K .LE. LEN(LINE)) LINE(K:K) = CHAR(C)
    5 CONTINUE
      RETURN
   10 PEOF1 = 1
      IEOF = 1
      RETURN
   20 IF (PEOFS .NE. 0) THEN
        FLUSH (6)
        CALL EXIT(0)
      END IF
      PEOFS = 1
      PEOF5 = 1
      IEOF = 1
      BACKSPACE (5, IOSTAT=IOS)
      RETURN
      END

      INTEGER FUNCTION PEOF(U)
      IMPLICIT INTEGER (A-Z)
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      PEOF = PEOF5
      IF (U .EQ. 1) PEOF = PEOF1
      RETURN
      END

C ----------------------------------------------------------------------
C  Characters to display code.  From the terminal by NOS's 6/12 ASCII
C  rules: a to z are 76B and the letter, @ ^ : and ` are 74B and 01, 02,
C  04, 07, and { | } ~ are 76B and 33-36.  The cards of DATBAS are plain
C  display code (the card reader has no lower case).
C ----------------------------------------------------------------------
      SUBROUTINE PA2D(LINE, SIX12, CODES, NC)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) LINE
      INTEGER CODES(*)
      CHARACTER*64 PDCTAB, T
      T = PDCTAB()
      NC = 0
      DO 10 I = 1, LEN_TRIM(LINE)
      C = ICHAR(LINE(I:I))
      IF (C .GE. 97 .AND. C .LE. 122) THEN
        IF (SIX12 .NE. 0) THEN
          NC = NC + 1
          CODES(NC) = 62
        END IF
        NC = NC + 1
        CODES(NC) = C - 96
        GOTO 10
      END IF
      IF (SIX12 .NE. 0) THEN
        E = 0
        IF (LINE(I:I) .EQ. '@') E = 60*64 + 1
        IF (LINE(I:I) .EQ. '^') E = 60*64 + 2
        IF (LINE(I:I) .EQ. ':') E = 60*64 + 4
        IF (C .EQ. 96) E = 60*64 + 7
        IF (C .GE. 123 .AND. C .LE. 126) E = 62*64 + C - 123 + 27
        IF (E .NE. 0) THEN
          CODES(NC+1) = E / 64
          CODES(NC+2) = MOD(E, 64)
          NC = NC + 2
          GOTO 10
        END IF
      END IF
      K = INDEX(T, LINE(I:I))
      IF (K .EQ. 0) K = 46
      NC = NC + 1
      CODES(NC) = K - 1
   10 CONTINUE
      RETURN
      END

C  N display codes (from I0) packed as one word, left justified and
C  blank filled - an An field
      INTEGER FUNCTION PPACK(CODES, NC, I0, N)
      IMPLICIT INTEGER (A-Z)
      INTEGER CODES(*)
      V = 0
      DO 10 I = 1, 10
      C = 45
      IF (I .LE. N .AND. I0 + I - 1 .LE. NC) C = CODES(I0 + I - 1)
      V = V * 64 + C
   10 CONTINUE
      PPACK = V
      RETURN
      END

      SUBROUTINE PRDA1(U, A, N, IEOF)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(1100), PPACK
      CHARACTER*512 LINE
      CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      CALL PA2D(LINE, 1, CODES, NC)
      DO 10 I = 1, N
   10 A(I) = PPACK(CODES, NC, I, 1)
      RETURN
      END

      SUBROUTINE PRDAN(U, W, N, IEOF)
      IMPLICIT INTEGER (A-Z)
      INTEGER CODES(1100), PPACK
      CHARACTER*512 LINE
      CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      CALL PA2D(LINE, 1, CODES, NC)
      W = PPACK(CODES, NC, 1, N)
      RETURN
      END

      SUBROUTINE PRDAW(U, A, NF, NW, IEOF)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(1100), PPACK
      CHARACTER*512 LINE
      CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      CALL PA2D(LINE, 1, CODES, NC)
      DO 10 I = 1, NF
   10 A(I) = PPACK(CODES, NC, (I - 1) * NW + 1, NW)
      RETURN
      END

      SUBROUTINE PRDR1(U, A, N, IEOF)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(1100)
      CHARACTER*512 LINE
      CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      CALL PA2D(LINE, 0, CODES, NC)
      DO 10 I = 1, N
      A(I) = 45
      IF (I .LE. NC) A(I) = CODES(I)
   10 CONTINUE
      RETURN
      END

      SUBROUTINE PRDEX(U, DIR, A, IEOF)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(1100), PPACK
      CHARACTER*512 LINE
      CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      CALL PA2D(LINE, 0, CODES, NC)
      DIR = PPACK(CODES, NC, 1, 2)
      DO 10 I = 1, 170
      A(I) = 45
      IF (I + 2 .LE. NC) A(I) = CODES(I + 2)
   10 CONTINUE
      RETURN
      END

C ----------------------------------------------------------------------
C  GUARD's list-directed READ (*,*) J,K and READ (*,*) J.  As FTN5 read
C  them from the terminal, values still missing at the end of a line
C  are looked for on the next one (an empty line counts for nothing),
C  and a slash ends the list, leaving the rest as they were.  A line
C  that is not numbers ends the read as the end of input would - FTN5
C  stopped the program with a fatal error.
C ----------------------------------------------------------------------
      SUBROUTINE PRLD2(U, J, K, IEOF)
      IMPLICIT INTEGER (A-Z)
      CALL PRLDN(U, 2, J, K, IEOF)
      RETURN
      END

      SUBROUTINE PRLD1(U, J, IEOF)
      IMPLICIT INTEGER (A-Z)
      K = 0
      CALL PRLDN(U, 1, J, K, IEOF)
      RETURN
      END

      SUBROUTINE PRLDN(U, N, J, K, IEOF)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*80 LINE
      CHARACTER*1000 ALL
      ALL = ' '
      L = 0
   10 CALL PRLIN(U, LINE, IEOF)
      IF (IEOF .NE. 0) RETURN
      M = LEN_TRIM(LINE)
      IF (L + M + 1 .GT. LEN(ALL)) GOTO 90
      IF (M .GT. 0) ALL(L+2:L+M+1) = LINE(1:M)
      L = L + M + 1
      JJ = J
      KK = K
      IF (N .EQ. 1) READ (ALL, *, END=10, ERR=90) JJ
      IF (N .EQ. 2) READ (ALL, *, END=10, ERR=90) JJ, KK
      J = JJ
      K = KK
      RETURN
   90 IEOF = 2
      RETURN
      END

C ----------------------------------------------------------------------
C  Display code to what an ASCII terminal showed: the 6/12 pairs (76B
C  and a letter is the letter in lower case, and so on); anything else
C  as the 64 character set has it.
C ----------------------------------------------------------------------
      SUBROUTINE PD2A(CODES, N, S, L)
      IMPLICIT INTEGER (A-Z)
      INTEGER CODES(*)
      CHARACTER*(*) S
      CHARACTER*64 PDCTAB, T
      T = PDCTAB()
      S = ' '
      L = 0
      I = 1
   10 IF (I .GT. N) RETURN
      C = CODES(I)
      IF (I .LT. N) THEN
        D = CODES(I + 1)
        IF (C .EQ. 62 .AND. D .GE. 1 .AND. D .LE. 26) THEN
          CALL PPUT(S, L, CHAR(96 + D))
          I = I + 2
          GOTO 10
        END IF
        IF (C .EQ. 62 .AND. D .GE. 27 .AND. D .LE. 30) THEN
          CALL PPUT(S, L, CHAR(123 + D - 27))
          I = I + 2
          GOTO 10
        END IF
        IF (C .EQ. 60 .AND. (D .EQ. 1 .OR. D .EQ. 2 .OR. D .EQ. 4
     +      .OR. D .EQ. 7)) THEN
          IF (D .EQ. 1) CALL PPUT(S, L, '@')
          IF (D .EQ. 2) CALL PPUT(S, L, '^')
          IF (D .EQ. 4) CALL PPUT(S, L, ':')
          IF (D .EQ. 7) CALL PPUT(S, L, CHAR(96))
          I = I + 2
          GOTO 10
        END IF
      END IF
      CALL PPUT(S, L, T(C+1:C+1))
      I = I + 1
      GOTO 10
      END

      SUBROUTINE PPUT(S, L, C)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S, C
      IF (L .LT. LEN(S)) THEN
        L = L + 1
        S(L:L) = C
      END IF
      RETURN
      END

C  WRITE(u,'(1X,170R1)') - the rightmost character of each word
      SUBROUTINE PWRR1(U, A, N)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(200)
      CHARACTER*200 S
      M = MIN(N, 200)
      DO 10 I = 1, M
   10 CODES(I) = IAND(A(I), 63)
      CALL PD2A(CODES, M, S, L)
      WRITE (U, '(1X,A)') S(1:L)
      RETURN
      END

C  WRITE(u,'(1X,78A1)') - the leftmost character of each word
      SUBROUTINE PWRA1(U, A, N)
      IMPLICIT INTEGER (A-Z)
      INTEGER A(*), CODES(200)
      CHARACTER*200 S
      M = MIN(N, 200)
      DO 10 I = 1, M
   10 CODES(I) = IAND(ISHFT(A(I), -54), 63)
      CALL PD2A(CODES, M, S, L)
      WRITE (U, '(1X,A)') S(1:L)
      RETURN
      END

C  WRITE(u,'('' V'',I1,''.'',I2,A1)') VMAJ,VMIN,VEDIT
      SUBROUTINE PWRVER(U, VMAJ, VMIN, VEDIT)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*64 PDCTAB, T
      T = PDCTAB()
      C = IAND(ISHFT(VEDIT, -54), 63)
      WRITE (U, '('' V'',I1,''.'',I2,A1)') VMAJ, VMIN, T(C+1:C+1)
      RETURN
      END

C  FTN5's Ow output field: the rightmost w of the word's 20 octal digits,
C  leading zeros and all (O6 of 62000B is 062000; a negative number is
C  its ones' complement), left justified for GUARD's Aw fields
      CHARACTER*20 FUNCTION POCT(V, W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*20 D
      M60 = 1152921504606846975
      P = V
      IF (P .LT. 0) P = M60 + P
      P = IAND(P, M60)
      DO 10 I = 20, 1, -1
      D(I:I) = CHAR(ICHAR('0') + IAND(P, 7))
      P = ISHFT(P, -3)
   10 CONTINUE
      POCT = D(21 - MIN(MAX(W, 1), 20):20)
      RETURN
      END

C ----------------------------------------------------------------------
C  Words.  SHIFT(X,N): N > 0 a circular left shift of the 60-bit word,
C  N < 0 a right shift that copies bit 59.  COMPL: the 60-bit
C  complement, which on the ones' complement Cyber is also minus X.
C ----------------------------------------------------------------------
      INTEGER FUNCTION PSHIFT(X, N)
      IMPLICIT INTEGER (A-Z)
      M60 = 1152921504606846975
      V = IAND(X, M60)
      IF (N .GE. 0) THEN
        S = MOD(N, 60)
        PSHIFT = IAND(IOR(ISHFT(V, S), ISHFT(V, S - 60)), M60)
      ELSE
        K = MIN(-N, 60)
        R = ISHFT(V, -K)
        IF (BTEST(V, 59)) R = IOR(R, IAND(M60, NOT(ISHFT(M60, -K))))
        PSHIFT = R
      END IF
      RETURN
      END

      INTEGER FUNCTION PCOMPL(X)
      IMPLICIT INTEGER (A-Z)
      IF (X .LT. 0) THEN
        PCOMPL = -X
      ELSE
        PCOMPL = IEOR(X, 1152921504606846975)
      END IF
      RETURN
      END

C  CONCAT (COMPASS: MX6 24, BX6 -X6) - whatever its arguments, a mask
C  of the low 36 bits
      INTEGER FUNCTION PCNCAT(A, B, C, D, E)
      IMPLICIT INTEGER (A-Z)
      PCNCAT = 68719476735
      RETURN
      END

C ----------------------------------------------------------------------
C  RANF, measured on NOS 2.8.7 (FTN 5.1): seed = seed * 44485709377909
C  mod 2**48, RANF = seed / 2**48, and a program starts with the seed
C  48131768981101 (octal 1274321477413155) - the same in every run, so
C  every game rolls the same dice.  --seed N starts elsewhere.
C ----------------------------------------------------------------------
      REAL FUNCTION PRANF()
      IMPLICIT INTEGER (A-Z)
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      IF (PSEED .EQ. 0) PSEED = 48131768981101
      PSEED = IAND(PSEED * 44485709377909, 281474976710655)
      PRANF = DBLE(PSEED) / 281474976710656.0D0
      RETURN
      END

C  TIME(1): " hh.mm.ss." in display code (Qork throws it away)
      INTEGER FUNCTION PTIME(X)
      IMPLICIT INTEGER (A-Z)
      INTEGER V(8), CODES(10), PPACK
      CHARACTER*10 S
      CALL DATE_AND_TIME(VALUES=V)
      WRITE (S, '(1X,I2.2,''.'',I2.2,''.'',I2.2,''.'')') V(5),V(6),V(7)
      CALL PA2D(S, 0, CODES, NC)
      PTIME = PPACK(CODES, NC, 1, 10)
      RETURN
      END

C  sense switch 1: on to play, off (--build) to set the database up
      SUBROUTINE PSSWT(N, I)
      IMPLICIT INTEGER (A-Z)
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      I = 1
      IF (PBUILD .NE. 0) I = 2
      RETURN
      END

C  CALL MOVETO(2): FTN5 let a LOGICAL FUNCTION be called; the result
C  is dropped
      SUBROUTINE PMOVTO(N)
      IMPLICIT INTEGER (A-Z)
      LOGICAL MOVETO, F
      F = MOVETO(N)
      RETURN
      END

C  GOTOER: the index of a computed GO TO was out of range
      SUBROUTINE PGOTOE
      WRITE (0, '(A)') 'qork: computed GO TO index out of range (GOTOER)'
      CALL EXIT(1)
      END

C ----------------------------------------------------------------------
C  Options and files.  qork.txt is DATBAS (TAPE1), read only by
C  --build, which writes the random text file qork.dat (RIO, TAPE2)
C  and the initial state qork.ini (TAPE3).  A game restores qork.ini;
C  after that unit 3 is the player's saves\QORK.SAV (PSAVUN).
C ----------------------------------------------------------------------
      SUBROUTINE POPTS
      IMPLICIT INTEGER (A-Z)
      CHARACTER*64 ARG
      COMMON /PQRKCM/ PBUILD,PSEED,PEOF5,PEOF1,PEOFS,PFIX
      N = COMMAND_ARGUMENT_COUNT()
      I = 1
   10 IF (I .GT. N) GOTO 50
      CALL GET_COMMAND_ARGUMENT(I, ARG)
      IF (ARG .EQ. '--build') THEN
        PBUILD = 1
      ELSE IF (ARG .EQ. '--no-fixes') THEN
        PFIX = 0
      ELSE IF (ARG .EQ. '--seed') THEN
        IF (I .GE. N) GOTO 45
        I = I + 1
        CALL GET_COMMAND_ARGUMENT(I, ARG)
        IF (VERIFY(TRIM(ARG), '0123456789') .NE. 0) GOTO 45
        READ (ARG, *, IOSTAT=IOS) PSEED
        IF (IOS .NE. 0 .OR. PSEED .LE. 0 .OR.
     +      PSEED .GT. 281474976710655) GOTO 45
      ELSE IF (ARG .EQ. '-h' .OR. ARG .EQ. '--help') THEN
        CALL PUSAGE
        CALL EXIT(0)
      ELSE
        GOTO 40
      END IF
      I = I + 1
      GOTO 10
   40 WRITE (0, '(3A)') 'qork: unknown option ', TRIM(ARG),
     +  ' (qork --help lists them)'
      CALL EXIT(2)
   45 WRITE (0, '(A)') 'qork: --seed takes a number from 1 to '//
     +  '281474976710655'
      CALL EXIT(2)
   50 CALL PINIT
      IF (PBUILD .NE. 0) THEN
        OPEN (1, FILE='qork.txt', STATUS='OLD', ACTION='READ', ERR=91)
        OPEN (3, FILE='qork.ini', FORM='UNFORMATTED', STATUS='REPLACE',
     +        ERR=92)
      ELSE
        OPEN (3, FILE='qork.ini', FORM='UNFORMATTED', STATUS='OLD',
     +        ACTION='READ', ERR=93)
      END IF
      RETURN
   91 WRITE (0, '(A)') 'qork: qork.txt is missing'
      CALL EXIT(1)
   92 WRITE (0, '(A)') 'qork: cannot write qork.ini'
      CALL EXIT(1)
   93 WRITE (0, '(A)') 'qork: qork.ini is missing (build.sh makes it,'//
     +  ' with qork --build)'
      CALL EXIT(1)
      END

      SUBROUTINE PUSAGE
      WRITE (6, 1)
    1 FORMAT('usage: qork [--seed N] [--build] [--no-fixes] [-h]'//
     +  'Qork V3.0A (S. O. Lidie, CDC NOS, 1984), the DECUS Dungeon as'/
     +  'it ran on a Control Data Cyber.'//
     +  '  --seed N    other dice (every game on the Cyber rolled the'/
     +  '              same ones)'/
     +  '  --build     set the database up from qork.txt, as the site'/
     +  '              did with sense switch 1 off'/
     +  '  --no-fixes  accepted, as by the collection''s other ports;'/
     +  '              this one changes nothing of the game'/
     +  '  -h, --help  this')
      RETURN
      END

C  after the initial state: unit 3 is saves\QORK.SAV, which starts as a
C  copy of that state, as a session's local TAPE3 did
      SUBROUTINE PSAVUN
      IMPLICIT INTEGER (A-Z)
      LOGICAL THERE
      CLOSE (3)
      INQUIRE (FILE='saves/QORK.SAV', EXIST=THERE)
      IF (.NOT. THERE) CALL PCOPYF('qork.ini', 'saves/QORK.SAV')
      OPEN (3, FILE='saves/QORK.SAV', FORM='UNFORMATTED', STATUS='OLD',
     +      ERR=90)
      RETURN
   90 WRITE (0, '(A)') 'qork: cannot open saves/QORK.SAV'
      CALL EXIT(1)
      END
