       SUBROUTINE HOURS(class)

C  Announce the current hours when the cave is open for adventuring.
C  This info is stored in WKDAY, WKEND, and HOLID, where bit SHIFT(1,N)
C  is on if the hour from N:00 to N:59 is "prime time" (cave closed).
C  WKDAY is for weekdays, WKEND for weekends, and HOLID for holidays.
C  The next holiday is from HBEGIN to HEND, and its name is HNAME.

C  Two types (classes) of players are accomodated.  Paying customers
C  (class 1) normally have freer access to the cave than in-house
C  (class 2) users, since they presumably pay for the privilege of
C  adventuring.

C  This routine is passed one argument, a "class" number indicating
C  what type of adventurer we've got here.  Users are not told which
C  class they are in, or even that there is another class.

       IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'
      character*5 day

       PRINT 1
1      FORMAT ()
       CALL HOURSX(WKDAY(class),'Mon -',' Fri:')
       CALL HOURSX(WKEND(class),'Sat -',' Sun:')
       CALL HOURSX(HOLID(class),'Holid','ays: ')
       CALL DATIME(D,T)
       IF(HEND.LT.D.OR.HEND.LT.HBEGIN)RETURN
       IF(HBEGIN.GT.D)GOTO 10
       PRINT 5, HNAME
5      FORMAT (/' Today is a holiday, namely ',4A5)
       goto 20

10     D=HBEGIN-D
       day='days,'
       IF (D.EQ.1) day='day, '
       PRINT 15, D,DAY,HNAME
15     FORMAT (/' The next holiday will be in',I3,' ',A5,' namely ',4A5)
20     RETURN
       END


       SUBROUTINE HOURSX(H,DAY1,DAY2)

C  Subroutine used by HOURS (above) to print cave hours for weekdays,
C  weekends, or holidays.

       IMPLICIT INTEGER(A-Z)
       LOGICAL FIRST
       character*5 day1,day2

       FIRST=.TRUE.
       FROM=-1
       IF (H.NE.0) GOTO 110
       PRINT 102, DAY1,DAY2
102    FORMAT (10X,2A5,'   Open all day')
       goto 130

110    FROM=FROM+1
       IF (IAND(H,SHIFT(1,FROM)).NE.0) GOTO 110
       IF (FROM.GE.24) GOTO 120
       TILL=FROM
114    TILL=TILL+1
       IF (IAND(H,SHIFT(1,TILL)).EQ.0.AND.TILL.NE.24) GOTO 114
       IF (FIRST) PRINT 116, DAY1,DAY2,FROM,TILL
       IF (.NOT.FIRST) PRINT 118, FROM,TILL
116    FORMAT (10X,2A5,I4,':00 to',I3,':00')
118    FORMAT (20X,I4,':00 to',I3,':00')
       FIRST=.FALSE.
       FROM=TILL
       GOTO 110

120    IF (FIRST) PRINT 122, DAY1,DAY2
122    FORMAT (10X,2A5,'  Closed all day')
130    RETURN

       END



       SUBROUTINE NEWHRS

C  Set up new hours for the cave.  Specified as inverse--i.e., when is it
C  closed due to prime time?  See HOURS (above) for desc of variables.

       IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'
      character*8 clsmsg

       CALL MSPEAK(21)
       do 20 clsnum=1,2
       clsmsg = 'paying'
       if (clsnum.eq.2) clsmsg = 'in-house'
10     format (/,' New hours for ',A8,' adventurers:')
       print 10, clsmsg
       WKDAY(clsnum)=NEWHRX('Weekd','ays: ')
       WKEND(clsnum)=NEWHRX('Weeke','nds: ')
       HOLID(clsnum)=NEWHRX('Holid','ays: ')
       CALL MSPEAK(22)
       call hours(clsnum)
20     continue
       RETURN
       END


       INTEGER FUNCTION NEWHRX(DAY1,DAY2)
       IMPLICIT INTEGER(A-Z)
       character*5 day1,day2

C  Input prime time specs and set up a word of internal format.

       NEWHRX=0
       PRINT 110, DAY1,DAY2
110    FORMAT (' Prime time on ',2A5)
120    PRINT 130
130    FORMAT (' From:')
C  A bad (non-numeric) answer here used to abort the whole process
C  with a libgfortran list-input error; treat it as the prompt's own
C  "leave it alone" answer instead.
       READ (5,*,IOSTAT=IOS) FROM
       IF (IOS.NE.0) FROM=-1
       IF (FROM.LT.0.OR.FROM.GE.24) goto 199
       PRINT 150
150    FORMAT (' Till:')
       READ (5,*,IOSTAT=IOS) TILL
       IF (IOS.NE.0) TILL=-1
       TILL=TILL-1
       IF (TILL.LT.FROM.OR.TILL.GE.24) goto 199
       DO 160 I=FROM,TILL
160    NEWHRX=IOR(NEWHRX,SHIFT(1,I))
       GOTO 120
199    return

       END
