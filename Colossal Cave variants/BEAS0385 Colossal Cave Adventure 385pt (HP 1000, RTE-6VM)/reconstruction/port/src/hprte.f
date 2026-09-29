C=======================================================================
C  hprte.f -- stand-ins for the HP 1000 RTE-6/VM facilities that this
C  Adventure calls.  None of this is translated game code; it replaces
C  three things that only exist on the machine:
C
C    * the HP "Q-string" package (ADVG4.FTN plus the ADV14/24/34/44/54
C      assembly primitives).  A Q-string is an INTEGER array whose first
C      word is a character count and whose characters run from the first
C      byte of the second word onwards, two per word.  Everything here
C      indexes those characters in memory order, which is also what
C      gfortran's A2 edit descriptor uses, so the string package and the
C      formatted I/O around it agree on any host.
C
C    * the FMP file interface (FmpOpen/Read/Write/SetPosition/Close/Purge).
C      #ADVZZ is read as an ordinary text file, one line per record;
C      #ADVXX -- which the program creates and then reads at random -- is a
C      direct-access file of 88-byte records, matching the ":::2:344:44"
C      type-2, 44-word-record file the original asks RTE for.
C
C    * the executive calls RMPAR, EXEC, SEGRT and SUSP, and the FORLIB
C      random number generator RANDM.
C
C  FMPREAD deliberately transfers only as many characters as the caller
C  asks for, exactly as FMP does.  Several calls in ADV03 ask for fewer
C  than a record holds; see the README.
C=======================================================================

C-----------------------------------------------------------------------
C  Text extraction.
C
C  The original pulls text out of a record with an A2 edit descriptor and
C  an INTEGER destination -- FORMAT(I5,1X,40A2) and friends.  gfortran
C  reads A into a non-CHARACTER item the way it reads a numeric field, so
C  a comma inside the text ends the field and the rest is blank filled;
C  location 3's "a building, a well house" comes out with the comma gone.
C  These two do the plain character copy the HP compiler does.
C-----------------------------------------------------------------------

      SUBROUTINE A2GET(SRC, IPOS, DEST, NWORDS)
C
C  Copy 2*NWORDS characters from SRC starting at column IPOS into DEST,
C  which holds two characters per 16-bit word.  Short source is padded
C  with blanks, as an A edit descriptor would.
C
      IMPLICIT NONE
      CHARACTER*(*) SRC
      INTEGER IPOS, NWORDS
      INTEGER*2 DEST(*)
      CALL A2GET1(SRC, IPOS, DEST, NWORDS)
      RETURN
      END

      SUBROUTINE A2GET1(SRC, IPOS, CD, NWORDS)
      IMPLICIT NONE
      CHARACTER*(*) SRC
      CHARACTER*1 CD(*)
      INTEGER IPOS, NWORDS, I, K
      DO I = 1, 2 * NWORDS
        K = IPOS + I - 1
        IF (K .GE. 1 .AND. K .LE. LEN(SRC)) THEN
          CD(I) = SRC(K:K)
        ELSE
          CD(I) = ' '
        ENDIF
      ENDDO
      RETURN
      END

      SUBROUTINE CASEUP(BUFFER)
C
C  The HP console runs in upper case (SIMH's "set TTY0 UC"), so the game
C  never folds what it reads.  A pipe does not, so the port folds here.
C
      IMPLICIT NONE
      CHARACTER*(*) BUFFER
      INTEGER I, C
      DO I = 1, LEN(BUFFER)
        C = ICHAR(BUFFER(I:I))
        IF (C .GE. 97 .AND. C .LE. 122) BUFFER(I:I) = CHAR(C - 32)
      ENDDO
      RETURN
      END

C-----------------------------------------------------------------------
C  Q-string primitives.  CB is the character view of a Q-string body,
C  reached by handing XQ(2) to a CHARACTER*1 dummy.
C-----------------------------------------------------------------------

      SUBROUTINE PUTQ(XQ, I, IASC)
C
C  Put the character with ASCII value IASC into position I of XQ.
C  Neither checks nor changes the string length.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), I, IASC
      CALL PUTQ1(XQ(2), I, IASC)
      RETURN
      END

      SUBROUTINE PUTQ1(CB, I, IASC)
      IMPLICIT NONE
      CHARACTER*1 CB(*)
      INTEGER*2 I, IASC
      CB(I) = CHAR(IAND(INT(IASC), 255))
      RETURN
      END

      INTEGER*2 FUNCTION JASCQ(XQ, I)
