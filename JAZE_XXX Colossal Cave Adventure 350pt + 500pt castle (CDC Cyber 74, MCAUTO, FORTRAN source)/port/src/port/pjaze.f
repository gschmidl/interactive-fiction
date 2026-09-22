C ======================================================================
C  PJAZE - what the CDC Cyber 74 and its FORTRAN Extended did for this
C  program, written out for gfortran.  A word is 60 bits and holds ten
C  display code characters; everything below either packs and unpacks
C  those words or is a library routine the program calls:
C
C    SHIFT MASK OR AND XOR COMPL   the bit primitives
C    JDATE CLOCK                   the clock, as display code words
C    OPENMS READMS WRITMS CLOSMS   the random access message file
C    PFGET                         attaching a permanent file
C    BUFFER IN/OUT, UNIT           the wizard's parameter file
C    ERRSET                        error trapping (nothing to do here)
C
C  All of it was measured on a Cyber 173 under NOS 1.3 (FTN 4.7) - the
C  probes are in ..\..\tests\cyber.  The machine is ones' complement, so
C  negating a word complements it; the one place that matters is A5TOA1.
C
C  Everything is INTEGER*8 (built -fdefault-integer-8) and a word is the
C  low 60 bits.
C ======================================================================

      BLOCK DATA PJZBD
      IMPLICIT INTEGER (A-Z)
      COMMON /PJZCOM/ PDAY,PTIM,PFIX,PDCINI
      DATA PDAY/-1/,PTIM/-1/,PFIX/1/,PDCINI/0/
      END

C ----------------------------------------------------------------------
C  Display code, the 64 character set: 00 is ':' and 77 octal is ';'.
C ----------------------------------------------------------------------
      SUBROUTINE PDCSET
      IMPLICIT INTEGER (A-Z)
      CHARACTER*64 TAB
      COMMON /PJZCOM/ PDAY,PTIM,PFIX,PDCINI
      COMMON /PJZTAB/ PENC(0:127),PDEC(0:63)
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
      DO 3 I=ICHAR('a'),ICHAR('z')
3     PENC(I)=PENC(I-32)
      RETURN
      END

      INTEGER FUNCTION PDCC(C)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*1 C
      COMMON /PJZTAB/ PENC(0:127),PDEC(0:63)
      CALL PDCSET
      I=ICHAR(C)
      IF(I.LT.0.OR.I.GT.127)I=32
      PDCC=PENC(I)
      RETURN
      END

