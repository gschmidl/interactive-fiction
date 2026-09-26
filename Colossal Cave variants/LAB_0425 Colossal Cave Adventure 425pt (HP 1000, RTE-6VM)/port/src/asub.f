C     FTN77,L
C     $EMA /TRVCOM/, /LINCOM/, /VOCCOM/
C======================================================================C
C                                                                      C
C REVISION LIST:                                                       C
C                                                                      C
C --DATE-- --BY-- -- D E S C R I P T I O N --                          C
C                                                                      C
C  3/07/81 AW     -ORIGINAL VERSION-                                   C
C 12/12/86 JLA    -RTE-A VERSION-                                      C
C  2/10/87 JLA    -EXPANDED CAVE AND OTHER GOODIES                     C
C  7/13/87 JLA    -FIXED SCORING, DATA BASE & OTHER BUGS               C
C                                                                      C
C======================================================================C
C SUBROUTINES FOR ADVENTURE
C
C MODIFIED FOR PDP-11 FORTRAN IV BY
C
C     R. SUPNIK
C     DISK ENGINEERING
C
C  DATA STRUCTURE ROUTINES (VOCAB, DESTROY, JUGGLE, MOVE, PUT, CARRY, DROP) 
C 
C 
      SUBROUTINE VOCAB(ID,INIT,V,INDEX)
C 
C  LOOK UP ID IN THE VOCABULARY (ATAB)
C  AND RETURN ITS "DEFINITION" (KTAB), OR 
C  -1 IF NOT FOUND.  IF INIT IS POSITIVE, THIS IS AN INIT CALL SETTING
C  UP A KEYWORD VARIABLE, AND NOT FINDING IT CONSTITUTES A BUG.  IT ALSO MEANS
C  THAT ONLY KTAB VALUES WHICH TAKEN OVER 1000 EQUAL INIT MAY BE CONSIDERED.
C  (THUS "STEPS", WHICH IS A MOTION VERB AS WELL AS AN OBJECT, MAY BE LOCATED 
C  AS AN OBJECT.)  AND IT ALSO MEANS THE KTAB VALUE IS TAKEN MOD 1000.
C 
      IMPLICIT NONE 
C 
      include 'voccom.fi'
      include 'ioccom.fi'
C 
      INTEGER*2 ID(8),INIT,V,I,J,INDEX
C 
      INDEX=0 
      IF(ID(1).EQ.8224.OR. ID(1).EQ.0) THEN 
        V=-1
        RETURN
      ENDIF 
C 
      DO I=1,TABSIZ 
        IF(KTAB(I).EQ.-1) THEN
          V=-1
          IF(INIT.GE.0) THEN
            WRITE(CRT,100) ID 
  100       FORMAT(8A2,"not found") 
            CALL BUG(5) 
          ENDIF 
          RETURN
        ENDIF 
        IF(INIT.LT.0 .OR. KTAB(I)/1000.EQ.INIT) THEN
          IF(ATAB(1,I).EQ.ID(1) .AND. ATAB(2,I).EQ.ID(2)) THEN
            DO J=3,8
              IF(ID(J).EQ.8224) GOTO 1000 
              IF(ID(J).NE.ATAB(J,I)) GOTO 500 
            ENDDO 
            GOTO 1000 
          ENDIF 
 500      CONTINUE
        ENDIF 
      ENDDO 
      CALL BUG(21)  
C 
 1000 V=KTAB(I) 
      IF(INIT.GE.0) V=MOD(V,1000) 
      INDEX=I 
C 
D     WRITE(CRT,1010) I,V,(ATAB(J,I),J=1,8) 
D1010 FORMAT(" VOCAB: INDEX:",I5," WORD:",I5,1X,8A2)  
C 
      RETURN
      END 
      SUBROUTINE DESTROY(OBJECT)
