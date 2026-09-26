/*
 * Amiga MUD
 *
 * Copyright (c) 1991 by Chris Gray
 */

/*
 * rooms - define the rooms for the world.
 */

private r_porch CreateThing(nil)$
setupRoom(r_porch, "on the front porch",
    "This is the porch of your uncle's house. "
    "The house itself is quite charming, having white siding-covered walls "
    "and green trim and shutters. "
    "The porch extends across the full width of the house, and is "
    "enclosed by a white and green railing. "
    "A swinging loveseat occupies the west half of the porch, while the "
    "east is filled by a round white table and four matching wicker chairs. "
    "Flowerpots hanging from the support posts contain blooming flowers and "
    "trailing ferns. "
    "The front door is to the north.")$

scenery(r_porch,
    "house."
    "wall;white,siding-covered.wall;white,siding,covered."
    "trim,shutters;green."
    "porch."
    "railing;white,and,green."
    "loveseat;swinging.seat;love,swinging."
    "table;round,white."
    "chair;four,matching,wicker."
    "flowerpot;hanging.pot;hanging,flower."
    "post;support."
    "flower;blooming."
    "fern;trailing."
    "door;front")$

private r_hallSouth CreateThing(nil)$
setupRoom(r_hallSouth, "at the south end of the hall",
    "There is a coatrack on one wall with a boot mat under it. "
    "A muddy carpet runs the length of the hall. "
    "The front door is to the south, and the living room is to the west.")$
biconnect(r_porch, r_hallSouth, p_rNorth, p_rSouth)$
biconnect(r_porch, r_hallSouth, p_rEnter, p_rExit)$

private o_coat CreateThing(nil)$
setupObject(o_coat, r_hallSouth, "overcoat,coat;nondescript", "")$
fullObject(o_coat)$
private o_hat CreateThing(nil)$
setupObject(o_hat, r_hallSouth, "hat;well,worn", "")$
fullObject(o_hat)$
private o_umbrella CreateThing(nil)$
setupObject(o_umbrella, r_hallSouth, "umbrella,brolly;well,worn", "")$
fullObject(o_umbrella)$
private o_boots CreateThing(nil)$
setupObject(o_boots, r_hallSouth, "boots;dirty,rubber", "")$
fullObject(o_boots)$
scenery(r_hallSouth, "coatrack.rack;coat.mat;boot.bootmat")$
private o_carpet CreateThing(nil)$
setupObject(o_carpet, r_hallSouth, "carpet,rug;muddy", "")$

private r_hallTee CreateThing(nil)$
setupRoom(r_hallTee, "at a tee junction in the hall",
    "The main hall runs north-south, and a secondary hallway runs east.")$
biconnect(r_hallSouth, r_hallTee, p_rNorth, p_rSouth)$
AddTail(r_hallTee@p_rContents, o_carpet)$

private r_hallNorth CreateThing(nil)$
setupRoom(r_hallNorth, "at the north end of the hall",
    "The kitchen is to the west.")$
biconnect(r_hallTee, r_hallNorth, p_rNorth, p_rSouth)$
AddTail(r_hallNorth@p_rContents, o_carpet)$

private r_hallEast CreateThing(nil)$
setupRoom(r_hallEast, "in the secondary hallway",
    "The main hall is to the west, and doors open to the north, south "
    "and east.")$
biconnect(r_hallTee, r_hallEast, p_rEast, p_rWest)$

private r_livingRoom CreateThing(nil)$
setupRoom(r_livingRoom, "in the living room",
    "The living room is quite comfortable, but not terribly stylish. "
    "A worn chesterfield fills the south wall, while home-made shelving "
    "on the west wall contains a stereo system and television set. "
    "To the east is the hallway, while the kitchen and dining room are "
    "to the northeast and northwest.")$
biconnect(r_hallSouth, r_livingRoom, p_rWest, p_rEast)$

scenery(r_livingRoom, "chesterfield,couch;worn")$
sceneryDesc(r_livingRoom, "shelves,shelf,shelving;home,made,home-made",
    "The shelves are a bit rough, but are well made.\n")$
sceneryDesc(r_livingRoom, "system;stereo.stereo",
    "The stereo system is good quality, but not the best. "
    "It's a bit dusty.\n")$
