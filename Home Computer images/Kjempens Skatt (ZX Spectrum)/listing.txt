1 GO TO 9500
2 LET n=0: LET s=0: LET st=0: LET v=0: LET o=0: LET ne=0: RETURN
10 PRINT '"Jeg kan gaa:";("Nord," AND n<>0);("S0r," AND s<>0);("0st," AND st<>0);("Vest," AND v<>0);("Opp," AND o<>0);("Ned," AND ne<>0);CHR$ 8;" "
11 PRINT "Jeg kan se:"'
12 LET in=0: FOR z=1 TO 14: IF o(z)=linje THEN PRINT o$(z): LET in=1
13 NEXT z: IF in=0 THEN PRINT "Ingenting"
14 RETURN
20 INPUT "Hva naa? "; LINE z$
21 IF z$="N" AND n<>0 THEN LET linje=n: GO TO linje
22 IF z$="S" AND s<>0 THEN LET linje=s: GO TO linje
23 IF z$="0" AND st<>0 THEN LET linje=st: GO TO linje
24 IF z$="V" AND v<>0 THEN LET linje=v: GO TO linje
25 IF z$="O" AND o<>0 THEN LET linje=o: GO TO linje
26 IF z$="NE" AND ne<>0 THEN LET linje=ne: GO TO linje
27 IF z$="SE" THEN GO TO linje
28 IF z$="I" THEN GO TO 900
29 IF z$="N" OR z$="S" OR z$="V" OR z$="0" OR z$="O" OR z$="NE" THEN PRINT "Jeg kan ikke gaa den veien": GO TO 20
30 IF z$=" STOP " OR z$="STOP" THEN GO TO 9995
31 IF z$="SAVE" THEN GO TO 9000
32 IF z$="LOAD" THEN GO TO 9250
33 LET a$="": LET b$="": FOR z=1 TO LEN z$
34 IF z$(z)=" " THEN LET a$=z$( TO z-1): LET b$=z$(z+1 TO ): GO TO 40
35 NEXT z: PRINT "Jeg forstaar ikke hva det betyr": GO TO 20
40 IF a$<>"TA" THEN RETURN
45 GO TO 100
50 FOR z=1 TO 14: IF b$=o$(q, TO LEN b$) THEN GO TO 60
55 NEXT z: PRINT "Hva?": GO TO 20
60 LET o(z)=linje: PRINT "O.K.": GO TO 20
100 FOR z=1 TO 14: IF b$=o$(z, TO LEN b$) THEN GO TO 120
110 NEXT z: PRINT "Hva?": GO TO 20
120 IF o(z)<>linje THEN PRINT "Jeg ser ikke ";b$;" her": GO TO 20
130 LET o(z)=0: PRINT "O.K.": GO TO 20
900 PRINT "Jeg har :"'
910 LET fo=0: FOR z=1 TO 14: IF o(z)=0 THEN PRINT ;o$(z): LET fo=1
920 NEXT z: IF fo=0 THEN PRINT "Ingenting"
930 GO TO 20
1000 GO SUB 2: LET n=1100: LET st=1200: LET v=1300
1010 CLS : PRINT "Jeg staar inne i en hule"
1020 GO SUB 10
1030 GO SUB 20: GO TO 1030
1100 GO SUB 2: LET s=1000: LET v=1400
1101 CLS : PRINT "Jeg gikk inn i et hus.Der var   det en mann som ga meg en fakkel"'
1110 GO SUB 10
1115 LET o(9)=0: LET linje=1150
1120 GO SUB 20: GO TO 1120
1150 CLS : PRINT "Jeg gikk inn i et hus.Der var   det en mann"
1160 GO SUB 10
1170 GO SUB 20: GO TO 1170
1200 GO SUB 2: LET v=1000: LET n=1500: LET st=1600
1210 CLS : PRINT "Foran meg deler gangen seg."
1211 GO SUB 10
1220 GO SUB 20: GO TO 1220
1300 GO SUB 2: LET st=1000: LET v=1700
1310 CLS : PRINT "Jeg datt ned i en innsj0.Jeg    sv0mte inn til kanten.Vest er   det en hytte.Bak meg kom det    et digert sj0uhyre opp bak meg."
1311 GO SUB 10
1312 GO SUB 20
1314 IF a$<>"DREP" THEN PRINT "Uhyret slukte meg.": GO TO 9900
1320 IF a$="DREP" THEN INPUT "Hva skal jeg bruke mot det? "; LINE p$
1330 IF p$<>"FAKKEL" THEN PRINT "Det slukte meg": GO TO 9900
1340 IF o(9)<>0 THEN PRINT "Jeg har ikke fakkel.Det slukte  meg": GO TO 9900
1350 PRINT "Jeg stakk fakkelen i 0yet paa   det.Det skrek,datt bakover og   druknet.": LET linje=1355
1351 GO SUB 20: GO TO 1351
1355 GO SUB 2: LET st=1000: LET v=1700: CLS : PRINT "Vest er det en hytte": GO SUB 10: GO TO 1351
1400 GO SUB 2: LET st=1100: LET v=1800
1410 CLS : PRINT "Jeg ser en diger slange foran   meg.Den ser sulten ut"
1411 GO SUB 10
1412 GO SUB 20
1420 IF a$<>"DREP" THEN PRINT "Den spiste meg": GO TO 9900
1423 INPUT "Hva skal jeg bruke mot det?"'; LINE p$
1430 IF p$<>"SVERD" THEN PRINT "Det spiste meg.": GO TO 9900
1440 IF o(1)<>0 THEN PRINT "Jeg har ikke sverd.Det spiste   meg.": GO TO 9900
1450 PRINT '"Jeg hogg av hodet paa den.": LET linje=1453
1451 GO SUB 2: LET st=1100: LET v=1800: GO SUB 10: GO SUB 20: GO TO 1451
1453 CLS : PRINT "Jeg ser en d0d slange foran meg.": GO TO 1451
1500 GO SUB 2: LET v=2400: LET st=1900
1510 CLS : PRINT "0st er det en d0r.Vest er det endverg."
1520 GO SUB 10: GO SUB 20: GO TO 1520
1600 GO SUB 2: LET v=1200: LET st=2000
1605 CLS : PRINT "0st er det en svart d0r."
1610 GO SUB 10
1620 GO SUB 20: GO TO 1620
1700 GO SUB 2: LET st=1355
1710 CLS : PRINT "Jeg er inne i hytta"
1711 IF o(1)<>0 THEN LET o(1)=linje
1712 IF o(3)<>0 THEN LET o(3)=linje
1713 IF o(4)<>0 THEN LET o(4)=linje
1720 GO SUB 10
1730 GO SUB 20
1740 GO TO 1730
1800 GO SUB 2: LET s=2100: LET st=1453
1810 CLS : PRINT "Jeg ser en kiste.Inni kisten er det en stokk": IF o(6)<>0 THEN LET o(6)=linje
1812 GO SUB 10
1814 GO SUB 20
1820 IF a$="TA" THEN LET linje=1830
1821 GO TO 1814
1830 CLS : PRINT "Jeg ser en kiste"
1831 GO SUB 10
1832 GO SUB 20: GO TO 1832
1900 IF o(4)<>0 THEN PRINT "Jeg har ikke n0kkel": GO TO 1520
1901 GO SUB 8500
1905 GO SUB 2: LET v=1500: LET o=2200
1910 CLS : PRINT "Jeg ser en diger stige foran    meg.Den gaar helt opp i skyene"
1912 GO SUB 10
1913 GO SUB 20: GO TO 1913
2000 IF o(4)<>0 THEN PRINT "Jeg har ikke n0kkel": LET linje=1600: GO TO 1620
2001 GO SUB 2: LET v=1600: LET n=2300
2002 CLS : PRINT "Jeg er i en lang gang."
2010 GO SUB 10
2020 GO SUB 20: GO TO 2020
2100 GO SUB 2: LET n=1800: LET st=1000
2110 CLS : PRINT "Jeg ser en diger hund foran meg.Den angriper meg.": INPUT "Hva skal jeg bruke mot den?"' LINE p$
2111 IF p$<>"FAKKEL" THEN PRINT "Den drepte meg.": GO TO 9900
2120 IF o(9)<>0 THEN PRINT "Jeg har ikke fakkel.Den spiste  hodet mitt": GO TO 9900
2130 PRINT "Den hoppet mot meg og beit meg ihanda.Jeg heiv fakkelen inn i   magen paa den.Den brente bort.": LET o(9)=1: LET linje=2180
2140 GO SUB 10
2150 GO SUB 20: GO TO 2150
2180 CLS : PRINT "Jeg ser en d0d hund foran meg.": GO TO 2140
2200 GO SUB 2: LET ne=1901: LET n=2500
2201 CLS : PRINT "Foran meg er det et digert      slott. Det er saa stort at det  bare kan bo en kjempe der."
2210 GO SUB 10
2220 GO SUB 20: GO TO 2220
2300 GO SUB 2: CLS : PRINT "Jeg vandret mange mil foer jeg  saa en grotte.Nysjerrig som jeg er gikk jeg inn i den,men det   skulle jeg ikke ha gjort.Der vardet fullt av vampyr flaggermusersom sugde ut alt blodet mitt.": GO TO 9900
2400 GO SUB 2: LET st=1500: LET v=2600: LET n=2700
2401 CLS : PRINT "Jeg ser en dverg.Han sier at hankan selge meg en stokk for 100  kroner."
2410 GO SUB 10
2411 GO SUB 20
2420 IF a$="KJ0P" OR a$="GI" THEN GO TO 2430
2421 GO TO 2411
2430 IF o(3)<>0 THEN PRINT "Jeg har ikke 100 kroner.": GO TO 2411
2440 PRINT "Jeg sa at jeg ville kjoepe den, saa jeg ga han 100 kroner,og hanga meg en stokk.": LET o(6)=0: LET o(3)=1
2450 GO SUB 20: GO SUB 10
2500 GO SUB 2: LET s=2200: LET n=2800: LET v=2900: LET st=3000: LET ne=3100
2502 CLS : PRINT "Jeg staar foran doera til       slottet.Doera er laast.Det gaar en trapp ned.  Det ser veldig   moerkt ut der.Vest er det noen  hoeye fjell.0st er det en fager jomfru."
2510 GO SUB 10
2520 GO SUB 20: GO TO 2520
2600 CLS : PRINT "Jeg gikk en stund,saa kom jeg   til en trapp.Jeg klatret opp.Jegkom til en stags Will West by,  De holdt akkurat paa med en     duell.Jeg kom midt imellom      kulene.": GO TO 9900
2700 GO SUB 2: LET s=2400: LET n=3200: LET st=3300
2702 CLS : PRINT "Jeg kom inn i en butikk.Der     har de et tilbud paa et kors.Detkoster bare 100 kroner."
2710 GO SUB 10
2720 GO SUB 20
2722 IF a$="GI" OR a$="KJ0P" THEN GO TO 2730
2723 GO TO 2720
2730 IF o(3)=0 THEN PRINT "Jeg kjoepte den.": LET o(3)=1: LET o(11)=0: GO TO 2720
2740 PRINT "Jeg har ikke 100 kroner": GO TO 2720
2800 IF o(10)<>0 THEN PRINT "Jeg har ikke dirk": LET linje=2500: GO TO 2520
2801 GO SUB 2: LET s=2500: LET n=3400: LET v=3500: LET st=3600
2803 CLS : PRINT "Jeg kom inn i soverommet til    kjempen.Der laa kona hans og    snorket.Jeg snublet i en kost   som stod der.Kona vaaknet og    gikk til angrep paa meg.": INPUT "Hva skal jeg bruke mot henne"' LINE q$
2805 IF q$="PIL OG BUE" AND o(2)=0 THEN PRINT "Jeg skjoet en pil rett i hjertetpaa henne.Den spiddet henne,men pilen smuldret opp,saa jeg heiv buen inn i peisen.": LET linje=2850: GO TO linje
2810 PRINT "Jeg angrep henne med ";q$;'"men hun drepte meg.": GO TO 9900
2850 LET linje=2870: GO SUB 10
2860 GO SUB 20: GO TO 2860
2870 GO SUB 2: LET s=2500: LET n=3400: LET v=3500: LET st=3600: CLS : PRINT "Jeg er i soverommet til kjempen.Kona  hans ligger doed paa      golvet.": GO TO 2850
2900 GO SUB 2: LET st=2500: LET n=3700: LET v=3800
2902 CLS : PRINT "Jeg er i noen hoeye fjell.0st erinngangen til slottet.Vest er   det en hoey.fjelltopp.Det ser utsom det ligger noe paa den      fjelltoppen."
2910 GO SUB 10
2920 GO SUB 20: GO TO 2920
3000 GO SUB 2: LET v=2500: LET st=3900: LET n=4000
3002 CLS : PRINT "Jeg ser en fager jomfru.Hun sierat hun kan gi meg mat i bytte   for en stokk."
3005 GO SUB 10
3007 GO SUB 20
3010 IF a$="GI" THEN GO TO 3020
3015 GO TO 3007
3020 IF o(6)<>0 THEN PRINT "Jeg har ikke stokk.": GO TO 3007
3025 PRINT "Jeg sa at jeg ville gjoere       byttehandelen,og gahun stokken.Hun ga meg maten.Hun ga meg et  kors ogsaa."
3030 LET o(6)=1: LET o(5)=0: LET o(11)=0: GO TO 3007
3100 CLS : PRINT "Jeg gikk ned trappa.Jeg kom inn i vinkjelleren til kjempen.Jeg  drakk litt vin,og litt til,og   litt til,foer jeg falt og slo   hodet mot en spiss bordkant.": GO TO 9900
3200 CLS : PRINT "Jeg kom inn i et maskineri.Der  fant jeg en flaske med noe blaatstoff oppi.Jeg kom til a soele  det paa en maskin.Den ble       levende.Den hoppet oppaa meg,saajeg ble tom for luft.": GO TO 9900
3300 GO SUB 2: LET v=2700
3305 CLS : PRINT "Jeg kom til en innsjoe full av  pirajaer.Paa andre siden er det en baat."
3307 GO SUB 10
3309 GO SUB 20
3310 IF a$="KAST" THEN GO TO 3320
3311 IF a$="SV0M" THEN PRINT "Jeg proevde og svoemme over,men pirajane spiste meg opp.": GO TO 9900
3312 GO TO 3309
3320 IF o(14)<>0 THEN PRINT "Jeg har ikke tau.": GO TO 3309
3322 PRINT "Jeg kastet tauet over.Det festetseg i baaten.Jeg dro inn tauet, og hoppet oppi baaten.Jeg seiltetil den andre siden.": LET linje=3350: GO SUB 2
3325 LET n=4200: LET s=3300: LET st=4300
3330 GO SUB 10
3340 GO SUB 20
3350 GO SUB 2: LET n=4200: LET s=3600: LET st=4300
3351 CLS : PRINT "Jeg er paa den andre siden av   innsjoen.Nord er det en grotte."
3360 GO TO 3330
3400 LET s=2800: LET n=4400: LET v=4500
3405 CLS : PRINT "Jeg er i en gang.Foran meg er   det et vindu.Plutselig dukket   det mange vakter fram."
3407 GO SUB 10
3409 GO SUB 20
3410 IF b$="TAU" THEN LET linje=4600: GO TO linje
3414 PRINT "Vaktene drepte meg.": GO TO 9900
3500 CLS : PRINT "Jeg kom inn i et rom som var    lite til kjemper aa vaere.      Plutselig ble det kaldt.For sentfant jeg ut at jeg var kommet i kjoeleskapet til kjempen.": GO TO 9900
3600 CLS : PRINT "Jeg datt oppi lekegrinda til    babykjempen.Der ble jeg brukt   som rangle.": GO TO 9900
3700 CLS : PRINT "Jeg datt utfor et stup.": GO TO 9900
3800 GO SUB 2: LET st=2900
3805 CLS : PRINT "Jeg klatret opp paa fjelltoppen."
3806 IF o(14)<>0 THEN LET o(14)=linje
3807 IF o(10)<>0 THEN LET o(10)=linje
3810 GO SUB 10
3820 GO SUB 20: GO TO 3820
3900 GO SUB 2: LET v=3000: LET s=1500
3905 CLS : PRINT "Jeg kom en plass der de hadde   fest.Jeg fikk en pil og bue. "
3909 LET o(2)=0
3910 GO SUB 10
3920 GO SUB 20: GO TO 3920
4000 GO SUB 2: LET n=4100: LET s=3000
4005 CLS : PRINT "Jeg moette noen folk som ga meg et kors."
4010 LET o(11)=0
4020 GO SUB 10
4030 GO SUB 20
4100 GO SUB 2: LET s=4000: LET n=4700: LET v=4800
4105 CLS : PRINT "Jeg kom til en eng.Jeg hoerte   uling bak meg.Det var ulver.Jeg sprang,og sprang,foer jeg kom   til en kloeft.Det hang et tau   fra et tre der.Skal jeg proeve  aa slenge meg over,eller proeve aa hoppe over(S/H)": INPUT LINE p$
4110 IF p$="S" THEN PRINT "Jeg hoppet til tauet.Da jeg var kommet halveis over,roek tauet.": GO TO 9900
4115 IF p$<>"H" THEN GO TO linje
4120 PRINT "Jeg klarte saa vidt aa hoppe    over.Jeg er paa den andre siden.": LET linje=4150
4130 GO SUB 10
4140 GO SUB 20
4150 CLS : PRINT "Jeg er paa nordsiden av en      kloeft.Paa andre siden staar    ulvene.": GO SUB 2: LET s=4000: LET n=4700: LET v=4800: GO TO 4130
4200 GO SUB 2: LET s=3350: LET n=4900
4205 CLS : PRINT "Jeg er i en grotte.Det kom ulverbak meg.Hva skal jeg bruke mot  dem?": INPUT LINE p$
4209 IF p$<>"KORS" THEN PRINT "De drepte meg.": GO TO 9900
4210 IF o(11)<>0 THEN LET p$="": GO TO 4209
4215 PRINT "Jeg kylte korset gjennom magen  paa lederulven.De andre flyktet.": LET linje=4250
4220 GO SUB 10
4230 GO SUB 20: GO TO 4230
4250 GO SUB 2: LET n=4900: LET s=3350
4260 CLS : PRINT "Jeg er i en grotte.": GO TO 4220
4300 CLS : PRINT "Jeg gikk en stund foer jeg ble  angrepet av ulver.De drepte meg.": GO TO 9900
4400 GO SUB 2: LET s=3400
4405 CLS : PRINT "Jeg er i skattkammeret til      kjempen.": IF o(12)<>0 THEN LET o(12)=linje
4410 GO SUB 10
4420 GO SUB 20: GO TO 4420
4500 CLS : PRINT "Vaktene kom etter meg og drepte meg.": GO TO 9900
4600 GO SUB 2: LET n=5000: LET st=5100
4605 CLS : PRINT "Jeg er nedenfor vinduet.Jeg tok tauet."
4610 GO SUB 10
4620 GO SUB 20: GO TO 4620
4700 LET s=4150: LET n=5400: LET st=5500
4705 CLS : PRINT "Jeg kom ut paa en lang slette."
4710 GO SUB 10
4720 GO SUB 20: GO TO 4720
4800 CLS : PRINT "Jeg kom til en sump som jeg saa vidt klarte aa komme meg over.  Men jeg fikk en sjelden sykdom. Heldigvis saa var det et sykehusher,men de ville bare hjelpe megom de fikk tauet.Skal jeg gi demdet?(J/N)": INPUT LINE p$
4805 IF p$="N" OR o(14)<>0 THEN PRINT "De drepte meg.": GO TO 9900
4810 IF p$<>"J" THEN GO TO linje
4815 PRINT "De satte en sproeyte paa meg.   Jeg besvimte nesten.Jeg hoerte  at de sa i bakgrunnen~Ha nei,vi tok gal sproeyte paa han her    ogsaa~.": GO TO 9900
4900 GO SUB 2: LET s=4200
4905 CLS : PRINT "Jeg kom i en stor sal.Der var   det en diamant.": IF o(7)<>0 THEN LET o(7)=linje
4910 GO SUB 10
4920 GO SUB 20: GO TO 4920
5000 GO SUB 2: LET s=4600: LET st=5200
5005 CLS : PRINT "Jeg ser en prektig hest foran   meg."
5009 LET b=0
5010 GO SUB 10
5015 GO SUB 20
5020 IF a$="RI" AND b=0 THEN PRINT "Jeg satte meg oppaa den.Den for i galopp rett mot en elv,der denslengte meg av.": GO TO 9900
5025 IF a$="GI" THEN LET b=1: PRINT "Gi hav?": INPUT LINE p$: IF p$<>"MAT" THEN PRINT "Den sparket meg opp saa hoeyt atjeg aldri kom ned igjen.": GO TO 9900
5030 IF b=1 AND p$="MAT" AND o(7)=0 THEN PRINT "Den fraktet meg til en by.": PAUSE 0: LET linje=5300: GO TO linje
5035 IF p$="MAT" THEN LET b=1: GO TO 5015
5040 IF a$="RI" AND b=1 THEN PRINT "DEN FRAKTET MEG TIL EN ELV DER  JEG FOR PAA HAU UTI.": GO TO 9900
5050 GO TO 5015
5100 CLS : PRINT "Jeg datt oppi et hull som gikk  helt ned til jordens indre.": GO TO 9900
5200 CLS : PRINT "Jeg kom i en lang gang.Jeg gikk og gikk,men jeg kom ikke til    slutten.Jeg doede av sult.": GO TO 9900
5300 CLS : PRINT "Jeg moette kongen i byen.Han    sier at han kan hjelpe meg hjem hvis jeg gir ham en ting og     svarer riktig paa en gaate.Hva  skal jeg gi ham?": INPUT LINE p$: IF p$<>"DIAMANT" THEN PRINT "Han godtok det ikke,og drepte   meg.": GO TO 9900
5305 IF o(7)<>0 THEN PRINT "Han drepte meg."
5310 LET o(7)=1: PRINT "Han godtok det.Naa stiller han  meg en gaate.Svarer jeg galt paaden dreper han meg."
5315 LET a=INT (RND*5)+5
5320 IF a=1 THEN PRINT "Hvilket dyr har 4 foetter om    morgenen,2 om dagen og 3 om     kvelden.?": INPUT LINE p$: IF p$<>"MENNESKET" THEN PRINT "GALT.HAN DREPTE MEG.": GO TO 9900
5325 IF a=1 THEN PRINT "RIKTIG": LET linje=6000: GO TO linje
5330 IF a=2 THEN PRINT "En storroeker ble spurt om hvor mange han roekte pr. dag.Han    svarte~Hvis jeg hadde roekt 5   ganger saa mange som jeg gjoer  naa ville jeg ha roekt like     mange over 99 som jeg naa roekerunder 99~.Hvor mange roekte han?": INPUT LINE p$: IF p$<>"33" THEN PRINT "GALT.HAN DREPTE MEG.": GO TO 9900
5335 IF a=2 THEN PRINT "RIKTIG": LET linje=6000: GO TO linje
5340 IF a=3 THEN PRINT "En murstein veier 1kg pluss 1/2 murstein.Hvor mye veier den?": INPUT LINE p$: IF p$<>"2" THEN PRINT "GALT.HAN DREPTE MEG.": GO TO 9900
5345 IF a=3 THEN PRINT "RIKTIG": LET linje=6000: GO TO linje
5350 IF a=4 THEN PRINT "En flaske med kork koster 1.10krFlasken koster 1kr mere enn     korken.Hvor mange oere koster   korken?": INPUT LINE p$: IF p$<>"5" THEN PRINT "GALT.HAN DREPTE MEG.": GO TO 9900
5355 IF a=4 THEN PRINT "RIKTIG": LET linje=6000: GO TO linje
5360 PRINT "Per er 44 aar gammel.Han er     dobbelt saa gammel som Paal var da Per var saa gammel som Paal  er naa.Hvor gammel blir Paal    neste aar?": INPUT LINE p$: IF p$<>"34" THEN PRINT "GALT.HAN DREPTE MEG.": GO TO 9900
5365 PRINT "RIKTIG": LET linje=6000: GO TO linje
5400 GO SUB 2: LET s=4700: LET n=5600: LET st=5700
5405 CLS : PRINT "Jeg er fortsatt paa denne lange sletta."
5410 GO SUB 10
5420 GO SUB 20: GO TO 5420
5500 CLS : PRINT "Jeg falt oppi en felle.Der      sultet jeg ihjel.": GO TO 9900
5600 LET s=5400: LET n=6100: LET v=5800: LET st=5900
5605 CLS : PRINT "Jeg er fortsatt paa denne lange sletta,men det ser ut som den   snart er slutt."
5610 GO SUB 10
5620 GO SUB 20: GO TO 5620
5700 CLS : PRINT "Jeg moette en gjeng mordere som drepte meg.": GO TO 9900
5800 CLS : PRINT "Jeg kom midt i en orkan,der jeg ble blaast til himmels.": GO TO 9900
5900 CLS : PRINT "Jeg doede av sult.": GO TO 9900
6000 IF o(12)<>0 THEN PRINT "Han fulgte meg hjem,og drepte   meg der.": GO TO 9900
6005 CLS : PRINT "Han fulgte meg hjem,der jeg     levde resten av livet mitt."'; FLASH 1;"Du klarte det."
6010 FOR n=1 TO 20: FOR f=0 TO 50 STEP n: OUT 160,n: OUT 320,f: BEEP .005,f: OUT 320,n: NEXT f: NEXT n
6020 GO TO 9990
6100 GO SUB 2: LET s=5600: LET st=6200
6105 CLS : PRINT "Jeg er paa slutten av denne     lange sletta."
6110 IF o(8)<>0 THEN LET o(8)=linje
6120 GO SUB 10
6130 GO SUB 20: GO TO 6130
6200 GO SUB 2: LET st=6300: LET v=6100
6205 CLS : PRINT "Jeg kom inn i en diamantgruve.  Der overrasket jeg noen tyver.  De angrep meg.Hva skal jeg brukemot dem?": INPUT LINE p$: IF p$<>"SPADE" THEN PRINT "De drepte meg.": GO TO 9900
6210 IF o(8)<>0 THEN PRINT "De drepte meg.": GO TO 9900
6220 PRINT "Jeg slo spaden i hodet paa den  foerste,saa han besvimte.Da kom eierne av gruven.De tok tyvene  med seg."
6230 GO SUB 10
6240 GO SUB 20: GO TO 6240
6300 CLS : PRINT "Jeg kom inn i en ny gruve.Der   var det ogsaa tyver.De var blittadvart saa de drepte meg.": GO TO 9900
8500 INK 9: CLS : PLOT 50,10: DRAW 30,140: PLOT 85,10: DRAW 20,140: PLOT 53,20: DRAW 33,0: PLOT 56,40: DRAW 33,0: PLOT 59,55: DRAW 31,0
8501 PLOT 62,70: DRAW 30,0: PLOT 65,85: DRAW 29,0: PLOT 70,100: DRAW 27,0: PLOT 73,115: DRAW 26,0: PLOT 75,130: DRAW 26,0: PLOT 79,145: DRAW 24,0
8502 INK 5: PLOT 10,160: DRAW 30,-10,2.5: DRAW 15,10: PLOT 10,160: DRAW 45,0,-PI/2:
8505 PLOT 125,150: DRAW 10,-10,2: DRAW 15,5,3: DRAW 10,5,1.5
8597 PLOT 125,150: DRAW 10,4,-2.5: DRAW 25,-4,-3: INK 9: PAUSE 0: RETURN
9000 LET o(15)=linje: SAVE "K-CODE "+STR$ linje DATA o()
9005 LET ver=9400
9010 CLS : PRINT "SPOL TILBAKE OG GJ0R KLAR TIL   VERIFY.HVIS DU FAAR TAPE LOADINGERROR,SKRIV GOTO VER            TRYKK EN TAST.": PAUSE 0: INK 2: VERIFY "K-CODE "+STR$ linje DATA o(): INK 9: GO TO linje
9250 LET ver=9400
9260 DIM o(15): CLS : PRINT "TRYKK EN TAST FOR AA LOADE.HVIS DU FAAR TAPE LOADING ERROR,SKRIVGOTO VER.": PAUSE 0: INK 2: LOAD "" DATA o(): INK 9: LET linje=o(15): GO TO linje
9400 CLS : PRINT "1.SAVE"'"2.LOAD"'''"TRYKK DITT VALG"
9410 IF INKEY$="1" THEN GO TO 9000
9420 IF INKEY$="2" THEN GO TO 9250
9430 GO TO 9410
9500 BORDER 1: PAPER 2: INK 9: CLEAR
9501 PRINT AT 2,7; FLASH 1;"KJEMPENS SKATT"
9502 POKE 23658,8
9509 LET A=1
9510 LET a$="K.S.er et adventure spill.Du    skal finne skatten som blir     voktet av en kjempe,og komme degut igjen!"
9511 PRINT
9512 PRINT a$(a);: IF CODE (CHR$ a)<>32 THEN BEEP .005,1
9513 IF a$(a)="!" THEN GO TO 9520
9515 LET a=a+1: GO TO 9512
9520 PRINT : PRINT : LET b=1
9521 LET b$="Ord du kan bruke:N=Nord,S=S0r,  V=Vest,0=0st,O=Opp,NE=Ned,I=Ser du hva du har,SE!"
9522 PRINT b$(b);: IF CODE (CHR$ b)<>32 THEN BEEP .005,2
9523 IF b$(b)="!" THEN GO TO 9530
9524 LET b=b+1: GO TO 9522
9530 PRINT : PRINT : LET c=1
9531 LET c$="RI,TA,KAST,GI,KJ0P,SV0M,DREP,   LAASOPP,Merk: Etter et verb maa det vaere et substantiv. F.eks. TA SVERD,DREP DVERGEN"
9532 PRINT c$(c);: IF CODE (CHR$ c)<>32 THEN BEEP .005,3
9533 IF c$(c)="N" THEN GO TO 9540
9534 LET c=c+1: GO TO 9532
9540 PRINT : PRINT : LET d=1
9541 LET d$="Du kan ogsaa bruke SAVE og LOAD."
9542 PRINT d$(d);: IF CODE (CHR$ d)<>32 THEN BEEP .005,4
9543 IF d$(d)="." THEN GO TO 9545
9544 LET d=d+1: GO TO 9542
9545 LET e=1: PRINT : PRINT
9546 LET e$="Du bruker STOP hvis du skal     slutte."
9547 PRINT e$(e);: IF CODE (CHR$ e)<>32 THEN BEEP .005,5
9548 IF e$(e)="." THEN GO TO 9550
9549 LET e=e+1: GO TO 9546
9550 PRINT #0;"   TRYKK EN TAST FOR AA STARTE"
9560 PAUSE 0
9561 DIM o$(14,13): DIM o(16)
9570 RESTORE 9600: FOR z=1 TO 14: READ o$(z),o(z): NEXT z
9580 LET linje=1000: GO TO linje
9600 DATA "SVERD",37,"PIL OG BUE",1,"100 KRONER",4600,"N0KKEL",1,"MAT",4100,"STOKK",4300,"DIAMANT",1,"SPADE",3100,"FAKKEL",1,"DIRK",1,"KORS",2,"SKATT",1,"KJ0TT",8,"TAU",6200
9900 PRINT '''"Bedre lykke neste gang"
9995 PRINT '"Et nytt spill (J/N)": INPUT LINE q$
9996 IF q$="N" THEN PRINT AT 21,0;"DU MAA LOADE DET INN PAA NYTT": POKE 23613,0: GO TO 9999
9997 IF q$="J" THEN RUN
9998 GO TO 9995
9999 PAUSE 0: GO TO 9996
