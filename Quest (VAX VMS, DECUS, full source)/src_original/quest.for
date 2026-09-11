*****
*
*  QUEST checks time restrictions, and does some terminal setups.
*
*****

	PROGRAM QUEST
	INCLUDE 'QSTCOM.FOR'
	CHARACTER SETUP*12,START_TIME*5,END_TIME*5
	INTEGER UP,DAY
	EXTERNAL QUEST_ERROR

	CALL LIB$ESTABLISH(QUEST_ERROR)
	CALL LIB$DAY(DAY)
	DAY=MOD(DAY,7)
	CALL TIME(TIM)
	CALL USERINFO(UIC,USERNAME)
	OPEN(UNIT=21,FILE='BSU$USER_2:[00CKKELLE.QUEST]ACCESS.FIL',
     +STATUS='OLD',FORM='FORMATTED',CARRIAGECONTROL='LIST',READONLY)
	READ(21,FMT='(I1,2A5)') UP,START_TIME,END_TIME
	CLOSE(UNIT=21)
	SETUP(1:1)=CHAR(27)
	SETUP(2:2)='>'
	SETUP(3:3)=CHAR(27)
	SETUP(4:4)='<'
	SETUP(5:5)=CHAR(27)
	SETUP(6:9)='[11m'
	SETUP(10:10)=CHAR(27)
	SETUP(11:12)='[m'
	WRITE(5,FMT='(1X,A12)') SETUP

	CALL CLEARSCREEN
	IF(UP.NE.0)THEN
	  CALL WRAITH(2)
	  CALL EXITR
	ELSE IF((TIM.GT.START_TIME.AND.TIM.LT.END_TIME).AND.
     +(DAY.NE.3.AND.DAY.NE.4))THEN
	  CALL WRAITH(1)
	  CALL EXITR
	  ENDIF

	CALL FORMAT(3,'Jim, the wizard, appears before you.!/
     +He unravels a scroll and begins to recite it.!/!/
     +Version 1.00 December 16, 1984.  Version 3.36 April 27, 1985.
     +!/!/QUEST - written by Chris Kelley!/!/!/!/!/!/!/!/!/
     +Special thanks to D.J. Kleiman!/
     +This game is dedicated to Robert C. Matney!_!_')
	CALL BAS$SLEEP(%VAL(4))
	PLAYER=' '
	PLAYER(252:252)='@'
	CALL PUTTEMPCORE(PLAYER)
	CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
	END




*****
*
*  WRAITH allows quest to print error messages.
*
*****

	SUBROUTINE WRAITH(I)
	INTEGER I

	CALL FORMAT(5,'You have approached the gates to the city of
     + Exeter.!/They are securely locked. A small man in a grey robe
     + appears from out!/of the mist. He is holding a tastefully
     + lettered sign which reads:!/')

	IF(I.EQ.2)THEN
	  CALL FORMAT(2,'Powers greater than mine prevent me from
     + letting you adventure!/in my world at this time. Please try
     + later.')
	ELSE IF(I.EQ.1)THEN
	  CALL FORMAT(2,'Quest is available before 8:00 a.m. and after!/
     +8:00 p.m. It is also available all day Saturday and Sunday.!/')
	  ENDIF

	CALL FORMAT(3,'The little man in the grey robes fades back
     + into the mist.')
	CALL EXITR
	END


