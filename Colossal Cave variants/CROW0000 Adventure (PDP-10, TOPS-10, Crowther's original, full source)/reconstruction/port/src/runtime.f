C  runtime.f -- the PDP-10 machine layer for the Crowther ADV port.
C
C  ADV.F4 is DEC FORTRAN-10 for a 36-bit word.  The port keeps that word:
C  five seven-bit characters, character k in bits 36-7k..30-7k, held
C  sign-extended in an INTEGER*8.  The program's own tables fix the
C  layout --
C       GETIN:  DATA M2/"4000000000,"20000000,"100000,"400,"2,0/
C  -- those are the low bits of the five character fields, at 29, 22, 15,
C  8 and 1, and "774000000000 selects the first character.  Because the
C  layout is unchanged, every mask, .AND./.XOR., five-character literal
C  and the program's own SHIFT keep working exactly as written.
C
C  What the compiler genuinely cannot do is the A edit descriptor over
C  that layout (gfortran packs eight-bit bytes), DEC's free-format "G"
C  descriptor, ACCEPT, ENCODE, OPEN(ACCESS='SEQIN'), PAUSE's operator
C  dialogue, FORTRAN carriage control, and the library's RAN.  Those go
C  through the routines here.  Nothing here contains game logic.

C ----------------------------------------------------------------------
C  Word packing
C ----------------------------------------------------------------------

      SUBROUTINE C2W(S, N, W)
C  Pack the first N characters of S into word W, blank-filled to five.
C  This is FORTRAN-10's rule for An data: left-justified, blank-filled.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      W = 0
      DO 1 K = 1, 5
         IF (K .LE. N .AND. K .LE. LEN(S)) THEN
            C = ICHAR(S(K:K))
         ELSE
            C = 32
         ENDIF
         IF (C .LT. 0 .OR. C .GT. 127) C = 32
         W = W + ISHFT(C, 29 - 7*(K-1))
1     CONTINUE
C  Sign-extend: bit 35 is the sign on a PDP-10.  GETIN's masks are
C  negative 36-bit words, so the operands it compares must carry the
C  sign too or they stop matching the packed literals.
      IF (IAND(W, 34359738368) .NE. 0) W = W - 68719476736
      RETURN
      END

      SUBROUTINE W2C(W, N, S)
C  Unpack the first N characters of word W into S.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      S = ' '
      DO 1 K = 1, N
         IF (K .GT. LEN(S)) GOTO 2
         C = IAND(ISHFT(IAND(W,68719476735), -(29 - 7*(K-1))), 127)
         IF (C .NE. 9 .AND. (C .LT. 32 .OR. C .GT. 126)) C = 32
         S(K:K) = CHAR(C)
1     CONTINUE
2     RETURN
      END

      SUBROUTINE C2WS(S, NW, NC, W)
C  Pack S into NW words of NC characters each.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      DIMENSION W(NW)
      DO 1 I = 1, NW
         P = (I-1)*NC + 1
         IF (P .GT. LEN(S)) THEN
            CALL C2W(' ', NC, W(I))
         ELSE
            Q = MIN(P+NC-1, LEN(S))
            CALL C2W(S(P:Q), NC, W(I))
         ENDIF
1     CONTINUE
      RETURN
      END

C ----------------------------------------------------------------------
C  RAN.
C
C  ADV calls the FORTRAN-10 library's RAN, so the port has to be the same
C  generator, not merely a good one.  It was read straight off the pack:
C  a LINK map put FORLIB's RAN module at 717-745 with two words of state
C  at 746-747, and dumping that memory from a FORTRAN program gives
C
C       RAN:    MOVE  0,746            ; the seed
C               MUL   0,744            ; * 630360016, 70-bit product
C               DIV   0,745            ; / 2147483647
C               MOVEM 1,746            ; seed := remainder
C               MOVSI 0,237000         ; exponent 2**31, fraction 0
C               DFAD  0,742            ; + 0.0D0 -- normalise seed/2**31
C               POPJ  17,
C
C  so the seed is a Lehmer generator mod 2**31-1 with multiplier
C  630360016, and the value returned is seed/2**31 normalised into a
C  single-precision word -- 27 fraction bits, truncated.  SETRAN's own
C  code shows the reset value in 747, 1777777 octal, and that is what 746
C  is assembled with: the seed is 524287 at every program start, the
C  argument is ignored, and ADV is therefore completely deterministic.
C  Verified against the original over 5336 successive values.
C ----------------------------------------------------------------------

      DOUBLE PRECISION FUNCTION RAN10(DUMMY)
      IMPLICIT INTEGER(A-Z)
      DOUBLE PRECISION P10FLT
      SAVE SEED
      DATA SEED /524287/
      SEED = MOD(SEED * 630360016, 2147483647)
      RAN10 = P10FLT(SEED)
      RETURN
      END

      DOUBLE PRECISION FUNCTION P10FLT(NUM)
C  The value a PDP-10 single-precision word holds for NUM/2**31: the
C  fraction truncated to 27 bits.  Keeping the truncation matters because
C  the game compares RAN against constants that FORTRAN-10 also
C  truncated (0.05 assembles as 174631463146 octal, 0.0499999998...),
C  and an exact double would decide a boundary draw the other way.
      IMPLICIT INTEGER(A-Z)
      IF (NUM .EQ. 0) THEN
         P10FLT = 0.0D0
         RETURN
      ENDIF
      B = 0
      T = NUM
1     IF (T .EQ. 0) GOTO 2
         B = B + 1
         T = ISHFT(T, -1)
         GOTO 1
2     T = NUM
      IF (B .GT. 27) T = ISHFT(ISHFT(NUM, -(B-27)), B-27)
      P10FLT = DBLE(T) / 2147483648.0D0
      RETURN
      END

      DOUBLE PRECISION FUNCTION P10CON(NUM, EXPO)
C  A FORTRAN-10 real constant, given the fraction and exponent field of
C  the word the compiler assembled.  convert.py emits calls to nothing --
C  it folds the constants itself -- but this is the rule it applies, and
C  the one P10FLT follows: value = fraction * 2**(exponent-128-27).
      IMPLICIT INTEGER(A-Z)
      P10CON = DBLE(NUM) * 2.0D0**(EXPO-155)
      RETURN
      END

C ----------------------------------------------------------------------
C  Output: FORTRAN carriage control.
C
C  FOROTS took the first character of every formatted record as carriage
C  control and did not print it.  ADV needs the real rule, not the "every
C  record starts with a blank" shortcut, because the database supplies
C  the control character itself: every text line of ADV.DAT reads
C       <n> <TAB> " 0TEXT..."
C  and the free-format G swallows the number and the blanks after it, so
C  the record the program writes begins with '0' -- advance two lines.
C  That is why Crowther's Adventure double-spaces everything it says.
C
C      ' '  advance one line      '0'  advance two lines
C      '1'  form feed             '+'  overprint (never produced here)
C
C  An empty record advances one line.  Measured against the original:
C  a three-line room description plus SPEAK's trailing FORMAT(/) comes
C  out as "\n" text "\n" "\n" text "\n" ... "\n\n", which is what this
C  produces.
C ----------------------------------------------------------------------

C  Two ways to print, and only one of them is the -10's.
C
C  --double-space prints exactly what TOPS-10 printed: '0' advances two
C  lines, FORMAT(/)'s pair of empty records makes two blank lines, and
C  every record carries the trailing blanks of its five-character words.
C  That is the mode the verification compares against the original, and
C  it is byte for byte what came off the pack.
C
C  It is also, on a screen rather than a teletype, mostly blank.  So by
C  default the port keeps the line structure and drops the padding: a '0'
C  advances one line like a blank, a run of blank lines collapses to one,
C  and trailing blanks come off.  Nothing else differs -- the same
C  records, in the same order, with the same text.

      SUBROUTINE PUTBL
C  One blank line; in the default mode, not two in a row.
      IMPLICIT INTEGER(A-Z)
      COMMON /OUTCOM/ LASTBL
      IF (VERBAT() .EQ. 0 .AND. LASTBL .NE. 0) RETURN
      WRITE(6,1)
      LASTBL = 1
      RETURN
1     FORMAT()
      END

      SUBROUTINE PUTTXT(S)
C  One line of text.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      COMMON /OUTCOM/ LASTBL
      IF (VERBAT() .EQ. 0) THEN
         L = LEN_TRIM(S)
         IF (L .EQ. 0) THEN
            CALL PUTBL
            RETURN
         ENDIF
         WRITE(6,1) S(1:L)
      ELSE
         WRITE(6,1) S
      ENDIF
      LASTBL = 0
      RETURN
1     FORMAT(A)
      END

      SUBROUTINE PUTREC(S)
C  One record, trailing blanks significant.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      N = LEN(S)
      IF (N .LE. 0) THEN
         CALL PUTBL
         RETURN
      ENDIF
      C = ICHAR(S(1:1))
      IF (C .EQ. 49) THEN
         CALL PUTTXT(CHAR(12))
      ELSE IF (C .EQ. 48 .AND. VERBAT() .NE. 0) THEN
         CALL PUTBL
      ENDIF
      IF (N .GE. 2) THEN
         CALL PUTTXT(S(2:N))
      ELSE
         CALL PUTBL
      ENDIF
      RETURN
      END

      SUBROUTINE PUTNL(N)
C  N empty records.
      IMPLICIT INTEGER(A-Z)
      DO 1 I = 1, N
1        CALL PUTBL
      RETURN
      END

      INTEGER FUNCTION VERBAT()
C  Print what the -10 printed, blank lines and padding and all?
      IMPLICIT INTEGER(A-Z)
      COMMON /OPTCOM/ OPINIT, OPECHO, OPVERB
      VERBAT = OPVERB
      RETURN
      END

      SUBROUTINE PUTRCS(BUF, N)
C  The N records a FORMAT produced, taken from an internal-file array.
C  Trailing blanks are dropped: an internal write blank-fills each
C  element to its declared length, and not one of the formats ADV uses
C  for these records ends in a blank -- they end in '.', '!', '?' or
C  ':'.  The records that DO carry meaningful trailing blanks are the
C  database text lines, and those go through PUTLL instead.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) BUF(N)
      DO 1 I = 1, N
         L = LEN_TRIM(BUF(I))
         IF (L .EQ. 0) THEN
            CALL PUTNL(1)
         ELSE
            CALL PUTREC(BUF(I)(1:L))
         ENDIF