C
C  ASCII value of the I-th character of XQ.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), I, JASCQ1
      JASCQ = JASCQ1(XQ(2), I)
      RETURN
      END

      INTEGER*2 FUNCTION JASCQ1(CB, I)
      IMPLICIT NONE
      CHARACTER*1 CB(*)
      INTEGER*2 I
      JASCQ1 = ICHAR(CB(I))
      RETURN
      END

      SUBROUTINE MOVQQ(XQ, I, YQ, J, N)
C
C  Move N characters from XQ starting at character I to YQ starting at
C  character J, left to right.  Checks nothing.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), YQ(*), I, J, N
      CALL MOVQQ1(XQ(2), I, YQ(2), J, N)
      RETURN
      END

      SUBROUTINE MOVQQ1(CX, I, CY, J, N)
      IMPLICIT NONE
      CHARACTER*1 CX(*), CY(*)
      INTEGER*2 I, J, N
      INTEGER K
      DO K = 0, N - 1
        CY(J + K) = CX(I + K)
      ENDDO
      RETURN
      END

      SUBROUTINE MOVEQ(XQ, I, YQ, J, N)
C
C  As MOVQQ, but valid when the two strings overlap: the original works
C  out the direction from the addresses, which is what LOC does here.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), YQ(*), I, J, N
      INTEGER*2 JASCQ
      INTEGER*8 XLOC, YLOC, IDIF
      INTEGER K
      IF (N .LE. 0) RETURN
      XLOC = LOC(XQ) + I
      YLOC = LOC(YQ) + J
      IDIF = YLOC - XLOC
      IF (IDIF .EQ. 0) RETURN
      IF (IDIF .GT. 0 .AND. IDIF .LT. N) THEN
C       Overlapping and moving right: copy from the right-hand end.
        DO K = N - 1, 0, -1
          CALL PUTQ(YQ, INT(J + K, 2), JASCQ(XQ, INT(I + K, 2)))
        ENDDO
      ELSE
        CALL MOVQQ(XQ, I, YQ, J, N)
      ENDIF
      RETURN
      END

      SUBROUTINE CLRQ(XQ, I, J)
C
C  Put spaces in positions I through J inclusive.  Neither checks nor
C  changes the string length.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), I, J
      INTEGER K
      IF (I .GT. J) RETURN
      DO K = I, J
        CALL PUTQ(XQ, INT(K, 2), 32_2)
      ENDDO
      RETURN
      END

      INTEGER*2 FUNCTION IPOSQ(XQ, YQ, Z)
C
C  Position of the first occurrence of YQ in XQ, searching from the Z-th
C  character of XQ.  0 if not found or if XQ is null; Z if YQ is null;
C  Z <= 0 is read as 1; Z > LEN(XQ) gives 0.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), YQ(*), Z
      INTEGER*2 JASCQ
      INTEGER LENX, LENY, START, I, J
      IPOSQ = 0
      LENX = XQ(1)
      LENY = YQ(1)
      START = Z
      IF (START .LE. 0) START = 1
      IF (LENX .LE. 0) RETURN
      IF (LENY .LE. 0) THEN
        IPOSQ = START
        RETURN
      ENDIF
      IF (START .GT. LENX) RETURN
      DO I = START, LENX - LENY + 1
        DO J = 1, LENY
          IF (JASCQ(XQ, INT(I + J - 1, 2)) .NE. JASCQ(YQ, INT(J, 2)))
     &        GO TO 10
        ENDDO
        IPOSQ = I
        RETURN
   10   CONTINUE
      ENDDO
      RETURN
      END

      SUBROUTINE DLETQ(XQ, I, N)
C
C  Delete N characters from XQ starting at character I.  Unchanged if
C  I < 1, I > LEN(XQ) or N <= 0; truncates if fewer than N remain.
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), I, N
      INTEGER*2 I1, NMOVE
      IF (I .LT. 1 .OR. I .GT. XQ(1) .OR. N .LE. 0) RETURN
      IF (I + N .GT. XQ(1)) THEN
        XQ(1) = I - 1
        RETURN
      ENDIF
      I1 = I + N
      NMOVE = XQ(1) - I1 + 1
      CALL MOVEQ(XQ, I1, XQ, I, NMOVE)
      XQ(1) = XQ(1) - N
      RETURN
      END

      SUBROUTINE NSRTQ(XQ, I, YQ)
