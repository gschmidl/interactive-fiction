C ======================================================================
C  PDAVE - what VAX/VMS and its FORTRAN run-time library did for
C  DAVESCAVE, written out for gfortran:
C
C    RAN(I1,I2)   FOR$IRAN, the PDP-11 compatible generator, taken
C                 instruction by instruction from FORRTL.EXE
C    SECNDS(X)    seconds since midnight, less X
C    PREAD(LINE)  a line from the terminal, in upper case
C    PNAMEB/I     the file names the program keeps in byte arrays
C ======================================================================

      BLOCK DATA PDAVBD
      IMPLICIT INTEGER (A-Z)
      COMMON /PDAVCM/ PTIM,PFIX
      REAL PTIM
      DATA PTIM/-1.0/,PFIX/1/
      END

C ----------------------------------------------------------------------
C  FOR$IRAN, from the VAX run-time library (FORRTL.EXE, entry at
C  transfer vector +400):
C
C      MOVW  @4(AP),R0          R0 = I1*65536 + I2: I1 is the high word
C      ROTL  #16,R0,R0
C      MOVW  @8(AP),R0
C      BEQL  zero
C      MULL3 #^X10003,R0,R1     seed * 65539 - RANDU - modulo 2**32
C      BBCC  #31,R1,1$          and bit 31 cleared
C  1$: CVTLF R1,R0              to F_floating, rounded
C      MULF2 #2**-31,R0
C      MOVW  R1,@8(AP)          the new seed back into I2 and I1
C      ROTL  #16,R1,R1
C      MOVW  R1,@4(AP)
C      RET
C zero: ADDL2 #^X10000,R0       I2 = 0: bump I1, make I2 3, and
C      MOVW  #3,R0              return that as it stands
C      MOVL  R0,R1
C      BRB   1$
C
C  CVTLF rounds half away from zero, where IEEE rounds half to even, so
C  the conversion is done by hand: the result is then exact in REAL*4,
C  which has the same 24 bit fraction as F_floating.
C ----------------------------------------------------------------------
      REAL FUNCTION RAN(I1,I2)
      IMPLICIT INTEGER*8 (A-Z)
      INTEGER*2 I1,I2
      REAL PCVTLF
      R0=IOR(ISHFT(IAND(INT(I1,8),65535_8),16),IAND(INT(I2,8),65535_8))
      IF(I2.EQ.0)THEN
         R0=IAND(R0+65536_8,4294967295_8)
         R1=IOR(IAND(R0,-65536_8),3_8)
      ELSE
         R1=IAND(R0*65539_8,4294967295_8)
         R1=IAND(R1,2147483647_8)
      ENDIF
      RAN=PCVTLF(R1)*2.0**(-31)
      I2=INT(IAND(R1,65535_8)-ISHFT(IAND(R1,32768_8),1),2)
      H=IAND(ISHFT(R1,-16),65535_8)
      I1=INT(H-ISHFT(IAND(H,32768_8),1),2)
      RETURN
      END

C  CVTLF: a longword (the low 32 bits of L, as a signed number) to
C  F_floating, rounding half away from zero.
      REAL FUNCTION PCVTLF(L)
      IMPLICIT INTEGER*8 (A-Z)
      V=IAND(L,4294967295_8)
      IF(V.GE.2147483648_8)V=V-4294967296_8
      SGN=1
      IF(V.LT.0)THEN
         SGN=-1
         V=-V
      ENDIF
      NB=0
      W=V
1     IF(W.GT.0)THEN
         NB=NB+1
         W=ISHFT(W,-1)
         GOTO 1
      ENDIF
      S=0
      IF(NB.GT.24)THEN
         S=NB-24
         V=ISHFT(V+ISHFT(1_8,S-1),-S)
      ENDIF
      PCVTLF=REAL(SGN*V)*2.0**S
      RETURN
      END

C ----------------------------------------------------------------------
C  SECNDS(X): seconds since midnight less X, to the hundredth, as VMS
C  gives it.  --time holds it still.
C ----------------------------------------------------------------------
      REAL FUNCTION SECNDS(X)
      IMPLICIT INTEGER (A-Z)
      REAL X
      DIMENSION V(8)
      COMMON /PDAVCM/ PTIM,PFIX
      REAL PTIM
      IF(PTIM.GE.0.0)THEN
         SECNDS=PTIM-X
      ELSE
         CALL DATE_AND_TIME(VALUES=V)
         SECNDS=REAL(V(5)*3600+V(6)*60+V(7))+REAL(V(8)/10)/100.0-X
      ENDIF
      RETURN
      END

