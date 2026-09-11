C  runtime.f -- the PDP-10 machine layer for the ADVENT 350 port.
C
C  ADVENT.FOR is DEC FORTRAN-10 for a 36-bit word.  The port keeps that
C  word: five seven-bit characters, character k in bits 36-7k..30-7k,
C  held sign-extended in an INTEGER*8.  The program's own tables fix the
C  layout --
C       GETIN:   DATA MASKS/"4000000000,"20000000,"100000,"400,"2,0/
C       A5TOA1:  DATA MASK,BLANK/"774000000000,' '/
C  -- the low bit of each of the five fields is at 29, 22, 15, 8, 1, and
C  "774000000000 selects the first character.  Because the layout is
C  unchanged every mask, SHIFT, .AND./.XOR. and five-character literal in
C  the game logic still works exactly as written, and not one line of
C  game logic had to be touched.
C
C  What the compiler genuinely cannot do is the A edit descriptor over
C  that layout (gfortran packs eight-bit bytes), the DEC free-format "G"
C  descriptor, ACCEPT, and CALL DATE/CALL TIME.  Those go through the
C  routines here.  Nothing here contains game logic.

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
C  Sign-extend: bit 35 is the sign on a PDP-10 and the program tests it
C  (A5TOA1 inserts a blank only IF(C.LT.0), i.e. when the word's first
C  character is 100 octal or above).  Literals are packed the same way.
      IF (IAND(W, 34359738368) .NE. 0) W = W - 68719476736
      RETURN
      END

      SUBROUTINE W2C(W, N, S)
C  Unpack the first N characters of word W into S.  TAB is passed
C  through: section 6 of the database contains lines whose text really
C  does start with tabs ("- - -" and "--- POOF!! ---").
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

      CHARACTER*160 FUNCTION WSTR(W, NW, NC)
C  Unpack NW words of NC characters into a string, for the A edit
C  descriptor.  Used with an exact-width An descriptor the output is
C  character for character what FORTRAN-10 produced from nAm.
      IMPLICIT INTEGER(A-Z)
      DIMENSION W(NW)
      CHARACTER*8 T
      WSTR = ' '
      DO 1 I = 1, NW
         CALL W2C(W(I), NC, T)
         P = (I-1)*NC + 1
         IF (P+NC-1 .LE. 160) WSTR(P:P+NC-1) = T(1:NC)
1     CONTINUE
      RETURN
      END

      INTEGER FUNCTION SHIFT(VAL, DIST)
C  Logical shift of a 36-bit word; positive DIST shifts left.  The
C  program's own SHIFT is 36-bit arithmetic that tests bit 35 as the sign
C  (IF(SHIFT.LT.0)) and re-inserts it; in 64 bits a left shift would
C  leave the result un-sign-extended, so words with bit 35 set would stop
C  comparing equal to the packed literals.  Same bits, done directly.
      IMPLICIT INTEGER(A-Z)
      PARAMETER (M36 = 68719476735)
      V = IAND(VAL, M36)
      IF (DIST .GE. 0) THEN
         SHIFT = IAND(ISHFT(V, DIST), M36)
         IF (IAND(SHIFT, 34359738368) .NE. 0) SHIFT = SHIFT - 68719476736
      ELSE
         SHIFT = ISHFT(V, DIST)
      ENDIF
      RETURN
      END

C ----------------------------------------------------------------------
C  Clock.  DATIME itself is the original, untouched: it decodes the ASCII
C  that DEC's CALL DATE / CALL TIME hand back.  Only the two library
C  calls are replaced, by routines that hand back exactly the same ASCII
C  a TOPS-10 FOROTS does -- so D still comes out as days since
C  01-JAN-1977 and T as minutes past midnight, by the program's own
C  arithmetic.
C
C  The year is the interesting part.  FOROTS writes dd-mmm-yy with yy =
C  year-1900 formatted as two characters, tens digit '0'+(yy/10) -- which
C  for 2007 overflows past '9' into ':' ("06-Jan-:7") and for 2010 into
C  ';' ("23-Nov-;0").  DATIME's I2 takes the low four bits of each
C  character, so ':' reads as 10 and ';' as 11, and the year decodes as
C  107 and 110.  The 1977 program therefore still gets the date right in
C  the 21st century, by accident.  Measured against the original running
C  under SIMH: with the pack's clock at 06-Jan-2007 12:00 the game's
C  wizard challenge is TEWYF, which pins D=10962, T=720; at
C  23-Nov-2010 09:37 it is EPZOP, which pins D=12379, T=577.  Both are
C  the true days since 01-JAN-1977.  This code reproduces that.
C ----------------------------------------------------------------------

      SUBROUTINE DCDATE(DAT)
