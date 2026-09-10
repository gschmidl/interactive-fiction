         INTEGER FUNCTION RAN(IRANGE)
C
C   RAN RETURNS AN INTEGER VALUE UNIFORMLY SELECTED BETWEEN
C     0 AND IRANGE -1.  SEE KNUTH, P. 155.
C   THIS ALGORITHM IS GOOD FOR MACHINES WITH WORD LENGTH OF 24 OR MORE.
C
C   Portable rewrite: seeded from SYSTEM_CLOCK instead of the Univac
C   TDATE$ packed word (only used here to pick a starting seed).
C
         IMPLICIT INTEGER (A-Z)
         DATA J/0/
         IF (J.NE.0) GOTO 20
         CALL SYSTEM_CLOCK(J)
         J = MOD(IABS(J),1000000)+1
20       CONTINUE
         J = IAND (8388607, J * 8385709 )
         J = IAND ( 8388607,J + 1772721)
         RAN = INT((REAL(J)/8388607.) * IRANGE)
         RETURN
       END
