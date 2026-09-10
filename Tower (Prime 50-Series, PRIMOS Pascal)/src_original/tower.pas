PROGRAM CHIP1(INPUT,OUTPUT,ROAD2,LEX2);

TYPE
 WORD          =   PACKED ARRAY[1..3] OF CHAR;
 GLOSSARY      =   RECORD
                    BOOK: ARRAY[1..200] OF WORD;
                    B_L : INTEGER;
                   END;
VAR
 C             : CHAR;
 WINNER,
 DONE,
 FIN,
 BLIP,
 OFFER,
 DUDLEY,
 HISS,
 DRACULA,
 QUIT,
 DEAD,
 FLUENT,
 NOTHING,
 SEE           : BOOLEAN;
 ATONIC        : PACKED ARRAY [1..15] OF CHAR;
 L,
 H,
 Y,
 K,
 J,
 I,
 U,
 W,
 X,
 V,
 QQ,
 JJ,
 II,
 WEIGHT,
 LOCATION,
 TURN,
 BONUS,
 DOLLOP,
 BATTERY,
 SCORE_TOT     : INTEGER;
 NOUNS,
 VERBS,
 DICTIONARY,
 ANIMATE,
 INANIMATE,
 PROPS,
 NOVEL,
 TREASURES,
 MOVERS,
 PLACES,
 DIRECTIONS    : GLOSSARY;
 USAGE,
 COGNATE,
 TERM,
 PRED          : WORD;
 ROUTES        : ARRAY [1..100] OF ARRAY [1..10] OF INTEGER;
 LEX2          : TEXT;
 ROAD2         : FILE OF INTEGER;
 OBJECT        : ARRAY [1..50] OF INTEGER;

