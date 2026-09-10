/*
 * Adventure game -- new database
 *
 * Copyright (C) 1979 by  Mark D. Niemiec
 */

vocab {

	/*
	 * Necessary words
	 */
	"Copyright (C) 1979 by  Mark D. Niemiec"=
	Zero;
	Number;
	Typo;
	Intreq;
	"YES"="Y"="YEA"="YEAH"="OK"="OKAY"="O.K."="SURE"="OUI"="OUAIS";
	"NO"="NAY"="NON";

	FIRST_OBJ =

	/*
	 * Furniture definitions
	 */
	"ICE";
	"TREES"="ORCHARD";
	"WEED";
	"ROCK"="STONE";
	"GRATE";
	"CONCRETE";
	"ELEVATOR"="OTIS"="NUMBERS";
	"DOOR";
	"STAIR"="STAIRCASE"="STAIRWELL";
	"MACHINE"="VENDING"="CANTEEN"="GAMBLING";
	"TABLE";
	"CHANDELIER";
	"SKELETON"="CORPSE"="CADAVER";
	"RAFTER";
	"STATUE";
	"TOMB"="TOMBSTONE"="COFFIN"="SARCOPHAGUS";
	"BED";
	"HAY"="HAYSTACK"="STRAW";
	"ASH"="ASHES"="CINDER";
	"FIRE"="INFERNO"="BONFIRE"="FLAME";
	"TOWER";
	"WALL";
	"RUNE"="MESSAGE"="LETTER"="NOTE"="INSCRIPTION"="ENGRAVING"="WORD"="WRITING"=
		"SIGN";

	/*
	 * Miscellaneous objects outside
	 */
	"LIGHT"="ILLUMINATION";
	"WATER"="H20";
	"OIL";
	"LAMP"="FLASHLIGHT"="TORCH";
	"KEYS"="KEYCHAIN";
	"BOTTLE"="JUG"="JAR";

	/*
	 * Miscellaneous objects from new cave
	 */
	"HANKY"="HANDKERCHIEF"="KERCHIEF"="NAPKIN";
	"STAKE"="POST"="WOOD";
	"ROPE"="CORD"="TWINE"="HEMP";
	"EGG";
	"BROOM"="BROOMSTICK"=MOVE_OBJ;
	"MIRROR"="STEEL";
	"TREE"="MALLORN";

	/*
	 * Miscellaneous treasures from new cave
	 */
	"RHINESTONE"="RHINE";
	"SWORD"="EXCALIBUR";
	"KEY";
	"RUBY";
	"PAINTING"="ART"="MASTER"="MASTERPIECE"="PICTURE"="MURAL";
	"SLIPPER";
	"NEEDLE";
	"THREAD";
	"COIN"="CHANGE"="MONEY"="CASH";
	"LANTERN";
	"BOOK"="TEXT"="VOLUME";
	"APPLE";
	"NUT"="ACORN";
	"COIL";
	"NECKLACE";
	"DIAMOND";
	"CRYSTAL"="LENSES"="LENS";
	"TIARA";
	"BAR"="CANDY"="$61453";
	"BAG"="SACK"="TREASURE";
	"THRONE";

	/*
	 * Miscellaneous vehicles
	 */
	"SKATES";
	"RING";
	"GLASSES"="GOGGLE"="SPEC"="SPECTACLE"="SUNGLASSES";
	"RAFT"="BOAT"="DINGHY";

	"FROG"="TOAD";

	/*
	 * Miscellaneous creatures
	 */
	"ME"="ADVENTURER"="TRAVELLER"="MYSELF";
	"ORC";
	"GOLLUM"="SMEAGOL"="CREATURE"="SLIMY"="EVIL";
	"HARE";
	"ALCHEMIST";
	"PRINCESS";
	"TROLL";
	"WITCH";
	"MEDUSA"="MEDUSAE";
	"CLOCK"="GRANDFATHER";

	/*
	 * Misc. stuff
	 */
	ENDG_1; ENDG_2;

#text "vocab.f:"
#include "games/f/include/vocab"

	/*
	 * Magic words
	 */
	FIRST_VM=
	OLD_MAGIC="ABRA"="CADABRA"="ABRACADABRA"="SHAZAM"="HOCUS"="POCUS"=
		"XYZZY"="PLUGH"="FEE"="FIE"="FOE"="FOO"="FUM";
	"F00D"; "DURIN"; "FRIEND";
	LAST_VM;

	/*
	 * Miscellaneous words
	 */
	FIRST_STV=
	"ROAD"; "HILL"; "BUSHES"; "BUILD"="BUILDING"; "FOREST"; "VALLEY"="GULLY";
	"SLIT"; "DEPRESSION"; "ENTRANCE"; "DOWNSTREAM"; "UPSTREAM"; "STREAM";
	"BALLROOM"; "CLOSET"; "CRACK"; "CAVERN"; "EAGLE"; "GALLERY";
	"PIT"; "CANYON"; "PASSAGE"; "CRAWL"; "ACROSS"; "DIRT";
	"SURFACE"; "OUTDOORS"; "HOLE"; "STEP";
	"FOOD"; "BOOTH"; "SEARCH";
	"OVERBOARD"; "FOUNTAIN";
	LAST_STV;
	"WISH"="DESIRE"; "SWIM"="DIVE"; "DROWN"; "SMOKE"; "GONG"; "CIRCLE";

	/*
	 * Outside locations
	 */
	FIRST_LOC=
	START_LOC=LAMP_LOC=ROAD_BTM; ROAD_TOP;
	SCORE_LOC=RESTART_LOC=WELL_HOUSE;
	STREAM_VALLEY; FOREST_1; FOREST_2; SLIT_2IN; ABOVE_GRATE;
	ORCH_BUSH; ORCH_1; ORCH_2; ORCH_3; ORCH_4; ORCH_5; ORCH_6; ORCH_7;

	FIRST_CAVE=
	/*
	 * Locations in new cave
	 */
	BELOW_GRATE; GRATE_E; BALL_RM; BROOM_CLOSET=MOVE_OBJ_LOC; STAIR_CASE;
	UG_CRACK; SMALL_RM; UG_RIVER; UG_EW; LO_JCT; HI_JCT; TOMB_NS; TOMB_RM;
	CAVE_WW; CAVE_NW; CAVE_NN; CAVE_NE; CAVE_EE; CAVE_SE; CAVE_SS; CAVE_SW;
	CAVE_W; CAVE_N; CAVE_E; CAVE_S; CAVE_BTM; CAVE_LO; CAVE_HI; CAVE_TOP;
	CAVE_END; CUBBY_HOLE; EAGLE_NEST; SWORD_RM; ART_GALLERY; ELEV_RM;
	FOOD_JCT; FOOD_RM; ICE_PSG; WAITING_RM; WINDING_PSG;
	ICE_N; ICE_NE; ICE_E; ICE_SE; ICE_S; ICE_SW; ICE_W; ICE_NW;
	ICE_RINK; SKATE_RM; ICE_DEAD_END; FOUNT_RM; HAY_RM; FIRE_RM;
	STONE_DOOR; STATUE_RM; MEDUSA_RM; LAB; LAB2;
	DOOR_HOLE; DOOR_RM; KEY_RM; LAIR_0; LAIR_1; LAIR_2; LAIR_3; LAIR_4;
	LAIR_5; LAIR_6; LAIR_7; LAIR_8; LAIR_9;
	TREE_PSG; TREE_BTM; TREE_MID; TREE_TOP; BEDCHAMBER; BED_RM; TREE_W; BIG_PIT;
	ISLAND; SEA;

	LO_STEPS; HI_STEPS; PLATEAU_A;
	WEED_1A; WEED_2A; WEED_3A; WEED_4A; WEED_5A; WEED_6A; WEED_7A; WEED_8A;
	WEED_9A; WEED_10A; WEED_11A; WEED_12A; WEED_13A; TOWER_A; TOWERT_A;
	WEED_1B; WEED_2B; WEED_3B; WEED_4B; WEED_5B; WEED_6B; WEED_7B; WEED_8B;
	WEED_9B; WEED_10B; WEED_11B; WEED_12B; WEED_13B; TOWER_B; TOWERT_B;
	PLATEAU_B; TREE_FRONT;
	TREE_0; TREE_1; TREE_2; TREE_3; TREE_4; TREE_5; TREE_6; TREE_7; TREE_8;
	TREE_9; TREE_10; TREE_11;
	TREET_0; TREET_1; TREET_2; TREET_3; TREET_4; TREET_5; TREET_6; TREET_7;
	TREET_8; TREET_9; TREET_10; TREET_11;

	REPOS_NW; REPOS_SE; REPOS_NE; REPOS_SW; REPOS_ELEV=LAST_LOC;
}

