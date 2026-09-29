ASMB,R,L
      NAM A1A2,7 (TWH) 1982-11-17 CONVERT A1 TO A2 FORMAT
      ENT A1A2
      EXT .ENTR
*
BUFF  BSS 1
CHARS BSS 1
*
A1A2  NOP
      JSB .ENTR
      DEF BUFF
*
      LDA CHARS,I
      ADA N1
      CAX
*
      LDA BUFF
      CCE
      ELA
      STA DBADD
      INA
      STA SBADD
*
LOOP  LDB SBADD
      LBT
      INB
      STB SBADD
*
      LDB DBADD
      SBT
      STB DBADD
*
      DSX
      JMP LOOP
      JMP A1A2,I
*
DBADD BSS 1
N1    DEC -1
SBADD BSS 1
      END