procedure getcommand(VAR word1, word2 : word);
(* written by Dave Montuori at the creator's request *)
(* SLIGHT MODIFICATIONS BY RLTICE CONTINUOUSLY*)
type
   string80 = packed array[1..80] of char;
var
   inline : string80;
  j, k, i, linelength : integer;
begin
   word1 := '   '; word2 := '   ';
   linelength:=1;
   readln(inline:linelength);
   for i := 1 to linelength do
   if (inline[i] in ['a'..'z']) then
   inline[i] := chr(ord(inline[i])- 32);
   i := 1;
   while (inline[i]=' ') and (i<=linelength) do i := i + 1;
   if (i> linelength) and (linelength>0) then i:=linelength;
   j:=i;
   while (inline[j]<>' ') and (j<=linelength) do j:= j + 1;
   if j>i+2 then j:=i+2;
   if j > linelength then j:=linelength;
      for k :=i to j do
      word1[k-i+1] := inline[k];
      while (inline[i]<>' ') and (i<=linelength) do i := i + 1;
      while (inline[i] =' ') and (i<=linelength) do i := i + 1;
      if (i>linelength) and (linelength>0) then i:=linelength;
      j:=i;
      while (inline[j]<>' ') and (j<=linelength) do j:=j+1;
      if j>i+2 then
        j:=i+2;
      if j > linelength then j := linelength;
      for k := i to j do
         word2[k-i+1] := inline[k];
end; (* of proc. getcommand *)

FUNCTION IN_BOOK(LODE: WORD;TOME :GLOSSARY): BOOLEAN;
BEGIN
 X:=1;
 FIN:= FALSE;
 REPEAT
  IF ( TOME.BOOK[X]=LODE) THEN
    FIN:=TRUE
  ELSE
   X:=X+1;
 UNTIL FIN  OR (X> TOME.B_L);
  IN_BOOK:=FIN;
END;{IN_BOOK}

FUNCTION DEFINITION(PHRASE: WORD;COMIC: GLOSSARY): INTEGER;
BEGIN
 BLIP:=FALSE;
 W:=1;
 REPEAT
  IF (PHRASE=COMIC.BOOK[W])THEN
   BLIP:=TRUE
  ELSE
   W:=W+1;
 UNTIL BLIP OR (W>COMIC.B_L);
 IF W>COMIC.B_L THEN W := COMIC.B_L;
 DEFINITION:=W;
END;{DEFINITION}

PROCEDURE SCORE;
BEGIN
 Y:=1;
 SCORE_TOT:=0;
 REPEAT
  IF (OBJECT[Y]=64) THEN
   SCORE_TOT:=SCORE_TOT + 10 + Y;
  Y:=Y+1;
 UNTIL Y>9;
 IF SCORE_TOT>135 THEN
  WINNER:=TRUE;
 SCORE_TOT:=SCORE_TOT + BONUS;
 WRITELN('Your score is ',SCORE_TOT,' of 135, and you used ',TURN,' turns.');
 WRITELN('You lose points for dying, and there are only 9 treasures.');
END; {SCORE}

PROCEDURE GAD_ABOUT(G:INTEGER);
VAR
 L : INTEGER;
BEGIN
 L:=1;
 WRITELN;
 REPEAT
  IF OBJECT[L]= G THEN
   CASE L OF
     1 : WRITELN('silver bells      ');
     2 : WRITELN('ruby eggs         ');
     3 : WRITELN('lead crystals     ');
     4 : WRITELN('bronze torc       ');
     5 : WRITELN('emerald gizzard   ');
     6 : WRITELN('rough diamonds    ');
     7 : WRITELN('china teapot      ');
     8 : WRITELN('platinum coins    ');
     9 : WRITELN('gold fillings     ');
    10 : WRITELN('iron key          ');
    11 : WRITELN('glass beads       ');
    12 : WRITELN('strange skeleton  ');
    13 : WRITELN('copper cross      ');
    14 : WRITELN('scissor tongs     ');
    15 : WRITELN('ice cube          ');
    16 : WRITELN('dead rat          ');
    17 : WRITELN('old branch        ');
    18 : WRITELN('parchment scroll  ');
    19 : WRITELN('paper note        ');
    20 : WRITELN('bird''s nest      ');
    21 : WRITELN('purple amulet     ');
   OTHERWISE
    WRITELN(L,' IS AN UNKNOWN OBJECT THAT IS HERE.');
   END; {CASE AND IF}
  L:=L+1;
 UNTIL L>21;
END;{GAD_ABOUT}

PROCEDURE FILL(VAR FICTION: GLOSSARY);
BEGIN
 U:=0;
 READLN(LEX2,TERM);
 WHILE ( TERM<>'XXX') DO
  BEGIN
   U:=U+1;
   FICTION.BOOK[U]:=TERM;
   READLN(LEX2,TERM)
  END;
 FICTION.B_L:=U;
END;{FILL}

PROCEDURE GET_INPUT(VAR SPOKEN : WORD; BUCH: GLOSSARY);
VAR
 I  : INTEGER;
BEGIN
 REPEAT
  WRITELN(ATONIC);
  READLN(SPOKEN);
  FOR I:=1 TO 3 DO
  IF(SPOKEN[I] IN ['a'..'z']) THEN
  SPOKEN[I]:= CHR(ORD(SPOKEN[I])- 32);
 UNTIL (IN_BOOK(SPOKEN,BUCH));
END;{GET_INPUT}

PROCEDURE INVENTORY;
BEGIN
 NOTHING:=TRUE;
 WRITELN('You are carrying:');
 V:=1;
 REPEAT
  IF (OBJECT[V]=0) THEN
    NOTHING:=FALSE;
  V:=V+1;
 UNTIL V>21;
 IF NOTHING THEN
  WRITELN('Nothing.')
 ELSE
  BEGIN
   V:=0;
   GAD_ABOUT(V);
  END;
END;{INVENTORY}

FUNCTION WTN(CHUM : WORD): INTEGER;
BEGIN
 H:=DEFINITION(CHUM,NOUNS);
 CASE  H   OF
   1 : QQ := 1;
   2 : QQ := 2;
   3 : QQ := 3;
   4 : QQ := 4;
   5 : QQ := 5;
   6 : QQ := 6;
   7 : QQ := 7;
   8 : QQ := 8;
   9 : QQ := 9;
  10 : QQ :=10;
  11,
  12 : QQ := 1;
  13,
  14 : QQ := 2;
  15,
  16 : QQ := 3;
  17,
  18 : QQ := 4;
  19,
  20 : QQ := 5;
  21,
  22 : QQ := 6;
  23,
  24 : QQ := 7;
  25,
  26 : QQ := 8;
  27,
  28 : QQ := 9;
  29,
  30 : QQ :=10;
  31,
  32 : QQ :=11;
  33,
  34,
  35,
  36 : QQ :=12;
  37,
  38 : QQ :=13;
  39,
  40 : QQ :=14;
  41,
  42 : QQ :=15;
  43,
  44 : QQ :=16;
  45,
  46 : QQ :=17;
  47,
  48 : QQ :=18;
  49,
  50 : QQ :=19;
  51,
  52 : QQ :=20;
  53,
  54 : QQ :=21;
 OTHERWISE
  QQ:=LOCATION;
 END;
 WTN:=QQ;
END; {FUNCTION}

PROCEDURE GO_ALONG;
BEGIN
 ATONIC:='What direction?';
 COGNATE := PRED;
 IF (COGNATE = '   ') OR (NOT(IN_BOOK(COGNATE,INANIMATE))) THEN
 GET_INPUT(COGNATE,INANIMATE);
 IF (IN_BOOK(COGNATE,DIRECTIONS)) THEN
  IF ROUTES[LOCATION,WTN(COGNATE)]<1 THEN
   BEGIN
    WRITELN('You can''t GO that way.');
    IF ROUTES[LOCATION,WTN(COGNATE)]<0 THEN
     WRITELN('The way is blocked.');
   END
  ELSE
   LOCATION:= ROUTES[LOCATION, WTN(COGNATE)]
 ELSE
  IF(IN_BOOK(COGNATE,PLACES))THEN
   WRITELN('What DIRECTION is that?')
  ELSE
   WRITELN('Where is it?');
END;{GO_ALONG}

PROCEDURE PASSWORD;
VAR
 I : INTEGER;
BEGIN
 IF PRED<>'   ' THEN
   TERM := PRED
 ELSE BEGIN
  WRITELN('What should I say? ');
  READLN(TERM);
  FOR I := 1 TO 3 DO
   IF(TERM[I] IN ['a'..'z']) THEN
    TERM[I] := CHR(ORD(TERM[I])-32);
 END;
 NOTHING:=TRUE;
 CASE LOCATION OF
   1 : IF TERM = 'SUN' THEN
        BEGIN
        ROUTES[ 1,4]:= 2;
        WRITELN('The sun shines BRIGHTLY through the east wall!');
         NOTHING:=FALSE
        END;
  21 : IF TERM = 'YES' THEN
        BEGIN
         LOCATION :=44;
         WRITELN('The cavern spins wildly!');
         NOTHING := FALSE
        END;
  42 : IF TERM = 'TOM' THEN
        BEGIN
         LOCATION := 21;
         WRITELN('The cavern wildly spins!');
         NOTHING:=FALSE
        END;
  56 : IF TERM = 'T42' THEN
        BEGIN
         OBJECT[7]:=LOCATION;
         WRITELN('An expensive teapot appears!');
         NOTHING :=FALSE;
        END;
  58 : IF TERM = 'FAL' THEN
        BEGIN
         WRITELN('The floor opens beneath your feet, and you fall down!');
         WRITELN;
         WRITELN('Down........');
         WRITELN;
         WRITELN('....Down....');
         WRITELN;
         WRITELN('........THUD!');
         LOCATION := 24 ;
         NOTHING:=FALSE
        END
       ELSE
        WRITELN('That''s not it.....');
 OTHERWISE
  NOTHING:=TRUE;
 END;
 IF NOTHING THEN
  WRITELN('Mumble, mumble . . . nothing happened.');
END;{PASSWORD}

PROCEDURE INIT_GAME;
BEGIN
 WEIGHT:=0;
 DRACULA:=TRUE;
 FLUENT:=FALSE;
 WINNER:=FALSE;
 DEAD:=FALSE;
 QUIT:=FALSE;
 LOCATION:=1;
 RESET(ROAD2);
 SEE:=FALSE;
 BONUS:=0;
 TURN:=0;
 RESET(LEX2);
 READLN(LEX2,TERM);
 FILL(DIRECTIONS);
 FILL(TREASURES);
 FILL(MOVERS);
 FILL(PROPS);
 FILL(PLACES);
 FILL(ANIMATE);
 FILL(INANIMATE);
 FILL(NOUNS);
 FILL(VERBS);
 CLOSE(LEX2);
 I:=1;
 REPEAT
  J:=1;
  REPEAT
   READ(ROAD2,K);
   ROUTES[I,J]:=K;
   J:=J+1;
  UNTIL J>10;
  I:=I+1;
 UNTIL I>59 ;
 I:=1;
 REPEAT
  READ(ROAD2,K);
  OBJECT[I]:=K;
  I:=I+1;
 UNTIL K>150;
 CLOSE(ROAD2);
END;{INIT_GAME}

PROCEDURE CLIMB;
BEGIN
 IF ROUTES[LOCATION,2]>0 THEN
  LOCATION:=ROUTES[LOCATION,2]
 ELSE
  BEGIN
   IF ROUTES[LOCATION,6]>0 THEN
    LOCATION:=ROUTES[LOCATION,6]
   ELSE
    WRITELN('You try to climb up yourself, and you take a fall!');
  END;
END; {CLIMB}

PROCEDURE PICK_UP;
BEGIN
 ATONIC:=' Take what?    ';
 COGNATE := PRED;
 IF (COGNATE= '   ') OR (NOT(IN_BOOK(COGNATE,NOUNS))) THEN
 GET_INPUT(COGNATE,NOUNS);
 IF(IN_BOOK(COGNATE,ANIMATE)) THEN
  IF (OBJECT[WTN(COGNATE)]=LOCATION) THEN
   IF WEIGHT < 11 THEN
    BEGIN
     WEIGHT:=WEIGHT +1;
     OBJECT[WTN(COGNATE)]:=0
    END
   ELSE
    WRITELN('I can''t carry all of this and that too!')
  ELSE
   WRITELN('I don''t see it here.')
 ELSE
  WRITELN('It is too difficult to hold that.');
END;{PICK_UP}

PROCEDURE PUT_DOWN;
BEGIN
 ATONIC:=' Drop what?    ';
 COGNATE := PRED;
 IF (COGNATE = '   ') OR (NOT(IN_BOOK(COGNATE,NOUNS))) THEN
 GET_INPUT(COGNATE,NOUNS);
 IF(IN_BOOK(COGNATE,ANIMATE))THEN
  IF OBJECT[WTN(COGNATE)]=0 THEN
    BEGIN
    WEIGHT:=WEIGHT-1;
    OBJECT[WTN(COGNATE)]:=LOCATION
    END
  ELSE
    WRITELN('I am not holding it.')
 ELSE
  WRITELN('I couldn''t have held that at any time.');
END;{PUT_DOWN}

PROCEDURE GYPSY;
BEGIN
 IF LOCATION <> 9 THEN
  BEGIN
   IF ((TURN MOD 30) = 0) THEN
    BEGIN
     NOTHING:=FALSE;
     JJ:=1;
     REPEAT
      IF OBJECT[JJ]=0 THEN
       BEGIN
        WEIGHT:=WEIGHT - 1;
        OBJECT[JJ]:=59;
        WRITELN('You feel a tug at your pocket and as you turn you glimpse');
        WRITELN('a gypsy running away with one of your possessions! ');
        NOTHING:=TRUE;
       END;
      JJ:=JJ+1;
     UNTIL(JJ>9)OR NOTHING;
    END;
  END
 ELSE
  BEGIN
   JJ:=1;
   REPEAT
    IF OBJECT[JJ]=LOCATION THEN
     IF OFFER THEN
      IF JJ<10 THEN
       BEGIN
        WEIGHT :=WEIGHT - 1;
        OBJECT[JJ]:=64 ;
        WRITELN('The gypsy smiles and says,"This will increase you''re credit!');
        WRITELN('He takes the treasure and writes something on a scroll.');
       END
      ELSE
       BEGIN
        WEIGHT := WEIGHT - 1;
        WRITELN('The gypsy takes it and throws it into his tent.');
        WRITELN('He says,"I do collect worthless junk. Thank you!"');
        OBJECT[JJ]:=59
       END
     ELSE
      BEGIN
       WEIGHT:= WEIGHT-1;
       WRITELN('The gypsy takes it and throws it into his tent.');
       WRITELN('He says, "If you don''t want it, I''ll take it!"');
       OBJECT[JJ]:=59;
      END;
     JJ :=JJ +1;
   UNTIL JJ>21;
  END;
END;{GYPSY}

PROCEDURE MOVE_IT;
BEGIN
 ATONIC:=' Move what?    ';
 COGNATE := PRED;
 IF (COGNATE = '   ') OR (NOT(IN_BOOK(COGNATE,NOUNS))) THEN
  GET_INPUT(COGNATE,NOUNS);
 II:=DEFINITION(COGNATE,NOUNS);
 CASE II OF
 1,2,3,4,5,
 6,7,8,9,10  : WRITELN('If you want to go in that DIRECTION, type: GO .');
 11,12,13,
 14,15,16,
 17,18,19,
 20,21,22,
 23,24,25,
 26,27,28,
 29,30,31,
 32,33,34,
 35,36,37,
 38,39,40,
 41,42,43,
 44,45,46,
 47,48,49,
 50,51,52,
 54          : IF OBJECT[WTN(COGNATE)]= LOCATION THEN
                WRITELN('It moves easily.')
               ELSE
                WRITELN('It isn''t here.');
 56,57,58,
 59,63,64,
 68,70       : WRITELN('Moving that changes nothing.');
 65          : IF LOCATION = 49 THEN
                BEGIN
                WRITELN('The west wall appears loose, and');
                WRITELN('it swings on hidden hinges!');
                ROUTES[LOCATION,9]:=ROUTES[LOCATION,9]*(-1);
                END
               ELSE
                WRITELN('All walls here are too heavy to move.');
  69        :  WRITELN('It won''t move.');
  75,76     :  IF LOCATION = 3 THEN
                BEGIN
                WRITE(' They move easily around, revealing a hole ');
                WRITELN( 'in the ground!');
                ROUTES[LOCATION,6]:=33;
                END
               ELSE
                WRITELN('Doing that here changes nothing.');
    OTHERWISE
       WRITELN(' That''s a pretty impossible thing to do.');
   END;
END; {MOVE_IT}

PROCEDURE HELP_HIM;
BEGIN
 WRITELN('You can do many things on this adventure. but the');
 WRITELN('only clues you have are these:                   ');
 WRITELN('You GO directions, but you only MOVE other objects.');
 WRITELN;
 WRITELN('To score, you must sell your treasures to a gypsy.');
 WRITELN('But watch out! the gypsy is a thief, and will follow');
 WRITELN('you to steal what you find.  No fighting is allowed');
 WRITELN('in this dungeon, you must survive by your wits. To');
 WRITELN('interact with this adventure, type in the ACTION');
 WRITELN('you wish to make and then the DIRECTION, or THING,');
 WRITELN('you want to act with; e.g. WALK WEST >return<.');
 WRITELN;
 WRITELN(' You can abbreviate most words up to 3 letters (WEST');
 WRITELN('becomes WES ), but the DIRECTIONS NorthWest, SouthEast,');
 WRITELN('SouthWest, NorthEast, abbreviate like this:');
 WRITELN('S-W, N-W, N-E, S-E');
 WRITELN;
 WRITELN('All objects will be recognized by the way they are described.');
 WRITELN;
 WRITELN('if you are stuck or lost and want to start over, type:  STUCK.');
WRITELN(' It usually works.');
 WRITELN('And since you can''t swim, find some flotsam before entering WATER!');
 WRITELN('Please press <RETURN>');
 READLN;
END; {HELP_HIM}

PROCEDURE DEAD_MAN;
VAR
 I : INTEGER;
BEGIN
 WRITELN('You can see and hear nothing, You are in LIMBO.');
 WRITELN('You are dead. Sigh. Do you want to try REINCARNATION?');
 READLN(TERM);
 FOR I:=1 TO 3 DO
  IF(TERM[I] IN ['a'..'z']) THEN
   TERM[I]:=CHR(ORD(TERM[I])-32);
 IF TERM='YES' THEN
  BEGIN
  WRITELN('Sputter, CRACKLE, blip, blop, bloop!');
  WRITELN('BANG!');
  WRITELN('Bang, BANG!');
  WRITELN('Bang, BANG, bang!');
  WRITELN('BANG, bang, BANG!');
  WRITELN('      BANG, bang!');
  WRITELN('            BANG!');
  IF OBJECT[8]=0 THEN
   BEGIN
    WRITELN('             POW!!!!!!!!!!!!!!!!!');
    WRITELN('I had to borrow something from you to bring you back.');
    OBJECT [8]:= 7
   END
  ELSE
    BEGIN
     WRITELN('            fizzle.');
     WRITELN(' Darn, I didn''t have a magic amulet to bring you back with.');
     DEAD:=TRUE;
    END
 END
END;{DEAD_MAN}

PROCEDURE DOOR;
BEGIN
 IF PRED='DOO' THEN
  IF(OBJECT[10]=0) THEN
   IF(LOCATION=4)OR(LOCATION=58)AND(ROUTES[4,1]=-58)THEN
    BEGIN
     ROUTES[58,7]:=4;
     ROUTES[4,1]:=58;
     WRITELN('The door crumbles to dust when you try to unlock it!')
    END
   ELSE
    WRITELN('There aren''t any locks here!')
  ELSE
    WRITELN('You don''t have a key!')
 ELSE
  WRITELN('That''s a silly thing to try!')
END;

PROCEDURE HINT_LINE;
BEGIN
IF (PRED = 'PAR') OR (PRED = 'SCR') THEN
 IF (OBJECT[18]=0) THEN
  BEGIN
   FLUENT:=TRUE;
   WRITELN('This is an English/Trog dictionary!')
  END
 ELSE
  WRITELN('I am not holding a parchment scroll!');
IF ((PRED = 'PAP') OR (PRED = 'NOT')) THEN
  IF OBJECT[19]=0 THEN
   WRITELN('This dungeon courtesy of R. L. Tice.')
   ELSE
    WRITELN('I''m not holding a paper note!');

 IF NOT FLUENT AND ((PRED = 'WAL') OR (PRED = 'WRI')
                 OR (PRED = 'WEI') OR (PRED = 'CUR')) THEN
   WRITELN('It looks like unintelligible Troglodytic!')
 ELSE
  IF ((PRED = 'WAL') OR (PRED = 'WRI') OR
      (PRED = 'WEI') OR (PRED = 'CUR')) THEN
   CASE LOCATION OF
      27 : BEGIN
           WRITELN('It says, "Yesterday. Say it Here but not Now."');
           WRITELN('         "Tomorrow. Say it Before but not Then. "')
           END;
      21 : WRITELN('It says, "Here."');
      19 : WRITELN('It says, "Now."');
      33 : WRITELN('It says, "T42. Place your order in the parlor."');
      42 : WRITELN('It says, "Before."');
      43 : WRITELN('It says, "Then."');
  OTHERWISE
   WRITELN('There is nothing on the wall to read right now.   ');
  END;
END;

PROCEDURE CRUNCH_DATA;

BEGIN
 ATONIC:=('What''ll you do?');
 REPEAT
  WRITELN(ATONIC);
  GETCOMMAND(USAGE,PRED);
 UNTIL IN_BOOK(USAGE,VERBS);
 DOLLOP:=DEFINITION(USAGE,VERBS);
 CASE DOLLOP OF
  1,2,3,
  4,5       : PICK_UP;
  6         : CLIMB;
  7,8,9,
  10,11,12,
  13        : GO_ALONG;
  14        : WRITELN('I need a shovel to do that.');
  15,16,17,
  18        : MOVE_IT;
  19,20,21  : HINT_LINE;
  22,23,24,
  25        : WRITELN('Now, now, NO violence!');
  26        : HELP_HIM;
  27        : INVENTORY;
  28,29     : WRITELN('I don''t need a light in this dungeon game.');
  30,31     : DOOR;
  32,33     : QUIT:=TRUE;
  34,35,36,
  37,47     : PASSWORD;
  38        : SCORE;
  39,40,41,
  42        : PUT_DOWN;
  43,44,45  : BEGIN
               OFFER:=TRUE;
               PUT_DOWN
              END;
   46      : INIT_GAME;
 OTHERWISE
   WRITELN('I''m not sure about that.');
 END;
 TURN:=TURN+1;

END; {CRUNCH_DATA}


PROCEDURE CHANGE_LINES;

BEGIN
 CASE LOCATION OF
  3      : IF ROUTES[LOCATION,6]>0 THEN
             WRITELN('THere is a hole under the bushes.');
  4      : BEGIN
           IF ROUTES[LOCATION,1]>0 THEN
            WRITELN('There is a pile of dust where the door was.')
           ELSE
            WRITELN('There is a stout looking door in the entrance.');
           IF OBJECT[17]<>4 THEN
            WRITELN('You sink swiftly into the moat waters!')
           ELSE
            WRITELN('You are floating on a branch in the moat.   ')
           END;

 19,21,
 27,33  : WRITELN('There is strange writing on the wall here!');

 42,43   : WRITELN('There is curious writing on the wall here!');

  46    : IF OBJECT[16]=LOCATION THEN
           BEGIN
            WRITELN('The Tyrranosaurus quickly grabs the dead rat');
            WRITELn('and wolfs it down.  It keels over from food ');
            WRITELN('poisoning, crumbles to dust, leaving only it''s');
            WRITELN('innards behind!');
            OBJECT[5]:= LOCATION;
            OBJECT[16]:= 63;
           END
          ELSE
           WRITELN('There is a hungry looking Tyrannosaurus here!');

  49     : IF ROUTES[LOCATION,9]>0 THEN
           WRITELN('There is a doorway in the western wall.');

  52     : IF DRACULA THEN
            IF OBJECT[13]<>LOCATION THEN
             WRITELN('There is a vampire bat here!')
            ELSE
             BEGIN
              WRITELN('A bat flies away, squeaking in fear!');
              DRACULA := FALSE
             END;

  100    : BEGIN
            WRITELN('Oblivion. . . . .');
           END;
 OTHERWISE
 BEGIN
 END
 END

END;

PROCEDURE BASIC_LINES;

BEGIN
WRITELN;
 CASE LOCATION OF
     1   : BEGIN
           WRITELN('You are in a room that has a southern door marked');
           WRITELN('"REALITY". A word is scrawled on the east wall,"SUN".');
           WRITELN('There is a table in this room that has a message ');
           WRITELN('written on it, "Type this word,''help'',for more');
           WRITELN('information."');
           END;
     2   : BEGIN
           WRITELN('You are on a grassy lawn south of a moat-encircled');
           WRITELN('castle.  To the south, east, and west is a forest,');
           WRITELN('and there is an entrance across the moat from you.');
           END;
     3   : BEGIN
           WRITELN('You are north of an old moat-encricled castle.');
           WRITELN('There is forest to the north, east, and west.');
           WRITELN('There are some wysteria bushes in neat rows here.');
           END;
     4   : BEGIN
           WRITELN('You are in a moat full of swift moving water.');
           WRITE('To the north you see the front entrance of the');
           WRITELN(' castle.');
           END;
 5,6,7   : BEGIN
           WRITELN('You are lost in a forest of evergreen trees. Paths');
           WRITELN('lead in all directions.');
           END;
     8   : BEGIN
           WRITELN('You are at a gravesite in a forest clearing.');
           END;
     9   : BEGIN
           WRITELN('You are standing in a clearing with a tent to the');
           WRITELN('north. There is a gypsy standing in front of the');
           WRITELN('tent with his arms crossed, blocking the entry.');
           END;
 10,11,12: BEGIN
           WRITELN('you are at the top of an evergreen tree, from which');
           WRITELN('there is only one exit: down.');
           END;
 13,14,15,16,17,
 18,19,20,21,22,
 23,24,25,26,27,
 28,29,30,31,32,
 33,34,35,36,
 37,38   : BEGIN
           WRITELN('You are in a rough hewn corridor, with exits in');
           WRITELN('many directions. The walls glow with an eerie light.');
           END;
    39   : BEGIN
           WRITELN('You are in a cold cellar, the walls are chilly to');
           WRITELN('touch.  There are doorways to the north and west.');
           END;
    40   : BEGIN
           WRITELN('You are in a natural cavern, deep beneath the Earth,');
           WRITELN('from which there is only one exit: North.');
           END;
41,42,43,44   : BEGIN
           WRITELN('You are in a rough hewn corridor, with exits in');
           WRITELN('many directions.  The walls glow here, also. ');
           END;
    45   : BEGIN
           WRITELN('You are in a natural cavern, deep beneath the Earth, ');
           WRITELN('and there is a hungry looking Troglodyte King');
           WRITELN('here, wearing a bronze torc. There is one exit, a');
           WRITELN('crack in the wall to the north.');
           END;
    46   : BEGIN
           WRITELN('You are at the bottom of a deep pit. There is one');
           WRITELN('way out, a hole leading down. The walls are too');
           WRITELN('steep to climb.');
           END;
47,48,50 : BEGIN
           WRITELN('You are on a staircase with two obvious exits: up and down.');
           END;
    49    : WRITELN('You are on a staircase with two obvious exits: down and up.');
    51   : BEGIN
           WRITELN('You are in a hidden library with a door to the');
           WRITELN('east. On the shelves are moldering piles of what');
           WRITELN('at one time could have been books.');
           END;
    52   : BEGIN
           WRITELN('You are in a belfry. There is a door to the north');
           WRITELN('and a hole in the floor with stairs leading down.');
           END;
    53   : BEGIN
           WRITELN('You are on a parapet with a view of the surrounding' );
           WRITELN('territory. The forest seems to extend forever.');
           WRITELN('There is a door to the south, and a hole in the ');
           WRITELN('floor where stones appear to have fallen out.');
           END;
    54   : BEGIN
           WRITELN('You are in a torture chamber. There is dried blood');
           WRITELN('on the floor, and several piles of rust that at');
           WRITELN(' one time could have been almost anything.');
           WRITELN('There is a door to the west and a break in the');
           WRITELN('ceiling, through which you see the sky.');
           END;
    55   : BEGIN
           WRITELN('You are in the main library of the castle. Shelves');
           WRITELN('are full of piles of what at one time could have ');
           WRITELN('been books, but are too decomposed to read now.');
           WRITELN('Light comes from tiny windows in the north wall.');
           WRITELN('there is a large break in the floor. A door ');
           WRITELN('leads east.');
           END;
    56   : BEGIN
           WRITELN('You are in the tea parlor of the castle. A pile');
           WRITELN('of rubble on the floor is all that remains of ');
           WRITELN('the chandelier. There is a door to the north that');
           WRITELN('is rusted shut, and a staircase to the south that');
           WRITELN('leads down.');
           END;
    57   : BEGIN
           WRITELN('You are in the basement.  Massive pillars hold up the');
           WRITELN('ceiling.  Light comes from the stairwell to the south,');
           WRITELN('and a hole can barely be seen in the east wall.');
           END;
    58   : BEGIN
           WRITELN('You are in the entrance chamber of the castle. All the exits');
           WRITELN('are rusted shut, except for the southern door.');
           WRITELN('This is written on the east wall, "Name a season of ');
           WRITELN('the year."');
           END;
    59   : BEGIN
           WRITELN('You are in a tent, with a lot of worthless junk');
           WRITELN('lying about. You see a flap to the south, and a');
           WRITELN('hole in the floor to the north-east.');
           END;

  100    : BEGIN
           WRITELN('Darkness descends.');
           END;
  200    : BEGIN
           WRITELN('The door opens, and Reality comes leaping in on');
           WRITELN('this fantasy world! Reality engulfs you!! You');
           WRITE('cannot get away!! The shock is so terrible, you ');
           WRITELN('DIE!!!!'); WRITELN;
           LOCATION := 100
           END;
 OTHERWISE
  LOCATION:= LOCATION;

END;


END;

PROCEDURE SLOW_DEATH;

 BEGIN

 BASIC_LINES;
 CHANGE_LINES;
  I:=1;
  REPEAT
   ATONIC:=('What''ll you do?');
   WRITELN(ATONIC);
   GETCOMMAND(USAGE,PRED);
   IF(USAGE='DRO') AND((PRED='BRA')OR(PRED='OLD'))THEN
      PUT_DOWN
   ELSE
     WRITELN('You are going under again!   >GLUB<!');

   IF OBJECT[17]= 4 THEN
    I := 16;
  I:=I+1;
  UNTIL I>10;
 IF I<15 THEN
  BEGIN
  LOCATION :=100 ;
  END

 END;{SLOW_DEATH}

PROCEDURE EXTRA;
BEGIN

   IF (LOCATION = 8  ) AND ( OBJECT[13]= 0) AND (ROUTES[LOCATION,6]<1)
    THEN  BEGIN
     WRITELN('The grave caves in with a rumble!');
     ROUTES[13,2]:=8;
     ROUTES[LOCATION,6]:= 13;
     END;

  IF(OBJECT[15]=0)AND(OBJECT[14]<>0)THEN
   BEGIN
   OBJECT[15]:=LOCATION;
   WRITELN('Ice is too cold to carry in your pocket. Find something');
   WRITELN('to carry it with!');
   END;

  IF(OBJECT[16]=0)AND(OBJECT[15]<>0)THEN
   BEGIN
   OBJECT[16]:=LOCATION;
   WRITELN('The rat stinks too much. Find some ice to keep it on.');
   END;

   IF(LOCATION = 4) AND (OBJECT[17]<>4) THEN
     SLOW_DEATH;

   IF(LOCATION = 52) AND DRACULA AND (OBJECT[1] = 0) THEN
    BEGIN
     WRITELN('The vampire swoops on you, and kills you!');
     OBJECT[1]:= LOCATION;
     LOCATION := 100
    END;
   IF LOCATION =100 THEN
      DEAD_MAN
      ELSE
         GYPSY;

   IF((LOCATION = 45) AND OFFER)THEN
    IF (PRED = 'GLA') OR (PRED = 'BEA') THEN
      IF OBJECT[11] = LOCATION THEN
       BEGIN
        OBJECT[11] := 63;
        WRITE(' The king takes your trinkets and throws ');
        WRITELN('some rough diamonds down in trade!');
        OBJECT[6] := LOCATION;
       END
      ELSE
       WRITELN('You don''t have any glass beads!')
    ELSE
     WRITELN('The King ignores the offer.          ');

   OFFER:=FALSE;
 END;{EXTRA}

BEGIN {MAIN}

INIT_GAME;
REPEAT
BASIC_LINES;
CHANGE_LINES;
GAD_ABOUT(LOCATION);
IF LOCATION<>100 THEN
CRUNCH_DATA;
EXTRA;
UNTIL DEAD OR QUIT OR WINNER;
IF DEAD OR QUIT THEN
SCORE;

END.{MAIN}