sceneryDesc(r_livingRoom, "television,tv.set;television,tv",
    "The television is one of those all-black ones. There seem to be a "
    "lot of finger-prints on the screen.\n")$

private r_diningRoom CreateThing(nil)$
setupRoom(r_diningRoom, "in the dining room",
    "The dining room table and chairs are one of those horrible chrome, "
    "arborite and plastic combinations. "
    "The kitchen is to the east and the living room is to the southeast.")$
biconnect(r_livingRoom, r_diningRoom, p_rNorthWest, p_rSouthEast)$

scenery(r_diningRoom,
    "combination;horrible,arborite,and,plastic."
    "chair;dining,room,table,and.chair,table;dining,room")$

private r_kitchen CreateThing(nil)$
setupRoom(r_kitchen, "in the kitchen",
    "The standard kitchen equipment is here, along with a somewhat scratched "
    "double sink and lots of cupboards. "
    "The dining room is to the west, the living room is to the southwest and "
    "the hall is to the east.")$
biconnect(r_livingRoom, r_kitchen, p_rNorthEast, p_rSouthWest)$
biconnect(r_diningRoom, r_kitchen, p_rEast, p_rWest)$
biconnect(r_hallNorth, r_kitchen, p_rWest, p_rEast)$

scenery(r_kitchen,
    "equipment;standard,kitchen."
    "sink;somewhat,scratched,double,kitchen."
    "cupboard;lots,of,kitchen")$

private r_bathroom CreateThing(nil)$
setupRoom(r_bathroom, "in the bathroom",
    "The sink shows signs of wear, and is a bit rusty. The bathtub has "
    "one of those nasty rings. The usual bathroom cabinet with a mirror "
    "door is over the toilet.")$
biconnect(r_hallEast, r_bathroom, p_rNorth, p_rSouth)$
uniconnect(r_bathroom, r_hallEast, p_rExit)$

scenery(r_bathroom,
    "sink;rusty,bathroom."
    "bathtub.tub,bath."
    "ring;nasty")$
sceneryDesc(r_bathroom,
    "cabinet;usual,bathroom.mirror;usual,bathroom.door;mirror.",
    "There is nothing of interest inside the bathroom cabinet. "
    "The image in the mirror is somewhat unpleasant.\n")$
sceneryDesc(r_bathroom, "toilet",
    "It's a toilet - what more can be said?\n")$

private r_bedroom CreateThing(nil)$
setupRoom(r_bedroom, "in the bedroom",
    "A queen sized waterbed fills most of this room, but a nightstand and "
    "chest of drawers find their places along the walls.")$
biconnect(r_hallEast, r_bedroom, p_rEast, p_rWest)$
uniconnect(r_bedroom, r_hallEast, p_rExit)$

private o_diary CreateThing(nil)$
setupObject(o_diary, r_bedroom, "diary",
    "The diary records your Uncle's activities. The entries end about a month "
    "before he was declared missing.\n")$
fullObject(o_diary)$
o_diary@p_oText :=
    "...\n"
    "March 20: Tryed a small dose of the suspender today. Worked O.K., but "
    "I felt a bit nauseous.\n\n"
    "March 21: No work today - went to Janet Reble's party and listened to "
    "Shirley Thomson ramble on about parallel computers.\n\n"
    "March 22: Explored the north-east area again. Still nothing new.\n\n"
    "March 23: Tryed a few more random mixtures. Made a huge stench, but "
    "nothing significant happened.\n\n"
    "March 24: Running low on eyes - have to go to the pet store soon.\n"$

scenery(r_bedroom, "bed;queen,sized,water.waterbed,queen,sized,queen-sized")$
sceneryDesc(r_bedroom, "stand;night.nightstand",
    "You rummage in the nightstand, but find nothing of interest.\n")$
sceneryDesc(r_bedroom, "drawers;chest,of.chest",
    "Checking through all of the drawers, you find nothing useful.\n")$

private r_study CreateThing(nil)$
setupRoom(r_study, "in the study",
    "This study appears to be well used. A large desk occupies the west "
    "wall, while an overstuffed armchair and ottoman occupy the center of "
    "the room. The south wall has a low set of bookshelves, while the "
    "east wall has a high set of bookshelves.")$
