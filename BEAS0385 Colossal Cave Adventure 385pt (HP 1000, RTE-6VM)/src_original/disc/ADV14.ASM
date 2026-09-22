ASMB,R
      HED     <STRING FUNCTION IPOSQ(X$,Y$,Z)>
      NAM     IPOSQ,7  ARW 4/77 
      ENT     IPOSQ 
      EXT     .ENTR 
*     FUNCTION IPOSQ(X$,Y$,Z) 
* 
*     SEARCHES FOR AND RETURNS THE POSITION OF THE FIRST
*     OCCURENCE OF Y$ IN X$ STARTING WITH THE Z-TH CHARACTER OF X$. 
*     IF THE STRING Y$ IS NOT FOUND IN THE STRING X$, THEN 0
*     IS RETURNED. IF X$ IS A NULL STRING, 0 IS RETURNED. 
*     IF Y$ IS A NULL STRING, Z IS RETURNED.
*     IF Z .LE. 0, 1 IS ASSUMED.
*     IF Z .GT. LEN(X$), THEN 0 IS RETURNED.
* 
*     THIS FUNCTION IS IDENTICAL TO THE PDP BASIC FUNCTION 'POS'. 
* 
* 
*     ARW 4/19/76   UPDATED 4/7/77
* 
PRAM  BSS     3 
IPOSQ NOP 
      JSB     .ENTR 
      DEF     PRAM
*SAVE REGISTERS 
*GET STRING LENGTHS 
      LDA     PRAM,I    GET LENGTH OF X$
      STA     LENX
      LDA     PRAM+1,I  GET LENGTH OF Y$
      STA     LENY
      LDA     PRAM+2,I
      STA     IZ
*JPOS=0 
      CLA 
      STA     JPOS
*IF (LENX .EQ. 0) RETURN
      LDA     LENX
      SZA,RSS 
      JMP     RETRN 
*IF (LENY .EQ. 0) JPOS=IZ AND RETURN
      LDA     LENY
      SZA 
      JMP     A0
      LDA     IZ
      STA     JPOS
      JMP     RETRN 
*IF (IZ .GT. LENX) RETURN 
A0    LDA     IZ
      CMA,INA 
      ADA     LENX
      SSA 
      JMP     RETRN 
*IF (IZ .LE. 0) IZ=1
      LDA     IZ
      CMA,INA 
      CMA 
      SSA,RSS 
      JMP     A1
      LDA     =B1 
      STA     IZ
*IZ1=IZ-1 
A1    LDA     =B1 
      CMA,INA 
      ADA     IZ
      STA     IZ1 
*N=LENX-IZ1-LENY+1
      LDA     IZ1 
      CMA,INA 
      ADA     LENX
      STA     1 
      LDA     LENY
      CMA,INA 
      ADA     1 
      ADA     =B1 
      STA     N 
      CAY 
*IF (N .LE. 0) RETURN 
      CMA,INA 
      CMA 
      SSA 
      JMP     RETRN 
*COMPUTE CHARACTER ADDRESSES FOR SEARCH 
      LDA     PRAM
      RAL 
      ADA     =B1 
      ADA     IZ1       ADDRESS OF FIRST CHAR OF X$ TO BE COMPARED
      CAX 
      LDA     PRAM+1
      RAL 
      ADA     =B2 
      STA     YSTRT     ADDRESS OF FIRST CHAR OF Y$ 
*START OF COMPARE LOOP
LOOP  ISX               INCREMENT X 
      CXA                 TO GET NEXT X$ START ADDRESS
      LDB     YSTRT 
      CBT     LENY      COMPARE BYTES 
      JMP     CMPAR     GOOD COMPARE
      NOP               COMPARE BAD 
      NOP               COMPARE BAD 
      DSY               DECREMENT Y AND SKIP IF ZERO
      JMP     LOOP
      JMP     RETRN     END OF UNSUCCESSFUL SEARCH
*JPOS=IZ+N-Y
CMPAR CYA 
      CMA,INA 
      ADA     N 
      ADA     IZ
      STA     JPOS
RETRN LDA     JPOS
      JMP     IPOSQ,I 
* 
JPOS  BSS     1 
IZ    BSS     1 
IZ1   BSS     1 
LENX  BSS     1 
LENY  BSS     1 
N     BSS     1 
YSTRT BSS     1 
      END 
      $END
