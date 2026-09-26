       LOGICAL FUNCTION WIZARD ()

C  Ask if he's a wizard.  If he says yes, ask for the magic word.
C  Return TRUE if he gives the correct word.
C
C  Simplified from the original: the Univac version, after this same
C  magic-word check, went on to run a randomized Fieldata challenge/
C  response cipher using the FFDASC/FASCFD system routines, plus a
C  PRIVIL account-number bypass for ISD staff -- all tied to Univac
C  1100 EXEC 8 infrastructure (and undocumented system routines) that
C  no longer exists.  This port keeps the magic-word gate and drops
C  the cipher/account check, exactly as was done for the sibling
C  Adventure port's wizard-mode login gate.

       IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'
       LOGICAL YESM
      character*5  wd1, wd2
      character*12 wd1x, wd2x

       WIZARD=YESM(16,0,7)
       IF(.NOT.WIZARD)RETURN

       CALL MSPEAK(17)
       CALL GETIN (WD1, WD1X, WD2, WD2X)
      WIZARD = WD1 .EQ. MAGIC

       IF (WIZARD) THEN
         CALL MSPEAK(19)
       ELSE
         CALL MSPEAK(20)
       END IF
       RETURN
       END


       SUBROUTINE MAINT

C  SOMEONE SAID THE MAGIC WORD TO INVOKE MAINTENANCE MODE.  MAKE SURE HE'S A
C  WIZARD.  IF SO, LET HIM TWEAK ALL SORTS OF RANDOM THINGS, THEN EXIT SO CAN
C  SAVE TWEAKED VERSION.  SINCE MAGIC WORD MUST BE FIRST COMMAND GIVEN, ONLY
C  THING WHICH NEEDS TO BE FIXED UP IS ABB(1).

      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comabb.fi'
      include 'comblk.fi'
      include 'comwiz.fi'
      logical yesm,wizard
      character*5 word1
      character*8 clsmsg

       IF(.NOT.WIZARD())RETURN
       BLKLIN=.FALSE.
       IF (.not.(YESM(10,0,0))) goto 60
       do 50 clsnum=1,2
       clsmsg = 'paying'
       if (clsnum.eq.2) clsmsg = 'in-house'
40     format (/,' Hours for ',A8,' adventurers:')
       print 40, clsmsg
       call hours(clsnum)
50     continue
       print 1
60     IF(YESM(11,0,0))CALL NEWHRS
       IF(.NOT.YESM(26,0,0))GOTO 10
       CALL MSPEAK(27)
C  A bad (non-numeric) answer here used to abort the whole process
C  with a libgfortran list-input error; treat it as the prompt's own
C  "leave it alone" answer instead.
       READ (5,*,IOSTAT=IOS) HBEGIN
       IF (IOS.NE.0) HBEGIN=0
1      FORMAT()
       CALL MSPEAK(28)
       READ (5,*,IOSTAT=IOS) HEND
       IF (IOS.NE.0) HEND=0
       CALL DATIME(D,T)
       HBEGIN=HBEGIN+D
       HEND=HBEGIN+HEND-1
       CALL MSPEAK(29)
       READ (5,2) HNAME
2      FORMAT(4A5)
10     PRINT 12,SHORT
12     FORMAT(' Length of short game (zero to leave at',I3,'):')
       READ (5,*,IOSTAT=IOS) X
       IF (IOS.NE.0) X=0
       IF(X.GT.0)SHORT=X
       CALL MSPEAK(12)
       read (5,2) word1
       IF (word1.NE.'     ') MAGIC=word1
       CALL MSPEAK(13)
       READ (5,*,IOSTAT=IOS) X
       IF (IOS.NE.0) X=0
       IF(X.GT.0)MAGNM=X
       PRINT 16,LATNCY
16     FORMAT(' Latency for restart (zero to leave at',I3,'):')
       READ (5,*,IOSTAT=IOS) X
       IF (IOS.NE.0) X=0
       IF(X.GT.0.AND.X.LT.45)CALL MSPEAK(30)
       IF(X.GT.0)LATNCY=MAX0(45,X)
       IF(YESM(14,0,0))CALL MOTD(.TRUE.)
      check = -1
       SAVED=0
       SETUP=2
       ABB(1)=0
       CALL MSPEAK(15)
       BLKLIN=.TRUE.
       CALL CIAO
       END