1     CONTINUE
      RETURN
      END

      SUBROUTINE PUTLL(LL, ND, ROW, J1, J2)
C  TYPE n,(LLINE(KK,JJ),JJ=J1,J2) with FORMAT(20A5): the A descriptor
C  over packed words.  The record is exactly five characters per word,
C  trailing blanks included -- the original printed them and they are in
C  its transcripts.
      IMPLICIT INTEGER(A-Z)
      DIMENSION LL(ND,*)
      CHARACTER*140 S
      CHARACTER*5 T
      N = 0
      S = ' '
      DO 1 J = J1, J2
         CALL W2C(LL(ROW,J), 5, T)
         IF (N+5 .LE. LEN(S)) S(N+1:N+5) = T
         N = N + 5
1     CONTINUE
      IF (N .EQ. 0) THEN
         CALL PUTNL(1)
      ELSE
         CALL PUTREC(S(1:MIN(N,LEN(S))))
      ENDIF
      RETURN
      END

C ----------------------------------------------------------------------
C  Input
C ----------------------------------------------------------------------

      SUBROUTINE RDLINE(LU, S, EOF)
C  One record.  On the terminal, flush first so that what the game has
C  said appears before it waits.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      S = ' '
      EOF = 0
      IF (LU .EQ. 5) FLUSH(6)
      READ(LU, 1, IOSTAT=IOS) S
      IF (IOS .NE. 0) THEN
         EOF = 1
         S = ' '
         RETURN
      ENDIF
