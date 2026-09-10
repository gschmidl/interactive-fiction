    PROGRAM ANKH1(INPUT,OUTPUT,ROADS,LEXICON);

    (***********************************************
     *                                             *
     *    THIS PROGRAM WAS CONCEIVED AND WRITTEN   *
     *   BY RANDALL TICE IN A MOMENT OF BOREDOM.   *
     *    HE FRANKLY HOPES THAT YOU ENJOY PLAYING  *
     *   IT.                                       *
     *    HE MUST ALSO THANK DAVE MONTOURI, MIKE   *
     *   DULLAGHAN, AND CHIP ROBERSON FOR THEIR    *
     *   INVALUABLE HELP AND COMMENTS.             *
     *    ESPECIALLY DAVE, WHO INSTALLED THE       *
     *   GETCOMMAND PROCEDURE IN ORDER TO MAKE IT  *
     *   A MUCH MORE USER FRIENDLY GAME.           *
     *    WRITTEN IN JANUARY,1984, AT W&M COLLEGE. *
     *    MINOR MODIFICATIONS IN DECEMBER,1984.    *
     *                                             *
     ***********************************************)
    TYPE

     WORD     = PACKED ARRAY[1..3] OF CHAR;
     GLOSSARY = RECORD
                 BOOK: ARRAY[1..100] OF WORD;
                 B_L : INTEGER;
                END;
    VAR
     C                       : CHAR;
     GAME,
     WINNER, DONE,
     FIN, BLIP,
     DUDLEY, HISS,
     QUIT, DEAD,
     NOTHING, SEE            : BOOLEAN;
     ATONIC                  : PACKED ARRAY [1..15] OF CHAR;
     II, V, X, W, U,
     H, QQ, L, JJ, I,
     J, K, Y, LOCATION,
     TURN, BONUS, DOLLOP,
     SCORE_TOT, BATTERY         : INTEGER;
     NOUNS,VERBS,DICTIONARY,
     ANIMATE, INANIMATE,
     PROPS, NOVEL,TREASURES,
     MOVERS, PLACES,
     DIRECTIONS              : GLOSSARY;
     USAGE, COGNATE,
     TERM, PRED              : WORD;
     ROUTES                  : ARRAY [1..24] OF ARRAY [1..10] OF INTEGER;
     LEXICON                 : TEXT;
     ROADS                   : FILE OF INTEGER;
     OBJECT                  : ARRAY [1..17] OF INTEGER;

    procedure getcommand(VAR word1, word2 : word);

    (* written by Dave Montuori at the creator's request *)
    type
       string80 = packed array[1..80] of char;
    var
       inline : string80;
      j, k, i, linelength : integer;
    begin
       word1 := '   '; word2 := '   ';
       linelength:=0;
       readln(inline:linelength);
       for i := 1 to linelength do
        if (inline[i] in ['a'..'z']) then
         inline[i] := chr(ord(inline[i])-32);
       i := 1;
       while (inline[i]=' ') and (i<=linelength) do i := i + 1;
       if i > linelength then i:=linelength;
       j:=i;

       while (inline[j]<>' ') and (j<=linelength) do j:= j + 1;
       if j>i+2 then j:=i+2;
       if j > linelength then j:=linelength;

          for k :=i to j do
          word1[k-i+1] := inline[k];

          while (inline[i]<>' ') and (i<=linelength) do i := i + 1;
          while (inline[i] =' ') and (i<=linelength) do i := i + 1;
          if i > linelength then i := linelength;
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
      W:=0;
      REPEAT
       W:=W+1;
       IF (PHRASE=COMIC.BOOK[W])THEN
        BLIP:=TRUE;
      UNTIL BLIP OR (W=COMIC.B_L);

      DEFINITION:=W;
    END;{DEFINITION}

    PROCEDURE SCORE;

     BEGIN
    Y:=1;
    SCORE_TOT:=0;
    REPEAT
    IF (OBJECT[Y]=3) THEN
     SCORE_TOT:=SCORE_TOT + 10 + Y;
    Y:=Y+1;
    UNTIL Y>6;
    IF OBJECT[3]=3 THEN
     SCORE_TOT:=SCORE_TOT - 13;
    IF OBJECT[17]=3 THEN
     SCORE_TOT:=SCORE_TOT + 13;
     IF SCORE_TOT>80 THEN
      WINNER:=TRUE;
    SCORE_TOT:=SCORE_TOT + BONUS;
    WRITELN('Your score is ',SCORE_TOT,'%, and you used ',TURN,' turns.');
    WRITELN('You lose percentage for Dying and Helps, and there are only 6 treasures.');

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
           1 : WRITELN('Silver Ankh       ');
           2 : WRITELN('Jade Barrel       ');
           3 : WRITELN('Net               ');
           4 : WRITELN('Onyx Bracelet     ');
           5 : WRITELN('Gold Oxen         ');
           6 : WRITELN('Platinum Elephant ');
           7 : WRITELN('Apple             ');
           8 : WRITELN('Myna Bird         ');
           9 : WRITELN('Daisy             ');
          10 : WRITELN('Tuna Fish         ');
          11 : WRITELN('Clover            ');
          12 : WRITELN('Flashlight        ');
          13 : WRITELN('Igneous Rock      ');
          14 : WRITELN('Rubber Raft       ');
          15 : WRITELN('Log               ');
          16 : WRITELN('Shovel            ');
          17 : WRITELN('Jeweled Sceptre   ');

      OTHERWISE

       WRITELN(L,' IS AN UNKNOWN OBJECT THAT IS HERE.');
      END; {CASE}
      L:=L+1;
     UNTIL L>17;
    END;{GAD_ABOUT}

    PROCEDURE FILL(VAR FICTION: GLOSSARY);

    BEGIN
    U:=0;

     REPEAT
      READLN(LEXICON,TERM);
      IF TERM<>'XXX' THEN
       BEGIN
        U:=U+1;
        FICTION.BOOK[U]:=TERM;
        END;

     UNTIL (TERM='XXX');
     FICTION.B_L:=U;
    END;{FILL}

    PROCEDURE GET_INPUT(VAR SPOKEN : WORD; BUCH: GLOSSARY);
    VAR
     I   :  INTEGER;
    BEGIN
     REPEAT
      WRITELN(ATONIC);
      READLN(SPOKEN);
      FOR I := 1 TO 3 DO
       IF (SPOKEN[I] IN ['a'..'z']) THEN
         SPOKEN[I]:= CHR(ORD(SPOKEN[I])-32);
     UNTIL (IN_BOOK(SPOKEN,BUCH));

    END;{GET_INPUT}

    PROCEDURE INVENTORY;

    BEGIN

     NOTHING:=TRUE;
     WRITELN('You are carrying:');
     V:=0;
     GAD_ABOUT(V); V:=1;
     REPEAT
     IF (OBJECT[V]=0) THEN
       NOTHING:=FALSE;
    V:=V+1;
     UNTIL V>17;
     IF NOTHING THEN
      BEGIN
       WRITELN('Nothing.')
      END;
    END;{INVENTORY}


    FUNCTION WTN(CHUM : WORD): INTEGER;
    BEGIN
     H:=DEFINITION(CHUM,NOUNS);
     CASE  H   OF

      76  : QQ:=1;
      77  : QQ:=2;
      78  : QQ:=3;
      79  : QQ:=4;
      80  : QQ:=5;
      81  : QQ:=6;
      82  : QQ:=7;
      83  : QQ:=8;
      84  : QQ:=9;
      85  : QQ:=10;
      1 , 2    : QQ:=1;
      3 , 4    : QQ:=2;
      5 , 6    : QQ:=17;
      7 , 8    : QQ:=4;
      9 ,10    : QQ:=5;
      11,12    : QQ:=6;
      13       : QQ:=7;
      14,15    : QQ:=8;
      16       : QQ:=9;
      17,18    : QQ:=10;
      19       : QQ:=11;
      20,21    : QQ:=12;
      22,23    : QQ:=13;
      24,25    : QQ:=14;
      26,27    : QQ:=15;
      28       : QQ:=16;
      29       : QQ:=3;
      30,31    : QQ:=20;
      32       : QQ:=6;
      33       : BEGIN
                  IF LOCATION = 6 THEN
                   QQ:=6
                  ELSE
                   IF LOCATION = 20 THEN
                     QQ:=20
                   ELSE
                    IF LOCATION = 5 THEN
                      QQ:=5
                    ELSE
                      QQ:=0
                 END;
      34        : QQ:=20;
      35        : IF ROUTES[1,9]<1 THEN
                   QQ:=1
                  ELSE
                   QQ:=2;
      36        : QQ:=10;
      37        : QQ:=1;
      38        : QQ:=19;
      39        : QQ:=22;
      40        : QQ:=7;
      41,42     : QQ:=10;
      43        : QQ:=23;
      44        : QQ:=1;
      45        : QQ:=13;
      46        : IF LOCATION=8 THEN
                  QQ:=8
                  ELSE
                  QQ:=18;
      47        : QQ:=19;
      48        : QQ:=3;
      49        : QQ:=7;
      50 , 51 ,
      52        : QQ:=12;
      53, 54    : QQ:=14;
      55        :QQ:=23;
      56 , 57   : QQ:=21;
      58        : QQ:=6;
      59        : QQ:=9;
      60, 61,
      62        : QQ:=17;
      63        : QQ:=LOCATION-1;
      64 , 65   : QQ:=11;
      66        : QQ:=20;
      67        : QQ:=10;
      68        : QQ:=5;
      69        : QQ:=15;
      70,71     : QQ:=18;
      72        : QQ:=2;
      73        : QQ:=LOCATION+1;
      74        : QQ:=LOCATION-1;
      75        : QQ:=24;
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
       WRITELN('You can''t go that way.');
        IF ROUTES[LOCATION,WTN(COGNATE)]<0 THEN
          WRITELN('The way is blocked.');
      END
     ELSE
       LOCATION:= ROUTES[LOCATION, WTN(COGNATE)]
    ELSE
     IF(IN_BOOK(COGNATE,PLACES))THEN
      WRITELN('What direction is that?')
     ELSE
      WRITELN('Where is it?');

    END;{GO_ALONG}

    PROCEDURE PASSWORD;
    VAR
     I   :  INTEGER;
    BEGIN
     IF PRED<>'   ' THEN TERM := PRED
     ELSE
     BEGIN
      WRITELN('What should I say? ');
      READLN(TERM);
      FOR I :=1 TO 3 DO
       IF (TERM[I] IN ['a'..'z']) THEN
        TERM[I]:=CHR(ORD(TERM[I])-32);
     END;
     NOTHING:=TRUE;

     IF TERM = 'HEL' THEN
      BEGIN
      WRITELN('If you want Help just type : Help .');
      NOTHING:=FALSE;
      END;
     IF (TERM = 'JAN') AND (LOCATION=5) THEN
      BEGIN
       NOTHING:=FALSE;
      WRITELN('KERFOOOOOOOOOM!!!!!!!');
      ROUTES[4,7]:=ROUTES[4,7]*(-1);
      ROUTES[5,9]:=ROUTES[5,9]*(-1);
      END;

     IF(TERM='FOO')AND (LOCATION=4) THEN
      BEGIN
      WRITELN('KERFLAAAAAM!');
      ROUTES[4,7]:=ROUTES[4,7]*(-1);
      ROUTES[5,9]:=ROUTES[5,9]*(-1);
      NOTHING:=FALSE;
      END;

     IF(LOCATION=23)AND(TERM='MAN')THEN
       BEGIN
       WRITELN('The Sphinx falls over and crumbles to dust!');
       ROUTES[23,9]:=24;
        NOTHING:=FALSE
       END;

     IF NOTHING THEN
      WRITELN('Mumble, Mumble, nothing happens.');

    END;{PASSWORD}

    PROCEDURE INIT_GAME;

    BEGIN
    WINNER:=FALSE;
    DEAD:=FALSE;
    QUIT:=FALSE;
    BATTERY:=100;
    LOCATION:=1;
    RESET(ROADS);
    SEE:=FALSE;
    BONUS:=19;
    TURN:=0;
    RESET(LEXICON);
    FILL(TREASURES);
    FILL(MOVERS);
    FILL(PROPS);
    FILL(PLACES);
    FILL(DIRECTIONS);
    FILL(ANIMATE);
    FILL(INANIMATE);
    FILL(NOUNS);
    FILL(VERBS);
    CLOSE(LEXICON);
    I:=1;
    REPEAT
     J:=1;
     REPEAT
      READ(ROADS,K);
      ROUTES[I,J]:=K;
      J:=J+1;
     UNTIL J>10;
     I:=I+1;
    UNTIL I>24;
    I:=1;
    REPEAT
     READ(ROADS,K);
     OBJECT[I]:=K;
     I:=I+1;
    UNTIL I>17;
    CLOSE(ROADS);
    END;{INIT_GAME}

    PROCEDURE PICK_UP;

    BEGIN
     ATONIC:=' Take what?    ';
     COGNATE := PRED;
     IF (COGNATE='   ') OR (NOT(IN_BOOK(COGNATE,NOUNS))) THEN
     GET_INPUT(COGNATE,NOUNS);
     IF(IN_BOOK(COGNATE,ANIMATE)) THEN
      IF (OBJECT[WTN(COGNATE)]=LOCATION) THEN
       OBJECT[WTN(COGNATE)]:=0
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
        OBJECT[WTN(COGNATE)]:=LOCATION
      ELSE
        WRITELN('I am not holding it.')
     ELSE
      WRITELN('I couldn''t have held that at any time.');

    END;{PUT_DOWN}

    PROCEDURE GREMLIN;

    BEGIN
    IF(LOCATION>4)AND((TURN MOD 20) = 0) THEN
     BEGIN
    NOTHING:=FALSE;
    JJ:=1;
    REPEAT
    IF OBJECT[JJ]=0 THEN
     BEGIN
      IF JJ>3 THEN
       OBJECT[JJ]:=LOCATION-1
      ELSE
       OBJECT[JJ]:=6;
      WRITELN('You feel a tug at your pocket and as you turn you see');
      WRITELN('a Gremlin scampering away with one of your possessions!');
     NOTHING:=TRUE;
     END;
    JJ:=JJ+1;
    UNTIL(JJ>6)OR NOTHING;
    END;
    END;{GREMLIN}

    PROCEDURE MOVE_IT;
    BEGIN
    ATONIC:=' Move what?    ';
    COGNATE := PRED;
    IF (COGNATE = '   ') OR (NOT(IN_BOOK(COGNATE,NOUNS))) THEN
    GET_INPUT(COGNATE,NOUNS);
       II:=DEFINITION(COGNATE,NOUNS);
        CASE II OF
         47  : BEGIN
                WRITELN('It gorges itself on you!');
                LOCATION := 100;
                DEAD :=TRUE;
               END;
         46  : WRITELN('It moves right back.');
         48  : BEGIN
                DEAD := TRUE;
                LOCATION := 100;
                WRITELN('Reality comes crashing in!');
               END;
      30,31  : BEGIN
                WRITELN('It bites you!');
                DEAD := TRUE;
                LOCATION := 100;
               END;
         35  : BEGIN
               WRITELN('It moves into the wall.');
               ROUTES[2,6]:=ROUTES[2,6]*(-1);
               ROUTES[2,4]:=ROUTES[2,4]*(-1);
               ROUTES[1,9]:=ROUTES[1,9]*(-1);
               ROUTES[2,7]:=ROUTES[2,7]*(-1);
               END;
       41,42 : BEGIN
                WRITELN('The odor overcomes you and you die.');
                DEAD := TRUE;
                LOCATION := 100;
               END;
         43  : BEGIN
                WRITELN('The Sphinx hits you over your head.');
                DEAD := TRUE;
                LOCATION := 100;
               END;
         45  :BEGIN
               WRITELN('The throne moves to the side.');
               ROUTES[13,7]:=ROUTES[13,7]* (-1);
              END;
        OTHERWISE
           WRITELN(' That''s a futile thing to do right now!');
       END;

      CASE II OF

       30,31,41,
       42,43,47,48 : LOCATION:=100;

      OTHERWISE
       LOCATION:=LOCATION;
      END;

    END; {MOVE_IT}

    PROCEDURE HELP_HIM;

    BEGIN
     IF (OBJECT[8]=0) THEN
      BEGIN
      CASE LOCATION OF
         1 : WRITELN('A beginner, eh?');
         2 : BEGIN
              WRITELN('The Myna looks disgusted and says:');
              WRITELN('"You''re such an untidy twit. You didn''t put');
              WRITELN('the bookcase back where you found it."');
             END;
         4 : BEGIN
              WRITELN(' The Myna Bird says:');
              WRITELN('"Can''t you read, Fool? It''s as obvious as ');
              WRITELN('saying your own Name !"');
             END;
         5 : BEGIN
              WRITELN('The Myna looks perplexed but says:');
              WRITELN('"You know, I bet Janus wrote that note!"');
             END;
         8 : BEGIN
              WRITELN('The Myna bird squawks:');
              WRITELN('"So get out of the water! I''m not a lifeguard!"');
             END;
        13 :  WRITELN('Myna: "Where did the King go! Where did he hide!"');
        10 :  WRITELN('Myna: "Rawk! That skunk can sure smell!"');
        17 : WRITELN('"Don''t go west! Don''t go west!",The bird says.');
        18 : WRITELN('"You might raft across, Dodo!", Myna says.');
        19 : WRITELN('"Cats love seafood! But not birds!" pleads the bird.');
        20 : BEGIN
             WRITELN('"You might kill with sex, drugs, Rock or roll,"');
             WRITELN('sings the Myna.');
             END;
        22 : WRITELN('Myna: "Sex and drugs and Rock and roll, chirp!"');
        23 : WRITELN('Myna: "Man, that''s tough!" says the bird.');
     OTHERWISE
         WRITELN('The Myna looks bored. try going somewhere else.');
     END;
    END
   ELSE
     WRITELN('Why don''t you ask the bird for help?');
    IF LOCATION=1 THEN
     BEGIN
      WRITELN('This is the only place where help is free.  You can');
      WRITELN('go places, but you can only move other objects. To');
      WRITELN('interact with this adventure, type in the action');
      WRITELN('you wish to make and then the direction, or thing,');
      WRITELN('you want to act with, e.g. WALK WEST <return>.');
      WRITELN(' You can abbreviate most words up to 3 letters (WEST');
      WRITELN('becomes WES), but the directions NORTHWEST, SOUTHEAST,');
      WRITELN('SOUTHWEST, NORTHEAST, abbreviate like this:');
      WRITELN('N-W, S-E, S-W, N-E.');
       WRITELN('Do not go near the door to the south.  Reality comes');
       WRITELN('crashing in and you die.  There is another way out of');
       WRITELN('this room.');
      WRITELN;
     END
    ELSE
     BONUS:=BONUS-1;

    END; {HELP_HIM}

    PROCEDURE DEAD_MAN;
     VAR
      I : INTEGER;
     BEGIN
     WRITELN('You can see and hear nothing. You are in Limbo.');
     WRITELN('You are dead. Sigh. Do you want to try reincarnation?');
     READLN(TERM);
     FOR I := 1 TO 3 DO
      IF (TERM[I] IN ['a'..'z']) THEN
       TERM[I]:=CHR(ORD(TERM[I])-32);
     IF TERM='YES' THEN
      BEGIN
      WRITELN('SPUTTER, CRACKLE, BLIP,BLIP,BLIP!');
      WRITELN('BANG!');
      WRITELN('BANG, BANG!');
      WRITELN('BANG, BANG, BANG!');
      WRITELN('BANG, BANG, BANG!');
      WRITELN('      BANG, BANG!');
      WRITELN('            BANG!');
      IF OBJECT[2]=0 THEN
       OBJECT[2] := 20;
      IF OBJECT[11]=0 THEN
       BEGIN
        WRITELN('             POW!!!!!!!!!!!!!!!!!');
        WRITELN('I had to borrow something from you to bring you back.');
        OBJECT[11]:=9; LOCATION:=1; BONUS:=BONUS-4;
       END
      ELSE
        BEGIN
         WRITELN('            fizzle.');
         WRITELN('Damn. I ran out of clovers. sorry, you''re a corpse.');
         DEAD:=TRUE;
        END
     END
    END;{DEAD_MAN}

    PROCEDURE SLOW_DEATH;

     BEGIN

      I:=1;
      REPEAT
       ATONIC:=('What"ll you do?');
       GETCOMMAND(USAGE,PRED);
       IF(USAGE='DRO')THEN
          PUT_DOWN
       ELSE
         WRITELN('No time for that, you''re sinking fast!');
       IF OBJECT[15]=15 THEN
        I := 16;
      I:=I+1;
      UNTIL I>10;
     IF I<15 THEN
      LOCATION:=100;
     END;{SLOW_DEATH}

    PROCEDURE EXTRA;
    VAR
     I  : INTEGER;
     BEGIN
    IF SEE THEN
     BATTERY:=BATTERY-1;
    IF SEE AND ( BATTERY<1 ) THEN
      BEGIN
      WRITELN('The flashlight''s batteries are dead!!!');
      SEE:=FALSE
       END;
    IF SEE AND (BATTERY<23) THEN
     WRITELN('The flashlight is growing dim...');
      TURN:=TURN+1;
       IF OBJECT[13]=20 THEN
        HISS := FALSE
       ELSE
        HISS := TRUE;
       IF HISS AND (OBJECT[2]=0) THEN
        BEGIN
         HISS := FALSE;
         LOCATION:=100;
         WRITELN('The snake bites you, and you die!!!')
        END;
       IF LOCATION =100 THEN
          DEAD_MAN
    (* ELSE IF (LOCATION=15) AND (OBJECT[15]<>15) THEN
         SLOW_DEATH *)
         ELSE
             GREMLIN;
     IF OBJECT[14]=0 THEN
      ROUTES[18,4]:=19
     ELSE
      ROUTES[18,4]:=8;

     IF OBJECT[9]=10 THEN
      BEGIN
      ROUTES[10,8]:=11;
      ROUTES[11,1]:=10;
      END
     ELSE
     BEGIN
     ROUTES[10,8]:=-11;
     ROUTES[11,1]:=-10
     END;

     IF(OBJECT[10]=0)AND(OBJECT[3]<>0)THEN
      BEGIN
       OBJECT[10]:=LOCATION;
       WRITELN('The fish slips out of your grasp!');
      END;

    IF QUIT THEN
     BEGIN
      WRITELN('Do you wish to quit? ');
      READLN(TERM);
      FOR I :=1 TO 3 DO
       IF (TERM[I] IN ['a'..'z']) THEN
        TERM[I]:=CHR(ORD(TERM[I])-32);
      IF TERM<> 'YES' THEN
       QUIT:=FALSE
     END

     END;{EXTRA}

    PROCEDURE CRUNCH_DATA;

    BEGIN
     ATONIC:=('What"ll you do?');
     REPEAT
      WRITELN(ATONIC);
      GETCOMMAND(USAGE,PRED);
     UNTIL IN_BOOK(USAGE,VERBS);
     DOLLOP:=DEFINITION(USAGE,VERBS);
     CASE DOLLOP OF
      1,2,3,4,5  : PICK_UP;
      6,7,8,9,10,11,12  : GO_ALONG;
      13  : WRITELN('I need a shovel to do that.');
      14,15,16,17 : MOVE_IT;
      18,19,20 : WRITELN('This is all you can see:');
      21,22,23,24 : WRITELN('Now, now, NO violence!');
      25  : HELP_HIM;
      26  : INVENTORY;
      27,28 : IF (OBJECT[12] =0)OR(OBJECT[12]=LOCATION)THEN
                 SEE:=TRUE;
      29,30 : IF (OBJECT[12]=0) OR (OBJECT[12]=LOCATION)THEN
                 SEE:=FALSE;
      31,32 : QUIT:=TRUE;
      33,34,35,36  : PASSWORD;
      37  : SCORE;
      38,39,40,41 : PUT_DOWN;
     OTHERWISE
       WRITELN('I''n not sure how to do that.');
     END;

    END; {CRUNCH_DATA}

    PROCEDURE CHANGE_LINES;

    BEGIN
     CASE LOCATION OF
         1   : IF ROUTES[1,9]<1 THEN
               WRITELN('There is a bookcase on the west wall.')
               ELSE
               WRITELN('There is a passage in the west wall.');
         2   : IF ROUTES[1,9]<1 THEN
               WRITELN('The stairs also go down and south.')
               ELSE
               BEGIN
               WRITELN('A passage leads east.');
               WRITELN('The bookcase is to the south.');
               END;
         4   : IF ROUTES[4,7]<1 THEN
                BEGIN
                WRITELN('There is a brick wall to the south with this');
                WRITE('message written on it: Speak, Fool, and you may');
                WRITELN(' pass!');
                END
               ELSE
               WRITELN('South is a wall with a passage through it.');
         5   : IF ROUTES[4,7]<1 THEN
               WRITELN('West is a wall with a hole in it.')
               ELSE
                BEGIN
              WRITELN('West is a wall with this message: ');
              WRITELN('What is my two-faced name?');
                END;
        10    :  IF ROUTES[10,8]<1 THEN
                 WRITELN('There is a skunk blocking the door.');
        11    : BEGIN
                IF ROUTES[10,8]<1 THEN
                 WRITELN('There is a skunk blocking the door.');
                IF OBJECT[14]<>LOCATION THEN
                 BEGIN
                 ROUTES[11,6]:=16;
                 ROUTES[16,2]:=11;
                 WRITELN('There is a hole in the floor.')
                 END
                ELSE
                 BEGIN
                 ROUTES[11,6]:=-16;
                 ROUTES[16,2]:=-11
                 END
                END;
        13   : BEGIN
                IF NOT SEE THEN
                  WRITELN('It is too dark to see. Watch out for holes.')
               ELSE
                BEGIN
                 WRITELN('You are in a room with a throne to the south.');
                 WRITELN('A passage leads west from here. ');
                 IF ROUTES[13,7]>0 THEN
                  WRITELN('There is a passage beside the throne.');
                 WRITELN('There is a hole in the ceiling.');
                 IF OBJECT[17]=0 THEN
                  WRITELN('The sceptre will not fit through the hole.')
               END;
                 IF OBJECT[17]=0 THEN
                  ROUTES[13,2]:=-12
                 ELSE
                  ROUTES[13,2]:=12;
              END;
        14   : IF SEE THEN
                BEGIN
                 WRITE('You are in a small  King''s chamber. Only one');
                 WRITELN(' exit is visible, a northeast door.')
                END
                 ELSE
                 WRITELN('It is too dark to see.');

        15   : IF OBJECT[15]=15 THEN
                WRITELN('You are standing on a log over the quicksand.')
               ELSE
                BEGIN
                WRITELN('You are sinking into the quicksand!');
                SLOW_DEATH
                END;
        16   : IF OBJECT[14]=11 THEN
                WRITELN('The ramp is blocked at the top.');
        19   : IF OBJECT[10]<> 19 THEN
                 BEGIN
                ROUTES[19,5]:=-20;
               WRITELN('There is a hungry tiger blocking the beach!');
               END
               ELSE
               BEGIN
               ROUTES[19,5]:=20;
               WRITELN('A tiger is eating a fish here.');
               END;
        20   : IF(HISS) AND (OBJECT[2]=20) THEN
                WRITELN(' There is an asp here, hissing at you.')
               ELSE
                WRITELN(' There is a crushed asp here.');
        23   : IF (ROUTES[23,9]<1)THEN
                BEGIN
                  WRITELN(' A Sphinx in front of the pyramid asks:');
               WRITELN('What walks on 4 legs in the morning, 2 in the');
               WRITELN('afternoon, and 3 in the evening?');
                END
               ELSE
                  WRITELN('There is a passage into the pyramid.');
        24   : IF SEE THEN
                BEGIN
                WRITE('You are in a crypt with a northeast door. There');
                WRITELN(' are hieroglyphics on the');
                WRITELN('walls that say''Home of the Last Treasure''')
                END
                ELSE
                BEGIN
                WRITE('You are inside a dark, gloomy puramid. you can');
                WRITELN(' not see anything for the lack of light.')
                END;
      100    : BEGIN
                WRITELN('Oblivion');
               END;
     OTHERWISE
      WRITELN;
     END

    END;

    PROCEDURE BASIC_LINES;

    BEGIN
    WRITELN;
     CASE LOCATION OF
         1   : BEGIN
               WRITELN('You are in a room that has one door to the south.');
               WRITELN('A table is in this room and it has a note     ');
               WRITELN('scratched on it''s surface, ''Type this word,   ');
               WRITELN('HELP, for some game information.''');
               END;
         2   : BEGIN
               WRITE  ('You are at a landing of a staircase that goes ');
               WRITELN('up and north.');
               END;
         3   : BEGIN
               WRITELN('You are in the attic.  Stairs go down and south. This is');
               WRITELN(' where all treasures must be taken to score points.');
               END;
         4   : BEGIN
               WRITELN('You are at the bottom of the staircase. ');
               END;
         5   : BEGIN
               WRITE('You are in an apple orchard.  All the trees are ');
               WRITELN('too young to bear fruit.');
               END;
         6   : BEGIN
       WRITELN('You are in a forest of young evergreens, too small to climb.');
               END;
         7   : BEGIN
               WRITELN('You are on a sandy beach. South is the ocean.');
               WRITELN('East is a high, impassable mountain. ');
               END;
         8   : BEGIN
               WRITE('The current has swept you to calm waters south ');
               WRITELN('of a beach.');
               END;
         9   : BEGIN
               WRITELN('You are in a garden. South is a beach. North');
               WRITELN('Is an ocean. East is a cave-riddled mountain.');
               END;
        10   : BEGIN
               WRITELN('You are on a grassy lawn. West is a forest. A');
               WRITELN('Twig hut is to the southwest. North is an ocean.');
               WRITELN('East is a mountain with cave openings.');
               END;
     11   : WRITELN('You are in a twig hut. There is a door to the north.');
        12   : BEGIN
               WRITELN('You are in a buried hall. Passages go down,');
               WRITELN('south, and southwest. light comes from above.');
               END;
        15   : BEGIN
               WRITELN('You are in a room with exits to the north and ');
               WRITELN('east. Light comes from above. The floor is ');
               WRITELN('composed of quicksand.');
               END;
        16   : BEGIN
               WRITELN('You are on a ramp going up to the north and   ');
               WRITELN('down to the south.             ');
               END;
        17   : BEGIN
               WRITELN('You are on a mountain top. To the north is ocean,');
            WRITELN('a passage goes south, and to the east is a valley.');
               END;
        18   : BEGIN
              WRITE('You are west of a river in a valley. To the west is a ');
               WRITELN('mountain passage.');
               END;
        19   : BEGIN
               WRITELN('You are on the east bank of a river. To the north,');
               WRITELN('south, and east is what looks like an impassable jungle.');
               END;
        20   : BEGIN
               WRITELN('You are in a dense jungle, with twisty paths ');
               WRITELN('leading in all directions. The palm trees here ');
               WRITELN('are too slippery to climb. To the southwest you');
               WRITELN('can see a large structure.');
               END;
        21   : BEGIN
               WRITE('You are on top of an Egyptian pyramid. A path ');
               WRITELN('goes down and north of here.');
               END;
        22   : BEGIN
               WRITELN('You are at a rocky place in the dense jungle.');
               END;
        23   : BEGIN
               WRITELN('You are in a desert with a pyramid to the west.');
               END;
      100    : BEGIN
               WRITELN('Darkness descends.');
               END;
      200    : BEGIN
               WRITELN('The door opens, and Reality comes leaping in on');
               WRITELN('this fantasy world! Reality engulfs you!! You');
               WRITELN('cannot get out!!! The shock is so terrible, you');
               WRITELN('die!!!!'); WRITELN;
               LOCATION := 100
               END;
     OTHERWISE ;

    END
    END;

    BEGIN {MAIN}

    GAME :=TRUE;
  WHILE GAME DO
   BEGIN
    INIT_GAME;
    REPEAT
    BASIC_LINES;
    CHANGE_LINES;
    IF (LOCATION<25) THEN
     IF (SEE AND(ROUTES[LOCATION,5]=-100))OR(ROUTES[LOCATION,5]<>-100)THEN
    GAD_ABOUT(LOCATION);
    IF LOCATION<>100 THEN
    CRUNCH_DATA;
    EXTRA;
    UNTIL DEAD OR QUIT OR WINNER;
    IF DEAD OR QUIT THEN
    SCORE;
    GAME :=FALSE;
    WRITELN('Would you like to play again? yes or no ');
    READLN(PRED);
      FOR I :=1 TO 3 DO
       IF (PRED[I] IN ['a'..'z']) THEN
        PRED[I]:=CHR(ORD(PRED[I])-32);
    IF PRED='YES' THEN GAME:=TRUE;
    END
    END.{MAIN}