C 
C  PERMANENTLY ELIMINATE "OBJECT" BY MOVING TO A NON-EXISTENT LOCATION. 
C 
      IMPLICIT NONE 
C 
      INTEGER*2 OBJECT
C 
      CALL MOVE(OBJECT,0) 
      RETURN
      END 
      SUBROUTINE JUGGLE(OBJECT)
C 
C  JUGGLE AN OBJECT BY PICKING IT UP AND PUTTING IT DOWN AGAIN, THE PURPOSE 
C  BEING TO GET THE OBJECT TO THE FRONT OF THE CHAIN OF THINGS AT ITS LOC.
C 
      IMPLICIT NONE 
C 
      include 'placom.fi'
C 
      INTEGER*2 I,J,OBJECT
C 
      I=PLACE(OBJECT) 
      J=FIXED(OBJECT) 
      CALL MOVE(OBJECT,I) 
      CALL MOVE(OBJECT+100,J) 
      RETURN
      END 
      SUBROUTINE MOVE(OBJECT,WHERE)
C 
C  PLACE ANY OBJECT ANYWHERE BY PICKING IT UP AND DROPPING IT.  MAY ALREADY BE
C  TOTING, IN WHICH CASE THE CARRY IS A NO-OP.  MUSTN'T PICK UP OBJECTS WHICH 
C  ARE NOT AT ANY LOC, SINCE CARRY WANTS TO REMOVE OBJECTS FROM ATLOC CHAINS. 
C 
      IMPLICIT NONE 
C 
      include 'placom.fi'
C 
      INTEGER*2 OBJECT,WHERE,FROM 
C 
      IF(OBJECT.LE.100) THEN
        FROM=PLACE(OBJECT)
      ELSE
        FROM=FIXED(OBJECT-100)
      ENDIF 
      IF(FROM.GT.0.AND.FROM.LE.300) CALL CARRY(OBJECT,FROM) 
      CALL DROP(OBJECT,WHERE) 
C 
      RETURN
      END 
      INTEGER*2 FUNCTION PUT(OBJECT,WHERE,PVAL)
C 
C  PUT IS THE SAME AS MOVE, EXCEPT IT RETURNS A VALUE USED TO SET UP THE
C  NEGATED PROP VALUES FOR THE REPOSITORY OBJECTS.
C 
      IMPLICIT NONE 
C 
      INTEGER*2 OBJECT,WHERE,PVAL 
C 
      CALL MOVE(OBJECT,WHERE) 
      PUT=(-1)-PVAL 
      RETURN
      END 
      SUBROUTINE CARRY(OBJECT,WHERE)
C 
C  START TOTING AN OBJECT, REMOVING IT FROM THE LIST OF THINGS AT ITS FORMER
C  LOCATION.  INCR HOLDNG UNLESS IT WAS ALREADY BEING TOTED.  IF OBJECT>100 
C  (MOVING "FIXED" SECOND LOC), DON'T CHANGE PLACE OR HOLDNG. 
C 
      IMPLICIT NONE 
C 
      include 'placom.fi'
C 
      INTEGER*2 OBJECT,WHERE,TEMP 
C 
      IF(OBJECT.LE.100) THEN
        IF(PLACE(OBJECT).EQ.-1) RETURN  
        PLACE(OBJECT)=-1
        HOLDNG=HOLDNG+1 
      ENDIF 
      IF(ATLOC(WHERE).EQ.OBJECT) THEN 
        ATLOC(WHERE)=LINK(OBJECT) 
        RETURN
      ENDIF 
      TEMP=ATLOC(WHERE) 
 1000 IF(LINK(TEMP).EQ.OBJECT) GO TO 1010 
      TEMP=LINK(TEMP) 
      GOTO 1000 
 1010 LINK(TEMP)=LINK(OBJECT) 
C 
      RETURN
      END 
      SUBROUTINE DROP(OBJECT,WHERE)
