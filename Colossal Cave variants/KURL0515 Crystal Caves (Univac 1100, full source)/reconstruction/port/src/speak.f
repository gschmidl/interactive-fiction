      SUBROUTINE SPEAK(N)
C
C  PRINT THE MESSAGE WHICH STARTS AT LINES(N).  PRECEDE IT WITH A BLANK
C  LINE UNLESS BLKLIN IS FALSE.
C
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comtxt.fi'
      include 'comblk.fi'
C     Originally a leading carriage-control character ('0' for a blank
C     line before this message, ' ' for none) embedded as the first
C     data item of the WRITE.  gfortran doesn't interpret that old
C     line-printer convention -- it would just print the character
C     literally -- so this now tracks the same "blank line still
C     owed?" state as a LOGICAL and prints a real blank line instead.
      LOGICAL SPACNG
      character*4 ignorm
      data ignorm/'>$< '/
C
      IF (N.EQ.0) RETURN
      IF (CLINES(N+1).EQ.ignorm) RETURN
      SPACNG = BLKLIN
    2 FORMAT()
      K=N
   10 L=IABS(LINES(K))-1
      IF (L-K) 50,30,15
   15 K=K+1
      IF (L-K.GT.17) GOTO 50
      IF (SPACNG) WRITE(6,2)
      WRITE(6,20) (CLINES(I),I=K,L)
   20 FORMAT(1X,18A4)
      SPACNG = .FALSE.
      GOTO 40
C
   30 WRITE(6,2)
   40 K=L+1
      IF (LINES(K).GE.0) GOTO 10
      RETURN
   50 CALL BUG(29)
      END


      SUBROUTINE MSPEAK(I)
C
C  PRINT THE I-TH "MAGIC" MESSAGE (SECTION 12 OF DATABASE).
C
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'commtx.fi'
C
      IF (I.NE.0) CALL SPEAK(MTEXT(I))
      RETURN
      END


      SUBROUTINE PSPEAK(MSG,SKIP)
C
C  FIND THE SKIP+1ST MESSAGE FROM MSG AND PRINT IT.
C  MSG SHOULD BE THE INDEX OF THE INVENTORY MESSAGE FOR OBJECT.
C  (INVEN+N+1 MESSAGE IS PROP=N MESSAGE).
C
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comtxt.fi'
      include 'comptx.fi'
C
      M=PTEXT(MSG)
      IF (SKIP.LT.0) GOTO 9
      DO 3 I=0,SKIP
    1 M=IABS(LINES(M))
      IF (LINES(M).GE.0) GOTO 1
    3 CONTINUE
    9 CALL SPEAK(M)
      RETURN
      END


      SUBROUTINE RSPEAK(I)
C
C  PRINT THE I-TH "RANDOM" MESSAGE (SECTION 6 OF DATABASE).
C
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comtxt.fi'
      include 'comblk.fi'
C
      if (rmsgx(i)) mltcmd = .false.
      if (i .eq. 54) blklin = .false.
      IF (I.NE.0) CALL SPEAK(RTEXT(I))
      RETURN
      END
