       SUBROUTINE POOF

C  AS PART OF DATABASE INITIALISATION, WE CALL POOF TO SET UP SOME DUMMY
C  PRIME-TIME SPECS, MAGIC WORDS, ETC.

       IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'

       wkday(1) = 0
       wkday(2) = 261888
       wkend(1) = 0
       wkend(2) = 0
       holid(1) = 0
       holid(2) = 0
       HBEGIN=0
       HEND=-1
       SHORT=30
       MAGIC='dwarf'
       MAGNM=11111
       LATNCY=90
       RETURN
       END
