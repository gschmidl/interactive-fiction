r"""Rebuild C:\ADV751\, the folder Carl Ruby's QBasic ADV751.BAS runs in, from the files he
e-mailed Arthur O'Dwyer in March 2014 (../src_original/qbasic-adv751).

Ruby sent the game's messages as three dumps, rooms.txt, texts.txt and 1timers.txt: the small
.TXT files the program reads, pasted one after another, each still ending in its "//" line, but
with the file names gone.  This script cuts them apart again and gives every piece the name and
folder ADV751.BAS opens it by (the tables below; README.md says how each was identified).
history.txt is BOOK.TXT, the book in the safe.

Two folders the program reads were never sent: CHANGE\ (one-line descriptions that replace an
object's own when something happens to it) and HINTS\ (the offers of help and the hints).  The
program cannot show even the first room without CHANGE\GENTLE, so every missing file gets a
stand-in made only from this game's own words (STAND_INS).

    python make_tree.py [OUT] [--no-fixes]

writes OUT (default: ADV751 beside this script), keeping a VAR that is already there.  VAR, the
program's data file, is made by Ruby's REFILL.BAS under QBasic, not here: make_var.py, again
whenever REFILL.BAS changes (see README.md).  --no-fixes leaves out FIXES and REFILL_FIXES,
Ruby's own slips, and ADDITIONS, this project's debug command #BEAR, for the program as he sent
it (PATCHES, which make it run in this setup, always go in).
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src_original", "qbasic-adv751")

# rooms.txt, entry by entry.  Mostly room order; each alternative description follows its room.
ROOMS = ([f"RMS/ROOM{n}" for n in range(1, 28)] + ["RMS/ALT27"]
         + [f"RMS/ROOM{n}" for n in range(28, 39)] + ["RMS/ALT38"]           # ALT38: never read
         + [f"RMS/ROOM{n}" for n in range(39, 63)] + ["RMS/ROOM88"]
         + [f"RMS/ROOM{n}" for n in range(121, 126)] + [f"RMS/ROOM{n}" for n in range(127, 135)]
         + [f"RMS/ROOM{n}" for n in range(137, 140)] + ["RMS/ALT139"]
         + [f"RMS/ROOM{n}" for n in range(140, 158)] + ["RMS/ALT157"]         # ALT157: never read
         + [f"RMS/ROOM{n}" for n in range(158, 164)]
         + ["RMS/ROOM230"]           # never read: the program prints room 230 from A$(34)
         + ["RMS/ROOM164", "RMS/ROOM165", "RMS/ROOM166", "RMS/ROOM167", "RMS/ROOM168"]
         + [f"RMS/ROOM{n}" for n in range(170, 178)]
         + ["RMS/ROOM180", "RMS/ROOM178", "RMS/ROOM179"]   # 178/179: two identical placeholders
         + [f"RMS/ROOM{n}" for n in range(181, 200)] + ["RMS/ALT199"]
         + [f"RMS/ROOM{n}" for n in range(200, 210)])

# texts.txt, in the alphabetical order of the names.  None: a message no line of the program
# reads, so its name is unknown.  The CASABLNC entry is two files: CASABLNC.TXT (read with two
# INPUT #s, so it has no "//") ran straight on into CAVE.TXT.
TEXTS = ["BILBOARD", "BRIDGE0", "BRIDGE1", "BRSHAKES", "CARD", ("CASABLNC", "CAVE"), "CAVEIN-1",
         "CHUCKLE", "CHUTE1", "CHUTE2", "COPTER", "DOCUMENT", "DRAGGED", "DRINK", "DUSTROCK",
         "ENGSTART", None, "FIND", "FNORD!", "GLUTTONY", "HOURS", "INFRMOUT", "LEAFLET", "MIST",
         "NORTH", "OYSTER", "PAPER", "PASTBEAR", "PHUCE1", "PHUCE2", "PIRATE", "PLANT1", "PLANT2",
         "PLANT3", "POSTER", "SANDWICH", "SHIELD", "SOMETHIN", "SORRY", None, "STOP", "TOOBRITE",
         "TOOCOLD", "TREES", "TROLLAXE", "TUNNEL",
         "1TIMERS/VOICE2", "1TIMERS/VOICE2A"]      # dumped here, but read from 1TIMERS\

# With the fixes, KILL TROLL prints texts.txt's troll message again (line 2310 printed BB$(8),
# "oil.").  No line names that file, so FENDOFF is this project's name for it: alphabetically
# it lies between ENGSTART and FIND.
FIX_TEXTS = {16: "FENDOFF"}

# 1timers.txt, alphabetical too.  NOPOINT and POWDER are read from the top folder.
ONCE = ["AXEMISS", "CABINET", "CLOSED!", "COLLAPSE", "DEATH0", "DEATH1", "DEATH2", "DEATH3",
        "DIMGOBAK", "DIMRPLAC", "DIMWRAP", None, "FIRSTAXE", "FLURRY1", "FLURRY2", "FREEDUP",
        "HIDDEN", "HORN", "INLAID", "LUMBERS", "MUSHROOM", "../NOPOINT", "PEARL", "POOF1",
        "POOF2", "../POWDER", None, "SAVE", "SLOT", "SUSPEND", "VANQUISH", "VASEDROP", "VOICE1",
        "WUMPWAKE"]

# With the additions, #BEAR prints 1timers.txt's message for the bear calming down ("The bear
# eagerly wolfs down your food ..."), which no line reads: the bear is never tamed.  FEDBEAR is
# this project's name for it, between DIMWRAP and FIRSTAXE like the entry.
FIX_ONCE = {11: "FEDBEAR"}

# Stand-ins for the files that were never sent.  A CHANGE\ file is one INPUT # item (DRAGON:
# two, the dragon and then the rug), so no commas; the texts are plain "There is ... here."
# lines made of this game's own names for the things.  HINTS\ files are read like any .TXT.
STAND_INS = {
    "CHANGE/GENTLE": ["There is a cave bear here."],
    "CHANGE/CONTENTE": ["There is a contented cave bear here."],     # "CONTENTED", 8.3
    "CHANGE/CHAIN": ["There is a golden chain here."],
    "CHANGE/OLDBTTRY": ["There are some old batteries here."],
    "CHANGE/POSTER": ["There is a faded poster here."],
    "CHANGE/SHARDS": ["There are shards of a ming vase here."],
    "CHANGE/AXE": ["There is a little axe here."],                   # the program's own line 504
    "CHANGE/DRAGON": ["There is a dead dragon here.", "There is a persian rug here."],
    "CHANGE/MASSIVE": ["There is a massive iron door here."],
    "CHANGE/PLANT1": ["There is a tiny little plant in the pit."],
    "CHANGE/PLANT2": ["There is a 12-foot-tall beanstalk in the pit."],
    "CHANGE/PLANT3": ["There is a gigantic beanstalk in the pit."],
    "CHANGE/CLOAK": ["There is a velvet cloak here."],
}
for n in range(2, 7):
    STAND_INS[f"HINTS/ASK{n}.TXT"] = ["Do you need help?", "//"]
    STAND_INS[f"HINTS/HINT{n}.TXT"] = ["The text of this hint has been lost.", "//"]
STAND_INS["HINTS/ALT1.TXT"] = ["The text of this hint has been lost.", "//"]

# Changes to the program itself, each (file, old, new, what for).  Every old text must occur once.
PATCHES = [
    ("HELLO.BAS", b'1300 RUN "C:\\WINDOWS\\ADV\\ADV751.BAS"', b'1300 RUN "C:\\ADV751\\ADV751"',
     "HELLO.BAS still ran the game from an older folder (no extension: QBasic adds .BAS, a "
     "compiled HELLO.EXE .EXE)"),
    ("ADV751.BAS", b"7890 END", b"7890 SYSTEM",
     "after SAVE: leave QBasic instead of stopping in its editor"),
    ("ADV751.BAS", b"GOSUB 9991: STOP", b"GOSUB 9991: GOTO 9989",
     "when the cave closes: show the score (QUITS.BAS) instead of breaking into the editor"),
    ("QUITS.BAS", b"5135 END", b"5135 SYSTEM",
     "after the top rating: leave QBasic like every other rating does"),
]

# Ruby's slips, fixed unless --no-fixes: references that point at the wrong thing.  Ruby
# renumbered his objects more than once (now treasures 1-44, other things 45-100, fixed
# features 101-132), and some lines kept an old number, each era its own: the same old number
# can mean the keys in one routine and the bottle in another.  So every fix below follows the
# routine's own message or logic and Ruby's current tables (K$, J, I$, P$, CO in the program
# and REFILL.BAS), never a blanket renumbering.  Each old text occurs exactly once.
FIXES = [
    # grate: keys are 46 (was 21 and 16), the grate 114 (was 43); line 1008 put the axe (58)
    # at the grate instead of the grate itself
    (b"1008 IF P = 8 OR P = 9 THEN J(58) = P", b"1008 IF P = 8 OR P = 9 THEN J(114) = P"),
    (b"2718 IF J(21) = P OR J(21) = 300", b"2718 IF J(46) = P OR J(46) = 300"),
    (b"2725 IF K = 43 THEN", b"2725 IF K = 114 THEN"),
    (b"3230 IF (K = 5 OR K = 43) AND J(16) <> P AND J(16) <> 300",
     b"3230 IF (K = 5 OR K = 114) AND J(46) <> P AND J(46) <> 300"),
    (b"3260 IF K = 43 THEN", b"3260 IF K = 114 THEN"),
    (b"8405 IF J(16) <> P AND J(16) <> 300", b"8405 IF J(46) <> P AND J(46) <> 300"),
    # the snake is 101 (was 31, a placeholder that is never in room 23)
    (b"120 IF J(31) = 23", b"120 IF J(101) = 23"),
    (b"125 IF P = 23 AND J(31) = 23", b"125 IF P = 23 AND J(101) = 23"),
    (b"THEN SN = 0: J(31) = 0: GOTO 2180", b"THEN SN = 0: J(101) = 0: GOTO 2180"),
    # the bird is 49, the cage 48, the pillow 54, the knife 119; DROP BIRD tested a flag CAGE
    # that nothing sets: the cage is container 2, open when CI(2) = 1 (it starts at 2, closed
    # but see-through, like the bottle)
    (b'cave.": J(49) = 0', b'cave.": J(119) = 0'),
    (b'2057 IF M$ = "BIRD" AND J(23) <> 300', b'2057 IF M$ = "BIRD" AND J(48) <> 300'),
    (b'2059 IF M$ = "CAGE" AND J(24) = P THEN J(24) = 300',
     b'2059 IF M$ = "CAGE" AND J(49) = P THEN J(49) = 300'),
    (b"2111 IF K = 12 AND J(24) <> P", b"2111 IF K = 12 AND J(54) <> P"),
    (b"2112 IF K = 12 AND J(24) = P", b"2112 IF K = 12 AND J(54) = P"),
    (b'2120 IF M$ = "BIRD" AND CAGE = 0', b'2120 IF M$ = "BIRD" AND CI(2) <> 1'),
    (b'GOSUB 8900: J(19) = 0: GOTO 100', b'GOSUB 8900: J(49) = 0: GOTO 100'),
    (b'2128 IF M$ = "CAGE" AND J(48) = 300 THEN J(48) = P',
     b'2128 IF M$ = "CAGE" AND J(49) = 300 THEN J(49) = P'),
    (b'2820 IF M$ = "SNAKE" AND J(24) = 300 THEN J(24) = 0',
     b'2820 IF M$ = "SNAKE" AND J(49) = 300 THEN J(49) = 0'),
    (b"2390 IF J(49) = P THEN H = H - 1", b"2390 IF J(49) = 300 THEN H = H - 1"),
    # the dwarf is 112 (was 54, the pillow, and 42, which printed "42" in the room)
    (b"1409 IF DW > 0 THEN J(54) = P", b"1409 IF DW > 0 THEN J(112) = P"),
    (b"1420 J(42) = P", b"1420 J(112) = P"),
    (b"2260 IF J(54) = P THEN", b"2260 IF J(112) = P THEN"),
    (b"2290 DW = DW - 1: IF DW = 0 THEN J(54) = 0", b"2290 DW = DW - 1: IF DW = 0 THEN J(112) = 0"),
    (b"2330 IF J(54) = P THEN", b"2330 IF J(112) = P THEN"),
    # the bear is 99 (was 29, 42, 44 and 97, the Wumpus); KILL BEAR gets BB$(5), the
    # "*HIS* bear hands" line REFILL.BAS defines and nothing printed
    (b"AND P = 51 AND J(29) = 300 THEN", b"AND P = 51 AND J(99) = 300 THEN"),
    (b"300 IF J(97) = 300 THEN", b"300 IF J(99) = 300 THEN"),
    # (and the file's name cut to 8 letters: MS-DOS does that itself, DOSBox gives File not found)
    (b'"CONTENTED": NI = 42:', b'"CONTENTE": NI = 99:'),
    (b"2920 IF J(42) = 300", b"2920 IF J(99) = 300"),
    (b"2930 IF J(42) = 300", b"2930 IF J(99) = 300"),
    (b"TR = 0: J(29) = 0: GOTO 9800", b"TR = 0: J(99) = 0: GOTO 9800"),
    (b"3075 IF J(44) = 300 THEN", b"3075 IF J(99) = 300 THEN"),
    (b"2350 IF P = 59 AND BE = 1 THEN 9070", b"2350 IF P = 59 AND BE = 1 THEN PRINT BB$(5): GOTO 100"),
    # the dragon: KILL's test was the wrong way round (it said "I see no dragon here" when the
    # dragon was there); its question WW$ is BB$(4); its change goes to its own
    # description, 102 (was 30, a placeholder)
    (b"2300 GOSUB 9600: IF J(K) = P AND", b"2300 GOSUB 9600: IF K > 0 AND J(K) <> P AND"),
    (b"2360 PRINT WW$:", b"2360 PRINT BB$(4):"),
    (b'2385 M$ = "DRAGON": NI = 30:', b'2385 M$ = "DRAGON": NI = 102:'),
    (b"8967 IF NI = 30 THEN", b"8967 IF NI = 102 THEN"),
    # the troll: KILL TROLL printed BB$(8), "oil."; he took only treasures below 20.  Walking
    # onto the bridge (SW from its far side, 51; NE from the near one, 50: the travel table's own
    # crossings) passed him, and a fallen bridge, as if they were not there: only CROSS looked
    # (2920-2940: the troll, paid or not, the bridge, the bear).  Line 129 sent the walk there
    # only for the bear with the troll gone; now every crossing on foot goes through CROSS's lines
    (b"THEN PRINT BB$(8): GOTO 100", b'THEN M$ = "FENDOFF": GOSUB 8900: GOTO 100'),
    (b"2230 IF K < 20 AND", b"2230 IF K < 45 AND"),
    (b'129 IF D$ = "SW" AND TR = 0 AND P = 51 AND J(99) = 300 THEN 2945',
     b'129 IF (D$ = "SW" AND P = 51) OR (D$ = "NE" AND P = 50) THEN 2920'),
    # the clam is 52 and the oyster 53 (were 47, the lamp, 27, the sword - opening the clam
    # made the sword vanish - and 28)
    (b"2716 IF J(47) = P OR J(47) = 300", b"2716 IF J(52) = P OR J(52) = 300"),
    (b"2717 IF J(52) = P OR J(52) = 300", b"2717 IF J(53) = P OR J(53) = 300"),
    (b"2755 J(28) = J(27): J(27) = 0:", b"2755 J(53) = J(52): J(52) = 0:"),
    # the safe: GET POSTER revealed it but, unlike GET ALL (line 1921), left it out of the room
    (b"NI = 71: GOSUB 8950: GOTO 2090", b"NI = 71: GOSUB 8950: J(131) = 2: GOTO 2090"),
    # LOOK: tested object K3 even with no object (0), and K3 was never cleared; and it repeated
    # the long description only from the third view (NU(P) > 1): the first LOOK after arriving
    # gave the short one and left the long one for the next room shown
    (b'M$ = "": K = 0: FOR X = 1 TO 10', b'M$ = "": K = 0: K3 = 0: FOR X = 1 TO 10'),
    (b"4801 GOSUB 9600: IF J(K3) <> P", b"4801 GOSUB 9600: IF K3 > 0 AND J(K3) <> P"),
    (b"318 IF NU(P) > 1 AND LD = 1", b"318 IF NU(P) > 0 AND LD = 1"),
    # the bridges are 110 and the fissure 111 (were 39 and 40); CB$ is BB$(11)
    (b"THEN J(39) = P: J(40) = P: IF CB = 1 THEN PRINT CB$",
     b"THEN J(110) = P: J(111) = P: IF CB = 1 THEN PRINT BB$(11)"),
    (b"IF BR = 1 THEN J(39) = P", b"IF BR = 1 THEN J(110) = P"),
    # the cave closes with the magazine at Witt's End: the magazine is 55 (was 25)
    (b"1113 IF JT = 15 AND P > 9 AND J(25) = 62", b"1113 IF JT = 15 AND P > 9 AND J(55) = 62"),
    (b"(JT = 15 AND J(25) = 62) THEN DW = 0", b"(JT = 15 AND J(55) = 62) THEN DW = 0"),
    (b"(JT = 15 AND J(25) = 62) OR (P > 48", b"(JT = 15 AND J(55) = 62) OR (P > 48"),
    (b"1408 IF SE = 1 OR (J(25) = 62", b"1408 IF SE = 1 OR (J(55) = 62"),
    # batteries are 56 (were 31 and 26); the vending machine is in room 295 (was 124, a
    # forest), where DIMWRAP sends you with coins
    (b"1310 IF NB = 1 AND (J(31) = 300 OR J(31) = P)", b"1310 IF NB = 1 AND (J(56) = 300 OR J(56) = P)"),
    (b"2113 IF K = 3 AND P = 124 THEN J(K) = 0: J(26) = P: PRINT I$(26)",
     b"2113 IF K = 3 AND P = 295 THEN J(K) = 0: J(56) = P: PRINT I$(56)"),
    # words: FOOD is 50 (was 25), PHONE 128 (125), PLANT 107 (32), CAVE 121 (50), TABLET 104
    # (33); the bottle is 51 (21), the shovel 66 (72); the slippers also click when worn
    (b"2887 IF K <> 25 THEN", b"2887 IF K <> 50 THEN"),
    (b"3460 IF K = 125 THEN", b"3460 IF K = 128 THEN"),
    (b"3505 IF K > 0 AND K <> 32 AND", b"3505 IF K > 0 AND K <> 107 AND"),
    (b"4210 IF K = 50 THEN", b"4210 IF K = 121 THEN"),
    (b"4300 IF WB = 1 AND (J(21) = P OR J(21) = 300)", b"4300 IF WB = 1 AND (J(51) = P OR J(51) = 300)"),
    (b"4520 IF K = 33 OR", b"4520 IF K = 104 OR"),
    (b"6910 IF J(72) = P OR J(72) = 300", b"6910 IF J(66) = P OR J(66) = 300"),
    (b"4905 IF J(78) <> 300 THEN", b"4905 IF J(78) <> 300 AND J(78) <> 400 THEN"),
    # bounds: V() has treasures 1-15 (taking 16-19 back from the building ran off its end);
    # portable things now go to 86, the matches
    (b"2095 IF K < 20 AND P = 2", b"2095 IF K < 16 AND P = 2"),
    (b"1900 FOR X = 1 TO 83:", b"1900 FOR X = 1 TO 86:"),
    (b"IF H1 = 84 THEN 3050", b"IF H1 = 87 THEN 3050"),
    (b"3055 FOR X = 1 TO 83", b"3055 FOR X = 1 TO 86"),
    # jumps: the plover tunnel went to 4750, a subroutine (RETURN without GOSUB), not 4725;
    # ORIENT set Q and then skipped the move (730); PIT moved without describing the room
    (b"J(8) <> 300)) THEN 4750", b"J(8) <> 300)) THEN 4725"),
    (b"1820 IF P = 33 THEN Q = 35: GOTO 210", b"1820 IF P = 33 THEN Q = 35: GOTO 730"),
    (b"6610 IF P = 36 THEN P = 38: GOTO 100", b"6610 IF P = 36 THEN P = 38: GOTO 80"),
    (b"6620 IF P = 37 THEN P = 39: GOTO 100", b"6620 IF P = 37 THEN P = 39: GOTO 80"),
    (b"6630 IF P = 66 THEN P = 80: GOTO 100", b"6630 IF P = 66 THEN P = 80: GOTO 80"),
    # "Please answer the question first." is BB$(1): PL$ was never set
    (b"7760 PRINT PL$:", b"7760 PRINT BB$(1):"),
    (b"8250 PRINT PL$:", b"8250 PRINT BB$(1):"),
    (b"8340 PRINT : PRINT PL$:", b"8340 PRINT : PRINT BB$(1):"),
    (b"9956 PRINT : PRINT PL$:", b"9956 PRINT : PRINT BB$(1):"),
    # EAT with nothing named: a misplaced bracket made J(-1), "Subscript out of range"
    (b"4620 IF K = 0 AND J(50 <> P AND J(50) <> 300) THEN",
     b"4620 IF K = 0 AND J(50) <> P AND J(50) <> 300 THEN"),
    # DROP POWDER: CLOTH, like CAGE, is a container's open flag from before CI(), and nothing
    # sets it: the cloth bag is container 4.  2103 tested the powder as 43 in an old container
    # code (252); it is 57, in the bag (304).  BB$(18), the start of "... cloth bag.", was never
    # written and lay outside BB$(15) ("Subscript out of range"): a stand-in, "First open the ".
    # OPEN's container search went on from where the last one had stopped (OC), so opening a
    # second container could miss it
    (b"V(15), BB$(15), TILE$(12)", b"V(15), BB$(18), TILE$(12)"),
    (b"27 FOR X = 0 TO 15: INPUT #1, BB$(X): NEXT X",
     b'27 FOR X = 0 TO 15: INPUT #1, BB$(X): NEXT X: BB$(18) = "First open the "'),
    (b'2103 IF M$ = "POWDER" AND J(43) = 252 THEN', b'2103 IF M$ = "POWDER" AND J(57) = 304 THEN'),
    (b'2115 IF M$ = "POWDER" AND CLOTH = 0 THEN', b'2115 IF M$ = "POWDER" AND CI(4) <> 1 THEN'),
    (b"2701 GOSUB 9600: IF K > 0", b"2701 GOSUB 9600: OC = 0: IF K > 0"),
    # subroutines left by GOTO: each such exit leaves a return address on QBasic's stack, and
    # about 310 of them stop the program ("Out of stack space").  The room display (GOSUB 1000)
    # printed the safe, the grate and the tiled door through the commands' own lines, which end
    # in GOTO 100: they now print and carry on with the display.  The hint dialog and the dim
    # lamp's warning return; the "don't understand" answers go back as they did, by RETURN 100.
    (b'1003 PRINT "The safe door"; : ON CI(1) + 1 GOTO 9380, 9370',
     b'1003 PRINT "The safe door"; : IF CI(1) = 0 THEN PRINT " is locked." ELSE PRINT " is open."'),
    (b'1008 IF P = 8 OR P = 9 THEN J(114) = P: PRINT "The grate "; : ON GA + 1 GOTO 9380, 9370\r\n',
     b'1008 IF P <> 8 AND P <> 9 THEN 1014\r\n'
     b'1009 J(114) = P: PRINT "The grate "; : IF GA = 0 THEN PRINT " is locked." ELSE PRINT " is open."\r\n'),
    (b"1048 IF (P = 149 OR P = 150) AND SSD = 1 THEN 9215", b"1048 IF (P = 149 OR P = 150) AND SSD = 1 THEN GOSUB 9215"),
    (b"1049 IF P = 149 THEN ON LGT + 1 GOTO 9200, 9205, 9210",
     b"1049 IF P = 149 AND SSD = 0 THEN ON LGT + 1 GOSUB 9200, 9205, 9210"),
    (b'9215 PRINT "The stainless steel door"; : ON SSD + 1 GOTO 9380, 9390\r\n',
     b'9215 PRINT "The stainless steel door"; : IF SSD = 0 THEN PRINT " is locked." ELSE PRINT " is unlocked."\r\n'
     b'9216 RETURN\r\n'),
    (b"8220 IF H$ = \"N\" OR H$ = \"NO\" THEN GOSUB 9000: GOTO 100",
     b"8220 IF H$ = \"N\" OR H$ = \"NO\" THEN GOSUB 9000: RETURN"),
    (b"8330 IF H$ = \"N\" OR H$ = \"NO\" THEN AR = 0: GOSUB 9000: GOTO 100",
     b"8330 IF H$ = \"N\" OR H$ = \"NO\" THEN AR = 0: GOSUB 9000: RETURN"),
    (b"8420 AR = 0: GOTO 100", b"8420 AR = 0: RETURN"),
    (b'"DIMWRAP": GOSUB 8900: GOTO 100', b'"DIMWRAP": GOSUB 8900: RETURN'),
    (b"1390 IF P = 25 THEN PRINT : PRINT GH$: GOTO 100", b"1390 IF P = 25 THEN PRINT : PRINT GH$: RETURN 100"),
    # ...and LOOK with a word it does not know went there by GOTO, so three times in four the
    # answer's RETURN had no GOSUB to go back to ("RETURN without GOSUB")
    (b"6530 PRINT \"I don't understand the word \"; M$: GOTO 100",
     b"6530 PRINT \"I don't understand the word \"; M$: RETURN 100"),
    (b'4816 IF M$ <> "IN" THEN 6500', b'4816 IF M$ <> "IN" THEN GOSUB 6500: GOTO 100'),
    (b"4830 IF K2 = 0 THEN 6500", b"4830 IF K2 = 0 THEN GOSUB 6500: GOTO 100"),
    (b"2884 IF K = 0 AND TL = 0 THEN 6530", b"2884 IF K = 0 AND TL = 0 THEN GOSUB 6530"),
    # health: the drain had no floor, and below 0 the health messages index H2$(-1)
    (b"104 IF HLTH > 100 THEN HLTH = 100\r\n", b"104 IF HLTH > 100 THEN HLTH = 100\r\n105 IF HLTH < 0 THEN HLTH = 0\r\n"),
    # IN kept going through its table after a match: below the grate it went on from 10 to 47
    (b"2435 IF P = EN(X) THEN P = EX(X): INX = 1", b"2435 IF P = EN(X) THEN P = EX(X): INX = 1: X = 7"),
    # the tiled door: the tiles are TILE$(4-12) but only 1-9 were looked at (ORANGE, NACRE and
    # BLACK were never recognised, the last press counted instead); a wrong tile used the
    # display's list without its first entry, so the second one skipped yellow and the third
    # fell through to "Done." as if right.  The light now stops at red.
    (b"5731 FOR X = 1 TO 9", b"5731 PTI = 0: FOR X = 1 TO 12"),
    (b"5740 LGT = LGT + 1: ON LGT + 1 GOTO 9205, 9210\r\n",
     b"5740 LGT = LGT + 1: IF LGT > 2 THEN LGT = 2\r\n5741 ON LGT + 1 GOSUB 9200, 9205, 9210: GOTO 100\r\n"),
    # LOOK in BRIEF mode skipped the "Sorry" and also the long description it promises
    (b"4881 IF BF = 1 THEN 4884", b"4881 IF BF = 1 THEN LD = 1: GOTO 4884"),
    # EXAMINE and PUT shared BUILDING's line; they get the answer OFFICE, WALL and TERSE get
    (b"ON W - 90 GOTO 1800, 1800, 1800, 9005", b"ON W - 90 GOTO 1800, 2915, 2915, 9005"),
    # Ruby's debugging prints: OIL on anything but the door printed 88 (the routine's own answer
    # elsewhere is 2915), LIGHT printed the lamp's state, LOOK IN the container's number
    (b"THEN PRINT 88: GOTO 100", b"THEN 2915"),
    (b'4020 PRINT "Your lamp is now on."; LS', b'4020 PRINT "Your lamp is now on."'),
    (b"4825 NEXT X: PRINT K2, CI(K2)", b"4825 NEXT X"),
    # TH$ and NH$ end in a space in REFILL.BAS, which INPUT # drops ("hereto eat", "toattack");
    # the two lines that made up for it lose their extra space
    (b"29 INPUT #1, TH$\r\n", b'29 INPUT #1, TH$: TH$ = TH$ + " "\r\n'),
    (b"30 INPUT #1, NH$\r\n", b'30 INPUT #1, NH$: NH$ = NH$ + " "\r\n'),
    (b'PRINT NH$; " eat."', b'PRINT NH$; "eat."'),
    (b'PRINT NH$ + " with which', b'PRINT NH$ + "with which'),
    # LOCK GRATE printed GA$(0), which nothing sets: the game's own "The grate" + " is locked."
    (b"THEN GA = 0: PRINT GA$(0): GOTO 100", b'THEN GA = 0: PRINT "The grate"; : GOTO 9380'),
    # SAVE kept the score, the places of things and the flags of the 350-point game, not the
    # rest: after RESUME the grate was locked again, the concrete gone, the bag, cage and safe
    # shut, the cave could not close (JT) and every treasure room paid its 2 points again (NU).
    # The rest now follows J() in the file, and RESUME rebuilds what the flags imply - the
    # travel-table changes and the changed descriptions - in a new routine at 27000.  A name
    # that cannot be opened (none, a mistyped one) stopped the program; now the question that
    # led to it is asked again, SUSPEND's or RESUME's
    (b'7840 SA$ = "C:\\ADV751\\GAMES\\" + SA$\r\n', b'7840 SA$ = "C:\\ADV751\\GAMES\\" + SA$: ON ERROR GOTO 7896\r\n'),
    (b"7850 OPEN SA$ FOR OUTPUT AS #1\r\n", b"7850 OPEN SA$ FOR OUTPUT AS #1: ON ERROR GOTO 0\r\n"),
    (b"7870 FOR X = 1 TO 132: PRINT #1, J(X): NEXT X\r\n",
     b"7870 FOR X = 1 TO 132: PRINT #1, J(X): NEXT X\r\n"
     b"7871 PRINT #1, GA, WET, CB, PS, CL, LT, BH, PH, LK, SSD, LGT, PTJ, CLO\r\n"
     b"7872 PRINT #1, WUMP, HLTH, JT, PV, NB, WE, CAB, DUM, HB, HEP, HEV, RML, BF, BG\r\n"
     b"7873 FOR X = 1 TO 12: PRINT #1, CI(X): NEXT X: FOR X = 1 TO 239: PRINT #1, NU(X): NEXT X\r\n"),
    (b"7890 SYSTEM\r\n", b"7890 SYSTEM\r\n7896 RESUME 7897\r\n7897 ON ERROR GOTO 0: GOTO 7700\r\n"),
    (b'7975 SA$ = "C:\\ADV751\\GAMES\\" + SA$\r\n', b'7975 SA$ = "C:\\ADV751\\GAMES\\" + SA$: ON ERROR GOTO 7998\r\n'),
    (b"7987 FOR X = 1 TO 132: INPUT #1, J(X): NEXT X\r\n7988 CLOSE #1\r\n7990 GOTO 80\r\n",
     b"7984 FOR X = 1 TO 132: INPUT #1, J(X): NEXT X\r\n"
     b"7985 INPUT #1, GA, WET, CB, PS, CL, LT, BH, PH, LK, SSD, LGT, PTJ, CLO\r\n"
     b"7986 INPUT #1, WUMP, HLTH, JT, PV, NB, WE, CAB, DUM, HB, HEP, HEV, RML, BF, BG\r\n"
     b"7987 FOR X = 1 TO 12: INPUT #1, CI(X): NEXT X: FOR X = 1 TO 239: INPUT #1, NU(X): NEXT X\r\n"
     b"7988 CLOSE #1: ON ERROR GOTO 0: GOSUB 27000\r\n"
     b"7990 GOTO 80\r\n"
     b"7998 RESUME 7999\r\n"
     b"7999 ON ERROR GOTO 0: CLOSE #1: GOTO 7900\r\n"),
    (b"26020 DATA 120,10,2,47,59,13,163\r\n",
     b"26020 DATA 120,10,2,47,59,13,163\r\n"
     b"27000 REM AFTER RESUME: WHAT THE SAVED FLAGS IMPLY\r\n"
     b"27010 IF WET = 1 THEN DI(139, 1) = 157\r\n"
     b"27020 IF BH = 1 THEN DI(27, 1) = 172\r\n"
     b"27030 IF SE = 1 THEN DI(9, 6) = 520\r\n"
     b"27040 IF J(52) = 300 OR J(53) = 300 THEN DI(60, 2) = 513\r\n"
     b'27050 IF J(52) = 0 THEN CLAM$ = "oyster"\r\n'
     b"27060 IF WB > 0 THEN I$(51) = TB$ + BB$(WB + 8)\r\n"
     b'27070 IF PS = 0 THEN M$ = "POSTER": NI = 71: GOSUB 8950\r\n'
     b'27080 IF PO > 0 THEN M$ = "SHARDS": NI = 12: GOSUB 8950\r\n'
     b'27090 IF DM = 0 THEN M$ = "OLDBTTRY": NI = 56: GOSUB 8950\r\n'
     b'27100 IF DD = 0 THEN M$ = "DRAGON": NI = 102: GOSUB 8950\r\n'
     b'27110 IF LT = 1 THEN M$ = "MASSIVE": NI = 103: GOSUB 8950\r\n'
     b'27120 IF PL > 1 THEN M$ = "PLANT" + RIGHT$(STR$(PL), 1): NI = 107: GOSUB 8950\r\n'
     b'27130 IF CLO = 1 THEN M$ = "CLOAK": NI = 79: GOSUB 8950\r\n'
     b'27140 IF BE = 3 THEN M$ = "CONTENTE": NI = 99: GOSUB 8950\r\n'
     b"27150 RETURN\r\n"),
]

# REFILL.BAS's slips, fixed unless --no-fixes (VAR has to be made again, make_var.py): three
# object names that inventories and FEED print kept the working number Ruby gave the object
# before he named it, as the treasures he never placed are still just "28" to "43"
REFILL_FIXES = [
    (b"25023 DATA 23ingot", b"25023 DATA ingot"),
    (b"25024 DATA 24rose", b"25024 DATA rose"),
    (b"25068 DATA 58coil of rope", b"25068 DATA coil of rope"),
]

# This project's additions, in with the fixes (after them; --no-fixes leaves them out too).
# Nothing in Ruby's game tames the bear: FEED BEAR FOOD only gets SANDWICH.TXT's refusal ("All
# you have are watercress sandwiches"), and no line sets BE = 2, the tame bear that the chain,
# GET BEAR (BE = 3, following), the troll and the bridge are written for.  In NEW ADVENTURE, a
# relative of the 751-point game (a PC-SIG walkthrough of 1990), the bear is fed a honeycomb from
# an apiary, which Ruby's cave does not have.  So a debug command,
# next to Ruby's own L (the lamp's counters) and DDD (the containers): #BEAR turns the fierce bear
# tame, with Ruby's message for that (FEDBEAR, FIX_ONCE); any other time nothing happens.
ADDITIONS = [
    (b'141 IF D$ = "DDD" THEN FOR X = 1 TO 12: PRINT CI(X); " "; : NEXT X: PRINT : GOTO 100\r\n',
     b'141 IF D$ = "DDD" THEN FOR X = 1 TO 12: PRINT CI(X); " "; : NEXT X: PRINT : GOTO 100\r\n'
     b'142 IF D$ = "#BEAR" THEN 27200\r\n'),
    (b"27150 RETURN\r\n",
     b"27150 RETURN\r\n"
     b"27200 REM DEBUG COMMAND #BEAR: THE BEAR TAMED, AS IF IT HAD EATEN\r\n"
     b"27210 IF BE <> 1 THEN 9030\r\n"
     b'27220 BE = 2: M$ = C1$ + "FEDBEAR": GOSUB 8900: GOTO 100\r\n'),
]

CRLF = b"\r\n"


def entries(name):
    """A dump -> [(lines, blank lines after the "//")].  A dump entry ends at its "//" line;
    the blank lines between one entry's "//" and the next entry are kept after the "//" of the
    first, where the program never reads.  (ROOM5's "//" came as "//q", which would make the
    program read on past the end of the file.)"""
    lines = open(os.path.join(SRC, name), "rb").read().split(CRLF)
    assert lines[-1] == b"", name
    out, cur = [], []
    for line in lines[:-1]:
        if line.startswith(b"//"):
            if line != b"//":
                assert (name, len(out), line) == ("rooms.txt", 4, b"//q"), (name, len(out), line)
            out.append([cur, []])
            cur = []
        elif not line.strip() and out and not cur:
            out[-1][1].append(line)
        else:
            cur.append(line)
    assert not cur, (name, cur)
    return out


def write(root, rel, lines, after=()):
    path = os.path.join(root, *rel.replace("\\", "/").split("/"))
    assert not os.path.exists(path), rel
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"".join(line + CRLF for line in list(lines) + list(after)))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    fixes = "--no-fixes" not in sys.argv[1:]
    out = os.path.abspath(args[0] if args else os.path.join(HERE, "ADV751"))
    var = None
    if os.path.exists(os.path.join(out, "VAR")):
        var = open(os.path.join(out, "VAR"), "rb").read()
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(os.path.join(out, "GAMES"))                        # SAVE writes GAMES\NAME
    if var is not None:
        open(os.path.join(out, "VAR"), "wb").write(var)

    for dump, names, folder in (("rooms.txt", ROOMS, ""), ("texts.txt", TEXTS, ""),
                                ("1timers.txt", ONCE, "1TIMERS/")):
        got = entries(dump)
        assert len(got) == len(names), (dump, len(got), len(names))
        for i, ((lines, after), name) in enumerate(zip(got, names)):
            if name is None and fixes:
                name = {"texts.txt": FIX_TEXTS, "1timers.txt": FIX_ONCE}.get(dump, {}).get(i)
            if name is None:
                continue
            if isinstance(name, tuple):                            # CASABLNC + CAVE
                assert lines[2] == b"" and lines[0] == b"MBT120O3", lines[:3]
                write(out, name[0] + ".TXT", lines[:3])
                write(out, name[1] + ".TXT", lines[3:] + [b"//"], after)
                continue
            rel = os.path.normpath(folder + name + ".TXT").replace(os.sep, "/")
            write(out, rel, lines + [b"//"], after)

    # BOOK.TXT, the book in the safe, as sent; ALT139.TXT is also read from the top folder
    shutil.copyfile(os.path.join(SRC, "history.txt"), os.path.join(out, "BOOK.TXT"))
    shutil.copyfile(os.path.join(out, "RMS", "ALT139.TXT"), os.path.join(out, "ALT139.TXT"))

    for rel, lines in STAND_INS.items():
        write(out, rel, [line.encode("ascii") for line in lines])

    for name in ("ADV751.BAS", "HELLO.BAS", "QUITS.BAS", "REFILL.BAS"):
        data = open(os.path.join(SRC, name), "rb").read()
        for file, old, new, _ in PATCHES:
            if file == name:
                assert data.count(old) == 1, (name, old)
                data = data.replace(old, new)
        for old, new in {"ADV751.BAS": FIXES + ADDITIONS, "REFILL.BAS": REFILL_FIXES}.get(name, []) if fixes else []:
            assert data.count(old) == 1, (name, old)
            data = data.replace(old, new)
        open(os.path.join(out, name), "wb").write(data)

    count = sum(len(files) for _, _, files in os.walk(out))
    print("%s: %d files, %s%s" % (out, count, "%d fixes, #BEAR" % (len(FIXES) + len(REFILL_FIXES))
                                  if fixes else "no fixes",
                                  "" if var is not None else " (no VAR yet: run make_var.py)"))


if __name__ == "__main__":
    main()
