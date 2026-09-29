100 CLS
105 GOTO 200
110 PRINT "Is your 3.5" + CHR$(34) + " disk in drive A or B?";
120 INPUT "", A$
150 IF A$ <> "A" AND A$ <> "B" THEN 120
160 PRINT A$
200 PRINT "Welcome to Adventure! Would you like instructions?"
210 PRINT : INPUT "", D$
400 IF D$ <> "Y" AND D$ <> "YES" THEN L1 = INT(25 * RND(1) + 420): GOTO 990
500 PRINT "Somewhere nearby is Colossal Cave, where some have found fortunes in"
510 PRINT "treasure and gold, though it is rumored that some who enter are never"
520 PRINT "seen again. I will be your eyes and hands. Direct me with natural"
530 PRINT "one- or two-word English commands and I will tell you what I see and"
540 PRINT "describe what is happening around you."
600 PRINT
610 PRINT "Some common instructions include:"
620 PRINT "GET - To pick up an object"
630 PRINT "DROP - self-explanatory"
635 PRINT "INVEN - to check what you have"
640 PRINT "SCORE - to check your you-know-what"
645 PRINT "QUIT - to end the game"
650 PRINT "SAVE - to save your current game"
660 PRINT "RESUME - to restore a previously saved game"
665 PRINT "******************************************************"
670 PRINT "* IMPORTANT!! YOU MUST TYPE IN ALL CAPS!!!!!!!!!!!!! *"
675 PRINT "******************************************************"
680 PRINT "There are a few other commands that are self-explanatory, plus a"
690 PRINT "few words you'll discover. Remember - every object has a purpose."
700 PRINT "you must take all the treasures back to the building without getting"
710 PRINT "killed by dwarves or falling into a pit. Good luck RP1!"
950 L1 = INT(25 * RND(1) + 1000)
990 FOR X = 1 TO 1000: NEXT X
1000 N$ = "LGHT"
1010 OPEN N$ FOR OUTPUT AS #1
1011 PRINT #1, L1, A$
1022 CLOSE #1
1300 RUN "C:\ADV751\ADV751"
2000 END

