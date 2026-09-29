{ debug.inc - the port's hidden test bench (adventure --debug).  Not part of
  the 1980 game: with --debug, a line typed at any prompt that starts with '#'
  comes here instead of going to the game.  It exists so that the deep parts
  of the cave can be checked directly instead of by hoping a scripted
  walk-through survives the dwarves.

    #goto N          stand at location N (the game then does a LOOK)
    #take N          carry object N (from wherever it is); treasures are
                     marked as found the way WHATSHERE would
    #put N LOC       put object N at location LOC (0 destroys it)
    #prop N V        PROP[N]:=V
    #where N         show PLACE, FIXED and PROP of object N
    #dwarf I LOC     move dwarf I (6 is the pirate); LOC 0 kills him
    #set NAME V      CLOCK1 CLOCK2 LIMIT TURNS DFLAG TALLY TALLY2 NUMDIE
                     BONUS FOOBAR KNFLOC DKILL ABBNUM SEED
    #show            where we are and what the counters say }

FUNCTION ADVDEBUG(CONST CMD:STRING):STRING;
VAR W:ARRAY[1..3] OF STRING;
    V:ARRAY[2..3] OF LONGINT;
    N,P,CODE:INTEGER;
    S:STRING;

PROCEDURE SPLIT;
VAR N:INTEGER;
BEGIN
    S:=CMD;
    FOR N:=1 TO 3 DO BEGIN
	WHILE (LENGTH(S)>0)AND(S[1]=' ') DO DELETE(S,1,1);
	P:=POS(' ',S);
	IF P=0 THEN P:=LENGTH(S)+1;
	W[N]:=COPY(S,1,P-1);
	DELETE(S,1,P);
    END;
    FOR N:=1 TO LENGTH(W[1]) DO W[1][N]:=UPCASE(W[1][N]);
    FOR N:=1 TO LENGTH(W[2]) DO W[2][N]:=UPCASE(W[2][N]);
    FOR N:=2 TO 3 DO BEGIN
	VAL(W[N],V[N],CODE);
	IF CODE<>0 THEN V[N]:=0
    END
END;

FUNCTION OBJOK(X:LONGINT):BOOLEAN;
VAR OK:BOOLEAN;
BEGIN
    OK:=(X>=1)AND(X<=MAXTRS);
    IF NOT OK THEN WRITELN('# object numbers are 1..',MAXTRS);
    OBJOK:=OK
END;

BEGIN
    ADVDEBUG:='';
    SPLIT;
    IF W[1]='#GOTO' THEN BEGIN
	IF (V[2]<1)OR(V[2]>LOCSIZ) THEN WRITELN('# locations are 1..',LOCSIZ)
	{ the game cannot get outside once the cave is closing, and loops if put there }
	ELSE IF CLOSING AND (V[2]<9) THEN WRITELN('# not outside while the cave is closing')
	{ forced-motion locations are messages, not places: nobody ever stands in one }
	ELSE IF FORCED(V[2]) THEN WRITELN('# ',V[2]:1,' is a forced-motion location, not a place')
	ELSE BEGIN
	    LOCATION:=V[2]; NEWLOC:=LOCATION; OLDLOC:=LOCATION; OLDLC2:=LOCATION;
	    ADVDEBUG:='LOOK'
	END
    END
    ELSE IF W[1]='#TAKE' THEN BEGIN
	IF OBJOK(V[2]) THEN BEGIN
	    IF NOT TOTING(V[2]) THEN CARRY(V[2]);
	    IF PROP[V[2]]<0 THEN BEGIN
		PROP[V[2]]:=0;
		IF (V[2]=RUG)OR(V[2]=CHAIN) THEN PROP[V[2]]:=1;
		TALLY:=TALLY-1
	    END;
	    WRITELN('# carrying ',V[2]:1,', holding ',HOLDING:1,', tally ',TALLY:1)
	END
    END
    ELSE IF W[1]='#PUT' THEN BEGIN
	IF OBJOK(V[2]) AND (V[3]>=0) AND (V[3]<=LOCSIZ) THEN BEGIN
	    IF TOTING(V[2]) THEN DROP(V[2],V[3]) ELSE MOVE(V[2],V[3]);
	    WRITELN('# object ',V[2]:1,' now at ',PLACE[V[2]]:1)
	END
    END
    ELSE IF W[1]='#PROP' THEN BEGIN
	IF OBJOK(V[2]) THEN BEGIN
	    PROP[V[2]]:=V[3];
	    WRITELN('# PROP[',V[2]:1,']=',PROP[V[2]]:1)
	END
    END
    ELSE IF W[1]='#WHERE' THEN BEGIN
	IF OBJOK(V[2]) THEN WRITELN('# object ',V[2]:1,': PLACE ',PLACE[V[2]]:1,
	    ' FIXED ',FIXED[V[2]]:1,' PROP ',PROP[V[2]]:1)
    END
    ELSE IF W[1]='#DWARF' THEN BEGIN
	IF (V[2]>=1)AND(V[2]<=6)AND(V[3]>=0)AND(V[3]<=LOCSIZ) THEN BEGIN
	    DLOC[V[2]]:=V[3]; ODLOC[V[2]]:=V[3];
	    WRITELN('# dwarf ',V[2]:1,' at ',V[3]:1)
	END ELSE WRITELN('# #dwarf 1..6 0..',LOCSIZ)
    END
    ELSE IF W[1]='#SET' THEN BEGIN
	S:=W[2];
	IF ((S='CLOCK1')OR(S='CLOCK2'))AND(LOCATION<15) THEN BEGIN
	    WRITELN('# the closing clocks only run inside the cave (location 15 on)');
	    S:='-'
	END
	ELSE IF S='CLOCK1' THEN CLOCK1:=V[3]
	ELSE IF S='CLOCK2' THEN CLOCK2:=V[3]
	ELSE IF S='LIMIT' THEN LIMIT:=V[3]
	ELSE IF S='TURNS' THEN TURNS:=V[3]
	ELSE IF S='DFLAG' THEN DFLAG:=V[3]
	ELSE IF S='TALLY' THEN TALLY:=V[3]
	ELSE IF S='TALLY2' THEN TALLY2:=V[3]
	ELSE IF S='NUMDIE' THEN NUMDIE:=V[3]
	ELSE IF S='BONUS' THEN BONUS:=V[3]
	ELSE IF S='FOOBAR' THEN FOOBAR:=V[3]
	ELSE IF S='KNFLOC' THEN KNFLOC:=V[3]
	ELSE IF S='DKILL' THEN DKILL:=V[3]
	ELSE IF S='ABBNUM' THEN ABBNUM:=V[3]
	ELSE IF S='SEED' THEN BEGIN SEED:=V[3]; STRTRAN:=FALSE END
	ELSE S:='';
	IF S='' THEN WRITELN('# no such variable')
	ELSE IF S<>'-' THEN WRITELN('# ',S,'=',V[3]:1)
    END
    ELSE IF W[1]='#SHOW' THEN BEGIN
	WRITELN('# loc ',LOCATION:1,' oldloc ',OLDLOC:1,' oldlc2 ',OLDLC2:1,
	    '  turns ',TURNS:1,' limit ',LIMIT:1,' holding ',HOLDING:1);
	WRITELN('# tally ',TALLY:1,' tally2 ',TALLY2:1,' clock1 ',CLOCK1:1,' clock2 ',CLOCK2:1,
	    '  closing ',CLOSING,' panic ',PANIC,' closed ',CLOSED);
	WRITE('# dflag ',DFLAG:1,' dkill ',DKILL:1,' numdie ',NUMDIE:1,' seed ',SEED:1,'  dwarves');
	FOR N:=1 TO 6 DO WRITE(' ',DLOC[N]:1);
	WRITELN;
	WRITE('# carrying');
	FOR N:=1 TO MAXTRS DO IF TOTING(N) THEN WRITE(' ',N:1);
	WRITELN
    END
    ELSE WRITELN('# #goto #take #put #prop #where #dwarf #set #show')
END;