C 
C  PLACE AN OBJECT AT A GIVEN LOC, PREFIXING IT ONTO THE ATLOC LIST.
C  DECREMENT HOLDNG IF THE OBJECT WAS BEING TOTED.
C 
      IMPLICIT NONE 
C 
      include 'placom.fi'
C 
      INTEGER*2 OBJECT,WHERE
C 
      IF(OBJECT.LE.100) THEN
        IF(PLACE(OBJECT).EQ.-1) HOLDNG=HOLDNG-1 
        PLACE(OBJECT)=WHERE 
      ELSE
        FIXED(OBJECT-100)=WHERE 
      ENDIF 
      IF(WHERE.LE.0) RETURN 
      LINK(OBJECT)=ATLOC(WHERE) 
      ATLOC(WHERE)=OBJECT 
C 
      RETURN
      END 
      SUBROUTINE ITEM(ID,END)
C 
C  REMOVE TRAILING BLANKS FROM ID AND ADD THE TWO CHARACTERS FROM 'END' 
C  TO THE END OF THE STRING. THEN DISPLAY ON CRT. 
C 
      IMPLICIT NONE 
C 
      include 'ioccom.fi'
C 
      INTEGER*2 I,ID(8),BUFFER(8),END,LAST
      INTEGER*2 TRIMLEN 
      CHARACTER*2 ADD 
      CHARACTER*16 STRING 
      EQUIVALENCE (BUFFER,STRING),(LAST,ADD)
C 
      DO I=1,8
        BUFFER(I)=ID(I)
      ENDDO
      LAST=END
      DO I=16,1,-1
        IF(STRING(I:I).NE.' ') GOTO 1000
      ENDDO
      CALL BUG(0)
 1000 STRING(I+1:I+2)=ADD(1:2)
      I=TRIMLEN(STRING)
      WRITE(CRT,1010) STRING(1:I)
 1010 FORMAT(A)
C
      RETURN
      END
      !FUNC TIMER WAS OF TYPE INTEGER*2, CHANGED TO REAL*4 - DWH
      REAL*4 FUNCTION TIMER(IFUNC)
C
C     ELAPSED TIME FUNCTION
C
C        TIME=TIMER(IFUNC)
C
C          IFUNC=0 START TIMER
C          IFUNC#0 RETURN ELAPSED TIME IN SECONDS
C
C     NOTE: IF THIS GAMES LASTS MORE THAN A YEAR, FORGET IT!
C
      IMPLICIT NONE

C
      INTEGER*2 ITIME(5),MSEC,SEC,MIN,HOUR,DAY
      INTEGER*2 SDAY,DAYS,IYEAR,SYEAR,IFUNC
C
      REAL*4 TIME,START
      SAVE START,SDAY,SYEAR !  THIS MUST BE STATIC! - DWH
C
      EQUIVALENCE (ITIME(1),MSEC),(ITIME(2),SEC),(ITIME(3),MIN),
     &            (ITIME(4),HOUR),(ITIME(5),DAY)
C
C  GET CURRENT FROM OPERATION SYSTEM AND CONVERT TO SECONDS
C
      CALL EXEC(11,ITIME,IYEAR)
      TIME=REAL(HOUR)*3600.+REAL(MIN)*60.+REAL(SEC)+REAL(MSEC)*.01
      IF(IFUNC.EQ.0) THEN
        START=TIME
        SDAY=DAY
        SYEAR=IYEAR
        TIMER=0.
      ELSE
        TIMER=TIME-START+(DAY-SDAY)*60*60*24
        IF(IYEAR.GT.SYEAR) THEN
          IF(MOD(IYEAR,4).EQ.0) THEN
            DAYS=366-SDAY+DAY
          ELSE
            DAYS=365-SDAY+DAY
          ENDIF
          TIMER=TIME-START+DAYS*60*60*24
        ENDIF
      ENDIF
C
      RETURN
      END
      LOGICAL*2 FUNCTION PRIMTIM(N)