C ----------------------------------------------------------------------
C  PREAD(LINE): a line from the terminal as the program expects it.  It
C  knows its commands and answers in upper case only, as upper-case
C  terminals sent them, so lower case is folded to upper; bit 8 is
C  masked off and NULs are dropped.  At a terminal the input never ends;
C  piped input does, and then the game stops quietly.  Every READ from
C  the terminal reads its items from this line (src\convert.py).
C ----------------------------------------------------------------------
      SUBROUTINE PREAD(LINE)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) LINE
      CHARACTER*256 RAW
      READ(5,'(A)',END=9)RAW
      LINE=' '
      K=0
      DO 1 I=1,LEN_TRIM(RAW)
      C=IAND(ICHAR(RAW(I:I)),127)
      IF(C.EQ.0)GOTO 1
      IF(C.GE.97.AND.C.LE.122)C=C-32
      K=K+1
      IF(K.LE.LEN(LINE))LINE(K:K)=CHAR(C)
1     CONTINUE
      RETURN
9     STOP
      END

C ----------------------------------------------------------------------
C  File names.  The program keeps them in byte arrays, with the player's
C  initials as the extension, and on the device DB3:.  Here the device
C  goes: TEXTFILE.DAD is the game's own data and lives beside the .exe,
C  the player's three files in saves\ beside it.
C ----------------------------------------------------------------------
      CHARACTER*260 FUNCTION PNAMES(S)
      IMPLICIT INTEGER (A-Z)
      CHARACTER*(*) S
      CHARACTER*260 D
      CHARACTER*40 N
      N=S
      K=INDEX(N,':')
      IF(K.GT.0)N=N(K+1:)
      CALL PGMDIR(D)
      L=LEN_TRIM(D)
      IF(N(1:8).EQ.'TEXTFILE')THEN
         PNAMES=D(1:L)//N
      ELSE
         PNAMES=D(1:L)//'saves'//CHAR(92)//N
      ENDIF
      RETURN
      END

      CHARACTER*260 FUNCTION PNAMEB(B)
      IMPLICIT INTEGER (A-Z)
      INTEGER*1 B(*)
      CHARACTER*40 S
      CHARACTER*260 PNAMES
      S=' '
      DO 1 I=1,40
      IF(B(I).EQ.0)GOTO 2
1     S(I:I)=CHAR(B(I))
2     PNAMEB=PNAMES(S)
      RETURN
      END

      CHARACTER*260 FUNCTION PNAMEI(T)
      IMPLICIT INTEGER (A-Z)
      INTEGER*1 T(*)
      CHARACTER*260 PNAMEB
      PNAMEI=PNAMEB(T)
      RETURN
      END

C ----------------------------------------------------------------------
C  Options (the main program is in pmain.f, so that test drivers can
C  link this file with a main of their own).
C ----------------------------------------------------------------------
      SUBROUTINE POPTS
      IMPLICIT INTEGER (A-Z)
      CHARACTER*80 A
      COMMON /PDAVCM/ PTIM,PFIX
      REAL PTIM
      N=COMMAND_ARGUMENT_COUNT()
      I=1
1     IF(I.GT.N)RETURN
      CALL GET_COMMAND_ARGUMENT(I,A)
      IF(A(1:6).EQ.'--time')THEN
         I=I+1
         CALL GET_COMMAND_ARGUMENT(I,A)
         READ(A,*,ERR=9)K
         PTIM=REAL((K/10000)*3600+MOD(K/100,100)*60+MOD(K,100))
      ELSE IF(A(1:10).EQ.'--no-fixes')THEN
         PFIX=0
      ELSE IF(A(1:2).EQ.'-h'.OR.A(1:6).EQ.'--help')THEN
         CALL PUSAGE
      ELSE
         GOTO 9
      ENDIF
      I=I+1
      GOTO 1
9     PRINT 2,A(1:20)
2     FORMAT(' PORT: bad option ',A)
      CALL PUSAGE
      END

      SUBROUTINE PUSAGE
      PRINT 1
1     FORMAT(
     +' davescave [--time HHMMSS] [--no-fixes] [-h]'//
     +' Dave''s Dungeon - Dungeons and Dragons written by Dave Parker,'/
     +' distributed by the MITRE Corporation, version 1.0 of Sept 1980.'/
     +' Your character, your dungeon and your map are kept in saves\'/
     +' under your three initials, as the original kept them.'//
     +'   --time HHMMSS  hold the clock still.  The dice are seeded'/
     +'                  from it, so this makes a game reproducible.'/
     +'   --no-fixes     turn off this port''s fixes (there are none)'/)
      STOP
      END

C  ININT (MTH$ININT): the nearest INTEGER*2, halves away from zero.
      INTEGER*2 FUNCTION ININT(X)
      REAL X
      ININT=INT(NINT(X),2)
      RETURN
      END
