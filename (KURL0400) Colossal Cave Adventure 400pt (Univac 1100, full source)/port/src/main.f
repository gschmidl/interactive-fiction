
C  Adventure -- main program

C  This version customized for Univac 1100 series ASCII FORTRAN
C  by Duff Kurland
C     Information Systems Design
C     3205 Coronado Drive
C     Santa Clara, CA  95051
C     (408) 727-8100

C  December 19, 1978

C  Major reconstruction in a few areas of the cave, using database
C  modifications supplied by Bob Elman, Four Phase Systems.

C  December 27, 1979

C  (Original had no IMPLICIT statement here; the whole Univac codebase
C  was compiled under a site-wide compiler option that defaulted every
C  letter to INTEGER, unlike the ANSI I-N default.  Made explicit here.)
      IMPLICIT INTEGER(A-Z)

C  RAN must be declared EXTERNAL: gfortran (like most compilers with
C  GNU extensions enabled) supplies its own RAN as an intrinsic - a
C  REAL generator whose argument is a seed - so without this every
C  call below silently binds to that instead of the game's own
C  INTEGER FUNCTION RAN(IRANGE) in ran.f, and returns the same tiny
C  REAL value forever.
      EXTERNAL RAN
      INTEGER RAN

      include 'params.fi'
      include 'comabb.fi'
      include 'comblk.fi'
      include 'commtx.fi'
      include 'compla.fi'
      include 'comptx.fi'
      include 'comtxt.fi'
      include 'comvoc.fi'
      include 'comwiz.fi'

C     (All the state below used to be plain locals of this main
C     program; they now live in GAMECOM so the SUSPEND/RESTORE
C     replacement routines in savefile.f can reach them.  See
C     gamecom.fi for why.)
      include 'gamecom.fi'
      character*4  ckk
      equivalence (kk, ckk)

C  BLKLIN is true if SPEAK should skip a line before printing
C  WZDARK says whether the locatin he's leaving was dark
C  LMWARN says whether he's been warned about lamp going dim
C  CLOSNG says whether it's closing time yet
C  PANIC says whether he's found out he's trapped in the cave
C  CLOSED says whether we're all the way closed
C  GAVEUP says whether he exited via QUIT
C  SCORNG tells to the SCORE routine if we're doing a SCORE command
C  DEMO is true if this is a prime-time demonstration game
C  YEA is random yes/no reply
C  YES is an external function, referenced as YES(X,Y,Z), where X is
C      the number of a (Section 6) question to ask, Y is the number of
C      the message to display upon a "yes" response (0 if none), and
C      Z is the message to display upon a "no" response (0 if none).
C      Returns a value of TRUE if the user did answer "yes". and
C      repeats the question until a reasonable response is given.
C  START is an external function which returns a TRUE value if
C      this is a demo game.

      logical yes,start
      CHARACTER*80 UPPERC

C  (As of FTN level 9R1, function types must be declared or implied).

      logical TOTING, HERE, AT, BITSET, FORCED, DARK, PCT

      DATA SETUP/0/,BLKLIN/.TRUE./

C  Initiallize the various text-pointer arrays to 0.  All text is stored
C  in array LINES: Each line is preceded by a word pointing to the next
C  pointer (i.e., the word following the end of the line).  The pointer
C  is negative for the first line of a message.  The text-pointer
C  arrays contain indices of pointer-words in LINES.  STEXT(N)
C  is short description of location N.  LTEST(N) is long description.
C  PTEXT(N) points to message for PROP(N).  Successive PROP messages
C  are found by chasing pointers.  RTEXT contains Section 6's stuff.
C  CTEXT(N) points to a proficiency message.  MTEXT is for Section 12.
C  We also clear COND.  See description of Section 9 for details.

      data stext/locsiz*0/, ltext/locsiz*0/, cond/locsiz*0/
      data ptext/objsiz*0/, rtext/rtxsiz*0/, mtext/magsiz*0/
      data ctext/levsiz*0/, rmsgx/rtxsiz*.false./, star/'*'/

C  Statement functions

C  TOTING(OBJ) = true if object OBJ is being carried
C  HERE(OBJ)   = true if object OBJ is at LOC (or is being carried)
C  AT(OBJ)     = true is on either side of two-placed object
C  LIQ()         = object number of liquid in bottle
C  LIQLOC(LOC) = object number of liquid (if any) at location LOC
C  BITSET(L,N) = true if COND(L) has bit N set (bit 0 is units bit)
C  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
C  DARK()        = true if locatin LOC is dark
C  PCT(N)      = true N% of the time (N is integer from 0 to 100)

      TOTING(OBJ)=PLACE(OBJ).EQ.-1
      HERE(OBJ)=PLACE(OBJ).EQ.LOC.OR.TOTING(OBJ)
      AT(OBJ)=PLACE(OBJ).EQ.LOC.OR.FIXED(OBJ).EQ.LOC
      LIQ2(PBOTL)=(1-PBOTL)*WATER+(PBOTL/2)*(WATER+OIL)
      LIQLOC(LOC)=LIQ2((MOD(COND(LOC)/2*2,8)-5)*MOD(COND(LOC)/4,2
     *)+1)
      BITSET(L,N)=IAND(COND(L),SHIFT(1,N)).NE.0
      FORCED(LOC)=COND(LOC).EQ.2
C     LIQ() and DARK() moved out to liqdark.f as real external functions
C     (call sites now say LIQ()/DARK()): a Fortran statement function
C     needs at least an empty pair of dummy-argument parens, and the
C     original Univac source defined these with none at all, which
C     only worked because the whole file was compiled with implicit
C     all-integer defaults papering over the ambiguity.
      PCT(N)=RAN(100).LT.N


C  Description of the database format

C  The data file contains several sections.  Each begins with a line containing
C  a number identifying the section, and ends with a line containing  -1 .

C  Section 1: Long form descriptions.  Each line contains a location number,
C      and a line of text.  The set of (necessarily adjacent) lines
C      whose numbers are X form the long description of location X.

C  Section 2: Short form descriptions. Same format as long form.  Not all
C      places have short descriptions.

C   Section 3: Travel table.  Each line contains a location number (X), a second
C       location number (Y), and a list of motion numbers (see Section 4).
C       Each motion represents a verb which will go to Y if currently at X.
C       Y, in turn, is interpreted as follows.  Let M=Y/1000, N=Y mod 1000.

C               If N<=300       it is the location to go to.
C               If 300<N<=500   N-300 is used in a computed GOTO to
C                                       a section of special code.
C               If N>500        message N-500 from Section 6 is printed,
C                                       and he stays wherever he is.

C       Meanwhile, M specifies the conditions on the motion.

C               If M=0          it's unconditional.
C               If 0<M<100      it is done with M% probability.
C               If M=100        unconditional, but forbidden to dwarves.
C               If 100<M<=200   he must be carrying object M-100.
C               If 200<M<=300   must be carrying or in same room as M-200.
C               If 300<M<=400   PROP(M mod OBJSIZ) must NOT be 0.
C               If 400<M<=500   PROP(M mod OBJSIZ) must NOT be 1.
C               If 500<M<=600   PROP(M mod OBJSIZ) must NOT be 2, etc.

C       If the condition (if any) is not met, then the next DIFFERENT
C       destination value is used (unless it fails to meet ITS condition,
C       in which case the next is found, etc.).  Typically, the next dest will
C       be for one of the same verbs, so that its only use is as the alternate
C       destination for those verbs.  For instance:

C               15      110022  29      31      34 35 23 43
C               15      14      29

C       This says that, from LOC 15, any of the verbs 29, 31, etc., will take
C       him to 22 if he's carrying object 10, and otherwise will go to 14.

C               11      303008  49
C               11      9       50

C       This says that, from 11, 49 takes him to 8 unless PROP(3)=0, in which
C       case he goes to 9.  Verb 50 takes him to 9 regardless of PROP(3).

C   Section 4: Vocabulary.  Each line contains a number (N), and a
C       five-letter word.  Call M=N/1000.  if M=0, then the word is a motion
C       verb for use in travelling (see Section 3).  Else, if M=1, the word is
C       an object.  Else, if M=2, the word is an action verb (such as  CARRY
C       or  ATTACK  ).  Else, if M=3, the word is a special case verb (such
C       DIG ) and N mod 1000 is an index into Section 6.  Objects from 50 to
C       (currently, anyway) 79 are considered treasures (for pirate, closeout).

C   Section 5: Object descriptions.  Each line contains a number (N),
C       and a message.  If N is from 1 to 100, the message is the  inventory
C       message for Object N.  otherwise, N should be 000, 100, 200, etc., and
C       the message should be the description of the preceding object when its
C       PROP value is N/100.  The N/100 is used only to distinguish multiple
C       messages from multi-line messages; the prop info actually requires all
C       messages for an object to be present and consecutive.  Properties which
C       produce no message should be given the message  >$<

C   Section 6: Arbitrary messages.  Same format as Sections 1, 2, and 5, except
C       the numbers bear no relation to anything (except for special verbs
C       in Section 4).

C   Section 7: Object locations.  Each line contains an object number and its
C       initial location (zero (or omitted) if none).  If the object is
C       immovable, the location is followed by a  -1.  If it has two locations
C       (e.g. the grate) the first location is followed with the second, and
C       the object is assumed to be immovable.

C   Section 8: Action defaults. Each line contains an  action-verb  number and
C       the index (in Section 6) of the default message for the verb.

C   Section 9: Liquid assets, etc.  Each line contains a number (N) and up to 20
C       location numbers.  Bit N (where 0 is the units bit) is set in COND(LOC)
C       for each LOC given.  The COND bits currently assigned are:

C               0       Light
C               1       If bit 2 is on: on for oil, off for water
C               2       Liquid asset, see bit 1
C               3       Pirate doesn't go here unless following player

C       Other bits are used to indicate areas of interest to  hint  routines:

C               4       Trying to get into cave
C               5       Trying to catch bird
C               6       Trying to deal with snake
C               7       Lost in maze
C               8       Pondering Dark room
C               9       At Witt's End

C       COND(LOC) is set to 2, overriding all other bits, if LOC has forced
C       motion.

C   Section 10: Proficiency messages.  Each line contains a number (N), and a
C       message describing a classification of player.  The scoring section
C       selects the appropriate message, where each message is considered to
C       apply to players whose scores are higher than the previous N but not
C       higher than this N.  Note that these scores probably change with every
C       modification (and particularly expansion) of the program.

C   Section 11: Hints.  Each line contains a hint number (corresponding to a
C       COND bit, see Section 9), the number of turns he must be at the right
C       LOC(s) before triggering the hint, the points deducted for taking the
C       hint, the message number (Section 6) of the question, and the message
C       number of the hint.  These values are stashed in the  HINTS  array.
C       HNTMAX is set to the max hint number (<= HNTSIZ).  Numbers 1-3 are
C       unusable since COND bits are otherwise assigned, so 2 is used to
C       remember if he's read the clue in the repository, and 3 is used to
C       remember whether he asked for instructions (gets more turns, but loses
C       points).

C   Section 12: Magic messages.  Identical to Section 6 except put in a separate
C       section for easier reference.  Magic messages are used by the startup,
C       maintenance mode, and related routines.

C   Section 0 - End of database


C  Read the database if we have no yet done so


C  Here we call a MASM subroutine for initialization of the D-bank.
C  This routine is for Univac 1100 series machines, and may not
C  be necessary on other systems.

      CALL INITDB

      IF (SETUP.NE.0) GO TO 320
      PRINT 10
10    FORMAT (' Initializing...')

      OPEN (UNIT=8, FILE='ADV.DAT', STATUS='OLD', ACTION='READ')
      RINIT=8
      SETUP=1
      LINUSE=1
      TRVS=1
      LEVS=1
      sect = 0

C  Start new data section.  SECT is the section number.

30    kk = sect
32    READ (RINIT,34) SECT
34    format (I3)
40    FORMAT ()

C  The following kludge permits comments in the database,  Since
C  Univac FORTRAN treats blanks as zeroes in numeric fields.

      if (kk .lt. 12 .and. sect .eq. 0) go to 32

      OLDLOC=-1
      if (sect .eq. 0) go to 320
      IF (SECT .NE. KK+1 .OR. SECT .GT. 12) CALL BUG (9)
      GO TO (50,50,150,210,50,50,250,260,270,50,290,50), sect
C             1  2  3   4   5  6  7   8   9  10  11  12

C  Sections 1, 2, 5, 6, 10, 12.  Read messages and set up pointers.

50    READ (RINIT,60) LOC, chr, (CLINES(J),J=LINUSE+1,LINUSE+18), CKK
60    FORMAT (I3, A1, 19A4)
      IF (CKK.NE.'    ') call bug(0)
      IF (LOC.EQ.-1) GO TO 30
      DO 70 K=18,1,-1
      KK=LINUSE+K
      IF (CLINES(KK).NE.'    ') GO TO 80
70    CONTINUE
      if (sect .eq. 2 .or. sect .eq. 10) call bug (1)
      kk = linuse