C
C  Insert YQ into XQ after character I of XQ, i.e.
C     1 <= I <= LEN(XQ)   XQ = XQ(1:I) // YQ // XQ(I+1:)
C     I = 0               XQ = YQ // XQ
C     I < 0               XQ = YQ // (-I spaces) // XQ
C     I >= LEN(XQ)        XQ = XQ // (I-LEN(XQ) spaces) // YQ
C
      IMPLICIT NONE
      INTEGER*2 XQ(*), I, YQ(*)
      INTEGER*2 LENYQ
      LENYQ = YQ(1)
      IF (LENYQ .LT. 0) LENYQ = 0
      IF (I .GE. XQ(1)) THEN
C       Only a suffix has to be added; pad with spaces first.
        CALL CLRQ(XQ, INT(XQ(1) + 1, 2), I)
        CALL MOVEQ(YQ, 1_2, XQ, INT(I + 1, 2), LENYQ)
        XQ(1) = I + LENYQ
        RETURN
      ENDIF
      IF (I .GT. 0) THEN
C       Split XQ and insert: shift the right-hand part out of the way.
        CALL MOVEQ(XQ, INT(I + 1, 2), XQ, INT(I + 1 + LENYQ, 2),
     &             INT(XQ(1) - I, 2))
        CALL MOVEQ(YQ, 1_2, XQ, INT(I + 1, 2), LENYQ)
        XQ(1) = XQ(1) + LENYQ
        RETURN
      ENDIF
C     I <= 0: move XQ right intact, drop YQ in front, pad between.
      CALL MOVEQ(XQ, 1_2, XQ, INT(LENYQ - I + 1, 2), XQ(1))
      CALL MOVEQ(YQ, 1_2, XQ, 1_2, LENYQ)
      CALL CLRQ(XQ, INT(LENYQ + 1, 2), INT(LENYQ - I, 2))
      XQ(1) = XQ(1) + LENYQ - I
      RETURN
      END