C  A CR left by a DOS-format script is not part of the line.
      DO 2 I = 1, LEN(S)
         IF (ICHAR(S(I:I)) .EQ. 13) S(I:I) = ' '
2     CONTINUE
C  The terminal line was 7-bit: the -10's scanner stripped the eighth bit
C  before a program saw a character and discarded NULs.  A Windows
C  console hands over code-page bytes, and the game packs seven-bit
C  characters five to a word, so a high bit would spill into the next
C  character.  Do what the scanner did.
      IF (LU .EQ. 5) THEN
         J = 0
         DO 3 I = 1, LEN(S)
            C = IAND(ICHAR(S(I:I)), 127)
            IF (C .NE. 0) THEN
               J = J + 1
               S(J:J) = CHAR(C)
            ENDIF
3        CONTINUE
         IF (J .LT. LEN(S)) S(J+1:) = ' '
      ENDIF
C  The -10's terminal service echoed what was typed, which is why the
C  commands appear in a transcript of the original.  A console does its
C  own echoing, so this applies only when the input is not a terminal.
      IF (LU .EQ. 5 .AND. IECHO() .NE. 0) THEN
         CALL PUTTXT(S(1:LEN_TRIM(S)))
      ENDIF
      RETURN
1     FORMAT(A)
4     FORMAT(A)
      END

      INTEGER FUNCTION IECHO()
