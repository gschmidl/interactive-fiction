C     FTN77,L
C
C     $FILES 1,1,2,FREESPACE !DON'T PLAY WITH THIS!! - DWH
C     $EMA /TRVCOM/, /LINCOM/, /VOCCOM/
C
      PROGRAM ADVENT
C======================================================================C
C                                                                      C
C REVISION LIST:                                                       C
C                                                                      C
C --DATE-- --BY-- -- D E S C R I P T I O N --                          C
C                                                                      C
C  3/07/81 AW     -ORIGINAL VERSION-                         1.0       C
C 12/12/86 JLA    -FTN77,LINK,MACRO & CI COMPATABLE          2.0       C
C  2/10/87 JLA    -EXPANDED CAVE AND OTHER GOODIES           2.1       C
C  7/13/87 JLA    -FIXED SCORING, DATA BASE & OTHER BUGS     2.2       C
C                                                                      C
C======================================================================C
C
C Modified for HP-21 MXE by Beasley   Sep 78
C
C ADVENTURES  
C 
C MODIFIED BY KENT BLACKETT 
C             ENGINEERING SYSTEMS GROUP 
C             DIGITAL EQUIPMENT CORP. 
C             15-JUL-77 
C MODIFIED BY     BOB SUPNIK
C           DISK ENGINEERING
C           21-OCT-77 
C 
C ORIGINAL VERSION WAS FOR DECSYSTEM-10 
C NEXT VERSION WAS FOR FORTRAN IV-PLUS UNDER
C THE IAS OPERATING SYSTEM ON THE PDP-11/70 
C THIS VERSION IS FOR FORTRAN 77 USING A HP 1000
C 
C 
C  CURRENT LIMITS:
C 
C     800 TRAVEL OPTIONS (TRAVEL, TRVSIZ).
C     330 VOCABULARY WORDS (KTAB, ATAB, TABSIZ).
C     150 LOCATIONS (LTEXT, STEXT, KEY, COND, ABB, ATLOC, LOCSIZ).
C     100 OBJECTS (PLAC, PLACE, FIXD, FIXED, LINK (TWICE), PTEXT, PROP).  
C      35 "ACTION" VERBS (ACTSPK, VRBSIZ).
C     230 RANDOM MESSAGES (RTEXT, RTXSIZ).
C      10 DIFFERENT PLAYER CLASSIFICATIONS (CTEXT, CVAL, CLSMAX). 
C      10 HINTS, LESS 3 (HINTLC, HINTED, HINTS, HNTSIZ).
C      35 MAGIC MESSAGES (MTEXT, MAGSIZ). 
C 
C  THERE ARE ALSO LIMITS WHICH CANNOT BE EXCEEDED DUE TO THE STRUCTURE OF 
C  THE DATABASE.  (E.G., THE VOCABULARY USES N/1000 TO DETERMINE WORD TYPE, 
C  SO THERE CAN'T BE MORE THAN 1000 WORDS.)  THESE UPPER LIMITS ARE:
C 
C     1000 NON-SYNONYMOUS VOCABULARY WORDS
C     300 LOCATIONS 
C     100 OBJECTS 
C 
      IMPLICIT NONE 
C 
      include 'ioccom.fi'
      include 'lincom.fi'
      include 'trvcom.fi'
      include 'voccom.fi'
C 
      INTEGER*2 LOGLU,SES 
C 
C     GET TERMINAL LU FROM SYSTEM 
C 
      CRT = LOGLU(SES)
      KBD = 5
      LU = 10 
      REVISION = 2.2
C 
      CALL CLEAR(CRT) 
      WRITE(CRT,10) REVISION
   10 FORMAT(/"HP 1000 Adventure Version",F4.1) 
C 
      CALL INIT 
      CALL MAIN 
      CALL EXIT 
C 
      END 
      BLOCK DATA ADCOM
C 
      IMPLICIT NONE 
C 
      include 'alphas.fi'
      include 'arycom.fi'
      include 'ioccom.fi'
      include 'magcom.fi'
      include 'miscom.fi'
      include 'placom.fi'
      include 'txtcom.fi'
C 
       END

