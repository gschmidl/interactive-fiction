C  Prints the port's RAN(I1,I2) for a few seeds, including I2 = 0,
C  so that tests\pran.py can check it against a model of the VAX
C  instructions of FOR$IRAN.
      PROGRAM PRAN
      INTEGER*2 I1,I2,S1(4),S2(4)
      EXTERNAL RAN
      REAL RAN,X
      DATA S1/0,1234,-1,32767/,S2/1,5678,0,-32768/
      DO 2 K=1,4
      I1=S1(K)
      I2=S2(K)
      DO 1 N=1,6
      X=RAN(I1,I2)
      WRITE(6,10)K,N,X,I1,I2
10    FORMAT(I2,I3,Z9.8,2I8)
1     CONTINUE
2     CONTINUE
      END