C     &  ,Determine if Prime Time <250926.1734>
C
      IMPLICIT NONE
C
      include 'magcom.fi'
C
      INTEGER*2 I,N,HOUR,MIN,AMPM,IHRS,DAYMON,MONTH,DAYWK
      INTEGER*2 TIME(15),DAYWEEK(2),MON(2),MONTAB(2,12),DAYTAB(2,7)
      CHARACTER*30 CTIME
C
      EQUIVALENCE (TIME,CTIME)
C
      DATA MONTAB/16714,11854, 17734,11842, 16717,11858, 20545,11858, 16717,8281, 21834,17742,
     &            21834,22860, 21825,11847, 17747,11856, 17231,11860, 20302,11862, 17732,11843/
      DATA DAYTAB/21843,11854, 20301,11854, 21844,11845, 17751,11844, 18516,11861, 21062,11849,
     &            16723,11860/ 
C 
C  FTIME BUFFER FORMAT
C 
C     HH:MM PM  DAY., DD  MON., YEAR
C      1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
C 
      CALL FTIME(TIME)  
      READ(CTIME,10) HOUR,MIN,AMPM,DAYWEEK,DAYMON,MON 
   10 FORMAT(I2,1X,I2,1X,A2,2X,2A2,2X,I2,2X,2A2)
C 
C  FIND MONTH AND DAY OF WEEK 
C 
      MONTH=0 
      DO I=1,12 
        IF(MON(1).EQ.MONTAB(1,I) .AND.
     &     MON(2).EQ.MONTAB(2,I)) MONTH=I 
      ENDDO 
C 
      DAYWK=0 
      DO I=1,7
        IF(DAYWEEK(1).EQ.DAYTAB(1,I) .AND.
     &     DAYWEEK(2).EQ.DAYTAB(2,I)) DAYWK=I 
      ENDDO 
C 
C  OK TO PLAY SATURDAY AND SUNDAY 
C 
      IF(DAYWK.EQ.1 .OR. DAYWK.EQ.7) GO TO 20 
C 
C  OK TO PLAY JAN 1, JULY 4, AND DEC 25.
C 
      IF(MONTH.EQ.1 .AND. DAYMON.EQ.1) GO TO 20 
      IF(MONTH.EQ.7 .AND. DAYMON.EQ.4) GO TO 20 
      IF(MONTH.EQ.12 .AND. DAYMON.EQ.25) GO TO 20 
C 
C  OK TO PLAY MEMORIAL DAY AND LABOR DAY
C 
      IF(MONTH.EQ.5 .AND. DAYMON.GT.24 .AND. DAYWK.EQ.2) GO TO 20 
      IF(MONTH.EQ.9 .AND. DAYMON.LT.7 .AND. DAYWK.EQ.2) GO TO 20
C 
C  OK TO PLAY THANKSGIVING
C 
      IF(MONTH.EQ.11 .AND. DAYMON.GE.24 .AND. DAYWK.EQ.5) GO TO 20
C 
C  NOT A WEEKEND OR HOLIDAY, SO CHECK HOURS 
C 
      IF(IHOURS(1,1).EQ.-1) GO TO 20
      IF(HOUR.EQ.12) HOUR=0 
      IHRS=HOUR*60+MIN
      IF(AMPM.EQ.19792) IHRS=IHRS+12*60
      DO I=1,10 
        IF(IHOURS(1,I).LT.0) GO TO 30 
        IF(IHRS.GE.IHOURS(1,I) .AND.
     &     IHRS.LE.IHOURS(2,I)) GO TO 20
      ENDDO 
      GO TO 30
C 
C  NOT PRIME TIME, LET HIM PLAY FOR A WHILE 
C 
   20 PRIMTIM=.FALSE. 
      RETURN