WAIT_TIME = 45;		/* wait time for resuming saved game */

first_var;		/* must be first variable */
orc_clock, orc_time, orc_orc; orc_back;		/* orc clock */
gollum_clock, gollum_time, gollum_gollum; gollum_back;	/* gollum clock */
gong_clock, gong_time, gong_gong;	/* big ben clock */

/*
 * Variables
 */
.LAMP;		/* n<=0: off, n seconds of power left;	 n>0: on, dies at time n */
.BOTTLE;	/* 0=empty, 1=water */
.GRATE;		/* 0=locked, 1=closed, 2=open */
.FIRE;		/* 0=big, 1=medium, 2=small, 3=out */
.STAKE;		/* Is stake burning ? */
.ROPE;		/* where the rope has been thrown; zero if held */
.ELEVATOR;	/* what floor the elevator is at */
.SWORD;		/* Are you King Arthur ? */
.ACORN;		/* 0=acorn, 1=small-tree, 2=large-tree */
.CLOCK;		/* wound yet? */
.MACHINE;	/* broken yet? */
.STONE_DOOR;/* 0=wall, 1=open door */
.DOOR_RM;	/* which one of the 8 doors which is NOT fatal */
.X, .Y;		/* where in the sea you are (in dream) */
.WISH;		/* How many wishes have been used */

