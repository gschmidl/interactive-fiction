ASMB,R
      NAM           ISHFT  ZERO-FILL LEFT SHIFTER   BGL 
* 
*     THIS IS A FORTRAN-CALLABLE ROUTINE TO LEFT-SHIFT AN INTEGER 
*     WORD "N" PLACES.  TO USE, NEWWD=ISHFT(IWORD,N).  THE LEFT 
*     SHIFT IS ZERO-FILLED. 
* 
      EXT           .ENTR 
      ENT           ISHFT 
ADR1  BSS           1 
N     BSS           1 
ISHFT NOP 
      JSB           .ENTR 
      DEF           ADR1
      LDY           ADR1,I
      LDA           N,I 
      SZA 
      JMP           NONZ
      JMP           NSHFT 
NONZ  XAY 
      CLE 
AGAIN ELA,CLE 
      DSY 
      JMP           AGAIN 
      JMP           ISHFT,I 
NSHFT XAY 
      JMP           ISHFT,I 
      END 