C  Stand-in for DEC's CALL DATE(DAT): dd-mmm-yy packed 5 characters per
C  word, two words.
      IMPLICIT INTEGER(A-Z)
      DIMENSION DAT(2)
      CHARACTER*9 S
      CHARACTER*36 MONS
      DATA MONS/'JanFebMarAprMayJunJulAugSepOctNovDec'/
      CALL PCLOCK(Y, M, D, HH, MM)
      YY = Y - 1900
      WRITE(S,10) D, MONS(3*M-2:3*M), CHAR(48+YY/10), CHAR(48+MOD(YY,10))
10    FORMAT(I2.2,'-',A3,'-',A1,A1)
      CALL C2W(S(1:5), 5, DAT(1))
      CALL C2W(S(6:9), 5, DAT(2))
      RETURN
      END

      SUBROUTINE DCTIME(TIM)
C  Stand-in for DEC's CALL TIME(TIM).  The -10 handed back hh:mm:ss;
C  DATIME reads only the first word, so only hh:mm is built here.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*5 S
      CALL PCLOCK(Y, M, D, HH, MM)
      WRITE(S,10) HH, MM
10    FORMAT(I2.2,':',I2.2)
      CALL C2W(S, 5, TIM)
      RETURN
      END

      SUBROUTINE PCLOCK(Y, M, D, HH, MM)
C  The host clock, or the frozen clock set by -d/-t or ADVENT_DATE /
C  ADVENT_TIME.  Freezing it makes the whole game deterministic, because
C  RAN is seeded from DATIME and from nothing else.
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      INTEGER V(8)
      IF (IFROZE .NE. 0) THEN
         Y = FY
         M = FM
         D = FD
         HH = FHH
         MM = FMM
         RETURN
      ENDIF
      CALL DATE_AND_TIME(VALUES = V)
      Y = V(1)
      M = V(2)
      D = V(3)
      HH = V(5)
      MM = V(6)
      RETURN
      END

C ----------------------------------------------------------------------
C  Input.  ACCEPT and the DEC free-format G descriptor.
C ----------------------------------------------------------------------

      SUBROUTINE RDLINE(LU, S, EOF)
C  One record.  On the terminal, flush first so that prompts appear
C  before the read even when output is a pipe.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      S = ' '
      EOF = 0
      IF (LU .EQ. 5) FLUSH(6)
      READ(LU, '(A)', IOSTAT=IOS) S
      IF (IOS .NE. 0) THEN
         EOF = 1
         S = ' '
         RETURN
      ENDIF
C  A CR left by a DOS-format script is not part of the line.
      DO 1 I = 1, LEN(S)
         IF (ICHAR(S(I:I)) .EQ. 13) S(I:I) = ' '
1     CONTINUE
C  The -10's terminal service echoed what was typed, which is why the
C  commands appear in a transcript of the original.  A console does its
C  own echoing, so this applies only when the input is not a terminal.
      IF (LU .EQ. 5 .AND. IECHO() .NE. 0) WRITE(6,2) S(1:LEN_TRIM(S))
2     FORMAT(A)
      RETURN
      END

      INTEGER FUNCTION IECHO()
C  Echo typed lines?  ADVENT_ECHO=0 or 1 decides it if set; otherwise
C  echo exactly when the input is not a terminal.
      IMPLICIT INTEGER(A-Z)
      LOGICAL ISATTY
      CHARACTER*8 EV
      SAVE KNOWN, VAL
      DATA KNOWN /0/
      IF (KNOWN .EQ. 0) THEN
         CALL GET_ENVIRONMENT_VARIABLE('ADVENT_ECHO', EV)
         IF (EV(1:1) .EQ. '0') THEN
            VAL = 0
         ELSE IF (EV(1:1) .EQ. '1') THEN
            VAL = 1
         ELSE IF (ISATTY(5)) THEN
            VAL = 0
         ELSE
            VAL = 1
         ENDIF
         KNOWN = 1
      ENDIF
      IECHO = VAL
      RETURN
      END

      SUBROUTINE RDFRE(LU, V, N)