/*
 * Playing hours
 */
D_HRS_1 = 8*3600;	/* start of production hours -- weekdays */
D_HRS_2 = 24*3600;	/* end of " */
D_HRS_3 = 12*3600;	/* start of production hours -- weekends */
D_HRS_4 = 17*3600;	/* end of " */
SAVE_LATENCY = 45*60;

#text "main.f:"
#include "games/f/include/main"
/* #virtual goes here, if desired */
#text "adv.f:"


/*
 *    D A T A B A S E - D E P E N D A N T      R O U T I N E S
 */


/*
 * Operate rope
 */
rope(here, there, fn) {
	auto old;

	old = agent_clockp[3];
	if(.ROPE==here || .ROPE==there)
		switch(fn) {
		Enter1:
			.ROPE = there;
			furnish(ROPE);	/* ##### should be furnished during 'furnish' */
		D:
			putstr("You scurry down the rope.\n");
			there;
		U:
			putstr("You scurry up the rope.\n");
			there;
		CLIMB:
			putstr("The rope holds as you climb it.\n");
			there;
		JUMP:
			"I respectfully suggest that you use the rope.\n";
		}
	else
		switch(fn) {
		U:
			"It is much too high to reach without a rope.\n";
		D:
			"If you try to go down without a rope, you are liable to get killed.\n";
		CLIMB:
			"I don't suggest that you try climbing without a rope.\n";
		JUMP:
			if(here<FIRST_CAVE || here>there)
				jump(there);
			else
				"Jumping up and down won't get you very far.\n";
		}
}


/*
 * The silly fool tried to jump
 */
jump(there) {
	if(pct(95))
		die(there,"You fell and broke your neck.\n");
	else
		move(there);
}

/*
 * Operate elevator
 */
elev(n) {
	if(at(agent,FROG))
		return("Frogs aren't tall enough to reach the elevator controls.\n");
	if(.ELEVATOR == n)
		return(ELEV_RM);
	if(n < .ELEVATOR)
		-- .ELEVATOR;
	else if(n > .ELEVATOR)
		++ .ELEVATOR;
	"Sorry, the doors won't open, as the elevator isn't on this floor.\n";
	if(agent == ME) {
		flush();
		objexec(agent, where(agent), Here, ELEVATOR, 1);
	}
	return(0);
}


/*
 * Output number with leading zero, if necessary
 */
puthrs(n) {
	putchar(n/10 + '0');
	putchar(n%10 + '0');
}


/*
 * Initiate endgame proceedings
 */