C  A quoted or nH constant, and the A edit descriptor: left justified,
C  blank filled to ten characters.  "BEG" and 3HBEG are the same word.
      INTEGER FUNCTION PDCL(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CHARACTER*10 B
      CALL PDCSET
      K=MIN(10,LEN(S))
      B=' '
      B(1:K)=S(1:K)
      PDCL=0
      DO 1 I=1,10
1     PDCL=PDCL*64+PDCC(B(I:I))
      RETURN
      END

C  An nR constant: right justified, binary zero fill.
      INTEGER FUNCTION PDCW(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CALL PDCSET
      PDCW=0
      DO 1 I=1,LEN(S)
1     PDCW=PDCW*64+PDCC(S(I:I))
      RETURN
      END

C  A word as characters: PDCS gives all ten, left to right, which is
C  what An prints for n up to 10 (it takes the LEFTmost n).
      SUBROUTINE PDCS(W,S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CHARACTER*10 B
      COMMON /PJZTAB/ PENC(0:127),PDEC(0:63)
      CALL PDCSET
      V=W
      DO 1 I=10,1,-1
      B(I:I)=CHAR(PDEC(IAND(V,63)))
1     V=V/64
      S=B(1:MIN(10,LEN(S)))
      RETURN
      END

      CHARACTER*4 FUNCTION PDCA4(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*4 S
      CALL PDCS(W,S)
      PDCA4=S
      RETURN
      END

      CHARACTER*5 FUNCTION PDCA5(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*5 S
      CALL PDCS(W,S)
      PDCA5=S
      RETURN
      END

      CHARACTER*10 FUNCTION PDCA10(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*10 S
      CALL PDCS(W,S)
      PDCA10=S
      RETURN
      END

C  FORMAT(" ",8A10) with L items in the list - SPEAK's line printer.
      SUBROUTINE PRTL8(LINES,L)
      IMPLICIT INTEGER (A-Z)
      DIMENSION LINES(*)
      CHARACTER*80 S
      CHARACTER*10 W
      S=' '
      DO 1 I=1,L
      CALL PDCS(LINES(I),W)
1     S(10*I-9:10*I)=W
      K=10*L
2     IF(K.GT.1.AND.S(K:K).EQ.' ')THEN
         K=K-1
         GOTO 2
      ENDIF
      PRINT 3,S(1:K)
3     FORMAT(' ',A)
      RETURN
      END

C ----------------------------------------------------------------------
C  Reading.  Unit 1 is the database (a permanent file the program
C  attaches with PFGET); the terminal is INPUT.
C ----------------------------------------------------------------------
      SUBROUTINE PRDLIN(IU,S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      S=' '
      READ(IU,1,END=9,ERR=9)S
1     FORMAT(A)
      RETURN
9     IF(IU.EQ.5)STOP
      PRINT 2,IU
2     FORMAT(' PORT: end of file on unit ',I3)
      STOP
      END

C  FORMAT(20R1) from the terminal: twenty words, one character each,
C  right justified.
      SUBROUTINE PRDR1(W,N)
      IMPLICIT INTEGER (A-Z)
      DIMENSION W(*)
      CHARACTER*150 S
      CALL PRDLIN(5,S)
      DO 1 I=1,N
1     W(I)=PDCC(S(I:I))
      RETURN
      END

C  FORMAT(I2) and FORMAT(I3) from the terminal (the wizard's dialogue).
      SUBROUTINE PRDINT(V,N)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*150 S
      CALL PRDLIN(5,S)
      V=PINT(S(1:N))
      RETURN
      END

C  FORMAT(2A10) from the terminal: two words of ten characters.
      SUBROUTINE PRDA10(W,N)
      IMPLICIT INTEGER (A-Z)
      DIMENSION W(*)
      CHARACTER*150 S
      CALL PRDLIN(5,S)
      DO 1 I=1,N
1     W(I)=PDCL(S(10*I-9:10*I))
      RETURN
      END

C  FORMAT(I3,X,8A10) - a line of the message sections of the database.
      SUBROUTINE PRDTXT(LOC,LINES)
      IMPLICIT INTEGER (A-Z)
      DIMENSION LINES(8)
      CHARACTER*150 S
      CALL PRDLIN(1,S)
      LOC=PINT(S(1:3))
      DO 1 I=1,8
1     LINES(I)=PDCL(S(10*I-5:10*I+4))
      RETURN
      END

C  FORMAT(I4,1X,A4,1X) - a line of the vocabulary.
      SUBROUTINE PRDVOC(K,A)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*150 S
      CALL PRDLIN(1,S)
      K=PINT(S(1:4))
      A=PDCL(S(6:9))
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
C  The bit primitives.  SHIFT left is end around; MASK(n) is the n
C  leftmost bits; PNOT is the ones' complement, which on this machine is
C  also what unary minus does to a word.
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

      INTEGER FUNCTION PNOT(X)
      IMPLICIT INTEGER (A-Z)
      PM60=1152921504606846975
      PNOT=IAND(NOT(X),PM60)
      RETURN
      END

      INTEGER FUNCTION OR(A,B)
      IMPLICIT INTEGER (A-Z)
      OR=IOR(A,B)
      RETURN
      END

      INTEGER FUNCTION XOR(A,B)
      IMPLICIT INTEGER (A-Z)
      XOR=IEOR(A,B)
      RETURN
      END

      INTEGER FUNCTION COMPL(A)
      IMPLICIT INTEGER (A-Z)
      COMPL=PNOT(A)
      RETURN
      END

C ----------------------------------------------------------------------
C  The clock.  JDATE gives the Julian date as a word of display code -
C  five characters of binary zero, then yyddd - and CLOCK gives
C  " hh.mm.ss.".  DATIME takes them apart with SHIFT and DECODE.
C ----------------------------------------------------------------------
      SUBROUTINE PDATE(DAY,TIM)
      IMPLICIT INTEGER (A-Z)
      DIMENSION V(8)
      COMMON /PJZCOM/ PDAY,PTIM,PFIX,PDCINI
      DIMENSION MD(12)
      DATA MD/0,31,59,90,120,151,181,212,243,273,304,334/
      IF(PDAY.GE.0)THEN
         DAY=PDAY
      ELSE
         CALL DATE_AND_TIME(VALUES=V)
         DAY=MD(V(2))+V(3)
         IF(V(2).GT.2.AND.MOD(V(1),4).EQ.0)DAY=DAY+1
      ENDIF
      IF(PTIM.GE.0)THEN
         TIM=PTIM
      ELSE
         CALL DATE_AND_TIME(VALUES=V)
         TIM=V(5)*3600+V(6)*60+V(7)
      ENDIF
      RETURN
      END

      SUBROUTINE JDATE(J)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*5 S
      DIMENSION V(8)
      COMMON /PJZCOM/ PDAY,PTIM,PFIX,PDCINI
      CALL PDATE(DAY,TIM)
      CALL DATE_AND_TIME(VALUES=V)
      YR=MOD(V(1),100)
      WRITE(S,1)YR,MOD(DAY,1000)
1     FORMAT(I2.2,I3.3)
C  Five characters of binary zero, then the year and the day of year.
      J=PDCW(S)
      RETURN
      END

      SUBROUTINE CLOCK(K)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*10 S
      CALL PDATE(DAY,TIM)
      WRITE(S,1)TIM/3600,MOD(TIM/60,60),MOD(TIM,60)
1     FORMAT(' ',I2.2,'.',I2.2,'.',I2.2,'.')
      K=PDCW(S)
      RETURN
      END

C ----------------------------------------------------------------------
C  The random access message file (unit 2): OPENMS with an index array,
C  then WRITMS and READMS by record number.  Here it is an array, 1050
C  records of nine words - the message number and eight words of ten
C  characters.
C ----------------------------------------------------------------------
      SUBROUTINE OPENMS(IUN,INDEX,N,ITYPE)
      IMPLICIT INTEGER (A-Z)
      DIMENSION INDEX(*)
      RETURN
      END

      SUBROUTINE CLOSMS(IUN)
      IMPLICIT INTEGER (A-Z)
      RETURN
      END

      SUBROUTINE PMSCHK(IREC)
      IMPLICIT INTEGER (A-Z)
      IF(IREC.GE.1.AND.IREC.LE.1100)RETURN
      PRINT 1,IREC
1     FORMAT(' PORT: message file record ',I8,' is out of range')
      STOP
      END

      SUBROUTINE WRITMS(IUN,BUF,N,IREC)
      IMPLICIT INTEGER (A-Z)
      DIMENSION BUF(*)
      COMMON /PMSFIL/ PMS(9,1100)
      CALL PMSCHK(IREC)
      DO 1 I=1,MIN(N,9)
1     PMS(I,IREC)=BUF(I)
      RETURN
      END

      SUBROUTINE READMS(IUN,BUF,N,IREC)
      IMPLICIT INTEGER (A-Z)
      DIMENSION BUF(*)
      COMMON /PMSFIL/ PMS(9,1100)
      CALL PMSCHK(IREC)
      DO 1 I=1,MIN(N,9)
1     BUF(I)=PMS(I,IREC)
      RETURN
      END

C ----------------------------------------------------------------------
C  PFGET attached a permanent file to a unit.  Unit 1 is the database
C  the player chose, unit 3 the wizard's parameters, which the program
C  reads and writes with BUFFER IN and BUFFER OUT.
C ----------------------------------------------------------------------
      SUBROUTINE PPFGET(IUN,NAME)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) NAME
      CHARACTER*260 D
      CHARACTER*300 P
      COMMON /PJZFIL/ PAMNT
      CHARACTER*300 PAMNT
      CALL PGMDIR(D)
      K=260
1     IF(K.GT.0.AND.D(K:K).EQ.' ')THEN
         K=K-1
         GOTO 1
      ENDIF
      P=D(1:K)//NAME
      IF(IUN.EQ.3)THEN
         PAMNT=P
         RETURN
      ENDIF
      OPEN(IUN,FILE=P,STATUS='OLD',ERR=9)
      RETURN
9     PRINT 2,P(1:60)
2     FORMAT(' PORT: cannot open ',A)
      STOP
      END

C  BUFFER IN / BUFFER OUT of N words, and UNIT, which the program tests
C  with an arithmetic IF: negative means the transfer was complete.
      SUBROUTINE PBUFIN(IUN,BUF,N)
      IMPLICIT INTEGER (A-Z)
      DIMENSION BUF(*)
      COMMON /PJZFIL/ PAMNT
      CHARACTER*300 PAMNT
      COMMON /PJZUNT/ PUST
      PUST=1
      OPEN(9,FILE=PAMNT,STATUS='OLD',FORM='UNFORMATTED',
     +     ACCESS='STREAM',ERR=9)
      READ(9,ERR=8,END=8)(BUF(I),I=1,N)
      CLOSE(9)
      PUST=-1
      RETURN
8     CLOSE(9)
9     RETURN
      END

      SUBROUTINE PBUFOT(IUN,BUF,N)
      IMPLICIT INTEGER (A-Z)
      DIMENSION BUF(*)
      COMMON /PJZFIL/ PAMNT
      CHARACTER*300 PAMNT
      COMMON /PJZUNT/ PUST
      PUST=1
      OPEN(9,FILE=PAMNT,STATUS='UNKNOWN',FORM='UNFORMATTED',
     +     ACCESS='STREAM',ERR=9)
      WRITE(9,ERR=8)(BUF(I),I=1,N)
      CLOSE(9)
      PUST=-1
      RETURN
8     CLOSE(9)
9     RETURN
      END

      INTEGER FUNCTION PUNIT(IUN)
      IMPLICIT INTEGER (A-Z)
      COMMON /PJZUNT/ PUST
      PUNIT=PUST
      RETURN
      END

      SUBROUTINE ERRSET(N,A,B,C,D,E)
      IMPLICIT INTEGER (A-Z)
      LOGICAL A,B,C,D
      RETURN
      END

C ----------------------------------------------------------------------
C  Options
C ----------------------------------------------------------------------
      SUBROUTINE POPTS
      IMPLICIT INTEGER (A-Z)
      CHARACTER*80 A
      COMMON /PJZCOM/ PDAY,PTIM,PFIX,PDCINI
      N=COMMAND_ARGUMENT_COUNT()
      I=1
1     IF(I.GT.N)RETURN
      CALL GET_COMMAND_ARGUMENT(I,A)
      IF(A(1:5).EQ.'--day')THEN
         I=I+1
         CALL GET_COMMAND_ARGUMENT(I,A)
         PDAY=PINT(A)
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
     +' advent [--day N] [--time HHMM] [--no-fixes] [-h]'//
     +' Colossal Cave Adventure as it ran on the MCAUTO Cyber 74:'/
     +' answer BEG for the 350 point cave or ADV for the 500 point'/
     +' cave with the castle and the Black Wizard.'//
     +'   --day N      hold the day of year still (1-366).  The'/
     +'                random number generator is seeded from the'/
     +'                clock, so --day with --time makes a run'/
     +'                reproducible.'/
     +'   --time HHMM  hold the time of day still.  Prime time is'/
     +'                08.00-17.59 on weekdays, when only a wizard'/
     +'                may play a full game.'/
     +'   --no-fixes   turn off this port''s fixes'/)
      STOP
      END

C  DECODE(n,fmt,word) of an integer: the leftmost N characters of the word
C  read as a number, which is how DATIME takes JDATE and CLOCK apart.
      SUBROUTINE PDECI(W,N,V)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*10 S
      CALL PDCS(W,S)
      V=PINT(S(1:N))
      RETURN
      END

      CHARACTER*2 FUNCTION PDCA2(W)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*2 S
      CALL PDCS(W,S)
      PDCA2=S
      RETURN
      END