biconnect(r_hallEast, r_study, p_rSouth, p_rNorth)$
uniconnect(r_study, r_hallEast, p_rExit)$
r_study@p_rEastCheck :=
    proc
	if Me()@p_pBookCombination = 2 then
	    continue
	else
	    Print("You can't go in that direction.\n");
	    fail
	fi
    corp$
sceneryDesc(r_study, "desk;large,wooden",
    "The desk is made of wood, and has several drawers, all of which "
    "prove to be locked. The junk on top doesn't look useful.\n")$
sceneryDesc(r_study, "armchair;overstuffed.chair;overstuffed,arm",
    "You search, but find no loose change in the armchair.\n")$
sceneryDesc(r_study, "ottoman;overstuffed",
    "The ottoman is a good match for the armchair.\n")$
sceneryDesc(r_study,
    "shelves,shelf,shelving,case,bookcase,bookshelves,bookshelf;"
	"low,set,of,south,wall,book",
    "The low bookshelves are filled with random technical references.\n")$
sceneryDesc(r_study,
    "shelves,shelf,shelving,case,bookcase,bookshelves,bookshelf;"
	"high,set,of,east,wall,book",
    "The high bookshelves are filled with a large encyclopedia.\n")$
private o_encyclopedia CreateThing(nil)$
setupObject(o_encyclopedia, r_study, "encyclopedia;large",
    "The encyclopedia is in 10 volumes, volume one to volume ten.\n")$
o_encyclopedia@p_oCarryable := true$
o_encyclopedia@p_oText :=
    "The encyclopedia must be read one volume at a time.\n"$
private proc encyclopediaGet(thing what)status:
    Print("The encyclopedia must be taken one volume at a time.\n");
    fail
corp;
o_encyclopedia@p_oGetAction := encyclopediaGet$
private o_reference CreateThing(nil)$
setupObject(o_reference, r_study, "reference;random,technical",
    "The references include 'Quantum Mechanics', 'Classical Mechanics', "
    "'Software Tools', 'Advanced Calculus' and 'Peasants'.\n");
o_reference@p_oCarryable := true$
o_reference@p_oText := "The references must be read by name.\n"$
private proc referenceGet(thing what)status:
    Print("The references must be taken by name.\n");
    fail
corp;
o_reference@p_oGetAction := referenceGet$
o_reference@p_oFixedText :=
    "You could read the references if you were holding them.\n"$
private o_book CreateThing(nil)$
setupObject(o_book, r_study, "book,volume",
    "You must specify a specific book.\n")$
o_book@p_oCarryable := true$
private proc genericBookGet(thing what)status:
    Print("You must specify a specific book to get.\n");
    fail
corp;
o_book@p_oGetAction := genericBookGet$
o_book@p_oFixedText := "You must specify a specific book to read.\n"$

private r_hiddenStudy CreateThing(nil)$
setupRoom(r_hiddenStudy, "in the hidden study",
    "This is the hidden study. It is filled with a lot of strange stuff, "
    "much of it quite old. The south wall is occupied by yet another set "
    "of bookshelves, but the volumes there are mostly bound in leather "
    "rather than the usual cardboard and glossy paper. The east wall has "
    "shelves holding labelled jars containing various unidentifiable things. "
    "In the center of the room, on a raised platform, is a bookstand "
    "holding a single large volume entitled 'Magical Formulae'. A large "
    "cauldron sits in the northwest corner. "
    "The secret doorway heads west, and a wooden staircase in the "
    "northeast corner leads downwards.")$
biconnect(r_study, r_hiddenStudy, p_rEast, p_rWest)$
sceneryDesc(r_hiddenStudy,
    "shelves,shelf,shelving,case,bookcase,bookshelves,bookshelf;"
	"set,of,south,wall,book",
    "Volumes here include: 'Necronomicon', 'Ars Magica', "
    "'The Book of the Dead' and 'Wisdom of WeatherWax'.\n")$
sceneryDesc(r_hiddenStudy, "bookstand;grotesque.stand;grotesque,book",
    "The bookstand appears to be carved from a single piece of a very dark "
    "wood. It abounds with gruesome figures. It's base is a three-clawed "
    "foot, and its top is a pair of taloned hands.\n")$