endgame() {
	auto i, j;

	agent = ME;
	agent_clockp = &my_clock;
	io_select(ME);
	putstr("A booming voice says 'The cave is now closed.', and you are surrounded
		by a cloud of green smoke. When the smoke clears,\n");
	dropall(ME, where(ME));
	.GRATE = 0;
	.WISH = 0;
	endg_flag = 1;
	raw_score += 50;
	for(i=FIRST_OBJ; i<=LAST_OBJ; ++i)
		if(j = objexec(ME, 0, Endgame, i, 1))
			drop(i,j);
	dechron(&eg1_clock);
	dechron(&eg2_clock);
	dechron(&lt1_clock);
	dechron(&lt2_clock);
	.LAMP = -3600;
	.BOTTLE = 0;
	destroy(WATER);
	furnish(LIGHT);
	dream_loc = 0;
	move(-REPOS_NE);
}
/*
 * include file without general read for wizard function
 */
#include "games/f/newadv/wizard"


/*
 * Special functions performed dependant on clock interrupts
 */
wakeproc(event) {
	auto i, j, k;

	io_select(event>=ME ? event : ME);

	switch(event) {

	default:
		putnum(event);
		putstr(": Unknown wakeup type.\n");

	LIGHT:
		if(at(LAMP,where(ME)))
			"Your flashlight is getting low, you had better wrap this up soon.\n";

	LAMP:
		.LAMP = 0;
		if(at(LAMP,where(ME))) {
			putstr("Your flashlight has just run out of power.\n");
			destroy(LIGHT);
			look(ME);
		}

	CLOCK:
		i = where(ME);
		if(i == TOWERT_B)
			putstr("The clock emits an extremely loud");
		else if(i>=PLATEAU_A && i<=TREET_11)
			putstr("You hear, somewhere nearby,");
		else
			putstr("You hear, off in the distance,");
		putstr(" \"GONG");
		j = (vclock_time/3600-1) % 12;
		for(i=0; i<j; ++i)
			putstr(", GONG");
		putstr("\".\n");
		tick(3600);

	ENDG_1:
		endg_flag = 1;
		.GRATE = -1;
		for(i=ME+1; i<=LAST_OBJ; ++i)
			destroy(i);
		if(where(ME) >= FIRST_CAVE)
			"A sepulchral voice says:
			'Cave is closing soon, please leave via main exit.'\n";
		else
			endgame();

	ENDG_2:
		endgame();

	ME:
		if(find_score >= need_score) {
			raw_score += find_score+50;
			find_score = 0;
			enchron(&eg1_clock, vclock_time+20*60, ENDG_1);
			enchron(&eg2_clock, vclock_time+40*60, ENDG_2);
		}

		if(at(ME,0)) {
			tick(0);
			raw_score -= 5;
			putstr("\nOh my, you seem to have gotten yourself killed.\n");
			if(dream_loc) {
				putstr("Fortunately, when you come to your senses, you realize that it was a dream.\n");
				drop(ME, my_back = dream_loc);
				dream_loc = 0;
				return(suitable(ME, where(ME), LOOK, 0));
			}
			raw_score -= 10;
			if(endg_flag)
				quit("Well, seeing how it is closing time, we might as well call it quits.\n",0);
			while(yes("Do you want me to try and resurrect you? "));
			if(!true)
				quit("O.K.\n", 0);
			putstr("Very well. Here goes...
				Everything vanishes in a puff of orange smoke...\n");
			raw_score -= 10;
			drop(LAMP,LAMP_LOC);
			drop(ME, my_back = RESTART_LOC);
			if(.LAMP > 0)
				objexec(ME, where(ME), EXTINGUISH, LAMP, 1);
			look(ME);
		}

		do {
			do {
				putstr(". ");
			} while((uargc=getstr()) == 0);
		} while(parse_comm() <= 0);
		"OK\n";
		behalf(agent, agent_clockp, uargn[0], uargn[1], 10);

	GOLLUM:
		if(!at(ME,where(agent)) || at(ME,FROG) || wearing(RING,ME))
			if(container(RING) == where(agent))
				behalf(agent, agent_clockp, WEAR, RING, 10);
			else {
				behalf(agent, agent_clockp, MEANDER, 0, 30);
				"";
			}
		else if(wearing(RING,agent))
			behalf(agent,agent_clockp,KILL,ME,10);
		else {
			j = 0;
			for(i=FIRST_OBJ; i<=LAST_OBJ; ++i)
				if(carrying(i,ME) && objexec(GOLLUM,where(GOLLUM),Score,i,1)>0) {
					drop(i,LAIR_9);
					++j;
				}
			if(j > 0) {
				objloc[LAIR_9] = -1;
				putstr("Before you catch on, he steals your treasures and runs away!\n");
				move(LAIR_9);
			}
		}

	ORC:
		if(at(ME,where(ORC)) && !at(ME,FROG) && !wearing(RING,ME))
			behalf(ORC, &orc_clock, KILL, ME, 8);
		else {
			behalf(ORC, &orc_clock, MEANDER, 0, 30);
			"";
		}

	}
}


/*
 * Warn an agent of the approach of another agent
 */
warnexec(who, loc, other) {

	switch(who, other) {


	LAMP: {
		ME:
			if(.LAMP > 0)
				drop(LIGHT,LAMP);
		}

	BOTTLE: {
		ME:
			if(.BOTTLE == 1)
				drop(WATER,BOTTLE);
		}

	STAKE: {
		ME:
			if(.STAKE == 1)
				furnish(LIGHT);
		}

	ME: {
		ORC:
			enchron(&my_clock, orc_time-3, ME);
			putstr("A vicious looking orc just entered the room!\n");
			if(wearing(RING,ME))
				putstr("He doesn't appear to notice your presence.\n");
		GOLLUM:
			enchron(&my_clock, gollum_time-3, ME);
			if(wearing(RING,GOLLUM))
				return("You don't see a slimy creature creep up behind you.\n");
			putstr("A slimy creature just entered, muttering 'gollum, gollum'\n");
			if(wearing(RING,ME))
				putstr("He doesn't appear to notice your presence.\n");
		}

	ORC: {
		ME:
			enchron(&orc_clock, my_time+3, ORC);
		}

	GOLLUM: {
		ME:
			enchron(&gollum_clock, my_time+3, GOLLUM);
		}

	HARE: {
		ME:
			destroy(HARE);
			"A march hare runs across your path, looks at his watch, and cries
			'Oh dear, I am late for tea!', and disappears to the east.\n";
		}

	ALCHEMIST: {
		ME:
			if(near(RHINESTONE,ME)) {
				mutate(RHINESTONE,DIAMOND);
				putstr("There is an alchemist here, who, seeing your rhinestone,
				waves his hands, and in a puff of emerald-coloured smoke, it
				turns into a beautiful diamond. Rather pleased with himself, he
				shouts 'Eureka! It finally works! I will have to work some more
				on this formula!'. With this, he shoos you out of the room,
				so that he may continue in his work uninterrupted.\n");
				behalf(ME, &my_clock, S, 0, 10);
			} else if(near(GLASSES,ME)) {
				destroy(ALCHEMIST);
				mutate(GLASSES,LENSES);
				carry(LENSES,ME);
				create(MIRROR,LAB);
				putstr("There is an alchemist here, who, seeing your glasses, exclaims
				\"Aha! just what I need for my next experiment!\", and before you
				can stop him, takes them and gives you some finely-ground crystal
				lenses in return. He vanishes in a puff of white smoke.\n");
				putstr("There is a shiny steel mirror here.\n");
			} else {
				putstr("There is an alchemist here, who, seeing you enter the room,
				shouts \"Out! Out! You are disturbing my work!\", and shoos you
				out the door.\n");
				behalf(ME, &my_clock, S, 0, 10);
			}
		}


	}
}


/*
 * Determine which object(s) a word is referring to
 */
nounexec(loc, who, fn, obj, arg) {

	switch(obj, loc) {

	default: {
		default:
			return(loc == obj);
		}

	IT: {
		default:
			return(loc == it_obj);
		}

	HIM: {
		default:
			return(loc == him_obj);
		}

	}
}


/*
 *	Give initial instructions
 */
instr() {
	.LAMP *= 2;
	raw_score -= 5;

	putstr("Nearby is a colossal cave, where several people are said to have
		amassed great fortunes in treasures, although many have gone and never
		returned.  Magic is said to work in the cave, although you will have to
		discover how that works on your own. During your explorations, you may
		encounter other persons or creatures in the cave, so don't say you
		haven't been warned. As it stands now, parts of the cave are still
		under construction, so some objects may not yet be useful for the
		purposes they were designed for, although almost every item in the
		cave has some ultimate purpose for being there.
		The creatures will wander in MOST cases but if you are in The room with
		one he will attempt to carry out his function (gollum will try to rob you,
		the orc will try to kill you, etc...). However there are ways to prevent
		such crimes, but I am not going to tell you everthing!
		I will be your hands and eyes. I understand simple commands, such as
		\"turn on the lamp\", \"go north\", etc. I also understand a fair set
		of abbreviations and synonyms, for example \"n\" is the same as \"north\",
		\"inv\" is the same as \"inventory\", \"art\" is the same as \"painting\", etc.
		If a word seems too long to type, try another english word or abbreviation
		which would seem to fit, and chances are it will work.
		The main object of the game is to discover and collect treasures,
		and leave them at the building. See how many points you can achieve!\n");
		putstr("                press return to continue");
	getstr();
	putstr("\n\"Value\" will tell the market value of the object (only if you're holding it).
		\"Score\" tells you how much you have so far, \"quit\" quits the game.
		\"Save [filename]\" saves a copy of the cave in the named file so that
		the game can be resumed at a later date. (The file is roughly 3 llinks)\n");
}

#text "obj.f:"
#include "games/f/newadv/obj.f"

#text "loc.f:"
#include "games/f/newadv/loc.f"
#text "pass 2:"