80    LINES(LINUSE)=KK+1
      IF (LOC.EQ.OLDLOC) GO TO 140
      LINES(LINUSE)=-LINES(LINUSE)
      goto (90,85,82,82,100,110,82,82,82,120,82,130), sect
C           1  2  x  x   5   6  x  x  x   10 x   12
82    call bug (9)

85    STEXT(LOC)=LINUSE
      GO TO 140

90    LTEXT(LOC)=LINUSE
      GO TO 140

100   IF (LOC.GT.0.AND.LOC.LE.100) PTEXT(LOC)=LINUSE
      GO TO 140

110   IF (LOC.GT.RTXSIZ) CALL BUG (6)
      RTEXT(LOC)=LINUSE
      if (chr .eq. star) rmsgx(loc) = .true.
      GO TO 140

120   CTEXT(LEVS)=LINUSE
      CVAL(LEVS)=LOC
      LEVS=LEVS+1
      GO TO 140

130   IF (LOC.GT.MAGSIZ) CALL BUG (6)
      MTEXT(LOC)=LINUSE

140   LINUSE=KK+1
      LINES(LINUSE)=-1
      OLDLOC=LOC
      IF (LINUSE+18.GT.LINSIZ) CALL BUG (2)
      GO TO 50

C  The stuff for Section 3 is encoded here.  Each from-location gets a
C  contiguous section of the TRAVEL array.  Each entry in TRAVEL is
C  NEWLOC*1000 + KEYWORD (from Section 4, motion verbs), and is negated
C  if this is the last entry for this location.  KEY(N) is the index in
C  TRAVEL of the first option at location N.

150   DO 155 I=1,20
155     TK(I) = 0
      READ (RINIT,*) LOC, NEWLOC, TK
      IF (LOC.EQ.-1) GO TO 30
      IF (KEY(LOC).NE.0) GO TO 170
      KEY(LOC)=TRVS
      GO TO 180
170   TRAVEL(TRVS-1)=-TRAVEL(TRVS-1)
180   DO 190 L=1,20
        IF (TK(L).EQ.0) GO TO 200
        TRAVEL(TRVS)=NEWLOC*1000+TK(L)
        TRVS=TRVS+1
        IF (TRVS.EQ.TRVSIZ) CALL BUG(3)
190     CONTINUE
200   TRAVEL(TRVS-1)=-TRAVEL(TRVS-1)
      GO TO 150

C  Here we read in the vocabulary.  KTAB(N) is the word number, ATAB(N)
C  is the corresponding word.  The -1 at the end of section 4 is left in
C  KTAB as an end-marker.  The words are given a minimal hash to make
C  reading the core-image harder.

210   DO 240 TABNDX=1,TABSIZ
220     READ (RINIT,230) KTAB(TABNDX),ATAB(TABNDX)
230     FORMAT (I4, 1X, A5)

        IF (KTAB(TABNDX).EQ.-1) GOTO 30
        call hash(atab(tabndx))

240     continue
      CALL BUG (4)

C  Read in the initial locations for each object.  Also the immovability
C  info.  The PLAC array contains initial locations of objects.  FIXD
C  is -1 for immovable objects (including the snake), or = second
C  location for two-placed objects.

250   READ (RINIT,*) OBJ,J,K
      IF (OBJ.EQ.-1) GO TO 30
      PLAC(OBJ)=J
      FIXD(OBJ)=K
      GO TO 250

C  Read default message numbers for action verbs, store in ACTSPK array.

260   READ (RINIT,*) VERB,J
      IF (VERB.EQ.-1) GO TO 30
      ACTSPK(VERB)=J
      GO TO 260

C  Read info about available liquids and other conditions, store in
C  COND array.

270   DO 275 I=1,20
275     TK(I) = 0
      READ (RINIT,*) K,TK
      IF (K.EQ.-1) GO TO 30
      DO 280 I=1,20
        LOC=TK(I)
        IF (LOC.EQ.0) GO TO 270
        IF (BITSET(LOC,K)) CALL BUG (8)
280     COND(LOC)=COND(LOC)+SHIFT(1,K)
      GO TO 270

C  Read data for hints.

290   HNTMAX=0
300   DO 305 I=1,20
305     TK(I) = 0
      READ (RINIT,*) K,TK
      IF (K.EQ.-1) GO TO 30
      IF (K.EQ.0) GO TO 300
      IF (K.LT.0.OR.K.GT.HNTSIZ) CALL BUG (7)
      DO 310 I=1,4
310     HINTS(K,I)=TK(I)
      HNTMAX=MAX0(HNTMAX,K)
      GO TO 300

C  Find out what type of adventurer we're dealing with
C  (in-house or out-house).

320    class = 0
       call getcls(class)
       if (class.eq.1.or.class.eq.2) goto 322
       print 321, class
321    format (/' Bad class number of ',I12)
       stop
322    continue


C  We have read in database cards (SETUP = 1),
C        or gotten user's D-bank  (SETUP = -1),
C        or gotten master D-bank  (SETUP = 2).

C  Finish constructing internal data tables.

C  If SETUP=2, we don't need to do this.  It's only necessary if we
C  haven't done it at all, or if the program has been run since then.

      CALL RESTOREDB
      IF (SETUP.EQ.2) GO TO 500
      IF (SETUP.NE.-1) GO TO 325

C  User is restarting a saved game.

      YEA=START(class)
      SETUP=3
      K=NULL
      GO TO 1210