sceneryDesc(r_hiddenStudy, "figure;gruesome",
    "You really don't want a detailed description of the gruesome figures "
    "on the grotesque bookstand!\n")$
scenery(r_hiddenStudy,
    "platform;raised."
    "stuff;strange,old."
    "hand;pair,of,taloned."
    "foot,feet;three,clawed,three-clawed."
    "claw;three."
    "page;parchment."
    "lettering,text."
    "staircase,case;wooden,stair."
    "things;various,unidentifiable")$
AddTail(r_hiddenStudy@p_rContents, o_book)$
private o_cauldron CreateThing(nil)$
setupObject(o_cauldron, r_hiddenStudy,
    "cauldron;giant,black,iron,massive,large","")$
private proc cauldronDesc(thing what)status:
    Print("The cauldron is made of iron, and is quite massive. "
	    "It sits on three short legs over a gas burner. ");
    if what@p_oCurrentMix = "" then
	Print("Currently, the cauldron is empty.\n");
    else
	Print("A vile-looking mess is sitting in the cauldron, waiting "
		"to be brewed.\n");
    fi;
    continue
corp;
o_cauldron@p_oLookAction := cauldronDesc$
o_cauldron@p_oCauldron := true$
o_cauldron@p_oCurrentMix := ""$
private o_jar CreateThing(nil)$
setupObject(o_jar, r_hiddenStudy,
    "jar;labelled.shelves,shelf,shelving,case;east,wall,jar",
    "The jars are small, and labelled with computer-printed paper labels. "
    "Labels include: 'fillet of fenny snake', 'eye of newt', 'toe of frog', "
    "'wool of bat', 'tongue of dog', 'scale of dragon', 'tooth of wolf', "
    "'root of hemlock', 'gall of goat' and 'slips of yew'.\n")$
o_jar@p_oCarryable := true$
private proc genericJarGet(thing what)status:
    Print("You must specify a specific jar to get.\n");
    fail
corp;
o_jar@p_oGetAction := genericJarGet$
private o_magicBook CreateThing(nil)$
setupObject(o_magicBook, r_hiddenStudy,
    "Formulae;Magical.Magical.volume;single,large",
    "This huge volume lies open on the bookstand. The lettering on its "
    "parchment pages seems to writhe as you look at it. Your eye is drawn "
    "to the text within, but your mind has difficulty retaining what it "
    "reads.\n")$
o_magicBook@p_oCarryable := true$
private proc magicBookGet(thing book)status:
    Print("Magical Formulae resists being taken - some kind of force binds "
	    "it permanently to the grotesque bookstand.\n");
    fail
corp;
o_magicBook@p_oGetAction := magicBookGet$
o_magicBook@p_oFixedText :=
    "...\n"
    "light - eye of newt, scale of dragon, tongue of dog\n"
$

private r_stairsJoin CreateThing(nil)$
setupRoom(r_stairsJoin, "at the bottom of the wooden staircase",
    "You are in a short section of passageway. It is constructed of stone "
    "blocks about one foot square. The passageway ran to the north and south, "
    "but both ends appear to have collapsed long ago. A similarly constructed "
    "spiral staircase heads down into the gloom, and the wooden stairs up to "
    "the hidden study end by one wall.")$
scenery(r_stairsJoin,
    "block;stone.staircase,case,stair;spiral,wooden,stone,stair")$
biconnect(r_hiddenStudy, r_stairsJoin, p_rDown, p_rUp)$

private r_landing1 CreateThing(nil)$
darkRoom(r_landing1, "on a landing in the stone staircase", "")$
biconnect(r_stairsJoin, r_landing1, p_rDown, p_rUp)$

private r_landing2 CreateThing(nil)$
darkRoom(r_landing2, "on a landing in the stone staircase", "")$
biconnect(r_landing1, r_landing2, p_rDown, p_rUp)$

private r_landing3 CreateThing(nil)$
darkRoom(r_landing3, "on a landing in the stone staircase", "")$
biconnect(r_landing2, r_landing3, p_rDown, p_rUp)$

private r_landing4 CreateThing(nil)$
darkRoom(r_landing4, "on a landing in the stone staircase", "")$
biconnect(r_landing3, r_landing4, p_rDown, p_rUp)$

