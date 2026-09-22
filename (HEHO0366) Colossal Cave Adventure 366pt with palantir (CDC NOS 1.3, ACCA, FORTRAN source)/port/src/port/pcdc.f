C ======================================================================
C  PCDC - what the CDC 6000/Cyber and NOS 1.3 did for this program, and
C  what FORTRAN Extended did for it, written out for gfortran.
C
C  The machine is a 60-bit word machine and this program keeps characters
C  in words, six bits each, in display code.  Everything below either
C  packs and unpacks those words (the R and A edit descriptors, the nR
C  and nH constants), or is a library routine the program calls:
C
C    SHIFT  MASK             the bit primitives (SHIFT left is CIRCULAR)
C    RANF RANSET SECOND      FORLIB's random number generator
C    GET PUT FILEWA OPENM    the record manager, on word addressable
C    CLOSEM                  files - here two arrays
C    EOF                     end of file on a unit
C
C  Everything is INTEGER*8 (the port is built -fdefault-integer-8) and a
C  word is the low 60 bits of one; PM60 masks it back down.
C ======================================================================

      BLOCK DATA PCDCBD
      IMPLICIT INTEGER (A-Z)
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      DATA PSEED/0/,PTIM/-1/,PFIX/1/,PEOFF/0/,PDCINI/0/,PSFIX/0/
      END

