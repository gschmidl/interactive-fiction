/*
 * Copyright (C) 1979  by  Mark D. Niemiec
 *
 * Constraints on objects being worn
 */
wearexec(who, loc, fn, obj, arg) {
	auto i, j;

	switch(loc, fn) {

	default: {
		default:
			return(arg);
		}

	SKATES: {
		U:
		D:
		CLIMB:
			"Skates should only be used on level surfaces.\n";
		default:
			loc = where(who);
			if((i=getloc(loc,fn)) == 0)
				arg;
			else if(i==ICE_RINK || i>=ICE_N && i<=ICE_NW)
				i;
			else
				"I don't suggest skating on dry land.\n";
		}

	RING: {
		Look:
			"You are invisible.\n";
		}

	GLASSES: {
		READ:
			"The light in here is too dim to be able to read while wearing sunglasses.\n";
		}

	}
}


/*
 * Constraints on vehicles
 */
vehexec(who, loc, fn, obj, arg) {
	auto i, j;

	switch(loc, fn) {

	default: {
		default:
			return(arg);
		}

	RAFT: {
		Look:
			"You are in a small rubber dinghy.\n";
		N::NW:
			.X += which(fn-N, 0,1,1,1,0,-1,-1,-1);
			.Y += which(fn-N, -1,-1,0,1,1,1,0,-1);
			if(.X*256+.Y == 0)
				-ISLAND;
			else
				-SEA;
		}

	FROG: {
		Look:
			"You are a frog.\n";
		KISS:
			arg;
		default:
			loc = where(who);
			if((i=getloc(loc,fn)) == 0)
				"Sorry, but frogs aren't capable of doing that.\n";
			else if((j=backloc(loc,i))==U || j==CLIMB)
				"Sorry, but frogs aren't capable of climbing anything.\n";
			else
				i;
		SAY:
			"O.K. 'Ribbit'.\n";
		EAT:
			"Frogs only eat flies, and there are no flies in this cave.\n";
		INV:
			"Frogs never carry anything.\n";
		JUMP:
			"Please specify movement by only giving a direction.\n";
		SWIM:
			"Just because a witch turned you into a frog, doesn't mean you now know how to swim all of a sudden!\n";
		}

	}
}


/*
 * Location constraints and commands
 */
