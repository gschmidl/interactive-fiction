      SUBROUTINE GETIN (WORD1, WORD1X, WORD2, WORD2X)

C  Get a command from the adventurer.  Snarf out the first word, pad it
C  with blanks, and return the first five characters in WORD1.  The full
C  word (up to 12 characters) will be returned in WORD1X,
C  in case we need to print out the whole word in an error
C  message.  Any number of blanks may follow the first word of a
C  command.  If a second word appears, it is returned in WORD2
C  and WORD2X in a similar manner, else WORD2 is set to blanks.

C  All leading blanks in a command are ignored.  A command is
C  terminated by a comma, a period, or the end of the line.

C  If a command ends with a period, the current image will be held
C  for the next GETIN call, and will be scanned starting with the
C  character after the period (unless the caller has set MLTCMD
C  false).  If a period is immediately preceded by a blank, the
C  rest of the image will be ignored.  This all permits commands
C  such as "IN.  GET LAMP.  OUT     . comment " .

C  If a command ends with a comma, SAMVRB will be set when
C  scanning the next part of the image, to cope with commands
C  such as "GET LAMP,KEYS" .

C  An input image consisting of just "AGAIN", "DITTO", etc., will
C  result in resubmission of the previous input image.

      IMPLICIT INTEGER (A-Z)
      include 'comblk.fi'
      parameter (insize=80)
      CHARACTER*80 INP, INP2
      CHARACTER*5  WORD1, WORD2, TMPWRD
      CHARACTER*12 WORD1X, WORD2X
      CHARACTER*1  CHAR, BLANK, COMMA, PERIOD
      CHARACTER*1  TMPCHR, LCHAR
      CHARACTER*80 LOWERC
      DATA CHAR/' '/, BLANK/' '/, COMMA/','/, PERIOD/'.'/
C     INP2 must survive between separate calls to GETIN (that's how
C     "AGAIN" repeats the last command line) -- a plain local without
C     SAVE is only guaranteed to keep its value across calls if it's
C     DATA-initialized (like the ones above) or explicitly SAVEd, so
C     make that explicit here.  (NXTCHR has the same requirement, but
C     is already in COMMON via comblk.fi, which already guarantees it.)
      SAVE INP2

      WORD1  = '     '
      WORD2  = '     '
      WORD1X = '            '
      WORD2X = '            '
      GO TO 12
10    MLTCMD = .FALSE.

12    IF (.NOT. MLTCMD) THEN
        IF (BLKLIN) PRINT 15
15      FORMAT ()
        READ (5,1000,END=9999) INP
1000    FORMAT (A80)
        NXTCHR = 0
        MLTCMD = .TRUE.
        INP = LOWERC (INP)
      END IF
      I = NXTCHR + 1

C  Check for "AGAIN", which repeats previous command string.

      IF (NXTCHR .EQ. 0) THEN
        TMPWRD = INP(I:I+4)
        if (tmpwrd .eq. 'again' .or. tmpwrd .eq. 'repea' .or.
     1      tmpwrd .eq. 'ditto' .or. tmpwrd .eq. '"    ' .or.
     2      tmpwrd .eq. '''''   ') then
          INP = INP2
        ELSE
          INP2 = INP
        END IF
      END IF

C  Extract the first word.  If NXTCHR is nonzero, we're in the middle of
C  an earlier input image:  a comma or period as the first character
C  at this point indicates a multiple command line, and we continue with
C  it.  Anything else indicates end of the image, and we get another.

      SAMVRB = .FALSE.
      IF (NXTCHR .NE. 0) THEN
        TMPCHR = INP(NXTCHR:NXTCHR)
        IF (TMPCHR .NE. COMMA .AND. TMPCHR .NE. PERIOD) THEN
          GO TO 10
        ELSE
          IF (TMPCHR .EQ. COMMA) THEN
            SAMVRB = .TRUE.
          END IF
        END IF
      END IF

      CALL GATHER (WORD1X, 12)
      WORD1 = WORD1X(1:5)
      GO TO (111,222,333), ST

C  Long first word:  skip until blank or end.

111   ST = GCHAR (I)
      GO TO (111,222,333), ST

C  Extract second word.

222   CALL GATHER (WORD2X, 12)
      WORD2 = WORD2X(1:5)
333   CONTINUE

      IF (NXTCHR .NE. 0 .AND. WORD1 .EQ. '     ') THEN
        GO TO 10
      END IF
      RETURN

9999  print 9998
9998  format (/' But, but, ... ok, goodbye.')
      stop


      CONTAINS

C  Subroutine to collect N chars in one word
C  Stops on end of input, blank or got N chars

      SUBROUTINE GATHER (WHERE, N)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*12 WHERE

C  Skip leading blanks

200   ST = GCHAR (I)
      GO TO (300,200,999), ST
300   I = I - 1

      L = 1
      DO 500 M=1,N
      ST = GCHAR (I)
      IF (ST .NE. 1) GO TO 999
      WHERE(L:L) = CHAR
      IF (L .NE. N) GO TO 400

C  If we filled 'where' and next char is blank
C  return blank status in ST

      OST = ST
      ST = GCHAR (I)
      I = I - 1
      IF (ST .NE. 2) ST = OST
      GO TO 999

400   L = L + 1
500   CONTINUE

999   RETURN
      END SUBROUTINE GATHER

C  Subroutine to get next character from image
C  Returns char in 'char'
C  Returns status  1=got char
C                  2=got blank
C                  3=end of input

      INTEGER FUNCTION GCHAR (II)
      IMPLICIT INTEGER (A-Z)

      GCHAR = 3
      IF (II .GT. INSIZE) RETURN
      LCHAR = CHAR
      CHAR = INP(II:II)
      NXTCHR = II
      II = II + 1
      IF (CHAR .EQ. BLANK) THEN
        GCHAR = 2
      ELSE
        IF (CHAR .EQ. PERIOD .OR. CHAR .EQ. COMMA) THEN
          IF (LCHAR .EQ. BLANK) MLTCMD = .FALSE.
        ELSE
          GCHAR = 1
        END IF
      END IF
      RETURN

      END FUNCTION GCHAR

      END SUBROUTINE GETIN


      CHARACTER*80 FUNCTION LOWERC(S)
C  Portable replacement for the Univac FTN LOWERC intrinsic.
      CHARACTER*(*) S
      CHARACTER*1 C
      INTEGER I
      LOWERC = S
      DO 10 I=1,LEN(S)
      C = S(I:I)
      IF (C.GE.'A' .AND. C.LE.'Z') THEN
        LOWERC(I:I) = CHAR(ICHAR(C)+32)
      END IF
10    CONTINUE
      RETURN
      END