C  DEC free-format input, FORMAT(G) and FORMAT(99G): up to N integers
C  from one record, the rest left zero.  The program depends on the
C  zero fill -- section 3 records carry a variable number of motion
C  verbs and the loop stops at the first zero -- and on a null line from
C  the terminal reading as zero ("null to leave at NN").
      IMPLICIT INTEGER(A-Z)
      DIMENSION V(N)
      CHARACTER*300 LINE
      DO 1 I = 1, N
1        V(I) = 0
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) THEN
         IF (LU .EQ. 5) CALL PQUIT
         RETURN
      ENDIF
      I = 1
      K = 0
      L = LEN(LINE)
5     IF (I .GT. L) RETURN
      C = ICHAR(LINE(I:I))
      IF (C .EQ. 45 .OR. (C .GE. 48 .AND. C .LE. 57)) GOTO 10
      I = I + 1
      GOTO 5
10    SGN = 1
      IF (C .EQ. 45) THEN
         SGN = -1
         I = I + 1
      ENDIF
      VAL = 0
      GOT = 0
15    IF (I .GT. L) GOTO 20
      C = ICHAR(LINE(I:I))
      IF (C .LT. 48 .OR. C .GT. 57) GOTO 20
      VAL = VAL*10 + (C - 48)
      GOT = 1
      I = I + 1
      GOTO 15
20    IF (GOT .EQ. 0) GOTO 5
      K = K + 1
      V(K) = SGN*VAL
      IF (K .GE. N) RETURN
      GOTO 5
      END

      SUBROUTINE RDMSG(LU, LOC, W, KK)
C  FORMAT(1G,15A5) over a database text record: a location number, the
C  tab that terminated it, then up to 70 characters of text in 14 words
C  and a 15th word which must be blank (the program calls BUG(0) if the
C  line was longer than 70 characters).
      IMPLICIT INTEGER(A-Z)
      DIMENSION W(14)
      CHARACTER*300 LINE
      CHARACTER*75 TEXT
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) THEN
         LOC = -1
         CALL C2W(' ', 5, KK)
         RETURN
      ENDIF
      TEXT = ' '
      LOC = 0
      I = 1
      L = LEN(LINE)
      SGN = 1
C  leading blanks, then the number
2     IF (I .GT. L) GOTO 3
      IF (LINE(I:I) .NE. ' ') GOTO 3
      I = I + 1
      GOTO 2
3     IF (I .GT. L) GOTO 5
      IF (LINE(I:I) .NE. '-') GOTO 5
      SGN = -1
      I = I + 1
5     IF (I .GT. L) GOTO 10
      C = ICHAR(LINE(I:I))
      IF (C .LT. 48 .OR. C .GT. 57) GOTO 10
      LOC = LOC*10 + (C - 48)
      I = I + 1
      GOTO 5
C  The G descriptor consumed the delimiter -- and FORTRAN-10 treated a
C  whole run of blanks and tabs as ONE delimiter, not one character of
C  it.  The -10 proves it: ADVENT.DAT stores message 82 of section 6 as
C  82<tab><tab><tab>    --- POOF!! ---  and RTEXT 1 as
C  1<tab><tab><tab><tab>      - - -  , yet the original prints both hard
C  against the left margin.  Consuming only the first tab would indent
C  them to column 31 and 25.
10    LOC = SGN*LOC
11    IF (I .GT. L) GOTO 12
      C = ICHAR(LINE(I:I))
      IF (C .NE. 9 .AND. C .NE. 32) GOTO 12
      I = I + 1
      GOTO 11
12    CONTINUE
      IF (I .LE. L) TEXT = LINE(I:MIN(L,I+74))
      CALL C2WS(TEXT(1:70), 14, 5, W)
      CALL C2W(TEXT(71:75), 5, KK)
      RETURN
      END

      SUBROUTINE RDVOC(LU, K, W)
C  FORMAT(G,A5) over a vocabulary record: number, tab, five letters.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*300 LINE
      CALL RDLINE(LU, LINE, EOF)
      IF (EOF .NE. 0) THEN
         K = -1
         CALL C2W(' ', 5, W)
         RETURN
      ENDIF
      K = 0
      I = 1
      L = LEN(LINE)
      SGN = 1
2     IF (I .GT. L) GOTO 3
      IF (LINE(I:I) .NE. ' ') GOTO 3
      I = I + 1
      GOTO 2
3     IF (I .GT. L) GOTO 5
      IF (LINE(I:I) .NE. '-') GOTO 5
      SGN = -1
      I = I + 1
5     IF (I .GT. L) GOTO 10
      C = ICHAR(LINE(I:I))
      IF (C .LT. 48 .OR. C .GT. 57) GOTO 10
      K = K*10 + (C - 48)
      I = I + 1
      GOTO 5
10    K = SGN*K
      IF (I .LE. L) THEN
         C = ICHAR(LINE(I:I))
         IF (C .EQ. 9 .OR. C .EQ. 32) I = I + 1
      ENDIF
      IF (I .GT. L) THEN
         CALL C2W(' ', 5, W)
      ELSE
         CALL C2W(LINE(I:MIN(L,I+4)), 5, W)
      ENDIF
      RETURN
      END

      SUBROUTINE RDA5(W, N)
C  ACCEPT n,(A(I),I=1,N) with FORMAT(nA5): one terminal record, 5N
C  characters, left-justified and blank-filled into N words.  Case is
C  NOT folded here -- GETIN does that itself, arithmetically, and MAINT
C  wants the holiday name as typed.
      IMPLICIT INTEGER(A-Z)
      DIMENSION W(N)
      CHARACTER*160 LINE
      CALL RDLINE(5, LINE, EOF)
      IF (EOF .NE. 0) CALL PQUIT
      CALL C2WS(LINE(1:5*N), N, 5, W)
      RETURN
      END

      SUBROUTINE PQUIT
C  End of input.  The -10 would have killed the job.
      IMPLICIT INTEGER(A-Z)
      FLUSH(6)
      STOP
      END

C ----------------------------------------------------------------------
C  The database file
C ----------------------------------------------------------------------

      SUBROUTINE DBOPEN(LU)