C 
C  OH OH, MUST BE PRIME TIME, TELL HIM ABOUT IT 
C 
   30 PRIMTIM=.TRUE.
      RETURN
      END 
      SUBROUTINE HOURS
C 
      IMPLICIT NONE 
C 
      include 'ioccom.fi'
      include 'magcom.fi'
C 
      INTEGER*2 I,OPENHR,OPENMN,CLOSHR,CLOSMN,OAMPM,CAMPM,IHRS
C 
C  IF NO HOURS TELL HIM WERE OPEN ALL THE TIME
C 
      WRITE(CRT,10) 
   10 FORMAT(" ") 
      IF(IHOURS(1,1).EQ.-1) THEN
        CALL RSPEK(202) 
      ELSE
        DO I=1,10 
          IHRS=IHOURS(1,I)
          IF(IHRS.LT.0) RETURN
          IF(IHRS.EQ.1440) IHRS=0 
          IF(IHRS.GE.720) THEN
            IHRS=IHRS-720 
            OAMPM=19792
          ELSE
            OAMPM=19777
          ENDIF 
          OPENHR=IHRS/60
          OPENMN=IHRS-OPENHR*60 
          IF(OPENHR.EQ.0) OPENHR=12 
C 
          IHRS=IHOURS(2,I)
          IF(IHRS.EQ.1440) IHRS=0 
          IF(IHRS.GE.720) THEN
            IHRS=IHRS-720 
            CAMPM=19792
          ELSE
            CAMPM=19777
          ENDIF 
          CLOSHR=IHRS/60
          CLOSMN=IHRS-CLOSHR*60 
          IF(CLOSHR.EQ.0) CLOSHR=12 
C 
          WRITE(CRT,20) OPENHR,OPENMN,OAMPM,CLOSHR,CLOSMN,CAMPM 
   20     FORMAT(20X,"From ",I2,":",I2.2,1X,A2," to ",I2,":",I2.2,1X,A2)
        ENDDO 
      ENDIF 
C 
      RETURN
      END 
      SUBROUTINE SUSPEND
C 
      IMPLICIT NONE 
C 
      include 'ioccom.fi'
      include 'magcom.fi'
C 
      LOGICAL*2 PRIMTIM 
C 
      INTEGER*2 ELAPSE
C 
      REAL*4 SUSTIME,TIMER
C 
C  SAVE CURRENT TIME AND SUSPEND
C 
      CALL RSPEK(206)
      SUSTIME=TIMER(1)
   10 CALL EXEC(7)
C
C  A 'GO' WAS USED TO CONTINUE, HOW LONG WAS IT?
C
      ELAPSE=(TIMER(1)-SUSTIME)/60
C
C  IF IT WAS NOT LONG ENOUGH, SUSPEND HIM AGAIN, OTHERWISE CONTINUE
C
      IF(.NOT.WIZARD .AND. PRIMTIM(0) .AND. ELAPSE.LT.MAXSUS) THEN
        CALL MSPEK(8)
        GO TO 10
      ENDIF
C
C  ADD THE SUSPEND TIME TO THE AMOUNT OF PLAYING TIME LEFT
C
      AMOUNT=AMOUNT+ELAPSE
      IF(AMOUNT.GT.MAXAMT) AMOUNT=MAXAMT
C
      IF(ELAPSE.GT.3600.) CALL MSPEK(9)
      CALL RSPEK(207) 
C 
      RETURN
      END 
      LOGICAL*2 FUNCTION GUARD(N)
C     &  ,Guardian of Various Goodies <250926.1734>
C 
      IMPLICIT NONE 
C 
      include 'ioccom.fi'
      include 'miscom.fi'
C 
      INTEGER*2 PASS(6),PWORD,PASANS,I,N,RNDM 
C 
C  IF ALREADY PASSED TEST, THEN RETURN
C 
      IF(WIZARD) THEN 
        GUARD=.TRUE.
        RETURN
      ENDIF 