private r_stairsBottom CreateThing(nil)$
darkRoom(r_stairsBottom, "at the bottom of the spiral stairs",
    "The spiral staircase ends to one side here, and a stone-block passageway "
    "heads to the north and south.")$
scenery(r_stairsBottom,
    "staircase,stair,case;spiral,stone,stair."
    "passageway,way;stone-block,stone,block,passage")$
biconnect(r_landing4, r_stairsBottom, p_rDown, p_rUp)$

private r_tunnelNorth1 CreateThing(nil)$
darkRoom(r_tunnelNorth1, "in the main north-south passageway",
    "The bottom of the spiral staircase can be see to the south.")$
biconnect(r_stairsBottom, r_tunnelNorth1, p_rNorth, p_rSouth)$

private proc pitFall()status:
    Print("You have falled into a bottomless pit!!!!\n");
    Die();
    fail
corp;

private r_pitRoomS CreateThing(nil)$
darkRoom(r_pitRoomS, "at the south entrance to the pit room",
    "This is a large, circular chamber, half filled by a huge pit, which "
    "seems to be bottomless. You can go safely past the pit to the northeast "
    "and northwest, and south heads back into the main tunnel.")$
biconnect(r_tunnelNorth1, r_pitRoomS, p_rNorth, p_rSouth)$
uniconnect(r_pitRoomS, r_pitRoomS, p_rNorth)$
r_pitRoomS@p_rNorthCheck := pitFall$

private r_pitRoomE CreateThing(nil)$
darkRoom(r_pitRoomE, "on the east side of the pit room",
    "You can safely go northwest and southwest.")$
biconnect(r_pitRoomS, r_pitRoomE, p_rNorthEast, p_rSouthWest)$
uniconnect(r_pitRoomE, r_pitRoomE, p_rWest)$
r_pitRoomE@p_rWestCheck := pitFall$

private r_pitRoomW CreateThing(nil)$
darkRoom(r_pitRoomW, "on the west side of the pit room",
    "You can safely go northeast and southeast.")$
biconnect(r_pitRoomS, r_pitRoomW, p_rNorthWest, p_rSouthEast)$
uniconnect(r_pitRoomW, r_pitRoomW, p_rEast)$
r_pitRoomW@p_rEastCheck := pitFall$

private r_pitRoomN CreateThing(nil)$
darkRoom(r_pitRoomN, "at the north entrance to the pit room",
    "You can safely go past the pit to the southeast and southwest, and "
    "north heads back into the main tunnel.")$
biconnect(r_pitRoomE, r_pitRoomN, p_rNorthWest, p_rSouthEast)$
biconnect(r_pitRoomW, r_pitRoomN, p_rNorthEast, p_rSouthWest)$
uniconnect(r_pitRoomN, r_pitRoomN, p_rSouth)$
r_pitRoomN@p_rSouthCheck := pitFall$

private r_tunnelNorth2 CreateThing(nil)$
darkRoom(r_tunnelNorth2, "in the main north-south passageway",
    "Smaller passageways head to the east and west.")$
biconnect(r_pitRoomN, r_tunnelNorth2, p_rNorth, p_rSouth)$

private r_smallEast1 CreateThing(nil)$
darkRoom(r_smallEast1, "in a small east-west passageway",
    "Doorways open in the north and south walls.")$
biconnect(r_tunnelNorth2, r_smallEast1, p_rEast, p_rWest)$

private r_smallEast2 CreateThing(nil)$
darkRoom(r_smallEast2, "at the end of a small east-west passageway",
    "Doorways open in the north and south walls.")$
biconnect(r_smallEast1, r_smallEast2, p_rEast, p_rWest)$

private r_smallWest1 CreateThing(nil)$
darkRoom(r_smallWest1, "in a small east-west passageway",
    "Doorways open in the north and south walls.")$
biconnect(r_tunnelNorth2, r_smallWest1, p_rWest, p_rEast)$

private r_smallWest2 CreateThing(nil)$
darkRoom(r_smallWest2, "at the end of a small east-west passageway",
    "Doorways open in the north and south walls.")$
biconnect(r_smallWest1, r_smallWest2, p_rWest, p_rEast)$