C  OPEN(UNIT=1,NAME='ADVENT',ACCESS='SEQIN') -- ADVENT.DAT, from the
C  working directory, or from beside the executable so the game can be
C  run from anywhere.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*512 P
      LOGICAL EX
      INQUIRE(FILE='ADVENT.DAT', EXIST=EX)
      IF (EX) THEN
         OPEN(UNIT=LU, FILE='ADVENT.DAT', STATUS='OLD')
         RETURN
      ENDIF
      CALL GET_COMMAND_ARGUMENT(0, P)
      L = LEN_TRIM(P)
      DO 1 I = L, 1, -1
         IF (P(I:I) .EQ. '/' .OR. P(I:I) .EQ. '\') THEN
            P = P(1:I) // 'ADVENT.DAT'
            INQUIRE(FILE=P, EXIST=EX)
            IF (EX) THEN
               OPEN(UNIT=LU, FILE=P, STATUS='OLD')
               RETURN
            ENDIF
            GOTO 2
         ENDIF
1     CONTINUE
2     PRINT 3
3     FORMAT(' ADVENT.DAT not found in the current directory or beside'
     1       ,' the program.')
      CALL PQUIT
      END

C ----------------------------------------------------------------------
C  Options, and the core image
C ----------------------------------------------------------------------

      INTEGER FUNCTION VERBOS()
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      VERBOS = IVERB
      RETURN
      END

      INTEGER FUNCTION UNLIM()
C  -u.  Lifts the prime-time lockout, the demonstration-game turn limit
C  and the latency wait before a suspended game may be resumed.  Nothing
C  else: scoring, the random number generator and the game logic are
C  untouched by it.
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      UNLIM = IUNLIM
      RETURN
      END

      SUBROUTINE INITD
C  Stands in for PAUSE 'INIT Done'.  On TOPS-10 the program stopped here
C  so the user could type SAVE ADVENT and keep the initialised core
C  image; this port has no monitor to return to and just carries on.
      IMPLICIT INTEGER(A-Z)
      IF (VERBOS() .NE. 0) PRINT 1
1     FORMAT(' INIT Done')
      RETURN
      END

      SUBROUTINE BOOT
C  Entered before the program's first statement, which is where a
C  restarted TOPS-10 core image came back in.  Reads the command line,
C  and if a saved game was named, reloads the whole state -- after which
C  SETUP is -1 or 2 exactly as it was when the image was written, and the
C  program picks up at 8305 or at 1 by itself.
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      COMMON /PORTNM/ SAVNAM
      CHARACTER*512 SAVNAM, A, IMG
      CHARACTER*512 EV
      IVERB = 0
      IUNLIM = 0
      IFROZE = 0
      SAVNAM = 'ADVENT.SAV'
      IMG = ' '
      CALL GET_ENVIRONMENT_VARIABLE('ADVENT_DATE', EV)
      IF (EV .NE. ' ') CALL SETDAT(EV)
      CALL GET_ENVIRONMENT_VARIABLE('ADVENT_TIME', EV)
      IF (EV .NE. ' ') CALL SETTIM(EV)
      CALL GET_ENVIRONMENT_VARIABLE('ADVENT_SAVE', EV)
      IF (EV .NE. ' ') SAVNAM = EV
      N = COMMAND_ARGUMENT_COUNT()
      I = 1
10    IF (I .GT. N) GOTO 90
      CALL GET_COMMAND_ARGUMENT(I, A)
      IF (A .EQ. '-u' .OR. A .EQ. '-U') THEN
         IUNLIM = 1
      ELSE IF (A .EQ. '-v' .OR. A .EQ. '-V') THEN
         IVERB = 1
      ELSE IF (A .EQ. '-h' .OR. A .EQ. '--help' .OR. A .EQ. '-?') THEN
         CALL USAGE
      ELSE IF (A .EQ. '-d' .OR. A .EQ. '--date') THEN
         I = I + 1
         CALL GET_COMMAND_ARGUMENT(I, A)
         CALL SETDAT(A)
      ELSE IF (A .EQ. '-t' .OR. A .EQ. '--time') THEN
         I = I + 1
         CALL GET_COMMAND_ARGUMENT(I, A)
         CALL SETTIM(A)
      ELSE IF (A .EQ. '-s' .OR. A .EQ. '--save') THEN
         I = I + 1
         CALL GET_COMMAND_ARGUMENT(I, SAVNAM)
      ELSE IF (A(1:1) .EQ. '-') THEN
         PRINT 11, A(1:LEN_TRIM(A))
11       FORMAT(' Unknown option ',A)
         CALL USAGE
      ELSE
         IMG = A
      ENDIF
      I = I + 1
      GOTO 10
90    IF (IMG .NE. ' ') CALL STLOAD(IMG)
      RETURN
      END

      SUBROUTINE USAGE
      IMPLICIT INTEGER(A-Z)
      PRINT 1
1     FORMAT(
     1 ' advent350 -- Colossal Cave Adventure, 350 points.'/
     2 ' Woods and Crowther, DEC FORTRAN-10, DECUS conversion by'
     3 ,' Paul T. Robinson 1980.'//
     4 ' usage:  advent350 [options] [saved-game]'//
     5 '   -u             lift the cave hours, the demonstration-game'
     6 ,' turn limit and the'/
     7 '                  wait before a suspended game may be resumed.'
     8 ,''/
     9 '                  It changes nothing else: not scoring, not the'
     1 ,' random numbers.'/
     2 '                  It also answers MAGIC MODE for you: the magic'
     1 ,' word, and the'/
     2 '                  reply to the challenge, are printed as you'
     3 ,' are asked for them.'/
     4 '   -s FILE        write SUSPEND to FILE (default ADVENT.SAV).'/
     1 '   -d DD-MMM-YYYY freeze the date.  -t HHMM freezes the time.'/
     2 '                  ADVENT_DATE, ADVENT_TIME and ADVENT_SAVE do'
     3 ,' the same.'/
     4 '   -v             show the database initialisation report.'/
     5 '   -h             this.'//
     6 ' A saved-game argument reloads a game left by SUSPEND, or a'
     7 ,' version left by'/
     8 ' MAGIC MODE, the way RUNning a saved core image did on'
     9 ,' TOPS-10.'/)
      CALL PQUIT
      END

      SUBROUTINE SETDAT(S)
C  -d DD-MMM-YYYY
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      CHARACTER*(*) S
      CHARACTER*36 MONS
      CHARACTER*3 M3
      DATA MONS/'JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC'/
      IF (LEN_TRIM(S) .LT. 11) GOTO 90
      READ(S(1:2),'(I2)',IOSTAT=IOS) D
      IF (IOS .NE. 0) GOTO 90
      M3 = S(4:6)
      DO 1 I = 1, 3
         C = ICHAR(M3(I:I))
         IF (C .GE. 97 .AND. C .LE. 122) M3(I:I) = CHAR(C-32)
1     CONTINUE
      M = 0
      DO 2 I = 1, 12
2        IF (M3 .EQ. MONS(3*I-2:3*I)) M = I
      IF (M .EQ. 0) GOTO 90
      READ(S(8:11),'(I4)',IOSTAT=IOS) Y
      IF (IOS .NE. 0) GOTO 90
      FY = Y
      FM = M
      FD = D
      IF (IFROZE .EQ. 0) THEN
         FHH = 0
         FMM = 0
      ENDIF
      IFROZE = 1
      RETURN
90    PRINT 91
91    FORMAT(' Date must be DD-MMM-YYYY, e.g. 06-JAN-2007.')
      CALL PQUIT
      END

      SUBROUTINE SETTIM(S)
C  -t HHMM (or HH:MM)
      IMPLICIT INTEGER(A-Z)
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      CHARACTER*(*) S
      CHARACTER*4 T4
      INTEGER V(8)
      L = LEN_TRIM(S)
      IF (L .EQ. 5 .AND. S(3:3) .EQ. ':') THEN
         T4 = S(1:2) // S(4:5)
      ELSE IF (L .EQ. 4) THEN
         T4 = S(1:4)
      ELSE
         GOTO 90
      ENDIF
      READ(T4(1:2),'(I2)',IOSTAT=IOS) HH
      IF (IOS .NE. 0) GOTO 90
      READ(T4(3:4),'(I2)',IOSTAT=IOS) MM
      IF (IOS .NE. 0) GOTO 90
      IF (HH .LT. 0 .OR. HH .GT. 23 .OR. MM .LT. 0 .OR. MM .GT. 59)
     1   GOTO 90
      IF (IFROZE .EQ. 0) THEN
         CALL DATE_AND_TIME(VALUES = V)
         FY = V(1)
         FM = V(2)
         FD = V(3)
      ENDIF
      FHH = HH
      FMM = MM
      IFROZE = 1
      RETURN
90    PRINT 91
91    FORMAT(' Time must be HHMM, e.g. 1200.')
      CALL PQUIT
      END

C ----------------------------------------------------------------------
C  SUSPEND, and the saved core image.
C
C  On TOPS-10 SUSPEND and MAGIC MODE both ended in CIAO, which stopped
C  the program and told the user to SAVE the core image at the monitor
C  prompt; restarting that image re-entered the program at its first
C  statement with SETUP still -1 (a suspended game) or 2 (a version
C  tweaked in maintenance mode).  There is no core image here, so the
C  program writes the file itself.  It is a complete capture: every
C  COMMON block, every variable of the main program (they all live in
C  /ADVSTA/, generated by tools/convert.py from the declarations so that
C  none can be missed), MOTD's message of the day and RAN's seed.
C ----------------------------------------------------------------------

      SUBROUTINE BLKIO(LU, MODE, W, N)
C  One variable or array, written or read.  MODE 1 writes, 0 reads,
C  2 only counts.  The running count and sum let the file check itself.
      IMPLICIT INTEGER(A-Z)
      DIMENSION W(N)
      COMMON /PORTIO/ NWORD, CKSUM
      IF (MODE .EQ. 1) THEN
         WRITE(LU) (W(I), I = 1, N)
      ELSE IF (MODE .EQ. 0) THEN
         READ(LU) (W(I), I = 1, N)
      ENDIF
      NWORD = NWORD + N
      IF (MODE .NE. 2) THEN
         DO 1 I = 1, N
1           CKSUM = IAND(IEOR(CKSUM*31 + W(I), N), 281474976710655)
      ENDIF
      RETURN
      END

      SUBROUTINE CIAOSV(K)
C  Replaces CALL MSPEAK(K) in CIAO.  Magic message 32 is "BE SURE TO
C  SAVE YOUR CORE-IMAGE..." and 31 is "BREAK OUT OF THIS AND SAVE YOUR
C  CORE-IMAGE."  Neither is true here, so this says what did happen.
      IMPLICIT INTEGER(A-Z)
      DIMENSION HNAME(4)
      COMMON /WIZCOM/ WKDAY,WKEND,HOLID,HBEGIN,HEND,HNAME,
     1   SHORT,MAGIC,MAGNM,LATNCY,SAVED,SAVET,SETUP
      COMMON /PORTNM/ SAVNAM
      CHARACTER*512 SAVNAM
      CALL STSAVE(SAVNAM, IERR)
      L = LEN_TRIM(SAVNAM)
      IF (IERR .NE. 0) THEN
         PRINT 1, SAVNAM(1:L)
1        FORMAT(/' I could not write ',A,'.  Nothing has been saved.')
         RETURN
      ENDIF
      IF (SETUP .EQ. 2) THEN
         PRINT 2, SAVNAM(1:L), SAVNAM(1:L)
2        FORMAT(/' This version has been saved in ',A,'.'/
     1           ' Run the game with ',A,' as its argument to use it.')
      ELSE
         PRINT 3, SAVNAM(1:L), SAVNAM(1:L)
3        FORMAT(/' Your adventure has been saved in ',A,'.'/
     1           ' To resume it, run the game with ',A,
     2           ' as its argument.')
      ENDIF
      RETURN
      END

      SUBROUTINE STSAVE(FN, IERR)
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) FN
      CHARACTER*16 MAGIC
      COMMON /PORTIO/ NWORD, CKSUM
      DATA MAGIC/'ADVENT350-IMAGE '/
      IERR = 0
      NWORD = 0
      CKSUM = 0
      OPEN(UNIT=9, FILE=FN, STATUS='REPLACE', FORM='UNFORMATTED',
     1     ACCESS='STREAM', IOSTAT=IOS)
      IF (IOS .NE. 0) THEN
         IERR = 1
         RETURN
      ENDIF
      WRITE(9) MAGIC
      WRITE(9) 1
      CALL STATIO(9, 1)
      WRITE(9) NWORD
      WRITE(9) CKSUM
      CLOSE(9)
      RETURN
      END

      SUBROUTINE STLOAD(FN)
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) FN
      CHARACTER*16 MAGIC, GOT
      COMMON /PORTIO/ NWORD, CKSUM
      DATA MAGIC/'ADVENT350-IMAGE '/
      NWORD = 0
      CKSUM = 0
      OPEN(UNIT=9, FILE=FN, STATUS='OLD', FORM='UNFORMATTED',
     1     ACCESS='STREAM', IOSTAT=IOS)
      IF (IOS .NE. 0) THEN
         PRINT 1, FN(1:LEN_TRIM(FN))
