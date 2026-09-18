{ wizhint.inc - under -u the game answers its own wizard test.  Not part of
  the 1980 game.

  The test (WIZARD in ASK.PAS) has two secrets.  The magic word is DWARF as
  shipped, but maintenance mode lets a wizard change it, and a changed one is
  otherwise unrecoverable.  The reply to the five random letters depends on the
  magic number and on the hour, so it goes stale when the hour turns - between
  reading it and typing it, if you are unlucky.  With -u the game says both as
  it asks, the way the TOPS-10 350-point port does:

      Prove it!  Say the magic word!
       -u: the magic word is DWARF
      ...
      XJNBV
       -u: the reply is HTXLF

  This is not a second implementation of the rule.  CHKMAGIC (WIZMAG.PAS)
  works out the expected reply in order to check it; OMSIWIZREPL below is that
  loop, statement for statement, with the hour passed in instead of read from
  the clock.  If a wizard changes the magic number the hint and the check move
  together.  Nothing is printed, and nothing changes, without -u. }

PROCEDURE OMSIWIZWORD(VAR MAGICWORD:WORD);
BEGIN
    IF OMSIUNLIMITED THEN WRITELN(' -u: the magic word is ',MAGICWORD)
END;

PROCEDURE OMSIWIZREPL(VAR MAGICWORD:WORD;VAR MAGICNUMBER:ARR5;MAGICTIME:INTEGER;VAR WRDX:WORD);
VAR I:INTEGER;
BEGIN
    FOR I:=1 TO 5 DO WRDX[I]:=CHR(ORD('A')+((ORD(MAGICWORD[I])-ORD('A')+
    MAGICNUMBER[I]+MAGICTIME) MOD 26));
END;

{ MAGICWORD is by now the random challenge WRTMAGIC has just written.  On a live
  clock, in the last ten minutes of the hour, also say what the reply turns into
  when the hour does; with the clock frozen (-t) it cannot go stale. }
PROCEDURE OMSIWIZREPLY(VAR MAGICWORD:WORD;VAR MAGICNUMBER:ARR5);
VAR WRDX:WORD;
    H,HN:INTEGER;
BEGIN
    IF OMSIUNLIMITED THEN BEGIN
	H:=TRUNC(TIME);
	OMSIWIZREPL(MAGICWORD,MAGICNUMBER,H,WRDX);
	WRITELN(' -u: the reply is ',WRDX);
	IF (NOT OMSIFROZEN) AND (TRUNC((TIME-H)*60.0)>=50) THEN BEGIN
	    HN:=(H+1) MOD 24;
	    OMSIWIZREPL(MAGICWORD,MAGICNUMBER,HN,WRDX);
	    WRITE(' -u: from ');
	    IF HN<10 THEN WRITE('0');
	    WRITELN(HN:1,':00 it becomes ',WRDX)
	END
    END
END;