C-----------------------------------------------------------------------
C  FMP file interface.
C
C  Each DCB the program hands us gets a slot in the table below; the slot
C  number is stashed in the first word of the caller's DCB array.
C     KIND 1  text file, one line per record, read only  (#ADVZZ)
C     KIND 2  direct access, RECLN-byte records          (#ADVXX)
C-----------------------------------------------------------------------

      BLOCK DATA FMPINI
      IMPLICIT NONE
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      DATA FUNIT /21, 22, 23, 24/
      DATA FKIND /0, 0, 0, 0/
      DATA FREC /0, 0, 0, 0/
      DATA FMAX /0, 0, 0, 0/
      END

      SUBROUTINE FMPOPEN(IDCB, IERR, NAME, MODE, IDUMMY)
C
C  NAME carries RTE's file type and size specification after a colon,
C  e.g. "#ADVXX:::2:344:44"; only the name itself means anything here.
C  MODE is 'RO' for read, or contains 'C' when the file is to be created.
C
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR, IDUMMY
      CHARACTER*(*) NAME, MODE
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      CHARACTER*64 FN
      INTEGER I, SLOT, IOS, K
      LOGICAL THERE, CREATE
C
      K = INDEX(NAME, ':')
      IF (K .GT. 0) THEN
        FN = NAME(1:K-1)
      ELSE
        FN = NAME
      ENDIF
      K = LEN_TRIM(FN)
      IF (K .LT. 1) THEN
        IERR = -6
        RETURN
      ENDIF
      CREATE = INDEX(MODE, 'C') .GT. 0
C
      SLOT = 0
      DO I = 1, 4
        IF (FKIND(I) .EQ. 0) THEN
          SLOT = I
          GO TO 10
        ENDIF
      ENDDO
      IERR = -12
      RETURN
C
   10 INQUIRE(FILE=FN(1:K), EXIST=THERE)
      IF (.NOT. THERE .AND. .NOT. CREATE) THEN
        IERR = -6
        RETURN
      ENDIF
      IF (CREATE) THEN
C       A created file is the random-access message file #ADVXX.
        OPEN(UNIT=FUNIT(SLOT), FILE=FN(1:K), ACCESS='DIRECT',
     &       FORM='UNFORMATTED', RECL=88, IOSTAT=IOS)
        FKIND(SLOT) = 2
      ELSEIF (FN(1:K) .EQ. '#ADVXX') THEN
        OPEN(UNIT=FUNIT(SLOT), FILE=FN(1:K), ACCESS='DIRECT',
     &       FORM='UNFORMATTED', RECL=88, STATUS='OLD', IOSTAT=IOS)
        FKIND(SLOT) = 2
      ELSE
        OPEN(UNIT=FUNIT(SLOT), FILE=FN(1:K), STATUS='OLD',
     &       FORM='FORMATTED', IOSTAT=IOS)
        FKIND(SLOT) = 1
      ENDIF
      IF (IOS .NE. 0) THEN
        FKIND(SLOT) = 0
        IERR = -6
        RETURN
      ENDIF
      FREC(SLOT) = 1
      FMAX(SLOT) = 0
      IDCB(1) = SLOT
      IERR = 0
      RETURN
      END

      INTEGER*2 FUNCTION FMPREAD(IDCB, IERR, BUF, NCHAR)
C
C  Read the next record into BUF, transferring at most NCHAR characters
C  and returning the number transferred.  Anything the record holds
C  beyond NCHAR is dropped, and BUF is not padded -- both are what FMP
C  does, and ADV03 relies on having blanked the buffer beforehand.
C
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR, BUF(*), NCHAR
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      CHARACTER*128 LINE
      INTEGER SLOT, IOS, N, I
C
      IERR = 0
      FMPREAD = 0
      SLOT = IDCB(1)
      IF (SLOT .LT. 1 .OR. SLOT .GT. 4 .OR. FKIND(SLOT) .EQ. 0) THEN
        IERR = -6
        FMPREAD = -1
        RETURN
      ENDIF
      LINE = ' '
      IF (FKIND(SLOT) .EQ. 1) THEN
        READ(FUNIT(SLOT), '(A)', IOSTAT=IOS) LINE
        IF (IOS .NE. 0) THEN
          IERR = -12
          FMPREAD = -1
          RETURN
        ENDIF
        N = LEN_TRIM(LINE)
      ELSE
        READ(FUNIT(SLOT), REC=FREC(SLOT), IOSTAT=IOS) LINE(1:88)
        IF (IOS .NE. 0) THEN
          IERR = -12
          FMPREAD = -1
          RETURN
        ENDIF
        FREC(SLOT) = FREC(SLOT) + 1
        N = 88
      ENDIF
      IF (N .GT. NCHAR) N = NCHAR
      CALL FMPPUT(BUF, LINE, N)
      FMPREAD = N
      RETURN
      END

      SUBROUTINE FMPPUT(CB, LINE, N)
      IMPLICIT NONE
      CHARACTER*1 CB(*)
      CHARACTER*(*) LINE
      INTEGER N, I
      DO I = 1, N
        CB(I) = LINE(I:I)
      ENDDO
      RETURN
      END

      SUBROUTINE FMPWRITE(IDCB, IERR, BUF, NCHAR)
C
C  Append one record.
C
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR, BUF(*), NCHAR
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      CHARACTER*88 LINE
      INTEGER SLOT, IOS, N
C
      IERR = 0
      SLOT = IDCB(1)
      IF (SLOT .LT. 1 .OR. SLOT .GT. 4 .OR. FKIND(SLOT) .NE. 2) THEN
        IERR = -6
        RETURN
      ENDIF
      LINE = ' '
      N = NCHAR
      IF (N .GT. 88) N = 88
      CALL FMPGET(BUF, LINE, N)
      WRITE(FUNIT(SLOT), REC=FREC(SLOT), IOSTAT=IOS) LINE
      IF (IOS .NE. 0) THEN
        IERR = -12
        RETURN
      ENDIF
      FREC(SLOT) = FREC(SLOT) + 1
      IF (FREC(SLOT) - 1 .GT. FMAX(SLOT)) FMAX(SLOT) = FREC(SLOT) - 1
      RETURN
      END

      SUBROUTINE FMPGET(CB, LINE, N)
      IMPLICIT NONE
      CHARACTER*1 CB(*)
      CHARACTER*(*) LINE
      INTEGER N, I
      DO I = 1, N
        LINE(I:I) = CB(I)
      ENDDO
      RETURN
      END

      SUBROUTINE FMPSETPOSITION(IDCB, IERR, POS, NEGPOS)
C
C  Position to record POS.  The callers pass the record number twice,
C  once negated, which is FMP's way of saying "absolute record".
C
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR
      INTEGER*4 POS, NEGPOS
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      INTEGER SLOT
      IERR = 0
      SLOT = IDCB(1)
      IF (SLOT .LT. 1 .OR. SLOT .GT. 4 .OR. FKIND(SLOT) .EQ. 0) THEN
        IERR = -6
        RETURN
      ENDIF
      IF (POS .LT. 1) THEN
        IERR = -6
        RETURN
      ENDIF
      FREC(SLOT) = POS
      RETURN
      END

      SUBROUTINE FMPCLOSE(IDCB, IERR)
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR
      INTEGER FUNIT(4), FKIND(4), FREC(4), FMAX(4)
      COMMON /FMPTAB/ FUNIT, FKIND, FREC, FMAX
      INTEGER SLOT
      IERR = 0
      SLOT = IDCB(1)
      IF (SLOT .LT. 1 .OR. SLOT .GT. 4) RETURN
      IF (FKIND(SLOT) .EQ. 0) RETURN
      CLOSE(UNIT=FUNIT(SLOT))
      FKIND(SLOT) = 0
      IDCB(1) = 0
      RETURN
      END

      SUBROUTINE CLOSE(IDCB, IERR)
C
C  The older FMGR entry point; ADV03 still calls it once.
C
      IMPLICIT NONE
      INTEGER*2 IDCB(*), IERR
      CALL FMPCLOSE(IDCB, IERR)
      RETURN
      END

      SUBROUTINE FMPPURGE(NAME)
      IMPLICIT NONE
      CHARACTER*(*) NAME
      CHARACTER*64 FN
      INTEGER K, IOS
      LOGICAL THERE
      K = INDEX(NAME, ':')
      IF (K .GT. 0) THEN
        FN = NAME(1:K-1)
      ELSE
        FN = NAME
      ENDIF
      K = LEN_TRIM(FN)
      IF (K .LT. 1) RETURN
      INQUIRE(FILE=FN(1:K), EXIST=THERE)
      IF (.NOT. THERE) RETURN
      OPEN(UNIT=29, FILE=FN(1:K), STATUS='OLD', IOSTAT=IOS)
      IF (IOS .NE. 0) RETURN
      CLOSE(UNIT=29, STATUS='DELETE')
      RETURN
      END

C-----------------------------------------------------------------------
C  Executive calls and FORLIB.
C-----------------------------------------------------------------------

      SUBROUTINE RMPAR(IPAR)
C
C  RTE returns the five run-string parameters; the program takes the
C  first as its terminal LU.  Standard output is unit 6.
C
      IMPLICIT NONE
      INTEGER*2 IPAR(*)
      INTEGER I
      IPAR(1) = 6
      DO I = 2, 5
        IPAR(I) = 0
      ENDDO
      RETURN
      END

      SUBROUTINE SEGRT
C
C  Return from a segment.  Here the segments are ordinary subroutines,
C  so the RETURN that follows this call does the work.
C
      RETURN
      END

      SUBROUTINE SUSP(I, J)
C
C  Suspend the program until the operator restarts it.
C
      IMPLICIT NONE
      INTEGER*2 I, J
      CHARACTER*1 ANS
      WRITE(*, '(/"[Game suspended.  Press RETURN to resume.]")')
      READ(5, '(A)', END=900, ERR=900) ANS
  900 RETURN
      END

      SUBROUTINE EXEC(ICODE, IA, IB)
C
C  EXEC(11,ITIME) is a time request: ITIME = (10ms ticks, seconds,
C  minutes, hours, day of year).  Only the clock is used, to seed the
C  random number generator.
C
      IMPLICIT NONE
      INTEGER*2 ICODE, IA(*), IB
      INTEGER V(8), DOY, MD(12)
      DATA MD /0,31,59,90,120,151,181,212,243,273,304,334/
      IF (ICODE .EQ. 6) THEN
        CALL EXIT
      ELSEIF (ICODE .EQ. 11) THEN
        CALL DATE_AND_TIME(VALUES=V)
        DOY = MD(V(2)) + V(3)
        IF (V(2) .GT. 2 .AND. MOD(V(1),4) .EQ. 0 .AND.
     &      (MOD(V(1),100) .NE. 0 .OR. MOD(V(1),400) .EQ. 0))
     &      DOY = DOY + 1
        IA(1) = V(8) / 10
        IA(2) = V(7)
        IA(3) = V(6)
        IA(4) = V(5)
        IA(5) = DOY
      ENDIF
      RETURN
      END

      REAL FUNCTION RANDM(SEED)
C
C  FORLIB's uniform generator: returns a deviate in [0,1) and updates
C  SEED in place.  The HP stream cannot be reproduced off the machine,
C  and the caller only ever seeds it from the clock, so this is an
C  ordinary 32-bit linear congruential generator.
C
      IMPLICIT NONE
      REAL SEED
      INTEGER*4 S
      S = TRANSFER(SEED, S)
      IF (S .EQ. 0) S = 123459876
      S = S * 1103515245 + 12345
      SEED = TRANSFER(S, SEED)
      RANDM = REAL(IAND(ISHFT(S, -16), 32767)) / 32768.0
      RETURN
      END
