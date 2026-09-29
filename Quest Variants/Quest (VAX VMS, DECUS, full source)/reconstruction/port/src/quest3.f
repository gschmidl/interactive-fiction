*****
*
*  Dungeon data for Quest:
*
*
*  Location data:
*
*  0  - Nothing
*  1  - Fountain
*  2  - Teleporter, same level same dungeon
*  3  - Teleporter, any level same dungeon
*  4  - Teleporter, any dungeon any level
*  5  - Throne
*  6  - Pool
*  7  - Pit
*  8  - Gas Cloud, Sleep
*  9  - Gas Cloud, Poison damage
*  10 - Gas Cloud, Death
*  11 - Gas Cloud, Fog (lose way)
*  12 - Stream, prevent travel North
*  13 - Stream, prevent travel East
*  14 - Stream, prevent travel South
*  15 - Stream, prevent travel West
*  16 - Disappearing stairs up
*  17 - Disappearing stairs down
*  18 - Stairs Up
*  19 - Stairs Down
*  20 - 80% chance of random monster
*  21 - 50% chance per round of treasure
*  22 - Never meet monster
*  23 - Never locate treasure
*  24 - Not used
*  25 - Magic 3 times more often than usual
*  26 - No magic ever at this location
*
*
*  Wall codes for Quest:
*
*  0 - No wall
*  1 - Wall
*  2 - Doorway  where (North,South)='MM---MM'   (East,West)='!'
*  3 - Archway  where (Archway)='MM   MM'
*  4 - Secret door
*  5 - Iron Gate
*  6 - Brass Gate
*  7 - Steel Gate
*  8 - Disappearing Wall
*
*
*  Note: Quest stores dungeon maps in integer format where the information
*  stored for each location consists of:
*
*  A 2 digit number indication the object at current location
*  A 1 digit number indicating the North wall
*  A 1 digit number indicating the West wall
*
*  Example:   MMMMMMM        would be stored as    01    Fountain
*             M FNT                                  1   Wall to North
*             !                                       2  Door to West
*             M                                   ------
*             M                                    0112  Code for this location
*
*
*             MM   MM        would be stored as    04    Teleporter (anywhere)
*             M TEL                                  3   Archway
*             :                                       6  Brass gate 
*             M                                          (All gates look alike)
*             M                                   ------
*                                                  0436  Code for this location
*
*
*             MM---MM        would be stored as    08    Sleep cloud
*              (  ))                                 1   Door to the North
*               ( )                                   0  Nothing to the West
*               (( )                              ------
*                                                  0810  Code for this location
*
*  Etcetera.....
*
*  All information needed to build EAST walls is taken from the WEST wall of
*  the location to your East. Information to build SOUTH walls is taken from
*  the NORTH wall of the location to your South.
*  
*****
*
*  QUEST3 is the driver routine for dungeon adventuring.
*
*****

      SUBROUTINE QUEST3
      INCLUDE 'qstcom.inc'
      EXTERNAL QUEST_ERROR

      SEED1=INT(VSECND(0.0))
      SEED1=IOR(SEED1,1)
      CALL GETTEMPCORE(PLAYER)
      CALL ASCIITONUMERIC
      CALL ARMOR
      CALL CHECKLEVEL
      CALL CLEARSCREEN
      CALL GETMON
      CALL RUNIT

      RETURN
      END





*****
*
*  RUNIT handles entering the dungeon by the player.
*
*****

      SUBROUTINE RUNIT
      INCLUDE 'qstcom.inc'
      CHARACTER DUNNAM(6)*37,CHOICE*5
      DATA COLOR/'Purple','Orange','Yellow','Green','Blue','Red',
     +'Clear','Slimy','Golden','Black','Pink'/

      CALL OPENDUNGEON
      EXPERT=0
      IF(DUNGEON.NE.0)THEN
        CALL SINGLE(7)
        CALL FORMAT(9,'!_!_Thus endeth time stop.')
        IF(DUNLVL.LT.1.OR.DUNLVL.GT.8)DUNLVL=1
        IF(SPELLTOREGEN.LT.1.OR.SPELLTOREGEN.GT.5)SPELLTOREGEN=1
        CALL GETDUNGEON(DUNGEON,DUNLVL)
        IF(XCOORD.LT.1.OR.XCOORD.GT.LEVELLENGTH)XCOORD=STAIRSUPX
        IF(YCOORD.LT.1.OR.YCOORD.GT.LEVELWIDTH)YCOORD=STAIRSUPY
      ELSE
        PROTEVIL=0
        BLINK=0
        CALL SPELL(SPELLS)
        SPELLTOREGEN=1
        SPELLREGEN=0
        HPREGEN=0
5       CALL FORMAT(0,'There are numerous dungeons and passages
     + beneath the city of!/Exeter, accessable through gratings,
     + basements of unsuspecting!/shopkeepers, and abandoned abodes.
     + Which one would you like to explore now?!/!/Places of
     + adventuring are:!/!/')
        CALL OPENDUNNAM
        I=1
        J=0
        DO WHILE(.TRUE.)
          READ(21,FMT='(A37)',ERR=2,END=2) DUNNAM(I)
          CALL TRIMMER(DUNNAM(I))
          I=I+1
          J=J+1
          CALL TTYNL(1)
        ENDDO
2       CALL CLOSEFILE(21)
        CALL FORMAT(3,'Name of the dungeon you wish to enter: _____')
        DO N1=1,5
          CALL SINGLE(8)
        ENDDO
        CALL ASCII(CHOICE,1)
        I=1
3       IF(DUNNAM(I)(1:5).EQ.CHOICE)GOTO 4
        I=I+1
        IF(I.LE.J)GOTO 3
        CALL FORMAT(4,'That dungeon not found!!!!')
        CALL QSLEEP(1)
        CALL CLEARSCREEN
        GOTO 5

4       IF(I.GT.4.AND.CHARLVL.LT.5)THEN
          CALL FORMAT(3,'Jim, the wizard, appears before you.!/!/
     +"I cannot let such an inexperienced player into so difficult
     + a dungeon."')
          CALL QSLEEP(5)
          CALL CLEARSCREEN
          GOTO 5
          ENDIF
        DUNLVL=1
        DUNGEON=I
        CALL GETDUNGEON(DUNGEON,DUNLVL)
        XCOORD=STAIRSUPX
        YCOORD=STAIRSUPY
        ENDIF

      CALL MAINADVENTURE
      END



*****
*
*  OPENDUNGEON opens the dungeon data file for direct access input.
*
*****

      SUBROUTINE OPENDUNGEON

      CHARACTER QFN*256
      CALL QPATH('dungeon.dta',QFN)
      OPEN(UNIT=24,FILE=QFN,STATUS='OLD',ACCESS='DIRECT',RECL=4,
     +FORM='FORMATTED',ACTION='READ')

      RETURN
      END





*****
*
*  OPENDUNNAM opens the file containing the names of the dungeons.
*
*****

      SUBROUTINE OPENDUNNAM

      CHARACTER QFN*256
      CALL QPATH('dunnam.dta',QFN)
      OPEN(UNIT=21,FILE=QFN,STATUS='OLD',FORM='FORMATTED',ACTION='READ')

      RETURN
      END




*****
*
*  MAINADVENTURE controls the character's wandering through the dungeon.
*
*****

      SUBROUTINE MAINADVENTURE
      INCLUDE 'qstcom.inc'
      INTEGER RESULT

1     PRINTFLAG=0
      PRINTCONTROL=0
      MOVES=MOVES+1
      IF(MOVES.GT.365)THEN
        MOVES=1
        AGE=AGE+1
        IF(AGE.GT.80+STATS(4))THEN
          CALL FORMAT(5,' Jim, the wizard, appears before you.!/!/
     +"You have grown very old. Your venerable character must now!/
     +retire...........forever!!"')
          GOTO 999
          ENDIF
        ENDIF

      IF(DISEASE.GT.0.AND.DICE(1,4).EQ.1)THEN
        DISEASE=DISEASE+1
        IF(DISEASE.GT.STATS(4))THEN
          CALL FORMAT(3,'You could not get out of the dungeon in
     + time. Your!/disease has killed you!!!!')
          GOTO 999
          ENDIF
        ENDIF

      HITPOINTS=HITPOINTS+FINDMAGIC(24)
      IF(FINDMAGIC(51).GT.0)CALL ADDGOLD(DICE(1,25))
      IF(FINDMAGIC(79).GT.0.AND.DICE(1,3).EQ.1)HITPOINTS=HITPOINTS-1
      IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
      IF(HITPOINTS.LT.1)GOTO 999
      IF(PROTEVIL.GT.0)THEN
        PROTEVIL=PROTEVIL-1
        IF(PROTEVIL.EQ.0)CALL ARMOR
        ENDIF
      IF(BLINK.GT.0)BLINK=BLINK-1

      CALL CLEARSCREEN
      CALL BUILDMAP

      I=2
      IF(FINDMAGIC(26).GT.0)I=8
      IF(FINDMAGIC(39).GT.0)I=I/2
      IF(LOCATION.EQ.20)I=8
      J=0
      IF(DICE(1,10).LE.I.AND.LOCATION.NE.22)THEN
        RESULT=0
        CALL MONSTR(RESULT)
        IF(LIFE.EQ.0)GOTO 999
        IF(RESULT.EQ.2)GOTO 1
        J=J+1
        IF(RESULT.EQ.60)THEN
          I=60
          GOTO 3
          ENDIF
        ENDIF


      I=9
      IF(LOCATION.EQ.21)I=50
3     IF(DICE(1,100).LE.I.AND.LOCATION.NE.23)THEN
        PRINTFLAG=1
        CALL FORMAT(3,'You have thoroughly searched the area.')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        CALL TREASURE(I)
        CALL ARMOR
        CALL ADDGOLD(I)
        IF(LIFE.EQ.0)GOTO 999
      ELSE IF(J.EQ.1)THEN
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        ENDIF

      IF((LOCATION.GT.0.AND.LOCATION.LT.20).OR.(LOCATION.GT.26))THEN
        PRINTCONTROL=1
        CALL SPECIALROOM
        CALL ARMOR
        IF(LIFE.EQ.0)GOTO 999
        IF(FLAG.EQ.1)GOTO 1
        ENDIF

      IF(PRINTFLAG.EQ.0)CALL QPRINT
      CALL DIRECTION
      IF(LIFE.NE.0)GOTO 1

999   CALL DEATH
      DUNGEON=DICE(1,6)
      DUNLVL=DICE(1,7)
      CALL GETDUNGEON(DUNGEON,DUNLVL)
      XCOORD=DICE(1,LEVELLENGTH)
      YCOORD=DICE(1,LEVELWIDTH)
      GOTO 1

      END




*****
*
*  SPECIALROOM contains the routines for handling rooms with things in them.
*
*****

      SUBROUTINE SPECIALROOM
      INCLUDE 'qstcom.inc'

      FLAG=0
      IF(LOCATION.EQ.1)THEN
        CALL FOUNTAIN
      ELSE IF(LOCATION.GE.2.AND.LOCATION.LE.4)THEN
        CALL TELEPORTER
      ELSE IF(LOCATION.EQ.5)THEN
        CALL THRONE
      ELSE IF(LOCATION.EQ.6)THEN
        CALL POOL
      ELSE IF(LOCATION.EQ.7)THEN
        CALL PIT
      ELSE IF(LOCATION.GE.8.AND.LOCATION.LE.11)THEN
        CALL CLOUD(LOCATION-7)
      ELSE IF(LOCATION.GE.12.AND.LOCATION.LE.15)THEN
        PICTURE(6)(10:12)='/ /'
        PICTURE(7)(9:11)='/X/'
        PICTURE(8)(8:10)='/ /'
        PRINTFLAG=1
        CALL QPRINT
      ELSE IF(LOCATION.EQ.16)THEN
        IF(DICE(1,5).GT.2)THEN
          PRINTCONTROL=0
        ELSE
          PRINTFLAG=1
          CALL STAIRSUP
          ENDIF
      ELSE IF(LOCATION.EQ.17) THEN
        IF(DICE(1,5).GT.2)THEN
          PRINTCONTROL=0
        ELSE
          PRINTFLAG=1
          CALL STAIRSDOWN
          ENDIF
      ELSE IF(LOCATION.EQ.18)THEN
        CALL STAIRSUP
      ELSE IF(LOCATION.EQ.19)THEN
        CALL STAIRSDOWN
      ELSE IF(LOCATION.EQ.27.AND.CHARLVL.GT.5)THEN
        CALL DRAGON
        ENDIF
      IF((LOCATION.GT.0.AND.LOCATION.LT.16).OR.(LOCATION.EQ.18.OR.
     +LOCATION.EQ.19))PRINTFLAG=1

      RETURN
      END




*****
*
*  MONSTR handles the encounter of a monster.
*
*****

      SUBROUTINE MONSTR(RESULT)
      INCLUDE 'qstcom.inc'
      INTEGER UNDEAD(10),RESULT,WHOSETURN,TURNED,UNDEAD_LOOKUP(10,12)
      INTEGER BACKSTAB
      DATA UNDEAD/12,24,28,35,50,61,73,87,88,100/
      DATA UNDEAD_LOOKUP/10,13,16,19,20,21,21,23,24,26,
     +7,10,13,16,19,20,21,21,23,24,
     +4,7,10,13,16,19,20,21,21,23,
     +0,4,7,10,13,16,19,20,21,21,
     +0,0,4,7,10,13,16,19,20,21,
     +-1,0,0,4,7,10,13,16,19,20,
     +-1,-1,0,0,4,7,10,13,16,19,
     +-1,-1,-1,0,0,4,7,10,13,16,
     +-1,-1,-1,-1,0,0,4,7,10,13,
     +-1,-1,-1,-1,-1,0,0,4,7,10,
     +-1,-1,-1,-1,-1,-1,0,0,4,7,
     +-1,-1,-1,-1,-1,-1,-1,0,0,4/

      BACKSTAB=0
      IF(CLASS.EQ.7.AND.DICE(1,3).EQ.1)BACKSTAB=DICE(1,3)+1
      TURNED=0
      CALL SYMBOL(2)
      CALL QPRINT
      IF(FINDMAGIC(63).GT.0.AND.DICE(1,3).EQ.1)THEN
        MONNUM=UNDEAD(DICE(1,3))
        IF(DICE(1,9).LE.DUNLVL)MONNUM=UNDEAD(DICE(1,10))
      ELSE IF(FINDMAGIC(73).GT.0.AND.DICE(1,3).EQ.1)THEN
        MONNUM=12
      ELSE IF(FINDMAGIC(77).GT.0.AND.DICE(1,3).EQ.1)THEN
        MONNUM=85
      ELSE
1       MONNUM=DICE(1,DUNLVL*14)
        IF(DICE(1,90).EQ.1)MONNUM=115
        IF((DICE(1,60).EQ.1).OR.(FINDMAGIC(60).GT.0.AND.DICE
     +(1,8).EQ.1))MONNUM=113
        IF(DICE(1,70).EQ.1.AND.HITPOINTS.NE.TOTALHITPOINTS.AND.
     +DUNLVL.NE.1)MONNUM=114
        IF(DICE(1,100).LT.4+INT(FLOAT(CHARLVL)/1.5))MONNUM=DICE(1,113)
        IF(MONNUM.EQ.115.AND.FINDMAGIC(59).GT.0)GOTO 1
        ENDIF
      CALL RETRIEVEMONSTER(MONNUM)

2     CALL FORMAT(3,'You have encountered ')
      IF(INDEX('AEIOU',MONNAM(1:1)).NE.0)THEN
        CALL FORMAT(0,'an ')
      ELSE
        CALL FORMAT(0,'a ')
        ENDIF
      CALL TRIMMER(MONNAM)
      IF(MONNUM.EQ.115)CALL FORMAT(3,'He has turned invisible.')

      IF(FINDMAGIC(55).GT.0)THEN
        CALL FORMAT(4,'You are confused. You have decided to ')
        I=DICE(1,3)
        GOTO(77,9,10) I
        ENDIF

      IF(MONUNDEADMAGIC.EQ.1.OR.MONUNDEADMAGIC.EQ.3)THEN
        IF(PROTEVIL.GT.0)GOTO 50
        IF(FINDMAGIC(64).EQ.0)GOTO 3
        IF(DICE(1,10).EQ.1)GOTO 8
        CALL FORMAT(5,'It has turned to ashes before you!!!!')
        GOTO 35
        ENDIF

3     IF(MONSA1.GT.0)THEN
        CALL SPECMON(MONSA1)
        IF(LIFE.EQ.0.OR.FLAG.EQ.1)RETURN
        IF(FLAG.EQ.2)GOTO 8
        ENDIF

4     CALL FORMAT(3,'What would you like to do ("H" for Help) ?   ')
      CALL INPUT(I,1)

      IF(I.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +F - fight!/
     +R - run away!/
     +K - fight to the death!/
     +C - cast a spell!/
     +M - manipulate an item!/
     +X - current statistics!/
     +Z - personal statistics!/
     +I - inventory!/')
        IF(CLASS.EQ.8)CALL FORMAT(0,'T - turn undead!/')
      ELSE IF(I.EQ.70.OR.I.EQ.0.OR.I.EQ.55)THEN
77      CALL FORMAT(0,'Fight!/')
8       IF(MONNUM.EQ.115)THEN
          CALL FORMAT(4,'It is changing form !!!!')
          GOTO 1
        ELSE
          WHOSETURN=1
          IF(STATS(5).GT.MONDXTRTY.OR.FINDMAGIC(27).GT.0)GOTO 22
          GOTO 21
          ENDIF
      ELSE IF(I.EQ.82.OR.I.EQ.53)THEN
9       CALL FORMAT(0,'Run away')
        I=STATS(5)*12-MONDXTRTY*3-NUMBEROFWALLS*4
        IF(CLASS.EQ.7)I=I+10
        IF(FINDMAGIC(58).GT.0)I=I-15
        IF(FINDMAGIC(42).GT.0)I=I+5
        IF(FINDMAGIC(44).GT.0)I=I-8
        IF(FINDMAGIC(45).GT.0)I=I+5
        IF(I.GT.98)I=98
        IF(I.LT.20)I=20
        IF(DICE(1,100).GT.I.OR.NUMBEROFWALLS.GT.3)THEN
          CALL FORMAT(3,'You can''t get away !!!!!!')
          WHOSETURN=1
          GOTO 21
        ELSE
          CALL MOVER(I)
          RESULT=2
          RETURN
          ENDIF
      ELSE IF(I.EQ.77)THEN
        CALL FORMAT(0,'Manipulate an item')
        CALL MANIPULATE(I,1)
        MONHTPT=MONHTPT-I
        IF(MONHTPT.LT.1)GOTO 35
        WHOSETURN=0
        GOTO 21
      ELSE IF(I.EQ.67.OR.I.EQ.51)THEN
10      CALL FORMAT(0,'Cast a spell')
        CALL CAST(I,1)
        IF(FLAG.EQ.4)GOTO 50
        IF(FLAG.EQ.2)RETURN
        IF(FLAG.EQ.6)GOTO 35
        IF(FLAG.EQ.3)GOTO 7
        MONHTPT=MONHTPT-I
        IF(MONHTPT.LT.1)GOTO 35
        WHOSETURN=0
        GOTO 21
      ELSE IF(I.EQ.75.OR.I.EQ.49)THEN
        CALL FORMAT(0,'Fight to the death')
        WHOSETURN=-1
        IF(STATS(5).GT.MONDXTRTY)GOTO 22
        GOTO 21
      ELSE IF(I.EQ.66)THEN
        IF(BLINK.GT.0)THEN
7         RESULT=2
          CALL FORMAT(0,'Blink')
          CALL BLNK
          RETURN
          ENDIF
      ELSE IF(I.EQ.84.AND.CLASS.EQ.8)THEN
        CALL FORMAT(0,'Turn undead')
        IF(TURNED.GT.0)THEN
          CALL FORMAT(3,'You''ve already tried to turn this monster.')
          GOTO 2
          ENDIF
        TURNED=1
        DO N1=1,10
          IF(MONNUM.EQ.UNDEAD(N1))GOTO 80
        ENDDO
        CALL FORMAT(3,'That monster is not undead.......')
        GOTO 8
80      K=DICE(1,20)
        IF(STATS(3).GT.14)K=K+1
        IF(STATS(3).GT.17)K=K+1
        M=CHARLVL
        IF(M.LT.1)THEN
          M=1
        ELSE IF(M.GT.12)THEN
          M=12
          ENDIF
        L=UNDEAD_LOOKUP(N1,M)
        IF(L.LT.0)THEN
          CALL FORMAT(3,'You have dispelled the monster.')
          GOTO 35
        ELSE IF(K.GE.L)THEN
          CALL FORMAT(3,'You have turned the monster away.')
          RETURN
        ELSE
          CALL FORMAT(3,'You were unable to turn the monster.')
          GOTO 2
          ENDIF
        ENDIF

      CALL TTYNL(2)
      GOTO 2

21    CALL MFIGHT
      IF(LIFE.EQ.0.OR.FLAG.EQ.1)RETURN
      IF(WHOSETURN.EQ.0)GOTO 4
      IF(WHOSETURN.EQ.1)WHOSETURN=0

22    CALL CFIGHT(I,MONYOUHIT,MONUNDEADMAGIC,BACKSTAB)
      MONHTPT=MONHTPT-I
      IF(MONHTPT.LT.1)GOTO 35
      IF(WHOSETURN.EQ.0)GOTO 4
      IF(WHOSETURN.EQ.1)WHOSETURN=0
      GOTO 21

50    CALL FORMAT(4,'It is undead...........You have a protection
     + from evil!/working right. It has turned and run from you.')
      RETURN

35    IF(MONSA1.EQ.0.AND.MONSA2.EQ.0.AND.MONSA3.EQ.0)THEN
        I=5*MONTHTPT
      ELSE
        I=6*MONTHTPT
        ENDIF
      I=I*(((DUNGEON+1)/2)+1)
      EXPERIENCE=EXPERIENCE+I
      IF(MONNUM.EQ.4)THEN
        I=DICE(1,6)
        CALL FORMAT(4,'The spore has exploded!!!/')
        CALL PRINT_DAMAGE(I)
        IF(LIFE.EQ.0)RETURN
        ENDIF

      IF(MONHTPT.GE.-8)THEN
        CALL FORMAT(5,'You have killed it !!!!!!')
      ELSE
        CALL FORMAT(5,'You really killed him !!!!!/There are pieces
     + of ')
        CALL TRIMMER(MONSTER(MONNUM))
        CALL FORMAT(0,' all over the place.')
        ENDIF

      CALL CHECKLEVEL
      RESULT=60
      RETURN
      END
      

*****
*
*  SPECMON contains the special attacks for the monsters.
*
*****

      SUBROUTINE SPECMON(ATTACK)
      INCLUDE 'qstcom.inc'
      INTEGER ATTACK
      LOGICAL SAVINGTHROW
      CHARACTER DESCRIPT*36

      FLAG=0

      GOTO(5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,
     +90,95,100) ATTACK

5     IF(SAVINGTHROW(STATS(4)).OR.FINDMAGIC(57).GT.0)RETURN
      IF(MONNUM.EQ.65)THEN
        LIFE=0
        CALL FORMAT(3,'You have been injected with a fatal poison.
     +!/You will die a horrible death.')
      ELSE
        I=DICE(1,8)
        HITPOINTS=HITPOINTS-I
        IF(HITPOINTS.LT.1)LIFE=0
        CALL FORMAT(3,'You have also taken ')
        CALL OUTNUM(I)
        CALL FORMAT(0,' points of damage from the monster''s
     + poison.')
        ENDIF
      RETURN

10    CALL FORMAT(3,'You have been hit by it''s stinging tail!!!/')
      IF(SAVINGTHROW(STATS(4)).OR.FINDMAGIC(57).GT.0)RETURN
      I=DICE(1,6)
      CALL FORMAT(3,'The poison has made you drowsy. You have
     + fallen asleep for ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' days.!/')
      DO J=1,I
        K=J
        IF(DICE(1,5).EQ.1)THEN
          LIFE=0
          CALL FORMAT(3,'You did not live through day number ')
          CALL OUTNUM(K)
          RETURN
        ELSE
          CALL FORMAT(2,'You made it through day number ')
          CALL OUTNUM(K)
          ENDIF
      ENDDO
      HITPOINTS=TOTALHITPOINTS
      FLAG=1
      RETURN

15    IF(SAVINGTHROW(STATS(4)))RETURN
      LIFE=0
      CALL FORMAT(3,'You were in contact with the green slime too
     + long.!/You have become a green slime......')
      RETURN

20    IF(SAVINGTHROW(STATS(4)+2))RETURN
      LIFE=0
      CALL FORMAT(3,'The touch of this monster causes paralysis.!/
     +You have been paralyzed........you will be eaten by passing
     + monsters.')
      RETURN

25    IF((FINDMAGIC(67).EQ.0).AND.
     +(HITPOINTS.GT.0.5*TOTALHITPOINTS.OR.FINDMAGIC(57).GT.0))RETURN
      FLAG=1
      DISEASE=DISEASE+1
      CALL FORMAT(4,'You have contracted Lycanthropy. You must be
     + healed by the!/cleric in the city or else you will soon die.')
      RETURN

30    IF(SAVINGTHROW(STATS(4)))RETURN
      IF(MONNUM.EQ.35)THEN
        I=1
      ELSE
        I=2
        ENDIF
      CALL FORMAT(3,'You have been in close contact with the monster.
     +!/You have lost some ')
      IF(MONNUM.EQ.35)CALL FORMAT(0,'strength.')
      IF(MONNUM.EQ.56.OR.MONNUM.EQ.80)CALL FORMAT(0,'intelligence.')
      CALL CHANGESTATS(-1,I)
      RETURN

35    IF(MONNUM.EQ.108)THEN
        I=DICE(2,5)+CHARLVL
      ELSE
        I=DICE(1,5)+(CHARLVL/2)
        ENDIF
      CALL FORMAT(3,'The monster is spitting acid !! You have been
     + hit.!/')
      CALL PRINT_DAMAGE(I)
      RETURN

40    CALL FORMAT(3,'The harpie is singing a sweet song.....')
      IF(SAVINGTHROW(STATS(2)-4))RETURN
      IF(SAVINGTHROW(STATS(3)-4))THEN
        GOLDONPERSON=0
        CALL REMOVEMAGIC(0,0,J)
        FLAG=1
        CALL FORMAT(3,'The harpie has asked for all of your
     + treasure.!/She takes it and leaves, making sure you don''t
     + follow.')
      ELSE
        LIFE=0
        CALL FORMAT(3,'You have been charmed by the harpie. She has
     + asked you to give up!/your life......you have agreed.')
        ENDIF
      IF(DELAY.NE.0)CALL QSLEEP(DELAY)
      RETURN

45    IF(SAVINGTHROW(STATS(2)))RETURN
      LIFE=0
      CALL FORMAT(3,'The gaze of this monster causes hypnosis.!/
     +You were unable to avoid it''s gaze. You have been hynotized,
     + never to awaken again.')
      RETURN

50    IF(STENCH.GT.0)RETURN
      STENCH=2
      CALL FORMAT(3,'This monster exudes a horrible stench. You are
     + having trouble!/breathing. You will fight at a disadvantage.')
      RETURN

55    IF(DICE(1,100).GT.50)RETURN
      CALL FORMAT(3,'He has turned invisible.')
      IF(DICE(1,5).LT.5)THEN
        FLAG=2
        CALL FORMAT(2,'He has attempted to steal something. You
     + caught him in the act.')
        RETURN
        ENDIF

      CALL FORMAT(3,'You are unable to locate him.')
      PRINTFLAG=1
      FLAG=1
      IF(CLASS.EQ.7)RETURN
      I=MAGICPERLEVEL(CHARLVL)
      IF(FINDMAGIC(0).EQ.I)RETURN
56    J=DICE(1,I)
      IF(MAGIC(J).EQ.0)GOTO 56
      I=MAGIC(J)
      MAGIC(J)=0
      PROPERTIES(J)=0
      CALL OPENMAGIC
      READ(23,58,REC=I) DESCRIPT
58    FORMAT(10X,A36)
      CALL CLOSEFILE(23)
      CALL FORMAT(2,'You are also unable to locate your ')
      CALL TRIMMER(DESCRIPT)
      CALL ARMOR
      RETURN

60    IF(SAVINGTHROW(STATS(4)))RETURN
      LIFE=0
      IF(MONNUM.EQ.57)THEN
        CALL FORMAT(3,'The touch of this monster causes petrifi
     +cation.!/You have turned to stone !!!!')
      ELSE
        CALL FORMAT(3,'You have gazed on the monster''s face.!/
     +You have turned to stone.')
        ENDIF
      RETURN

65    I=DICE(2,9)+CHARLVL
      CALL FORMAT(3,'The monster is breathing a cone of fire at you.
     +!/')
      CALL PRINT_DAMAGE(I)
      RETURN

70    IF(CHARLVL.EQ.1)THEN
        EXPERIENCE=-1
      ELSE
        EXPERIENCE=IEXPER(CLASS,CHARLVL-1)+1
        ENDIF
      CALL FORMAT(3,'The chill touch of this monster drains an
     + energy level.!/')
      AGE=AGE+3
      CALL CHECKLEVEL
      RETURN

75    IF(SAVINGTHROW(STATS(5)))RETURN
      ADJTOAC=ADJTOAC+1
      CALL ARMOR
      CALL FORMAT(3,'The monster has hit your armor!!!! It has
     + been corroded and is now less effective.')
      RETURN

80    CALL FORMAT(3,'The monster is hurling spikes !!!!')
      K=DICE(DICE(1,4),4)
      HITPOINTS=HITPOINTS-K
      IF(HITPOINTS.LT.1)LIFE=0
      CALL FORMAT(3,'You were hit by some of the spikes for ')
      CALL OUTNUM(K)
      CALL FORMAT(0,' points of damage.')
      RETURN

85    IF(DICE(1,100).GT.40+DUNLVL*2+CHARLVL)RETURN
      I=DICE(8,8)
      IF(SAVINGTHROW(STATS(5)))I=INT(FLOAT(I)/2.0)+1
      CALL FORMAT(3,'He is casting a frosty cone of cold at you!!
     +!/')
      CALL PRINT_DAMAGE(I)
      RETURN

90    GOLDONPERSON=INT(FLOAT(GOLDONPERSON)*0.8)
      CALL FORMAT(3,'Xorn are very fond of eating treasure.!/
     +This one is eating yours...........')
      RETURN

95    FLAG=1
      PRINTFLAG=1
      CALL FORMAT(3,'He has offered to heal you for a donation
     + to his church.!/How much would you like to donate?  ')
      CALL INPUTNUMBER(I)
      IF(I.LT.1.OR.I.GT.GOLDONPERSON)RETURN
      GOLDONPERSON=GOLDONPERSON-I
      HITPOINTS=HITPOINTS+I/100
      IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
      CALL FORMAT(3,'He waves his hands in front of you.!/
     +You have healed some wounds. Current hit points: ')
      CALL OUTNUM(HITPOINTS)
      RETURN

100   I=MAGICPERLEVEL(CHARLVL)
      IF(FINDMAGIC(0).EQ.I)RETURN
101   J=DICE(1,I)
      IF(MAGIC(J).EQ.0)GOTO 101
      MAGIC(J)=0
      PROPERTIES(J)=0
      CALL FORMAT(3,'The monster has touched you. One of your
     + magic items has been disenchanted.')
      RETURN
      END



*****
*
*  CFIGHT controls the player's swing at the monster.
*
*****

      SUBROUTINE CFIGHT(I,J,M,BACKSTAB)
      INCLUDE 'qstcom.inc'
      INTEGER I,J,M,WEAPON,BACKSTAB
      CHARACTER TYPE*10

      I=0
      CALL HIT(WEAPON,K)
      IF(K.LT.J)THEN
        CALL FORMAT(2,'You missed.')
        RETURN
        ENDIF

      IF(MAGIC_RANGE(89,91).GT.0)THEN
        TYPE='Defender.'
      ELSE
        IF(CLASS.EQ.10.OR.CLASS.EQ.7)THEN
          TYPE='Sword.'
        ELSE IF(CLASS.EQ.8)THEN
          TYPE='Mace.'
        ELSE
          TYPE='Dagger.'
          ENDIF
        ENDIF
      CALL FORMAT(3,'You have hit with your ')
      CALL TRIMMER(TYPE)

      IF(M.GT.1.AND.WEAPON.EQ.0)THEN
        CALL FORMAT(2,'This monster can only be hurt by magical
     + weapons and you don''t have one.')
        RETURN
        ENDIF

      CALL DAMAGE(I,WEAPON)
      IF(MONNUM.EQ.12.AND.CLASS.NE.8)I=INT(FLOAT(I)/2.)
      IF(BACKSTAB.NE.0)THEN
        I=I*BACKSTAB
        BACKSTAB=0
        CALL FORMAT(3,'You have stabbed the monster from the back.')
        ENDIF
      IF(I.LT.1)I=1
      CALL FORMAT(2,'You did ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' points of damage.')
      IF(FINDMAGIC(88).GT.0)THEN
        HITPOINTS=HITPOINTS+INT(FLOAT(I)/2.)
        IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
        ENDIF
      RETURN
      END




*****
*
*  MFIGHT allows the monster to swing at the player and use special
*  attacks.
*
*****

      SUBROUTINE MFIGHT
      INCLUDE 'qstcom.inc'

      IF(MONNUM.EQ.74)THEN
        MONHTPT=MONHTPT+2
        IF(MONHTPT.GT.MONTHTPT)MONHTPT=MONTHTPT
        ENDIF
      FLAG=0
      Z=1+FINDMAGIC(29)
      IF(MONSA2.GT.0)THEN
        CALL SPECMON(MONSA2)
        IF(LIFE.EQ.0.OR.FLAG.EQ.1)RETURN
        ENDIF

2     I=DICE(1,20)
      J=I+ARMORCLASS-BLIND
      IF(I.EQ.1.OR.J.LT.MONHITYOU)THEN
        CALL FORMAT(2,'It missed.')
        GOTO 1
        ENDIF
      I=DICE(MONDMGDICE,MONDDSIDES)
      IF(FINDMAGIC(80).GT.0)I=I/2
      IF(I.LT.1)I=1
      CALL FORMAT(3,'You have been hit!!!/')
      CALL PRINT_DAMAGE(I)
      IF(MONSA3.GT.0)CALL SPECMON(MONSA3)
      IF(LIFE.EQ.0)RETURN
1     Z=Z-1
      IF(Z.GT.0)GOTO 2

      RETURN
      END




*****
*
*  GETDUNGEON reads in a dungeon level where the calling sequence is:
*
*  CALL GETDUNGEON(DUNGEON NUMBER, DUNGEON LEVEL)
*
*****

      SUBROUTINE GETDUNGEON(I,J)
      INCLUDE 'qstcom.inc'
      INTEGER I,J,K,L,M

      IF(I.GT.0)THEN
        K=I*16-15
        DO L=1,8
          READ(24,2,REC=K) POINTER(L)
          READ(24,2,REC=K+1) M
          POINTER(L)=POINTER(L)*10000+M
          K=K+2
        ENDDO
        ENDIF

2     FORMAT(I4)

      K=POINTER(J)
      READ(24,2,REC=K) LEVELLENGTH
      READ(24,2,REC=K+1) LEVELWIDTH
      K=K+2

      DO L=1,LEVELLENGTH
        DO L1=1,LEVELWIDTH
          READ(24,2,REC=K) MAP(L,L1)
          K=K+1
        ENDDO
      ENDDO

      READ(24,2,REC=K) STAIRSUPX
      READ(24,2,REC=K+1) STAIRSUPY
      READ(24,2,REC=K+2) STAIRSDOWNX
      READ(24,2,REC=K+3) STAIRSDOWNY

      RETURN
      END




*****
*
*  TREASURE contains all routines that calculate, find, and determine
*  the treasure found in the dungeon, both magical and otherwise.
*
*****

      SUBROUTINE TREASURE(J)
      INCLUDE 'qstcom.inc'
      CHARACTER KIND(6)*8,TYPE(7)*9,PLACE(3)*6
      REAL WORTH(7)
      LOGICAL SAVINGTHROW
      DATA WORTH/.005,.05,.5,1.,5.,2.,2.5/
      DATA KIND/'Crown','Orb','Jewel','Ring','Sceptor',
     +'Bracelet'/
      DATA TYPE/'Copper.','Silver.','Electrum.','Gold.',
     +'Platinum.','Sapphire.','Jewelry.'/
      DATA PLACE/'Door.','Floor.','Wall.'/

      J=0
      IF(DICE(1,160-DUNLVL-CHARLVL).EQ.1)THEN
        CALL DECK
        CALL QSLEEP(1)
        IF(LIFE.EQ.0)RETURN
        GOTO 50
        ENDIF

      IF(DICE(1,6).EQ.1)GOTO 50
      TRAP=0
      IF(DICE(1,10).GT.6)TRAP=1
      I=DICE(1,6)
      I1=DICE(1,3)
      CALL SYMBOL(I+2)
      CALL QPRINT
      IF(I.EQ.2.AND.DICE(1,7).EQ.1)THEN
        J=DICE(1,2000)+600
        CALL FORMAT(3,'You have discovered a coffer.!/!/!/
     +Out of it falls a priceless ')
        CALL TRIMMER(KIND(DICE(1,6)))
        CALL FORMAT(0,' worth ')
        CALL OUTNUM(J)
        CALL FORMAT(0,' in gold.')
        RETURN
        ENDIF

1     IF(I.EQ.1)THEN
        CALL FORMAT(3,'You have found a vase which is sealed with
     + a lid.')
      ELSE IF(I.EQ.2)THEN
        CALL FORMAT(3,'You have discovered a coffer.')
      ELSE IF(I.EQ.3)THEN
        CALL FORMAT(3,'You have found a chest.')
      ELSE IF(I.EQ.4)THEN
        CALL FORMAT(3,'You have found a hidden panel in the ')
        CALL TRIMMER(PLACE(I1))
      ELSE IF(I.EQ.5)THEN
        CALL FORMAT(3,'You have found a bag with something in it.')
      ELSE IF(I.EQ.6)THEN
2       CALL FORMAT(3,'You have discovered a huge vat in the room.!/
     +Do you wish to climb up and in?   ')
        CALL INPUT(K,2)
        IF(K.EQ.78.OR.K.EQ.0.OR.K.EQ.54)RETURN
        IF(K.NE.89.AND.K.NE.57)GOTO 2
        IF(DICE(1,100).GT.50)THEN
          CALL FORMAT(3,'As you reach the top, you slip and fall in.')
          GOTO 20
        ELSE
          CALL FORMAT(3,'As you reach the top, you can see in the
     + vat.!/It has some treasure in it which you can reach.')
          GOTO 50
          ENDIF
        ENDIF

      IF(FINDMAGIC(49).GT.0)THEN
        CALL FORMAT(3,'Your wand of Open-Sesame has floated away
     + from you.!/It is waving at the container. The container has
     + opened.')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        GOTO 10
        ENDIF

      IF(FINDMAGIC(55).GT.0)THEN
        K=DICE(1,3)
        CALL FORMAT(3,'You are confused. You have decided to ')
        GOTO(11,12,13) K
        ENDIF

      CALL FORMAT(2,'What would you like to do ("H" for Help) ?   ')
      CALL INPUT(K,1)

      IF(K.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +O - open the object!/
     +L - leave it alone!/
     +C - cast a spell!/
     +M - manipulate an item!/
     +Z - personal statistics!/
     +X - current statitics!/
     +I - inventory')
        GOTO 1
      ELSE IF(K.EQ.76.OR.K.EQ.0.OR.K.EQ.54)THEN
11      CALL FORMAT(0,'Leave it alone')
        RETURN
      ELSE IF(K.EQ.67.OR.K.EQ.51)THEN
12      CALL FORMAT(0,'Cast a spell')
        CALL CAST(J,2)
        IF(FLAG.EQ.7)GOTO 10
        IF(FLAG.EQ.8.OR.FLAG.EQ.9)RETURN
        GOTO 1
      ELSE IF(K.EQ.77)THEN
        CALL FORMAT(0,'Manipulate an item')
        CALL MANIPULATE(K,2)
        GOTO 1
      ELSE IF(K.EQ.79.OR.K.EQ.57)THEN
13      CALL FORMAT(0,'Open it')
        GOTO 10
        ENDIF
      GOTO 1

10    IF(TRAP.EQ.0)GOTO 50
      IF(FINDMAGIC(46).GT.0)THEN
14      CALL FORMAT(3,'Your wand of trap detection points directly
     + at it.!/It is trapped. Do you STILL wish to open it?    ')
        CALL INPUT(K,2)
        IF(K.EQ.78.OR.K.EQ.0)THEN
          RETURN
        ELSE IF(K.NE.89)THEN
          GOTO 14
          ENDIF
        ENDIF

20    I=0
      IF(CLASS.EQ.7)I=2
      IF(FINDMAGIC(44).GT.0)I=I-2
      IF(FINDMAGIC(45).GT.0)I=I+1
      IF(FINDMAGIC(46).GT.0)I=I+2
      IF(FINDMAGIC(49).GT.0)I=I-1
      IF(FINDMAGIC(58).GT.0)I=I-2
      CALL FORMAT(3,'It was trapped!! ')
      IF(.NOT.SAVINGTHROW(STATS(5)+I))THEN
        K=DICE(1,100)
        IF(K.LE.85)THEN
          I=DICE(1,DUNLVL*10)
          CALL PRINT_DAMAGE(I)
        ELSE IF(K.LE.90)THEN
          AGE=AGE+1
          CALL FORMAT(3,'It was trapped with a Symbol of Time.!/
     +You have aged 1 year.')
        ELSE IF(K.LE.94)THEN
          CALL FORMAT(3,'There is contact poison on the lock.')
          IF(SAVINGTHROW(STATS(4)))THEN
            CALL FORMAT(3,'You noticed it in time and avoided touching
     + it.')
          ELSE
            I=DICE(DUNLVL,4)
            CALL PRINT_DAMAGE(I)
            ENDIF
        ELSE IF(K.LE.96)THEN
          CALL FORMAT(3,'It was trapped with a teleport spell.!/
     +You have been teleported.')
          XCOORD=DICE(1,LEVELLENGTH)
          YCOORD=DICE(1,LEVELWIDTH)
          CALL BUILDMAP
          PRINTFLAG=1
          CALL QPRINT
          FLAG=1
          RETURN
        ELSE IF(K.LE.99)THEN
          CALL FORMAT(3,'It was trapped with poison darts. You have
     + fallen into!/a deep slumber. While you were asleep, a band of
     + wandering leprechauns!/came through the room.')
          IF(FINDMAGIC(0).NE.MAGICPERLEVEL(CHARLVL))THEN
798         J=DICE(1,8)
            I=MAGIC(J)
            IF(I.EQ.0)GOTO 798
            MAGIC(J)=60
            RETURN
            ENDIF
        ELSE
          I=DICE(DICE(2,DUNLVL),DUNLVL)+(DUNLVL*5)
          CALL FORMAT(3,'A stone slab (a HUGE one) has fallen from the
     + ceiling.!/')
          CALL PRINT_DAMAGE(I)
          ENDIF
      ELSE
        CALL FORMAT(0,'You, being as dexterous as you are, avoided
     + injury.')
        ENDIF
      IF(LIFE.EQ.0)RETURN
      IF(DELAY.NE.0)CALL QSLEEP(DELAY)

50    CALL SYMBOL(1)
      CALL QPRINT
      I=9+FINDMAGIC(54)*2
      IF(LOCATION.EQ.25)I=I*3
      IF(LOCATION.EQ.26)I=0
      IF(DICE(1,100).LE.I)THEN
        CALL LOCATEMAGIC(0)
        RETURN
        ENDIF

      I=DICE(1,7)
      IF(DICE(1,100).LT.25)I=DICE(1,4)
      J=INT(FLOAT(DICE(1,240))*(FLOAT(DUNLVL)**1.2))
      IF(FINDMAGIC(54).GT.0)THEN
        J=J+DICE(1,J)
        CALL FORMAT(3,'Your jewel has magically increased the
     + treasure you''ve found.')
        DO K1=1,8
          IF(MAGIC(K1).EQ.54)THEN
            K2=K1
            CALL CHECKCHARGES(K2)
            GOTO 52
            ENDIF
        ENDDO
        ENDIF

52    IF(FINDMAGIC(50).GT.0)THEN
        J=DICE(1,J)
        CALL FORMAT(3,'Your wand of treasure destruction has blasted
     + the treasure.!/Only part of it remains.')
        ENDIF

      CALL FORMAT(3,'You have found ')
      CALL OUTNUM(J)
      CALL FORMAT(0,' pieces of ')
      CALL TRIMMER(TYPE(I))
      IF(I.EQ.4)RETURN
      J=INT(FLOAT(J)*WORTH(I))
      IF(J.LT.1)J=1
      CALL FORMAT(2,'It is worth ')
      CALL OUTNUM(J)
      CALL FORMAT(0,' in gold.')
      RETURN
      END



*****
*
*  CHECKCHARGES checks to see if a magic item has run out of charges.
*
*****

      SUBROUTINE CHECKCHARGES(NUMBER)
      INCLUDE 'qstcom.inc'
      CHARACTER DESCRIPT*36

      PROPERTIES(NUMBER)=PROPERTIES(NUMBER)-1
      IF(PROPERTIES(NUMBER).GT.0)RETURN
      I=MAGIC(NUMBER)
      MAGIC(NUMBER)=0
      CALL OPENMAGIC
      READ(23,1,REC=I) DESCRIPT
1     FORMAT(10X,A36)
      CALL CLOSEFILE(23)
      CALL FORMAT(3,'As it''s last charge is expended, your ')
      CALL TRIMMER(DESCRIPT)
      CALL FORMAT(2,'crumbles to dust.!/')
      IF(DELAY.NE.0)CALL QSLEEP(DELAY)
      RETURN
      END



*****
*
*  MONSAV rolls saving throws for the monsters.
*
*****

      LOGICAL FUNCTION MONSAV(LEVEL,NUMBER)
      INCLUDE 'qstcom.inc'
      INTEGER LEVEL,NUMBER,I,J,K
      LOGICAL SAVINGTHROW

      MONSAV=.FALSE.
      I=18
      IF(NUMBER.EQ.115)THEN
        I=I+5
      ELSE IF(NUMBER.EQ.16)THEN
        I=I+7
      ELSE IF(NUMBER.EQ.113)THEN
        I=I+18
      ELSE IF(NUMBER.EQ.2)THEN
        I=I+4
      ELSE IF(NUMBER.GT.115)THEN
        I=I+18
        ENDIF

      IF(CLASS.EQ.10.OR.CLASS.EQ.7)THEN
        J=10
      ELSE IF(CLASS.EQ.8)THEN
        J=STATS(3)
      ELSE
        J=STATS(2)
        ENDIF

      MONSAV=SAVINGTHROW(I-J-CHARLVL+LEVEL)
      RETURN
      END



*****
*
*  RETRIEVEMONSTER pulls a monster's statistics from the monster array.
*  "I" is the number of the monster to get.
*
*****

      SUBROUTINE RETRIEVEMONSTER(I)
      INCLUDE 'qstcom.inc'

      MONNAM=MONSTER(I)
      MONHTDICE=IPICK(MONSTERSTAT(I,1),7,8)
      MONHDSIDES=IPICK(MONSTERSTAT(I,1),6,0)
      MONHTPT=DICE(MONHTDICE,MONHDSIDES)
      MONTHTPT=MONHTPT
      MONDXTRTY=IPICK(MONSTERSTAT(I,2),2,0)
      MONDMGDICE=IPICK(MONSTERSTAT(I,1),5,0)
      MONDDSIDES=IPICK(MONSTERSTAT(I,1),4,0)
      MONSA1=0
      MONSA2=0
      MONSA3=0
      MONHITYOU=IPICK(MONSTERSTAT(I,2),5,6)
      MONYOUHIT=IPICK(MONSTERSTAT(I,2),3,4)
      MONUNDEADMAGIC=IPICK(MONSTERSTAT(I,2),1,0)
      J=IPICK(MONSTERSTAT(I,1),2,3)
      J1=IPICK(MONSTERSTAT(I,1),1,0)
      IF(J1.EQ.1)THEN
        MONSA1=J
      ELSE IF(J1.EQ.2)THEN
        MONSA2=J
      ELSE IF(J1.EQ.3)THEN
        MONSA3=J
        ENDIF
      MONDXTRTY=DICE(3,MONDXTRTY)
      BLIND=0
      STENCH=0
      FLAG=0
      MONLVL=INT(FLOAT(I+13)/14.)
      IF(I.GT.112.OR.FINDMAGIC(35).EQ.0.OR.FINDMAGIC(40).GT.0)RETURN
      MONNAM=MONSTER(DICE(1,112))

      RETURN
      END




*****
*
*  GETMON reads in the monsters from the monster data file.
*
*****

      SUBROUTINE GETMON
      INCLUDE 'qstcom.inc'

      CHARACTER QFN*256
      CALL QPATH('mon.dta',QFN)
      OPEN(UNIT=22,FILE=QFN,STATUS='OLD',FORM='FORMATTED',ACTION='READ')
      I=1
      DO WHILE(.TRUE.)
        READ(22,1,ERR=2,END=2) MONSTER(I),MONSTERSTAT(I,1),
     +MONSTERSTAT(I,2)
1       FORMAT(A20,I8,I6)
        I=I+1
      ENDDO
2     CALL CLOSEFILE(22)

      RETURN
      END



*****
*
*  MOVER moves a character in a random direction. It should ONLY be called
*  if there is an open direction to move in.
*
*****

      SUBROUTINE MOVER(I)
      INCLUDE 'qstcom.inc'

1     I=DICE(1,4)
      IF(WALL(I).NE.'     '.AND.WALL(I).NE.'M---M'.AND.WALL(I).NE.
     +'!'.AND.WALL(I).NE.'M   M')GOTO 1

      GOTO(2,3,4,5) I

2     XCOORD=WRAPAROUND(XCOORD-1,LEVELLENGTH)
      RETURN

3     YCOORD=WRAPAROUND(YCOORD+1,LEVELWIDTH)
      RETURN

4     XCOORD=WRAPAROUND(XCOORD+1,LEVELLENGTH)
      RETURN

5     YCOORD=WRAPAROUND(YCOORD-1,LEVELWIDTH)
      RETURN
      END



*****
*
*  PIT handles the special room code "PIT".
*
*****

      SUBROUTINE PIT
      INCLUDE 'qstcom.inc'
      LOGICAL SAVINGTHROW

      PICTURE(6)(9:11)='PIT'
      CALL QPRINT

      IF(SAVINGTHROW(STATS(5)).OR.FINDMAGIC(45).GT.0)THEN
        CALL FORMAT(3,'You avoided the pit.')
      ELSE
        I=DICE(DUNLVL,6)
        CALL FORMAT(3,'You have fallen into a pit in the dungeon
     + floor.!/')
        CALL PRINT_DAMAGE(I)
        ENDIF

      RETURN
      END



*****
*
*  SYMBOL places certain symbols on the map.
*
*****

      SUBROUTINE SYMBOL(I)
      INCLUDE 'qstcom.inc'

      J=DICE(1,2)*2+4
      GOTO(1,2,3,4,5,6,7,8,9) I

1     PICTURE(J)(9:9)='$'
      RETURN

2     PICTURE(J)(9:9)='#'
      RETURN

3     PICTURE(6)(9:11)='VAS'
      RETURN

4     PICTURE(6)(9:11)='CFR'
      RETURN

5     PICTURE(6)(9:11)='CHT'
      RETURN

6     PICTURE(6)(9:11)='PNL'
      RETURN

7     PICTURE(6)(9:11)='BAG'
      RETURN

8     PICTURE(6)(9:11)='VAT'
      RETURN

9     IF(DICE(1,2).EQ.1)THEN
        PICTURE(J)(8:8)='S'
      ELSE
        PICTURE(J)(12:12)='S'
        ENDIF

      RETURN
      END



*****
*
*  BLINK moves the player randomly when the "BLINK" spell is cast.
*
*****

      SUBROUTINE BLNK
      INCLUDE 'qstcom.inc'

      I=DICE(1,3)
      IF(DICE(1,2).EQ.1)I=-I
      XCOORD=WRAPAROUND(XCOORD+I,LEVELLENGTH)
      I=DICE(1,3)
      IF(DICE(1,2).EQ.1)I=-I
      YCOORD=WRAPAROUND(YCOORD+I,LEVELWIDTH)

      RETURN
      END



*****
*
*  LEAVING prepares the character for leaving the dungeon.
*
*****

      SUBROUTINE LEAVING
      INCLUDE 'qstcom.inc'

      DUNGEON=0
      DUNLVL=0
      XCOORD=0
      YCOORD=0
      MOVES=MOVES+7
      IF(MOVES.GT.365)THEN
        MOVES=MOVES-365
        AGE=AGE+1
        ENDIF
      CALL CLOSEFILE(24)
      CALL GOLDTOEXPERIENCE
      CALL CHECKLEVEL
      CALL NUMERICTOASCII
      CALL PUTTEMPCORE(PLAYER)
      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST2.Q7R')
      END




*****
*
*  WRAPAROUND checks to see if the player has crossed the dungeon boundary,
*  and if so, places him back on the opposite side of the dungeon.
*
*****

      INTEGER FUNCTION WRAPAROUND(I,J)

      WRAPAROUND=I
      IF(I.GT.J)THEN
        WRAPAROUND=I-J
      ELSE IF(I.LT.1)THEN
        WRAPAROUND=I+J
        ENDIF

      RETURN
      END




*****
*
*  GOLDTOEXPERIENCE changes the gold the player is carrying into gold in his
*  credit ring. The player is given experience for the gold.
*
*****

      SUBROUTINE GOLDTOEXPERIENCE
      INCLUDE 'qstcom.inc'

      GOLD=GOLD+GOLDONPERSON
      IF(CHARLVL.EQ.1)THEN
        EXPERIENCE=EXPERIENCE+GOLDONPERSON
      ELSE
        EXPERIENCE=EXPERIENCE+(GOLDONPERSON/(CHARLVL*2))
        ENDIF
      GOLDONPERSON=0

      RETURN
      END



*****
*
*  PRINT prints the map to the screen.
*
*****

      SUBROUTINE QPRINT
      INCLUDE 'qstcom.inc'

      CALL CLEARSCREEN
      IF(PROTEVIL.GT.0)CALL FORMAT(0,'Protection from Evil')
      IF(BLINK.GT.0)CALL FORMAT(2,'Blink Capabilities')
      CALL TTYNL(3)

      IF(EXPERT.EQ.0.OR.PRINTCONTROL.NE.0)THEN
        DO I=1,13
          CALL TRIMMER(PICTURE(I))
          CALL TTYNL(1)
        ENDDO
        ENDIF

      PRINTCONTROL=0
      PICTURE(6)(8:12)='     '
      PICTURE(7)(8:12)='  X  '
      PICTURE(8)(8:12)='     '
      RETURN
      END




*****
*
*  BUILDMAP builds the output picture from the encoded dungeon.
*
*****

      SUBROUTINE BUILDMAP
      INCLUDE 'qstcom.inc'
      CHARACTER DISPLAY(0:19)*3
      COMMON/GONZO/V(4,4,3),SECRET,NOSECRET,FAIR
      DATA DISPLAY/'   ','FNT','TEL','TEL','TEL','THR','[_]','PIT',
     +'GAS','GAS','GAS','GAS','STR','STR','STR','STR','DSU','DSD',
     +'SU','SD'/

      DO I=1,13
        PICTURE(I)='                   '
      ENDDO
      PICTURE(7)(10:10)='X'

      SECRET=FINDMAGIC(33)+FINDMAGIC(47)
      NOSECRET=FINDMAGIC(48)
      FAIR=FINDMAGIC(36)

      DO I=-1,2
        I1=I
        K=WRAPAROUND(XCOORD+I1,LEVELLENGTH)
        DO J=-1,2
          I1=J
          K1=WRAPAROUND(YCOORD+I1,LEVELWIDTH)
          I1=MAP(K,K1)
          V(I+2,J+2,1)=IPICK(I1,2,0)
          V(I+2,J+2,2)=IPICK(I1,1,0)
          V(I+2,J+2,3)=IPICK(I1,3,4)
        ENDDO
      ENDDO
      LOCATION=V(2,2,3)

      CALL HORIZONTAL(5,8,2,2)
      IF(V(2,2,1).EQ.0.OR.V(2,2,1).EQ.3)THEN
        IF(FAIR.NE.0.AND.V(1,2,3).LT.20)PICTURE(2)(9:11)=
     +DISPLAY(V(1,2,3))
        CALL VERTICAL(1,7,1,2)
        IF(V(1,2,2).EQ.0.OR.V(1,2,2).EQ.3)CALL HORIZONTAL(1,2,1,1)
        CALL HORIZONTAL(1,8,1,2)
        CALL VERTICAL(1,13,1,3)
        IF(V(1,3,2).EQ.0.OR.V(1,3,2).EQ.3)CALL HORIZONTAL(1,14,1,3)
        ENDIF

      CALL VERTICAL(5,13,2,3)
      IF(V(2,3,2).EQ.0.OR.V(2,3,2).EQ.3)THEN
        IF(FAIR.NE.0.AND.V(2,3,3).LT.20)PICTURE(6)(15:17)=
     +DISPLAY(V(2,3,3))
        CALL HORIZONTAL(5,14,2,3)
        IF(V(2,3,1).EQ.0.OR.V(2,3,1).EQ.3)CALL VERTICAL(1,19,1,4)
        CALL VERTICAL(5,19,2,4)
        CALL HORIZONTAL(9,14,3,3)
        IF(V(3,3,1).EQ.0.OR.V(3,3,1).EQ.3)CALL VERTICAL(9,19,3,4)
        ENDIF

      CALL HORIZONTAL(9,8,3,2)
      IF(V(3,2,1).EQ.0.OR.V(3,2,1).EQ.3)THEN
        IF(FAIR.NE.0.AND.V(3,2,3).LT.20)PICTURE(10)(9:11)=
     +DISPLAY(V(3,2,3))
        CALL VERTICAL(9,13,3,3)
        IF(V(3,3,2).EQ.0.OR.V(3,3,2).EQ.3)CALL HORIZONTAL(13,14,4,3)
        CALL HORIZONTAL(13,8,4,2)
        CALL VERTICAL(9,7,3,2)
        IF(V(3,2,2).EQ.0.OR.V(3,2,2).EQ.3)CALL HORIZONTAL(13,2,4,1)
        ENDIF

      CALL VERTICAL(5,7,2,2)
      IF(V(2,2,2).EQ.0.OR.V(2,2,2).EQ.3)THEN
        IF(FAIR.NE.0.AND.V(2,1,3).LT.20)PICTURE(6)(3:5)=
     +DISPLAY(V(2,1,3))
        CALL HORIZONTAL(9,2,3,1)
        IF(V(3,1,1).EQ.0.OR.V(3,1,1).EQ.3)CALL VERTICAL(9,1,3,1)
        CALL VERTICAL(5,1,2,1)
        CALL HORIZONTAL(5,2,2,1)
        IF(V(2,1,1).EQ.0.OR.V(2,1,1).EQ.3)CALL VERTICAL(1,1,1,1)
        ENDIF

      WALL(1)=PICTURE(5)(8:12)
      WALL(2)=PICTURE(7)(13:13)
      WALL(3)=PICTURE(9)(8:12)
      WALL(4)=PICTURE(7)(7:7)

      NUMBEROFWALLS=0
      DO I=1,4
        IF(WALL(I).NE.'!'.AND.WALL(I).NE.'M---M'.AND.WALL(I).NE.
     +' '.AND.WALL(I).NE.'M   M')NUMBEROFWALLS=NUMBEROFWALLS+1
      ENDDO

      RETURN
      END



*****
*
*  HORIZONTAL builds the horizontal walls into the dungeon picture.
*
*****

      SUBROUTINE HORIZONTAL(I1,I2,I3,I4)
      INCLUDE 'qstcom.inc'
      INTEGER I1,I2,I3,I4
      COMMON/GONZO/V(4,4,3),SECRET,NOSECRET,FAIR
      CHARACTER W(8)*5
      DATA W/'     ','MMMMM','M---M','M   M','M---M','M:::M',
     +'M:::M','M:::M'/

      I=I1
      N=V(I3,I4,1)
      IF(N.NE.8.AND.N.NE.4)THEN
        PICTURE(I)(I2:I2+4)=W(N+1)
        IF(N.EQ.0)RETURN
      ELSE IF(N.EQ.4)THEN
        N=2
        IF(DICE(1,5).EQ.1)N=3
        IF(SECRET.GT.0)N=3
        IF(NOSECRET.GT.0)N=2
        PICTURE(I)(I2:I2+4)=W(N)
      ELSE
        N=2
        IF(DICE(1,10).EQ.1)N=1
        PICTURE(I)(I2:I2+4)=W(N)
        IF(N.EQ.1)RETURN
        ENDIF

      PICTURE(I)(I2-1:I2-1)='M'
      PICTURE(I)(I2+5:I2+5)='M'
      RETURN
      END



*****
*
*  VERTICAL places vertical walls into the dungeon map.
*
*****

      SUBROUTINE VERTICAL(I1,I2,I3,I4)
      INCLUDE 'qstcom.inc'
      INTEGER I1,I2,I3,I4
      COMMON/GONZO/V(4,4,3),SECRET,NOSECRET,FAIR
      CHARACTER W(8)*1
      DATA W/'M','!',' ','!',':',':',':','M'/

      I=I1
      N=V(I3,I4,2)
      IF(N.EQ.0)THEN
        RETURN
      ELSE IF(N.EQ.8.AND.DICE(1,10).EQ.1)THEN
        RETURN
      ELSE
        PICTURE(I)(I2:I2)='M'
        PICTURE(I+1)(I2:I2)='M'
        PICTURE(I+3)(I2:I2)='M'
        PICTURE(I+4)(I2:I2)='M'
        ENDIF

      PICTURE(I+2)(I2:I2)=W(N)
      IF(N.EQ.4)THEN
        N=1
        IF(DICE(1,5).EQ.1)N=2
        IF(SECRET.GT.0)N=2
        IF(NOSECRET.GT.0)N=1
        PICTURE(I+2)(I2:I2)=W(N)
        ENDIF

      RETURN
      END

      


*****
*
*  HIT rolls to see if the player hits the monster.
*
*****

      SUBROUTINE HIT(WEAPON,J)
      INCLUDE 'qstcom.inc'
      INTEGER WEAPON,J

      WEAPON=0
      DO I=1,8
        IF(MAGIC(I).GT.0.AND.MAGIC(I).LT.13)THEN
          WEAPON=INT(FLOAT(MAGIC(I)+2)/3.)
        ELSE IF(MAGIC(I).GT.12.AND.MAGIC(I).LT.16)THEN
          WEAPON=-1
        ELSE IF(MAGIC(I).GT.15.AND.MAGIC(I).LT.19)THEN
          WEAPON=-2
          ENDIF
      ENDDO

      J=DICE(1,20)
      IF(J.EQ.20)J=100
      IF(J.EQ.1)J=-100
      IF(CLASS.EQ.10.OR.CLASS.EQ.7)THEN
        K=1
      ELSE IF(CLASS.EQ.8)THEN
        K=2
      ELSE
        K=3
        ENDIF
      I=MAGIC_RANGE(89,91)
      IF(I.NE.0)WEAPON=5-PROPERTIES(MAGIC_POSITION(I))
      J=J+INT(FLOAT(CHARLVL)/FLOAT(K))+WEAPON

      IF(STATS(1).GT.14)J=J+1
      IF(STATS(1).GT.17)J=J+1
      IF(STATS(1).LT.9)J=J-1
      IF(FINDMAGIC(30).GT.0)J=J-2
      IF(FINDMAGIC(31).GT.0)J=J+2
      IF(FINDMAGIC(38).GT.0)J=J-1
      IF(FINDMAGIC(58).GT.0)J=J-1
      RETURN
      END




*****
*
*  DAMAGE rolls the damage when a character hits a monster.
*
*****

      SUBROUTINE DAMAGE(I,WEAPON)
      INCLUDE 'qstcom.inc'
      INTEGER WEAPON

      IF(CLASS.EQ.10.OR.CLASS.EQ.7)THEN
        I=DICE(1,8)
      ELSE IF(CLASS.EQ.8)THEN
        I=DICE(1,6)
      ELSE
        I=DICE(1,4)
        ENDIF

      I=I+WEAPON
      IF(STATS(1).GT.19)I=I+1
      IF(STATS(1).GT.17)I=I+1
      IF(STATS(1).GT.14)I=I+1
      IF(FINDMAGIC(30).GT.0)I=I-3
      IF(FINDMAGIC(31).GT.0)I=I+4
      RETURN
      END




*****
*
*  MANIPULATE allows the user to manipulate items the player is carrying.
*
*****

      SUBROUTINE MANIPULATE(DAMAGE,WHERE)
      INCLUDE 'qstcom.inc'
      INTEGER DAMAGE,WHERE
      LOGICAL MONSAV
      
      DAMAGE=0
1     CALL LISTMAGIC
      CALL FORMAT(3,'Number of the item you wish to manipulate?  ')
      CALL INPUTNUMBER(Z)
      IF(Z.LT.1.OR.Z.GT.MAGICPERLEVEL(CHARLVL))RETURN
      IF(MAGIC(Z).EQ.0)RETURN

      J=MAGIC(Z)
      IF(J.EQ.52)THEN
        CALL FORMAT(4,'Your gem begins to grow very brightly...')
        CALL CHECKCHARGES(Z)
        CALL LEAVING
      ELSE IF(J.EQ.53)THEN
        CALL FORMAT(4,'Your jewel has the number ')
        CALL OUTNUM(DUNLVL)
        CALL FORMAT(0,' glowing in it''s center.')
      ELSE IF(J.GE.89.AND.J.LE.91)THEN
        CALL FORMAT(3,'Enter bonuses to armorclass: ')
        CALL INPUTNUMBER(I)
        IF(I.LT.0.OR.I.GT.5)I=5
        PROPERTIES(Z)=I
        CALL ARMOR
      ELSE IF(J.EQ.74)THEN
        DISEASE=0
        I=DICE(1,8)
        HITPOINTS=HITPOINTS+I
        IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
        CALL FORMAT(4,'You tap yourself with the staff. Any disease
     + has been cured.!/Current hit points: ')
        CALL OUTNUM(HITPOINTS)
        CALL CHECKCHARGES(Z)
      ELSE IF(J.EQ.75)THEN
        CALL FORMAT(4,'A cone of fire issues from your staff!!!!')
        CALL CHECKCHARGES(Z)
        IF(WHERE.NE.1)RETURN
        DAMAGE=DICE(6,10)
        IF(MONSAV(MONLVL,MONNUM))DAMAGE=INT(FLOAT(DAMAGE+1)/2.)
        CALL FORMAT(2,'The monster has taken damage.')
        RETURN
      ELSE IF(J.EQ.76)THEN
        IF(WHERE.LT.3)THEN
          CALL FORMAT(3,'That can''t be used right now.')
          RETURN
          ENDIF
        CALL FORMAT(3,'Dungeon level you wish to be on (0 to exit
     + dungeon) ?  ')
        CALL INPUTNUMBER(I)
        IF(I.LT.0.OR.I.GT.5)RETURN
        IF(I.EQ.0)CALL LEAVING
        DUNLVL=I
        CALL CHECKCHARGES(Z)
        CALL GETDUNGEON(0,DUNLVL)
        XCOORD=DICE(1,LEVELLENGTH)
        YCOORD=DICE(1,LEVELWIDTH)
        FLAG=7
      ELSE IF(J.GE.81.AND.J.LE.84)THEN
        CALL FORMAT(4,'The letters of the ancient tome begin to
     + writhe and glow....')
        CALL REMOVEMAGIC(Z,0,K)
        EXPERIENCE=(IEXPER(CLASS,CHARLVL+1)+IEXPER(CLASS,CHARLVL+2))/2
        CALL CHECKLEVEL
      ELSE IF(J.EQ.87)THEN
        I=(((XCOORD-STAIRSUPX)**2)+((YCOORD-STAIRSUPY)**2))
        I=INT(SQRT(FLOAT(I)))
        CALL FORMAT(3,'Your gem has the number ')
        CALL OUTNUM(I)
        CALL FORMAT(0,' glowing in it''s center.')
      ELSE
        CALL FORMAT(3,'That item does not need to be manipulated
     + to work.')
        ENDIF
      RETURN
      END




*****
*
*  DIRECTION handles the movement of players through the dungeon.
*
*****

      SUBROUTINE DIRECTION
      INCLUDE 'qstcom.inc'
      COMMON/GONZO/V(4,4,3)
      INTEGER LOCKED(4)
      CHARACTER TYPE(3)*5

      DATA TYPE/'Iron','Brass','Steel'/
      FLAG=0
      I=0
      IF(FINDMAGIC(68).GT.0)I=1
      LOCKED(1)=I
      LOCKED(2)=I
      LOCKED(3)=I
      LOCKED(4)=I

      IF(HITPOINTS.LT.TOTALHITPOINTS)THEN
        HPREGEN=HPREGEN+1
        IF(HPREGEN.GT.33-STATS(4))THEN
          HITPOINTS=HITPOINTS+1
          CALL FORMAT(3,'Current hit points: ')
          CALL OUTNUM(HITPOINTS)
          HPREGEN=0
          ENDIF
        ENDIF

      IF(CLASS.EQ.4.OR.CLASS.EQ.8)THEN
        SPELLREGEN=SPELLREGEN+1
        IF(SPELLREGEN.GT.36-STATS(4))THEN
          CALL SPELL(K)
          DO I=1,5
            IF(IPICK(SPELLS,SPELLTOREGEN,0).LT.IPICK(K,SPELLTOREGEN,0))THEN
       SPELLS=SPELLS+(10**(SPELLTOREGEN-1))
       SPELLTOREGEN=SPELLTOREGEN+1
       IF(SPELLTOREGEN.GT.5)SPELLTOREGEN=1
       SPELLREGEN=0
       CALL FORMAT(3,'You''ve regenerated a spell......')
       GOTO 3
       ENDIF
          SPELLTOREGEN=SPELLTOREGEN+1
          IF(SPELLTOREGEN.GT.5)SPELLTOREGEN=1
          ENDDO
          SPELLREGEN=6
          ENDIF
        ENDIF

3     IF(FINDMAGIC(37).GT.0.AND.DICE(1,100).EQ.1)THEN
        STATS(DICE(1,6))=3
        CALL REMOVEMAGIC(0,37,J)
        CALL FORMAT(3,'A flash of light bursts from your helm....')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        ENDIF

      IF(FINDMAGIC(43).GT.0.AND.NUMBEROFWALLS.LT.4.AND.DICE(1,5)
     +.GT.1)THEN
        CALL MOVER(I)
        CALL FORMAT(3,'You and your boots dance to the ')
        IF(I.EQ.1)THEN
          CALL FORMAT(0,'North')
        ELSE IF(I.EQ.2)THEN
          CALL FORMAT(0,'East')
        ELSE IF(I.EQ.3)THEN
          CALL FORMAT(0,'South')
        ELSE
          CALL FORMAT(0,'West')
          ENDIF
        RETURN
        ENDIF

1     IF(EXPERT.EQ.0)THEN
        CALL FORMAT(3,'Direction ("H" for Help) ?    ')
      ELSE
        CALL FORMAT(3,'Dir:   ')
        ENDIF

      CALL INPUT(I,1)
      IF(I.EQ.69.OR.I.EQ.54)THEN
        CALL FORMAT(0,'East')
        GOTO 80
      ELSE IF(I.EQ.78.OR.I.EQ.56)THEN
        CALL FORMAT(0,'North')
        GOTO 85
      ELSE IF(I.EQ.83.OR.I.EQ.50)THEN
        CALL FORMAT(0,'South')
        GOTO 90
      ELSE IF(I.EQ.87.OR.I.EQ.52)THEN
        CALL FORMAT(0,'West')
        GOTO 95
      ELSE IF(I.EQ.85)THEN
        CALL FORMAT(0,'Update character file')
        CALL NUMERICTOASCII
        CALL OPENCHARFILE
        CALL REPLACEPLAYER(ERR,NAME)
        CALL CLOSEFILE(21)
      ELSE IF(I.EQ.84)THEN
        CALL FORMAT(0,'The time of day is: ')
        CALL VMSTIM(TIM)
        CALL TRIMMER(TIM)
      ELSE IF(I.EQ.81)THEN
        RUN=0
        CALL FORMAT(0,'Time stop')
        CALL NUMERICTOASCII
        CALL OPENCHARFILE
        CALL REPLACEPLAYER(ERR,NAME)
        CALL CLOSEFILE(21)
        CALL CLOSEFILE(24)
        PLAYER=' '
        PLAYER(252:252)='@'
        CALL PUTTEMPCORE(PLAYER)
        CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST1.Q7R')
      ELSE IF(I.EQ.67.OR.I.EQ.51)THEN
        CALL FORMAT(0,'Cast a spell')
        CALL CAST(J,3)
        IF(FLAG.EQ.1)RETURN
      ELSE IF(I.EQ.77)THEN
        CALL FORMAT(0,'Manipulate an item!/')
        FLAG=3
        CALL MANIPULATE(J,3)
        IF(FLAG.EQ.7)RETURN
      ELSE IF(I.EQ.65)THEN
        CALL FORMAT(0,'Expert map mode <re>set')
        EXPERT=1-EXPERT
      ELSE IF(I.EQ.66)THEN
        IF(BLINK.GT.0)THEN
          CALL BLNK
          CALL FORMAT(0,'Blink!!!!')
          RETURN
          ENDIF
      ELSE IF(I.EQ.80)THEN
        CALL FORMAT(0,'Print the map')
        PRINTCONTROL=1
        CALL QPRINT
      ELSE IF(I.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +N - to move north!/
     +E - to move east!/
     +S - to move south!/
     +W - to move west!/
     +X - current statistics!/
     +Z - personal statistics!/
     +I - inventory!/
     +0 - stay in the same location!/
     +U - update character file!/
     +M - manipulate an item!/
     +Q - quit and save your character!/
     +A - expert map mode <re>set!/
     +T - dungeon time!/
     +D - change delay time (default is 2)!/
     +P - print the map!/
     +C - cast a spell')
        IF(WISH.GT.0)CALL FORMAT(2,'Y - make a wish')
      ELSE IF(I.EQ.89)THEN
        IF(WISH.GT.0)THEN
          CALL FORMAT(0,'Make a wish')
          CALL WISH1
          CALL ARMOR
          IF(LIFE.EQ.0)RETURN
          ENDIF
      ELSE IF(I.EQ.48.OR.I.EQ.0)THEN
        CALL FORMAT(0,'Stay here.')
        RETURN
      ELSE IF(I.EQ.68)THEN
        CALL FORMAT(0,'Reset delay!/!/
     +Enter delay time in seconds: ')
        CALL INPUTNUMBER(DELAY)
        IF(DELAY.LT.0)DELAY=0
        IF(DELAY.GT.4)DELAY=4
        ENDIF
      GOTO 1

80    IF(WALL(2).EQ.'M')GOTO 900
      IF(LOCATION.EQ.13)GOTO 200
      IF(WALL(2).EQ.':')GOTO 300
      IF(WALL(2).NE.'!')GOTO 81
      IF(DICE(1,15).EQ.1)LOCKED(2)=1
      IF(LOCKED(2).NE.0)GOTO 100
81    YCOORD=YCOORD+1
      YCOORD=WRAPAROUND(YCOORD,LEVELWIDTH)
      GOTO 400

85    IF(WALL(1).EQ.'MMMMM')GOTO 900
      IF(LOCATION.EQ.12)GOTO 200
      IF(WALL(1).EQ.'M:::M')GOTO 300
      IF(WALL(1).NE.'M---M')GOTO 86
      IF(DICE(1,15).EQ.1)LOCKED(1)=1
      IF(LOCKED(1).NE.0)GOTO 100
86    XCOORD=XCOORD-1
      XCOORD=WRAPAROUND(XCOORD,LEVELLENGTH)
      GOTO 400

90    IF(WALL(3).EQ.'MMMMM')GOTO 900
      IF(LOCATION.EQ.14)GOTO 200
      IF(WALL(3).EQ.'M:::M')GOTO 300
      IF(WALL(3).NE.'M---M')GOTO 91
      IF(DICE(1,15).EQ.1)LOCKED(3)=1
      IF(LOCKED(3).NE.0)GOTO 100
91    XCOORD=XCOORD+1
      XCOORD=WRAPAROUND(XCOORD,LEVELLENGTH)
      GOTO 400

95    IF(WALL(4).EQ.'M')GOTO 900
      IF(LOCATION.EQ.15)GOTO 200
      IF(WALL(4).EQ.':')GOTO 300
      IF(WALL(4).NE.'!')GOTO 96
      IF(DICE(1,15).EQ.1)LOCKED(4)=1
      IF(LOCKED(4).NE.0)GOTO 100
96    YCOORD=YCOORD-1
      YCOORD=WRAPAROUND(YCOORD,LEVELWIDTH)

400   EXPERIENCE=EXPERIENCE+DICE(1,4)
      RETURN

900   CALL SINGLE(7)
      CALL FORMAT(3,'%% Illegal Move %%')
      GOTO 1

200   CALL FORMAT(4,'There is an enchanted stream running across
     + the dungeon floor.!/')
      IF((FINDMAGIC(41).GT.0).OR.(STATS(5).GT.16.AND.DICE(1,20)
     +.EQ.1))THEN
        CALL FORMAT(0,'You quickly wade through it, however.')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        GOTO 101
      ELSE
        CALL FORMAT(0,'You are unable to pass through it without
     + magical aid.')
        ENDIF
      GOTO 1

300   IF(I.EQ.78.OR.I.EQ.56)THEN
        J=V(2,2,1)-4
      ELSE IF(I.EQ.69.OR.I.EQ.54)THEN
        J=V(2,3,2)-4
      ELSE IF(I.EQ.83.OR.I.EQ.50)THEN
        J=V(3,2,1)-4
      ELSE
        J=V(2,2,2)-4
        ENDIF

301   CALL FORMAT(3,'There is a huge ')
      CALL TRIMMER(TYPE(J))
      CALL FORMAT(0,' gate barring travel in that
     + direction.!/What do you wish to do ("H" for Help) ?  ')
      CALL INPUT(K,0)
      IF(K.NE.76.AND.K.NE.0.AND.K.NE.84.AND.K.NE.72)GOTO 301
      IF(K.EQ.76.OR.K.EQ.0)THEN
        CALL FORMAT(0,'Leave it alone')
        GOTO 1
      ELSE IF(K.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +L - leave the gate alone!/
     +T - try a key')
        GOTO 301
      ELSE
        CALL FORMAT(0,'Try a key')
        IF(FINDMAGIC(70).EQ.0.AND.FINDMAGIC(71).EQ.0.AND.
     +FINDMAGIC(72).EQ.0)THEN
          CALL FORMAT(4,'You have nothing which will open the gate.')
          GOTO 1
          ENDIF
        ENDIF
      IF(FINDMAGIC(72).GT.0)THEN
305     CALL FORMAT(3,'The gate swings silently inward...........')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        GOTO 101
      ELSE IF(FINDMAGIC(71).GT.0.AND.(J.EQ.1.OR.J.EQ.2))THEN
        GOTO 305
      ELSE IF(FINDMAGIC(70).GT.0.AND.J.EQ.1)THEN
        GOTO 305
      ELSE
        CALL FORMAT(3,'The key you have doesn''t fit into the ornate
     + lock on the door.')
        GOTO 1
        ENDIF


100   IF(FINDMAGIC(69).GT.0)THEN
101     IF(I.EQ.69.OR.I.EQ.54)THEN
          GOTO 81
        ELSE IF(I.EQ.78.OR.I.EQ.56)THEN
          GOTO 86
        ELSE IF(I.EQ.83.OR.I.EQ.50)THEN
          GOTO 91
        ELSE
          GOTO 96
          ENDIF
        ENDIF

105   CALL FORMAT(3,'The door is locked.!/What do you wish to do
     + ("H" for Help) ?   ')
      CALL INPUT(J,1)
      IF(J.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +O - open it!/
     +L - leave it alone!/
     +X - current statistics!/
     +Z - personal statistics!/
     +I - inventory')
      ELSE IF(J.EQ.76.OR.J.EQ.0)THEN
        CALL FORMAT(0,'Leave it alone')
        GOTO 1
      ELSE IF(J.EQ.79)THEN
        CALL FORMAT(0,'Open it')
        IF((FINDMAGIC(31).GT.0.OR.(DICE(1,6)+STATS(1).GT.17)).AND.
     +(FINDMAGIC(68).EQ.0))THEN
          CALL FORMAT(3,'You have gotten through the door!!')
          IF(DELAY.NE.0)CALL QSLEEP(DELAY)
          GOTO 101
        ELSE
          CALL FORMAT(3,'You cannot open the door.')
          GOTO 105
          ENDIF
         ENDIF
      GOTO 105
     
      END




*****
*
*  CAST allows spell-using characters to cast spells.
*
*****

      SUBROUTINE CAST(DAMAGE,WHERE)
      INCLUDE 'qstcom.inc'
      INTEGER DAMAGE,WHERE
      LOGICAL MONSAV

      DAMAGE=0
      IF(CLASS.EQ.7.OR.CLASS.EQ.10)THEN
        CALL FORMAT(3,'Only clerics and magicians have the ability
     + to cast spells.')
        RETURN
       ENDIF

1     CALL FORMAT(3,'Spell LEVEL (-1 to exit / 0 to list spells)
     + ?   ')
      CALL INPUTNUMBER(I)
      IF(I.LT.0.OR.I.GT.5)THEN
        RETURN
      ELSE IF(I.EQ.0)THEN
        CALL LISTSPELLS
        GOTO 1
      ELSE
        IF(IPICK(SPELLS,I,0).EQ.0)THEN
          CALL FORMAT(3,'You have no level ')
          CALL OUTNUM(I)
          CALL FORMAT(0,' spells left.')
          GOTO 1
          ENDIF
          ENDIF

2     CALL FORMAT(3,'Spell NUMBER (-1 to exit / 0 to list spells)?   ')
      CALL INPUTNUMBER(I1)
      IF(I1.EQ.0)THEN
        CALL LISTSPELLS
        GOTO 1
      ELSE IF(I1.LT.0)THEN
        RETURN
      ELSE IF(I1.GT.4)THEN
        GOTO 2
      ELSE IF(I.EQ.5.AND.I1.GT.3)THEN
        GOTO 2
      ELSE IF(I.EQ.4.AND.I1.GT.3)THEN
        GOTO 2
      ELSE IF(I.EQ.3.AND.I1.GT.4)THEN
        GOTO 2
        ENDIF
      SPELLS=SPELLS-10**(I-1)
      GOTO(3,4,5,6,7) I

3     IF(CLASS.EQ.4)GOTO(45,50,55,60) I1
      GOTO(45,65,55,70) I1

4     IF(CLASS.EQ.4)GOTO(81,85,90,110) I1
      GOTO(95,100,105,110) I1

5     IF(CLASS.EQ.4)GOTO(121,125,130,215) I1
      GOTO(135,140,145,215) I1

6     IF(CLASS.EQ.4)GOTO(161,165,220) I1
      GOTO(170,175,220) I1

7     IF(CLASS.EQ.4)GOTO(201,210,230) I1
      GOTO(205,210,225) I1 

45    CALL FORMAT(3,'Protection from Evil. This spell will protect
     + you from all undead and!/will lower your armor class.')
      PROTEVIL=8
      CALL ARMOR
      IF(WHERE.EQ.1)THEN
        FLAG=5
        IF(MONUNDEADMAGIC.EQ.1.OR.MONUNDEADMAGIC.EQ.3)FLAG=4
        ENDIF
      RETURN


50    CALL FORMAT(3,'Magic Missles................')
      IF(WHERE.EQ.3)GOTO 700
      IF(WHERE.EQ.2)GOTO 710
      DAMAGE=DICE(INT(FLOAT(CHARLVL+1)/2.),4)+CHARLVL
      CALL FORMAT(2,'They hit..........they did ')
      CALL OUTNUM(DAMAGE)
      CALL FORMAT(0,' points of damage.')
      RETURN

55    CALL FORMAT(3,'Detect Trap................')
      IF(WHERE.EQ.1.OR.WHERE.EQ.3)GOTO 700
      FLAG=7
      IF(TRAP.EQ.0)RETURN

56    CALL FORMAT(2,'It is trapped. Do you still wish to open it?   ')
      CALL INPUT(J,2)
      IF(J.EQ.89)THEN
        FLAG=7
      ELSE
        FLAG=8
        ENDIF
      RETURN
      
60    CALL FORMAT(3,'Sleep...............')
      IF(WHERE.GT.1)GOTO 700
      IF(MONSAV(MONLVL,MONNUM).OR.MONUNDEADMAGIC.EQ.1.OR.
     +MONUNDEADMAGIC.EQ.3)THEN
        CALL FORMAT(2,'It was not affected by your spell.')
        RETURN
        ENDIF
64    CALL FORMAT(3,'It has fallen asleep. Do you wish to kill it?   ')
      CALL INPUT(J,2)
      IF(J.EQ.78.OR.J.EQ.0)THEN
        FLAG=2
      ELSE IF(J.EQ.89)THEN
        FLAG=6
        IF(DICE(1,CHARLVL+10).GT.1)RETURN
        FLAG=0
        CALL FORMAT(3,'It is no longer under the influence of your
     + spell!!!/What will power!!')
      ELSE
        GOTO 64
        ENDIF
      RETURN

65    J=DICE(1,8)
      IF(STATS(3).GT.14)J=J+1
      HITPOINTS=HITPOINTS+J
      IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
      CALL FORMAT(3,'Cure light wounds.!/You have healed some of
     + your wounds. Current hit points: ')
      CALL OUTNUM(HITPOINTS)
      RETURN

70    CALL FORMAT(3,'Light............')
      IF(WHERE.GT.1)GOTO 700
      BLIND=2
      CALL FORMAT(2,'You have blinded the monster with a dazzling
     + light.')
      RETURN

81    CALL FORMAT(3,'Web spell..........')
      IF(WHERE.GT.1)GOTO 700
      IF(MONNUM.EQ.14.OR.MONNUM.EQ.19.OR.MONSAV(MONLVL,MONNUM))THEN
        CALL FORMAT(2,'The web was not able to hold the monster.')
      ELSE
        FLAG=6
        CALL FORMAT(2,'The monster is tied up in the sticky mass of
     + webs and cannot move.')
        ENDIF
      RETURN

85    CALL FORMAT(3,'Fear..............')
      IF(WHERE.GT.1)GOTO 700
      IF(MONSAV(MONLVL,MONNUM))THEN
        CALL FORMAT(2,'The monster does not seem afraid.')
      ELSE
        FLAG=2
        CALL FORMAT(2,'The monster is moving away from you at
     + illegal speeds.')
        ENDIF
      RETURN

90    CALL FORMAT(3,'Stinking cloud........')
      IF(WHERE.GT.1)GOTO 700
      CALL FORMAT(2,'Billowing clouds of sickly green gas float
     + towards the monster.')
      IF(MONUNDEADMAGIC.EQ.1.OR.MONUNDEADMAGIC.EQ.3)THEN
        CALL FORMAT(3,'The cloud seems to have no effect on the
     + monster.')
      ELSE
        DAMAGE=DICE(2,6)+CHARLVL
        CALL FORMAT(3,'It is caught in the cloud. It is taking
     + damage.')
        ENDIF
      RETURN

95    CALL FORMAT(3,'Spiritual Hammer.')
      IF(WHERE.GT.1)GOTO 700
      DAMAGE=DICE(2,5)+CHARLVL
      CALL FORMAT(2,'This spell is granted by your diety. It will
     + last for one!/round only.!/!/You have hit with your hammer.
     + You did ')
      CALL OUTNUM(DAMAGE)
      CALL FORMAT(0,' points of damage.')
      RETURN

100   CALL FORMAT(3,'Continual Light.')
      IF(WHERE.GT.1)GOTO 700
      BLIND=4
      CALL FORMAT(2,'Your very intense light has blinded the
     + monster.')
      FLAG=5
      RETURN

105   CALL FORMAT(3,'Sanctuary.')
      IF(WHERE.GT.1)GOTO 700
      IF(MONSAV(MONLVL,MONNUM))THEN
        CALL FORMAT(2,'The monster is ignoring you as you chant.!/
     +Your spell has had no effect.')
      ELSE
        FLAG=2
        CALL FORMAT(2,'The monster is no longer interested in you.
     + It is!/wandering away.')
        ENDIF
      RETURN

110   CALL FORMAT(3,'Limited transport.!/This spell has the ability
     + to transport you to the stairs up!/to the next level.')
      IF(WHERE.LT.3)RETURN
      IF(DUNLVL.LT.5)THEN
        XCOORD=STAIRSUPX
        YCOORD=STAIRSUPY
      ELSE
        XCOORD=DICE(1,LEVELLENGTH)
        YCOORD=DICE(1,LEVELWIDTH)
        ENDIF
      FLAG=1
      RETURN

121   CALL FORMAT(3,'Blink!/This spell will allow you to move
     + randomly through!/the dungeon. To blink type "B".')
      BLINK=9
      FLAG=3
      RETURN

125   CALL FORMAT(3,'Fireball...........')
      IF(WHERE.EQ.2)THEN
        GOTO 710
      ELSE IF(WHERE.EQ.3)THEN
        GOTO 700
        ENDIF
      DAMAGE=DICE(CHARLVL,6)
      IF(MONSAV(MONLVL,MONNUM))DAMAGE=INT(FLOAT(DAMAGE+1)/2.)
      CALL FORMAT(2,'A huge fiery ball of glowing red flames flies
     + from your fingers.!/The monster is enveloped in flames!!')
      RETURN

130   CALL FORMAT(3,'Feign Death.')
      IF(WHERE.GT.1)GOTO 700
      IF(MONSAV(MONLVL,MONNUM))THEN
        CALL FORMAT(2,'The monster is not fooled by your simple
     + illusion.')
      ELSE
        FLAG=2
        CALL FORMAT(2,'The monster, seeing you in a cataleptic
     + state, believes you are dead.!/It has wandered away.')
        ENDIF
      RETURN

135   J=DICE(2,8)+3
      IF(STATS(3).GT.14)J=J+1
      HITPOINTS=HITPOINTS+J
      IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
      CALL FORMAT(3,'Cure serious wounds.!/You have healed some
     + wounds. Current hit points: ')
      CALL OUTNUM(HITPOINTS)
      RETURN

140   DISEASE=0
      CALL FORMAT(3,'Cure Disease.!/You have cured any disease
     + that you may have contracted.')
      RETURN

145   CALL FORMAT(3,'Insect Plague.')
      IF(WHERE.GT.1)GOTO 700
      DAMAGE=DICE(CHARLVL,6)
      IF(MONSAV(MONLVL,MONNUM))DAMAGE=INT(FLOAT(DAMAGE+1)/2.)
      CALL FORMAT(2,'Swarms of insects have leapt from your hand
     + and are now swarming!/over the monster!!')
      RETURN

161   CALL FORMAT(3,'Passwall.')
      IF(WHERE.NE.3)GOTO 700
162   CALL FORMAT(2,'Which wall (N,E,S,W) ?   ')
      CALL INPUT(J,0)
      IF(J.EQ.0)THEN
        RETURN
      ELSE IF(J.EQ.78)THEN
        CALL FORMAT(0,'North')
        PICTURE(5)(8:12)=' '
        WALL(1)=' '
      ELSE IF(J.EQ.69)THEN
        CALL FORMAT(0,'East')
        PICTURE(6)(13:13)=' '
        PICTURE(7)(13:13)=' '
        PICTURE(8)(13:13)=' '
        WALL(2)=' '
      ELSE IF(J.EQ.83)THEN
        CALL FORMAT(0,'South')
        PICTURE(9)(8:12)=' '
        WALL(3)=' '
      ELSE IF(J.EQ.87)THEN
        CALL FORMAT(0,'West')
        PICTURE(6)(7:7)=' '
        PICTURE(7)(7:7)=' '
        PICTURE(8)(7:7)=' '
        WALL(4)=' '
      ELSE
        GOTO 162
        ENDIF

      PRINTCONTROL=1
      CALL QPRINT
      RETURN

165   CALL FORMAT(3,'Ice Storm..............')
      IF(WHERE.EQ.2)GOTO 710
      IF(WHERE.EQ.3)GOTO 700
      DAMAGE=DICE(3,8)+CHARLVL
      IF(MONSAV(MONLVL,MONNUM))DAMAGE=INT(FLOAT(DAMAGE+1)/2.)
      CALL FORMAT(2,'Your enemy is covered with a layer of ice.')
      RETURN

170   J=DICE(3,8)+3
      IF(STATS(3).GT.14)J=J+1
      HITPOINTS=HITPOINTS+J
      IF(HITPOINTS.GT.TOTALHITPOINTS)HITPOINTS=TOTALHITPOINTS
      CALL FORMAT(2,'Cure Critical Wounds.!/!/You have healed some
     + wounds. Current hit points: ')
      CALL OUTNUM(HITPOINTS)
      RETURN

175   CALL FORMAT(3,'Flame strike!!')
      IF(WHERE.EQ.2)GOTO 710
      IF(WHERE.EQ.3)GOTO 700
      DAMAGE=DICE(6,8)
      IF(MONSAV(MONLVL,MONNUM))DAMAGE=INT(FLOAT(DAMAGE+1)/2.)
      CALL FORMAT(2,'Intricate whirlwinds of fire leap towards
     + the monster !!!/It has been fried.')
      RETURN

201   CALL FORMAT(3,'Cone of Cold................')
      IF(WHERE.EQ.2)GOTO 710
      IF(WHERE.EQ.3)GOTO 700
      DAMAGE=DICE(CHARLVL,DICE(1,4)+1)
      CALL FORMAT(2,'Silvery rays of frost spring from your fingers
     +...!/The monster has taken an icy blow.')
      RETURN

205   CALL FORMAT(3,'Holy Word.')
      IF(WHERE.GT.1)GOTO 700
      FLAG=6
      IF(MONUNDEADMAGIC.EQ.1.OR.MONUNDEADMAGIC.EQ.3)THEN
        DAMAGE=300
        CALL FORMAT(2,'The power of the Holy Word has vaporized
     + the monster.')
      ELSE
        DAMAGE=CHARLVL*4
        CALL FORMAT(2,'The Holy Word has caused the monster great
     + harm.')
        ENDIF
      RETURN

210   CALL FORMAT(3,'Remove Curse')
      CALL LISTMAGIC
      CALL FORMAT(3,'Number of the item you wish to be rid of?   ')
      CALL INPUTNUMBER(I)
      IF(I.LT.1.OR.I.GT.MAGICPERLEVEL(CHARLVL))RETURN
      IF(MAGIC(I).EQ.0)RETURN
      MAGIC(I)=0
      CALL FORMAT(2,'You are rid of that item.!/!/')
      RETURN

215   CALL FORMAT(3,'Extended Protection from Evil. This spell will
     + protect you from all undead and!/will lower your armor class.')
      PROTEVIL=40
      CALL ARMOR
      IF(WHERE.EQ.1)THEN
        FLAG=5
        IF(MONUNDEADMAGIC.EQ.1.OR.MONUNDEADMAGIC.EQ.3)FLAG=4
        ENDIF
      RETURN

220   CALL FORMAT(3,'Reverse transport.!/This spell has the ability
     + to transport you to the stairs down!/to the next level.')
      IF(WHERE.LT.3)RETURN
      IF(DUNLVL.LT.5)THEN
        XCOORD=STAIRSDOWNX
        YCOORD=STAIRSDOWNY
      ELSE
        XCOORD=DICE(1,LEVELLENGTH)
        YCOORD=DICE(1,LEVELWIDTH)
        ENDIF
      FLAG=1
      RETURN

225   CALL FORMAT(3,'Word of Recall!/!/')
      CALL LEAVING

230   CALL FORMAT(3,'Prismatic Sphere!/')
      DAMAGE=5
      IF(.NOT.MONSAV(MONLVL-9,MONNUM))THEN
        CALL FORMAT(2,'Yellow ring - 8 points.')
        DAMAGE=DAMAGE+8
        IF(.NOT.MONSAV(MONLVL-7,MONNUM))THEN
          CALL FORMAT(2,'Orange ring - 16 points.')
          DAMAGE=DAMAGE+16
          IF(.NOT.MONSAV(MONLVL-5,MONNUM))THEN
            CALL FORMAT(2,'Red ring - 25 points.')
            DAMAGE=DAMAGE+25
            IF(.NOT.MONSAV(MONLVL-3,MONNUM))THEN
       CALL FORMAT(2,'Violet ring - monster is permanently blind.')
       BLIND=4
       IF(.NOT.MONSAV(MONLVL+1,MONNUM))THEN
         CALL FORMAT(2,'Black ring - death.')
         DAMAGE=3000
         ENDIF
            ENDIF
          ENDIF
        ENDIF
      ENDIF
      RETURN

700   CALL FORMAT(3,'That was a worthless waste of a spell.')
      RETURN

710   CALL FORMAT(3,'You have destroyed the treasure.')
      FLAG=9
      RETURN

      END




*****
*
*  PRINT_DAMAGE prints the number of points of damage the player has taken.
*
*****

      SUBROUTINE PRINT_DAMAGE(I)
      INCLUDE 'qstcom.inc'
      INTEGER I

      HITPOINTS=HITPOINTS-I
      IF(HITPOINTS.LT.1)LIFE=0
      CALL FORMAT(0,'You have taken ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' points of damage.!/Current hit points: ')
      CALL OUTNUM(HITPOINTS)

      RETURN
      END



*****
*
*  LISTSPELLS lists all spells the player can currently cast.
*
*****

      SUBROUTINE LISTSPELLS
      INCLUDE 'qstcom.inc'

      CALL FORMAT(3,'Spells you may currently cast:')
      IF(IPICK(SPELLS,1,0).GT.0)THEN
        IF(CLASS.EQ.4)THEN
          CALL FORMAT(3,'Level 1:!/
     +1. Protection from Evil!/
     +2. Magic Missles!/
     +3. Detect Trap!/
     +4. Sleep')
        ELSE
          CALL FORMAT(3,'Level 1:!/
     +1. Protection from Evil!/
     +2. Cure Light Wounds!/
     +3. Detect Trap!/
     +4. Light')
          ENDIF
        ENDIF

      IF(IPICK(SPELLS,2,0).GT.0)THEN
        IF(CLASS.EQ.4)THEN
          CALL FORMAT(3,'Level 2:!/
     +1. Web!/
     +2. Fear!/
     +3. Stinking Cloud!/
     +4. Limited Transport')
        ELSE
          CALL FORMAT(3,'Level 2:!/
     +1. Spiritual Hammer!/
     +2. Continual Light!/
     +3. Sanctuary!/
     +4. Limited Transport')
          ENDIF
        ENDIF

      IF(IPICK(SPELLS,3,0).GT.0)THEN
        IF(CLASS.EQ.4)THEN
          CALL FORMAT(3,'Level 3:!/
     +1. Blink!/
     +2. Fireball!/
     +3. Feign Death!/
     +4. Extended Protection From Evil')
        ELSE
          CALL FORMAT(3,'Level 3:!/
     +1. Cure Serious Wounds!/
     +2. Cure Disease!/
     +3. Insect Plague!/
     +4. Extended Protection From Evil')
          ENDIF
        ENDIF

      IF(IPICK(SPELLS,4,0).GT.0)THEN
        IF(CLASS.EQ.4)THEN
          CALL FORMAT(3,'Level 4:!/
     +1. Passwall!/
     +2. Ice Storm!/
     +3. Reverse Limited Transport')
        ELSE
          CALL FORMAT(3,'Level 4:!/
     +1. Cure Critical Wounds!/
     +2. Flame Strike!/
     +3. Reverse Limited Transport')
          ENDIF
        ENDIF

      IF(IPICK(SPELLS,5,0).GT.0)THEN
        IF(CLASS.EQ.4)THEN
          CALL FORMAT(3,'Level 5:!/
     +1. Cone of Cold!/
     +2. Remove Curse!/
     +3. Prismatic Sphere')
        ELSE
          CALL FORMAT(3,'Level 5:!/
     +1. Holy Word!/
     +2. Remove Curse!/
     +3. Word of Recall')
          ENDIF
        ENDIF

      RETURN
      END




*****
*
*  STAIRSUP allows the user to go up to the next dungeon level.
*
*****

      SUBROUTINE STAIRSUP
      INCLUDE 'qstcom.inc'

      CALL SYMBOL(9)
      CALL QPRINT
      CALL FORMAT(3,'You have found some steps up. Would you like to
     +!/follow them?  ')
      CALL INPUT(I,2)
      IF(I.NE.89)RETURN
      IF(DUNLVL-1.GT.0)THEN
        DUNLVL=DUNLVL-1
        CALL GETDUNGEON(0,DUNLVL)
        XCOORD=STAIRSDOWNX
        YCOORD=STAIRSDOWNY
        FLAG=1
      ELSE
        CALL GOLDTOEXPERIENCE
        CALL FORMAT(3,'You have made it out of the dungeon. Congrats.')
        CALL CHECKLEVEL
        CALL SPELL(SPELLS)
        CALL FORMAT(3,'Would you like to reenter?   ')
        CALL INPUT(I,2)
        IF(I.EQ.78)CALL LEAVING
        ENDIF
      RETURN
      END



*****
*
*  STAIRSDOWN allows the player to go down to the next dungeon level.
*
*****

      SUBROUTINE STAIRSDOWN
      INCLUDE 'qstcom.inc'

      CALL SYMBOL(9)
      CALL QPRINT
      CALL FORMAT(3,'You have found some steps down. Would you like
     + to!/follow them?   ')
      CALL INPUT(I,2)
      IF(I.NE.89)RETURN
      DUNLVL=DUNLVL+1
      CALL GETDUNGEON(0,DUNLVL)
      XCOORD=STAIRSUPX
      YCOORD=STAIRSUPY
      FLAG=1
      RETURN
      END




*****
*
*  TELEPORTER contains the special room "TELEPORTER".
*
*****

      SUBROUTINE TELEPORTER
      INCLUDE 'qstcom.inc'

      PICTURE(6)(9:11)='TEL'
      CALL QPRINT
      IF((DICE(1,50).LT.3).OR.(FINDMAGIC(32).GT.0.AND.DICE(1,100)
     +.LT.93))THEN
        CALL FORMAT(4,'You were able to avoid the teleporter.')
        RETURN
        ENDIF
      CALL FORMAT(3,'You feel funny.!/Energy is pulsating through
     + you.!/It''s a teleporter.!/You''ve been teleported!!!!')
      IF(LOCATION.EQ.4)THEN
        DUNGEON=DICE(1,6)
        DUNLVL=DICE(1,7)
        CALL GETDUNGEON(DUNGEON,DUNLVL)
      ELSE IF(LOCATION.EQ.3)THEN
        DUNLVL=DICE(1,7)
        CALL GETDUNGEON(0,DUNLVL)
        ENDIF
      XCOORD=DICE(1,LEVELLENGTH)
      YCOORD=DICE(1,LEVELWIDTH)
      FLAG=1
      RETURN
      END




*****
*
*  CLOUD contains the special room "CLOUD".
*
*****

      SUBROUTINE CLOUD(I)
      INCLUDE 'qstcom.inc'
      LOGICAL SAVINGTHROW

      PICTURE(6)(9:11)='())'
      PICTURE(7)(8:12)='( X )'
      PICTURE(8)(9:12)='() )'
      CALL QPRINT

      IF(SAVINGTHROW(STATS(4)).OR.FINDMAGIC(45).GT.0)THEN
1       CALL FORMAT(3,'The cloud of gas has had no effect on you.')
        RETURN
        ENDIF

      IF(I.EQ.1)THEN
        CALL FORMAT(3,'This is a cloud of sleep gas. You have fallen
     + asleep.!/In a short time you wake up.')
        MOVES=MOVES+180
        HITPOINTS=TOTALHITPOINTS
        IF(MOVES.GT.365)THEN
          MOVES=MOVES-365
          AGE=AGE+1
          ENDIF
      ELSE IF(I.EQ.2)THEN
        IF(FINDMAGIC(57).GT.0)GOTO 1
        I=DICE(DUNLVL,6)
        CALL FORMAT(3,'You''ve stepped into a poisonous gas cloud.
     +!/')
        CALL PRINT_DAMAGE(I)
      ELSE IF(I.EQ.3)THEN
        LIFE=0
        CALL FORMAT(3,'You''ve stepped into a poisonous gas cloud.')
      ELSE
        FLAG=1
        CALL BLNK
        CALL BLNK
        CALL FORMAT(3,'You''ve become lost in the fog. You seem to
     + have!/wandered to a new location.')
        IF(DELAY.NE.0)CALL QSLEEP(DELAY)
        ENDIF
      RETURN
      END




*****
*
*  DECK is the deck of many things. Draw and die!
*
*****

      SUBROUTINE DECK
      INCLUDE 'qstcom.inc'
      INTEGER CARD(17)

      DO I=1,17
        CARD(I)=I
      ENDDO

2     CALL FORMAT(3,'You have found a deck of cards. It is a deck
     + of many things.!/A very, very magical deck of cards. The card
     + you draw will be the last,!/for the deck will vanish before
     + your eyes. This deck has 17 cards,!/Jack through Ace of each
     + suit, and a Joker. I will tell the!/power of two cards:!/!/
     +Ace of Hearts: 50000 experience points!/
     +Ace of Spades: Death!/!/
     +Do you wish to draw a card?   ')

      CALL INPUT(I,2)
      IF(I.EQ.78)THEN
        RETURN
      ELSE IF(I.NE.89)THEN
        GOTO 2
        ENDIF

      CALL FORMAT(3,'Shuffling...............')
      DO I=1,30
        DO J=1,17
          I1=DICE(1,17)
          K=CARD(J)
          CARD(J)=CARD(I1)
          CARD(I1)=K
        ENDDO
      ENDDO

5     CALL FORMAT(3,'Number of the card you wish to draw (1-17) ?   ')
      CALL INPUTNUMBER(I)

      IF(I.EQ.0)RETURN
      IF(I.LT.1.OR.I.GT.17)GOTO 5
      GOTO(10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90)CARD(I)

10    ADJSAVTHR=ADJSAVTHR-2
      CALL FORMAT(3,'Ace of Spades. All saving throws are at -2.')
      RETURN

15    EXPERIENCE=EXPERIENCE-20000
      CALL FORMAT(3,'King of Spades. Lose 20000 experience points.')
      CALL CHECKLEVEL
      RETURN

20    LIFE=0
      CALL FORMAT(3,'Queen of Spades. Death.')
      RETURN

25    GOLD=0
      CALL FORMAT(3,'Jack of Spades. Your credit ring is worthless.')
      RETURN

30    CALL FORMAT(3,'Ace of Clubs. All statistics have dropped 2.')
      CALL CHANGESTATS(-2,0)
      RETURN

35    ADJTOAC=ADJTOAC+2
      CALL FORMAT(3,'King of Clubs. Your armor class has been
     + permanently raised 2.')
      RETURN

40    LIFE=0
      CALL FORMAT(3,'Queen of Clubs. You have turned to stone.!/
     +A statue for all foolish adventurers......')
      RETURN

45    GOLD=0
      GOLDONPERSON=0
      CALL REMOVEMAGIC(0,0,J)
      CALL FORMAT(3,'Jack of Clubs. All property is torn from you.')
      RETURN

50    CALL FORMAT(3,'Ace of Diamonds. Gain 50000 gold pieces.')
      CALL ADDGOLD(50000)
      RETURN

55    CALL FORMAT(3,'King of Diamonds. All statistics have gone
     + up 2.')
      CALL CHANGESTATS(2,0)
      RETURN

60    ADJSAVTHR=ADJSAVTHR+2
      CALL FORMAT(3,'Queen of Diamonds. All saving throws are at
     + +2.')
      RETURN

65    WISH=WISH+1
      CALL FORMAT(3,'Jack of Diamonds. You have gained a wish.')
      RETURN

70    EXPERIENCE=EXPERIENCE+50000
      CALL FORMAT(3,'Ace of Hearts. Gain 50000 experience points.')
      CALL CHECKLEVEL
      RETURN

75    HITPOINTS=HITPOINTS+10
      TOTALHITPOINTS=TOTALHITPOINTS+10
      CALL FORMAT(3,'King of Hearts. Gain 10 hitpoints.')
      RETURN

80    CALL FORMAT(3,'Queen of Hearts. Your gold has tripled in value.')
      CALL ADDGOLD(GOLD*2)
      RETURN

85    CALL FORMAT(3,'Jack of Hearts. Gain 3 magic items.')
      CALL LOCATEMAGIC(-1)
      CALL LOCATEMAGIC(-1)
      CALL LOCATEMAGIC(-1)
      RETURN

90    EXPERIENCE=EXPERIENCE+20000
      CALL FORMAT(3,'Joker. Gain 20000 experience points. You may
     + draw again.')
      CALL CHECKLEVEL
      GOTO 2

      END




*****
*
*  THRONE is the throne routine. Take a seat.
*
*****

      SUBROUTINE THRONE
      INCLUDE 'qstcom.inc'
      CHARACTER TEXTURE(6)*6
      DATA TEXTURE/'Bronze','Brick','Lava','Wood','Brass','Stone'/

      FLAG=0
      PICTURE(6)(9:11)='THR'
      CALL QPRINT
      I=DICE(1,6)
1     CALL FORMAT(3,'You have discovered a throne of ')
      CALL TRIMMER(TEXTURE(I))
      CALL FORMAT(2,'Would you like to sit on it?   ')
      CALL INPUT(J,2)
      IF(J.EQ.78.OR.J.EQ.0)RETURN
      IF(J.NE.89)GOTO 1

      I=DICE(1,10)
      GOTO(2,3,4,5,6,7,8,9,10,11) I

2     CALL FORMAT(3,'There is a deck of cards on the seat of the
     + throne.')
      CALL DECK
      RETURN

3     ADJSAVTHR=ADJSAVTHR+1
      ADJTOAC=ADJTOAC-1
      CALL FORMAT(3,'The throne has conferred a +1 protection on
     + your!/armor class and saving throws.')
      RETURN

4     XCOORD=DICE(1,LEVELLENGTH)
      YCOORD=DICE(1,LEVELWIDTH)
      FLAG=1
      CALL FORMAT(3,'As you sit on the throne, the room begins to
     + spin.!/This is a teleporter!!!!!/You''ve been teleported!!!!')
      RETURN

5     I=DICE(1,18)+9
      CALL FORMAT(3,'You''ve been shocked by the throne. ')
      CALL PRINT_DAMAGE(I)
      RETURN

6     I=DICE(1,30000)+20000
      CALL REMOVEMAGIC(0,0,J)
      CALL FORMAT(3,'A vast pile of ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' gold pieces appears in front of you.!/
     +You take it but.......')
      CALL ADDGOLD(I)
      RETURN

7     CALL FORMAT(3,'Nothing seems to happen.')
      RETURN

8     ADJTOHIT=ADJTOHIT-2
      CALL FORMAT(3,'A huge Ogre bounds into the room, gives your
     + arm a stress test,!/and bounds away. Your mangled arm will
     + fight less effectively.')
      RETURN

9     CALL FORMAT(3,'There appears to be something lying on the
     + seat of the throne.')
      CALL LOCATEMAGIC(0)
      RETURN

10    LIFE=0
      CALL FORMAT(3,'As you sit on the throne, it begins to fold up
     + on you,!/trapping you and slowly crushing the life from you!!')
      RETURN

11    I=DICE(1,9)
      HITPOINTS=HITPOINTS+I
      TOTALHITPOINTS=TOTALHITPOINTS+I
      CALL FORMAT(3,'Energy is flowing through you.!/You have
     + gained ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' hit points !!')
      RETURN
      END



*****
*
*  POOL contains the special room "POOL".
*
*****

      SUBROUTINE POOL
      INCLUDE 'qstcom.inc'
      CHARACTER TEMPERATURE(2)*7
      DATA TEMPERATURE/'warmer.','colder.'/

      FLAG=0
      PICTURE(6)(9:11)='[_]'
      CALL QPRINT

      I=DICE(1,9)
1     CALL FORMAT(3,'You have found a pool filled with a ')
      CALL TRIMMER(COLOR(I))
      CALL FORMAT(0,' liquid.!/Would you like to enter it ?    ')
      CALL INPUT(J,2)
      IF(J.EQ.78.OR.J.EQ.0)RETURN
      IF(J.NE.89)GOTO 1

      IF(FINDMAGIC(67).GT.0)GOTO 66
2     I=DICE(1,18)
      GOTO(5,10,15,20,25,30,35,40,45,50,60,65,70,75,80,85,90,95)I

5     CALL FORMAT(3,'Nothing seems to happen.')
      RETURN

10    CALL FORMAT(3,'The Lady of the Lake appears and mistaking
     + you for a nymph,!/she gives you a magic item.')
      CALL LOCATEMAGIC(0)
      RETURN

15    I=DICE(1,2)
16    CALL FORMAT(3,'The water is growing ')
      CALL TRIMMER(TEMPERATURE(I))
      CALL FORMAT(2,'Do you wish to stay in the pool?   ')
      CALL INPUT(J,2)
      IF(J.EQ.78.OR.J.EQ.0)RETURN
      IF(J.NE.89)GOTO 16

      I=DICE(1,3)
      IF(I.EQ.1)THEN
        ADJTOAC=ADJTOAC-2
        CALL FORMAT(3,'This is water from the River Styxx.!/
     +Your armor class has permanently dropped 2.')
      ELSE IF(I.EQ.2)THEN
        CALL FORMAT(3,'You will be rewarded for your bravery.!/
     +You will be given a magic item you can use.')
        CALL LOCATEMAGIC(-1)
      ELSE
        I=DICE(3,9)
        CALL FORMAT(3,'It is water from the River Lethe.!/')
        CALL PRINT_DAMAGE(I)
        ENDIF
      RETURN

20    DUNGEON=DICE(1,6)
      DUNLVL=DICE(1,7)
      FLAG=1
      CALL FORMAT(3,'This is a pool of teleportation.!/
     +You have been teleported !!!!')
      CALL GETDUNGEON(DUNGEON,DUNLVL)
      XCOORD=DICE(1,LEVELLENGTH)
      YCOORD=DICE(1,LEVELWIDTH)
      RETURN

25    WISH=WISH+1
      CALL FORMAT(3,'This is a pool of wishes. You have gained a
     + wish for entering it.')
      RETURN

30    CALL FORMAT(3,'You have been rewarded for your courage. You
     +!/may choose to take a Ring of Resurrection or 2 Decks of Many
     +!/Things. Which do you choose (R or D) ?   ')
      CALL INPUT(I,0)
      IF(I.EQ.0)THEN
        RETURN
      ELSE IF(I.EQ.82)THEN
        CALL FORMAT(0,'The ring')
        CALL LOCATEMAGIC(25)
      ELSE IF(I.EQ.68)THEN
        CALL FORMAT(0,'The cards')
        CALL DECK
        CALL DECK
      ELSE
        GOTO 30
        ENDIF
      RETURN

35    I=DICE(1,12)
      HITPOINTS=HITPOINTS-I
      TOTALHITPOINTS=TOTALHITPOINTS-I
      IF(HITPOINTS.LT.1)LIFE=0
      CALL FORMAT(3,'The pool is filled with acid.!/You''ve lost ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' hit points.')
      RETURN

40    ADJSAVTHR=ADJSAVTHR-2
      CALL FORMAT(3,'This is a pool of curses. All saving throws
     + will be at -2.')
      RETURN

45    HITPOINTS=TOTALHITPOINTS
      CALL FORMAT(3,'The warm water overcomes you. When you wake up
     +, you!/feel refreshed but a little weak.')
      CALL CHANGESTATS(-DICE(1,2),0)
      RETURN

50    LIFE=0
      IF(FINDMAGIC(0).EQ.MAGICPERLEVEL(CHARLVL))THEN
        CALL FORMAT(3,'You are unable to swim. You sink to the bottom
     + and drown.')
      ELSE
        CALL FORMAT(3,'Are you kidding?? How can you possibly swim
     + with all!/of the things you''re carrying?')
        ENDIF
      RETURN


60    IF(GOLDONPERSON.EQ.0)GOTO 2
      I=DICE(1,GOLDONPERSON)
61    CALL FORMAT(3,'You''ve been captured by a water troll. He
     + has!/demanded ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' pieces of gold as ransom. Will you pay him
     +?  ')
      CALL INPUT(J,2)
      IF(J.EQ.78)THEN
        LIFE=0
        CALL FORMAT(3,'He has held you under water for a long,
     + long, time...')
      ELSE IF(J.EQ.89.OR.J.EQ.0)THEN
        GOLDONPERSON=GOLDONPERSON-I
      ELSE
        GOTO 61
        ENDIF
      RETURN

65    IF(FINDMAGIC(80).GT.0)THEN
        GOTO 5
      ELSE
66      DISEASE=DISEASE+1
        CALL FORMAT(3,'You''ve contracted lycanthropy from the
     + dirty water.!/You must be healed by the cleric in the city
     + or else you will!/soon die.')
        RETURN
        ENDIF

70    CALL FORMAT(3,'The water has affected your ability to fight.')
      IF(DICE(1,2).EQ.1)THEN
        ADJTOHIT=ADJTOHIT+1
        CALL FORMAT(3,'You will hit monsters MORE easily.')
      ELSE
        ADJTOHIT=ADJTOHIT-1
        CALL FORMAT(3,'You will hit monsters LESS easily.')
        ENDIF
      RETURN

75    BLINK=BLINK+30
      CALL FORMAT(3,'You have gained the ability to "Blink".!/
     +To blink, type "B".')
      RETURN

80    IF(CLASS.EQ.8.OR.CLASS.EQ.4)THEN
        SPELLS=SPELLS-IPICK(SPELLS,1,3)+999
        CALL FORMAT(3,'You have gained a temporary increase in
     + spell casting ability.')
      ELSE
        CALL FORMAT(3,'You have found a nice weapon and shield in
     + the pool.')
        CALL LOCATEMAGIC(10)
        CALL LOCATEMAGIC(22)
        ENDIF
      RETURN

85    CALL FORMAT(3,'You''ve found something in the pool.')
      CALL LOCATEMAGIC(0)
      RETURN

90    I=DICE(1,3)
      CALL FORMAT(3,'Pirhanna fish have dined on your toes. Your
     + dexterity!/and charisma have dropped ')
      CALL OUTNUM(I)
      CALL FORMAT(0,'.')
      CALL CHANGESTATS(-I,5)
      CALL CHANGESTATS(-I,6)
      RETURN

95    AGE=AGE+DICE(4,2)
      CALL FORMAT(3,'You have grown older.......')
      RETURN

      END



*****
*
*  FOUNTAIN contains the special room code "FOUNTAIN".
*
*****

      SUBROUTINE FOUNTAIN
      INCLUDE 'qstcom.inc'
      LOGICAL SAVINGTHROW

      PICTURE(6)(9:11)='FNT'
      CALL QPRINT
      I=DICE(1,11)
1     CALL FORMAT(3,'You have discovered a fountain shimmering with
     + a ')
      CALL TRIMMER(COLOR(I))
      CALL FORMAT(0,' liquid.!/Would you like to take a drink?  ')
      CALL INPUT(J,2)
      IF(J.EQ.78)RETURN
      IF(J.NE.89)GOTO 1

      I=DICE(1,12)
      GOTO(2,3,4,5,6,7,8,9,10,11,13,14) I

2     I=DICE(1,8)
      HITPOINTS=HITPOINTS-I
      TOTALHITPOINTS=TOTALHITPOINTS-I
      IF(HITPOINTS.LT.1)LIFE=0
      CALL FORMAT(3,'It was acid!!!! You have taken ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' permanent hit points of damage.')
      RETURN

3     I=DICE(1,6)
      HITPOINTS=HITPOINTS+I
      TOTALHITPOINTS=TOTALHITPOINTS+I
      CALL FORMAT(3,'You feel stronger!! It is a potion of
     + healthiness.!/You have gained ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' hit points.')
      RETURN

4     I=DICE(1,3)
      CALL FORMAT(3,'It is a potion of strength!! Your strength has
     + gone up ')
      CALL OUTNUM(I)
      CALL FORMAT(0,'.')
      CALL CHANGESTATS(I,1)
      RETURN

5     I=DICE(1,3)
      CALL FORMAT(3,'It is a potion of dexterity!! Your dexterity
     + has gone up ')
      CALL OUTNUM(I)
      CALL FORMAT(0,'.')
      CALL CHANGESTATS(I,5)
      RETURN

6     I=DICE(1,800)+200
      CALL FORMAT(3,'You have found a sparkling gem in the
     + fountain worth ')
      CALL OUTNUM(I)
      CALL FORMAT(0,' in gold.')
      CALL ADDGOLD(I)
      RETURN

7     I=DICE(1,4)
      CALL FORMAT(3,'Your feet have grown!! Your dexterity has
     + dropped ')
      CALL OUTNUM(I)
      CALL FORMAT(0,'.')
      CALL CHANGESTATS(-I,5)
      RETURN

8     I=DICE(1,6)
      CALL FORMAT(3,'It is a potion of enchantment!! Your charisma
     + went up ')
      CALL OUTNUM(I)
      CALL FORMAT(0,'.')
      CALL CHANGESTATS(I,6)
      RETURN

9     CALL FORMAT(3,'It is a potion of poison.')
      IF(FINDMAGIC(57).GT.0)THEN
12      CALL FORMAT(3,'Your stone protected you from the poison.')
        RETURN
        ENDIF
      IF(SAVINGTHROW(STATS(4)))THEN
        CALL FORMAT(3,'You realized it in time and did not drink.')
      ELSE
        LIFE=0
        CALL FORMAT(3,'You drank too much.......')
        ENDIF
      RETURN

10    CALL FORMAT(3,'Very potent poison........')
      IF(FINDMAGIC(57).GT.0)GOTO 12
      I=DICE(3,9)
      CALL PRINT_DAMAGE(I)
      RETURN

11    CALL FORMAT(3,'It is potion of gaseous form. Your armor class
     + has!/permanently dropped 1.')
      ADJTOAC=ADJTOAC-1
      RETURN

13    AGE=AGE+DICE(2,3)
      CALL FORMAT(3,'You have drank a potion of old age. You have aged.')
      RETURN

14    AGE=AGE-DICE(1,3)
      IF(AGE.LT.16)AGE=16
      CALL FORMAT(3,'It is a potion from the Fountain of Youth.!/
     +You have grown younger.')
      RETURN

      END




*****
*
*  WISH1 is the wish routine.
*
*****

      SUBROUTINE WISH1
      INCLUDE 'qstcom.inc'

1     CALL FORMAT(3,'Your wish is my command ("H" for Help) ?   ')
      CALL INPUT(I,0)

      IF(I.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +P - hitpoints!/
     +G - gold!/
     +E - experience!/
     +M - magic items!/
     +S - escape the dungeon!/
     +D - deck of many things!/
     +1 - strength!/
     +2 - intelligence!/
     +3 - wisdom!/
     +4 - constitution!/
     +5 - dexterity!/
     +6 - charisma!/
     +Q - save the wish for later')
        GOTO 1
      ELSE IF(I.GE.49.AND.I.LE.54)THEN
        CALL FORMAT(0,'Change a statistic')
        CALL CHANGESTATS(DICE(2,2),I-48)
      ELSE IF(I.EQ.80)THEN
        CALL FORMAT(0,'Hitpoints')
        I=DICE(1,12)+4
        HITPOINTS=HITPOINTS+I
        TOTALHITPOINTS=TOTALHITPOINTS+I
      ELSE IF(I.EQ.71)THEN
        CALL FORMAT(0,'Gold')
        I=DICE(1,30000)+15000
        CALL ADDGOLD(I)
      ELSE IF(I.EQ.69)THEN
        CALL FORMAT(0,'Experience')
        EXPERIENCE=EXPERIENCE+CHARLVL*5000
        CALL CHECKLEVEL
      ELSE IF(I.EQ.77)THEN
        CALL FORMAT(0,'Magic items')
        CALL LOCATEMAGIC(-1)
        CALL LOCATEMAGIC(-1)
        CALL LOCATEMAGIC(-1)
        CALL ARMOR
      ELSE IF(I.EQ.83)THEN
        CALL FORMAT(0,'Leave the dungeon')
        WISH=WISH-1
        CALL LEAVING
      ELSE IF(I.EQ.81.OR.I.EQ.0)THEN
        CALL FORMAT(0,'Save the wish')
        RETURN
      ELSE IF(I.EQ.68)THEN
        CALL FORMAT(0,'Deck of Many Things')
        CALL DECK
      ELSE
        GOTO 1
        ENDIF

      WISH=WISH-1
      AGE=AGE+2
      CALL FORMAT(4,'Your wish has been granted. You have aged 2
     + years.')
      RETURN
      END




*****
*
*  DRAGON contains the routines for encountering a dragon.
*
*****

      SUBROUTINE DRAGON
      INCLUDE 'qstcom.inc'
      INTEGER OLDCLASS(4),CHANCEISGONE,BACKSTAB

      DATA OLDCLASS/10,8,7,4/

      J=DUNGEON-2
      IF(IPICK(SOLVED,J,0).EQ.1)THEN
        PRINTCONTROL=0
        RETURN
        ENDIF
      PICTURE(6)(9:11)='###'
      CALL QPRINT
      BACKSTAB=0
      CHANCEISGONE=0
      MONNUM=115+J
      CALL RETRIEVEMONSTER(MONNUM)

1     CALL FORMAT(3,'Before you is a huge dragon. It is ')
      CALL TRIMMER(MONNAM)
      CALL FORMAT(0,' in color. It has seen you.!/What do you wish
     + to do ("H" for Help) ?   ')

      CALL INPUT(I,1)
      IF(I.EQ.72)THEN
        CALL FORMAT(0,'Help!/!/
     +F - fight!/
     +L - leave it alone!/
     +T - talk to it!/
     +X - current statistics!/
     +Z - personal statistics!/
     +I - inventory!/
     +C - cast a spell')
      ELSE IF(I.EQ.70.OR.I.EQ.55)THEN
        CALL FORMAT(0,'Fight!/')
        CHANCEISGONE=1
        GOTO 2
      ELSE IF(I.EQ.84)THEN
        CALL FORMAT(0,'Talk')
        IF(CHANCEISGONE.EQ.0.AND.DICE(1,3).GT.1)GOTO 5
        CALL FORMAT(3,'It opens it''s mouth to respond, and BOY does it
     + respond.....')
        L=1
        GOTO 7
      ELSE IF(I.EQ.76)THEN
        CALL FORMAT(0,'Leave it alone')
        CALL BLNK
        FLAG=1
        RETURN
      ELSE IF(I.EQ.67.OR.I.EQ.51)THEN
        CALL FORMAT(0,'Cast a spell')
        CALL CAST(K,1)
        MONHTPT=MONHTPT-K
        IF(MONHTPT.LT.1)GOTO 10
        L=0
        GOTO 7
        ENDIF
      GOTO 1

2     IF(DICE(1,3).EQ.1)THEN
        L=1
        GOTO 7
      ELSE
        CALL MFIGHT
        IF(LIFE.EQ.0)RETURN
        ENDIF
3     L=0
      CALL CFIGHT(K,MONYOUHIT,MONUNDEADMAGIC,BACKSTAB)
      MONHTPT=MONHTPT-K
      IF(MONHTPT.LT.1)GOTO 10
      GOTO 1

7     CALL FORMAT(3,'It is using it''s breath weapon.!/It is ')
      IF(MONNUM.EQ.116)THEN
        CALL FORMAT(0,'FIRE.!/')
      ELSE IF(MONNUM.EQ.117)THEN
        CALL FORMAT(0,'LIGHTNING.!/')
      ELSE IF(MONNUM.EQ.118)THEN
        CALL FORMAT(0,'POISON GAS.!/')
      ELSE
        CALL FORMAT(0,'FROST.!/')
        ENDIF
      CALL PRINT_DAMAGE(MONHTPT)
      IF(LIFE.EQ.0)RETURN
      IF(L.GT.0)GOTO 3
      GOTO 1

 10   CALL FORMAT(4,'You have killed a dragon !!!!!!')
      EXPERIENCE=EXPERIENCE+86000+DICE(1,86000)
      CALL CHECKLEVEL
      SOLVED=SOLVED+(10**(J-1))
      CALL QSLEEP(3)
      RETURN

5     CALL FORMAT(3,'You have approached the magnificent creature.
     + It speaks:!/!/Very wise. You have solved the mysteries of this
     + dungeon. Time to move on.!/For you, I will do the following:!/!/
     +Gain 20000 experience points!/
     +Be transported to the city!/
     +All statistics rise 1!/')
      HITPOINTS=TOTALHITPOINTS
      EXPERIENCE=EXPERIENCE+20000
      CALL CHANGESTATS(1,0)
      SOLVED=SOLVED+(10**(J-1))
      IF(OLDCLASS(J).NE.CLASS)THEN
        CALL FORMAT(0,'Change your class!/')
        CLASS=OLDCLASS(J)
        ENDIF
      CALL FORMAT(3,'With a wave of his claw the dragon makes the
     + changes, then,!/he magically exits the room.')
      CALL CHECKLEVEL
      CALL QSLEEP(7)
      CALL LEAVING
      END
