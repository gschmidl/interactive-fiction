ASMB,R
      HED   <BASIC SUBROUTINE TRIMQ(X$)>
      NAM     TRIMQ,7  ARW 770427 
      ENT     TRIMQ 
      EXT     .ENTR 
*     SUBROUTINE TRIMQ(X$)
* 
*     RETURNS X$ WITH ANY TRAILING BLANKS OR NULLS REMOVED
* 
*     ARW 4/20/76   UPDATED 4/7/77,770427 
* 
PRAM  BSS     1 
TRIMQ NOP 
      JSB     .ENTR 
      DEF     PRAM
*SET UP CHARACTER ADDRESSES 
      LDB     PRAM
      RBL 
      INB 
      STB     LADD      SAVE STRING ADDRESS 
      LDA     PRAM,I    GET STRING LENGTH 
      CAX 
      SZA,RSS           CHECK FOR N .EQ. 0
      JMP     DONE
*SEARCH FROM END OF STRING FOR NON-SPACE,NON-NULL CHARACTER 
LOOP  CXB 
      ADB     LADD
      LBT 
      CPA     =B40      SPACE?
      JMP     .TRM      YES.
      SZA               NO. NULL? 
      JMP     DONE           NO.
.TRM  DSX               DECREMENT CHARACTER POINTER 
      JMP     LOOP
DONE  CXA 
      STA     PRAM,I
      JMP     TRIMQ,I 
LADD  BSS     1 
A     EQU     0 
      END 
      $END