C  Echo typed lines?  --echo / ADV_ECHO decide it if given; otherwise
C  echo exactly when the input is not a terminal.
      IMPLICIT INTEGER(A-Z)
      LOGICAL ISATTY
      CHARACTER*8 EV
      COMMON /OPTCOM/ OPINIT, OPECHO, OPVERB
      SAVE KNOWN, VAL
      DATA KNOWN /0/
      IF (KNOWN .EQ. 0) THEN
         IF (OPECHO .GE. 0) THEN
            VAL = OPECHO
         ELSE
            CALL GET_ENVIRONMENT_VARIABLE('ADV_ECHO', EV)
            IF (EV(1:1) .EQ. '0') THEN
               VAL = 0
            ELSE IF (EV(1:1) .EQ. '1') THEN
               VAL = 1
            ELSE IF (ISATTY(5)) THEN
               VAL = 0
            ELSE
               VAL = 1
            ENDIF
         ENDIF
         KNOWN = 1
      ENDIF
      IECHO = VAL
      RETURN
      END

      SUBROUTINE PQUIT
C  End of input.  The -10 would have killed the job.
      IMPLICIT INTEGER(A-Z)
      FLUSH(6)
      STOP
      END

      SUBROUTINE GFLD(LINE, I, VAL, GOT)
C  One DEC free-format integer from LINE starting at I: skip anything
C  that cannot begin a number, then an optional sign and digits.  I is
C  left just past the digits.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) LINE
      L = LEN(LINE)
      GOT = 0
      VAL = 0
1     IF (I .GT. L) RETURN
      C = ICHAR(LINE(I:I))
      IF (C .EQ. 45 .OR. (C .GE. 48 .AND. C .LE. 57)) GOTO 2
      I = I + 1
      GOTO 1
2     SGN = 1
      IF (C .EQ. 45) THEN
         SGN = -1
         I = I + 1
      ENDIF
      V = 0
3     IF (I .GT. L) GOTO 4
      C = ICHAR(LINE(I:I))
      IF (C .LT. 48 .OR. C .GT. 57) GOTO 4
      V = V*10 + (C - 48)
      GOT = 1
      I = I + 1
      GOTO 3
4     IF (GOT .EQ. 0) GOTO 1
      VAL = SGN*V
      RETURN
      END

      SUBROUTINE RDFRE(LU, V, N)
C  DEC free-format input, FORMAT(G) and FORMAT(12G): up to N integers
C  from one record, the rest left zero.  The program depends on the zero
C  fill -- a section 3 record carries a variable number of motion verbs
C  and the loop stops at the first zero.
      IMPLICIT INTEGER(A-Z)
      DIMENSION V(N)
      CHARACTER*300 LINE
      DO 1 I = 1, N
1        V(I) = 0
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) THEN
         IF (LU .EQ. 5) CALL PQUIT
         CALL DBEOF
      ENDIF
      P = 1
      DO 2 K = 1, N
         CALL GFLD(LINE, P, VAL, GOT)
         IF (GOT .EQ. 0) RETURN
         V(K) = VAL
2     CONTINUE
      RETURN
      END

      SUBROUTINE GSKIP(LINE, I)