1        FORMAT(' Cannot open ',A,'.')
         CALL PQUIT
      ENDIF
      READ(9, IOSTAT=IOS) GOT
      IF (IOS .NE. 0 .OR. GOT .NE. MAGIC) GOTO 90
      READ(9, IOSTAT=IOS) VER
      IF (IOS .NE. 0 .OR. VER .NE. 1) GOTO 90
      CALL STATIO(9, 0)
      READ(9, IOSTAT=IOS) NW
      IF (IOS .NE. 0 .OR. NW .NE. NWORD) GOTO 90
      READ(9, IOSTAT=IOS) CK
      IF (IOS .NE. 0 .OR. CK .NE. CKSUM) GOTO 90
      CLOSE(9)
      RETURN
90    PRINT 91, FN(1:LEN_TRIM(FN))
91    FORMAT(' ',A,' is not a saved Adventure, or is damaged.')
      CALL PQUIT
      END

C ----------------------------------------------------------------------
C  The terminal's tab stops
C ----------------------------------------------------------------------

      CHARACTER*200 FUNCTION TABX(S)
C  Expand tabs to the next multiple-of-eight column, which is what the
C  -10's terminal service did with them.  577 lines of ADVENT.DAT use a
C  tab to separate sentences, and one uses tabs to indent, so without
C  this the port's text is laid out differently from the original's.
C  Column 1 is the first character actually printed -- the carriage
C  control character has already gone.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*(*) S
      TABX = ' '
      P = 0
      DO 1 I = 1, LEN(S)
         IF (S(I:I) .EQ. CHAR(9)) THEN
            P = (P/8 + 1)*8
         ELSE
            P = P + 1
            IF (P .LE. 200) TABX(P:P) = S(I:I)
         ENDIF