locexec(who, loc, fn, obj, arg) {
	auto i, j, k, x, y;

	switch(loc, fn) {

	default: {
		default:
			return(arg);
		Furnish:
		Locate:
			return(0);
		Brief:
			locexec(who, loc, Look, obj, arg);
		Look:
			if(loc < FIRST_LOC)
				return(0);
			keep_stats(-loc);
			putstr("A booming voice says 'This part of the cave is under construction.
			A tall figure in long flowing robes and a hard hat waves his hands, and
			in a puff of smoke ...\n");
			move(RESTART_LOC);
		Go:
			dechron(agent_clockp);
			travel();
		SWIM:
			"Don't be ridiculous.\n";
		OLD_MAGIC:
			"Old magic words like that just aren't in vogue around here!\n";
		DURIN:
			if(!at(who,where(ORC)))
				"Leave the dead in peace, why don't you!\n";
			else {
				putstr("You are enveloped by a bright shimmering cloud.
				When your eyes adjust to the light,\n");
				move(TOMB_RM);
			}
		WISH:
			if(.WISH == 0)
				"Wishing won't do you much good here.\n";
			else if(obj == 0)
				0;
			else if(at(obj,loc))
				"Why bother wishing? What you want is close at hand.\n";
			else if(.WISH++ > 3)
				"Sorry, but you have already had your maximum of three wishes.\n";
			else if(dream_loc)
				"Wishes are what dreams are made of....\n";
			else if((i=where(obj)) && objloc[i]<0) {
				putstr("In a puff of green smoke...\n");
				move(-i);
			} else {
				--.WISH;
				"I don't know where that is. Please try again.\n";
			}
		}


	/*
	 * Outside locations
	 */

	ROAD_BTM: {
		Furnish:
			furnish(LIGHT,WATER);
		Look:
			"You are standing at the end of a road before a small brick building,
			Around you is a forest. A small stream flows out of the building and
			down a gully. There are some bushes to the north.\n";
		Brief:
			"You're at end of road again.\n";
		Go:
			travel(W,U,ROAD,HILL, ROAD_TOP,
				E,BUILD,IN, WELL_HOUSE,
				N,BUSHES, ORCH_BUSH,
				S,VALLEY,DOWNSTREAM,-DEPRESSION,-GRATE, STREAM_VALLEY,
				NW,SE,NE,SW,FOREST, -FOREST_1);
		}

	ROAD_TOP: {
		Furnish:
			furnish(LIGHT);
		Look:
			"You have walked up a hill, still in the forest. The road slopes back
			down the other side of the hill. There is a building in the distance.\n";
		Brief:
			"You're at hill in road.\n";
		Go:
			travel(E,N,D,BUILD,HILL,ROAD,ROAD_BTM, S,FOREST,-FOREST_1);
		}

	WELL_HOUSE: {
		Furnish:
			if(at(FROG,loc))
				destroy(FROG);
			furnish(BUILDING,LIGHT,WATER);
		Look:
			"You are inside a building, a well house for a large spring.\n";
		Brief:
			"You are in building.\n";
		Go:
			travel(W,OUT,OUTDOORS, ROAD_BTM);
		DURIN:
			if(objloc[TOMB_RM] < 0)
				TOMB_RM;
			else
				"There is a loud thunderclap, and a puff of bright red smoke,
				otherwise nothing happens.\n";
		F00D:
			if(objloc[FOOD_RM] < 0)
				FOOD_RM;
			else
				"There is a vulgar belching noise, and puff of brown smoke, but
				otherwise nothing happens.\n";
		DOWNSTREAM:
			"The stream flows out through a pair of 2 foot diameter sewer pipes.
			I suggest that you use the exit.\n";
		}

	STREAM_VALLEY: {
		Furnish:
			furnish(LIGHT,WATER,STREAM);
		Look:
			"You are in a valley in the forest beside a stream tumbling along a
			rocky bed.\n";
		Brief:
			"You're in valley.\n";
		Go:
			travel(N,BUILD,UPSTREAM, ROAD_BTM,
				E,W,U,FOREST, -FOREST_1,
				DOWNSTREAM,D,W,S,SLIT,-DEPRESSION,-GRATE, SLIT_2IN);
		}

	FOREST_1: {
		Furnish:
			furnish(LIGHT,FOREST,TREES);
		Look:
			"You are in open forest, with a deep valley to one side.\n";
		Brief:
			"You're in forest.\n";
		Go:
			travel(D,E,VALLEY, STREAM_VALLEY,
				W,S, -FOREST_1,
				FOREST,N, pct(50)?-FOREST_1:-FOREST_2);
		}

	FOREST_2: {
		Furnish:
			furnish(LIGHT,TREES,FOREST);
		Look:
			"You are in open forest, near both a valley and a road.\n";
		Brief:
			"You're in forest.\n";
		Go:
			travel(N,ROAD, ROAD_BTM,
				D,E,W,VALLEY, STREAM_VALLEY,
				FOREST,S, -FOREST_1);
		}

	SLIT_2IN: {
		Furnish:
			furnish(LIGHT,WATER,SLIT,ROCK);
		Look:
			"At your feet all the water of the stream splashes into a 2-inch slit
			in the rock. Downstream the streambed is bare rock.\n";
		Brief:
			"You're at slits.\n";
		Go:
			travel(N,UPSTREAM,VALLEY,-BUILD,-ROAD, STREAM_VALLEY,
				E,W,FOREST, -FOREST_1,
				S,DEPRESSION,DOWNSTREAM,GRATE,ROCK,BED, ABOVE_GRATE);
		D:
		SLIT:
			"You don't fit through a 2 inch slit!!\n";
		}

	ABOVE_GRATE: {
		Furnish:
			furnish(LIGHT,GRATE,CONCRETE,DIRT);
		Look:
			"You are in a 20-foot depression floored with bare dirt. Set into the
			dirt is a strong steel grate mounted in concrete. A dry streambed
			leads into the depression\n";
		Brief:
			"You're outside grate.\n";
		Go:
			travel(E,W,S,FOREST, -FOREST_1,
				N,VALLEY,-BUILD,-ROAD,-HILL, SLIT_2IN,
				D,IN,GRATE, BELOW_GRATE);
		DOWNSTREAM:
			"The streambed ends here.\n";
		D:
		IN:
		GRATE:
			switch(.GRATE) {
			default:
				"You can't go through a locked steel grate!\n";
			1:
				"You can't go through a closed steel grate!\n";
			2:
				BELOW_GRATE;
			}
		}

	ORCH_BUSH: {
		Furnish:
			furnish(LIGHT,BUSHES);
		Look:
			"You are in a tangle of bushes, with an orchard to the east.
			There is a building by a road to the south.\n";
		Go:
			travel(S,BUILD,ROAD,ROAD_BTM, E,ORCHARD,-ORCH_7);
		}

	ORCH_1: {
		Furnish:
			furnish(LIGHT,TREES,DOOR,ELEVATOR);
		Look:
			"You are in an orchard. There is a small booth here with numbers above it.\n";
		Go:
			travel(N,-ORCH_6, S,-ORCH_2, E,-ORCH_4, W,-ORCH_6, IN,ELEV_RM);
		IN:
			elev(0);
		}

	ORCH_2: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in an orchard.\n";
		Go:
			travel(N,-ORCH_1, S,-ORCH_4, E,-ORCH_3, W,-ORCH_5);
		}

	ORCH_3: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in an orchard.\n";
		Go:
			travel(N,-ORCH_6, S,-ORCH_4, E,-ORCH_2, W,-ORCH_7);
		}

	ORCH_4: {
		Furnish:
			furnish(LIGHT,TREES);
		Enter1:
		JUMP:
		D:
		CLIMB:
			rope(ORCH_4, TREE_BTM+.ACORN, fn);
		Look:
			"You are in an orchard. There is a small hole in the ground here.\n";
		Go:
			travel(N,-ORCH_5, S,-ORCH_1, E,-ORCH_3, W,-ORCH_2, D,CLIMB,TREE_BTM+.ACORN);
		}

	ORCH_5: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in an orchard.\n";
		Go:
			travel(N,-ORCH_7, S,-ORCH_2, E,-ORCH_4, W,-ORCH_6);
		}

	ORCH_6: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in an orchard.\n";
		Go:
			travel(N,-ORCH_7, S,-ORCH_1, E,-ORCH_3, W,-ORCH_5);
		}

	ORCH_7: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in an orchard. There are some bushes in the distance.\n";
		Go:
			travel(N,BUSHES,ORCH_BUSH, S,-ORCH_3, E,-ORCH_6, W,-ORCH_5);
		}



	BELOW_GRATE: {
		Furnish:
			furnish(LIGHT,GRATE);
		Locate:
			10;
		Look:
			"You are in a small chamber beneath a 3x3 steel grate to the surface.
			A small passage leads downwards to the east.\n";
		Brief:
			"You're below the grate.\n";
		Go:
			travel(U,DEPRESSION,OUT,ABOVE_GRATE, E,D,GRATE_E);
		U:
		OUT:
		DEPRESSION:
			switch(.GRATE) {
			default:
				"You can't go through a locked steel grate!\n";
			1:
				"You can't go through a closed steel grate!\n";
			2:
				ABOVE_GRATE;
			}
		}

	GRATE_E: {
		Look:
			"You are in a passage sloping downwards gently to the east.\n";
		Go:
			travel(W,U,-DEPRESSION,-GRATE,BELOW_GRATE, E,D,BALLROOM,BALL_RM);
		}

	BALL_RM: {
		Furnish:
			furnish(LIGHT,CHANDELIER,STAIR);
		Look:
			"You are in a magnificent ballroom carved out of solid rock.
			There is a large crystal chandelier sparkling brilliantly from the ceiling.
			Passages lead off in all directions, the south one leading down a staircase.\n";
		Brief:
			"You're in the ballroom.\n";
		Go:
			travel(N,CLOSET,BROOM_CLOSET, E,CAVERN,CAVE_WW,
				S,D,STAIR,STAIR_CASE, W,-DEPRESSION,-GRATE,GRATE_E);
		}

	BROOM_CLOSET: {
		Look:
			"You are in a small broom closet. The only way out is south.\n";
		Go:
			travel(S,OUT,BALLROOM, BALL_RM);
		}

	STAIR_CASE: {
		Furnish:
			furnish(STAIR,SKELETON);
		Look:
			"You are at the bottom of a long flight of stairs which lead to the north.
			The passage continues downwards to the south.\n";
		Brief:
			"You're at bottom of stairs.\n";
		Go:
			travel(U,N,BALL_RM,BALLROOM, S,D,CRACK,UG_CRACK);
		}

	UG_CRACK: {
		Furnish:
			furnish(CRACK);
		Look:
			"You are in a north-south passage, with large cracks in both walls.\n";
		Go:
			travel(N,U,STAIR,STAIR_CASE, S,D,UG_RIVER, E,CRACK,UG_EW, W,SMALL_RM);
		W:
			if(at(FROG,loc))
				SMALL_RM;
			else
				"The cracks in the walls are too small for a human to fit through.\n";
		}

	SMALL_RM: {
		Furnish:
			furnish(CRACK);
		Look:
			"You are in a small room, with some tiny cracks in the east wall.\n";
		Go:
			travel(E,OUT, UG_CRACK);
		E:
		OUT:
			if(at(FROG,loc))
				UG_CRACK;
			else
				"The cracks in the wall are too small for a human to fit through.\n";
		}

	UG_RIVER: {
		Furnish:
			furnish(WATER);
		Look:
			"The passage ends, as it runs into an underground river.
			The only way out is back north.\n";
		Brief:
			"You're at an underground river.\n";
		Go:
			travel(N,U,OUT, UG_CRACK);
		D:
		S:
		SWIM:
			"I don't suggest swimming in ice-cold water.\n";
		}

	UG_EW: {
		Look:
			"You are in a low east-west passage\n";
		Go:
			travel(E,LO_JCT, W,CRACK,UG_CRACK);
		}

	LO_JCT: {
		Look:
			"You are in an east-west passage beneath a north-south passage.\n";
		Go:
			travel(U,HI_JCT, E,ICE_W, W,UG_EW);
		}

	HI_JCT: {
		Look:
			"You are in a north-south passage above an east-west passage.\n";
		Go:
			travel(D,LO_JCT, N,CAVERN,CAVE_SS, S,TOMB_NS);
		}

	TOMB_NS: {
		Look:
			"You are in a passage sloping gently downwards to the the south.\n";
		Go:
			travel(N,U,CAVERN,HI_JCT, S,D,TOMB_RM);
		}

	TOMB_RM: {
		Furnish:
			furnish(RUNE,TOMB);
		Look:
			"You are in a large chamber hewn out of the living rock.
			There is a large tomb here, with runes engraved into the side.
			The only exit is to the north.\n";
		Brief:
			"You're at Durin's tomb.\n";
		Go:
			travel(N,U,OUT, TOMB_NS);
		DURIN:
			if(at(ORC,loc)) {
				 destroy(ORC);
				"The orc lets out an agonized screech, as he is enveloped by a bright
				 shimmering cloud. When your eyes adjust to the light, he is gone.\n";
			} else
				WELL_HOUSE;
		Inread:
			"The runes on the tombstone say: Here lies Durin, king of Moria.'
			It is said that Durin, even in death, will protect all sworn enemies of orcs.\n";
		}

	CAVE_WW: {
		Look:
			"You are above the west side of a large cavern. A path leads
			down into the cavern, while another path circles the cavern.
			There is an exit to the west.\n";
		Brief:
			"You're on path above west side of a large cavern.\n";
		Enter2:
			if(at(who,TROLL)) {
				destroy(WITCH);
				"Seeing the troll, the witch lets out a frightened screech, and runs
				away, leaving her broom behind.\n";
			}
		U:
			"The cavern wall is too steep to climb here.\n";
		Go:
