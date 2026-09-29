ASMB
      HED     <FUNCTION LCO(I)> 
      NAM     LCO,7  ARW 6/76 
      ENT     LCO 
      EXT     .ENTR 
* 
*FUNCTION TO RETURN THE ABSOLUTE LCOATION OF I IN CORE. 
* 
PRAM  BSS     1 
LCO   NOP 
      JSB     .ENTR 
      DEF     PRAM
      CLB 
      LDA     PRAM
      JMP     LCO,I 
      END 
      END$