1     CONTINUE
      RETURN
      END

C  ----------------------------------------------------------------
C  MAGIC MODE under -u.
C
C  WIZARD proves you are a wizard twice over: it asks for the magic
C  word, and then it prints five random letters and demands five back.
C  The reply is a function of those letters, the magic number and the
C  clock -- and the clock enters it as (T/60)*40+(T/10)*10, so an answer
C  is good only for the ten minutes it was computed in.  That is what
C  the distribution's README means by "the timing is tricky".
C
C  The arithmetic below is not a second implementation of that rule: it
C  is WIZARD's own loop, copied statement for statement, so the hint
C  cannot drift away from the check it is predicting.  If a wizard
C  changes the magic number, both follow it together.
C
C  Nothing here is reachable without -u, and nothing here changes what
C  WIZARD accepts -- an impostor is still an impostor.

      SUBROUTINE WZREPL(VAL, MAGNM, TMIN, S)
C  The reply WIZARD will expect for challenge VAL at TMIN minutes past
C  midnight.  Compare WIZARD's loop at label 19.
      IMPLICIT INTEGER(A-Z)
      DIMENSION VAL(5)
      CHARACTER*5 S
      T = (TMIN/60)*40 + (TMIN/10)*10
      D = MAGNM
      DO 1 Y = 1, 5
         Z = MOD(Y,5) + 1
         X = MOD(IABS(VAL(Y)-VAL(Z))*MOD(D,10) + MOD(T,10), 26) + 1
         S(Y:Y) = CHAR(64+X)
         T = T/10
         D = D/10