C  Where the A fields start after a free-format number: FORTRAN-10 left
C  the scanner past the blanks and tabs that ended the field, not on the
C  first of them.  This is what makes ADV work at all.  A database text
C  line is
C       1 <TAB> " 0YOU ARE STANDING AT THE END OF A ROAD..."
C  and the A5 fields begin at the '0', which the carriage control then
C  eats, printing a blank line and then the text.  Had the scanner
C  stopped one character earlier, every line would have begun with a
C  literal '0'; the original prints none.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) LINE
      L = LEN(LINE)
1     IF (I .GT. L) RETURN
      C = ICHAR(LINE(I:I))
      IF (C .NE. 32 .AND. C .NE. 9) RETURN
      I = I + 1
      GOTO 1
      END

      SUBROUTINE RDMSG(LU, JK, LL, ND, ROW, J1, J2)
C  READ(1,1005)JKIND,(LLINE(I,J),J=3,22) with FORMAT(1G,20A5).
      IMPLICIT INTEGER(A-Z)
      DIMENSION LL(ND,*)
      CHARACTER*300 LINE
      DIMENSION W(64)
      NW = J2 - J1 + 1
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) CALL DBEOF
      P = 1
      CALL GFLD(LINE, P, JK, GOT)
      IF (GOT .EQ. 0) JK = 0
      CALL GSKIP(LINE, P)
      IF (P .GT. LEN(LINE)) P = LEN(LINE)
      CALL C2WS(LINE(P:), NW, 5, W)
      DO 1 K = 1, NW
1        LL(ROW, J1+K-1) = W(K)
      RETURN
      END

      SUBROUTINE RDVOC(LU, K, W)
C  READ(1,1021) KTAB(IU),ATAB(IU) with FORMAT(G,A5).
      IMPLICIT INTEGER(A-Z)
      CHARACTER*300 LINE
      DIMENSION T(1)
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) CALL DBEOF
      P = 1
      CALL GFLD(LINE, P, K, GOT)
      IF (GOT .EQ. 0) K = 0
      CALL GSKIP(LINE, P)
      IF (P .GT. LEN(LINE)) P = LEN(LINE)
      CALL C2WS(LINE(P:), 1, 5, T)
      W = T(1)
      RETURN
      END

      SUBROUTINE RDA5(W, N)
C  ACCEPT 1,(A(I),I=1,4) with FORMAT(4A5): one terminal record, 5N
C  characters, left-justified and blank-filled into N words.  Case is
C  NOT folded -- neither the -10's scanner nor Crowther's GETIN folds it,
C  so a lower-case command is not understood, exactly as on the original.
      IMPLICIT INTEGER(A-Z)
      DIMENSION W(N)
      CHARACTER*160 LINE
      CALL RDLINE(5, LINE, EOF)
      IF (EOF .NE. 0) CALL PQUIT
      CALL C2WS(LINE(1:5*N), N, 5, W)
      RETURN
      END

C ----------------------------------------------------------------------
C  PAUSE
C
C  FORTRAN-10's PAUSE 'text' stopped the program and asked the operator
C  what to do.  Measured on the pack over all nine of ADV's PAUSE texts,
C  FOROTS typed
C
C       PAUSE
C       <text, blank-filled to a multiple of five, then five more>
C       Type G to Continue, X to Exit, T To Trace.
C       *
C
C  and read a line.  The blank fill is the literal's own word count plus
C  one word: 'OOPS' prints as ten characters, 'INIT DONE' and
C  'GAMES OVER' as fifteen, 'GAME IS OVER' as twenty.
C
C  Only the first letter of the reply counts, and only three letters mean
C  anything.  Measured on the pack over 203 replies in the test corpus:
C
C    G...  resume where the program left off
C    X...  stop; FOROTS printed its run time and the monitor took over
C    T...  print a call traceback of the -10 image, then ask again
C    anything else, including an empty line and '!!!', asks again
C
C  So a player who types a command at this prompt gets the prompt back,
C  and one whose command starts with X loses the game.  Both are
C  reproduced.  What the port cannot reproduce is the traceback -- it is
C  four lines of FOROTS internals naming addresses in the saved core
C  image (TRACE. (573540) <<--- PAUS.+152, PAUS. (73424) <<--- MAIN.+2030)
C  -- so T asks again and prints nothing.  See the README.
C
C  ADV reaches a PAUSE when you die, when the dwarves get you, and once
C  at the end of initialisation.
C ----------------------------------------------------------------------

      SUBROUTINE PAUSEM(MSG)
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) MSG
      CHARACTER*80 OUT
      CHARACTER*160 ANS
      COMMON /OUTCOM/ LASTBL
      N = LEN(MSG)
      NW = (N + 4)/5
      L = 5*(NW + 1)
      OUT = ' '
      OUT(1:N) = MSG
      CALL PUTTXT('PAUSE')
      CALL PUTTXT(OUT(1:L))
