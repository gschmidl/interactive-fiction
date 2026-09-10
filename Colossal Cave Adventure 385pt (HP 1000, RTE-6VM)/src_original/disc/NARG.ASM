ASMB
      NAM NARG,7 Get # of calling arguments    ARW 801204 
      ENT NARG
* 
*When called as the FIRST EXECUTABLE STATEMENT in a host FORTRAN
*routine, returns # of arguments in call to host routine. 
* 
*     NARGS=NARG(IDUMY) 
*            where IDUMY= dummy argument (must be present)
*                  NARGS= # of arguments in call to host routine
*                  B-reg return: Direct address of 1st DEF in parameter list
*                                of the routine CALLING the host routine. 
* 
******WARNING!!!  NARG will not work if any arrays in the FTN4 host routine 
*                 are dimensioned with dummy values which are passed
*                 through as parameters to the routine. 
* 
*     NOTE: NARG works equally well, but with the same restrictions, under
*           FTN4 or FTN4X.  However, under FTN4X it is preferable to use
*           the intrinsic function PCOUNT which works without restriction.
* 
*May be called in an ASMB routine immediately following JSB .ENTR as follows: 
*                     ENT SUBR
*                     EXT NARG
*               ARGS  BSS n 
*               SUBR  NOP 
*                     JSB .ENTR 
*                     DEF ARGS
*                     JSB NARG         Get # of calling arguments-- 
*                     NOP                  A-reg returns # of calling args
*                     NOP                  B-reg returns direct address of 1st
*                      etc...                 DEF in parameter list of the routine
*                                             CALLING the host routine. 
* 
* 
*     ARW 781015, revised 801204
* 
*Technique: On return from .ENTR, A-reg contains return address of host 
*           routine to its calling routine.  Search backwards from this 
*           address until a match is found, which will be the DEF following 
*           the JSB in the calling routine to NARG's host.  The number of 
*           location stepped backwards gives the # of calling parameters. 
*           As presently written, NARG demands that the DEF immediately 
*           following the JSB SUBR (in the routine calling the host routine)
*           be a DIRECT address.  So far as I know, this will always be the 
*           case in any ordinary circumstances. 
* 
IARG  NOP 
NARG  NOP 
      ISZ NARG
      ISZ NARG      SET RETURN ADDRESS
      STA B 
LOOP  ADB =B-1
      CPA B,I 
      JMP *+2 
      JMP LOOP
      INB 
      STB SAVE
      CMB,INB 
      ADA B 
      LDB SAVE
      JMP NARG,I
SAVE  NOP 
A     EQU 0 
B     EQU 1 
      END 
