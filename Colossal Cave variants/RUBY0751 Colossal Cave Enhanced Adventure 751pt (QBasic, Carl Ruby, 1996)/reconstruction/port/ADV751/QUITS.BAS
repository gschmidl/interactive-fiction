10 C$ = CHR$(34)
39 MP$ = "more points."
40 TA$ = "To achieve the next higher rating, you need"
50 N$ = "SCM"
100 OPEN N$ FOR INPUT AS #1
110 INPUT #1, SC, M
120 CLOSE #1
130 IF SC > 751 THEN SC = 751
5000 PRINT
5100 PRINT "Using"; M; "turns, you scored"; SC; "points out of a possible 751."
5110 IF SC < 51 THEN 5140
5111 IF SC > 50 AND SC < 101 THEN 5141
5112 IF SC > 100 AND SC < 151 THEN 5142
5113 IF SC > 150 AND SC < 201 THEN 5143
5114 IF SC > 200 AND SC < 251 THEN 5144
5115 IF SC > 250 AND SC < 318 THEN 5145
5116 IF SC > 317 AND SC < 331 THEN 5146
5119 IF SC > 330 AND SC < 350 THEN 5147
5120 PRINT "You did it! You got a perfect 350 points!"
5125 PRINT "Now how about a nice game of enhanced adventure? I think that it's"
5130 PRINT "a little more of a challenge, don't you?"
5135 SYSTEM
5140 PRINT "You are obviously a rank amateur. Better luck next time.": GOTO 5180
5141 PRINT "You may now be considered as a novice class adventurer.": GOTO 5181
5142 PRINT "You are now classified as an experienced adventurer.": GOTO 5182
5143 PRINT "You may now consider yourself a " + C$ + "seasoned adventurer" + C$ + ".": GOTO 5183
5144 PRINT "You have achieved " + C$ + "junior master" + C$ + " status.": GOTO 5184
5145 PRINT "Your score puts you in master adventurer class c.": GOTO 5185
5146 PRINT "Your score puts you in master adventurer class b.": GOTO 5186
5147 PRINT "Your score puts you in master adventurer class a.": GOTO 5187
5180 PRINT TA$; 51 - SC; MP$: GOTO 5199
5181 PRINT TA$; 101 - SC; MP$: GOTO 5199
5182 PRINT TA$; 151 - SC; MP$: GOTO 5199
5183 PRINT TA$; 201 - SC; MP$: GOTO 5199
5184 PRINT TA$; 251 - SC; MP$: GOTO 5199
5185 PRINT TA$; 318 - SC; MP$: GOTO 5199
5186 PRINT TA$; 331 - SC; MP$: GOTO 5199
5187 PRINT TA$; 350 - SC; MP$
5199 SYSTEM