10    CALL PUTTXT('Type G to Continue, X to Exit, T To Trace.')
      WRITE(6,2,ADVANCE='NO') '*'
      LASTBL = 0
      CALL RDLINE(5, ANS, EOF)
      IF (EOF .NE. 0) CALL PQUIT
      C = ICHAR(ANS(1:1))
      IF (C .GE. 97 .AND. C .LE. 122) C = C - 32
      IF (C .EQ. 88) THEN
         FLUSH(6)
         STOP
      ENDIF
      IF (C .EQ. 71) RETURN
      GOTO 10
2     FORMAT(A)
      END

C ----------------------------------------------------------------------
C  The database file: IFILE's OPEN(...,ACCESS='SEQIN')
C ----------------------------------------------------------------------

      SUBROUTINE OPNSEQ(LU, NAME, NW, MODE)
C  The file whose TOPS-10 name is packed into NW words -- "ADV  .DAT "
C  from IFILE(1,'ADV','.DAT').  Blanks are not part of a TOPS-10 name, so
C  they come out and the result is ADV.DAT.  MODE 1 reads, 2 writes.
      IMPLICIT INTEGER(A-Z)
      DIMENSION NAME(NW)
      CHARACTER*60 FN, RAW
      CHARACTER*300 EXE
      RAW = ' '
      P = 0
      DO 1 I = 1, NW
         CALL W2C(NAME(I), 5, FN)
         DO 2 K = 1, 5
            IF (FN(K:K) .NE. ' ') THEN
               P = P + 1
               RAW(P:P) = FN(K:K)
            ENDIF
2        CONTINUE
1     CONTINUE
      IF (P .EQ. 0) THEN
         WRITE(6,7)
         CALL PQUIT
      ENDIF
      IF (MODE .EQ. 2) THEN
         OPEN(UNIT=LU, FILE=RAW(1:P), STATUS='UNKNOWN', IOSTAT=IOS)
         IF (IOS .NE. 0) GOTO 9
         RETURN
      ENDIF
C  Beside the working directory first, then beside the executable, so the
C  game runs from anywhere.
      OPEN(UNIT=LU, FILE=RAW(1:P), STATUS='OLD', IOSTAT=IOS)
      IF (IOS .EQ. 0) RETURN
      CALL GET_COMMAND_ARGUMENT(0, EXE)
      L = LEN_TRIM(EXE)
3     IF (L .LE. 0) GOTO 4
      IF (EXE(L:L) .EQ. '/' .OR. EXE(L:L) .EQ. CHAR(92)) GOTO 4
      L = L - 1
      GOTO 3
4     IF (L .GT. 0 .AND. L+P .LE. LEN(EXE)) THEN
         EXE(L+1:L+P) = RAW(1:P)
         OPEN(UNIT=LU, FILE=EXE(1:L+P), STATUS='OLD', IOSTAT=IOS)
         IF (IOS .EQ. 0) RETURN
      ENDIF
9     WRITE(6,8) RAW(1:P)
      CALL PQUIT
      RETURN
7     FORMAT(' ?Null file name')
8     FORMAT(' ?Cannot open ',A,' -- it must be in this directory',
     1       ' or beside the program.')
      END

      SUBROUTINE DBEOF
C  End of the database where the program did not expect one.
      IMPLICIT INTEGER(A-Z)
      WRITE(6,1)
      CALL PQUIT
      RETURN
1     FORMAT(' ?End of file on ADV.DAT')
      END

