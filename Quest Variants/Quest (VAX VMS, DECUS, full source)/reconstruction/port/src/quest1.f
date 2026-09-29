      SUBROUTINE QUEST1
      INCLUDE 'qstcom.inc'
      EXTERNAL QUEST_ERROR

      SEED1=INT(VSECND(0.0))
      SEED1=IOR(SEED1,1)
      CALL USERINFO(UIC1,USERNAME1)
      IF(USERNAME1.NE.'00CKKELLEY')CALL GETTEMPCORE(PLAYER)

      I=0
      DO WHILE(I.NE.81)

      CALL CLEARSCREEN
1     CALL VMSTIM(TIM)
      CALL FORMAT(4,'The time of day is ')
      CALL FORMAT(0,TIM)
      CALL FORMAT(4,'Option ("H" for Help) ?   ')

      CALL INPUT1(I)
      IF(I.EQ.67)THEN
        CALL FORMAT(0,'Create a character')
        CALL CREATE
      ELSE IF(I.EQ.82)THEN
        CALL FORMAT(0,'Run a character')
        CALL RUNPLAYER
      ELSE IF(I.EQ.75)THEN
        CALL FORMAT(0,'Kill a character')
        CALL QKILL
      ELSE IF(I.EQ.80.OR.I.EQ.76)THEN
        CALL FORMAT(0,'List the players')
        CALL LISTPLAYERS
      ELSE IF(I.EQ.89)THEN
        CALL FORMAT(0,'List a user''s characters')
        CALL ONEPLAYER_LIST
      ELSE IF(I.EQ.70)THEN
        CALL FORMAT(0,'Find experience for the levels')
        CALL FINDEXPERIENCE
      ELSE IF(I.EQ.72.OR.I.EQ.0)THEN
        CALL FORMAT(0,'Help!/!/
     +C - create a character!/
     +R - run a character!/
     +Y - list 1 person''s characters!/
     +P - list current players!/
     +F - find experience for the various levels!/
     +S - sort character file!/
     +K - kill a character!/
     +N - rename a character!/
     +Q - quit the game!/
     +Z - logoff')
        GOTO 1
      ELSE IF(I.EQ.78)THEN
        CALL FORMAT(0,'Rename a character')
        CALL QRENAME
      ELSE IF(I.EQ.83)THEN
        CALL FORMAT(0,'Sort the character file')
        CALL SORT
      ELSE IF(I.EQ.90)THEN
        CALL FORMAT(0,'Logoff!/!/!/')
        CALL LOGOUT
      ELSE IF(I.EQ.79.AND.UIC1.EQ.'065244')THEN
        CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]DNDOP.EXE')
      ENDIF

      ENDDO

      CALL FORMAT(0,'Quit the game')
      CALL CLEARSCREEN
      CALL FORMAT(0,' QUEST normal termination.')
      CALL EXITR
      RETURN
      END