C 
C  CALCULATE PASSWORD NUMERICS  
C 
      DO I=1,6  
        PASS(I)=INT(RNDM(9))
      ENDDO 
      CALL MSPEK(33)
      WRITE(CRT,10) PASS
   10 FORMAT(/18X,6I1," 'Identify yourself.'"//)
C 
C  THE PASSWORD IS CALCULATED BY ADDING TOGETHER THE  
C  MIDDLE 4 DIGITS AND SUBTRACTING THE END 2  
C 
      PWORD=PASS(2)+PASS(3)+PASS(4)+PASS(5)-PASS(1)-PASS(6) 
C 
C  DISPLAY AND PROMPT FOR PASSWORD  
C 
      PASANS=0
      READ(KBD,*,ERR=20,END=20) PASANS 
   20 IF (PASANS.NE.PWORD) THEN 
        CALL MSPEK(34)
        GUARD=.FALSE. 
      ELSE
        GUARD=.TRUE.
        WIZARD=.TRUE. 
      ENDIF 
C 
      RETURN
      END 
      SUBROUTINE SCORING
C 
      IMPLICIT NONE 
C 
      include 'arycom.fi'
      include 'ioccom.fi'
      include 'miscom.fi'
      include 'placom.fi'
      include 'txtcom.fi'
C 
C  LOCAL VARIABLES
C 
      LOGICAL*2 TELL
      INTEGER*2 I,K
C     Statement-function dummy, only ever handed a literal.
      INTEGER*4 DUMMY
C 
      TELL(DUMMY)=(.NOT.(SCORNG.OR.GAVEUP).AND.CLOSNG)
C 
C  THIS CODE WAS MOVED FROM AMAIN TO HELP COMPILER AND LINKER OUT.
C 
C  EXIT CODE.  WILL EVENTUALLY INCLUDE SCORING.  FOR NOW, HOWEVER, ...
C 
C  THE PRESENT SCORING ALGORITHM IS AS FOLLOWS: 
C 
C     OBJECTIVE:          POINTS:        PRESENT TOTAL POSSIBLE:
C  GETTING WELL INTO CAVE   25                    25
C  EACH TREASURE < CHEST    12                    60
C  TREASURE CHEST ITSELF    14                    14
C  EACH TREASURE > CHEST    16                   208
C  SURVIVING             (MAX-NUM)*10             30
C  NOT QUITTING              4                     4
C  REACHING "CLOSNG"        25                    25
C  "CLOSED": QUIT/KILLED    10
C            KLUTZED        25
C            WRONG WAY      30
C            SUCCESS        45                    45
C  CAME TO WITT'S END        1                     1
C  SAW CHARON SAIL IN        5                     5
C  LOVED PRINCESS            3                     3
C  KILLING EACH DWARF        1                     5
C  ROUND OUT THE TOTAL       0                     0
C                                       TOTAL:   425
C 
C  (POINTS CAN ALSO BE DEDUCTED FOR USING HINTS.) 
C 
 2800 SCORE=0 
      MXSCOR=0
C 
C  FIRST TALLY UP THE TREASURES.  MUST BE IN BUILDING AND NOT BROKEN. 
C  GIVE THE POOR GUY 2 POINTS JUST FOR FINDING EACH TREASURE. 
C  TELL HOW MANY POINTS HE LOST IF HE REACHED CLOSING.
C 
      BLKLIN=.FALSE.
      WRITE(CRT,'(" ")')
      DO I=50,MAXTRS
        IF(PTEXT(I).NE.0) THEN
          K=12
          IF(I.EQ.CHEST) K=14 
          IF(I.GT.CHEST) K=16 
          IF(PLACE(I).EQ.3.AND.PROP(I).EQ.0) THEN 
            SCORE=SCORE+K 
          ELSEIF(PROP(I).GE.0) THEN 
            SCORE=SCORE+2 
            IF(TELL(0)) THEN
              WRITE(CRT,'("You did not not return the ",$)') 
              CALL PSPEK(I,-1)
              WRITE(CRT,'("  to the building, costing",I3," points.")') 
     &          K-2 
            ENDIF 
          ELSE
            IF(TELL(0)) THEN
              WRITE(CRT,'("You did not find the ",$)') 
              CALL PSPEK(I,-1)
              WRITE(CRT,'("  which cost you",I3," points.")') K 
            ENDIF 
          ENDIF 
          MXSCOR=MXSCOR+K 
        ENDIF 
      ENDDO 
      BLKLIN=.TRUE. 
C 
C  NOW LOOK AT HOW HE FINISHED AND HOW FAR HE GOT.  MAXDIE AND NUMDIE TELL US 
C  HOW WELL HE SURVIVED.  GAVEUP SAYS WHETHER HE EXITED VIA QUIT.  DFLAG WILL 
C  TELL US IF HE EVER GOT SUITABLY DEEP INTO THE CAVE.  CLOSNG STILL INDICATES
C  WHETHER HE REACHED THE ENDGAME.  AND IF HE GOT AS FAR AS "CAVE CLOSED" 
C  (INDICATED BY "CLOSED"), THEN BONUS IS ZERO FOR MUNDANE EXITS OR 133, 134, 
C  135 IF HE BLEW IT (SO TO SPEAK). 
C 
      SCORE=SCORE+(MAXDIE-NUMDIE)*10
      IF(.NOT.SCORNG .AND. NUMDIE.NE.0) THEN
        WRITE(CRT,'("You died",I2," times, costing you",I3," points.")')
     &     NUMDIE,NUMDIE*10 
      ENDIF 
      MXSCOR=MXSCOR+MAXDIE*10 
      IF(.NOT.(SCORNG.OR.GAVEUP)) THEN
        SCORE=SCORE+4 
      ELSE
        IF(.NOT.SCORNG) THEN
          WRITE(CRT,'("Giving up cost you 4 points.")') 
        ENDIF 
      ENDIF 
      MXSCOR=MXSCOR+4 
      IF(DFLAG.NE.0) THEN 
        SCORE=SCORE+25
      ELSE
        IF(.NOT.SCORNG) THEN
          WRITE(CRT,'("You did not get far enough into the cave costing"
     &       " you 25 points.")') 
        ENDIF 
      ENDIF 
      MXSCOR=MXSCOR+25
      IF(CLOSNG) THEN 
        SCORE=SCORE+25
      ELSE
        IF(.NOT.(SCORNG.OR.GAVEUP)) THEN
          WRITE(CRT,'("You did not reach closing time costing you", 
     &       " 25 points.")') 
        ENDIF 
      ENDIF 
      MXSCOR=MXSCOR+25
      K=SCORE 
      IF(CLOSED) THEN 
        IF(BONUS.EQ.0) SCORE=SCORE+10 
        IF(BONUS.EQ.135) SCORE=SCORE+25 
        IF(BONUS.EQ.134) SCORE=SCORE+30 
        IF(BONUS.EQ.133) SCORE=SCORE+45 
      ENDIF 
      IF(TELL(0) .AND. SCORE.NE.K+45) THEN
        WRITE(CRT,'("You lost",I3," points by not solving the final", 
     &    " puzzle.")') K+45-SCORE  
      ENDIF 
      MXSCOR=MXSCOR+45
C 
C  DID HE COME TO WITT'S END AS HE SHOULD?
C 
      IF(PLACE(MAGZIN).EQ.108) THEN 
        SCORE=SCORE+1 
      ELSE
        IF(.NOT.(SCORNG.OR.GAVEUP)) THEN
          WRITE(CRT,'("You lost 1 point for not comming to Witts End."
     &      )') 
        ENDIF 
      ENDIF 
      MXSCOR=MXSCOR+1 
C 
C  DID HE LOVE THE PRINCESS, BUT NOT KISS HER?
C 
      IF(PROP(41).EQ.2) THEN
        SCORE=SCORE+3 
      ELSE
        IF(TELL(0)) THEN
          WRITE(CRT,'("You lost 3 points for not loving princess and" 
     &      " kissing her.")')
        ENDIF 
      ENDIF 
      MXSCOR=MXSCOR+3 
C 
C  DID HE SEE CHARON COME INTO VIEW BUT NOT SAIL AWAY?
C 
      IF(PROP(CHARO).EQ.2) THEN 
        SCORE=SCORE+5 
      ELSEIF(PROP(CHARO).EQ.3) THEN 
        IF(.NOT.(SCORNG.OR.GAVEUP)) 
     &    WRITE(CRT,'("You lost 5 points for Charon sailing away",
     &      " leaving you behind.")') 
      ELSE
        IF(.NOT.(SCORNG.OR.GAVEUP)) 
     &    WRITE(CRT,'("You lost 5 points for not seeing Charon sail", 
     &      " into view.")')
      ENDIF 
      MXSCOR=MXSCOR+5 
C 
C  DID HE KILL EACH DWARF? (BUT NOT PIRATE) 
C 
      K=0 
      DO I=1,5
        IF(DLOC(I).EQ.0) THEN 
          SCORE=SCORE+1 
          K=K+1 
        ENDIF 
      ENDDO 
      IF(TELL(0) .AND. K.NE.5) THEN 
        I=6-K 
        WRITE(CRT,'("You lost",I2," points because you only killed",
     &    I2," Dwarfs.")') I,K
      ENDIF 
      MXSCOR=MXSCOR+5 
C 
C  ROUND IT OFF.
C 
      SCORE=SCORE+0 
      MXSCOR=MXSCOR+0 
C 
C  DEDUCT POINTS FOR HINTS.  HINTS < 4 ARE SPECIAL; SEE DATABASE DESCRIPTION. 
C 
      DO I=1,HNTMAX 
        IF(HINTED(I))  THEN 
          SCORE=SCORE-HINTS(I,2)
          IF(.NOT.(SCORNG.OR.GAVEUP)) THEN
            WRITE(CRT,'("You lost",I3," points for hint number",I2)') 
     &        HINTS(I,2),I
          ENDIF 
        ENDIF 
      ENDDO 
C 
C  RETURN TO SCORE COMMAND IF THAT'S WHERE WE CAME FROM 
C 
      IF(SCORNG) RETURN 
C 
C  THAT SHOULD BE ENOUGH.  LET'S TELL HIM ALL ABOUT IT
C 
      WRITE (CRT,2810 ) SCORE,MXSCOR,TURNS
 2810 FORMAT(/"You scored",I4," out of a possible",I4,
     &      " points, using",I5," turns.")
C 
      DO 2820  I=1,CLSSES 
        IF(CVAL(I).GE.SCORE) GO TO 2840 
 2820 CONTINUE
      WRITE (CRT,2830 ) 
 2830 FORMAT(/"You just went off my scale!!"/)
      GO TO 2900
C 
 2840 CALL SPEK(CTEXT(I)) 
      IF(I.EQ.CLSSES-1) GO TO 2880
      K=CVAL(I)+1-SCORE 
      WRITE (CRT,2850 ) K 
 2850 FORMAT(/"To achieve the next higher rating, you need",I3, 
     &      " more point",$) 
      IF (K.EQ.1) WRITE (CRT,2860 ) 
      IF (K.NE.1) WRITE (CRT,2870 ) 
 2860 FORMAT("."/)
 2870 FORMAT("s."/) 
      GO TO 2900
C 
 2880 WRITE (CRT,2890 ) 
 2890 FORMAT (/"To acheive the next higher rating ",
     &      "would be a neat trick!"//"Congratulations!!"/) 
C 
 2900 RETURN
      END 