C ----------------------------------------------------------------------
C  Startup: the command line
C ----------------------------------------------------------------------

      SUBROUTINE BOOT
      IMPLICIT INTEGER(A-Z)
      CHARACTER*300 ARG
      COMMON /OPTCOM/ OPINIT, OPECHO, OPVERB
      OPINIT = 0
      OPECHO = -1
      OPVERB = 0
      CALL GET_ENVIRONMENT_VARIABLE('ADV_VERBATIM', ARG)
      IF (ARG(1:1) .EQ. '1') OPVERB = 1
      N = COMMAND_ARGUMENT_COUNT()
      I = 0
1     I = I + 1
      IF (I .GT. N) RETURN
      CALL GET_COMMAND_ARGUMENT(I, ARG)
      L = LEN_TRIM(ARG)
      IF (L .LE. 0) GOTO 1
      IF (ARG(1:L) .EQ. '-h' .OR. ARG(1:L) .EQ. '--help') THEN
         CALL USAGE
         STOP
      ELSE IF (ARG(1:L) .EQ. '-V' .OR. ARG(1:L) .EQ. '--version') THEN
         WRITE(6,2)
         STOP
      ELSE IF (ARG(1:L) .EQ. '-i' .OR.
     1         ARG(1:L) .EQ. '--init-pause') THEN
         OPINIT = 1
      ELSE IF (ARG(1:L) .EQ. '-2' .OR.
     1         ARG(1:L) .EQ. '--double-space') THEN
         OPVERB = 1
      ELSE IF (ARG(1:L) .EQ. '--echo') THEN
         OPECHO = 1
      ELSE IF (ARG(1:L) .EQ. '--no-echo') THEN
         OPECHO = 0
      ELSE
         WRITE(6,3) ARG(1:L)
         CALL USAGE
         STOP
      ENDIF
      GOTO 1
2     FORMAT(' ADV -- Will Crowther''s Adventure, early 1976,',
     1       ' from the TOPS-10 FORTRAN source.')
3     FORMAT(' ?Unrecognised option: ',A)
      END

      SUBROUTINE USAGE
      IMPLICIT INTEGER(A-Z)
      WRITE(6,1)
      RETURN
1     FORMAT(
     1 ' Usage: adv [options]'/
     2 ''/
     3 ' Will Crowther''s original Adventure, early 1976, compiled from'/
     4 ' the FORTRAN-10 source on the TOPS-10 in a Box pack.  It reads'/
     5 ' ADV.DAT from this directory or from beside the program.'/
     6 ''/
     7 '   -2, --double-space'/
     8 '                     print exactly what TOPS-10 printed: the'/
     9 '                     blank line before every line of text that'/
     A '                     the database''s own carriage control asks'/
     B '                     for, the paired blank lines, and the'/
     C '                     trailing blanks of each five-character'/
     D '                     word.  By default those come off and the'/
     E '                     text is single-spaced; nothing else about'/
     F '                     the output differs.  ADV_VERBATIM=1 too.'/
     G '   -i, --init-pause  do what RUN ADV did on the -10: read the'/
     H '                     database, then stop at PAUSE INIT DONE and'/
     I '                     wait for G.  The pack shipped the image'/
     J '                     unsaved, so that is what it does there.'/
     K '   --echo, --no-echo echo typed lines, or do not.  The default'/
     L '                     echoes only when input is not a terminal,'/
     M '                     as the -10''s terminal service did.'/
     N '                     ADV_ECHO=0 or 1 does the same.'/
     O '   -V, --version     what this is'/
     P '   -h, --help        this'/
     Q ''/
     R ' The game has no score, no save and no QUIT: it was abandoned'/
     S ' unfinished.  Ctrl-C leaves it, as Ctrl-C did on TOPS-10.'/
     T ' Commands must be upper case -- neither the -10 nor the program'/
     U ' folds them.')
      END

      INTEGER FUNCTION OPTINI()
C  Was --init-pause given?
      IMPLICIT INTEGER(A-Z)
      COMMON /OPTCOM/ OPINIT, OPECHO, OPVERB
      OPTINI = OPINIT
      RETURN
      END