*****
*
*  CREATE handles the creation of new characters.
*
*****

      SUBROUTINE CREATE
      INCLUDE 'qstcom.inc'
      INTEGER ESC,K,B,A,C,D,BACK(6)

      CALL SCRINF(A,B,C,D)
      DO I=1,6
        BACK(I)=12
      ENDDO
      CALL CLEARSCREEN

      PLAYER=' '
      PLAYER(252:252)='@'
      ESC=27
      K=75

      RUN=1
      LIFE=1
      UIC=UIC1
      USERNAME=USERNAME1
      DUNGEON=0
      DUNLVL=0
      SPELLTOREGEN=0
      SPELLS=0
      CALL DATER(DAYS)
      EXPERIENCE=0
      GOLD=DICE(8,10)
      GOLDONPERSON=0
      DO I=1,8
        MAGIC(I)=0
        PROPERTIES(I)=0
      ENDDO
      CHARLVL=1
      XCOORD=0
      YCOORD=0
      HPREGEN=0
      DISEASE=0
      ADJTOAC=0
      PROTEVIL=0
      BLINK=0
      ADJHIT=0
      ADJSAVTHR=0
      WISH=0
      SPELLREGEN=0
      DELAY=2
      AGE=18
      MOVES=1
      SOLVED=0

      CALL OPENCHARFILE
      IF(USERNAME.NE.'00CKKELLEY')THEN
        CALL COUNTCHARACTER(USERNAME,I)
        IF(I.GT.3)THEN
          CALL FORMAT(4,'You already have ')
          CALL OUTNUM(I)
          CALL FORMAT(0,' player characters.!/Quest only allows users to
     + have 4 characters at one time.!/!/Hit any key to continue --> ')
          CALL INPUT1(I)
          RETURN
          ENDIF
        ENDIF

      CALL FORMAT(0,' Q - to quit,    K - to keep,
     +    R - to roll again,    B - recall last roll!/!/!/!/
     +!/ !_STR   INT   WIS   CON   DEX   CHR!/
     + !_---   ---   ---   ---   ---   ---!/')

4     DO I=1,6
        STATS(I)=DICE(3,5)+3
      ENDDO
      IF(DICE(1,6).EQ.1)STATS(5)=STATS(5)+1

      
7     CALL FORMAT(0,'!_')
      CALL VARFORMAT(STATS(1),3)
      DO I=2,6
        CALL VARFORMAT(STATS(I),6)
      ENDDO
      CALL FORMAT(0,'!_Q-K-R-B:  ')
      CALL SINGLE(27)
      IF(B.EQ.96)CALL SINGLE(ICHAR('['))
      CALL SINGLE(75)

      CALL INPUT1(I)
      IF(I.EQ.81)THEN
        CALL FORMAT(0,'Quit')
        RETURN
      ELSE IF(I.EQ.66)THEN
        CALL FORMAT(0,'Recall last roll')
        DO I=1,6
          STATS(I)=BACK(I)
        ENDDO
        CALL SINGLE(13)
        GOTO 7
      ELSE IF(I.NE.75)THEN
        CALL FORMAT(0,'Roll')
        DO I=1,6
          BACK(I)=STATS(I)
        ENDDO
        CALL SINGLE(13)
        GOTO 4
      ENDIF

      CALL FORMAT(0,'Keep!/')
10    CALL FORMAT(4,'Character class ("H" for Help) ?  ')

      CALL INPUT1(I)

      IF(I.EQ.72.OR.I.EQ.0)THEN
        CALL FORMAT(0,'Help!/!/
     +Character classes are:!/!/
     + F - fighter!/
     + C - cleric!/
     + T - thief!/
     + M - magic user')
        GOTO 10
      ELSE IF(I.EQ.70)THEN
        CALL FORMAT(0,'Fighter')
        HITPOINTS=10
        TOTALHITPOINTS=10
        ARMORCLASS=2
        CLASS=10
      ELSE IF(I.EQ.67)THEN
        CALL FORMAT(0,'Cleric')
        HITPOINTS=8
        TOTALHITPOINTS=8
        ARMORCLASS=5
        CLASS=8
        SPELLS=2
      ELSE IF(I.EQ.84)THEN
        CALL FORMAT(0,'Thief')
        HITPOINTS=7
        TOTALHITPOINTS=7
        ARMORCLASS=5
        CLASS=7
      ELSE IF(I.EQ.77)THEN
        CALL FORMAT(0,'Magician')
        HITPOINTS=6
        TOTALHITPOINTS=6
        ARMORCLASS=10
        CLASS=4
        MAGIC(1)=24
        SPELLS=2
      ELSE
        GOTO 10
      ENDIF
      
      IF(STATS(5).GT.14)THEN
        ARMORCLASS=ARMORCLASS+14-STATS(5)
      ELSE IF(STATS(5).LT.9)THEN
        ARMORCLASS=ARMORCLASS+9-STATS(5)
        ENDIF

      CALL NUMERICTOASCII
15    CALL FORMAT(3,'Enter a name for your character: ')
      CALL ASCII(NAME,0)
      DO I=1,15
        IF(NAME(1:1).NE.' ')GOTO 99
        NAME=NAME(2: )
      ENDDO

99    PLAYER(1:15)=NAME
      CALL PUTPLAYER(ERR)
      IF(ERR.NE.0)THEN
        CALL FORMAT(4,'That player already exists. Please try
     + another name.!/')
        GOTO 15
        ENDIF

16    CALL FORMAT(4,'Enter a secret name for your character: ')
      CALL GETINPUT_NOECHO(SECRETNAME)
      PLAYER(16:25)=SECRETNAME
      CALL REPLACEPLAYER(ERR,NAME)
      IF(ERR.NE.0)THEN
        CALL FORMAT(2,'%QSTOTS - unknown CREATE error.')
        CALL EXITR
        ENDIF
      CALL CLOSEFILE(21)
      CALL PUTTEMPCORE(PLAYER)
      CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST2.Q7R')

      END
      
      
*****
*
*  RUN checks username etc. to see if a player may run his character.
*
*****

      SUBROUTINE RUNPLAYER
      INCLUDE 'qstcom.inc'
      LOGICAL CHECKSECRET

      CALL CLEARSCREEN

      CALL FORMAT(4,'Enter the name of the character to run this
     + adventure: ')
      CALL ASCII(NAME,0)
      CALL OPENCHARFILE
      CALL GETPLAYER(ERR,NAME)
      IF(ERR.NE.0)THEN
        CALL CLOSEFILE(21)
        CALL FORMAT(4,'That character was not found.')
        CALL QSLEEP(1)
        RETURN
        ENDIF
      CALL ASCIITONUMERIC
      IF(UIC1.NE.'065244')THEN
        IF(RUN.NE.0)THEN
          CALL CLOSEFILE(21)
          CALL FORMAT(5,'That character is already adventuring!!!!')
          CALL QSLEEP(1)
          RETURN
          ENDIF
        ENDIF
      CALL FORMAT(4,'Jim: Welcome to Quest, ')
      CALL FORMAT(0,NAME)
      IF(UIC1.NE.'065244')THEN
        IF(UIC.NE.UIC1)THEN
          IF(.NOT.CHECKSECRET(SECRETNAME,KEY1))THEN
            CALL CLOSEFILE(21)
            RETURN
            ENDIF
            ENDIF
        ENDIF
      
      RUN=1
      CALL DATER(DAYS)
      CALL NUMERICTOASCII
      CALL REPLACEPLAYER(ERR,PLAYER(1:15))
      CALL CLOSEFILE(21)
      CALL PUTTEMPCORE(PLAYER)
      IF(DUNGEON.EQ.0)THEN
        CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST2.Q7R')
      ELSE
        CALL CHAIN('BSU$USER_2:[00CKKELLE.QUEST]QUEST3.Q7R')
        ENDIF
      END


*****
*
*  RENAME allows players to rename living characters.
*
*****

      SUBROUTINE QRENAME
      INCLUDE 'qstcom.inc'
      CHARACTER TEMP_NAME*15
      LOGICAL CHECKSECRET

      CALL CLEARSCREEN
      CALL FORMAT(5,'Enter name of character to be renamed: ')
      CALL ASCII(NAME,0)
      CALL OPENCHARFILE
      CALL GETPLAYER(ERR,NAME)
      IF(ERR.NE.0)THEN
        CALL CLOSEFILE(21)
        CALL FORMAT(5,'That character NOT found.')
        CALL QSLEEP(1)
        RETURN
        ENDIF
      CALL ASCIITONUMERIC
      IF(UIC1.NE.'065244')THEN
        IF(UIC.NE.UIC1)THEN
          IF(.NOT.CHECKSECRET(SECRETNAME,KEY1))THEN
            CALL CLOSEFILE(21)
            RETURN
            ENDIF
            ENDIF
        ENDIF

      TEMP_NAME=NAME
1     CALL FORMAT(4,'Enter new name: ')
      CALL ASCII(NAME,0)
      CALL FORMAT(3,'Enter new secret name: ')
      CALL GETINPUT_NOECHO(SECRETNAME)
      CALL NUMERICTOASCII
      CALL PUTPLAYER(ERR)
      IF(ERR.NE.0)THEN
        CALL FORMAT(4,'That player already exists. Try another name.')
        GOTO 1
      ELSE
        CALL KILLPLAYER(ERR,TEMP_NAME)
        ENDIF
      CALL CLOSEFILE(21)
      RETURN
      END



*****
*
*  CHECKSECRET checks to see if the user knows a character's secret name.
*
*****

      LOGICAL FUNCTION CHECKSECRET(SECRETNAME,KEY1)
      CHARACTER SECRETNAME*10,KEY1*15,L*10

      CHECKSECRET=.TRUE.
      CALL FORMAT(4,'Enter character''s secret name: ')
      CALL GETINPUT_NOECHO(L)
      IF(L.EQ.SECRETNAME)RETURN
      CHECKSECRET=.FALSE.
      CALL FORMAT(2,'That name is NOT correct!!')
      CALL QSLEEP(1)
      RETURN
      END



*****
*
*  KILL allows the user to kill unwanted living characters.
*
*****

      SUBROUTINE QKILL
      INCLUDE 'qstcom.inc'
      LOGICAL CHECKSECRET

      CALL CLEARSCREEN
      CALL FORMAT(5,'Name of the character you wish to kill: ')
      CALL ASCII(NAME,0)
      CALL OPENCHARFILE
      CALL GETPLAYER(ERR,NAME)
      IF(ERR.NE.0)THEN
        CALL CLOSEFILE(21)
        CALL FORMAT(4,'That character NOT found.')
        CALL QSLEEP(1)
        RETURN
        ENDIF
      CALL ASCIITONUMERIC
      IF(UIC1.NE.'065244')THEN
        IF(UIC.NE.UIC1)THEN
          IF(.NOT.CHECKSECRET(SECRETNAME,KEY1))THEN
            CALL CLOSEFILE(21)
            RETURN
            ENDIF
            ENDIF
        IF(RUN.EQ.1)THEN
          CALL FORMAT(5,'That player is currently adventuring.')
          IF(UIC.NE.UIC1)THEN
            CALL CLOSEFILE(21)
            CALL QSLEEP(2)
            RETURN
          ELSE
            CALL FORMAT(3,'Do you still wish to kill it?   ')
            CALL INPUT1(I)
            IF(I.EQ.89)THEN
       CALL FORMAT(0,'Yes')
            ELSE
       CALL FORMAT(0,'No')
       RETURN
       ENDIF
          ENDIF
        ENDIF
      ENDIF
      CALL KILLPLAYER(ERR,NAME)
      CALL CLOSEFILE(21)
      IF(ERR.EQ.0)THEN
        CALL FORMAT(8,'Player has died a sorry death.....')
        CALL QSLEEP(1)
        RETURN
      ELSE
        CALL FORMAT(4,'%QSTOTS - death error.')
        CALL QSLEEP(2)
        ENDIF
      RETURN
      END



*****
*
*  LISTPLAYERS list all living characters to the screen.
*
*****

      SUBROUTINE LISTPLAYERS
      INCLUDE 'qstcom.inc'
      CHARACTER CLASS1*1,QREC*78
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      CALL KOPEN
      KEYOPN=.TRUE.
      CALL KSEQ_REWIND
      CALL CLEARSCREEN
      CALL FORMAT(0,'Character list:!/!/
     +Character         Class   STR INT WIS CON DEX CHR   Lvl
     +    Expr.      Owner   !/---------------   -----   --- --- ---
     + --- --- ---   ---    -----  ------------')

      CALL SETSCROLL(5,24)
      DO WHILE(.TRUE.)

5       CALL KREAD_NEXT(PLAYER,IOS)
        IF(IOS.EQ.FOR$IOS_SPERECLOC)THEN
          GOTO 7
        ELSE IF(IOS.NE.0)THEN
          GOTO 2
          ENDIF
        CALL ASCIITONUMERIC
        IF(CLASS.EQ.4)THEN
          CLASS1='M'
        ELSE IF(CLASS.EQ.7)THEN
          CLASS1='T'
        ELSE IF(CLASS.EQ.8)THEN
          CLASS1='C'
        ELSE IF(CLASS.EQ.10)THEN
          CLASS1='F'
        ELSE
          CLASS1='U'
          ENDIF
      WRITE(QREC,1) NAME,CLASS1,(STATS(I),I=1,6),CHARLVL,EXPERIENCE,
     +USERNAME
1     FORMAT(A15,5X,A1,5X,6(I3,1X),2X,I3,2X,I7,2X,A12)
      CALL TTYREC(ICHAR(' '),QREC)

7     ENDDO

2     CALL CLOSEFILE(21)
      CALL SETSCROLL(1,24)
      CALL FORMAT(4,'Hit any key to continue:  ')
      CALL INPUT1(I)
      RETURN
      END


*****
*
*  ONEPLAYER_LIST allows the user to list characters created under one username.
*
*****

      SUBROUTINE ONEPLAYER_LIST
      INCLUDE 'qstcom.inc'

      CALL CLEARSCREEN
1     CALL FORMAT(5,'Type the USERNAME of the person whose characters
     + you wish to list,!/or hit RETURN to list YOUR characters: ')
      CALL ASCII(USERNAME,1)

      IF(USERNAME.EQ.' ')USERNAME=USERNAME1
      CALL LISTPLAYERS_YOURS(USERNAME)
      RETURN
      END



*****
*
*  LISTPLAYERS_YOURS allows the player to list a characters for a specified
*  Username.
*
*****

      SUBROUTINE LISTPLAYERS_YOURS(USER)
      INCLUDE 'qstcom.inc'
      CHARACTER CLASS1*1,USER*12,QREC*78
      LOGICAL KEYOPN
      COMMON/QKEYED/KEYOPN

      CALL KOPEN
      KEYOPN=.TRUE.

      CALL CLEARSCREEN
      CALL FORMAT(0,'Character list:!/!/
     +Character         Class   STR INT WIS CON DEX CHR   Lvl
     +    Expr.      Owner   !/---------------   -----   --- --- ---
     + --- --- ---   ---    -----  ------------')

      CALL SETSCROLL(5,24)

5       CALL KREAD_KEY(PLAYER,USER,1,0,IOS)
        IF(IOS.EQ.FOR$IOS_SPERECLOC)THEN
          GOTO 7
        ELSE IF(IOS.NE.0)THEN
          GOTO 2
          ENDIF

      DO WHILE(IOS.EQ.0)
        CALL ASCIITONUMERIC
        IF(CLASS.EQ.4)THEN
          CLASS1='M'
        ELSE IF(CLASS.EQ.7)THEN
          CLASS1='T'
        ELSE IF(CLASS.EQ.8)THEN
          CLASS1='C'
        ELSE IF(CLASS.EQ.10)THEN
          CLASS1='F'
        ELSE
          CLASS1='U'
          ENDIF
      WRITE(QREC,1) NAME,CLASS1,(STATS(I),I=1,6),CHARLVL,EXPERIENCE,
     +USERNAME
1     FORMAT(A15,5X,A1,5X,6(I3,1X),2X,I3,2X,I7,2X,A12)
      CALL TTYREC(ICHAR(' '),QREC)

7     CALL KREAD_NEXT(PLAYER,IOS)
      IF(IOS.LT.0)GOTO 2
      IF(USER.NE.PLAYER(26:37))IOS=1
      ENDDO

2     CALL CLOSEFILE(21)
      CALL SETSCROLL(1,24)
      CALL FORMAT(4,'Hit any key to continue:  ')
      CALL INPUT1(I)
      RETURN
      END




*****
*
*  SORT sorts the character file, deleting characters not run for 21
*  days, and characters with no experience not run for 3 days.
*
*****

      SUBROUTINE SORT
      INCLUDE 'qstcom.inc'
      CHARACTER NAME1(400)*15,USERNAME2(400)*12,TEMP2*15,TEMP3*12
      CHARACTER CLASS1(400)*1,TEMP4*1
      INTEGER LEVEL(400),TEMP1,EXP(400),DATT,J
      CHARACTER QREC*61

      T=0
      CALL DATER(DATT)
      CALL OPENCHARFILE
17    CALL KREAD_KEY(PLAYER,'               ',0,1,IOS)
      IF(IOS.EQ.FOR$IOS_SPERECLOC)THEN
        GOTO 17
      ELSE IF(IOS.NE.0)THEN
        GOTO 10
        ENDIF

1     CALL ASCIITONUMERIC
      IF(DATT-DAYS.GT.10.AND.RUN.EQ.1)THEN
        CALL KDELETE(IOS)
        GOTO 7
*   RUN=0
*   CALL NUMERICTOASCII
*   CALL REPLACEPLAYER(ERR,NAME)
        ENDIF
      IF(EXPERIENCE.NE.0)THEN
        TEMP1=DATT-DAYS
        IF(TEMP1.GT.21)THEN
          CALL KDELETE(IOS)
          GOTO 7
          ENDIF
        ELSE
          IF(RUN.EQ.0)THEN
            CALL KDELETE(IOS)
            GOTO 7
            ENDIF
            ENDIF
      T=T+1
      NAME1(T)=NAME
      USERNAME2(T)=USERNAME
      LEVEL(T)=CHARLVL
      EXP(T)=EXPERIENCE
      IF(CLASS.EQ.10)THEN
        CLASS1(T)='F'
      ELSE IF(CLASS.EQ.7)THEN
        CLASS1(T)='T'
      ELSE IF(CLASS.EQ.8)THEN
        CLASS1(T)='C'
      ELSE IF(CLASS.EQ.4)THEN
        CLASS1(T)='M'
      ELSE
        CLASS1(T)='U'
        ENDIF
7     CALL KREAD_NEXT(PLAYER,IOS)
      IF(IOS.EQ.FOR$IOS_SPERECLOC)THEN
        GOTO 7
      ELSE IF(IOS.NE.0)THEN
        GOTO 10
        ENDIF
      GOTO 1

10    CALL CLOSEFILE(21)

      IF(T.GT.1)THEN
        DO J=1,T-1
          Z=J
            DO I=J+1,T
       IF(EXP(Z).LT.EXP(I))Z=I
            ENDDO
          TEMP2=NAME1(J)
          NAME1(J)=NAME1(Z)
          NAME1(Z)=TEMP2
          TEMP3=USERNAME2(J)
          USERNAME2(J)=USERNAME2(Z)
          USERNAME2(Z)=TEMP3
          TEMP1=LEVEL(J)
          LEVEL(J)=LEVEL(Z)
          LEVEL(Z)=TEMP1
          TEMP1=EXP(J)
          EXP(J)=EXP(Z)
          EXP(Z)=TEMP1
          TEMP4=CLASS1(J)
          CLASS1(J)=CLASS1(Z)
          CLASS1(Z)=TEMP4
        ENDDO
        ENDIF
      CALL CLEARSCREEN
      CALL FORMAT(0,'###  Character        Experience  Level
     +     Owner         Class!/
     +---  ---------------  ----------  -----  -----------      -----')
      CALL SETSCROLL(3,24)
      DO I=1,T
        WRITE(QREC,12) I,NAME1(I),EXP(I),LEVEL(I),USERNAME2(I),
     +CLASS1(I)
12      FORMAT(I3,2X,A15,5X,I7,5X,I2,2X,A12,7X,A1)
        CALL TTYREC(ICHAR(' '),QREC)
      ENDDO
      CALL FORMAT(4,'Hit any key to continue: ')
      CALL INPUT1(I)
      CALL SETSCROLL(1,24)
      RETURN
      END





*****
*
*  FINDEXPERIENCE shows the user how many experience points it takes to reach
*  certain levels.
*
*****

      SUBROUTINE FINDEXPERIENCE
      INCLUDE 'qstcom.inc'

1     CALL CLEARSCREEN
2     CALL FORMAT(4,'Character class (F,C,T,M - "Q" to Quit) ?  ')
      CALL INPUT1(I)
      J=0

      IF(I.EQ.81.OR.I.EQ.0)THEN
        CALL FORMAT(0,'Quit')
        RETURN
      ELSE IF(I.EQ.70)THEN
        CALL FORMAT(0,'Fighter')
        J=10
      ELSE IF(I.EQ.67)THEN
        CALL FORMAT(0,'Cleric')
        J=8
      ELSE IF(I.EQ.77)THEN
        CALL FORMAT(0,'Magic user')
        J=4
      ELSE IF(I.EQ.84)THEN
        CALL FORMAT(0,'Thief')
        J=7
      ELSE
        GOTO 1
        ENDIF

      CALL FORMAT(3,'Level to see (0 for all, else 1-20) ?  ')
      CALL INPUTNUMBER(I)

      CALL TTYNL(3)
      IF(I.LT.0.OR.I.GT.20)RETURN
      IF(I.NE.0)THEN
        K=IEXPER(J,I)
        CALL FORMAT(2,'It takes ')
        CALL VARFORMAT(K,8)
        CALL FORMAT(0,' experience points to reach level ')
        CALL VARFORMAT(I,3)
      ELSE
        DO K=1,20
          I=K
          N=IEXPER(J,I)
          CALL FORMAT(2,'It takes ')
          CALL VARFORMAT(N,8)
          CALL FORMAT(0,' experience points to reach level ')
          CALL VARFORMAT(I,3)
        ENDDO
        ENDIF

      GOTO 2
      END
