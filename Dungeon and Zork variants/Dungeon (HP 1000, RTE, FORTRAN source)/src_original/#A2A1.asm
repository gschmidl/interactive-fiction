ASMB,R,L
      NAM A2A1,7 (TWH) 1982-11-17 CONVERT A2 TO A1 FORMAT
      ENT A2A1
      EXT .ENTR
*
BUFF  BSS 1
CHARS BSS 1
*
A2A1  NOP
      JSB .ENTR
      DEF BUFF
*
      LDA CHARS,I
      CAX
*
      LDA BUFF
      ADA CHARS,I
      ADA N1
      CLE,ELA
      STA DBADD
      LDA BUFF
      CLE,ELA
      ADA CHARS,I
      ADA N1
      STA SBADD
*
LOOP  LDB SBADD
      LBT
      ADB N2
      STB SBADD
*
      LDB DBADD
      SBT
      LDA BLANK
      SBT
      ADB N4
      STB DBADD
*
      DSX
      JMP LOOP
      JMP A2A1,I
*
BLANK ASC 1,
DBADD BSS 1
N1    DEC -1
N2    DEC -2
N4    DEC -4
SBADD BSS 1
      END
