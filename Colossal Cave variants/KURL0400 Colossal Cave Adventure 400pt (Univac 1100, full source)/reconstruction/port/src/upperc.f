      CHARACTER*80 FUNCTION UPPERC(S)
C  Portable replacement for the Univac FTN UPPERC intrinsic.
      CHARACTER*(*) S
      CHARACTER*1 C
      INTEGER I
      UPPERC = S
      DO 10 I=1,LEN(S)
      C = S(I:I)
      IF (C.GE.'a' .AND. C.LE.'z') THEN
        UPPERC(I:I) = CHAR(ICHAR(C)-32)
      END IF
10    CONTINUE
      RETURN
      END
