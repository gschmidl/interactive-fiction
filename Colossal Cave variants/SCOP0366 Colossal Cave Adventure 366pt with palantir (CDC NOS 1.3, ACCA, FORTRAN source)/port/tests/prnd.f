C  Check the port's random number generator against the machine.
C
C  tests\cyber\probe.job ran this on NOS 1.3, seeded by CALL RANSET(1.)
C  which left the seed at 04631463146314631 (octal), and printed
C
C     RND   1 GAVE     30     4      RND   5 GAVE     75     4
C     RND   2 GAVE     79     4      RND   6 GAVE     20     3
C     RND   3 GAVE     15     3      RND   7 GAVE     51     1
C     RND   4 GAVE     65     2      RND   8 GAVE     90     0
C
C  This driver puts the same seed in and calls the game's own RND, so it
C  goes through RANF, the REAL multiply and the truncation exactly as the
C  game does.  tests\cmpcyber.py compares the two.
      PROGRAM PRND
      IMPLICIT INTEGER (A-Z)
      COMMON /PCDCOM/ PSEED,PTIM,PFIX,PEOFF,PDCINI
      DIMENSION S(4)
C  The seeds the machine was measured with, as 48 bit integers.
      DATA S/168884986026393,140737488355329,211106232532993,
     +       48131768981101/
C  RND seeds itself from the clock on its first call, so let it.
      A=RND(1)
      DO 2 K=1,4
      PSEED=S(K)
      DO 1 I=1,8
      A=RND(100)
      B=RND(5)
      PRINT 10,K,I,A,B
10    FORMAT(' SEED ',I2,' RND ',I3,' GAVE ',2I6)
1     CONTINUE
2     CONTINUE
      END
