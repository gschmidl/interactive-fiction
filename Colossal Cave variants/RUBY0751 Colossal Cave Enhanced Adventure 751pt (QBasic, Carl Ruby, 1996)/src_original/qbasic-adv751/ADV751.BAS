0 DIM TX$(90), DI(246, 10), A$(50), CO(12), CI(12), PC(12), I$(109), J(135), K$(135), B$(141), W$(99), P$(100), NU(239), V(15), BB$(15), TILE$(12)
1 REM  ************************************
2 REM  *                                  *
3 REM  * COLOSSAL CAVE ENHANCED ADVENTURE *
4 REM  *       (751-point Version)        *
5 REM  *                                  *
6 REM  *   Adapted from Crowther/Woods    *
7 REM  *     Expanded by David Long       *
8 REM  *   (Partial version for QBASIC    *
9 REM  *      Written by Carl Ruby)       *
10 REM *                                  *
11 REM ************************************
12 C$ = CHR$(34)
13 CD$ = "C:\ADV751\"
14 C1$ = "C:\ADV751\1TIMERS\"
15 DK$ = "I don't know how"
16 CLAM$ = "clam"
17 CI(2) = 2: CI(7) = 2: CI(12) = 1
18 FB$ = "You are being followed by a very large, tame bear."
19 NY$ = CD$ + "VAR"
20 TB$ = "There is a bottle of "
21 WB$(1) = "clear water": WB$(2) = "thick oil"
22 OPEN NY$ FOR INPUT AS #1
23 FOR Y = 1 TO 246: FOR X = 1 TO 10: INPUT #1, DI(Y, X): NEXT X, Y
24 FOR X = 1 TO 141: INPUT #1, B$(X): NEXT X
25 FOR X = 1 TO 86: INPUT #1, P$(X): NEXT X
26 FOR X = 1 TO 10: INPUT #1, MZ$(X): NEXT X
27 FOR X = 0 TO 15: INPUT #1, BB$(X): NEXT X
28 FOR X = 0 TO 3: INPUT #1, TR$(X): NEXT X
29 INPUT #1, TH$
30 INPUT #1, NH$
31 FOR X = 1 TO 4: INPUT #1, HLTH$(X): NEXT X
32 FOR X = 1 TO 4: INPUT #1, H2$(X): NEXT X
33 FOR X = 1 TO 5: INPUT #1, A$(X): NEXT X
34 INPUT #1, A$(8)
35 FOR X = 10 TO 13: INPUT #1, A$(X): NEXT X
36 FOR X = 15 TO 17: INPUT #1, A$(X): NEXT X
37 FOR X = 21 TO 27: INPUT #1, A$(X): NEXT X
38 FOR X = 29 TO 32: INPUT #1, A$(X): NEXT X
39 FOR X = 34 TO 39: INPUT #1, A$(X): NEXT X
40 FOR X = 1 TO 12: INPUT #1, CO(X): NEXT X
41 FOR X = 1 TO 4: INPUT #1, WUMP$(X): NEXT X
42 FOR X = 2 TO 9: INPUT #1, I$(X): NEXT X
43 FOR X = 13 TO 23: INPUT #1, I$(X): NEXT X
44 FOR X = 25 TO 44: INPUT #1, I$(X): NEXT X
45 FOR X = 46 TO 61: INPUT #1, I$(X): NEXT X: INPUT #1, I$(63): INPUT #1, I$(64)
46 FOR X = 66 TO 71: INPUT #1, I$(X): NEXT X
47 FOR X = 73 TO 75: INPUT #1, I$(X): NEXT X
48 CLOSE #1: SV = INT(6 * RND(1) + 14)
49 NA$ = CD$ + "LGHT": OPEN NA$ FOR INPUT AS #1
50 INPUT #1, L1
51 CLOSE #1
52 P = 1: BR = 1: TR = 1: PL = 1: BE = 1: WB = 1: SN = 1: DD = 1: OI = 1
53 CH = 1: WI = 5: DM = 1: PS = 1: SD = 1: BD = 1: HLTH = 100: CAB = 2: DUM = 1
54 PC(1) = 2: PC(10) = 184: PC(11) = 153
55 READ A$(6), A$(7), A$(9), A$(14)
56 FOR X = 18 TO 20: READ A$(X): NEXT X
57 READ A$(28), A$(33)
58 FOR X = 40 TO 44: READ A$(X): NEXT X
59 READ I$(1), I$(10), I$(11), I$(12), I$(24), I$(45), I$(62), I$(65), I$(72)
60 FOR X = 76 TO 109: READ I$(X): NEXT X
63 FOR X = 1 TO 132: READ J(X): IF X < 45 THEN NU(J(X)) = 300
64 NEXT X
65 FOR X = 1 TO 132: READ K$(X): NEXT X
69 FOR X = 1 TO 94: READ W$(X): NEXT X
71 FOR X = 1 TO 15: READ V(X): NEXT X
72 FOR X = 1 TO 12: READ TILE$(X): NEXT X
73 FOR X = 1 TO 5: READ TILE(X): NEXT X
74 FOR X = 1 TO 7: READ EN(X): NEXT X
75 FOR X = 1 TO 7: READ EX(X): NEXT X
79 CC = INT(6 * RND(1) + 15)
80 FOR X = 1 TO 5: E(X) = INT(8 * RND(1) + 1): NEXT X: E(6) = INT(WI * RND(1) + 1): E(7) = INT(15 * RND(1) + 1)
81 IF P = 149 OR P = 63 OR P = 199 OR P = 151 THEN J(103) = P
82 GOSUB 1100: IF ZP = P THEN 9800
91 GOSUB 300
92 IF (P = 183 OR P = 200) AND PH = 0 THEN PRINT "The telephone is out of order."
93 IF P = 183 AND PH > 0 THEN PRINT "The telephone booth is empty.";
94 IF P = 183 AND PH = 2 THEN PRINT "  The phone is ringing."
95 PRINT
98 GOSUB 400: IF P < 202 THEN NU(P) = NU(P) + 1
99 GOSUB 1000
100 M = M + 1: E(8) = INT(3 * RND(1) + 1): TL = 0: SM = 0: AR = 0: D$ = "": M$ = "": K = 0: FOR X = 1 TO 10: R$(X) = "": NEXT X: IF LS > 0 THEN LA = LA + 1
101 IF J(76) = P THEN HLTH = HLTH - 4
102 IF J(76) = 300 THEN HLTH = HLTH - 9
103 IF HLTH < 100 AND J(76) <> P AND J(76) <> 300 THEN HLTH = HLTH + 5
104 IF HLTH > 100 THEN HLTH = 100
108 IF P = 140 OR P = 163 THEN J(122) = P
112 PRINT : INPUT "", D$: IF LEN(D$) = 0 THEN 100
113 IF J(119) > 0 AND D$ <> "GET KNIFE" THEN J(119) = 0
114 IF D$ = "NORTH" OR D$ = "SOUTH" OR D$ = "EAST" OR D$ = "WEST" OR D$ = "DOWN" OR D$ = "UP" OR D$ = "BUILDING" THEN D$ = LEFT$(D$, 1)
115 IF D$ = "D" AND P = 183 AND E(1) = 3 THEN P = 182: GOTO 80
117 IF (D$ = "D" AND (P = 8 OR P = 9) OR (D$ = "IN" AND P = 8)) AND GA = 0 THEN AR = 2: GOTO 2450
118 IF D$ = "D" AND P = 14 AND J(1) = 300 THEN 800
119 IF DD = 1 AND ((D$ = "S" AND P = 73) OR (D$ = "W" AND P = 19)) THEN RR = P
120 IF J(31) = 23 AND ((D$ = "D" AND P = 15) OR (D$ = "S" AND P = 65) OR (D$ = "E" AND P = 67) OR (D$ = "D" AND P = 67)) THEN RR = P
121 IF D$ = "N" AND (P = 18 OR P = 83) THEN PRINT "You have crawled through a very low wide passage parallel to and north of the hall of mists."
122 IF D$ = "N" AND P = 63 AND LT = 0 THEN 2795
123 IF D$ = "U" AND P = 15 AND J(1) = 300 THEN PRINT "The dome is unclimbable.": GOTO 100
124 IF ((D$ = "E" AND P = 24) OR (D$ = "W" AND P = 11)) AND ((H > 1) OR (H = 1 AND J(8) <> 300)) THEN 4750
125 IF P = 23 AND J(31) = 23 AND ((D$ = "N" AND RR = 15) OR (D$ = "W" AND RR <> 67) OR (D$ = "U" AND RR = 65) OR (D$ = "S")) THEN PRINT : AR = 3: GOTO 9133
126 IF P = 79 AND DD = 1 AND (D$ = "N" AND RR = 19) OR (D$ = "E" AND RR = 73) THEN PRINT : PRINT "The dragon looks rather nasty. You'd best not try to get by.": GOTO 80
127 IF P <> 23 AND P <> 79 AND P <> 15 AND P <> 73 AND P <> 65 AND P <> 19 AND P <> 67 THEN RR = 0
128 IF D$ = "NW" AND P = 33 AND E(1) <> 3 AND E(1) <> 5 THEN 9500
129 IF D$ = "SW" AND TR = 0 AND P = 51 AND J(29) = 300 THEN 2945
130 IF D$ = "SW" AND TR = 2 AND ((P = 50 AND Y = 4) OR (P = 51 AND Y = 1)) THEN TR = 3: GOTO 80
131 IF ((D$ = "W" AND P = 226) OR (D$ = "E" AND P = 193)) AND LS > 0 THEN M$ = "TOOBRITE": GOSUB 8900: GOTO 80
132 IF D$ = "D" AND P = 227 AND J(79) <> 300 THEN M$ = "TOOCOLD": GOSUB 8900: GOTO 80
133 IF D$ = "D" AND P = 44 AND NU(208) = 0 THEN M$ = "DRAGGED": GOSUB 8900
134 IF (D$ = "U" OR D$ = "D" OR D$ = "N" OR D$ = "S") AND P = 76 AND E(1) <> 3 THEN 230
135 IF D$ = "U" AND P = 76 AND E(1) = 3 AND E(2) = 3 THEN P = 31: GOTO 80
136 IF D$ = "N" AND P = 144 AND LK = 0 THEN 3630
137 IF D$ = "L" THEN PRINT LA, LS, L1, L2: GOTO 100
138 IF P = 62 AND E(1) <> 3 AND LEN(D$) = 1 THEN CW = 1
139 IF CW = 1 THEN 230
141 IF D$ = "DDD" THEN FOR X = 1 TO 12: PRINT CI(X); " "; : NEXT X: PRINT : GOTO 100
143 IF M$ = "BUILDING" THEN 1800
145 FOR X = 1 TO LEN(D$)
146 IF MID$(D$, X, 1) = " " THEN M$ = MID$(D$, X + 1, LEN(D$)): D$ = LEFT$(D$, X - 1)
147 NEXT X
148 IF M$ = "" THEN K = 0
149 IF M$ = "GAUGE" THEN K = 127
152 GOSUB 9150
153 IF D$ = "OIL" AND P = 63 THEN 3900
154 FOR X = 1 TO 132: IF D$ = K$(X) THEN KD = X
155 NEXT X
156 IF KD = 0 THEN 170
158 IF J(KD) = P OR J(KD) = 300 THEN PRINT "What do you want to do with the " + K$(KD) + "?": KD = 0: GOTO 100
160 IF J(KD) <> P AND J(KD) <> 300 THEN 290
169 KD = 0
170 FOR X = 1 TO 94: IF D$ = W$(X) THEN W = X
174 NEXT X
175 IF M$ = "ALL" OR M$ = "BILLBOARD" THEN 180
179 IF W = 0 AND D$ <> "" THEN 5320
180 IF W > 28 OR W = 5 OR W = 7 OR W = 26 OR (W > 13 AND W < 28) THEN 190
185 IF M$ = "" THEN PRINT D$; " What?": INPUT "", M$
190 IF W < 31 OR W > 34 THEN FE = 0
195 IF W > 0 AND W < 4 THEN GOSUB 9600
196 IF W > 0 AND W < 11 THEN ON W GOTO 2000, 2100, 2000, 2000, 3000, 2200, 2300, 2800, 3200, 2700
197 IF W > 10 AND W < 21 THEN ON W - 10 GOTO 2700, 4200, 4100, 4800, 4890, 2500, 2600, 2400, 2400, 2460
198 IF W > 20 AND W < 31 THEN ON W - 20 GOTO 2460, 2900, 3500, 3900, 3200, 4500, 4300, 4600, 3100, 3800
199 IF W > 30 AND W < 41 THEN ON W - 30 GOTO 3700, 3710, 3720, 3730, 4000, 4000, 4400, 3620, 3620, 4700
200 IF W > 40 AND W < 51 THEN ON W - 40 GOTO 3300, 3330, 3600, 6600, 9020, 6000, 7000, 7010, 6800, 7700
201 IF W > 50 AND W < 61 THEN ON W - 50 GOTO 7900, 6900, 2915, 2915, 2915, 4950, 4960, 5500, 700, 700
202 IF W > 60 AND W < 71 THEN ON W - 60 GOTO 700, 700, 700, 700, 700, 700, 700, 700, 3000, 5300
203 IF W > 70 AND W < 81 THEN ON W - 70 GOTO 5300, 5300, 5600, 5700, 5700, 5400, 5800, 5800, 5900, 4900
204 IF W > 80 AND W < 91 THEN ON W - 80 GOTO 3400, 3350, 3450, 9020, 5000, 6650, 5200, 6110, 6120, 1820
205 IF W > 90 AND W < 101 THEN ON W - 90 GOTO 1800, 1800, 1800, 9005
209 IF M$ = "" AND D$ <> "" THEN 5320
210 IF (P = 47 AND D$ = "W") OR (P = 43 AND D$ = "NE") OR (P = 24 AND D$ = "W") THEN 214
211 IF P > 119 OR (P = 11 AND D$ = "PLOVER") THEN 214
213 IF ((LS = 0 AND P > 11) OR (P > 11 AND J(47) <> P AND J(47) <> 300)) AND E(8) <> 3 THEN 600
214 WI = 5: GOTO 80
230 CW = 0: GOTO 9500
290 REM lowercase
291 FOR X = 1 TO LEN(K$(KD))
292 LC$ = LC$ + CHR$(ASC(MID$(K$(KD), X, 1)) + 32)
293 NEXT X
295 PRINT "I see no "; LC$; " here."
296 LC$ = ""
298 KD = 0: GOTO 100
300 IF J(97) = 300 THEN PRINT : PRINT FB$
304 IF P > 62 AND P < 84 THEN PRINT "You are " + A$(P - 62): GOTO 398
305 IF P > 83 AND P < 88 THEN PRINT A$(P - 62): GOTO 398
306 IF P > 89 AND P < 100 THEN PRINT "You are in a " + MZ$(P - 89) + " passages, all different.": GOTO 398
307 IF P = 89 OR P > 290 THEN PRINT "Dead end": GOTO 398
308 IF P > 99 AND P < 120 THEN PRINT "You are " + A$(6): GOTO 398
309 IF P = 120 THEN PRINT "You're in the caretaker's storeroom.": GOTO 398
310 IF P = 126 OR P = 135 OR P = 136 THEN 340
311 IF P > 221 THEN PRINT A$(P - 196): IF P = 237 AND DI(237, 7) = 516 THEN PRINT "A NE passage is blocked by an impenetrable thicket of sharp thorny": PRINT "brambles."
312 IF P > 221 THEN 398
314 IF NU(P) = 300 THEN NU(P) = 0: SC = SC + 2
315 IF NU(P) = 254 THEN NU(P) = 0
316 IF NU(P) = 0 THEN 380
318 IF NU(P) > 1 AND LD = 1 THEN LD = 0: GOTO 380
340 REM ---++ B$ AREA ------>
345 IF P = 88 THEN PRINT "You're at window on pit.": GOTO 398
349 IF P = 6 OR (P > 120 AND P < 126) THEN PRINT "You're in forest.": GOTO 398
350 IF P < 63 THEN PRINT "You're " + B$(P): GOTO 398
356 IF P = 126 OR P = 127 OR P = 140 OR P = 141 OR P = 150 OR P = 154 OR P = 155 THEN PRINT "You are " + B$(P - 63): GOTO 398
360 IF P > 125 AND P < 166 THEN PRINT "You're " + B$(P - 63): GOTO 398
363 IF P = 172 THEN PRINT "You are " + B$(P - 67): GOTO 398
365 IF P > 169 AND P < 179 THEN PRINT "You're " + B$(P - 67): GOTO 398
366 IF P = 199 AND PH = 1 THEN PRINT "You're at underground sea.": GOTO 398
368 IF P > 179 AND P < 202 THEN PRINT "You're " + B$(P - 68): GOTO 398
380 REM ---++ A$ AREA ------>
381 IF P = 27 AND BH = 1 THEN M$ = "RMS\ALT27": GOTO 389
382 IF P = 139 AND WET = 1 THEN M$ = "RMS\ALT139": GOTO 389
383 IF P = 199 AND PH = 1 THEN M$ = "RMS\ALT199": GOTO 389
384 IF P < 63 OR P = 88 OR P > 120 THEN 388
385 GOTO 398
388 M$ = "RMS\ROOM" + MID$(STR$(P), 2, LEN(STR$(P)))
389 GOSUB 8900
398 RETURN
400 FOR X = 2 TO 9: PC(X) = P: NEXT X: PC(12) = P
500 IF LS > 0 THEN I$(47) = "There is a lamp shining nearby."
503 IF BE = 1 AND J(99) = 59 THEN M$ = "GENTLE": NI = 99: GOSUB 8950
504 IF BE <> 1 OR (BE = 1 AND J(99) <> 59) THEN I$(58) = "There is a little axe here."
505 IF CH = 1 AND BE <> 1 THEN I$(5) = "There is a golden chain locked to the wall!"
506 IF CH = 0 THEN M$ = "CHAIN": NI = 5: GOSUB 8950
510 FOR X = 1 TO 109
512 IF X = 95 AND J(X) = P AND (P = 141 OR P = 161) THEN PRINT "A helicopter is waiting nearby.": GOTO 520
513 IF X = 103 AND P <> 63 THEN 520
515 IF J(X) = P THEN PRINT I$(X)
517 IF X = 107 AND P = 39 AND PL = 2 THEN PRINT C$; "Water!! Water!!"; C$
520 NEXT X
530 IF P = 89 AND J(14) = 89 THEN PV = 2
599 RETURN
600 PRINT "You fell into; A; pit AND broke; every; bone; in; your; body!;"
620 SC = SC - 10
630 ZP = P: GOTO 9800
700 IF P > 290 THEN Q = DI(P - 50, W - 58): GOTO 710
708 Q = DI(P, W - 58)
710 IF Q = 0 THEN PRINT BB$(0): GOTO 80
712 IF Q > 500 THEN ON Q - 500 GOTO 9101, 9102, 9103, 9104, 9105, 9106, 9107, 9108, 9109, 9110, 9111, 9112, 9113, 9550, 9040, 9116, 9117, 9118, 9190, 2695, 9121, 9122
720 IF P = 188 AND Q <> P THEN BG = 0
730 P = Q
740 GOTO 80
800 PRINT "You are at the bottom of the pit with a broken neck."
810 ZP = 15: GOTO 9800
1000 IF P <> 2 OR PS = 1 THEN 1008
1002 PRINT "A steel safe is embedded in the wall."
1003 PRINT "The safe door"; : ON CI(1) + 1 GOTO 9380, 9370
1008 IF P = 8 OR P = 9 THEN J(58) = P: PRINT "The grate "; : ON GA + 1 GOTO 9380, 9370
1014 IF (P = 14 OR P = 15) AND J(1) <> 300 THEN PRINT "Rough stone steps lead "; BB$(P)
1017 IF (P = 17 OR P = 83) THEN J(39) = P: J(40) = P: IF CB = 1 THEN PRINT CB$
1025 IF P = 25 AND E(1) = 3 THEN PRINT "A hollow voice says " + C$ + "Plugh" + C$
1030 IF (P = 30 OR P = 88) AND LS > 0 THEN J(108) = P
1036 IF (P = 36 OR P = 37) AND PL > 1 THEN PRINT BB$(PL)
1043 IF P = 43 THEN PRINT C$ + "Congratulations on bringing light into the Dark-Room!!"; C$
1048 IF (P = 149 OR P = 150) AND SSD = 1 THEN 9215
1049 IF P = 149 THEN ON LGT + 1 GOTO 9200, 9205, 9210
1050 IF P = 50 OR P = 51 THEN M$ = "BRIDGE" + RIGHT$(STR$(BR), 1): GOSUB 8900: PRINT : PRINT TR$(TR): IF BR = 1 THEN J(39) = P
1088 IF P = 295 AND J(14) = 89 THEN PRINT C$ + "This is not the maze where the pirate leaves his treasure chest." + C$
1089 IF P = 295 THEN PRINT : PRINT C$ + "Insert coins to receive fresh batteries." + C$
1090 IF LS > 0 AND P <> 2 AND P < 10 AND E(3) = 3 AND J(47) = 300 THEN PRINT "You know, you are wasting your batteries by running around out here with        your light on."
1091 IF P = 62 THEN AR = 6
1092 IF P > 99 AND P < 113 THEN AR = 5
1095 IF AR > 1 THEN 8000
1096 GOSUB 5850
1097 IF WUMP > 0 THEN GOSUB 5860
1099 RETURN
1100 IF E(3) = 5 AND D = 0 AND P > 12 AND P < 120 THEN SC = SC + 52: M$ = C1$ + "FIRSTAXE": GOSUB 8900: D = 1: J(58) = P: MD = M
1103 IF LS = 2 THEN 1113
1104 IF LA = L1 THEN 1300
1106 IF (P > 119 AND P < 220) OR P = 225 THEN 1113
1108 IF (LS = 0 AND P > 11 AND P <> 226) OR (LS > 0 AND P > 11 AND P <> 226 AND J(47) <> P AND J(47) <> 300) THEN 1388
1113 IF JT = 15 AND P > 9 AND J(25) = 62 AND SE = 0 THEN SV = SV - 1: IF SV = 0 THEN 1175
1115 IF PV = 2 OR P < 23 THEN 1150
1120 IF PV = 0 AND E(7) = 6 THEN PV = 1: GOTO 1200
1130 IF PV = 1 AND E(7) = 6 THEN PV = 0
1150 IF D = 0 OR P < 18 OR (JT = 15 AND J(25) = 62) THEN DW = 0: GOTO 1180
1155 IF E(7) = 9 THEN DW = DW + 1: IF DW > 2 THEN DW = 2
1160 IF PV = 1 THEN 1200
1170 IF DW > 0 THEN 1400
1175 IF SV = 0 AND SE = 0 THEN 1500
1180 IF SE = 1 THEN CC = CC - 1
1185 IF CC = 0 THEN 1600
1199 RETURN
1200 PRINT
1205 IF P = 89 OR SE = 1 OR (JT = 15 AND J(25) = 62) OR (P > 48 AND P < 60) OR (P > 119 AND P < 225) THEN RETURN
1210 FOR X = 1 TO 15
1215 IF J(X) = 300 THEN PT = 1
1220 NEXT X
1230 IF PT = 0 THEN PRINT "There are faint rustling noises from the darkness behind you.": RETURN
1240 FOR X = 1 TO 15
1245 IF J(X) = 300 THEN J(X) = 89: H = H - 1
1250 NEXT X
1255 J(14) = 89: NU(89) = 300
1260 PRINT "Out from the shadows behind you pounces a bearded pirate! " + C$ + "Har, har," + C$ + " he"
1261 PRINT "chortles, "; C$; : M$ = "PIRATE": GOSUB 8900
1265 PT = 0: E(7) = INT(12 * RND(1) + 1)
1270 PRINT : RETURN
1300 PRINT
1301 L2 = INT(6 * RND(1) + 14) + L1
1305 IF LA = L2 AND LS < 2 THEN 1380
1310 IF NB = 1 AND (J(31) = 300 OR J(31) = P) THEN 1350
1315 IF J(56) = 300 THEN 1350
1318 IF J(56) = 0 THEN NB = 1: M$ = C1$ + "DIMWRAP": GOSUB 8900: GOTO 100
1322 IF J(56) <> P THEN NB = 1: M$ = C1$ + "DIMGOBAK": GOSUB 8900: RETURN
1350 M$ = C1$ + "DIMRPLAC": GOSUB 8900
1360 LS = 2: J(56) = P: NB = 0: DM = 0: M$ = "OLDBTTRY": NI = 56: GOSUB 8950
1377 LA = 0: RETURN
1380 LS = 0: PRINT
1381 IF P < 12 THEN 1392
1384 PRINT "Your lamp has run out of power.": RO = 1
1388 IF P > 12 AND P <> 226 AND P <> 193 AND P <> 199 THEN PRINT BB$(6)
1390 IF P = 25 THEN PRINT : PRINT GH$: GOTO 100
1392 M$ = "NOPOINT": GOSUB 8900
1399 GOTO 9985
1400 REM DWARF ROUTINE
1401 PRINT
1402 IF (P > 35 AND P < 40) OR (P > 46 AND P < 61) THEN RETURN
1404 IF P = 42 OR P = 63 OR P = 44 OR P = 72 OR P = 77 THEN RETURN
1406 IF M - MD < 3 OR P > 88 THEN RETURN
1408 IF SE = 1 OR (J(25) = 62 AND JT = 15) THEN RETURN
1409 IF DW > 0 THEN J(54) = P
1410 IF DW = 1 THEN PRINT "There is a threatening little dwarf in the room with you!": GOTO 1420
1415 IF DW > 1 THEN PRINT "There are"; DW; "threatening little dwarves in the room with you!"
1420 J(42) = P
1430 IF E(5) = 4 THEN RETURN
1450 IF DW > 1 THEN 1460
1455 PRINT "One sharp nasty knife is thrown at you!"
1458 IF E(3) = 2 THEN PRINT "It gets you!": ZP = P: RETURN
1459 PRINT "It misses!": J(119) = P: RETURN
1460 PRINT "2 of them throw knives at you!"
1465 IF E(3) = 2 THEN PRINT "Both of them get you!": ZP = P: RETURN
1470 PRINT "None of them hit you!": J(119) = P: RETURN
1500 PRINT "A sepulchral voice reverberating through the cave, says,"; C$; "Cave closing"
1510 M$ = C1$ + "VOICE1": GOSUB 8900
1520 JT = 0: SC = SC + 25: SE = 1: SV = 1: DI(9, 6) = 520: RETURN
1600 PRINT "The sepulchral voice entones, " + C$ + "The cave is now closed." + CHR$(34) + " As the"
1610 M$ = C1$ + "CLOSED!": GOSUB 8900
1630 SC = SC + 19: CC = 1: SE = 0: GOSUB 9991: STOP
1800 IF P < 8 THEN P = 2: GOTO 80
1810 GOTO 9020
1820 IF P = 33 THEN Q = 35: GOTO 210
1830 GOTO 9020
1900 FOR X = 1 TO 83: IF J(X) <> P THEN 1930
1910 PRINT P$(X)
1915 IF H = 8 THEN GOSUB 9147: GOTO 1930
1920 J(X) = 300: H = H + 1
1921 IF X = 71 AND P = 2 AND PS = 1 THEN PS = 0: M$ = C1$ + "HIDDEN": GOSUB 8900: M$ = "POSTER": NI = 71: GOSUB 8950: J(131) = 2: GOTO 1930
1929 PRINT "    Taken."
1930 NEXT X: GOTO 100
2000 REM *GET IT*
2001 IF M$ = "ALL" THEN 1900
2002 GOSUB 9600: IF J(K) < 300 THEN 2007
2003 IF J(K) = 400 THEN 3425
2004 IF J(K) > 300 AND P = PC(J(K) - 300) THEN 2051
2007 IF K = 71 AND PS = 1 AND P = 2 THEN PS = 0: M$ = C1$ + "HIDDEN": GOSUB 8900: M$ = "POSTER": NI = 71: GOSUB 8950: GOTO 2090
2008 IF J(K) = P THEN 2051
2010 IF J(K) = 300 THEN PRINT "You are already carrying it!": GOTO 100
2050 KD = K: K = 0: M$ = "": GOTO 290
2051 IF H = 8 THEN GOSUB 9147: GOTO 100
2052 IF M$ = "BEAR" AND BE = 2 AND CH = 0 THEN BE = 3: M$ = "CONTENTED": NI = 42: GOSUB 8950
2053 IF M$ = "BEAR" AND BE = 1 THEN 9040
2054 IF M$ = "MIRROR" THEN PRINT "It is too far up for you to reach.": GOTO 100
2055 IF M$ = "KNIFE" THEN PRINT "The dwarves' knives vanish as they strike the walls of the cave.": J(49) = 0
2056 IF K > 100 THEN 9070
2057 IF M$ = "BIRD" AND J(23) <> 300 THEN PRINT "You can catch the bird, but you cannot carry it.": GOTO 100
2058 IF K = 79 THEN PRINT "The cloak is stuck tight under the rocks.  You'll probably have to": PRINT "yank it out.": GOTO 100
2059 IF M$ = "CAGE" AND J(24) = P THEN J(24) = 300: H = H + 1
2060 IF M$ = "BEAR" AND BE > 1 AND CH = 1 THEN PRINT "The bear is still chained to the wall.": GOTO 100
2062 IF K = 4 AND DD = 1 THEN PRINT BS$: GOTO 100
2063 IF M$ = "AXE" AND BE = 1 AND P = 59 THEN 9080
2064 IF K = 5 AND BE = 1 THEN 2799
2065 IF K = 5 AND CH = 1 THEN PRINT "The chain is still locked.": GOTO 100
2090 H = H + 1: J(K) = 300: PRINT "Taken."
2092 IF K = 52 OR K = 53 THEN DI(60, 2) = 513
2095 IF K < 20 AND P = 2 THEN SC = SC - V(K): JT = JT - 1
2096 IF M$ = "MAGAZINE" AND P = 62 THEN SC = SC - 1
2099 GOTO 100
2100 REM -> DROP
2101 IF J(K) = P THEN PRINT "You aren't carrying it!": GOTO 100
2102 IF J(K) = 300 THEN 2110
2103 IF M$ = "POWDER" AND J(43) = 252 THEN 2115
2109 GOTO 2050
2110 J(K) = P: H = H - 1
2111 IF K = 12 AND J(24) <> P THEN PO = P: M$ = C1$ + "VASEDROP": GOSUB 8900: M$ = "SHARDS": NI = 12: GOSUB 8950: GOTO 100
2112 IF K = 12 AND J(24) = P THEN PRINT "The vase is now resting, delicately, on a velvet pillow.": GOTO 2140
2113 IF K = 3 AND P = 124 THEN J(K) = 0: J(26) = P: PRINT I$(26): GOTO 100
2115 IF M$ = "POWDER" AND CLOTH = 0 THEN PRINT BB$(18) + "cloth bag.": GOTO 100
2117 IF M$ = "POWDER" AND P = 139 AND WET = 0 THEN WET = 1: GOSUB 8900: M$ = "ALT139": DI(139, 1) = 157: GOSUB 8900: GOTO 100
2120 IF M$ = "BIRD" AND CAGE = 0 THEN PRINT "You can't get at the little bird.": GOTO 100
2122 IF M$ = "BIRD" AND P = 23 AND SN = 1 THEN SN = 0: J(31) = 0: GOTO 2180
2125 IF M$ = "BIRD" AND P = 79 AND DD = 1 THEN M$ = C1$ + "FLURRY2": GOSUB 8900: J(19) = 0: GOTO 100
2126 IF M$ = "MAGAZINE" AND P = 62 THEN SC = SC + 1
2127 IF M$ = "BEAR" AND P = 51 AND TR = 1 THEN TR = 0: M$ = C1$ + "LUMBERS": GOSUB 8900: GOTO 100
2128 IF M$ = "CAGE" AND J(48) = 300 THEN J(48) = P: H = H - 1
2130 IF W = 2 THEN PRINT "Dropped."
2131 IF W = 6 THEN PRINT "Thrown."
2132 IF K = 52 OR K = 53 THEN DI(60, 2) = 32
2140 IF K < 16 AND P = 2 THEN SC = SC + V(K): JT = JT + 1
2170 GOTO 100
2180 M$ = C1$ + "FLURRY1": GOSUB 8900: GOTO 100
2200 REM +> THROW
2205 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
2230 IF K < 20 AND (P = 50 OR P = 51) AND TR = 1 THEN J(K) = 0: H = H - 1: PRINT : PRINT "The troll catches your treasure and scurries away out of sight.": TR = 2: GOTO 100
2240 IF M$ = "AXE" THEN 2251
2250 GOTO 2100
2251 IF TR = 1 AND (P = 50 OR P = 51) THEN 2270
2253 IF P = 79 AND DR = 1 THEN 2280
2255 IF P = 23 AND SN = 1 THEN PRINT "Attacking the snake both doesn't work and is very dangerous.": GOTO 80
2257 IF P = 59 AND BE = 1 THEN M$ = C1$ + "AXEMISS": GOSUB 8900: M$ = "AXE": NI = 58: GOSUB 8950: GOTO 2275
2260 IF J(54) = P THEN 2285
2265 PRINT TH$ + "attack.": GOTO 100
2270 M$ = "TROLLAXE": GOSUB 8900
2275 IF J(58) = 300 THEN H = H - 1
2276 J(58) = P: GOTO 100
2280 PRINT "The axe bounces harmlessly off the dragon's thick scales.": GOTO 2275
2285 IF E(2) = 4 THEN PRINT "You attack a little dwarf, but he dodges out of the way.": IF E(1) = 3 THEN DW = DW + 1
2290 DW = DW - 1: IF DW = 0 THEN J(54) = 0
2295 PRINT "You killed a little dwarf. The body vanishes in a cloud of greasy black smoke.": GOTO 80
2300 GOSUB 9600: IF J(K) = P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
2305 IF P = 79 AND DD = 0 THEN PRINT "For crying out loud, the poor thing is already dead!": GOTO 100
2306 IF P = 79 THEN 2360
2310 IF TR = 1 AND (P = 50 OR P = 51) THEN PRINT BB$(8): GOTO 100
2320 IF (J(52) = P OR J(53) = P) AND J(35) <> P THEN PRINT "The shell is very strong and impervious to attack.": GOTO 100
2330 IF J(54) = P THEN PRINT BB$(4): GOTO 100
2340 IF P = 23 AND SN = 1 THEN 2255
2345 IF (J(49) = P OR J(49) = 300) AND M$ = "BIRD" THEN 2390
2350 IF P = 59 AND BE = 1 THEN 9070
2359 GOTO 2265
2360 PRINT WW$: INPUT "", D$: IF D$ = "Y" OR D$ = "YES" THEN 2380
2370 IF D$ = "N" OR D$ = "NO" THEN GOSUB 9000: GOTO 100
2375 GOTO 103
2380 DD = 0: M$ = C1$ + "VANQUISH": GOSUB 8900
2385 M$ = "DRAGON": NI = 30: GOSUB 8950
2388 GOTO 80
2390 IF J(49) = P THEN H = H - 1
2391 J(49) = 0: PRINT "The little bird is now dead. Its body disappears.": GOTO 100
2400 REM -> IN
2410 IF P = 8 AND GA = 1 THEN P = 9: GOTO 80
2415 IF P = 121 OR P = 141 OR P = 161 THEN HEP = P: P = 140: GOTO 80
2416 IF P = 144 AND LK = 0 THEN 3630
2417 IF P = 149 AND SSD = 1 THEN P = 150: GOTO 80
2420 IF P = 183 AND PH = 0 THEN P = 200: GOTO 80
2430 FOR X = 1 TO 7
2435 IF P = EN(X) THEN P = EX(X): INX = 1
2440 NEXT X
2444 IF INX = 1 THEN INX = 0: GOTO 80
2449 GOTO 2490
2450 GOTO 9101
2460 REM -> OUT
2461 IF P = 140 AND HEP < 161 THEN P = HEP + 20: J(95) = P: GOTO 80
2462 IF P = 140 AND HEP = 161 THEN P = 121: J(95) = 121: GOTO 80
2463 IF P = 120 THEN P = 2: GOTO 80
2464 IF P = 38 OR P = 39 THEN P = P - 2: GOTO 80
2465 IF P = 200 THEN P = 183: GOTO 80
2466 IF P = 150 THEN P = 149: GOTO 80
2470 FOR X = 1 TO 7
2475 IF P = EX(X) THEN P = EN(X): INX = 1
2476 NEXT X
2490 M$ = "INFRMOUT": GOSUB 8900: GOTO 80
2500 IF SE = 1 AND P = 47 THEN 2695
2501 IF P = 2 THEN P = 47: GOTO 80
2510 IF P = 47 THEN P = 2: GOTO 80
2590 GOTO 9030
2600 IF SE = 1 AND P = 25 THEN 2695
2601 IF P = 25 THEN P = 2: DW = 0: GOTO 80
2610 IF P = 2 THEN P = 25: GOTO 80
2690 GOTO 9030
2695 M$ = C1$ + "VOICE2": GOSUB 8900: PRINT C$; : M$ = C1$ + "VOICE2A": GOSUB 8900: GOTO 100
2700 REM >> OPEN
2701 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
2702 IF K = 131 THEN ON CI(1) + 1 GOTO 2797, 2798
2703 IF K = 132 AND CAB = 2 THEN CAB = 1: M$ = C1$ + M$: GOSUB 8900: J(109) = P: PRINT I$(109): GOTO 100
2704 IF K = 132 AND CAB = 0 THEN CAB = 1: GOSUB 9000: J(109) = P: GOTO 100
2705 OC = OC + 1: IF OC = 13 THEN 2709
2706 IF K <> (CO(OC)) THEN 2705
2707 IF CI(OC) <> 1 THEN CI(OC) = 1: GOSUB 9000: GOTO 100
2708 IF CI(OC) > 0 THEN 2798
2709 OC = 0: IF K = 0 THEN 2716
2710 IF K = 103 AND P = 63 THEN 2790
2711 IF K = 0 THEN 2716
2713 IF P = 8 OR P = 9 OR P = J(5) THEN 2718
2716 IF J(47) = P OR J(47) = 300 THEN 2750
2717 IF J(52) = P OR J(52) = 300 THEN 2760
2718 IF J(21) = P OR J(21) = 300 THEN 2720
2719 PRINT "You have no keys!": GOTO 100
2720 IF J(5) = P OR J(5) = 300 THEN 2730
2725 IF K = 43 THEN 2770
2729 PRINT TH$ + "lock or unlock.": GOTO 100
2730 IF P = 59 AND BE = 1 THEN 2799
2735 CH = 0: NI = 5: M$ = "CHAIN": GOSUB 8950: PRINT "The chain"; : GOTO 9360
2750 IF J(9) = P OR J(9) = 300 THEN 2755
2753 PRINT "You don't have anything strong enough to open the clam.": GOTO 100
2755 J(28) = J(27): J(27) = 0: J(15) = 72: M$ = C1$ + "PEARL": GOSUB 8900: CLAM$ = "oyster": GOTO 100
2760 M$ = "OYSTER": GOSUB 8900: GOTO 100
2770 GA = 1: PRINT "The grate";
2775 IF D$ = "UNLOCK" THEN 9360
2780 IF D$ = "OPEN" THEN 9370
2789 PRINT DK$; " to lock or unlock such a thing.": GOTO 100
2790 IF LT = 1 THEN GOSUB 9000
2795 PRINT "The door is extremely rusty and refuses to open.": GOTO 100
2797 PRINT "You don't have the combination.": GOTO 100
2798 PRINT "It's already open.": GOTO 100
2799 M$ = "PASTBEAR": GOSUB 8900: GOTO 100
2800 GOSUB 9600: IF J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
2810 IF K = 22 OR K = 23 OR K = 32 THEN PRINT IG$: GOTO 100
2820 IF M$ = "SNAKE" AND J(24) = 300 THEN J(24) = 0: H = H - 1: PRINT "The snake has now devoured your bird.": GOTO 100
2830 IF K = 101 OR K = 102 OR K = 97 THEN PRINT NH$ + "it wants to eat (except perhaps you).": GOTO 100
2840 IF M$ = "TROLL" THEN M$ = "GLUTTONY": GOSUB 8900: GOTO 100
2850 IF M$ = "DWARF" THEN PRINT "You fool, dwarves eat only coal! Now you've made him *really* mad!": GOTO 100
2860 IF M$ = "BIRD" THEN PRINT "It's not hungry (it's merely pinin' for the fjords). Besides, you have no bird seed.": GOTO 100
2870 IF M$ = "BEAR" THEN 2880
2875 IF K > 0 THEN PRINT NH$; " eat.": GOTO 100
2877 IF P <> 59 THEN 9400
2880 PRINT "What do you want to feed the bear?"
2882 INPUT ""; M$
2883 K = 0: TL = 0: GOSUB 8500
2884 IF K = 0 AND TL = 0 THEN 6530
2886 PRINT P$(K)
2887 IF K <> 25 THEN 9090
2888 M$ = "SANDWICH": GOSUB 8900: GOTO 80
2895 PRINT NH$ + "to eat.": GOTO 100
2900 GOSUB 9600: IF LEN(M$) = 0 THEN 2910
2905 IF J(K) <> P AND J(K) <> 300 AND J(K) <> 400 AND M$ <> "" THEN 2050
2910 IF P = 17 OR P = 83 THEN 9118
2911 IF P = 50 OR P = 51 THEN 2920
2912 IF P = 181 OR P = 182 THEN 2950
2915 GOTO 9020
2920 IF J(42) = 300 AND TR = 2 THEN TR = 3
2925 IF TR = 1 THEN PRINT "The troll refuses to let you cross.": GOTO 100
2927 IF TR = 3 THEN PRINT "The troll steps out from beneath the bridge and blocks your way.": TR = 1: GOTO 80
2928 IF BR = 0 THEN PRINT "There is no longer any way across the chasm.": GOTO 100
2930 IF J(42) = 300 AND TR <> 1 THEN 2945
2935 IF P = 51 THEN P = 50: GOTO 80
2940 P = 51: GOTO 80
2945 M$ = C1$ + "COLLAPSE": GOSUB 8900: BR = 0: ZP = 50: BE = 0: TR = 0: J(29) = 0: GOTO 9800
2950 IF H > 3 THEN M$ = "BRSHAKES": GOSUB 8900
2955 IF P = 181 THEN P = 182: GOTO 80
2960 P = 181: GOTO 80
3000 REM ----> INVENTORY
3001 H1 = 0: IF H = 0 THEN PRINT "You're not carrying anything.": GOTO 100
3002 PRINT "You are currently holding the following:"
3005 H2 = 0: H3 = 0: H1 = H1 + 1: IF H1 = 84 THEN 3050
3010 IF J(H1) <> 300 THEN 3005
3020 PRINT P$(H1)
3030 H2 = H2 + 1: IF H2 = 13 THEN 3005
3035 IF H1 = CO(H2) AND CI(H2) > 0 THEN PRR = H2 + 300: GOSUB 4750: GOTO 3005
3049 GOTO 3030
3050 IF WE > 0 THEN PRINT : PRINT "You are wearing:"
3055 FOR X = 1 TO 83
3060 IF J(X) = 400 THEN PRINT "    "; P$(X)
3070 NEXT X
3075 IF J(44) = 300 THEN PRINT : PRINT FB$
3080 GOSUB 5850
3090 GOTO 100
3100 IF P = 39 THEN 3120
3110 GOTO 2915
3120 ON PL GOTO 3130, 3140, 3150
3130 PRINT TH$ + "climb. Use 'up' or 'out' to leave the pit.": GOTO 100
3140 PRINT "You have climbed up the plant and out of the pit."
3145 P = 37: GOTO 80
3150 PRINT "You clamber up the plant and scurry through the hole at the top."
3160 P = 41: GOTO 80
3200 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3220 IF M$ = "DOOR" OR (K = 0 AND P = 63) THEN GOSUB 9000: GOTO 100
3230 IF (K = 5 OR K = 43) AND J(16) <> P AND J(16) <> 300 THEN PRINT : PRINT "You have no keys!": GOTO 100
3240 IF K = 5 AND P <> 59 THEN PRINT : PRINT TH$ + "which the chain can be locked.": GOTO 100
3250 IF K = 5 THEN CH = 1: PRINT "The chain is now locked.": I$(5) = "There is a golden chain locked to the wall!": J(5) = 59: H = H - 1: GOTO 100
3260 IF K = 43 THEN GA = 0: PRINT GA$(0): GOTO 100
3270 IF P <> 8 AND P <> 9 AND P <> 63 AND P <> J(5) AND J(5) <> 300 THEN 2729
3290 GOTO 2789
3300 PRINT
3310 IF P > 8 THEN PRINT "I need more detailed instructions to do that.": GOTO 100
3320 M$ = "CAVE": GOSUB 8900: GOTO 100
3330 PRINT "It is beyond your power to do that.": GOTO 100
3350 REM PHUCE
3355 IF P <> 199 THEN 9030
3360 IF PH = 0 THEN PH = 1: M$ = "PHUCE1": GOSUB 8900: GOTO 80
3365 IF PH = 1 THEN PH = 0: M$ = "PHUCE2": GOSUB 8900: GOTO 80
3400 REM WEAR IT!
3405 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3410 IF K = 22 OR K = 74 OR K = 78 OR K = 79 THEN 3425
3415 GOSUB 9220: PRINT DK$; " to wear a "; LC$; "."
3420 LC$ = "": GOTO 100
3425 IF J(K) = 400 THEN PRINT "You are already wearing it!": GOTO 100
3430 IF J(K) = 300 THEN H = H - 1
3435 J(K) = 400: WE = WE + 1
3440 GOSUB 9000: GOTO 100
3450 REM (DIAL)
3455 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3460 IF K = 125 THEN PRINT "You don't have the correct change.": GOTO 100
3470 GOTO 9080
3500 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3505 IF K > 0 AND K <> 32 AND P <> 39 THEN PRINT CN$: GOTO 100
3510 IF WB = 1 AND (J(51) = 39 OR J(51) = 300) THEN 3525
3520 PRINT "I see no water here.": GOTO 100
3525 WB = 0: GOSUB 3560
3530 IF PL = 1 THEN GOSUB 8900: PL = 2: GOTO 3550
3532 IF PL = 2 THEN GOSUB 8900: PL = 3: GOTO 3550
3534 IF PL = 3 THEN GOSUB 8900: PL = 1
3550 GOSUB 3560: NI = 107: GOSUB 8950: GOTO 80
3560 M$ = "PLANT" + RIGHT$(STR$(PL), 1): RETURN
3600 PRINT "Blasting requires dynamite.": GOTO 100
3620 PRINT "I am unsure how you are facing.  Use compass points or nearby objects.": GOTO 100
3630 PRINT "The door is tightly shut.  There is no apparent way to open it.": GOTO 100
3700 FE = 1: GOSUB 9000: GOTO 100
3710 IF FE = 1 THEN FE = 2: GOSUB 9000: GOTO 100
3715 GOTO 3725
3720 IF FE = 2 THEN FE = 3: GOSUB 9000: GOTO 100
3725 FE = 0: GOSUB 9000: GOTO 100
3730 IF FE = 3 THEN 3750
3740 GOTO 3725
3750 IF J(10) = 42 THEN 9030
3755 IF J(10) = P AND P <> 42 THEN J(10) = 42: GOTO 3785
3760 IF J(10) = 300 AND P <> 42 THEN J(10) = 42: H = H - 1: GOTO 3785
3765 IF J(10) = 0 THEN J(10) = 42: PRINT "Done!": GOTO 100
3770 IF P = 42 AND J(10) <> 42 THEN J(10) = 42: GOTO 3795
3775 IF P = 42 AND J(10) = 300 THEN H = H - 1: GOTO 3795
3785 PRINT "The nest of golden eggs has vanished!": J(10) = 42: GOTO 100
3795 PRINT I$(10): GOTO 100
3800 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3805 IF K > 0 AND K <> 51 AND K <> 81 THEN PRINT "You can't fill that.": GOTO 100
3810 IF J(51) = P OR J(51) = 300 THEN 3830
3820 GOTO 9400
3830 IF WB > 0 THEN PRINT "Your bottle was already full.": GOTO 100
3840 IF P = 38 AND OI = 1 THEN OI = 0: WB = 2: GOTO 3875
3850 IF P = 1 OR P = 4 OR P = 5 OR P = 26 OR P = 44 OR P = 80 THEN 3870
3860 PRINT NH$ + " with which to fill the bottle.": GOTO 100
3870 WB = 1
3875 PRINT "Your bottle is now full of "; BB$(WB + 6)
3880 I$(51) = TB$ + BB$(WB + 8): GOTO 100
3900 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
3905 IF P <> 63 OR (K > 0 AND K <> 103) THEN PRINT 88: GOTO 100
3910 IF WB <> 2 THEN PRINT "I see no oil here.": GOTO 100
3920 IF P <> 63 THEN 2915
3930 WB = 0
3940 M$ = C1$ + "FREEDUP": GOSUB 8900: M$ = "MASSIVE": NI = 103: GOSUB 8950
3950 LT = 1: GOTO 100
4000 IF J(47) <> P AND J(47) <> 300 THEN PRINT "You have no source of light.": GOTO 100
4005 IF W = 36 THEN 4050
4008 IF DM = 1 THEN LS = 1
4010 IF DM = 0 THEN LS = 2
4020 PRINT "Your lamp is now on."; LS
4030 IF P > 12 THEN 80
4040 GOTO 100
4050 LS = 0
4060 PRINT "Your lamp is now off.": IF P > 12 AND P <> 226 AND P <> 193 THEN PRINT BB$(6)
4090 GOTO 100
4100 REM ---PULL<----
4102 IF M$ = "CHAIN" AND P = 153 THEN 4170
4105 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
4110 IF P = 47 AND CB = 0 THEN CB = 1: GOSUB 9000: GOTO 100
4115 IF P = 47 AND CB = 1 THEN CB = 0: GOSUB 9000: GOTO 100
4130 GOTO 9030
4170 IF DUM = 1 THEN DUM = 0: GOTO 4180
4175 DUM = 1
4180 PRINT "The dumbwaiter slides smoothly "; : ON DUM + 1 GOTO 4190, 4191
4190 PRINT "out of sight.": GOTO 100
4191 PRINT "into view.": GOTO 100
4200 GOSUB 9600
4210 IF K = 50 THEN 3300
4220 IF J(K) = P OR J(K) = 300 THEN PRINT "I believe what you want is right here with you.": GOTO 100
4250 M$ = "FIND": GOSUB 8900: GOTO 100
4300 IF WB = 1 AND (J(21) = P OR J(21) = 300) THEN WB = 0: PRINT : PRINT "The bottle of water is now empty.": GOTO 100
4310 IF P = 1 OR P = 4 OR P = 5 OR P = 26 OR P = 44 OR P = 80 THEN M$ = "DRINK": GOSUB 8900: GOTO 100
4320 IF M$ = "OIL" THEN 9040
4390 PRINT TH$ + "drink.": GOTO 100
4400 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
4410 IF P = 50 OR P = 51 OR P = 17 OR P = 83 THEN 4430
4420 GOTO 2915
4430 IF ((P = 17 OR P = 83) AND CB = 1) OR ((P = 50 OR P = 51) AND BR = 1) THEN PRINT : PRINT "I respectfully request you go across the bridge instead of jumping.": GOTO 100
4440 PRINT "You didn't make it."
4450 ZP = P: GOTO 9800
4500 GOSUB 9600: IF M$ = "GAUGE" THEN K = 127
4505 IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
4510 IF K = 55 OR J(55) = P OR J(55) = 300 THEN 4569
4520 IF K = 33 OR (K = 0 AND P = 43) THEN PRINT C$ + "Congratulations on bringing light into the Dark-Room!!" + C$: GOTO 100
4525 IF K = 127 THEN 4570
4530 IF K = 126 THEN M$ = "BILBOARD": GOSUB 8900: GOTO 100
4540 IF K > 69 AND K < 77 THEN GOSUB 8900: GOTO 100
4560 IF K = 0 THEN PRINT "READ what?": GOTO 100
4565 GOTO 9060
4569 PRINT "I'm afraid the magazine is written in dwarvish.": GOTO 100
4570 PRINT "The meter reads "; : IF ME = 0 THEN PRINT "zero.": GOTO 100
4575 PRINT C$; RIGHT$(STR$(ME), 1); C$; ".": GOTO 100
4600 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
4605 IF K = 63 OR (K = 0 AND (J(63) = P OR J(63) = 300)) THEN M$ = C1$ + "MUSHROOM": GOSUB 8900: J(63) = 0: GOTO 100
4610 IF K = 50 OR (K = 0 AND (J(50) = P OR J(50) = 300)) THEN 4640
4620 IF K = 0 AND J(50 <> P AND J(50) <> 300) THEN 2895
4630 IF K <> 50 THEN PRINT "I think I just lost my appetite.": GOTO 100
4640 IF J(50) = 300 THEN H = H - 1
4650 J(50) = 0: PRINT : PRINT "Thank you, that was delicious!": GOTO 100
4700 IF P = 25 THEN P = 11: GOTO 80
4703 IF P = 43 THEN P = 11: GOTO 80
4706 IF P = 24 AND H = 0 THEN P = 11: GOTO 80
4708 IF P = 24 AND H > 1 OR (H = 1 AND J(8) <> 300) THEN 4725
4710 IF P = 11 THEN 4713
4712 GOTO 2915
4713 IF J(8) = 300 THEN H = H - 1: J(8) = 11
4720 P = 25: GOTO 80
4725 M$ = "SOMETHIN": GOSUB 8900: AR = 4: GOTO 80
4750 FOR X = 1 TO 97
4755 IF J(X) = PRR THEN H3 = X
4760 NEXT X: IF H3 = 0 THEN RETURN
4770 GOTO 4860
4800 REM --LOOK--
4801 GOSUB 9600: IF J(K3) <> P AND J(K3) <> 300 AND J(K3) <> 400 THEN KD = K3: M$ = R$(1): GOTO 290
4802 IF M$ = "" THEN 4881
4810 IF M$ = "FOR" THEN M$ = R$(1): GOTO 4200
4815 IF M$ = "AT" THEN M$ = R$(1): GOTO 4500
4816 IF M$ <> "IN" THEN 6500
4820 FOR X = 1 TO 12: IF R$(1) = K$(CO(X)) THEN K2 = X
4822 IF J(CO(X)) = P OR J(CO(X)) = 300 THEN K = X
4825 NEXT X: PRINT K2, CI(K2)
4830 IF K2 = 0 THEN 6500
4835 IF CI(K2) > 0 THEN 4840
4836 PRINT "It's still closed.": GOTO 100
4840 PRR = K2 + 300
4850 FOR X = 1 TO 99
4855 IF J(X) = PRR THEN SF = X
4857 NEXT X: IF SF > 0 THEN GOSUB 4860: GOTO 100
4859 PRINT "There's nothing inside.": GOTO 100
4860 PRINT "It contains:"
4870 FOR X = 1 TO 97
4873 IF X = 51 AND H2 = 7 AND CI(7) > 0 THEN PRINT "   "; WB$(WB): GOTO 4895
4875 IF J(X) <> PRR THEN 4895
4880 PRINT "    " + P$(X)
4895 NEXT X: RETURN
4881 IF BF = 1 THEN 4884
4882 M$ = "SORRY": GOSUB 8900: LD = 1
4884 GOSUB 1100
4885 GOTO 80
4890 BF = 1: PRINT "Okay, from now on I'll only describe a place in full the first time you come to it. To get the full description, say, 'LOOK'"
4899 GOTO 100
4900 REM CLICK
4905 IF J(78) <> 300 THEN 4940
4910 IF P = 178 THEN P = 194: GOTO 80
4915 IF P = 194 THEN P = 178: GOTO 80
4920 GOTO 9030
4940 PRINT "I am not sure what you are trying to do."
4945 GOTO 100
4950 M$ = "STOP": GOSUB 8900: GOTO 100
4960 PRINT "Using"; M; "turns, you scored"; SC; "points out of a possible 751.": GOTO 100
5000 REM *YANK*
5003 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
5010 IF K = 79 AND CLO = 0 THEN CLO = 1: J(79) = 300: NI = 79: GOSUB 8950: M$ = C1$ + "WUMPWAKE": GOSUB 8900: WUMP = M: GOTO 100
5090 GOTO 9020
5200 REM ///SWEEP
5205 IF J(59) <> P AND J(59) <> 300 THEN 9080
5210 IF P <> 31 OR (P = 31 AND BD = 0) THEN 5240
5220 M$ = "DUSTROCK": GOSUB 8900: BD = 0: GOTO 100
5240 PRINT "Enough dusting, already!  You're making me sneeze.": GOTO 100
5300 IF P = 2 THEN 5310
5305 GOTO 5320
5310 IF PS = 0 THEN 5330
5320 PRINT "I don't understand the word "; D$: GOTO 100
5330 IF W = 72 AND CL = 2 THEN PRINT "The safe door smoothly swings open.": CI(1) = 1: CL = 0: GOTO 100
5340 GOSUB 9010
5345 IF W = 70 THEN CL = 1
5350 IF W = 71 AND CL = 1 THEN CL = 2
5399 GOTO 100
5400 REM ***  F N O R D  ***
5410 IF P = 2 THEN P = 183: GOTO 80
5413 IF P = 183 THEN P = 2: GOTO 80
5420 IF P = 149 THEN P = 130: GOTO 80
5430 IF P = 130 AND LK = 1 THEN P = 149: GOTO 80
5440 IF P = 130 AND LK = 0 THEN M$ = "CHUCKLE": GOSUB 8900: GOTO 80
5490 GOTO 9030
5500 REM -> BLOW
5502 IF M$ = "" THEN PRINT "You are now out of breath.": GOTO 100
5505 GOSUB 9600: IF J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
5510 IF P = 27 AND BH = 0 THEN BH = 1: M$ = C1$ + "HORN": GOSUB 8900: M$ = "RMS\ALT27": GOSUB 8900: DI(27, 1) = 172: GOTO 100
5560 IF (P > 8 AND P < 120) OR P = 140 OR (P > 144 AND P < 154) OR P > 162 THEN 5580
5570 PRINT "The blast of the horn echoes throughout hill and dale.": GOTO 100
5580 PRINT "The chamber reverberates to the blast of the horn."
5581 PRINT "(Satchmo you ain't!)": GOTO 100
5600 REM >>> PLAY PLAY PLAY
5605 IF M$ = "HORN" THEN 5505
5610 IF M$ = "LYRE" THEN PRINT "The air is filled with beautiful music.": GOTO 100
5620 IF M$ = "PIANO" THEN 5640
5630 IF M$ = "FLUTE" AND P <> 2 THEN PRINT "Sounds good.  Are you in training to be the pied piper, or what?": GOTO 100
5639 GOTO 100
5640 N$ = "CASABLNC.TXT"
5650 OPEN N$ FOR INPUT AS #1
5655 INPUT #1, PL$(1): INPUT #1, PL$(2)
5659 CLOSE #1
5660 PLAY PL$(1): PLAY PL$(2)
5670 PRINT "Ahh, stroke them ivories!   Play it, Sam.": GOTO 100
5700 REM PUSH
5701 GOSUB 9600: IF K > 0 AND J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
5705 IF M$ = "BUTTON" AND P = 140 THEN HB = HB + 1: GOTO 5780
5706 IF M$ = "BUTTON" AND P = 163 THEN PRINT "Which button do you want to push?": GOTO 100
5707 IF R$(1) = "BUTTON" THEN 5750
5710 IF M$ = "TILE" AND P = 149 THEN PRINT "Which tile do you want to push?": GOTO 100
5712 GOSUB 9150
5713 IF R$(1) = "TILE" THEN 5730
5720 GOTO 100
5730 REM ----->>> STEEL DOOR ROOM 149 ((TILES))
5731 FOR X = 1 TO 9
5732 IF M$ = TILE$(X) THEN PTI = X
5733 NEXT X
5734 IF PTI < 4 THEN 9060
5735 IF PTJ = 0 AND M$ = "FUSCHIA" THEN 5747
5736 IF PTJ = 1 AND M$ = "NACRE" THEN 5747
5737 IF PTJ = 2 AND M$ = "ORANGE" THEN 5747
5738 IF PTJ = 3 AND M$ = "RED" THEN 5747
5739 IF PTJ = 4 AND M$ = "DUN" THEN 5748
5740 LGT = LGT + 1: ON LGT + 1 GOTO 9205, 9210
5747 PTJ = PTJ + 1: PRINT "Done.": GOTO 100
5748 PRINT "steel door with inlaid tiles"
5749 SSD = 1: M$ = C1$ + "INLAID": GOSUB 8900: GOTO 100
5750 REM ----->>> ENGINEERING ROOM 163 <BUTTONS>
5751 GOSUB 8500
5760 IF TL > 4 THEN 9060
5761 ON TL GOTO 5770, 5772, 5774, 5776
5770 IF RML = 0 THEN RML = 1: GOTO 5779
5771 PRINT "The room lights are already on.": GOTO 100
5772 IF HEV = 0 THEN 5779
5773 PRINT "The sound of the engine has ceased.": HEV = 0: GOTO 100
5774 IF RML = 1 THEN RML = 0: GOTO 5779
5775 PRINT "The room lights were already off.": GOTO 100
5776 IF HEV = 1 THEN 5779
5777 HEV = 1: M$ = "ENGSTART": GOSUB 8900: GOTO 100
5779 GOSUB 9010: GOTO 100
5780 REM ----->>> HELICOPTER ROOM 140 <BUTTON>
5781 IF HB > 1 THEN 5785
5782 M$ = "COPTER": GOSUB 8900
5784 GOTO 5790
5785 PRINT : PRINT "Rattle, roar, shake, etc...."
5790 IF HEP = 121 THEN HXP = 141: GOTO 100
5793 IF HEP = 141 THEN HXP = 161: GOTO 100
5796 IF HEP = 161 THEN HXP = 121: GOTO 100
5799 GOTO 100
5800 REM ((((HEALTH))))
5810 IF HLTH = 100 AND E(1) < 5 THEN PRINT "You are fit as a fiddle.": GOTO 100
5815 IF HLTH = 100 THEN PRINT "You are in perfect health.": GOTO 100
5820 PRINT "Your health rating is "; HLTH; " out of a possible 100."
5830 IF HLTH > 80 THEN PRINT "You are a bit off top form, but nothing to worry about.": GOTO 100
5835 IF INT(HLTH / 20) < HLTH / 20 THEN PRINT HLTH$(INT(HLTH / 20) + 1): GOTO 100
5840 PRINT HLTH$(INT(HLTH / 20))
5849 GOTO 100
5850 REM *BOTTOM HEALTH RATINGS
5852 IF (J(76) <> P AND J(76) <> 300) OR HLTH = 100 THEN 5859
5854 IF E(1) <> 5 THEN PRINT H2$(INT(HLTH / 20))
5859 RETURN
5860 MW = M - WUMP
5865 IF MW = 9 THEN 9199
5870 IF MW < 3 THEN PRINT WUMP$(1): RETURN
5872 IF MW < 5 THEN PRINT WUMP$(2): RETURN
5874 IF MW < 7 THEN PRINT WUMP$(3): RETURN
5876 PRINT WUMP$(4)
5890 RETURN
5900 REM +++ INSERT
5905 GOSUB 9600: IF J(K) <> P AND J(K) <> 300 AND J(K) <> 400 THEN 2050
5910 IF R$(1) = "" THEN PRINT "Where do you want to insert the card?": GOTO 5950
5920 IF R$(1) <> "IN" THEN 9020
5930 IF R$(2) = "SLOT" AND P = 144 THEN 5980
5950 INPUT IC$
5960 IF IC$ = "IN SLOT" THEN 5980
5970 GOTO 4940
5980 LK = 1: J(73) = 0: H = H - 1
5985 M$ = "1TIMERS\SLOT": GOSUB 8900: GOTO 100
6000 PRINT "Do you really want to quit now?": INPUT "", D$
6020 IF D$ = "Y" OR D$ = "YES" THEN GOSUB 9000: GOTO 9985
6030 IF D$ = "N" OR D$ = "NO" THEN GOSUB 9000: GOTO 100
6040 PRINT "Please answer the question first.": GOTO 6000
6100 REM CRAWL HOURS
6110 PRINT "Which way?": GOTO 100
6120 M$ = "HOURS": GOSUB 8900: GOTO 100
6500 PRINT : DN = INT(4 * RND(1) + 1)
6510 ON DN GOTO 6530, 6540, 6550, 6560
6530 PRINT "I don't understand the word "; M$: GOTO 100
6540 PRINT "I don't understand that!": RETURN
6550 PRINT "What?": RETURN
6560 PRINT "Huh?": RETURN
6570 GOTO 9060
6600 PRINT
6610 IF P = 36 THEN P = 38: GOTO 100
6620 IF P = 37 THEN P = 39: GOTO 100
6630 IF P = 66 THEN P = 80: GOTO 100
6650 REM GROTTO
6655 IF P = 127 THEN P = 199: GOTO 80
6659 GOTO 9020
6800 IF P = 65 OR P = 30 OR P = 71 OR P = 25 THEN P = 25: GOTO 80
6890 GOTO 9020
6900 REM *DIG*
6910 IF J(72) = P OR J(72) = 300 THEN 6930
6920 PRINT "Digging without a shovel is quite impractical. Even with a shovel progress is unlikely.": GOTO 100
6930 IF P <> 188 THEN 6950
6935 BG = BG + 1: IF BG > 3 THEN 6980
6940 ON BG GOTO 6960, 6970, 6980
6950 PRINT "The ground here is not suitable for digging.": GOTO 100
6960 PRINT "You are standing up to your knees in bat guano.": GOTO 100
6970 PRINT "You are now up to your hips in bat guano.": GOTO 100
6980 PRINT "You've got bat turds hanging off your ears, wimp!"
6982 PRINT "This isn't getting you anywhere.  Best try someplace else.": GOTO 100
7000 M$ = "MIST": GOSUB 8900: GOTO 100
7010 M$ = "TREES": GOSUB 8900: GOTO 100
7700 M$ = C1$ + "SUSPEND": GOSUB 8900
7730 PRINT : INPUT ""; SA$
7740 IF SA$ = "Y" OR SA$ = "YES" THEN 7800
7750 IF SA$ = "N" OR SA$ = "NO" THEN GOSUB 9000: GOTO 100
7760 PRINT PL$: GOTO 7730
7800 GOSUB 9000: M$ = "1TIMERS\SAVE": GOSUB 8900
7810 INPUT ""; SA$
7820 IF LEN(SA$) > 6 THEN SA$ = LEFT$(SA$, 6)
7840 SA$ = "C:\ADV751\GAMES\" + SA$
7850 OPEN SA$ FOR OUTPUT AS #1
7860 PRINT #1, SC, M, LA, LS, P, BR, TR, PL, BE, WB, SN, DD, OI, WI, H, D, MD
7865 PRINT #1, SE, CC, SV, CH, DE, PO, ZP, L1, DM, BD
7870 FOR X = 1 TO 132: PRINT #1, J(X): NEXT X
7880 CLOSE #1
7890 END
7900 PRINT "Do you want to restore a previously saved game?"
7920 PRINT : INPUT "", SA$
7930 IF SA$ = "Y" OR SA$ = "YES" THEN 7960
7940 GOSUB 9000: GOTO 100
7960 PRINT "Enter the name of the file:"
7970 INPUT ""; SA$
7975 SA$ = "C:\ADV751\GAMES\" + SA$
7980 OPEN SA$ FOR INPUT AS #1
7981 INPUT #1, SC, M, LA, LS, P, BR, TR, PL, BE, WB, SN, DD, OI, WI, H, D, MD
7982 INPUT #1, SE, CC, SV, CH, DE, PO, ZP, L1, DM, BD
7987 FOR X = 1 TO 132: INPUT #1, J(X): NEXT X
7988 CLOSE #1
7990 GOTO 80
8000 REM HINTS!!!
8010 AB = INT(9 * RND(1) + 1)
8015 IF AB <> 4 AND AR > 1 THEN 1099
8020 IF AB <> 4 THEN 100
8030 M$ = "HINTS\ASK" + MID$(STR$(AR), 2, LEN(STR$(AR))): GOSUB 8900
8200 INPUT H$
8210 PRINT : IF H$ = "Y" OR H$ = "YES" THEN 8300
8220 IF H$ = "N" OR H$ = "NO" THEN GOSUB 9000: GOTO 100
8250 PRINT PL$: GOTO 8000
8300 PRINT : PRINT "I am prepared to give you a hint, but it will cost you 2 points."
8310 PRINT "Do you want the hint?": INPUT "", H$
8320 IF H$ = "Y" OR H$ = "YES" THEN 8400
8330 IF H$ = "N" OR H$ = "NO" THEN AR = 0: GOSUB 9000: GOTO 100
8340 PRINT : PRINT PL$: GOTO 8300
8400 PRINT : SC = SC - 2
8405 IF J(16) <> P AND J(16) <> 300 THEN M$ = "HINTS\ALT1": GOSUB 8900: GOTO 8420
8410 M$ = "HINTS\HINT" + RIGHT$(STR$(AR), 1): GOSUB 8900
8420 AR = 0: GOTO 100
8500 REM ---))) SCAN THE M$!!!
8510 FOR X = 1 TO 132
8520 IF M$ = K$(X) THEN K = X
8530 NEXT X
8540 FOR X = 1 TO 12
8550 IF M$ = TILE$(X) THEN TL = X
8560 NEXT X
8590 RETURN
8900 REM *READ SUBROUTINE*
8905 NN = 0
8910 TX$ = M$ + ".TXT"
8912 OPEN TX$ FOR INPUT AS #1
8914 NN = NN + 1
8916 INPUT #1, TX$(NN)
8918 IF TX$(NN) = "//" THEN NN2 = NN: GOTO 8925
8920 GOTO 8914
8925 CLOSE #1
8930 FOR X = 1 TO NN2 - 1
8932 IF TX$(X) = "û" THEN PRINT "<Enter> for next page >"; : INPUT ""; TXX$: GOTO 8938
8936 PRINT TX$(X)
8938 NEXT X
8940 RETURN
8950 REM ^^^READ CHANGED STRINGS SUBROUTINE^^^
8955 TC$ = "C:\ADV751\CHANGE\" + M$
8960 OPEN TC$ FOR INPUT AS #1
8965 INPUT #1, I$(NI)
8967 IF NI = 30 THEN INPUT #1, I$(4)
8969 CLOSE #1
8970 RETURN
9000 PRINT : PRINT "Ok": RETURN
9005 PRINT DK$; ".": GOTO 100
9010 PRINT C$ + "Click." + C$: RETURN
9020 PRINT DK$; " to apply that word here.": GOTO 80
9030 PRINT "Nothing happens.": GOTO 100
9040 PRINT "Don't be ridiculous!": GOTO 100
9050 PRINT "Interesting.  How?": GOTO 100
9060 PRINT "I think you are a little confused!": GOTO 100
9070 PRINT "You can't be serious!": GOTO 100
9080 PRINT "I'm game!  Would you care to explain how?": GOTO 100
9090 PRINT "Get serious, you bozo!": GOTO 100
9100 REM RESTRICTIONS
9101 PRINT "You can't go through a locked steel grate!": GOTO 80
9102 PRINT "You don't fit through a 2-inch slit!": GOTO 80
9103 PRINT "The ledge drops off into a sheer cliff. You can't climb down.": GOTO 100
9104 PRINT "You can't fit through a six-inch door!": GOTO 80
9105 PRINT "The crack is far too small for you to follow.": GOTO 80
9106 PRINT "How? Through solid rock?": GOTO 80
9107 PRINT "The path in that direction is too steep.  You can't proceed.": GOTO 80
9108 PRINT "You can't swim. You'd best go by boat.": GOTO 80
9109 PRINT "The tunnel is completely flooded.  You can't proceed.": GOTO 80
9110 PRINT "You have stumbled blindly through the trackless muck.": GOTO 80
9111 PRINT C$ + "North" + C$; " "; : M$ = "NORTH": GOSUB 8900: GOTO 80
9112 PRINT "The cliff is unscalable.": GOTO 80
9113 PRINT "You can't fit this five-foot "; CLAM$; " through that little passage!": P = 60: GOTO 100
9116 PRINT "The bramblebush is too thick and thorny to penetrate.": GOTO 80
9117 PRINT "The dog won't let you pass.": GOTO 80
9118 IF CB <> 1 THEN PRINT "There is no way across the fissure.": GOTO 80
9119 IF P = 17 THEN P = 83: GOTO 80
9120 P = 17: GOTO 80
9121 M$ = "FNORD!": GOSUB 8900: P = 130: GOTO 80
9122 IF E(8) = 3 THEN P = 209: GOTO 80
9123 M$ = "CHUTE" + RIGHT$(STR$(E(8)), 1): GOSUB 8900: GOTO 80
9133 PRINT "You can't get by the snake.": GOTO 80
9147 PRINT "It's too heavy. You'll have to drop something first.": RETURN
9150 REM >> SPLIT THE M$!
9152 FOR X = 1 TO LEN(M$)
9153 IF MID$(M$, X, 1) = " " THEN SM = SM + 1: Z(SM) = X
9154 NEXT X
9155 Z(SM + 1) = X
9157 FOR W = 1 TO SM
9160 FOR X = Z(W) + 1 TO Z(W + 1) - 1
9170 R$(W) = R$(W) + MID$(M$, X, 1)
9175 NEXT X
9179 NEXT W
9180 M$ = LEFT$(M$, Z(1) - 1): RETURN
9190 IF H > 0 THEN M$ = "TUNNEL": GOSUB 8900: GOTO 80
9192 P = 12: GOTO 80
9199 PRINT C$; "Chomp, chomp."; C$; "  Crunch!  Chew!  Slurp!  Smack!  Yum!!": GOTO 9800
9200 COLOR 10: PRINT "Above the door, a green light is shining.": COLOR 7: GOTO 9215
9205 COLOR 14: PRINT "Above the door, a yellow light is shining.": COLOR 7: GOTO 9215
9210 COLOR 12: PRINT "Above the door, a "; : COLOR 28: PRINT "red light is flashing!": COLOR 7
9215 PRINT "The stainless steel door"; : ON SSD + 1 GOTO 9380, 9390
9220 FOR X = 1 TO LEN(M$)
9222 LC$ = LC$ + CHR$(ASC(MID$(M$, X, 1)) + 32)
9224 NEXT X
9229 RETURN
9300 REM OPEN & SHUT
9360 PRINT " is now unlocked.": GOTO 100
9370 PRINT " is open.": GOTO 100
9380 PRINT " is locked.": GOTO 100
9390 PRINT " is unlocked.": GOTO 100
9400 PRINT : PRINT D$ + " what?": GOTO 100
9500 REM CRAWLERS.....
9510 IF P = 62 AND D$ = "W" THEN 9550
9530 PRINT "You have crawled around in some little holes and wound up"
9540 PRINT "back in the main passage.": GOTO 80
9550 WI = 8: M$ = "CAVEIN-1": GOSUB 8900
9580 GOTO 80
9600 FOR X = 1 TO 132
9603 IF M$ = K$(X) THEN K = X
9604 IF R$(1) = K$(X) THEN K = X: K3 = X
9607 NEXT X
9610 FOR X = 1 TO 12
9613 IF M$ = TILE$(X) THEN TL = X
9616 NEXT X
9620 IF LEN(M$) = 0 THEN K = 0
9630 IF K = 0 AND TL = 0 AND LEN(M$) > 0 THEN 6500
9690 RETURN
9800 FOR X = 1 TO 99
9810 IF J(X) = 300 THEN J(X) = ZP
9820 NEXT X
9830 J(47) = 1: H = 0: DE = DE + 1
9840 P = 2
9850 IF SE = 1 THEN 9905
9860 PRINT : ON DE GOTO 9910, 9920, 9930
9905 M$ = "1TIMERS\DEATH0": GOSUB 8900: GOTO 9985
9910 M$ = "1TIMERS\DEATH1": GOSUB 8900: GOTO 9940
9920 M$ = "1TIMERS\DEATH2": GOSUB 8900: GOTO 9940
9930 M$ = "1TIMERS\DEATH3": GOSUB 8900: GOTO 9940
9940 ZP = 0: PRINT : INPUT "", QU$
9950 IF QU$ = "N" OR QU$ = "NO" THEN GOSUB 9000: GOTO 9985
9955 IF QU$ = "Y" OR QU$ = "YES" THEN ON DE GOTO 9960, 9970, 9980
9956 PRINT : PRINT PL$: GOTO 9840
9960 PRINT : M$ = "1TIMERS\POOF1": GOSUB 8900: GOTO 80
9970 PRINT : M$ = "1TIMERS\POOF2": GOSUB 8900: GOTO 80
9980 PRINT : PRINT "Okay, you're so smart, try it yourself! I'm leaving!"
9985 GOSUB 9991
9989 RUN "C:\ADV751\QUITS"
9990 END
9991 SCM$ = "SCM"
9992 OPEN SCM$ FOR OUTPUT AS #1
9993 PRINT #1, SC, M
9994 CLOSE #1
9995 RETURN
10000 REM --- SINGLE DESCRIPTIONS (A$)
10068 DATA "in a maze of little twisty passages, all alike."
10069 DATA "in a maze of twisty little passages, all different."
10071 DATA "in a jumble of rock, with cracks everywhere."
10076 DATA "in Bedquilt, a long east/west passage with holes everywhere.                    To explore at random select north, south, up or down."
10080 DATA "at the bottom of a small pit with a little stream, which enters                 and exits through tiny slits."
10081 DATA "in a long sloping corridor with ragged sharp walls."
10082 DATA "in a large low room. Crawls lead N, NE and SW."
10224 DATA "You are in the upper passage, a long level E/W tunnel."
10229 DATA "You are in a sloping muddy defile, next to a tumbling brook."
10236 DATA "You are at the foot of the Altar, an immense, broad stalagmite."
10237 DATA "You're at the east portal of the Gothic Cathedral."
10238 DATA "You're at the west portal of the Gothic Cathedral."
10239 DATA "You are on a narrow and treacherous ledge, which rises almost                    vertically to the east."
10240 DATA "You are at the intersection of two long tunnels.  One goes NW,                   the other NE."
21000 REM ----- TREASURES / UTILITIES  (I$)
21001 DATA "There is a large, sparkling nugget of gold here!"
21010 DATA "There is a large nest here, full of golden eggs!"
21011 DATA "There is a platinum pyramid here, 8 inches on a side!"
21012 DATA "There is a delicate, precious ming vase here!"
21024 DATA "Deep within the brambles is growing a perfect, blood-red rose!"
21045 DATA "A yellow pill-shaped tablet, as large as a doughnut, lies nearby."
21062 DATA "Nearby, a strange, greenish stone is glowing brightly."
21065 DATA "There is a heavy, grey metal cannister here."
21072 DATA "There is a dusty, leather-bound volume here."
21076 DATA "There is some soiled paper nearby."
21077 DATA "A delicate silver flute is lying nearby."
21078 DATA "There is a pair of ruby slippers here."
21079 DATA "A lovely velvet cloak lies partially buried under a pile of                     loose rocks."
21080 DATA "There is a large cloth bag lying nearby."
21081 DATA "There is an empty oaken cask here."
21082 DATA "There is a small matchbox here."
21083 DATA "There is a wax candle here."
21084 DATA "There is a match here."
21085 DATA "There are two matches here."
21086 DATA "There are three matches here."
21087 DATA "87"
21088 DATA "88"
21089 DATA "89"
21090 DATA "90"
21091 DATA "91"
21092 DATA "92"
21093 DATA "93"
21094 DATA "94"
21095 DATA "Not far away is a helicopter.  Its engine is idling slowly."
21096 DATA "There is a small wooden boat here."
21097 DATA "In the corner, a Wumpus is sleping peacefully."
21098 DATA "A hideous black dog bares his teeth and growls at your approach."
21099 DATA "There is a ferocious cave bear eying you from the far end of the room!"
21100 DATA "Several jac-booted orks are standing guard around the aircraft."
21101 DATA "A huge green fierce snake bars the way!"
21102 DATA "A huge green fierce dragon bars the way!"
21103 DATA "The way north is barred by a massive, rusty iron door."
21104 DATA "A massive stone tablet embedded in the wall reads,"
21105 DATA "There is a message scrawled in the dust in a flowery script, reading,"
21106 DATA "There is a massive vending machine here.  The instructions on it read:"
21107 DATA "There is a tiny little plant in the pit, murmuring, 'Water, water..."
21108 DATA "The shadowy figure seems to be trying to attract your attention."
21109 DATA "The dumbwaiter is at this level."
22010 REM INITIAL TREASURE LOCATIONS (J) [1-44] (20 PER LINE)
22011 DATA 16,0,67,79,59,54,64,11,44,42,43,35,200,0,0,65,83,185,133,145
22012 DATA 193,171,165,237,176,159,208,0,0,0,0,0,0,0,0,0,0,0,0,0
22013 DATA 0,0,0,204
22020 REM INITIAL UTILITY LOCATIONS  (J) [45-100]  (20 PER LINE)
22021 DATA 120,120,2,10,13,120,120,60,0,34,61,0,304,0,187,2,151,189,123,127
22022 DATA 88,188,152,304,131,146,2,301,183,150,310,302,184,178,180,8,203,2,216,308
22023 DATA 308,308,0,0,0,0,0,0,0,0,121,199,180,137,59,121
22030 REM INITIAL IMMOVABLE FEATURE LOCATIONS [101-132] (20 PER LINE)
22031 DATA 23,79,63,43,295,295,39,30,0,17,17,0,50,8,34,34,35,0,0,12
22032 DATA 0,140,47,48,38,122,163,200,208,184,0,153
23000 REM ------- TREASURES (K$) [1-44] (10 PER LINE)
23001 DATA GOLD,BARS,COINS,RUG,CHAIN,SPICES,JEWELRY,EMERALD,TRIDENT,NEST
23002 DATA PYRAMID,VASE,DIAMONDS,CHEST,PEARL,LYRE,HORN,SAPPHIRE,FLOWERS,TAPESTRY
23003 DATA SPHERE,CROWN,INGOT,ROSE,CHALICE,MARBLE,SWORD,X,X,X
23004 DATA X,X,X,X,X,X,X,X,X,X
23005 DATA X,X,X,WINE
23010 REM ------- UTILITIES (K$) [45-100] (10 PER LINE)
23011 DATA PILL,KEYS,LAMP,CAGE,BIRD,FOOD,BOTTLE,CLAM,OYSTER,PILLOW
23012 DATA MAGAZINE,BATTERIES,POWDER,AXE,BROOM,SACK,KNAPSACK,STONE,MUSHROOMS,POLE
23013 DATA CANNISTER,SHOVEL,CLAY,ROPE,BOX,LEAFLET,POSTER,BOOK,CARD,SHIELD
23014 DATA DOCUMENT,PAPER,FLUTE,SLIPPERS,CLOAK,BAG,CASK,MATCHBOX,CANDLE,MATCH
23015 DATA MATCH,MATCH,X,X,X,X,X,X,X,X
23016 DATA HELICOPTER,BOAT,WUMPUS,DOG,BEAR,ORKS
23030 REM ------- PERM FEATURES (K$) [101-132] (10 PER LINE)
23031 DATA SNAKE,DRAGON,DOOR,TABLET,MESSAGE,MACHINE,PLANT,FIGURE,DUMBWAITER,BRIDGE
23032 DATA FISSURE,DWARF,TROLL,GRATE,CARPET,MOSS,DRAWINGS,PIRATE,KNIFE,MIRROR
23033 DATA CAVE,BUTTON,LEVER,CHUTE,OIL,BILLBOARD,METER,PHONE,ANVIL,PIANO
23034 DATA SAFE,CABINET
24000 REM *** COMMANDS  (W$)  (10 PER LINE)
24001 DATA GET,DROP,CARRY,TAKE,INVENTORY,THROW,KILL,FEED,LOCK,OPEN
24011 DATA UNLOCK,FIND,PULL,LOOK,BRIEF,XYZZY,PLUGH,IN,ENTER,OUT
24021 DATA EXIT,CROSS,WATER,OIL,CLOSE,READ,DRINK,EAT,CLIMB,FILL
24031 DATA FEE,FIE,FOE,FOO,LIGHT,OFF,JUMP,RIGHT,LEFT,PLOVER
24041 DATA CAVE,BREAK,BLAST,PIT,ROOM,QUIT,MIST,TREES,Y2,SAVE
24051 DATA RESUME,DIG,OFFICE,WALL,TERSE,STOP,SCORE,BLOW,N,S
24061 DATA E,W,D,U,NE,NW,SE,SW,I,7
24071 DATA 22,34,PLAY,PUSH,PRESS,FNORD,DIAGNOSE,HEALTH,INSERT,CLICK
24081 DATA WEAR,PHUCE,DIAL,ANSWER,YANK,GROTTO,SWEEP,CRAWL,HOURS,ORIENT
24091 DATA B,EXAMINE,PUT,SWIM
24500 REM $$$ TREASURE VALUES (V)
24501 DATA 10,10,10,14,14,14,10,14,14,14,14,10,10,14,14
25500 REM +++++ COLORS +++++
25510 DATA GREEN,YELLOW,BROWN,RED,SILVER,FUSCHIA,GOLD,DUN,WHITE,ORANGE,NACRE,BLACK
25520 DATA 6,11,10,4,8
26000 REM IN & OUT
26010 DATA 2,9,1,10,56,77,162
26020 DATA 120,10,2,47,59,13,163


