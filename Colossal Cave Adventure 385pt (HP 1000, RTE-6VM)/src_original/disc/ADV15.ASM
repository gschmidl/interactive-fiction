ASMB,R,L
      NAM RANDM,7 
      ENT RANDM 
      EXT .ENTR,.PACK 
* 
* CALLING SEQUENCE: 
* 
*       X = RANDM(SEED) 
* 
*          WHERE SEED IS A 32 BIT NUMBER WHICH WILL 
*                     CHANGE THE SEQUENCE OF RANDOM 
*                     NUMBERS,
* 
*                X    IS THE RETURNED RANDOM NUMBER 
*                     AND 0.0 < X < 1.0.
* 
CONST OCT 16441 
      OCT 7701
RNDX1 NOP 
RNDX2 NOP 
RNDA1 DEC 1490
RNDA2 DEC 3805
RNDC1 NOP 
RNDC2 NOP 
PAR   NOP 
* 
RANDM NOP 
      JSB .ENTR 
      DEF PAR 
* 
* SEED RNDC1 AND RNDC2
      DLD PAR,I 
      SZA           CHECK FOR SEED=0
      JMP DST 
      DLD CONST 
DST   DST RNDC1 
* 
      LDA RNDX1 
      MPY RNDA2 
      STA RNDX1 
      LDA RNDX2 
      MPY RNDA1 
      ADA RNDX1 
      ADA RNDC1 
      STA RNDX1 
      CLA,CLE 
      STA EXP 
      LDA RNDX2 
      MPY RNDA2 
      ADA RNDC2 
      SEZ 
      INB 
      RAL,CLE,ERA 
      STA RNDX2 
      ELB 
      ADB RNDX1 
      ELB,CLE,ERB 
      STB RNDX1 
      RAL 
SWP   STA PAR 
      LDA 1 
      LDB PAR 
      JSB .PACK 
EXP   NOP 
      CPB =D2 
      RSS 
      JMP RANDM,I 
      LDA =B7777
      LDB =D-256
      JMP RANDM,I 
      END 