1     CONTINUE
      RETURN
      END

      SUBROUTINE WIZWRD(MAGIC)
C  Show the magic word.  It is DWARF as shipped, but MAINT lets a wizard
C  change it, and a changed one is otherwise unrecoverable.
      IMPLICIT INTEGER(A-Z)
      CHARACTER*5 S
      IF (UNLIM() .EQ. 0) RETURN
      CALL W2C(MAGIC, 5, S)
      PRINT 1, S
1     FORMAT(' -u: the magic word is ',A5)
      RETURN
      END

      SUBROUTINE WIZHNT(VAL, MAGNM)
C  Show the reply to the challenge just printed.  If the clock is live,
C  also show what it becomes at the next ten-minute boundary, since the
C  answer can expire between being read and being typed; with a frozen
C  clock (-d/-t) it cannot, so only one answer is shown.
      IMPLICIT INTEGER(A-Z)
      DIMENSION VAL(5)
      CHARACTER*5 S, S2
      COMMON /PORTOP/ IVERB,IUNLIM,IFROZE,FY,FM,FD,FHH,FMM
      IF (IUNLIM .EQ. 0) RETURN
      CALL DATIME(D, TMIN)
      CALL WZREPL(VAL, MAGNM, TMIN, S)
      PRINT 1, S
1     FORMAT(' -u: the reply is ',A5)
      IF (IFROZE .NE. 0) RETURN
      TN = (TMIN/10)*10 + 10
      IF (TN .GE. 1440) TN = 0
      CALL WZREPL(VAL, MAGNM, TN, S2)
      IF (S2 .EQ. S) RETURN
      PRINT 2, TN/60, MOD(TN,60), S2
2     FORMAT(' -u: from ',I2.2,':',I2.2,' it becomes ',A5)
      RETURN
      END