C  Having read in the database, certain things are now constructed.
C  PROPs are set to zero.  We finish settup up COND by checking for
C  forced-motion TRAVEL entries.  The PLAC and FIXD arrays are used to
C  set up ATLOC(N) as the first object at location N, and LINK(IBJ) as
C  the next object at the same location as OBJ.  (OBJ>OBJSIZ indicates
C  that FIXED(OBJ-OBJSIZ)=LOC.  LINK(OBJ) is still the correct link to
C  use.  ABB is zeroed:  it controls whether the abbreviated description
C  is printed.  Counts MOD 5 unless LOOK is used.

325   DO 330 I=1,OBJSIZ
        PLACE(I)=0
        PROP(I)=0
        LINK(I)=0
330     LINK(OBJSIZ+I)=0

      DO 340 I=1,LOCSIZ
        ABB(I)=0
        IF (LTEXT(I).EQ.0.OR.KEY(I).EQ.0) GO TO 340
        K=KEY(I)
        IF (MOD(IABS(TRAVEL(K)),1000).EQ.1) COND(I)=2
340     ATLOC(I)=0

C  Set up the ATLOC and LINK arrays as described above.  We'll use the
C  DROP subroutine, which prefaces new objects on the lists.  Since we
C  want things in the other order, we'll run the loop backwards.  If the
C  object is in two locations, we drop it twice.  This also sets up
C  PLACE and FIXED as copies of PLAC and FIXD.  Also, since two-placed
C  objects are typically best described last, we'll drop them first.

      DO 350 K=OBJSIZ,1,-1
        IF (FIXD(K).LE.0) GO TO 350
        CALL DROP (K+OBJSIZ,FIXD(K))
        CALL DROP (K,PLAC(K))
350     CONTINUE

      DO 360 K=OBJSIZ,1,-1
        FIXED(K)=FIXD(K)
360     IF (PLAC(K).NE.0.AND.FIXD(K).LE.0) CALL DROP (K,PLAC(K))

C  Treasures, as noted earlier, are objects 50 through MAXTRS (currently
C  79).  Their PROPs are initially -1, and are set to 0 the first time
C  they are described.  TALLY keeps track of how many are not yet found,
C  so we know when to close the cave.  TALLY2 counts how many can never
C  be found (e.g., if lost bird or bridge).

      MAXTRS=79
      TALLY=0
      TALLY2=0
      DO 370 I=50,MAXTRS
        IF (PTEXT(I).NE.0) PROP(I)=-1
370     TALLY=TALLY-PROP(I)

C  Clear the hint stuff.  HINTLC(I) is how long he's been at LOC with
C  COND bit I set.  HINTED(I) is true if hint I has been issued.

      DO 380 I=1,HNTMAX
        HINTED(I)=.FALSE.
380     HINTLC(I)=0

C  Define some handy mnemonics.  These correspond to object numbers.

      KEYS=VOCAB('keys ',1)
      LAMP=VOCAB('lamp ',1)
      GRATE=VOCAB('grate',1)
      CAGE=VOCAB('cage ',1)
      ROD=VOCAB('rod  ',1)
      ROD2=ROD+1
      STEPS=VOCAB('steps',1)
      BIRD=VOCAB('bird ',1)
      DOOR=VOCAB('door ',1)
      PILLOW=VOCAB('pillo',1)
      SNAKE=VOCAB('snake',1)
      SNAKE2 = SNAKE + 1
      FISSUR=VOCAB('fissu',1)
      TABLET=VOCAB('table',1)
      CLAM=VOCAB('clam ',1)
      OYSTER=VOCAB('oyste',1)
      MAGZIN=VOCAB('magaz',1)
      DWARF=VOCAB('dwarf',1)
      KNIFE=VOCAB('knife',1)
      FOOD=VOCAB('food ',1)
      BOTTLE=VOCAB('bottl',1)
      WATER=VOCAB('water',1)
      OIL=VOCAB('oil  ',1)
      PLANT=VOCAB('plant',1)
      PLANT2=PLANT+1
      AXE=VOCAB('axe  ',1)
      MIRROR=VOCAB('mirro',1)
      DRAGON=VOCAB('drago',1)
      CHASM=VOCAB('chasm',1)
      TROLL=VOCAB('troll',1)
      TROLL2=TROLL+1
      BEAR=VOCAB('bear ',1)
      MESSAG=VOCAB('messa',1)
      VEND=VOCAB('vendi',1)
      BATTER=VOCAB('batte',1)
      ROPE = VOCAB('rope ',1)
      ROPE2 = ROPE + 1

C  Objects from 50 through whatever are treasures.  Here are a few.

      SPICES=VOCAB('spice',1)
      NUGGET=VOCAB('gold ',1)
      COINS=VOCAB('coins',1)
      CHEST=VOCAB('chest',1)
      EGGS=VOCAB('eggs ',1)
      TRIDNT=VOCAB('tride',1)
      VASE=VOCAB('vase ',1)
      EMRALD=VOCAB('emera',1)
      PYRAM=VOCAB('pyram',1)
      PEARL=VOCAB('pearl',1)
      RUG=VOCAB('rug  ',1)
      CHAIN=VOCAB('chain',1)
      RUBY = VOCAB('ruby ',1)

C  These are motion-verb numbers.

      BACK=VOCAB('back ',0)
      LOOK=VOCAB('look ',0)
      CAVE=VOCAB('cave ',0)
      NULL=VOCAB('null ',0)
      ENTRNC=VOCAB('entra',0)
      DPRSSN=VOCAB('depre',0)

C  and some action verbs.

      SAY=VOCAB('say  ',2)
      LOCK=VOCAB('lock ',2)
      THROW=VOCAB('throw',2)
      FIND=VOCAB('find ',2)
      INVENT=VOCAB('inven',2)

C  Initialize the dwarves.  DLOC is location of dwarves, hard wired in.
C  ODLOC is prior location of each dwarf, initially garbage.  DALTC is
C  alternate initial location for dwarf, in case one of them starts out
C  on top of the adventurer (no 2 of the 5 initial locations are
C  adjacent.)  DSEEN is true if dwarf has seen him.  DFLAG controls the
C  level of activation of all this:

C      0   No dwarf stuff yet (wait until he reaches Hall of Mists)
C      1   Reached Hall of Mists, but hasn't yet met first dwarf
C      2   Met first dwarf, others start moving, no knives thrown yet.
C      3   A knife has been thrown (first set always misses)
C      3+  Dwarves are mad (increases   their accuracy)

C  Sixth dwarf is special (the pirate).  He always starts at his chest's
C  eventual location inside the maze.  This location is saved in CHLOC
C  for  reference.  The dead end in the other maze has its location
C  stored in CHLOC2.

      CHLOC=114
      CHLOC2=140
      DO 390 I=1,6
390     DSEEN(I)=.FALSE.
      DFLAG=0
      DLOC(1)=19
      DLOC(2)=27
      DLOC(3)=33
      DLOC(4)=44
      DLOC(5)=64
      DLOC(6)=CHLOC
      DALTLC=18

C  Other random flags and counters, as follows:

C   TURNS   Tallies how many commands he's given (ignores yes/no)
C   LIMIT   Lifetime of lamp (not set here)
C   IWEST   How many times he's said WEST instead of W
C   KNFLOC  0 if no knife here, LOC if knife here, -1 after caveat
C   DETAIL  How often we've said "not allowed to give more details."
C   ABBNUM  How often we should print non-abbreviated descriptions
C   MAXDIE  Number of reincarnation messages available (up to 5)
C   NUMDIE  Number of times killed so far
C   HOLDNG  Number of objects being carried
C   DKILL   Number or dwarves killed (unused in scoring, needed for msg)
C   FOOBAR  Current progress in saying "FEE FIE FOE FOO".
C   BONUS   Used to determine amount of bonus if he reaches closing
C   CLOCK1  Number of turns from finding last treasure till closing
C   CLOCK2  Number of turns from first warning till blinding flash

C  Logicals were explained earlier.

      TURNS=0
      LMWARN=.FALSE.
      mltcmd = .false.
      samvrb = .false.
      IWEST=0
      KNFLOC=0
      DETAIL=0
      ABBNUM=5
      DO 400 I=0,4
400     IF (RTEXT(2*I+81).NE.0) MAXDIE=I+1
      NUMDIE=0
      HOLDNG=0
      DKILL=0
      FOOBAR=0
      BONUS=0
      CLOCK1=30
      CLOCK2=25
      SAVED=0
      CLOSNG=.FALSE.
      PANIC=.FALSE.
      CLOSED=.FALSE.
      GAVEUP=.FALSE.
      SCORNG=.FALSE.

      SETUP=2

      DO 410 K=LOCSIZ,1,-1
        kk=k
        IF (LTEXT(KK).NE.0) GO TO 420
410     CONTINUE

420   OBJ=0
      DO 430 K=1,OBJSIZ
430     IF (PTEXT(K).NE.0) OBJ=OBJ+1

      DO 440 K=1,TABNDX
440     IF (KTAB(K)/1000.EQ.2) VERB=KTAB(K)-2000

      DO 450 K=RTXSIZ,1,-1
        j=k
        IF (RTEXT(J).NE.0) GO TO 460
450     CONTINUE

460   DO 470 K=MAGSIZ,1,-1
        i=k
        IF (MTEXT(I).NE.0) GO TO 480
470     CONTINUE

480   continue

          print 481, LINUSE,LINSIZ,TRVS,TRVSIZ,TABNDX,TABSIZ,KK,
     1       LOCSIZ,OBJ,OBJSIZ,VERB,VRBSIZ,J,RTXSIZ,LEVS,LEVSIZ,
     2       HNTMAX,HNTSIZ,I,MAGSIZ
481       format (
     1  /,'  LINES:',I6,' of',I6,' words used',
     2  /,' TRAVEL:',I6,' of',I6,' words used',
     3  /,'   KTAB:',I6,' of',I6,' words used',
     4  /,'  LTEXT:',I6,' of',I6,' words used',
     5  /,'  PTEXT:',I6,' of',I6,' words used',
     6  /,' ACTSPK:',I6,' of',I6,' words used',
     7  /,'  RTEXT:',I6,' of',I6,' words used',
     8  /,'  CTEXT:',I6,' of',I6,' words used',
     9  /,' HINTED:',I6,' of',I6,' words used',
     1  /,'  MTEXT:',I6,' of',I6,' words used',
     2  /)
482       continue

C  Finally, since we're clearly setting things up for the first time...

      CALL POOF
      CALL LOADCFG
      CLOSE(UNIT=8)
      PRINT 490
490   FORMAT (' Init done')

C  *****  Come here to start a new game.

C  Start-up, dwarf stuff

500   DEMO=START(class)
      CALL MOTD (.FALSE.)
      I=RAN(1)
      HINTED(3)=YES(65,1,0)
      NEWLOC=1
      SETUP=3
      LIMIT=350
      IF (HINTED(3)) LIMIT=1000

C  *****  Main loop

C  Can't leave cave once it's closing (except by main office).

510   IF (NEWLOC.GE.9.OR.NEWLOC.EQ.0.OR..NOT.CLOSNG) GO TO 520
      CALL RSPEAK (130)
      NEWLOC=LOC
      IF (.NOT.PANIC) CLOCK2=15
      PANIC=.TRUE.

C  See if a dwarf has seen him and has come from where he wants to go.
C  If so, the dwarf's blocking his way.  If coming from place forbidden
C  to pirate (dwarves rooted in place) let him get out (and attacke).

520   IF (NEWLOC.EQ.LOC.OR.FORCED(LOC).OR.BITSET(LOC,3)) GO TO 540
      DO 530 I=1,5
        IF (ODLOC(I).NE.NEWLOC.OR..NOT.DSEEN(I)) GO TO 530
        NEWLOC=LOC
        CALL RSPEAK (2)
        GO TO 540
530     CONTINUE
540   LOC=NEWLOC

C  Dwarf stuff.  See earlier comments for description of variables.
C  Remember, sixth dwarf is pirate, and is thus very different except
C  for motion rules.

C  First off, don't let the dwarves follow him into a pit or a wall.
C  Activate the whole mess the first time he gets as far as the Hall
C  of Mists (location 15).  If NEWLOC is forbidden to pirate (in
C  particular, if it's beyond the troll bridge), bypass dwarf stuff.
C  That way, pirate can't steal return toll, and dwarves can't meet the
C  bear.  Also means dwarves won't follow him into dead end in maze,
C  but c'est la vie.  They'll wait for him outside the dead end.

      IF (LOC.EQ.0.OR.FORCED(LOC).OR.BITSET(NEWLOC,3)) GO TO 780
      IF (DFLAG.NE.0) GO TO 550
      IF (LOC.GE.15) DFLAG=1
      GO TO 780

C  When we encounter the first dwarf, we kill 0, 1, or 2 of the five
C  dwarves.  If any of the survivors is at LOC, replace him with the
C  alternate.

550   IF (DFLAG.NE.1) GO TO 580
      IF (LOC.LT.15.OR.PCT(95)) GO TO 780
      DFLAG=2
      DO 560 I=1,2
        J=1+RAN(5)

C  If SAVED not = -1, he bypassed the START call.
C  If he is continuing a game, remove dwarves 50% of the time.

560     IF (PCT(50).AND.SAVED.EQ.-1) DLOC(J)=0
      DO 570 I=1,5
        IF (DLOC(I).EQ.LOC) DLOC(I)=DALTLC
570     ODLOC(I)=DLOC(I)
      CALL RSPEAK (3)
      CALL DROP (AXE,LOC)
      GO TO 780

C  Things are in full swing.  Move each dwarf at random, except if he's
C  seen us he sticks with us.  Dwarves never go to locations <15.  If
C  If wandering at random, they don't back up unless there's no
C  alternative.  If they don't have to move, they attack.  And, of
C  course, dead dwarves don't do much of anything.

580   DTOTAL=0
      ATTACK=0
      STICK=0
      DO 680 I=1,6
        IF (DLOC(I).EQ.0) GO TO 680
        J=1
        KK=DLOC(I)
        KK=KEY(KK)
        IF (KK.EQ.0) GO TO 610
590     NEWLOC=MOD(IABS(TRAVEL(KK))/1000,1000)
        IF (NEWLOC.GT.300.OR.NEWLOC.LT.15.OR.NEWLOC.EQ.ODLOC(I).OR.(J.GT
     1  .1.AND.NEWLOC.EQ.TK(J-1)).OR.J.GE.20.OR.NEWLOC.EQ.DLOC(I).OR.FOR
     2  CED(NEWLOC).OR.(I.EQ.6.AND.BITSET(NEWLOC,3)).OR.IABS(TRAVEL(KK))
     3  /1000000.EQ.100) GO TO 600
        TK(J)=NEWLOC
        J=J+1
600     KK=KK+1
        IF (TRAVEL(KK-1).GE.0) GO TO 590
610     TK(J)=ODLOC(I)
        IF (J.GE.2) J=J-1
        J=1+RAN(J)
        ODLOC(I)=DLOC(I)
        DLOC(I)=TK(J)
        DSEEN(I)=(DSEEN(I).AND.LOC.GE.15).OR.(DLOC(I).EQ.LOC.OR.ODLOC(I)
     1  .EQ.LOC)
        IF (.NOT.DSEEN(I)) GO TO 680
        DLOC(I)=LOC
        IF (I.NE.6) GO TO 670

C  The pirate's spotted him, but leaves him alone once he's found chest.
C  K counts if a treasure is here.  If not, and TALLY=TALL2 plus one
C  for an unseen chest, let the pirate be spotted.

        IF (LOC.EQ.CHLOC.OR.PROP(CHEST).GE.0) GO TO 680
        K=0
        DO 620 J=50,MAXTRS

C  Pirate won't take pyramid from Plover room or Dark room (too easy).

          IF (J.EQ.PYRAM.AND.(LOC.EQ.PLAC(PYRAM).OR.LOC.EQ.PLAC(EMRALD))
     1    ) GO TO 620
          IF (TOTING(J)) GO TO 630
620       IF (HERE(J)) K=1
        IF (TALLY.EQ.TALLY2+1.AND.K.EQ.0.AND.PLACE(CHEST).EQ.0.AND.HERE(
     1  LAMP).AND.PROP(LAMP).EQ.1) GO TO 660
        IF (ODLOC(6).NE.DLOC(6).AND.PCT(20)) CALL RSPEAK (127)

        GO TO 680

630     CALL RSPEAK (128)

C  Don't steal chest back from troll.

        IF (PLACE(MESSAG).EQ.0) CALL MOVE (CHEST,CHLOC)
        CALL MOVE (MESSAG,CHLOC2)
        DO 640 J=50,MAXTRS
          IF (J.EQ.PYRAM.AND.(LOC.EQ.PLAC(PYRAM).OR.LOC.EQ.PLAC(EMRALD))
     1) GO TO 640
          IF (AT(J).AND.FIXED(J).EQ.0) CALL CARRY (J,LOC)
          IF (TOTING(J)) CALL DROP (J,CHLOC)
640       CONTINUE
650     DLOC(6)=CHLOC
        ODLOC(6)=CHLOC
        DSEEN(6)=.FALSE.
        GO TO 680

660     CALL RSPEAK (186)
        CALL MOVE (CHEST,CHLOC)
        CALL MOVE (MESSAG,CHLOC2)
        GO TO 650

C  This threatening little dwarf is in the room with him.

670     DTOTAL=DTOTAL+1
        IF (ODLOC(I).NE.DLOC(I)) GO TO 680
        ATTACK=ATTACK+1
        IF (KNFLOC.GE.0) KNFLOC=LOC
        IF (RAN(1000).LT.95*(DFLAG-2)) STICK=STICK+1
680     CONTINUE

C  Now we know what's happening.  Let's tell the poor sucker about it.

      IF (DTOTAL.EQ.0) GO TO 780
      IF (DTOTAL.EQ.1) GO TO 700
      PRINT 690, DTOTAL
      mltcmd = .false.
690   FORMAT (/' There are ',I1,' threatening little dwarves in the',' r
     1oom with you.')
      mltcmd = .false.
      GO TO 710
700   CALL RSPEAK (4)
710   IF (ATTACK.EQ.0) GO TO 780
      IF (DFLAG.EQ.2) DFLAG=3

C  If SAVED not = -1, he bypassed the START call.  Dwarves get VERY mad.

      IF (SAVED.NE.-1) DFLAG=20
      IF (ATTACK.EQ.1) GO TO 770
      PRINT 720, ATTACK
720   FORMAT (/' ',I1,' of them throw knives at you.')
      mltcmd = .false.
      K=6
730   IF (STICK.GT.1) GO TO 740
      CALL RSPEAK (K+STICK)
      IF (STICK.EQ.0) GO TO 780
      GO TO 760
740   PRINT 750, STICK
750   FORMAT (/' ',I1,' of them get you.')
      mltcmd = .false.
760   OLDLC2=LOC
      GO TO 1430

770   CALL RSPEAK (5)
      K=52
      GO TO 730

C  Describe the current location and (maybe) get next command.

C  Print text for current location, unless it's dark.

780   IF (LOC.EQ.0) GO TO 1430
      KK=STEXT(LOC)
      IF (MOD(ABB(LOC),ABBNUM).EQ.0.OR.KK.EQ.0) KK=LTEXT(LOC)
      IF (FORCED(LOC).OR..NOT.DARK()) GO TO 790
      IF (WZDARK.AND.PCT(35)) GOTO 1420
      KK=RTEXT(16)
790   IF (TOTING(BEAR)) CALL RSPEAK(141)
      CALL SPEAK (KK)
      K=1
      IF (FORCED(LOC)) GO TO 1210
      IF (LOC.EQ.33.AND.PCT(25).AND..NOT.CLOSNG) CALL RSPEAK (8)

C  Print out descriptions of objects at this location.  If not closing,
C  and property value is negative, tally off another treasure.  Rug is
C  special case:  once seen, its PROP is 1 (dragon on it) till dragon
C  is killed.  Similarly for chain: PROP is initially 1 (locked to
C  bear).  These hacks are because PROP=0 is needed to get full score.

      IF (DARK().AND.TOTING(RUBY)) CALL RSPEAK (212)
      IF (DARK()) GO TO 860
      ABB(LOC)=ABB(LOC)+1
      I=ATLOC(LOC)
      blklin = .true.
800   IF (I.EQ.0) GO TO 860
      OBJ=I
      IF (OBJ.GT.OBJSIZ) OBJ=OBJ-OBJSIZ
      IF (OBJ.EQ.STEPS.AND.TOTING(NUGGET)) GO TO 820
      IF (PROP(OBJ).GE.0) GO TO 810
      IF (CLOSED) GO TO 820
      PROP(OBJ)=0
      IF (OBJ.EQ.RUG.OR.OBJ.EQ.CHAIN) PROP(OBJ)=1
      TALLY=TALLY-1

C  If remaining treasures too elusive, zap his lamp.

      IF (TALLY.EQ.TALLY2.AND.TALLY.NE.0) LIMIT=MIN0(35,LIMIT)
810   KK=PROP(OBJ)
      IF (OBJ.EQ.STEPS.AND.LOC.EQ.FIXED(STEPS)) KK=1
      CALL PSPEAK (OBJ,KK)
820   I=LINK(I)
      blklin = .false.
      GO TO 800

830   K=54
840   SPK=K
850   CALL RSPEAK (SPK)

860   oldvrb = verb
      VERB=0
      OBJ=0
      blklin = .true.

C  Check if this location is eligible for any hints.  If he's been here
C  long enough, branch to help section (on later page).  Hints all come
C  back here eventually to finish the loop.  Ignore hints <4 (special
C  stuff, see database.)

870   DO 990 HINT=4,HNTMAX
        IF (HINTED(HINT)) GO TO 990
        IF (.NOT.BITSET(LOC,HINT)) HINTLC(HINT)=-1
        HINTLC(HINT)=HINTLC(HINT)+1
        IF (HINTLC(HINT).GE.HINTS(HINT,1)) GO TO 880
        GO TO 990

C  *****  Hints

C  Come here if he's been long enough at required loc(s) for some unused hint.
C  Hint number is in variable HINT.  Branch to quick test for additional
C  conditions, then come back to do neat stuff.  GOTO 890 if condition
C  met and we want to offer the hint.  GOTO 910 to clear HINTLC back to
C  zero.  GOGO 920 to take no action yet.

880     IF ((HINT-3).LT.1.OR.(HINT-3).GT.6) CALL BUG (27)
        GO TO (930, 940, 950, 960, 970, 980),  hint-3
C             CAVE BIRD SNAKE MAZE DARK() WITT

890     HINTLC(HINT)=0
        IF (.NOT.YES(HINTS(HINT,3),0,54)) GO TO 990
        PRINT 900, HINTS(HINT,2)
900   FORMAT (/' I am prepared to give you a hint, but it will cost you'
     1I2,' points.')
      mltcmd = .false.
        HINTED(HINT)=YES(175,HINTS(HINT,4),54)
        IF (HINTED(HINT).AND.LIMIT.GT.30) LIMIT=LIMIT+30*HINTS(HINT,2)
910     HINTLC(HINT)=0
920     GO TO 990

C  Now for the quick tests.  See database description for one-line notes

930     IF (PROP(GRATE).EQ.0.AND..NOT.HERE(KEYS)) GO TO 890
        GO TO 910

940     IF (HERE(BIRD).AND.TOTING(ROD).AND.OBJ.EQ.BIRD) GO TO 890
        GO TO 920

950     IF ((AT(SNAKE).OR.AT(SNAKE2)).AND..NOT.HERE(BIRD).AND.PROP(SNAKE
     1).EQ.0) GO TO 890
        GO TO 910

960     IF (ATLOC(LOC).EQ.0.AND.ATLOC(OLDLOC).EQ.0.AND.ATLOC(OLDLC2).EQ.
     1  0.AND.HOLDNG.GT.1) GO TO 890
        GO TO 910

970     IF (PROP(EMRALD).NE.-1.AND.PROP(PYRAM).EQ.-1) GO TO 890
        GO TO 910

980     GO TO 890
990     CONTINUE

C  Kick the random number generator just to add variety to the chase.
C  And if closing time, check for any objects being toted with PROP<0
C  and set the PROP to -1-PROP.  This way, objects won't be described
C  until they have been picked up and put down deparate from their
C  respective piles.  Don't tick CLOCK1 unless well into cave (and not
C  at Y2.

      IF (.NOT.CLOSED) GO TO 1010
      IF (PROP(OYSTER).LT.0.AND.TOTING(OYSTER)) CALL PSPEAK (OYSTER,1)
      DO 1000 I=1,OBJSIZ
1000    IF (TOTING(I).AND.PROP(I).LT.0) PROP(I)=-1-PROP(I)
1010  WZDARK=DARK()
      IF (KNFLOC.GT.0.AND.KNFLOC.NE.LOC) KNFLOC=0
      I=RAN(1)

C  *****  Get next command

      CALL GETIN (WD1,WD1X,WD2,WD2X)

C  Every input, check FOOBAR flag.  If zero, nothing's going on.  If
C  positive, make negative.  If negative, he skipped a word, so make
C  it zero.

1020  FOOBAR=MIN0(0,-FOOBAR)
      IF (TURNS.EQ.0.AND.WD1.EQ.'magic'.AND.WD2.EQ.'mode') CALL MAINT
      TURNS=TURNS+1
      IF (DEMO.AND.TURNS.GE.SHORT) GO TO 2340
      if (samvrb) verb = oldvrb
      IF (VERB.EQ.SAY.AND.WD2.NE.'     ') VERB=0
      IF (VERB.EQ.SAY) GO TO 1090
      IF (TALLY.EQ.0.AND.LOC.GE.15.AND.LOC.NE.33) CLOCK1=CLOCK1-1
      IF (CLOCK1.EQ.0) GO TO 2260
      IF (CLOCK1.LT.0) CLOCK2=CLOCK2-1
      IF (CLOCK2.EQ.0) GO TO 2280
      IF (PROP(LAMP).EQ.1) LIMIT=LIMIT-1
      IF (LIMIT.LE.30.AND.HERE(BATTER).AND.PROP(BATTER).EQ.0.AND.HERE(LA
     1MP)) GO TO 2300
      IF (LIMIT.EQ.0) GO TO 2320
      IF (LIMIT.LT.0.AND.LOC.LE.8) GO TO 2330
      IF (LIMIT.LE.30) GO TO 2310
1030  K=43
      IF (LIQLOC(LOC).EQ.WATER) K=70
      IF (WD1.EQ.'enter'.AND.(WD2.EQ.'strea'.OR.WD2.EQ.'water')) GO TO 8
     140
      IF (WD1.EQ.'enter'.AND.WD2.NE.'     ') GO TO 1060
      IF ((WD1.NE.'water'.AND.WD1.NE.'oil').OR.(WD2.NE.'plant'.AND.WD2.N
     1E.'door')) GO TO 1040
      IF (AT(VOCAB(WD2,1))) WD2='pour '
1040  IF (WD1.NE.'west') GO TO 1050
      IWEST=IWEST+1
      IF (IWEST.EQ.10) CALL RSPEAK (17)
1050  I=VOCAB(WD1,-1)
      IF (I.EQ.-1) GO TO 1070
      K=MOD(I,1000)
      KQ=I/1000+1
      IF (KQ.GT.4) CALL BUG (22)
      GO TO (1210,1100,1080,840), KQ

C  Get second word for analysis.

1060  WD1=WD2
      WD1X=WD2X
      WD2='     '
      GO TO 1040

C  Gee, I don't understand.

1070  SPK=60
      IF (PCT(20)) SPK=61
      IF (PCT(20)) SPK=13
      CALL RSPEAK (SPK)
      GO TO 870

C  Analyse a verb.  Remember what it was, go back for object if second
C  word, unless verb is SAY, which snarfs arbitrary second word.

1080  VERB=K
      SPK=ACTSPK(VERB)
      IF (WD2.NE.'     '.AND.VERB.NE.SAY) GO TO 1060
       if (verb.eq.say.and.wd2.ne.'     ') obj=1
       if (verb.eq.say.and.wd2.eq.'     ') obj=0
       IF (OBJ.NE.0) GO TO 1090

C  Analyse an intransitive verb (ie, no object given yet).

      IF (VERB.GT.37) CALL BUG (23)
      GO TO (1480,1460,1460,1670,830,1670,1730,1740,1460,1460,850,1760,1
     1820,1840,1870,1460,1460,1960,1460,2000,1460,2070,2090,2100,2130,21
     250,2160,1460,1460,2210,2216,2220,1460,1460,1460,1460,1460), VERB
C           TAKE DROP  SAY OPEN NOTH LOCK   ON  OFF WAVE CALM
C           WALK KILL POUR  EAT DRNK  RUB TOSS QUIT FIND INVN
C           FEED FILL BLST SCOR  FOO  BRF READ BREK WAKE SUSP
C           HOUR TEST  CUT  TIE UNTI  RIG CLIK

C  Analyse a transitive verb.

1090  IF (VERB.GT.37) CALL BUG (24)
      GO TO (1500,1570,1640,1680,830,1680,1730,1740,1750,850,850,1760,18
     120,1860,1870,1880,1890,850,1980,1980,2020,2070,2090,850,850,850,21
     270,2180,2200,850,850,2220,2230,2240,2245,2250,2255), VERB
C           TAKE DROP  SAY OPEN NOTH LOCK   ON  OFF WAVE CALM
C           WALK KILL POUR  EAT DRNK  RUB TOSS QUIT FIND INVN
C           FEED FILL BLST SCOR  FOO  BRF READ BREK WAKE SUSP
C           HOUR TEST  CUT  TIE UNTI  RIG CLIK

C  Analyse an object word.  See if the thing is here, whether we've got
C  it yet, and so on.  Object must be here unless verb is FIND or
C  INVENTORY (and ne new verb yet to be analyzed).  WAIER and OIL are
C  also funny.  They are never acutally dropped at any location, but
C  might be here in the bottle or as a feature of the location.

1100  OBJ=K
      IF (FIXED(K).NE.LOC.AND..NOT.HERE(K)) GO TO 1130
1110  IF (WD2.NE.'     ') GO TO 1060
      IF (VERB.NE.0) GO TO 1090
       if (wd1.eq.'persi'.or.wd1.eq.'ming'.or.wd1.eq.'spelu')
     1 wd1x(1:1) = upperc(wd1x(1:1))
      PRINT 1120, 'What do you want to do with the ' // TRIM(WD1X) // '?'
1120  FORMAT (/,' ',A)
      mltcmd = .false.
      GO TO 870

1130  IF (K.NE.GRATE) GO TO 1140
      IF (LOC.EQ.1.OR.LOC.EQ.4.OR.LOC.EQ.7) K=DPRSSN
      IF (LOC.GT.9.AND.LOC.LT.15) K=ENTRNC
      IF (K.NE.GRATE) GO TO 1210
1140  IF (K.NE.DWARF) GO TO 1160
      DO 1150 I=1,5
        IF (DLOC(I).EQ.LOC.AND.DFLAG.GE.2) GO TO 1110
1150    CONTINUE
1160  IF ((LIQ().EQ.K.AND.HERE(BOTTLE)).OR.K.EQ.LIQLOC(LOC)) GO TO 1110
      IF (OBJ.EQ.SNAKE.AND.AT(SNAKE2).AND.PROP(SNAKE).EQ.0) GO TO 1110
      IF (OBJ.NE.PLANT.OR..NOT.AT(PLANT2).OR.PROP(PLANT2).EQ.0) GO TO 11
     170
      OBJ=PLANT2
      GO TO 1110
1170  IF (OBJ.NE.KNIFE.OR.KNFLOC.NE.LOC) GO TO 1180
      KNFLOC=-1
      SPK=116
      GO TO 850
1180  IF (OBJ.NE.ROD.OR..NOT.HERE(ROD2)) GO TO 1185
      OBJ=ROD2
      GO TO 1110
1185  IF (OBJ.NE.ROPE.OR..NOT.HERE(ROPE2)) GO TO 1190
      OBJ = ROPE2
      GO TO 1110
1190  IF((VERB.EQ.FIND.OR.VERB.EQ.INVENT).AND.WD2.EQ.'     ') GO TO 1110
       if (wd1.eq.'persi'.or.wd1.eq.'ming'.or.wd1.eq.'spelu')
     1 wd1x(1:1) = upperc(wd1x(1:1))
      PRINT 1120, 'I see no ' // TRIM(WD1X) // ' here.'
      mltcmd = .false.
      GO TO 860

C  Figure out the new location.

C  Given the current location in LOC, and a motion verb number in K, put
C  the new location in NEWLOC.  The current LOC is saved in OLDLOC in
C  case he wants to retreat.  The current OLDLOC is saved in OLDLC2, in
C  case he dies.  (If he does, NEWLOC will be in limbo, and OLDLOC
C  will be what killed him, so we need OLDLC2, which is the last place
C  he was safe.)

1210  KK=KEY(LOC)
      NEWLOC=LOC
      IF (KK.EQ.0) CALL BUG (26)
      IF (K.EQ.NULL) GO TO 510
      IF (K.EQ.BACK) GO TO 1340
      IF (K.EQ.LOOK) GO TO 1390
      IF (K.EQ.CAVE) GO TO 1400
      OLDLC2=OLDLOC
      OLDLOC=LOC

1220  LL=IABS(TRAVEL(KK))
      IF (MOD(LL,1000).EQ.1.OR.MOD(LL,1000).EQ.K) GO TO 1230

      IF (TRAVEL(KK).LT.0) GO TO 1410
      KK=KK+1
      GO TO 1220

1230  LL=LL/1000
1240  NEWLOC=LL/1000
      K=MOD(NEWLOC,100)
      IF (NEWLOC.LE.300) GO TO 1260
      IF (PROP(K).NE.NEWLOC/100-3) GO TO 1280
1250  IF (TRAVEL(KK).LT.0) CALL BUG (25)
      KK=KK+1
      NEWLOC=IABS(TRAVEL(KK))/1000
      IF (NEWLOC.EQ.LL) GO TO 1250
      LL=NEWLOC
      GO TO 1240

1260  IF (NEWLOC.LE.100) GO TO 1270
      IF (TOTING(K).OR.(NEWLOC.GT.200.AND.AT(K))) GO TO 1280

      GO TO 1250

1270  IF (NEWLOC.NE.0.AND..NOT.PCT(NEWLOC)) GO TO 1250

1280  NEWLOC=MOD(LL,1000)
      IF (NEWLOC.LE.300) GO TO 510
      IF (NEWLOC.LE.500) GO TO 1290
      CALL RSPEAK (NEWLOC-500)
      NEWLOC=LOC
      GO TO 510

C  Special motions come here.

1290  NEWLOC=NEWLOC-300
      IF (NEWLOC.GT.4) CALL BUG (20)
      GO TO (1300,1310,1320,1335), NEWLOC

C  Travel 301.  Plover - alcove passage.  Can carry only emerald.
C  Note:  TRAVEL table must include useless entries going through
C  passage, which cannot be used for actual motion, but can be spotted
C  by "GO BACK".

1300  NEWLOC=99+100-LOC
      IF (HOLDNG.EQ.0.OR.(HOLDNG.EQ.1.AND.TOTING(EMRALD))) GO TO 510

      NEWLOC=LOC
      CALL RSPEAK (117)
      GO TO 510

C  Travel 302.  Player transport.  Drop the emerald (only use spaciel
C  TRAVEL if toting it), so he's forced to use the Plover-passage to get
C  it out.  Having dropped it, go back and pretend he wasn't carrying
C  it after all.

1310  CALL DROP (EMRALD,LOC)
      GO TO 1250

C  Travel 303.  Troll bridge.  Must be done only as special motion so
C  that dwarves won't wander across and encounter the bear.  (They won't
C  follow player there because that region is forbidden to the pirate.)
C  If PROP(TROLL)=1, he's crossed since paying, so step out and block
C  him.  (Standard TRAVEL entries check for PROP(TROLL)=0.)  Special
C  stuff for bear.

1320  IF (PROP(TROLL).NE.1) GO TO 1330
      CALL PSPEAK (TROLL,1)
      PROP(TROLL)=0
      CALL DSTROY (TROLL2)
      CALL DSTROY (TROLL2+100)
      CALL MOVE (TROLL,PLAC(TROLL))
      CALL MOVE (TROLL+100,FIXD(TROLL))
      CALL JUGGLE (CHASM)
      NEWLOC=LOC
      GO TO 510

1330  NEWLOC=PLAC(TROLL)+FIXD(TROLL)-LOC
      IF (PROP(TROLL).EQ.0) PROP(TROLL)=1
      IF (.NOT.TOTING(BEAR)) GO TO 510
      CALL RSPEAK (162)
      PROP(CHASM)=1
      PROP(TROLL)=2
      CALL DROP (BEAR,NEWLOC)
      FIXED(BEAR)=-1
      PROP(BEAR)=3
      IF (PROP(SPICES).LT.0) TALLY2=TALLY2+1
      OLDLC2=NEWLOC
      GO TO 1430

C  Travel 304.  Down an anchored rope.

1335  IF ((HERE(ROPE).AND.(IAND(PROP(ROPE),1).EQ.1)).OR.
     1   (HERE(ROPE2).AND.(PROP(ROPE2).EQ.3))) GO TO 1336
      CALL RSPEAK (9)
      NEWLOC = LOC
      GO TO 510

1336  IF (LOC.EQ.63)  NEWLOC = 141
      IF (LOC.EQ.149) NEWLOC = 150
      IF (LOC.EQ.146) NEWLOC = 147
      IF (.NOT.(LOC.EQ.146.AND.PROP(ROPE).EQ.5)) GO TO 510

      CALL RSPEAK (208)
      PROP(ROPE) = 3
      GO TO 1430

C  End of specials.

C  Handle "GO BACK".  Look for verb which goes from LOC to OLDLOC, or to
C  OLDLC2 if OLDLOC has forced-motion.  K2 saves entry -> forced loc
C  -> previous loc.

1340  K=OLDLOC
      IF (FORCED(K)) K=OLDLC2
      OLDLC2=OLDLOC
      OLDLOC=LOC
      K2=0
      IF (K.NE.LOC) GO TO 1350
      CALL RSPEAK (91)
      GO TO 510

1350  LL=MOD((IABS(TRAVEL(KK))/1000),1000)
      IF (LL.EQ.K) GO TO 1380
      IF (LL.GT.300) GO TO 1360
      J=KEY(LL)
      IF (FORCED(LL).AND.MOD((IABS(TRAVEL(J))/1000),1000).EQ.K) K2=KK
1360  IF (TRAVEL(KK).LT.0) GO TO 1370
      KK=KK+1
      GO TO 1350

1370  KK=K2
      IF (KK.NE.0) GO TO 1380
      CALL RSPEAK (140)
      GO TO 510

1380  K=MOD(IABS(TRAVEL(KK)),1000)
      KK=KEY(LOC)
      GO TO 1220

C  Look.  Can't give more detail.  Pretend it wasn't dark (though it may
C  now be dark) so he won't fall into a pit while staring into the room.

1390  IF (DETAIL.LT.3) CALL RSPEAK (15)
      DETAIL=DETAIL+1
      WZDARK=.FALSE.
      ABB(LOC)=0
      GO TO 510

C  Cave.  Different messages depending on whether above ground..

1400  IF (LOC.LT.8) CALL RSPEAK (57)
      IF (LOC.GE.8) CALL RSPEAK (58)
      GO TO 510

C  Non-applicable motion.  Various messages depending on word given.

1410  SPK=12
      IF (K.GE.43.AND.K.LE.50) SPK=9
      IF (K.EQ.29.OR.K.EQ.30) SPK=9
      IF (K.EQ.7.OR.K.EQ.36.OR.K.EQ.37) SPK=10
      IF (K.EQ.11.OR.K.EQ.19) SPK=11
      IF (VERB.EQ.FIND.OR.VERB.EQ.INVENT) SPK=59
      IF (K.EQ.62.OR.K.EQ.65) SPK=42
      IF (K.EQ.17) SPK=80
      CALL RSPEAK (SPK)
      GO TO 510

C  *****  You're dead, Jim.

C  If the current LOC is zero, it means the clown got himself killed.
C  We'll allow this MAXDIE times.  MAXDIE is automatically set base on
C  the number of snide messages available.  Each death results in a
C  message (81, 83, etc.) which offers reincarnation:  if accepted, this
C  results in message 82, 84, etc.  The last time, if he wants another
C  chance, he gets a snide remark and we exit.  When reincarnated, all
C  objects being carried get dropped at OLDLC2 (presumably the last
C  place prior to being killed) without change of PROPs.  The loop runs
C  backwards to assure that the bird is dropped before the cage.  (This
C  kluge could be changed once we're sure all references to BIRD and
C  CAGE are done by keywords.)  The lamp is a special case (it wouldn't
C  do to leave it in the cave).  It is turned off and left outside the
C  building (only if he was carrying it, of course).  He himself is left
C  left inside the building (and Heaven help him if he tries to XYZZY
C  back into the cave without the lamp).  OLDLOC is zapped so he can't
C  just RETREAT.

C  The fastest way to get killed is to fall into a pit in pitch darkness

1420  CALL RSPEAK (23)
      OLDLC2=LOC

C  Okay, he's dead.  Let's get on with it.

1430  IF (CLOSNG) GO TO 1450
      YEA=YES(81+NUMDIE*2,82+NUMDIE*2,54)
      NUMDIE=NUMDIE+1
      IF (NUMDIE.EQ.MAXDIE.OR..NOT.YEA) GO TO 2360
      PLACE(WATER)=0
      PLACE(OIL)=0
      IF (TOTING(LAMP)) PROP(LAMP)=0
      DO 1440 J=1,100
        I=101-J
        IF (.NOT.TOTING(I)) GO TO 1440
        K=OLDLC2
        IF (I.EQ.LAMP) K=1
        CALL DROP (I,K)
1440    CONTINUE
      LOC=3
      OLDLOC=LOC
      GO TO 780

C  He died during closing time.  No resurrection.  Tally up a death
C  and exit.

1450  CALL RSPEAK (131)
      NUMDIE=NUMDIE+1
      GO TO 2360

C  Routines for performing the various action verbs.


C  Random intransitive verbs come here.  Clear OBJ just in case (see
C  ATTACK).

1460  WD1X(1:1) = upperc (WD1X(1:1))
      PRINT 1120, TRIM(WD1X) // ' what?'
      mltcmd = .false.
      OBJ=0
      GO TO 870

C  Carry, no object given yet.  Ok if only one object present.

1480  KLUGE=ATLOC(LOC)
      IF (KLUGE.EQ.0) GO TO 1460
      IF (LINK(KLUGE).NE.0) GO TO 1460
      DO 1490 I=1,5
        IF (DLOC(I).EQ.LOC.AND.DFLAG.GE.2) GO TO 1460
1490    CONTINUE
      OBJ=ATLOC(LOC)

C  Carry an object.  Special cases for bird and cage (if bird in cage,
C  can't take one without the other).  Liquids also special, since they
C  depend on status of bottle.  Also various side effects, etc.

1500  IF (OBJ.EQ.ROD.AND.TOTING(ROD).AND.HERE(ROD2)) OBJ = ROD2
      IF (OBJ.EQ.ROPE.AND.(TOTING(ROPE).OR.(IAND(PROP(ROPE),1))
     1.EQ.1).AND.HERE(ROPE2)) OBJ = ROPE2
      IF (TOTING(OBJ)) GO TO 850
      SPK=25
      IF (OBJ.EQ.PLANT.AND.PROP(PLANT).LE.0) SPK=115
      IF (OBJ.EQ.BEAR.AND.PROP(BEAR).EQ.1) SPK=169
      IF (OBJ.EQ.CHAIN.AND.PROP(BEAR).NE.0) SPK=170
      IF (FIXED(OBJ).NE.0) GO TO 850
      if (dark()) then
          spk = 202
          goto 850
      end if
      IF (OBJ.NE.WATER.AND.OBJ.NE.OIL) GO TO 1520
      IF (HERE(BOTTLE).AND.LIQ().EQ.OBJ) GO TO 1510
      OBJ=BOTTLE
      IF (TOTING(BOTTLE).AND.PROP(BOTTLE).EQ.1) GO TO 2070
      IF (PROP(BOTTLE).NE.1) SPK=105
      IF (.NOT.TOTING(BOTTLE)) SPK=104
      GO TO 850
1510  OBJ=BOTTLE
1520  IF (HOLDNG.LT.8) GO TO 1530
      CALL RSPEAK (92)
      GO TO 860
1530  IF (OBJ.NE.BIRD) GO TO 1560
      IF (PROP(BIRD).NE.0) GO TO 1560
      IF (.NOT.TOTING(ROD)) GO TO 1540
      CALL RSPEAK (26)
      GO TO 860
1540  IF (TOTING(CAGE)) GO TO 1550
      CALL RSPEAK (27)
      GO TO 860
1550  PROP(BIRD)=1
1560  IF ((OBJ.EQ.BIRD.OR.OBJ.EQ.CAGE).AND.PROP(BIRD).NE.0) CALL CARRY (
     1BIRD+CAGE-OBJ,LOC)
      CALL CARRY (OBJ,LOC)
      K=LIQ()
      IF (OBJ.EQ.BOTTLE.AND.K.NE.0) PLACE(K)=-1
      IF (OBJ.NE.RUBY) GO TO 830
      SPK = 211
      GO TO 850

C  Discard object.  THROW also comes here for most objects.  Special
C  cases for bird (might attack snake or dragon), cage (might contain
C  bird), and DROP COINS at vending machine for extra batteries.

1570  IF (TOTING(ROD2).AND.OBJ.EQ.ROD.AND..NOT.TOTING(ROD)) OBJ=ROD2
      IF (TOTING(ROPE2).AND.OBJ.EQ.ROPE.AND..NOT.TOTING(ROPE))
     1OBJ = ROPE2
      IF (.NOT.TOTING(OBJ)) GO TO 850
      IF (OBJ.NE.BIRD.OR..NOT.(AT(SNAKE).OR.AT(SNAKE2))) GO TO 1590
      CALL RSPEAK (30)
      IF (CLOSED) GO TO 2350
      CALL DSTROY (SNAKE)
      CALL DSTROY (SNAKE2)
      DO 1572 K=1,100
        IF (PLACE(K).EQ.PLAC(SNAKE) .OR.PLACE(K).EQ.FIXED(SNAKE) .OR.
     1      PLACE(K).EQ.PLAC(SNAKE2).OR.PLACE(K).EQ.FIXED(SNAKE2))
     2      CALL MOVE(K,19)
1572    CONTINUE
      LOC = 19

C  Set PROP for use by TRAVEL options.

      PROP(SNAKE)=1
1580  K=LIQ()
      IF (K.EQ.OBJ) OBJ=BOTTLE
      IF (OBJ.EQ.BOTTLE.AND.K.NE.0) PLACE(K)=0
      IF (OBJ.EQ.CAGE.AND.PROP(BIRD).NE.0) CALL DROP (BIRD,LOC)
      IF (OBJ.EQ.BIRD) PROP(BIRD)=0
      CALL DROP (OBJ,LOC)
      GO TO 860

1590  IF (OBJ.NE.COINS.OR..NOT.HERE(VEND).OR.PROP(VEND).NE.0) GO TO 1600
      CALL DSTROY (COINS)
      CALL DROP (BATTER,LOC)
      CALL PSPEAK (BATTER,0)
      GO TO 860

1600  IF (OBJ.NE.BIRD.OR..NOT.AT(DRAGON).OR.PROP(DRAGON).NE.0) GO TO 161
     10
      CALL RSPEAK (154)
      CALL DSTROY (BIRD)
      PROP(BIRD)=0
      IF (PLACE(SNAKE).EQ.PLAC(SNAKE)) TALLY2=TALLY2+1
      GO TO 860

1610  IF (OBJ.NE.BEAR.OR..NOT.AT(TROLL)) GO TO 1620
      CALL RSPEAK (163)
      CALL DSTROY (TROLL)
      CALL DSTROY (TROLL+100)
      CALL MOVE (TROLL2,PLAC(TROLL))
      CALL MOVE (TROLL2+100,FIXD(TROLL))
      CALL JUGGLE (CHASM)
      PROP(TROLL)=2
      GO TO 1580
1620  IF (OBJ.EQ.VASE.AND.LOC.NE.PLAC(PILLOW)) GO TO 1630
      CALL RSPEAK (54)
      GO TO 1580

1630  PROP(VASE)=2
      IF (AT(PILLOW)) PROP(VASE)=0
      CALL PSPEAK (VASE,PROP(VASE)+1)
      IF (PROP(VASE).NE.0) FIXED(VASE)=-1
      GO TO 1580

C  Say.  Echo WD2 (or WD1 if no WD2 (Say what?, etc.).)  Magic words
C  override.

1640  IF (WD2 .NE. '     ') WD1 = WD2
      I = VOCAB(WD1,-1)
      IF (I.EQ.62.OR.I.EQ.65.OR.I.EQ.71.OR.I.EQ.2025) GO TO 1660
      if (WD2 .EQ. '     ') then
        PRINT 1120, 'Okay, "' // TRIM(WD1X) // '"'
      else
        PRINT 1120, 'Okay, "' // TRIM(WD2X) // '"'
      endif
      GO TO 860

1660  WD2='     '
      OBJ=0
      GO TO 1050

C  Lock, unlock, no object given.  Assume various things if present.

1670  SPK=28
      IF (HERE(CLAM)) OBJ=CLAM
      IF (HERE(OYSTER)) OBJ=OYSTER
      IF (AT(DOOR)) OBJ=DOOR
      IF (AT(GRATE)) OBJ=GRATE
      IF (OBJ.NE.0.AND.HERE(CHAIN)) GO TO 1460
      IF (HERE(CHAIN)) OBJ=CHAIN
      IF (OBJ.EQ.0) GO TO 850

C  Lock, unlock object.  Special stuff for opening clam/oyster and for
C  chain.

1680  IF (OBJ.EQ.CLAM.OR.OBJ.EQ.OYSTER) GO TO 1700
      IF (OBJ.EQ.DOOR) SPK=111
      IF (OBJ.EQ.DOOR.AND.PROP(DOOR).EQ.1) SPK=54
      IF (OBJ.EQ.CAGE) SPK=32
      IF (OBJ.EQ.KEYS) SPK=55
      IF (OBJ.EQ.GRATE.OR.OBJ.EQ.CHAIN) SPK=31
      IF (SPK.NE.31.OR..NOT.HERE(KEYS)) GO TO 850
      IF (OBJ.EQ.CHAIN) GO TO 1710
      IF (.NOT.CLOSNG) GO TO 1690
      K=130
      IF (.NOT.PANIC) CLOCK2=15
      PANIC=.TRUE.
      GO TO 840

1690  K=34+PROP(GRATE)
      PROP(GRATE)=1
      IF (VERB.EQ.LOCK) PROP(GRATE)=0
      K=K+2*PROP(GRATE)
      GO TO 840

C  Clam/oyster.

1700  K=0
      IF (OBJ.EQ.OYSTER) K=1
      SPK=124+K
      IF (TOTING(OBJ)) SPK=120+K
      IF (.NOT.TOTING(TRIDNT)) SPK=122+K
      IF (VERB.EQ.LOCK) SPK=61
      IF (SPK.NE.124) GO TO 850
      CALL DSTROY (CLAM)
      CALL DROP (OYSTER,LOC)
      CALL DROP (PEARL,105)
      GO TO 850

C  Chain.

1710  IF (VERB.EQ.LOCK) GO TO 1720
      SPK=171
      IF (PROP(BEAR).EQ.0) SPK=41
      IF (PROP(CHAIN).EQ.0) SPK=37
      IF (SPK.NE.171) GO TO 850
      PROP(CHAIN)=0
      FIXED(CHAIN)=0
      IF (PROP(BEAR).NE.3) PROP(BEAR)=2
      FIXED(BEAR)=2-PROP(BEAR)
      GO TO 850

1720  SPK=172
      IF (PROP(CHAIN).NE.0) SPK=34
      IF (LOC.NE.PLAC(CHAIN)) SPK=173
      IF (SPK.NE.172) GO TO 850
      PROP(CHAIN)=2
      IF (TOTING(CHAIN)) CALL DROP (CHAIN,LOC)
      FIXED(CHAIN)=-1
      GO TO 850

C  Light lamp

1730  IF (.NOT.HERE(LAMP)) GO TO 850
      SPK=184
      IF (LIMIT.LT.0) GO TO 850
      PROP(LAMP)=1
      CALL RSPEAK (39)
      IF (WZDARK) GO TO 780
      GO TO 860

C  Lamp off

1740  IF (.NOT.HERE(LAMP)) GO TO 850
      PROP(LAMP)=0
      CALL RSPEAK (40)
      IF (DARK()) CALL RSPEAK (16)
      IF (DARK().AND.TOTING(RUBY)) CALL RSPEAK (212)
      GO TO 860

C  Wave.  No effect unless waving rod at fissure.

1750  IF ((.NOT.TOTING(OBJ)).AND.(OBJ.NE.ROD.OR..NOT.TOTING(ROD2)))
     1SPK = 29
      IF (OBJ.NE.ROD.OR..NOT.AT(FISSUR).OR..NOT.TOTING(OBJ).OR.CLOSNG) G
     1O TO 850
      PROP(FISSUR)=1-PROP(FISSUR)
      CALL PSPEAK (FISSUR,2-PROP(FISSUR))
      GO TO 860

C  Attack.  Assume target if unambiguous.  THROW also links here.
C  Objects fall into two categories: enemies (snake, dwarf, etc.) and
C  others (bird, clam).  Ambiguous if two enemies, or if no enemies but
C  two others.

1760  DO 1770 I=1,5
        IF (DLOC(I).EQ.LOC.AND.DFLAG.GE.2) GO TO 1780
1770    CONTINUE
      I=0
1780  IF (OBJ.NE.0) GO TO 1790
      IF (I.NE.0) OBJ=DWARF
      IF (AT(SNAKE).OR.AT(SNAKE2)) OBJ=OBJ*100+SNAKE
      IF (AT(DRAGON).AND.PROP(DRAGON).EQ.0) OBJ=OBJ*100+DRAGON
      IF (AT(TROLL)) OBJ=OBJ*100+TROLL
      IF (HERE(BEAR).AND.PROP(BEAR).EQ.0) OBJ=OBJ*100+BEAR
      IF (OBJ.GT.100) GO TO 1460
      IF (OBJ.NE.0) GO TO 1790

C  Can't attack bird by throwing axe.

      IF (HERE(BIRD).AND.VERB.NE.THROW) OBJ=BIRD

C  Clam and oyster both treated as clam for intransitive case:  no harm
C  done.

      IF (HERE(CLAM).OR.HERE(OYSTER)) OBJ=100*OBJ+CLAM
      IF (OBJ.GT.100) GO TO 1460
1790  IF (OBJ.NE.BIRD) GO TO 1800
      SPK=137
      IF (CLOSED) GO TO 850
      CALL DSTROY (BIRD)
      PROP(BIRD)=0
      IF (PLACE(SNAKE).EQ.PLAC(SNAKE)) TALLY2=TALLY2+1
      SPK=45
1800  IF (OBJ.EQ.0) SPK=44
      IF (OBJ.EQ.CLAM.OR.OBJ.EQ.OYSTER) SPK=150
      IF (OBJ.EQ.SNAKE) SPK=46
      IF (OBJ.EQ.DWARF) SPK=49
      IF (OBJ.EQ.DWARF.AND.CLOSED) GO TO 2350
      IF (OBJ.EQ.DRAGON) SPK=167
      IF (OBJ.EQ.TROLL) SPK=157
      IF (OBJ.EQ.BEAR) SPK=165+(PROP(BEAR)+1)/2
      IF (OBJ.NE.DRAGON.OR.PROP(DRAGON).NE.0) GO TO 850

C  Fun stuff for dragon.  If he insists on attacking it, win.  Set PROP
C  to dead.  Move dragon to central loc (still fixed), move rug
C  there (not fixed), and move him there too.
C  Then do a null motion to get new description.

      CALL RSPEAK (49)
      VERB=0
      OBJ=0
      CALL GETIN (WD1,WD1X,WD2,WD2X)
      IF (WD1.NE.'y'.AND.WD1.NE.'yes') GO TO 1020
      CALL PSPEAK (DRAGON,1)
      PROP(DRAGON)=2
      PROP(RUG)=0
      K=(PLAC(DRAGON)+FIXD(DRAGON))/2
      CALL MOVE (DRAGON+100,-1)
      CALL DSTROY (RUG+100)
      CALL MOVE (DRAGON,K)
      CALL MOVE (RUG,K)
      DO 1810 OBJ=1,100
        IF (PLACE(OBJ).EQ.PLAC(DRAGON).OR.PLACE(OBJ).EQ.FIXD(DRAGON)) CA
     1LL MOVE (OBJ,K)
1810    CONTINUE
      LOC=K
      K=NULL
      GO TO 1210

C  Pour.  If no object, or object is bottle, assume contents of bottle.
C  Special tests for pouring water or oil on plant or rusty door.

1820  IF (OBJ.EQ.BOTTLE.OR.OBJ.EQ.0) OBJ=LIQ()
      IF (OBJ.EQ.0) GO TO 1460
      IF (.NOT.TOTING(OBJ)) GO TO 850
      SPK=78
      IF (OBJ.NE.OIL.AND.OBJ.NE.WATER) GO TO 850
      PROP(BOTTLE)=1
      PLACE(OBJ)=0
      SPK=77
      IF (.NOT.(AT(PLANT).OR.AT(DOOR))) GO TO 850

      IF (AT(DOOR)) GO TO 1830
      SPK=112
      IF (OBJ.NE.WATER) GO TO 850
      CALL PSPEAK (PLANT,PROP(PLANT)+1)
      PROP(PLANT)=MOD(PROP(PLANT)+2,6)
      PROP(PLANT2)=PROP(PLANT)/2
      K=NULL
      GO TO 1210

1830  PROP(DOOR)=0
      IF (OBJ.EQ.OIL) PROP(DOOR)=1
      SPK=113+PROP(DOOR)
      GO TO 850

C  Eat.  Intransitive: assume food if present, else ask what.
C  Transitive: ok, some things lose appetite, rest are ridiculous.

1840  IF (.NOT.HERE(FOOD)) GO TO 1460
1850  CALL DSTROY (FOOD)
      SPK=72
      GO TO 850

1860  IF (OBJ.EQ.FOOD) GO TO 1850
      IF (OBJ.EQ.BIRD.OR.OBJ.EQ.SNAKE.OR.OBJ.EQ.CLAM.OR.OBJ.EQ.OYSTER.OR
     1.OBJ.EQ.DWARF.OR.OBJ.EQ.DRAGON.OR.OBJ.EQ.TROLL.OR.OBJ.EQ.BEAR)
     2SPK = 71
      GO TO 850

C  Drink.  If no object, assume water and look for it here.  If water is
C  in the bottle, drink that, else must be at a water loc, so drink
C  stream.

1870  IF (OBJ.EQ.0.AND.LIQLOC(LOC).NE.WATER.AND.(LIQ().NE.WATER.OR..NOT.
     1HERE(BOTTLE))) GO TO 1460
      IF (OBJ.NE.0.AND.OBJ.NE.WATER) SPK=110
      IF (SPK.EQ.110.OR.LIQ().NE.WATER.OR..NOT.HERE(BOTTLE)) GO TO 850
      PROP(BOTTLE)=1
      PLACE(WATER)=0
      SPK=74
      GO TO 850

C  Rub.  Yields various snide remarks.

1880  IF (OBJ.NE.LAMP) SPK=76
      GO TO 850

C  Throw.  Same as DISCARD unless axe.  Then same as ATTACK except
C  ignore bird, and if dwarf is present then one might be killed (only
C  way to do so).  Axe also special for dragon, bear, and troll.
C  Treasures special for troll.

1890  IF (TOTING(ROD2).AND.OBJ.EQ.ROD.AND..NOT.TOTING(ROD)) OBJ=ROD2
      IF (TOTING(ROPE2).AND.OBJ.EQ.ROPE.AND..NOT.TOTING(ROPE))
     1OBJ = ROPE2
      IF (.NOT.TOTING(OBJ)) GO TO 850
      IF (OBJ.GE.50.AND.OBJ.LE.MAXTRS.AND.AT(TROLL)) GO TO 1950
      IF (OBJ.EQ.FOOD.AND.HERE(BEAR)) GO TO 1940
      IF (OBJ.NE.AXE) GO TO 1570
      DO 1900 I=1,5

C  Needn't check DFLAG if axe is here.

        IF (DLOC(I).EQ.LOC) GO TO 1910
1900    CONTINUE
      SPK=152
      IF (AT(DRAGON).AND.PROP(DRAGON).EQ.0) GO TO 1920
      SPK=158
      IF (AT(TROLL)) GO TO 1920
      IF (HERE(BEAR).AND.PROP(BEAR).EQ.0) GO TO 1930
      OBJ=0
      GO TO 1760

1910  SPK=48

C  If SAVED not = -1, he bypassed the START call.ALL.

      IF (RAN(3).EQ.0.OR.SAVED.NE.-1) GO TO 1920
      DSEEN(I)=.FALSE.
      DLOC(I)=0
      SPK=47
      DKILL=DKILL+1
      IF (DKILL.EQ.1) SPK=149
1920  CALL RSPEAK (SPK)
      CALL DROP (AXE,LOC)
      K=NULL
      GO TO 1210

C  That'll teach him to throw the axe at the bear!

1930  SPK=164
      CALL DROP (AXE,LOC)
      FIXED(AXE)=-1
      PROP(AXE)=1
      CALL JUGGLE (BEAR)
      GO TO 850

C  But throwing food is another story.

1940  OBJ=BEAR
      GO TO 2020

1950  SPK=159

C  Snarf a treausre for the troll.

      CALL DROP (OBJ,0)
      CALL DSTROY (TROLL)
      CALL DSTROY (TROLL+100)
      CALL DROP (TROLL2,PLAC(TROLL))
      CALL DROP (TROLL2+100,FIXD(TROLL))
      CALL JUGGLE (CHASM)
      GO TO 850

C  Quit.  Intransitive only.  Verify intent and exit if that's what
C  he wants to do.

1960  GAVEUP=YES(22,54,54)
1970  IF (GAVEUP) GO TO 2360
      GO TO 860

C  Find.  Might be carrying it, or it might be here.  Else give caveat.

1980  IF (AT(OBJ).OR.(LIQ().EQ.OBJ.AND.AT(BOTTLE)).OR.K.EQ.LIQLOC(LOC))
     1 SPK=94
      DO 1990 I=1,5
1990    IF (DLOC(I).EQ.LOC.AND.DFLAG.GE.2.AND.OBJ.EQ.DWARF)
     1  SPK=94
      IF (CLOSED) SPK=138
      IF (TOTING(OBJ)) SPK=24
      GO TO 850

C  Inventory.  If object, treat same as FIND.  Else report on current
C  burden.

2000  SPK=98
      DO 2010 I=1,100
        if (I.NE.BEAR.AND.TOTING(I)) then
          if (SPK.EQ.98) then
            CALL RSPEAK (99)
            BLKLIN=.FALSE.
            SPK=0
          end if
          CALL PSPEAK (I,-1)
        end if
2010    CONTINUE
      BLKLIN=.TRUE.
      IF (TOTING(BEAR)) SPK=141
      GO TO 850

C  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dward, make
C  dwarf mad.  Bear: special.

2020  IF (OBJ.NE.BIRD) GO TO 2030
      SPK=100
      GO TO 850

2030  IF (OBJ.NE.SNAKE.AND.OBJ.NE.DRAGON.AND.OBJ.NE.TROLL) GO TO 2040
      SPK=102
      IF (OBJ.EQ.DRAGON.AND.PROP(DRAGON).NE.0) SPK=110
      IF (OBJ.EQ.TROLL) SPK=182
      IF (OBJ.NE.SNAKE.OR.CLOSED.OR..NOT.HERE(BIRD)) GO TO 850
      SPK=101
      CALL DSTROY (BIRD)
      PROP(BIRD)=0
      TALLY2=TALLY2+1
      GO TO 850

2040  IF (OBJ.NE.DWARF) GO TO 2050
      IF (.NOT.HERE(FOOD)) GO TO 850
      SPK=103
      DFLAG=DFLAG+1
      GO TO 850

2050  IF (OBJ.NE.BEAR) GO TO 2060
      IF (PROP(BEAR).EQ.0) SPK=102
      IF (PROP(BEAR).EQ.3) SPK=110
      IF (.NOT.HERE(FOOD)) GO TO 850
      CALL DSTROY (FOOD)
      PROP(BEAR)=1
      FIXED(AXE)=0
      PROP(AXE)=0
      SPK=168
      GO TO 850

2060  SPK=14
      GO TO 850

C  Fill.  Bottle must be empty, and some liquid available.  (Vase is
C  nasty.

2070  IF (OBJ.EQ.VASE) GO TO 2080
      IF (OBJ.NE.0.AND.OBJ.NE.BOTTLE) GO TO 850
      IF (OBJ.EQ.0.AND..NOT.HERE(BOTTLE)) GO TO 1460
      SPK=107
      IF (LIQLOC(LOC).EQ.0) SPK=106
      IF (LIQ().NE.0) SPK=105
      IF (SPK.NE.107) GO TO 850
      PROP(BOTTLE)=MOD(COND(LOC),4)/2*2
      K=LIQ()
      IF (TOTING(BOTTLE)) PLACE(K)=-1
      IF (K.EQ.OIL) SPK=108
      GO TO 850

2080  SPK=29
      IF (LIQLOC(LOC).EQ.0) SPK=144
      IF (LIQLOC(LOC).EQ.0.OR..NOT.TOTING(VASE)) GO TO 850
      CALL RSPEAK (145)
      PROP(VASE)=2
      FIXED(VASE)=-1
      GO TO 1590

C  Blast.  No effect unless he's got dynamite, which is a neat trick.

2090  IF (PROP(ROD2).LT.0.OR..NOT.CLOSED) GO TO 850
      BONUS=133
      IF (LOC.EQ.115) BONUS=134
      IF (HERE(ROD2)) BONUS=135
      CALL RSPEAK (BONUS)
      GO TO 2360

C  Score.  Go to socring section.

2100  SCORNG=.TRUE.
      GO TO 2360

2110  SCORNG=.FALSE.
      PRINT 2120, TURNS,SCORE,MXSCOR
2120  FORMAT (/' If you were to QUIT now (after',I5,' moves), you would
     1 score',I4,/,' out of a possible',I4,' points.')
      GO TO 860

C  FEE FIE FOE FOO (and FUM).  Advance to next state if given in peoper
C  order.  Look up WD1 in Section 3 of vocabulary to determine which
C  word we've got.  Last word zips the eggs back to the Giant room
C  (unless already there).

2130  K=VOCAB(WD1,3)
      SPK=42
      IF (FOOBAR.EQ.1-K) GO TO 2140
      IF (FOOBAR.NE.0) SPK=151
      GO TO 850

2140  FOOBAR=K
      IF (K.NE.4) GO TO 830
      FOOBAR=0
      IF (PLACE(EGGS).EQ.PLAC(EGGS).OR.(TOTING(EGGS).AND.LOC.EQ.PLAC(EGG
     1S))) GO TO 850

C  Bring back troll if we steal the eggs back from him before crossing.

      IF (PLACE(EGGS).EQ.0.AND.PLACE(TROLL).EQ.0.AND.PROP(TROLL).EQ.0) P
     1ROP(TROLL)=1
      K=2
      IF (HERE(EGGS)) K=1
      IF (LOC.EQ.PLAC(EGGS)) K=0
      CALL MOVE (EGGS,PLAC(EGGS))
      CALL PSPEAK (EGGS,K)
      GO TO 860

C  Brief.  Intransitive only.  Suppress long descriptions after first
C  time.

2150  SPK=156
      ABBNUM=10000
      DETAIL=3
      GO TO 850

C  Read.  Magazines in Dwarvish, message we've seen, and ...oyster?

2160  IF (HERE(MAGZIN)) OBJ=MAGZIN
      IF (HERE(TABLET)) OBJ=OBJ*100+TABLET
      IF (HERE(MESSAG)) OBJ=OBJ*100+MESSAG
      IF (CLOSED.AND.TOTING(OYSTER)) OBJ=OYSTER
      IF (OBJ.GT.100.OR.OBJ.EQ.0.OR.DARK()) GO TO 1460

2170  IF (DARK()) GO TO 1190
      IF (OBJ.EQ.MAGZIN) SPK=190
      IF (OBJ.EQ.TABLET) SPK=196
      IF (OBJ.EQ.MESSAG) SPK=191
      IF (OBJ.EQ.OYSTER.AND.HINTED(2).AND.TOTING(OYSTER)) SPK=194

      IF (OBJ.NE.OYSTER.OR.HINTED(2).OR..NOT.TOTING(OYSTER).OR..NOT.CLOS
     1ED) GO TO 850
      HINTED(2)=YES(192,193,54)
      GO TO 860

C  Break.  Only works for mirror in repository and, of course, the vase.

2180  IF (OBJ.EQ.MIRROR) SPK=148
      IF (OBJ.EQ.VASE.AND.PROP(VASE).EQ.0) GO TO 2190
      IF (OBJ.EQ.VEND.AND.PROP(VEND).EQ.0) GO TO 2195
      IF (OBJ.NE.MIRROR.OR..NOT.CLOSED) GO TO 850
      CALL RSPEAK (197)
      GO TO 2350

2190  SPK=198
      IF (TOTING(VASE)) CALL DROP (VASE,LOC)
      PROP(VASE)=2
      FIXED(VASE)=-1
      GO TO 850

2195  IF (.NOT.TOTING(AXE)) GO TO 850
      PROP(VEND) = 1
      CALL PSPEAK (VEND,1)
      GO TO 860

C  Wake.  Only use is to disturb the dwarves.

2200  IF (OBJ.NE.DWARF.OR..NOT.CLOSED) GO TO 850
      CALL RSPEAK (199)
      GO TO 2350

C  Suspend.  Offer to exit leaving things restartable, but requiring a
C  delay before restarting (so can't save the world before trying
C  something ridiculous).

C  Upon restarting, SETUP=-1 causes return to 1210 to pick up again.

2210  SPK=201
      IF (DEMO) GO TO 850
      PRINT 2212, LATNCY
2212  FORMAT (/' I can suspend your adventure for you so that you can','
     1 resume later, but'/' you will have to wait at least',I3,' minutes
     2 before continuing.')
      IF (.NOT.YES(200,54,54)) GO TO 860
      CALL DATIME (SAVED,SAVET)
      SETUP=-1

C  This is an assembly language routine written for Univac 1100 series
C  macines.  This routine may not be necessary, correct, or sufficient
C  for other machines.

      PRINT 2214
2214  FORMAT (' Saving...')
      CALL SAVEDB
      STOP


C  Hours.  Report current non-prime-time hours for this class
C  of user.

2216  CALL MSPEAK (6)
      CALL HOURS (class)
      GO TO 860

C  Test.  Reports remaining life in batteries.

2220  IF (.NOT.HERE(LAMP)) GO TO 850
      PRINT 2221, LIMIT
      GO TO 860
2221  FORMAT (/' There are',I4,' turns of life in your lamp batteries.')

C  Cut  (rope).  Must be carrying (or in same room as) axe and rope.
C  Rope must be in one piece, or knotted, and not anchored.

2230  IF (.NOT.HERE(AXE)) GO TO 850
      IF (HERE(ROPE).AND.(PROP(ROPE).EQ.0.OR.PROP(ROPE).EQ.4))
     1GO TO 2232
      SPK = 210
      IF (PROP(ROPE).EQ.1) SPK = 209
      IF (OBJ.NE.ROPE.AND.OBJ.NE.ROPE2) SPK = 146
      GO TO 850

2232  PROP(ROPE) = 2
      CALL MOVE (ROPE2,LOC)
      PROP(ROPE2) = 2
      CALL PSPEAK (ROPE,2)
      GO TO 860

C  Tie  (rope).

2240  IF (.NOT.(HERE(ROPE).AND.HERE(ROPE2).AND.PROP(ROPE).EQ.2))
     1GO TO 2250
      IF (OBJ.NE.ROPE.AND.OBJ.NE.ROPE2) THEN
        SPK = 14
        GO TO 850
      END IF
      PROP(ROPE) = 4
      CALL RSPEAK (54)
      CALL DSTROY (ROPE2)
      GO TO 860

C  Untie  (rope).

2245  IF (HERE(ROPE).AND.PROP(ROPE).EQ.4) GO TO 2232
      IF (OBJ.NE.ROPE.AND.OBJ.NE.ROPE2) THEN
        SPK = 14
        GO TO 850
      END IF
      OBJ = 0
      IF (HERE(ROPE2).AND.((IAND(PROP(ROPE2),1)).NE.0)) OBJ = ROPE2
      IF (HERE(ROPE) .AND.((IAND(PROP(ROPE), 1)).NE.0)) OBJ = ROPE
      IF (OBJ.EQ.0) GO TO 850
      PROP(OBJ) = PROP(OBJ) - 1
      FIXED(OBJ) = 0
      NEWLOC = LOC
      GO TO 780

C  Rig / anchor  (rope).

2250  IF (OBJ.NE.ROPE.AND.OBJ.NE.ROPE2) THEN
        SPK = 25
        GO TO 850
      END IF
      OBJ = 0
      I = SPK
      IF (HERE(ROPE2).AND.((IAND(PROP(ROPE2),1)).EQ.0)) OBJ = ROPE2
      IF (HERE(ROPE) .AND.((IAND(PROP(ROPE), 1)).EQ.0)) OBJ = ROPE
      IF (LOC.NE.63.AND.LOC.NE.149.AND.LOC.NE.146)
     1SPK = 204
      IF (LOC.EQ.146.AND.((OBJ.EQ.ROPE.AND.PROP(ROPE).EQ.2).OR.
     1OBJ.EQ.ROPE2)) SPK = 206
      IF (OBJ.EQ.0) SPK = 205
      IF (SPK.NE.I) GO TO 850
      PROP(OBJ) = PROP(OBJ) + 1
      IF (TOTING(OBJ)) CALL DROP (OBJ,LOC)
      FIXED(OBJ) = -1
      NEWLOC = LOC
      GO TO 780

C  Click heels.  Works only in the dark, when wearing ruby slippers.

2255  IF (OBJ.NE.RUBY.OR..NOT.TOTING(RUBY))
     1GO TO 850
      IF (.NOT.DARK()) THEN
        SPK = 213
        GO TO 850
      END IF
      NEWLOC = 3
      GO TO 510

C  Cave closing and scoring.


C  These sections handle the closing of the cave.  The cave closes
C  CLOCK1 turns after the last treausre has been located (including the
C  pirate's chest, which may, of course, never show up).  Note that
C  the treasures need not have been taken yet. just located.  Hence,
C  CLOCK1 must be large enough to get out of the cave (it only ticks
C  while inside the cave).  When it hits zero, we branch to 2260 to
C  start closing the cave, and then sit back and wait for him to try
C  to get out.  If he doesn't within CLOCK2 turns, we close the cave:
C  If he does try, we assume he panic, and give him a few additional
C  turns to get frantic before we close.  When CLOCK2 hits zero, we
C  branch to 2280 to transport him into the final puzzle.  Note that
C  the puzzle depends upon all sorts of random things.  For instance,
C  there must be no water or oil, since there are beanstalks which we
C  don't want to be able to water, since the code can't handle it.
C  Also, we can have no keys, since there is a grate (having moved the
C  fixed object) there separating him from all the treasures.  Most of
C  these problems arise from the use of negative PROP numbers to
C  suppress the object descriptions until he's actually moved objects.

C  When the first warning comes, we lock the grate, destroy the bridge,
C  kill all the dwarves (and the pirate), remove the troll and bear
C  (unless dead), and set CLOSNG to true.  Leave the dragon:  too much
C  trouble to move it.  From now until CLOCK2 runs out, he cannot
C  unlock the grate, move to any location outside the cave (LOC<9),
C  or create the bridge.  Nor can he be resurrected if he dies.  Note
C  that the snake is already gone, since he got to the treasures
C  acceiable only via the Hall of the Mt. King.  Also, he's been in
C  the Giant room (to get eggs), so he can refer to it.  Also also,
C  he's gotten the pearl, so we know the bivalve is an oyster.
C  AND, the dwarves must have been activated, since we've found the
C  pirate's chest.

2260  PROP(GRATE)=0
      PROP(FISSUR)=0
      DO 2270 I=1,6
        DSEEN(I)=.FALSE.
2270    DLOC(I)=0
      CALL DSTROY (TROLL)
      CALL DSTROY (TROLL+100)
      CALL MOVE (TROLL2,PLAC(TROLL))
      CALL MOVE (TROLL2+100,FIXD(TROLL))
      CALL JUGGLE (CHASM)
      IF (PROP(BEAR).NE.3) CALL DSTROY (BEAR)
      PROP(CHAIN)=0
      FIXED(CHAIN)=0
      PROP(AXE)=0
      FIXED(AXE)=0
      CALL RSPEAK (129)
      CLOCK1=-1
      CLOSNG=.TRUE.
      GO TO 1030

C  Once he's panicked, and CLOCK2 has run out, we come here to set up
C  the storage room.  The room has two locations, hardwired as 115
C  (NE) and 116 (SW).  At the NE end, we place empty bottles, a nursery
C  of plants, a bed of oysters, a pile of lamps, rods with starts,
C  sleeping dwarves, and him.  And at the SW end, we place grate over
C  treasures, snake pit, covey of caged birss, more rods, and pillows.
C  A mirror stretches across one wall.  Many of the objects come from
C  known locations and/or states (e.g., the snake is known to have
C  been destroyed and needn't be carried away from its old place),
C  making the various objects be handled differently.  We also drop all
C  other objects he might be carrying (lest he have some which could
C  cause trouble, such as the keys).  We describe the flash of light
C  and trundle back.

2280  PROP(BOTTLE)=PUT(BOTTLE,115,1)
      PROP(PLANT)=PUT(PLANT,115,0)
      PROP(OYSTER)=PUT(OYSTER,115,0)
      PROP(LAMP)=PUT(LAMP,115,0)
      PROP(ROD)=PUT(ROD,115,0)
      PROP(DWARF)=PUT(DWARF,115,0)
      LOC=115
      OLDLOC=115
      NEWLOC=115

C  Leave the grate with normal (non-negative) property.

      FOO=PUT(GRATE,116,0)
      PROP(SNAKE)=PUT(SNAKE,116,1)
      PROP(BIRD)=PUT(BIRD,116,1)
      PROP(CAGE)=PUT(CAGE,116,0)
      PROP(ROD2)=PUT(ROD2,116,0)
      PROP(PILLOW)=PUT(PILLOW,116,0)

      PROP(MIRROR)=PUT(MIRROR,115,0)
      FIXED(MIRROR)=116

      DO 2290 I=1,100
2290    IF (TOTING(I)) CALL DSTROY (I)

      CALL RSPEAK (132)
      CLOSED=.TRUE.
      GO TO 510

C  Another way we can force an end to things is by having the lamp give
C  out.  When it gets close, we come here to warn him.  We go to 2300 if
C  the lamp and fresh batteries are here, in which case we replace the
C  batteries and continue.  2310 is for other cases of lamp dying.
C  2320 if when it goes out, and 2330 is if he's wandered outside and
C  the lamp is used up, in which case we force him to give up.

2300  CALL RSPEAK (188)
      PROP(BATTER)=1
      IF (TOTING(BATTER)) CALL DROP (BATTER,LOC)
      LIMIT=LIMIT+2500
      LMWARN=.FALSE.
      GO TO 1030

2310  IF (LMWARN.OR..NOT.HERE(LAMP)) GO TO 1030
      LMWARN=.TRUE.
      SPK=187
      IF (PLACE(BATTER).EQ.0) SPK=183
      IF (PROP(BATTER).EQ.1) SPK=189
      CALL RSPEAK (SPK)
      GO TO 1030

2320  LIMIT=-1
      PROP(LAMP)=0
      IF (HERE(LAMP)) CALL RSPEAK (184)
      GO TO 1030

2330  CALL RSPEAK (185)
      GAVEUP=.TRUE.
      GO TO 2360

C  And, of course, DEMO games are ended by the wizard.

2340  CALL MSPEAK (1)
      GO TO 2360

C  Oh dear, he's disturbed the dwarves.

2350  CALL RSPEAK (136)

C  Exit code.  Will eventually include scoring.  For now, however, ...

C  The present scoring algorithm is as follows:

C     Objective:         Points:        Present total possible:
C  Getting well into cave   25                   25
C  Each treasure < chest    12                   60
C  Treasure chest itself    14                   14
C  Each treasure > chest    16                  192
C  Surviving            (MAX-NUM)*10             30
C  Not quitting             4                     4
C  Reaching    CLOSNG      25                    25
C      CLOSED  : QUIT/KILLED    10
C           Klutzed        25
C           Wrong way      30
C           Success        45                    45
C  Came to Witt's End       1                     1
C  Round out the total      4                     4
C                                      Total:   380

C  (Points can also be deducted for using hints.)

2360  SCORE=0
      MXSCOR=0
      ROUND = 4

C  First tally up the treasures.  Must be in building and not broken.
C  Give the poor guy 2 points just for finding each treasure.

      DO 2370 I=50,MAXTRS
        IF (PTEXT(I).EQ.0) GO TO 2370
        K=12
        IF (I.EQ.CHEST) K=14
        IF (I.GT.CHEST) K=16
        IF (PROP(I).GE.0) SCORE=SCORE+2
        IF (PLACE(I).EQ.3.AND.PROP(I).EQ.0) SCORE=SCORE+K-2
        MXSCOR=MXSCOR+K
2370    CONTINUE

C  Now look at how he finished and how far he got.  MAXDIE and NUMDIE
C  tell how well he survived.  GAVEUP says whether he exited via QUIT.
C  DFLAG tells us if he ever got suitably deep into the cave.  CLOSNG
C  still indicates whether he reached the endgame.  And if he got as far
C  as CAVE CLOSED (indicated by CLOSED), then bonus is zero for mundane
C  exits or 135 if he blew it (so to speak).

      SCORE=SCORE+(MAXDIE-NUMDIE)*10
      MXSCOR=MXSCOR+MAXDIE*10
      IF (.NOT.(SCORNG.OR.GAVEUP)) SCORE=SCORE+4
      MXSCOR=MXSCOR+4
      IF (DFLAG.NE.0) SCORE=SCORE+25
      MXSCOR=MXSCOR+25
      IF (CLOSNG) SCORE=SCORE+25
      MXSCOR=MXSCOR+25
      IF (.NOT.CLOSED) GO TO 2380
      IF (BONUS.EQ.0) SCORE=SCORE+10
      IF (BONUS.EQ.135) SCORE=SCORE+25
      IF (BONUS.EQ.134) SCORE=SCORE+30
      IF (BONUS.EQ.133) SCORE=SCORE+45
2380  MXSCOR=MXSCOR+45

C  Did he come to Witt's End as he should?

      IF (PLACE(MAGZIN).EQ.108) SCORE=SCORE+1
      MXSCOR=MXSCOR+1

C  Round it off.

      SCORE=SCORE+ROUND
      MXSCOR=MXSCOR+ROUND

C  Deduct points for hints.  Hints < 4 are special: see database
C  description for details.

      DO 2390 I=1,HNTMAX
2390    IF (HINTED(I)) SCORE=SCORE-HINTS(I,2)

C  Return to SCORE command if that's where we came from.

      IF (SCORNG) GO TO 2110

C  That should be good enough.  Let's tell him all about it.

      PRINT 2400, SCORE,MXSCOR,TURNS
2400  FORMAT (///' You scored',I4,' out of a possible',I4,', using',I5,'
     1 turns.')

      DO 2410 I=1,LEVS
        IF (CVAL(I).GE.SCORE) GO TO 2430
2410    CONTINUE
      PRINT 2420
2420  FORMAT (/' You just went off my scale!    '/)
      GO TO 2470

2430  CALL SPEAK (CTEXT(I))
      IF (I.EQ.LEVS-1) GO TO 2450
      K=CVAL(I)+1-SCORE
      CKK='s.  '
      IF (K.EQ.1) CKK='.   '
      PRINT 2440, K,CKK
2440  FORMAT (/' To achieve the next higher rating, you need',I3,' more
     1 point',A2/)
      GO TO 2470

2450  PRINT 2460
2460  FORMAT (/' To achieve the next higher rating would be a neat trick
     1.'//' Congratulations!'/)

2470  STOP


      END