C ----------------------------------------------------------------------
C  Display code.  The 64 character set: 00 is ':' and 63 (octal 77) is
C  ';'.  Which of the two sets the site ran only matters for a word held
C  in memory, never for what is printed, because the port encodes and
C  decodes with the same table, and nothing in the program compares text
C  against a constant containing ':'.
C ----------------------------------------------------------------------
      SUBROUTINE PDCSET
      IMPLICIT INTEGER (A-Z)
      CHARACTER*64 TAB
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      COMMON /PCDCTB/ PENC(0:127),PDEC(0:63)
      IF(PDCINI.NE.0)RETURN
      PDCINI=1
      TAB=':ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/()$= ,.#[]%"_!&'
      TAB(57:61)='''?<>@'
      TAB(62:62)=CHAR(92)
      TAB(63:63)='^'
      TAB(64:64)=';'
      DO 1 I=0,127
1     PENC(I)=45
      DO 2 I=0,63
      PDEC(I)=ICHAR(TAB(I+1:I+1))
2     PENC(PDEC(I))=I
C  Lower case is folded up; the machine had no lower case at all.
      DO 3 I=ICHAR('a'),ICHAR('z')
3     PENC(I)=PENC(I-32)
      RETURN
      END

C  ASCII character -> display code
      INTEGER FUNCTION PDCC(C)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*1 C
      COMMON /PCDCTB/ PENC(0:127),PDEC(0:63)
      CALL PDCSET
      I=ICHAR(C)
      IF(I.LT.0.OR.I.GT.127)I=32
      PDCC=PENC(I)
      RETURN
      END

C  An nR constant, and the R edit descriptor: the characters of S right
C  justified in the word with binary zero fill.
      INTEGER FUNCTION PDCW(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CALL PDCSET
      PDCW=0
      DO 1 I=1,LEN(S)
1     PDCW=PDCW*64+PDCC(S(I:I))
      RETURN
      END

C  An nH (or nL) constant: left justified, blank filled to ten
C  characters - one whole word.
      INTEGER FUNCTION PDCL(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CHARACTER*10 B
      K=MIN(10,LEN(S))
      B=' '
      B(1:K)=S(1:K)
      PDCL=PDCW(B)
      RETURN
      END

C  The R edit descriptor on output: the rightmost N characters of W.
      SUBROUTINE PDCS(W,N,S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      COMMON /PCDCTB/ PENC(0:127),PDEC(0:63)
      CALL PDCSET
      V=W
      DO 1 I=N,1,-1
      S(I:I)=CHAR(PDEC(IAND(V,63)))
1     V=V/64
      RETURN
      END

      CHARACTER*4 FUNCTION PDC4(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*4 S
      CALL PDCS(W,4,S)
      PDC4=S
      RETURN
      END

      CHARACTER*9 FUNCTION PDC9(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*9 S
      CALL PDCS(W,9,S)
      PDC9=S
      RETURN
      END

      CHARACTER*10 FUNCTION PDC10(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*10 S
      CALL PDCS(W,10,S)
      PDC10=S
      RETURN
      END

C  FORMAT(1X,18R4) with L items in the list.
      SUBROUTINE PRTL(LINES,L)
      IMPLICIT INTEGER (A-Z)
      DIMENSION LINES(*)
      CHARACTER*72 S
      S=' '
      DO 1 I=1,L
1     CALL PDCS(LINES(I),4,S(4*I-3:4*I))
      K=4*L
2     IF(K.GT.1.AND.S(K:K).EQ.' ')THEN
         K=K-1
         GOTO 2
      ENDIF
      PRINT 3,S(1:K)
3     FORMAT(1X,A)
      RETURN
      END

C ----------------------------------------------------------------------
C  Reading.  Every READ in the program reads one record: from the
C  terminal (unit 5) or from the database (unit 1, TAPE1).
C ----------------------------------------------------------------------
      SUBROUTINE PRDLIN(IU,S,N)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      S=' '
      READ(IU,1,END=9,ERR=9)S
1     FORMAT(A)
      N=LEN(S)
2     IF(N.GT.0.AND.S(N:N).EQ.' ')THEN
         N=N-1
         GOTO 2
      ENDIF
      RETURN
C  The program answers end of file by reading again, which at a terminal
C  means "ignore it".  Piped input really does end, so the port stops.
9     PEOFF=1
      IF(IU.EQ.5)STOP
      N=0
      RETURN
      END

      INTEGER FUNCTION EOF(IU)
      IMPLICIT INTEGER (A-Z)
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      EOF=PEOFF
      PEOFF=0
      RETURN
      END

C  FORMAT(An): N characters left justified, blank filled.
      SUBROUTINE PRDA(W,N)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*150 S
      CHARACTER*10 B
      CALL PRDLIN(5,S,K)
      B=' '
      DO 1 I=1,MIN(N,10)
1     B(I:I)=S(I:I)
      W=PDCW(B)
      RETURN
      END

C  FORMAT(nR1): N words, one character each, right justified.
      SUBROUTINE PRDR1(W,N)
      IMPLICIT INTEGER (A-Z)
      DIMENSION W(*)
      CHARACTER*150 S
      CALL PRDLIN(5,S,K)
      DO 1 I=1,N
1     W(I)=PDCC(S(I:I))
      RETURN
      END

C  FORMAT(I4,18R4) - a line of the message sections of the database.
      SUBROUTINE PRDTXT(LOC,LINES)
      IMPLICIT INTEGER (A-Z)
      DIMENSION LINES(18)
      CHARACTER*150 S
      CHARACTER*4 F
      CALL PRDLIN(1,S,K)
      LOC=PINT(S(1:4))
      DO 1 I=1,18
      F=S(4*I+1:4*I+4)
1     LINES(I)=PDCW(F)
      RETURN
      END

C  FORMAT(I6,R4) - a line of the vocabulary.
      SUBROUTINE PRDVOC(K,A)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*150 S
      CHARACTER*4 F
      CALL PRDLIN(1,S,N)
      K=PINT(S(1:6))
      F=S(7:10)
      A=PDCW(F)
      RETURN
      END

C  An integer field, with blanks read as zeros as the CDC read them.
      INTEGER FUNCTION PINT(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      V=0
      SGN=1
      DO 1 I=1,LEN(S)
      C=ICHAR(S(I:I))
      IF(C.EQ.ICHAR('-'))SGN=-1
      IF(C.GE.ICHAR('0').AND.C.LE.ICHAR('9'))V=V*10+(C-ICHAR('0'))
1     CONTINUE
      PINT=SGN*V
      RETURN
      END

C ----------------------------------------------------------------------
C  The bit primitives.  SHIFT left is end around; SHIFT right keeps the
C  sign bit (bit 59), which is why the program clears it by hand.
C  MASK(N) is the N leftmost bits of the word.
C ----------------------------------------------------------------------
      INTEGER FUNCTION SHIFT(X,N)
      IMPLICIT INTEGER (A-Z)
      PM60=1152921504606846975
      V=IAND(X,PM60)
      IF(N.EQ.0)THEN
         SHIFT=V
      ELSE IF(N.GT.0)THEN
         K=MOD(N,60)
         SHIFT=IAND(IOR(ISHFT(V,K),ISHFT(V,K-60)),PM60)
      ELSE
         K=MIN(-N,60)
         IF(IAND(V,576460752303423488).NE.0)V=IOR(V,NOT(PM60))
         SHIFT=IAND(SHIFTA(V,K),PM60)
      ENDIF
      RETURN
      END

      INTEGER FUNCTION MASK(N)
      IMPLICIT INTEGER (A-Z)
      PM60=1152921504606846975
      IF(N.LE.0)THEN
         MASK=0
      ELSE IF(N.GE.60)THEN
         MASK=PM60
      ELSE
         MASK=PM60-(ISHFT(1,60-N)-1)
      ENDIF
      RETURN
      END

C  .NOT. of a word - the complement inside 60 bits.
      INTEGER FUNCTION PNOT(X)
      IMPLICIT INTEGER (A-Z)
      PM60=1152921504606846975
      PNOT=IAND(NOT(X),PM60)
      RETURN
      END

C ----------------------------------------------------------------------
C  FORLIB's random number generator, as measured on NOS 1.3 (FTN 4.7,
C  the probes are in ..\..\tests\cyber):
C
C    seed' = seed * 44485709377909 (octal 1207264271730565) mod 2**48
C    RANF  = seed' / 2**48
C
C  and the seed the library starts with is 48131768981101 (octal
C  1274321477413155) - a constant, the same in every job.  RANSET takes
C  the 48 bit coefficient of its argument normalised as a fraction and
C  forces bit 0 on; an argument of zero leaves the seed alone.  That was
C  read back with RANGET for a dozen arguments.
C ----------------------------------------------------------------------
      SUBROUTINE PRNINI
      IMPLICIT INTEGER (A-Z)
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      IF(PSEED.EQ.0)PSEED=48131768981101
      RETURN
      END

      DOUBLE PRECISION FUNCTION RANF(X)
      IMPLICIT INTEGER (A-Z)
      REAL X
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      CALL PRNINI
      PSEED=IAND(PSEED*44485709377909,281474976710655)
      RANF=DBLE(PSEED)/281474976710656.0D0
      RETURN
      END

      SUBROUTINE RANSET(X)
      IMPLICIT INTEGER (A-Z)
      DOUBLE PRECISION X,F
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      CALL PRNINI
C  A seed given with --seed is the one the player asked for, so the
C  game's own CALL RANSET(SECOND(1.)) must not overwrite it.
      IF(PSFIX.NE.0)RETURN
      IF(X.EQ.0.0D0)RETURN
      F=FRACTION(ABS(X))
      PSEED=IOR(IAND(INT(F*281474976710656.0D0+0.5D0),
     +               281474976710655),1)
      RETURN
      END

C  SECOND is the job's CP seconds - a 48 bit float on the Cyber, so it
C  is double here: RANSET(SECOND(1.)) is how the game seeds itself and
C  every bit of it counts.
      DOUBLE PRECISION FUNCTION SECOND(X)
      IMPLICIT INTEGER (A-Z)
      REAL X
      INTEGER C,R
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      CALL SYSTEM_CLOCK(C,R)
      IF(R.LE.0)R=1
      SECOND=DBLE(MOD(C,R*1000))/DBLE(R)
      RETURN
      END

C ----------------------------------------------------------------------
C  TIME(1.) gives the time of day as one word of display code,
C  " hh.mm.ss.".  The program decodes it straight back into an integer
C  and clears the leading blank, so the port hands over the word.
C ----------------------------------------------------------------------
      INTEGER FUNCTION PTIMEW()
      IMPLICIT INTEGER (A-Z)
      CHARACTER*10 S
      DIMENSION V(8)
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      IF(PTIM.GE.0)THEN
         HH=PTIM/3600
         MM=MOD(PTIM/60,60)
         SS=MOD(PTIM,60)
      ELSE
         CALL DATE_AND_TIME(VALUES=V)
         HH=V(5)
         MM=V(6)
         SS=V(7)
      ENDIF
      WRITE(S,1)HH,MM,SS
1     FORMAT(' ',I2.2,'.',I2.2,'.',I2.2,'.')
      PTIMEW=PDCW(S)
      RETURN
      END

C ----------------------------------------------------------------------
C  Word addressable files.  TAPE2 holds the message text, nineteen words
C  to a record - the message number and eighteen words of four
C  characters - and TAPE3 one word per line, the record length and the
C  word address in TAPE2 packed together.  The length GET and PUT are
C  given is the record length in characters, which is what the program's
C  MRL is, so a word count is a tenth of it.
C ----------------------------------------------------------------------
      SUBROUTINE PFILEW(FIT,SLOT,MRL)
      IMPLICIT INTEGER (A-Z)
      DIMENSION FIT(*)
      FIT(1)=SLOT
      FIT(2)=MRL
      RETURN
      END

      SUBROUTINE POPENM(FIT,MODE)
      IMPLICIT INTEGER (A-Z)
      DIMENSION FIT(*)
      RETURN
      END

      SUBROUTINE PCLOSM(FIT)
      IMPLICIT INTEGER (A-Z)
      DIMENSION FIT(*)
      RETURN
      END

      SUBROUTINE PWACHK(SLOT,IDX,N)
      IMPLICIT INTEGER (A-Z)
      LIM=0
      IF(SLOT.EQ.1)LIM=40000
      IF(SLOT.EQ.2)LIM=2300
      IF(LIM.EQ.0)GOTO 9
      IF(IDX.LT.1.OR.IDX+N-1.GT.LIM)GOTO 9
      RETURN
9     PRINT 1,SLOT,IDX,N
1     FORMAT(' PORT: word addressable file ',I2,' word ',I8,
     +       ' for ',I6,' words is out of range')
      STOP
      END

      SUBROUTINE PGET(FIT,WSA,IDX,LEN)
      IMPLICIT INTEGER (A-Z)
      DIMENSION FIT(*),WSA(*)
      COMMON /PWAFIL/ PWA2(40000),PWA3(2300)
      N=LEN/10
      CALL PWACHK(FIT(1),IDX,N)
      IF(FIT(1).EQ.1)THEN
         DO 1 I=1,N
1        WSA(I)=PWA2(IDX+I-1)
      ELSE
         DO 2 I=1,N
2        WSA(I)=PWA3(IDX+I-1)
      ENDIF
      RETURN
      END

      SUBROUTINE PPUT(FIT,WSA,LEN,IDX)
      IMPLICIT INTEGER (A-Z)
      DIMENSION FIT(*),WSA(*)
      COMMON /PWAFIL/ PWA2(40000),PWA3(2300)
      N=LEN/10
      CALL PWACHK(FIT(1),IDX,N)
      IF(FIT(1).EQ.1)THEN
         DO 1 I=1,N
1        PWA2(IDX+I-1)=WSA(I)
      ELSE
         DO 2 I=1,N
2        PWA3(IDX+I-1)=WSA(I)
      ENDIF
      RETURN
      END

C ----------------------------------------------------------------------
C  Getting started: the options, then the database on unit 1 (TAPE1).
C ----------------------------------------------------------------------
      SUBROUTINE PIOINI
      IMPLICIT INTEGER (A-Z)
      CHARACTER*260 D
      CALL PGMDIR(D)
      K=260
1     IF(K.GT.0.AND.D(K:K).EQ.' ')THEN
         K=K-1
         GOTO 1
      ENDIF
      OPEN(1,FILE=D(1:K)//'adventure.txt',STATUS='OLD',ERR=9)
      RETURN
9     PRINT 2,D(1:K)
2     FORMAT(' PORT: cannot open adventure.txt in [',A,']')
      STOP
      END

      SUBROUTINE POPTS
      IMPLICIT INTEGER (A-Z)
      CHARACTER*80 A
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI,PSFIX
      N=COMMAND_ARGUMENT_COUNT()
      I=1
1     IF(I.GT.N)RETURN
      CALL GET_COMMAND_ARGUMENT(I,A)
      IF(A(1:6).EQ.'--seed')THEN
         I=I+1
         CALL GET_COMMAND_ARGUMENT(I,A)
         PSEED=IAND(PINT(A),281474976710655)
         IF(MOD(PSEED,2).EQ.0)PSEED=PSEED+1
         PSFIX=1
      ELSE IF(A(1:6).EQ.'--time')THEN
         I=I+1
         CALL GET_COMMAND_ARGUMENT(I,A)
         K=PINT(A)
         PTIM=(K/100)*3600+MOD(K,100)*60
      ELSE IF(A(1:10).EQ.'--no-fixes')THEN
         PFIX=0
      ELSE IF(A(1:2).EQ.'-h'.OR.A(1:6).EQ.'--help')THEN
         CALL PUSAGE
      ELSE
         PRINT 2,A(1:20)
2        FORMAT(' PORT: unknown option ',A)
         CALL PUSAGE
      ENDIF
      I=I+1
      GOTO 1
      END

      SUBROUTINE PUSAGE
      PRINT 1
1     FORMAT(
     +' adventure [--seed N] [--time HHMM] [--no-fixes] [-h]'//
     +' Colossal Cave Adventure, 366 points: the version with the'/
     +' gazebo and the palantir that Bill Hein and Shelley Hobson'/
     +' brought up under NOS 1.3 at ACCA, from Blackett and Supnik''s'/
     +' FORTRAN IV.'//
     +'   --seed N     the 48 bit seed to start the generator at,'/
     +'                instead of RANSET(SECOND(1.)) from the clock'/
     +'   --time HHMM  hold the clock still.  The game refuses to run'/
     +'                06.00-11.30 and 13.30-15.30 unless you are the'/
     +'                wizard, which is what the machine did.'/
     +'   --no-fixes   turn off this port''s fixes (there are none)'/)
      STOP
      END
