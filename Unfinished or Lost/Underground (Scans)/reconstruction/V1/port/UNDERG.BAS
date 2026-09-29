10 OPEN"KB:"ASFILE1%:DIM#2%,M%(87%,14),O%(45),X%(11):&"Are you recalling a saved game?":GOSUB3080:IFZ$="Y"THEN40
20 FORX%=65%TO90%:P$=CHR$(X%):F$="UNDER"+P$+".DAT":ONERRORGOTO30:OPENF$FORINPUTASFILE2%:CLOSE2%:NEXTX%:&"There are too many unfinished UNDERGROUND games in your account.":goto3120
30 iferl-20%thenresume60elseOPENF$asfile2%:resume70
40 input#1%,"Password:";P$:IFLEN(P$)-1%ORP$<"A"ORP$>"Z"THEN&"Illegal password.":GOTO40
50 ONERRORGOTO30:F$="UNDER"+P$+".DAT":OPENF$FORINPUTASFILE2%:GOTO970
60 &"Password not found.":GOTO10
70 X%(11%)=30%:X%(X%)=0%FORX%=10%TO1%STEP-1%:M%(X%,0%)=0%forX%=1%to87%:O%(0%)=39%:MATREADM%,O%:X%(0%)=8%:&"If you want to recall this game at a later date, the password is "P$".":goto970
80 DATA,8,,,,,,,,2,,8,8,
90 DATA-1,,,,,,,,1,,-1,-1,-1,
100 DATA7,9,4,8,,,,,,,,,,
110 DATA15,,,3,,,,,,15,15,3,,
120 DATA8,7,3,6,,,,,-3,-3,-3,,,
130 DATA9,5,8,7,,,,,,,,,,
140 DATA3,7,5,6,,,,,,,,,,
150 DATA3,6,1,5,,,,,,,1,,1,
160 DATA-2,-2,3,6,-2,,,,,-2,-2,,,
170 DATA,,,,,,,,,23,,,,
180 DATA,,,,18,,,,,,,27,,
190 DATA,41,,,,,42,,,,,,,
200 DATA,30,,,,,,,,,,30,,
210 DATA,,,,,,,,,,32,32,,
220 DATA16,,,4,,,,,4,16,16,4,,
230 DATA33,21,29,15,22,,,17,15,,,,,
240 DATA,,,,16,,,18,,,,,,
250 DATA11,17,,20,,,19,5,,5,,5,,
260 DATA18,,,-1,,,,,,,-1,-1,-1,
270 DATA,,-2,-2,18,,,-2,,,,,,
280 DATA,24,16,,,,,,23,,,,,
290 DATA,24,16,,,,,,23,,,,,
300 DATA22,,,21,,,,,10,,,,,
310 DATA,,,,22,,21,,25,25,,,,
320 DATA,,,,,,,,24,26,,,,
330 DATA,,,,,,,,25,27,,,,
340 DATA,,,,,,,,26,28,,,,
350 DATA,,,,,,,,27,,,27,,
360 DATA,,30,,,,16,,,,,,,
370 DATA13,,31,,29,,,,,,30,,,
380 DATA,30,,,,,,,,,,30,,
390 DATA,,,,,,,,,,14,-6,,
400 DATA,,,,,,,,,,,,,
410 DATA61,35,41,33,,,,,,-5,78,,,35
420 DATA,36,34,,,,,,,36,,,36,
430 DATA37,-4,,,,,,,,36,,,36,
440 DATA,,-4,,,39,,,,,-4,39,,-4
450 DATA,,,39,,,,,,,,39,,
460 DATA38,37,40,,,,,,,,,,,
470 DATA,39,,,,,,,-5,,,,,34
480 DATA,,,12,,,,34,,,,,,
490 DATA,12,43,,,,,,,,,,,
500 DATA44,,,,42,,,,,,,,,
501 DATA45,,,,43,,,,,,,,,
502 DATA,48,46,44,,,,,,,,,,
503 DATA47,,,,45,,,,,,,,,
504 DATA,,,46,,,,,,,,46,,
505 DATA,53,49,45,,,,,,,,45,,
506 DATA50,,,,48,,,,,,,,,
507 DATA51,,,,49,,,,,,,51,51,51
510 DATA52,52,,,,,,,,,,,,
520 DATA,,51,,,,,,,,,51,,
530 DATA48,,,,54,,,,,,,54,48,
540 DATA53,,,57,56,55,-10,,,,,,,
550 DATA,,,54,,,,,,,,,,
560 DATA54,,,,58,,,,,,58,,,
570 DATA,54,,58,,,,,,,,,,
580 DATA59,85,60,-8,,,,,,,,,,
590 DATA10,,,,-7,,,,,,,,,
600 DATA,-7,-9,,,,,,,,,,,
610 DATA,62,63,34,,,,,,,,,,
620 DATA63,73,61,72,,,,,,,,,,
630 DATA64,62,67,61,,,,,,,,,,
640 DATA,65,64,63,,,,,,,,,,
650 DATA,66,64,,,,,,,,,,,
660 DATA66,66,66,66,,,,,65,74,,,,
670 DATA68,70,63,,,,,,,,,,,
680 DATA69,67,68,68,,,,,,,,,,
690 DATA77,,68,,,,,,,,,,,
700 DATA75,,,67,,,,,,,,,,
710 DATA71,76,70,75,,,,,,,,,,
720 DATA62,,,,,,,,,,,62,,
730 DATA,,62,,,,,,,,,62,,
740 DATA,,,,,,,,,74,,74,,
750 DATA71,,,,,,,,,,,71,,
760 DATA,,71,,,,,,,,,71,,
770 DATA,,,,,,,,,40,,,,
780 DATA79,,,34,,,,,,,,,,
790 DATA82,80,,78,,,,,,,,,,
800 DATA,,79,,,,,,,,,79,,
810 DATA,,,83,,,,,,,,83,,
820 DATA84,,,79,,,,,,,,,,
830 DATA81,,,,,84,,,,,,,,
840 DATA,,82,,,83,,,,,,,,
850 DATA,,,,,,,,,,,,,
860 DATA,,54,,,,,,,,,,,
870 DATA,,,,,,,,,,,,,
880 DATA29,32,,,38,47,81,,,13,42,,55,77,,81,1,1,1,2,3,4,26,11,30,30,29,69,68,1,46,83,,57,49,84,78,80,79,82,16,20,33,29,86
970 dima$(1%):C$=SYS(CHR$(8%)+F$)
975 ON ERROR GOTO 0
980 IFX%(0%)>14%ANDX%(0%)-O%(24%)THENIFO%(17%)<>X%(0%)ANDO%(17%)+1%ORX%(5%)=0%ORX%(0%)=77%ORX%(3%)>300%THEN992
990 R%=100%*X%(0%):R%=R%+50%IFM%(X%(0%),0%)ANDA$<>"LOOK":M%(X%(0%),0%)=1%:CLOSE2%:CHAIN"LOOK.BAC"R%
992 RANDOM:R=R%*RND:R%=1%:IFR>.6666666THEN3040ELSE&"I can't see a thing.":GOTO1000
995 OPENSYS(CHR$(7%))ASFILE2%
997 OPEN"KB:"ASFILE1%
1000 V%,T%=-1%:A$(0%)="      ":A$(1%)="     "
1001 INPUTLINE#1%,A$:A$=CVT$$(A$,188%)+" ":IFA$=""THEN1010ELSEX%=INSTR(X%,A$," "):LSETA$(0%)=LEFT(A$,X%-1%):X%=X%+1%:LSETA$(1%)=MID(A$,X%,INSTR(X%,A$," ")-X%)
1005 onerrorgoto3200:V$=sys(chr$(6%)+CHR$(-7%))
1010 X%(2%)=X%(2%)+1%:IFX%(5%)THENX%(3%)=X%(3%)+1%:IFX%(3%)=250%THEN&"Your lamp is running out of power."ELSEIFX%(3%)=300%THENIFX%(0%)=87%THEN3160ELSE3170ELSEIFX%(3%)=297%ANDX%(0%)-87%THEN3175
1015 forx%=0%to1%:A$=A$(X%)
1020 IFA$="N"ORA$="NORTH"THENY%=1%:GOTO2000
1030 IFA$="E"ORA$="EAST"THENY%=2%:GOTO2000
1040 IFA$="W"ORA$="WEST"THENY%=3%:GOTO2000
1050 IFA$="S"ORA$="SOUTH"THENY%=4%:GOTO2000
1060 IFA$="NE"THENY%=5%:GOTO2000
1070 IFA$="NW"THENY%=6%:GOTO2000
1080 IFA$="SE"THENY%=7%:GOTO2000
1090 IFA$="SW"THENY%=8%:GOTO2000
1100 IFA$="U"ORA$="UP"ORA$="CLIMB"THENY%=9%:GOTO2000
1110 IFA$="D"ORA$="DOWN"THENY%=10%:GOTO2000
1120 IFA$="IN"ORA$="ENTER"THENY%=11%:GOTO2000
1130 IFA$="OUT"ORA$="EXIT"ORA$="LEAVE"THENY%=12%:GOTO2000
1140 IFA$="DOOR"THENY%=13%:GOTO2000
1150 IFA$="BARRE"THENY%=14%:GOTO2000
1160 IFA$="JUMP"ORA$="LEAP"ORA$="HOP"ORA$="DIVE"THEN2850
1170 IFA$="LOOK"THEN980
1220 IFA$="QUIT"THEN&"Are you sure?":GOSUB3080:IFZ$="N"THEN1000ELSEGOSUB1270:CLOSE2%:KILLSYS(CHR$(7%)):GOTO3200
1230 IFA$="SCORE"THENGOSUB1270:GOTO1000
1240 ifA$<>"INVEN"then1250
1241 X%=-1%
1242 &"Something valuable."IFX%=O%(0%):&"Platinum wire."ifX%=O%(1%):&"Uranium in container."IFO%(2%)=X%:&"Ancient statue."IFO%(3%)=X%:&"Gold nuggets."ifO%(4%)=X%:&"Gold record."IFO%(5%)=X%:&"Gold garbage can."ifO%(6%)=x%
1243 &"Gold pet's dish."ifO%(7%)=X%:&"Gnomes' silver."IFO%(8%)=X%:&"Large diamonds."IFO%(9%)=X%:&"Gold telescope."IFO%(10%)=X%:&"Large emerald."IFO%(11%)=X%:&"Rare coins."IFO%(12%)=X%:&"Rare stamps."ifO%(13%)=X%:&"Black pearl."IFO%(14%)=X%
1244 &"Golden key."ifO%(15%)=X%:&"Cat's leash."IFO%(16%)=X%:&"Electric lamp."IFO%(17%)=X%:&"Glass bottle."IFO%(18%)=X%:&"Water in the bottle."IFO%(19%)=x%:&"Pair of sunglasses."IFO%(20%)=X%:&"Rock."IFO%(21%)=X%:&"Laser pistol."ifO%(22%)=X%
1245 &"Lump of coal."ifO%(23%)=X%:&"Minuature sun in container."IFX%=O%(24%):&"Radiation-proof container."ifO%(25%)=X%:&"Space gloves."ifO%(26%)=X%:&"Remote-control box."ifO%(27%)=X%:&"Old skeleton's bone."IFO%(28%)=X%:&"Piece of cheese."IFO%(29%)=X%
1246 &"Penny."IFO%(30%)=X%:&"Dead rat."IFO%(31%)=X%:&"Cat on leash."ifO%(32%)=X%
1250 IFA$="INVEN"THEN1000ELSE1290
1270 &:&"You have scored";:x%=X%(1%):&X%"points out of 500 possible in"X%(2%)"turns.":x1%=X%/100%:&"This puts you in class "chr$(70%-x1%)".for the next rating you need";
1271 ifx%<500%then&x%-100%*x1%"more points."else&"to find a better program. congratulations!!!"
1280 &:return
1290 ifa$<>"2-7-3"then1310elseifx%(8%)orx%(0%)-80%then&"Hut-Hut-Hike!":goto1000
1300 &"The Gnomes' safe slides open. There is silver inside!":X%(8%)=1%:O%(8%)=80%:goto1000
1310 ifa$="GET"ORA$="TAKE"ORA$="CARRY"ORA$="WEAR"THENV%=1%:GOTO1980
1320 IFA$="DROP"THENV%=2%:GOTO1980
1330 IFA$="THROW"ORA$="TOSS"ORA$="GIVE"THENV%=3%:GOTO1980
1340 IFA$="DRINK"ORA$="EAT"THENV%=4%:GOTO1980
1350 IFA$="OPEN"ORA$="UNLOC"THENV%=5%:GOTO1980
1360 IFA$="KILL"ORA$="HIT"ORA$="FIGHT"ORA$="ATTAC"THENV%=6%:GOTO1980
1370 IFA$="SHOOT"ORA$="FIRE"THENV%=7%:GOTO1980
1380 IFA$="RUB"THENV%=8%:GOTO1980
1390 ifa$="POUR"thenV%=9%:GOTO1980
1400 IFA$="FILL"THENV%=10%:GOTO1980
1410 IFA$="READ"THENV%=11%:GOTO1980
1420 IFA$="FEED"THENV%=12%:GOTO1980
1430 IFA$="LIGHT"ORA$="ON"THENV%=13%:GOTO1980
1440 IFA$="EXTIN"ORA$="OFF"THENV%=14%:GOTO1980
1450 IFA$="PUSH"ORA$="PRESS"ORA$="PULL"THENV%=15%:GOTO1980
1460 IFA$="WIRE"ORA$="COIL"ORA$="PLATI"THENT%=1%:GOTO1980
1470 IFA$="URANI"THENT%=2%:GOTO1980
1480 ifa$="STATU"THENT%=3%:GOTO1980
1490 IFA$="GOLD"ORA$="NUGGE"THENT%=4%:GOTO1980
1500 IFA$="RECOR"THENT%=5%:GOTO1980
1510 IFA$="CAN"THENT%=6%:GOTO1980
1520 IFA$="DISH"THENT%=7%:GOTO1980
1530 IFA$="SILVE"THENT%=8%:GOTO1980
1560 IFA$="DIAMO"THENT%=9%:GOTO1980
1570 IFA$="TELES"THENT%=10%:GOTO1980
1580 IFA$="EMERA"THENT%=11%:GOTO1980
1590 IFA$="COINS"THENT%=12%:GOTO1980
1600 IFA$="STAMP"THENT%=13%:GOTO1980
1610 IFA$="PEARL"THENT%=14%:GOTO1980
1620 IFA$="KEY"THENT%=15%:GOTO1980
1630 IFA$="ME"THENT%=0%:GOTO1980
1640 IFA$="LAMP"THENT%=17%:GOTO1980
1650 IFA$="BOTTL"THENT%=18%:GOTO1980
1660 IFA$="WATER"THENT%=19%:GOTO1980
1670 IFA$="SUNGL"ORA$="GLASS"THENT%=20%:GOTO1980
1680 IFA$="ROCK"THENT%=21%:GOTO1980
1690 IFA$="LASER"ORA$="PISTO"ORA$="GUN"THENT%=22%:GOTO1980
1700 IFA$="LUMP"ORA$="COAL"THENT%=23%:GOTO1980
1710 IFA$="SUN"THENT%=24%:GOTO1980
1720 IFA$="CONTA"THENT%=25%:goto1980
1730 IFA$="GLOVE"THENT%=26%:GOTO1980
1740 IFA$="BOX"ORA$="BUTTO"THENT%=27%:GOTO1980
1750 IFA$="BONE"THENT%=28%:GOTO1980
1760 IFA$="CHEES"THENT%=29%:GOTO1980
1770 IFA$="PENNY"ORA$="COIN"THENT%=30%:GOTO1980
1780 IFA$="LEASH"THENT%=31%:GOTO1980
1790 IFA$="RAT"THENT%=31%:GOTO1980
1800 IFA$="CAT"THENT%=32%:GOTO1980
1810 IFA$="WOMAN"ORA$="GIRL"ORA$="PERSO"ORA$="FEMAL"THENT%=33%:GOTO1980
1820 IFA$="VAULT"THENT%=34%:GOTO1980
1830 IFA$="DWARF"THENT%=35%:GOTO1980
1840 IFA$="DOG"THENT%=36%:GOTO1980
1850 IFA$="GNOME"THENT%=37%:GOTO1980
1860 IFA$="SAFE"THENT%=38%:GOTO1980
1870 IFA$="MESSA"THENT%=39%:GOTO1980
1880 IFA$="ICE"THENT%=40%:GOTO1980
1890 IFA$="ROBOT"THENT%=41%:GOTO1980
1900 IFA$="CART"ORA$="WAGON"THENT%=42%:GOTO1980
1910 IFA$="LEVER"ORA$="SWITC"THENT%=43%:GOTO1980
1920 IFA$="COMPU"THENT%=44%:GOTO1980
1930 IFA$="WALL"THENT%=45%:GOTO1980
1940 IFA$="FUCK"THEN&"Not now-- I've got a headache.":GOTO1000
1950 IFA$="SHIT"THEN&"Does this room look like a toilet?":GOTO1000
1960 IFA$="HELL"THEN&"I can't help you there. You'll have to get there by yourself.":GOTO1980
1965 ifa$=""THEN1980
1970 IFA$="DAMN"THEN&"Swearing is not allowed in this game. What the hell is wrong with you?":GOTO1980
1975 &"I don't know what that means.":GOTO1000
1980 NEXTX%:IFV%=-1%THEN&"What about it?":GOTO1001
1985 ifT%+1%thenifO%(T%)+1%thenifO%(T%)-X%(0%)then2970
1990 ONV%GOTO2085,2180,2180,2180,2300,2380,2410,2500,2520,2550,2570,2650,2760,2760,2820
2000 X%=M%(X%(0%),Y%):IFX%<=0%THEN2870
2010 IFX%(11%)-30%ANDO%(31%)-X%(0%)THEN&"As you walk away, the rat bites you and you die.":GOTO3050
2020 IFO%(37%)=X%(0%)ORO%(35%)=X%(0%)ANDX%=50%THEN&"As you go, you are intercepted by a deadly knife.":GOTO3050
2030 ifo%(36%)=x%(0%)then&"The dog barks wildly, and the City of Gnome awakens, all carrying":&"knives.":goto3050
2040 ifO%(40%)=x%(0%)then&"The ice is to slippery for you to move on.":goto1000
2050 IFO%(41%)=X%(0%)THEN&"On your way out, the robot fires a laser blast which kills you.":goto3050
2060 ifO%(33%)=X%(0%)then&"You are somehow paralyzed by the woman.":goto1000
2065 ifx%=87%thenx%(3%)=296%:&"A woman appears in a cloud of smoke and asks you for your laser.":O%(33%)=87%
2070 ifx%=33%thenm%(X%,4%)=x%(0%)elseifx%(0%)=58%thenM%(x%(0%),y%)=-7%
2080 X%(0%)=x%:ifo%(20%)=-1%oro%(24%)-x%(0%)then980else&"Some light in the room you have entered momentarily blinds you.":goto997
2085 ifT%=-1%then2980
2090 IFO%(T%)-X%(0%)THEN2970ELSEIFT%>X%(11%)THEN&"Don't be stupid.":GOTO1000
2095 Y%=0%:Y%=Y%+1%IFO%(Z%)=-1%FORZ%=0%TOX%(11%):IFY%>8%THEN&"You can't carry so many things at once.":GOTO1000
2100 ift%=11%then&"The emerald falls out of your hand.":goto1000
2110 ift%<>24%andt%<>2%then2130elseifo%(26%)+1%then&"You can't touch that with bare hands.":goto1000
2120 IFO%(25%)+1%THEN2150
2130 GOTO2160
2140 IFT%-32%ORO%(16%)=-1%THEN2160
2150 &"You have nothing in which to carry that.":GOTO1000
2160 o%(T%)=-1%:X%(1%)=X%(1%)-25%ifx%(0%)=1%andt%<16%:ift%=18%ando%(19%)=x%(0%)ort%=19%THENO%(18%),O%(19%)=-1%
2165 ift%=20%ort%=26%then&"You are now wearing them.":GOTO1000
2170 &"Done.":GOTO1000
2180 ift%=-1%then2980
2190 ifo%(T%)+1%then&"You aren't carrying it.":GOTO1000
2200 onv%goto1990,2210,2260,2280
2210 ift%=11%then&"The emerald is stuck to your hand.":GOTO1000
2215 IFT%=25%THENIFO%(24%)=-1%ORO%(2%)=-1%THEN&"You must drop it's contents first.":goto1000
2220 ift%-22%then2230elseifo%(33%)=x%(0%)then&"The woman picks up the laser, examines it, and causes it to disappear.":O%(22%)=0%:ifx%(0%)=87%then3140
2225 ifx%(0%)=86%then&"The wall materializes back over the hole.":M%(86%,1%)=0%
2230 ift%=24%ando%(40%)=x%(0%)then&"The sun melts the ice.":O%(9%)=X%(0%):o%(40%)=0%
2235 ifx%(0%)=85%thenO%(T%),X%(0%)=54%:goto980
2240 O%(T%)=X%(0%):O%(37%-T%)=X%(0%)ifT%=18%ORT%=19%:X%(1%)=X%(1%)+25%IFX%(0%)=1%ANDT%<16%
2241 IFT%=31%ANDO%(32%)=X%(0%)THEN2700
2242 IFT%=28%ANDO%(36%)=X%(0%)THEN2740
2243 IFT%=23%ANDX%(0%)=49%THEN2720
2244 IFT%=32%ANDX%(0%)=57%THEN2985
2245 IFT%=30%ANDX%(0%)=52%THEN2990
2246 IFT%=29%THENIFO%(31%)=X%(0%)THEN2680ELSEIFO%(31%)=X%(0%)ORO%(32%)=X%(0%)THEN&"The animal sniffs at the cheese, and refuses it.":GOTO1000
2250 goto2170
2260 ift%=22%ando%(33%)=X%(0%)then&"The woman catches the laser, examines it, and causes it to disappear.":O%(22%)=0%:ifX%(0%)=87%then3140
2270 ift%=21%thenifO%(37%)=X%(0%)then2995elseifo%(33%)=X%(0%)then3130elseifo%(35%)=X%(0%)then3000elseifO%(41%)=X%(0%)then3010elseifX%(0%)=O%(36%)orX%(0%)=O%(32%)then3020else2210else2210
2280 ifT%=19%then&"The water in this game is polluted and unfit to drink.":GOTO1000
2290 ifT%=29%then3030else&"Don't be funny.":goto1000
2300 ifT%=-1%then2330elseifT%=34%thenX%(0%)=58%:goto980
2310 ifT%=38%then&"You cannot open the safe without a combination.":goto1000
2320 &"It doesn't have a lock.":goto1000
2330 ifX%(0%)=50%then&"The door IS open, stupid!":goto1000
2340 ifx%(0%)=2%then&"The door is locked from the other side.":goto1000
2350 ifX%(0%)-19%then&"Nothing here is locked.":goto1000
2360 ifO%(15%)+1%then&"You don't have a key.":GOTO1000
2370 &"As you unlock the door, you are pushed through it. The door locks behind you.":X%(0%)=2%:goto980
2380 ifT%=-1%then2980elseifT%=32%ort%=36%then3020elseifT%=35%ort%=37%ort%=41%then&"Your bare hands are ineffective.":goto1000
2390 ifT%=33%THen&"An invisible force deflects your fists.":GOTO1000
2400 &"Are you crazy?":goto1000
2410 ifo%(22%)+1%then&"You don't have a gun.":goto1000
2420 IFX%(7%)>5%THEN&"Your laser appears to be empty.":goto1000
2425 ifT%=-1%orT%=22%thenX%=X%(0%):T%=Y%ifO%(Y%)=X%forY%=31%to45%
2430 IFT%=-1%ORT%=22%THEN&"You must specify a target.":goto1001
2440 X%(7%)=X%(7%)+1%:ift%=2%then3110elseift%=31%ort%=35%orT%=37%then&"A laser-sensitive alarm rings, and a blast from the ceiling":&"kills you.":goto3050
2450 ifT%=33%then&"The laser blast breaks up into a harmless display of colors.":goto1000
2460 ift%=45%then&"A hole is blasted in the wall, wide enough for you to get through.":M%(86%,1%)=87%:goto1000
2470 ift%=44%then&"The computer is destroyed.":X%(6%)=1%:goto1000
2480 ift%=32%orT%=36%then3020elseifT%=25%orT%=18%orT%=34%orT%=38%orT%=39%orT%=40%then&"The laser beam strikes it's target and does no damage.":goto1000
2490 &"The laser beam strikes it's target and disintegrates it.":X%(1%)=X%(1%)-25%IFO%(T%)=1%andT%<16%:O%(T%)=0%:goto1000
2500 ifT%=-1%then2980elseift%-11%then2540elseifO%(T%)+1%then&"The emerald sticks to your hand.":goto2160
2510 &"The emerald falls out of your hand.":goto2240
2520 T%=19%IFT%=-1%:IFT%-19%THEN2400ELSEO%(19%)=0%:IFX%(0%)=28%THEN&"There is a loud explosion here, and you are blasted into millions of":&"pieces.":GOTo3050
2530 ifX%(0%)>24%andX%(0%)<28%then&"There is a loud explosion below you.":O%(4%)=28%:goto1000
2540 &"Nothing happens.":goto1000
2550 T%=18%ifT%=-1%:ifT%-18%then2400elseifX%(0%)-9%andX%(0%)<>20%then&"With what? Saliva?":goto1000
2560 X%(19%)=-1%:goto2170
2570 ifT%=-1%then2980
2571 ifO%(T%)-X%(0%)andO%(T%)<>-1%then2970
2572 ifT%<>5%then2600
2573 &:&"                    GNOME RECORDS, L. T. D.":&"                    EVERY TITLE A GUARANTEED GOLD RECORD":&:&'                    "GNOME ME" BY GERRY AND THE GNOMES':&"                    PRODUCED BY ARTHUR H. GNOME":&
2574 &"                    MFG. BY GNOME RECORDS L. T. D.":&:GOTO1000
2600 IFT%=18%THEN&:&"                         WATER:":&:&"                         NO DEPOSIT":&:&"                         NO RETURN":&:goto1000
2610 ifT%=19%then&"In god we trust":&"Liberty":&"1979":&:goto1000
2620 ifT%=34%then&"I suggest you LOOK to see what is written on it.":goto1000
2630 ifT%=39%then&:&"The safe of the Gnomes is near--":&"The combination is 2-7-36":&:goto1000
2640 &"There is nothing written on it.":goto1000
2650 ifT%=-1%then2980elseIFT%<31%orT%=34%orT%>36%then&"It isn't alive.":goto1000
2660 onT%-30%goto2670,2690,2750,1000,2710,2730,2750
2670 ifO%(29%)+1%then2750
2680 O%(29%)=0%:&"The rat eats the cheese and immediately dies.":X%(11%)=31%:goto1000
2690 ifO%(31%)+1%then2750
2700 X%(11%)=32%:O%(31%)=0%:&"The cat eats the rat and appears ready for travel.":GOTO1000
2710 ifO%(23%)+1%then2750
2720 O%(23%),O%(35%)=0%:&"The dwarf eats your coal and leaves.":GOTO1000
2730 ifO%(28%)+1%then2750
2740 O%(28%),O%(36%)=0%:&"The dog takes the bone and leaves.":GOTO1000
2750 &"There is nothing here you can use for food.":GOTO1000
2760 ifO%(17%)+1%then&"You don't have a lamp.":GOTO1000
2765 ifV%=14%then2800
2770 ifX%(3%)>300%then&"The lamp has no more power.":GOTO1000
2780 IFX%(5%)then&"It's already on.":GOTO1000
2790 &"The lamp is lit.":X%(5%)=1%:goto980
2800 IFX%(5%)=0%then&"It isn't on.":GOTO1000
2810 &"The lamp is extinguished.":X%(5%)=0%:GOTO980
2820 ifT%=27%andX%(6%)=0%thenM%(14%,12%),M%(14%,13%)=62%-M%(14%,12%):M%(30%,3%)=45%-M%(30%,3%):M%(32%,11%)=14%-M%(32%,11%):ifX%(0%)-31%then2170else&"A spaceship lands on the launch area and crushes you.":GOTO3050
2830 ifT%=42%andX%(0%)=20%thenO%(3%)=20%:O%(42%)=0%:&"The cart rolls into the bog.":&"An ancient statue flies out!":goto1000
2835 ifT%=33%then&"The woman is mildly irritated at your action. An invisible force pushes you into the wall.":goto1000
2840 ifT%-43%then2540elseM%(33%,4%)=50%-M%(33%,4%):goto2170
2850 IFX%(0%)=10%orX%(0%)=12%orX%(0%)=41%orX%(0%)>42%andX%(0%)<46%orX%(0%)=52%orX%(0%)=78%then&"A A A A a a a a a . . .                    <splat>":GOTO3050
2860 goto2450
2870 ON1%-X%goto2880,2890,2900,2905,2910,2920,2930,2940,2950,2960,3090
2880 &"You can't move in that direction.":GOTO980
2890 &"The door is closed.":goto980
2900 &"I wouldn't advise you to drown yourself.":GOTO980
2905 &"You can't get up the ramp.":GOTO980
2910 &"The barrel's spinning motion prevents you from moving this way.":GOTO980
2920 &"You cannot reach the hole.":GOTO980
2930 &"You are on the surface on the planet, which has an atmosphere of cyanide gas.":goto3050
2940 &"This passage has been somehow blocked.":goto980
2950 &"You are INSIDE an active volcano. Lava flies about in all directions.    The lava seems to be very hot, from the way it burns you when you ":&"touch it.":goto3050
2960 random:X%=85%*rnd+1%:ifm%(X%,0%)=0%then2960else2010
2970 &"It isn't here.":goto1000
2980 &"What?":goto1001
2985 X%(9%)=1%:M%(57%,3%)=19%:O%(15%)=57%:O%(32%)=0%:&"The cat pushes open the vault, then walks away in search of more food.":&"Inside the vault is a golden key!":goto1000
2990 O%(12%)=52%:O%(30%)=0%:goto980
2995 O%(37%)=0%:&"The rock hits the Gnome on the head, and he falls down the cliff.":GOTO1000
3000 &"The dwarf dodges the rock quickly, and throws his weapon, killing you.":goto3050
3010 &"The rock bounces harmlessly off of the robot.":goto1000
3020 &"S.P.C.A. and humane society regulations clearly prohibit you from doing that.":GOTO1000
3030 &"The cheese tastes strange. Evidently it was poisoned, because you died  immediately after eating it.":goto3050
3040 &"Oops! It seems that, not being able to see, you have had a fatal":&"accident."
3050 &"You are dead.":&"Do you want to continue?":gosub3080:ifZ$="N"then3070
3051 O%(X%)=X%(0%)ifO%(X%)=-1%forX%=0%TO15%
3052 O%(X%)=1%IFO%(X%)=-1%FORX%=16%TO32%
3053 X%(0%)=2%:X%(1%)=X%(1%)-10%:X%(10%)=X%(10%)+1%
3054 IFX%(10%)>2%THEN&"Sorry, but people who are incompetent enough to die three times in the ":&"same game don't deserve the chance to make it four.":goto3150
3060 goto980
3070 &"Good riddance.":GOTO3150
3080 Z$=" ":input#1%,Z1$:lsetZ$=Z1$:IFINSTR(1%,"YN",Z$)THENRETURNELSE&'Please answer "YES" or "NO", "Y" or "N".':GOTO3080
3090 ifX%(1%)>380%thenX%=86%:goto2010
3100 &"A woman appears in the next room. She says,":&'"Come back to this game when you learn to read signs..."':&"As she disappears, your surroundings seem to fade...":goto3120
3110 &"You are about to destroy an entire planet with a chain reaction.":&"You must be immediately removed from this game to prevent this..."
3120 close2%:V$=sys("^E^F"):goto3200
3130 &"The rock hits the woman on the head, but she is only stunned.":&"She seems to be very angry at you. Your surroundings fade...":KILLSYS(CHR$(7%)):V$=SYS(CHR$(8%)+"BYE/F"):CHAIN"$LOGOUT"
3140 X%(1%)=X%(1%)+100%:&"A cloud of smoke envelopes both you and the woman...":&:&"You are somewhere overlooking the entire UNDERGROUND game.":&"There is a forest, with a building and a path leading through"
3141 &"the forest and into a cave. There is a lake in the middle of the":&"forest. Somehow you can see into the cave. There are three ancient":&"cities in the cave, two populated by short beings, the other abandoned."
3142 &"A chasm separates two of the cities from the greater part of":&"cave. The cave also contains a room shaped like a barrel, above an":&"abandoned mine. To the north of the cave are a maze of passages and"
3143 &"an active volcano. To the south is a small swamp.":&:&"Around you are various gold objects, plus countless other valuable":&"objects.":&:&"A woman is standing before you. She says,"
3144 &'"The UNDERGROUND game is now yours. You now have all of it''s':&"treasures, and all of it's secrets, inhabitants, and other objects.":goto3150
3150 close2%:killsys(""):goto3200
3160 &"The woman seems to have run out of patience. She disappears.":&"It also seems that while you were paying attention to her, you lost":&"your laser and your lamp.":O%(17%),O%(22%),O%(33%)=0%:goto980
3170 ifX%(4%)then&"Your lamp has run out of power.":X%(5%)=0%:GOTO990
3175 &"A woman appears in a cloud of smoke. She says,":&"Your lamp is getting low on power. I will restore it, but it will cost ":&"you 50 points. You have 3 turns in which to decide."
3177 O%(33%)=O%(17%):IFX%(0%)=O%(17%)ORO%(17%)=-1%THEN1000ELSE&"A cloud of smoke envelopes you and the woman...":X%(0%)=O%(17%):GOTO980
3180 &'"Your time is up," says the woman.':&"Do you want the lamp?":gosub3080:ifZ$="N"then3195
3190 &"The woman touches your lamp, and it seems to glow brighter.":X%(1%)=X%(1%)-50%:X%(3%)=0%:X%(4%)=1%:goto1000
3195 goto1000
3200 END
