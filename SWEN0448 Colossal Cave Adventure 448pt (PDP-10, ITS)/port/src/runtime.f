C  runtime.f -- the PDP-10 word layer for the ADV448 port.
C
C  The port keeps the DECsystem-10 packing rather than modernising it: a
C  word is five seven-bit characters, character k in bits 36-7k..30-7k of
C  an INTEGER*8.  The program's own DATA MASKS fix that layout --
C  "774000000000 selects character 1 and "4000000000,"20000000,"100000,
C  "400,"2 are the low bits of the five fields (29,22,15,8,1).  Keeping it
C  means every mask, shift and .AND./.XOR. in the game logic still works.
C
C  What the compiler cannot supply is the A edit descriptor over that
C  layout -- gfortran packs eight-bit bytes -- so the A-format reads and
C  writes go through C2W/W2C here instead.  DEC FORTRAN-10 left-justified
C  An data in the word and blank-filled the rest; that is what C2W does.

      SUBROUTINE C2W(S, N, W)
C  Pack the first N characters of S into word W, blank-filled to five.
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
C  Sign-extend: bit 35 is the sign on a PDP-10 and the program
C  tests it (see A5TOA1).  Literals are packed the same way.
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
         IF (C .LT. 32 .OR. C .GT. 126) C = 32
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
C  Unpack NW words of NC characters into a string.
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
C  The original was 36-bit arithmetic on the sign bit; in 64 bits that
C  would test the wrong bit, so it is done directly here.  Positive DIST
C  shifts left, negative right, and the result is confined to 36 bits.
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

      INTEGER FUNCTION RAN10(RANGE)
C  Uniform in 0..RANGE-1.  The original used the PDP-10 clock.
      IMPLICIT INTEGER(A-Z)
      REAL R
      LOGICAL SEEDED
      SAVE SEEDED
      DATA SEEDED /.FALSE./
      IF (.NOT. SEEDED) THEN
         CALL RANSEED
         SEEDED = .TRUE.
      ENDIF
      IF (RANGE .LE. 0) THEN
         RAN10 = 0
         RETURN
      ENDIF
      CALL RANDOM_NUMBER(R)
      RAN10 = INT(R * RANGE)
      IF (RAN10 .GE. RANGE) RAN10 = RANGE - 1
      RETURN
      END

      SUBROUTINE RANSEED
      IMPLICIT INTEGER(A-Z)
      INTEGER, ALLOCATABLE :: SD(:)
      CALL RANDOM_SEED(SIZE = N)
      ALLOCATE(SD(N))
      CALL SYSTEM_CLOCK(COUNT = C)
      DO 1 I = 1, N
         SD(I) = C + 37*I
1     CONTINUE
      CALL RANDOM_SEED(PUT = SD)
      DEALLOCATE(SD)
      RETURN
      END

      SUBROUTINE DATIME(D, T)
C  Original packed the PDP-10 date/time words.  Callers only ever print
C  these or difference them, so plain numbers serve: D is days since
C  1 Jan 1900 and T is minutes since midnight.
      IMPLICIT INTEGER(A-Z)
      INTEGER V(8)
      CALL DATE_AND_TIME(VALUES = V)
      Y = V(1)
      M = V(2)
      DY = V(3)
      IF (M .LE. 2) THEN
         Y = Y - 1
         M = M + 12
      ENDIF
      D = 365*Y + Y/4 - Y/100 + Y/400 + (153*(M-3)+2)/5 + DY - 693901
      T = V(5)*60 + V(6)
      RETURN
      END