private proc createCell(thing hall; property thing dir)void:
    thing cell;
    property thing otherDir;

    cell := CreateThing(nil);
    otherDir :=
	if dir = p_rNorth then
	    p_rSouth
	else
	    p_rNorth
	fi;
    darkRoom(cell, "in a small, featureless room", "");
    biconnect(hall, cell, dir, otherDir);
    uniconnect(cell, hall, p_rExit);
corp;
createCell(r_smallEast1, p_rNorth)$
createCell(r_smallEast1, p_rSouth)$
createCell(r_smallEast2, p_rNorth)$
createCell(r_smallEast2, p_rSouth)$
createCell(r_smallWest1, p_rNorth)$
createCell(r_smallWest1, p_rSouth)$
createCell(r_smallWest2, p_rNorth)$
createCell(r_smallWest2, p_rSouth)$

private r_mainTee CreateThing(nil)$
darkRoom(r_mainTee, "at a T in the main passageways",
    "Branches head south, east and west.")$
biconnect(r_tunnelNorth2, r_mainTee, p_rNorth, p_rSouth)$

private r_mainEast1 CreateThing(nil)$
darkRoom(r_mainEast1, "in the main east-west passageway", "")$
biconnect(r_mainTee, r_mainEast1, p_rEast, p_rWest)$

private r_mainEast2 CreateThing(nil)$
darkRoom(r_mainEast2, "at the east end of the main passageway", "")$
biconnect(r_mainEast1, r_mainEast2, p_rEast, p_rWest)$

private r_mainWest1 CreateThing(nil)$
darkRoom(r_mainWest1, "in the main east-west passageway", "")$
biconnect(r_mainTee, r_mainWest1, p_rWest, p_rEast)$

private r_mainWest2 CreateThing(nil)$
darkRoom(r_mainWest2, "at the west end of the main passageway", "")$
biconnect(r_mainWest1, r_mainWest2, p_rWest, p_rEast)$

private r_tunnelSouth1 CreateThing(nil)$
darkRoom(r_tunnelSouth1, "in the main north-south passageway",
    "The bottom of the spiral staircase can be see to the north.")$
biconnect(r_stairsBottom, r_tunnelSouth1, p_rSouth, p_rNorth)$

private r_tunnelSouth2 CreateThing(nil)$
darkRoom(r_tunnelSouth2, "at a cave in",
    "The main passageway comes to an end here in a huge pile of broken stone "
    "which completely blocks off the passageway.")$
scenery(r_tunnelSouth2, "stone;huge,pile,of,broken.pile,huge")$
biconnect(r_tunnelSouth1, r_tunnelSouth2, p_rSouth, p_rNorth)$

private o_note CreateThing(nil)$
setupObject(o_note, nil, "note",
    "The note is a bit crumpled, probably from you stuffing it into your "
    "pocket, and from being read so many times.\n")$
fullObject(o_note)$
o_note@p_oText :=
    "If you receive this note, then I have been declared dead. That is bad. "
    "I may be still alive, in which case I will need restoring. Go to my "
    "house and start there. You'll find most of my books in the study. "
    "I'm sorry if you find this note a bit baffling - there are reasons.\n\n"
    "\t\t\t\t5,9,2\n"
    "Cedric Hawgrave\n\n"$

/*
 * doParse - need one with status result. Sigh.
 */

private proc doParse(string input)void:

    ignore Parse(G, input);
corp;

/*
 * newPlayer - this is the routine which we set up to be called when a
 *	new player is created.
 */

private proc newPlayer()void:
    list thing lt;
    thing me;
    action a;

    me := Me();
    lt := CreateThingList();
    AddTail(lt, o_note);
    me@p_pCarrying := lt;
    me@p_pBookCount := 0;
    me@p_pBookCombination := 0;
    me@p_pSpells := CreateThingList();
    me@p_pLight := false;
    a := SetCharacterInputAction(doParse);
    SetLocation(r_porch);
    Print("\n    Welcome, adventurer! "
	"Your eccentric uncle Cedric has disappeared and been declared dead. "
	"His will left his house and its contents to you, along with a "
	"cryptic note which you carry with you. Having arrived at his house "
	"you are now ready to try to carry out the note's instructions.\n\n"
	"You can use commands like: inventory, read, get, drop, look, "
	"enter, exit, north, n, down, d, etc. Good luck!\n\n");
corp;

ignore SetNewCharacterAction(newPlayer)$
