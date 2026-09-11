	INCLUDE 'QSTCOM.FOR'
	EXTERNAL QUEST_ERROR

	CALL LIB$ESTABLISH(QUEST_ERROR)
	SEED1=INT(SECNDS(0.0))
	SEED1=SEED1.OR.1

	CALL GETTEMPCORE(PLAYER)
	CALL ASCIITONUMERIC
	CALL ARMOR
	CALL SPELL(SPELLS)

1	CALL CLEARSCREEN
	CALL FORMAT(3,'Welcome to the fair city of Exeter.!/')

	I=0
	DO WHILE(I.NE.81)

	CALL TIME(TIM)
	CALL FORMAT(4,'Exeter time is ')
	CALL FORMAT(0,TIM)

	CALL FORMAT(3,'Option ("H" for Help) ?   ')

	CALL INPUT(I,1)

	IF(I.EQ.72.OR.I.EQ.0)THEN
	  CALL FORMAT(0,'Help!/!/
     +D - dungeon adventuring!/
     +C - visit the cleric!/
     +M - visit the magic shop!/
     +X - current statistics!/
     +Z - personal statistics!/
     +I - inventory!/
     +Q - stop adventuring')
	ELSE IF(I.EQ.68)THEN
	  CALL FORMAT(0,'Explore the dungeons')
	  CALL NUMERICTOASCII
	  CALL PUTTEMPCORE(PLAYER)
	  CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST3.Q7R')
	ELSE IF(I.EQ.67.OR.I.EQ.86)THEN
	  CALL FORMAT(0,'Visit the cleric')
	  CALL CLERIC
	ELSE IF(I.EQ.77)THEN
	  CALL FORMAT(0,'Visit the magic shop')
	  CALL SHOP
	ELSE IF(LIFE.EQ.0)THEN
	  CALL DEATH
	  ENDIF

	ENDDO

	CALL FORMAT(0,'Stop adventuring')
	RUN=0
	CALL NUMERICTOASCII
	CALL OPENCHARFILE
	CALL REPLACEPLAYER(ERR,NAME)
	CALL CLOSEFILE(21)
	PLAYER=' '
	PLAYER(252:252)='@'
	CALL PUTTEMPCORE(PLAYER)
	CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
	END




*****
*
*  SHOP is the magic user's shop.
*
*****

	SUBROUTINE SHOP
	INCLUDE 'QSTCOM.FOR'
	INTEGER I,J


	CALL CLEARSCREEN
	CALL FORMAT(2,'You have traveled across town to a quaint villa.
     + A magic shop is!/in sight. You enter and are greeted by Jim,
     + the owner.!/')

1	CALL FORMAT(4,'You are in the magic shop.!/
     +What may I do for you ("H" for Help) ?   ')
	CALL INPUT(I,0)

	IF(I.EQ.72)THEN
	  CALL FORMAT(0,'Help!/!/
     +B - buy a magic item!/
     +R - have a magic item recharged!/
     +L - leave the shop!/
     +S - sell a magic item')
	  GOTO 1
	ELSE IF(I.EQ.66)THEN
	  CALL FORMAT(0,'Buy a magic item')
	  CALL CLEARSCREEN
	  CALL BUY
	ELSE IF(I.EQ.82)THEN
	  CALL FORMAT(0,'Recharge an item')
	  CALL CLEARSCREEN
	  CALL RECHARGE
	ELSE IF(I.EQ.83)THEN
	  CALL FORMAT(0,'Sell a magic item')
	  CALL CLEARSCREEN
	  CALL SELL
	ELSE IF(I.EQ.76)THEN
	  CALL FORMAT(0,'Leave the shop')
	  RETURN
	  ENDIF
	GOTO 1

	END



*****
*
*  BUY allows the user to buy a magic item.
*
*****

	SUBROUTINE BUY
	INCLUDE 'QSTCOM.FOR'
	INTEGER I

	IF(FINDMAGIC(0).LT.2.AND.CHARLVL.LT.9)THEN
	  CALL FORMAT(4,'You have too many magic items to buy any
     + more!!')
	  RETURN
	  ENDIF

	I=CHARLVL*1500+((CHARLVL-1)*(DICE(1,3000)+2000))
	IF(CHARLVL.GT.2)I=I+CHARLVL*3000

	J=STATS(6)-12
	DO K=1,J
	  I=INT(0.96*FLOAT(I))
	ENDDO

	IF(GOLD.LT.I)THEN
	  CALL FORMAT(3,'I''m sorry. You do not have the ')
	  CALL OUTNUM(I)
	  CALL FORMAT(0,' gold pieces necessary to buy!/a magic
     + item!!')
	  RETURN
	  ENDIF

2	CALL FORMAT(3,'It will cost you ')
	CALL OUTNUM(I)
	CALL FORMAT(0,' gold pieces to buy a magic item.!/
     +Would you like to take a chance?  ')

	CALL INPUT(J,2)
	IF(J.EQ.78.OR.J.EQ.0)THEN
	  RETURN
	ELSE IF(J.NE.89)THEN
	  GOTO 2
	  ENDIF

	GOLD=GOLD-I
	CALL LOCATEMAGIC(-1)
	CALL ARMOR
	RETURN
	END


*****
*
*  SELL allows the player to sell magic items to the Magician.
*
*****

	SUBROUTINE SELL
	INCLUDE 'QSTCOM.FOR'
	INTEGER PRICE(91)

	DATA PRICE/2000,2000,2000,4000,4000,4000,8000,8000,8000,
     +10000,10000,10000,0,0,0,0,0,0,2000,4000,8000,10000,0,20000,
     +50000,2000,2500,5000,0,0,5000,12500,10000,0,0,15000,0,0,
     +2500,5000,6000,4500,0,0,7500,11000,10000,0,2500,0,8000,5000,
     +2500,10000,0,0,6000,0,7500,7500,7500,7500,0,10000,2000,5000,
     +0,0,1500,5000,10000,15000,2500,10000,15000,10000,0,0,0,9000,
     +60000,60000,60000,60000,0,12500,4000,10000,16000,16000,16000/

	IF(CHARLVL.LT.2)THEN
	  CALL FORMAT(4,'I''m sorry. I cannot deal with characters of
     + such little experience.')
	  RETURN
	  ENDIF

	CALL LISTMAGIC
	CALL FORMAT(3,'Number of the item you wish to sell?  ')
	CALL INPUTNUMBER(I)

	IF(I.LT.1.OR.I.GT.MAGICPERLEVEL(CHARLVL))RETURN
	IF(MAGIC(I).EQ.0)RETURN
	J=PRICE(MAGIC(I))

	IF(J.EQ.0)THEN
	  CALL FORMAT(3,'I don''t buy cursed magic items!!!!!/')
	  RETURN
	  ENDIF

10	CALL FORMAT(3,'I will give you ')
	CALL OUTNUM(J)
	CALL FORMAT(0,' gold pieces.!/Do you wish to sell it?   ')
	CALL INPUT(K,2)
	IF(K.EQ.89)THEN
	  GOLD=GOLD+J
	  MAGIC(I)=0
	  CALL FORMAT(3,'You have sold that item.')
	  RETURN
	ELSE IF(K.EQ.78)THEN
	  CALL FORMAT(3,'You have kept the item.')
	  RETURN
	ELSE
	  GOTO 10
	  ENDIF

	END

	
*****
*
*  RECHARGE allows the player to have magic items recharged (for a nominal
*  fee).
*
*****

	SUBROUTINE RECHARGE
	INCLUDE 'QSTCOM.FOR'

	CALL LISTMAGIC
	CALL FORMAT(3,'Number of the item you wish to have recharged?   ')
	CALL INPUTNUMBER(I)
	IF(I.LT.1.OR.I.GT.MAGICPERLEVEL(CHARLVL))RETURN
	IF(MAGIC(I).EQ.0)RETURN
	CALL FORMAT(3,'Amount of gold you wish to donate?  ')
	CALL INPUTNUMBER(J)
	IF(J.LT.1.OR.J.GT.GOLD)THEN
	  CALL FORMAT(3,'That is not possible!!')
	  RETURN
	  ENDIF

	GOLD=GOLD-J
	IF(MAGIC(I).EQ.52)THEN
	  K=J/1000
	  PROPERTIES(I)=PROPERTIES(I)+K
	  IF(PROPERTIES(I).GT.4)PROPERTIES(I)=4
	ELSE IF(MAGIC(I).EQ.74)THEN
	  K=J/500
	  PROPERTIES(I)=PROPERTIES(I)+K
	  IF(PROPERTIES(I).GT.30)PROPERTIES(I)=30
	ELSE IF(MAGIC(I).EQ.76)THEN
	  K=J/750
	  PROPERTIES(I)=PROPERTIES(I)+K
	  IF(PROPERTIES(I).GT.18)PROPERTIES(I)=18
	ELSE IF(MAGIC(I).EQ.54)THEN
	  K=J/400
	  PROPERTIES(I)=PROPERTIES(I)+K
	  IF(PROPERTIES(I).GT.99)PROPERTIES(I)=99
	ELSE
	  K=J/350
	  PROPERTIES(I)=PROPERTIES(I)+K
	  IF(PROPERTIES(I).GT.99)PROPERTIES(I)=99
	  ENDIF
	CALL FORMAT(3,'Jim takes the item and disappears into the back
     +room.!/He returns shortly and hands you the item.!/')

	RETURN
	END


*****
*
*  CLERIC contains the friendly cleric routine.
*
*****

	SUBROUTINE CLERIC
	INCLUDE 'QSTCOM.FOR'

	CALL CLEARSCREEN
	CALL FORMAT(2,'You have traveled across town to the temple
     + of Ra.!/Ahman, resident cleric, will help you for a small
     + donation to his church.!/')

1	CALL FORMAT(4,'How much gold would you like to donate? ')
	CALL INPUTNUMBER(I)
	IF(I.LT.1)RETURN
	IF(GOLD.LT.I)THEN
	  CALL FORMAT(5,'You only have ')
	  CALL OUTNUM(GOLD)
	  CALL FORMAT(0,' gold pieces.')
	  GOTO 1
	  ENDIF

	GOLD=GOLD-I
2	CALL FORMAT(4,'What may I do for you ("H" for Help) ?  ')
	CALL INPUT(J,0)

	IF(J.EQ.72.OR.J.EQ.0)THEN
	  CALL FORMAT(0,'Help!/!/
     +W - have wounds healed!/
     +D - have a disease cured!/
     +R - have a cursed magic item removed')
	ELSE IF(J.EQ.87.OR.J.EQ.67)THEN
	  CALL FORMAT(0,'Cure wounds!/!/!/
     +For your donation, I will heal some of your wounds.')
	  I=I/(CHARLVL*5)
	  HITPOINTS=HITPOINTS+I
	  IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
	  GOTO 5
	ELSE IF(J.EQ.68)THEN
	  CALL FORMAT(0,'Cure a disease')
	  IF(I.LT.250*CHARLVL)THEN
	    CALL FORMAT(4,'I can''t cure you for such a paltry
     + sum!!!!')
	    RETURN
	    ENDIF
	  DISEASE=0
	  CALL FORMAT(4,'You have been cured of any disease that
     + you might have contracted.')
	  GOTO 5
	ELSE IF(J.EQ.82)THEN
	  CALL FORMAT(0,'Remove a cursed item')
	  IF(I.LT.1000)THEN
	    CALL FORMAT(4,'I require a larger donation for such
     + dangerous work!!!!')
	    RETURN
	    ENDIF
	  CALL LISTMAGIC
	  I=MAGICPERLEVEL(CHARLVL)
	  CALL FORMAT(3,'Number of the item you wish to be rid of ?  ')
	  CALL INPUTNUMBER(J)
	  IF(J.LT.1.OR.J.GT.I)RETURN
	  IF(MAGIC(J).EQ.0)RETURN
	  MAGIC(J)=0
	  CALL FORMAT(4,'You are free of that item.')
	  CALL ARMOR
	  GOTO 5
	  ENDIF
	GOTO 2

5	CALL FORMAT(2,'You thank Ahman and leave the temple.')
	RETURN
	END
