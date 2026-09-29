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
			travel(E,D,CAVE_W, N,CAVE_NW, S,CAVE_SW, W,OUT,BALLROOM, BALL_RM);
		}

	CAVE_NW: {
		Look:
			"You are on a path above the northwest side of a large cavern.\n";
		Go:
			travel(NE,N,E,CAVE_NN, SW,S,W,CAVE_WW);
		}

	CAVE_NN: {
		Look:
			"You are above the north side of a large cavern. A path leads
			down into the cavern, while another path circles the cavern.
			There is an exit to the north.\n";
		Brief:
			"You're on path above north side of a large cavern.\n";
		U:
			"The cavern wall is too steep to climb here.\n";
		Go:
			travel(S,D,CAVE_N, N,OUT,LO_STEPS, E,CAVE_NE, W,CAVE_NW);
		}

	CAVE_NE:{
		Look:
			"You are on a path above the northeast side of a large cavern.\n";
		Go:
			travel(NW,N,W,CAVE_NN, SE,S,E,CAVE_EE);
		}

	CAVE_EE: {
		Look:
			"You are above the east side of a large cavern. A path leads
			down into the cavern, while another path circles the cavern.
			There is an exit to the east.\n";
		Brief:
			"You're on path above east side of a large cavern.\n";
		U:
			"The cavern wall is too steep to climb here.\n";
		Go:
			travel(W,D,CAVE_E, N,CAVE_NE, S,CAVE_SE, E,OUT,FOOD_JCT);
		}

	CAVE_SE: {
		Look:
			"You are on a path above the southeast side of a large cavern.\n";
		Go:
			travel(NE,N,E,CAVE_EE, SW,S,W,CAVE_SS);
		}

	CAVE_SS: {
		Look:
			"You are above the south side of a large cavern. A path leads
			down into the cavern, while another path circles the cavern.
			There is an exit to the south.\n";
		Brief:
			"You're on path above south side of a large cavern.\n";
		U:
			"The cavern wall is too steep to climb here.\n";
		Go:
			travel(N,D,CAVE_S, W,CAVE_SW, E,CAVE_SE, S,OUT,HI_JCT);
		}

	CAVE_SW: {
		Look:
			"You are on a path above the southwest side of a large cavern.\n";
		Go:
			travel(NW,N,W,CAVE_WW, SE,S,E,CAVE_SS);
		}

	CAVE_W: {
		Look:
			"You are on the west side of a large cavern. The path slopes
			up and down the side of the cavern, while another path circles the
			cavern. A path leads through a hole in the west wall.\n";
		Brief:
			"You're on west side of cavern.\n";
		Go:
			travel(U,CAVE_WW, E,D,CAVE_BTM, N,CAVE_N, S,CAVE_S, W,IN,HOLE,CAVE_END);
		}

	CAVE_N: {
		Look:
			"You are on the north side of a large cavern. The path slopes
			up and down the side of the cavern, while another path circles the
			cavern. A path leads through a hole in the north wall.\n";
		Brief:
			"You're on north side of cavern.\n";
		Go:
			travel(U,CAVE_NN, S,D,CAVE_BTM, W,CAVE_W, E,CAVE_E, N,IN,HOLE,CUBBY_HOLE);
		}

	CAVE_E: {
		Look:
			"You are on the east side of a large cavern. The path slopes
			up and down the side of the cavern, while another path circles the
			cavern. A path leads through a hole in the east wall.\n";
		Brief:
			"You're on east side of cavern.\n";
		Go:
			travel(U,CAVE_EE, W,D,CAVE_BTM, N,CAVE_N, S,CAVE_S, E,IN,WAITING_RM);
		}

	CAVE_S: {
		Look:
			"You are on the south side of a large cavern. The path slopes
			up and down the side of the cavern, while another path circles the
			cavern. A cold draft is blowing through a hole in the south wall.\n";
		Brief:
			"You're on south side of cavern.\n";
		Go:
			travel(U,CAVE_SS, N,D,CAVE_BTM, E,CAVE_E, W,CAVE_W, S,WINDING_PSG);
		}

	CAVE_BTM: {
		Furnish:
			furnish(WATER);
		Look:
			"You are in the middle of a huge cavern, where there is a large
			stalagmite growing up as far as you can see. There is a trickle of water
			running down the stalagmite.\n";
		Brief:
			"You're in middle of cavern.\n";
		Go:
			travel(U,CLIMB,CAVE_LO, N,CAVE_N, E,CAVE_E, S,CAVE_S, W,CAVE_W);
		}

	CAVE_LO: {
		Furnish:
			furnish(WATER);
		Enter1:
		U:
		CLIMB:
		JUMP:
			rope(CAVE_LO, CAVE_HI, fn);
		Look:
			"You are at the top of the stalagmite. You can see a stalactite
			hanging from the ceiling above you, but it is too high to reach.\n";
		Brief:
			"You're at top of stalagmite.\n";
		Go:
			travel(D,CAVE_BTM, U,CLIMB,CAVE_HI);
		}

	CAVE_HI: {
		Furnish:
			furnish(WATER);
		Enter1:
		JUMP:
		D:
		CLIMB:
			rope(CAVE_HI, CAVE_LO, fn);
		Look:
			"You are at the bottom of the stalactite. You can see a stalagmite
			immediately below you, although it is too far down to reach.\n";
		Brief:
			"You're at bottom of stalactite.\n";
		Go:
			travel(U,CAVE_TOP, D,CLIMB,CAVE_LO);
		DROP:
		THROW:
			if(fn==THROW && obj==ROPE)
				arg;
			else if(obj == SWORD)
				"The sword remains firmly in your hand, and will not allow you to let go.\n";
			else {
				putstr("You see your object fall into the gloom below.\n");
				if(objexec(who, CAVE_BTM, THROW, obj, arg));
					drop(obj,CAVE_BTM);
				0;
			}
		}

	CAVE_TOP: {
		Furnish:
			furnish(WATER);
		Look:
			"You are at the top of a stalactite which disappears into the depths below.\n";
		Go:
			travel(U,EAGLE,EAGLE_NEST, D,CAVE_HI);
		}

	CAVE_END: {
		Look:
			"Dead end.\n";
		Go:
			travel(E,OUT, CAVE_W);
		}

	CUBBY_HOLE: {
		Look:
			"You are in a cubby hole in the rock to the north of the cavern.\n";
		Go:
			travel(S,OUT, CAVE_N);
		}

	EAGLE_NEST: {
		Look:
			"You are in an eagles's eyre. You can make out some holes in the walls,
			as well as a small hole in the floor.\n";
		Brief:
			"You're in eagles's eyre.\n";
		Go:
			travel(D,CAVE_TOP, W,SWORD_RM, E,ART_GALLERY);
		E:
			if(carrying(SWORD,who))
				"The sword is too big to fit through the hole.\n";
			else
				ART_GALLERY;
		}

	SWORD_RM: {
		Furnish:
			furnish(ROCK);
		Look:
			"You are in a small room of stone. The only exit is to the east.
			There is a large rock on the floor.\n";
		Brief:
			"You're in sword room.\n";
		Go:
			travel(E,OUT,EAGLE, EAGLE_NEST);
		}

	ART_GALLERY: {
		Furnish:
			furnish(LIGHT,DOOR,ELEVATOR);
		Look:
			"You are in an art gallery. There is a crack in the west wall.
			There is a large door with numbers above it to the east.\n";
		Brief:
			"You're in art gallery.\n";
		Go:
			travel(W,CRACK,EAGLE,EAGLE_NEST, E,IN,ELEV_RM);
		E:
		IN:
			elev(1);
		W:
		CRACK:
		EAGLE:
			if(carrying(SWORD,who))
				"The sword is too big to fit through the wall.\n";
			else
				EAGLE_NEST;
		}

	ELEV_RM: {
		Furnish:
			furnish(LIGHT,DOOR,ELEVATOR);
		Look:
			"You are in a small room with the numbers 0 through 4 on the wall above.\n";
		U:
			if(.ELEVATOR==0)
				"Sorry, you cannot go any higher\n";
			else {
				--.ELEVATOR;
				ELEV_RM;
			}
		D:
			if(.ELEVATOR == 4)
				"Sorry, you cannot go any lower\n";
			else {
				++.ELEVATOR;
				ELEV_RM;
			}
		Go:
			travel(IN,U,D, ELEV_RM,
				OUT, which(.ELEVATOR,-ORCH_1,ART_GALLERY,FOOD_JCT,WAITING_RM,KEY_RM));
		OUT:
			i = getloc(loc,fn);
			if(i==KEY_RM && objloc[KEY_RM]==0)
				"Sorry, but passengers are not allowed to get out on this floor.\n";
			else
				i;
		}

	FOOD_JCT: {
		Furnish:
			furnish(LIGHT,DOOR,ELEVATOR);
		Look:
			"You are at a junction of a east-west passage with a passage
			going south. There is a door on the north wall with numbers above it.
			A strong smell is coming from the east, and a cold draft from the south.\n";
		Brief:
			"You are at junction of east-west passage and south passage.\n";
		Go:
			travel(W,CAVE_EE, E,FOOD_RM, S,ICE_PSG, N,IN,ELEV_RM);
		N:
		IN:
			elev(2);
		}

	FOOD_RM: {
		Furnish:
			furnish(MACHINE,TABLE);
		Look:
			"You are in a large room with many rotten wooden tables.
			There are stains on the walls which look like they were once remains
			of some kinds of food. Off in the corner there are several vending
			machines. Written in crayon above them appears the word 'F00D'.
			The only exit is to the west.\n";
		Brief:
			"You are in the cafeteria.\n";
		F00D:
			WELL_HOUSE;
		Go:
			travel(W,OUT, FOOD_JCT);
		}

	ICE_PSG: {
		Furnish:
			furnish(ICE);
		Look:
			"You are in a north-south passage whose walls seem to be covered with ice.\n";
		Go:
			travel(N,FOOD_JCT, S,SKATE_RM);
		}


	WAITING_RM: {
		Furnish:
			furnish(LIGHT,DOOR,ELEVATOR);
		Look:
			"You are in a small waiting room. There is a low passage to the west.
			There is a door on the east wall with numbers above it.\n";
		Brief:
			"You are in waiting room.\n";
		Go:
			travel(W,CAVE_E, E,IN,ELEV_RM);
		E:
		IN:
			elev(3);
		}

	WINDING_PSG: {
		Look:
			"You are in a narrow winding passage, which leads to the east and west.\n";
		Go:
			travel(W,CAVE_S, E,ICE_N);
		}

	ICE_N: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the north side of a large frozen surface.\n";
		Go:
			travel(N,WINDING_PSG, S,ICE_RINK, CROSS,ICE_S);
		}

	ICE_NE: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the northeast side of a large frozen surface.\n";
		Go:
			travel(NE,STONE_DOOR, SW,ICE_RINK, CROSS,ICE_SW);
		}

	ICE_E: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the east side of a large frozen surface.\n";
		Go:
			travel(E,FOUNT_RM, W,ICE_RINK, CROSS,ICE_W);
		}

	ICE_SE: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the southeast side of a large frozen surface.\n";
		Go:
			travel(SE,DOOR_HOLE, NW,ICE_RINK, CROSS,ICE_NW);
		}

	ICE_S: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the south side of a large frozen surface.\n";
		Go:
			travel(S,FIRE_RM, N,ICE_RINK, CROSS,ICE_N);
		}

	ICE_SW: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the southwest side of a large frozen surface.\n";
		Go:
			travel(SW,TREE_PSG, NE,ICE_RINK, CROSS,ICE_NE);
		}

	ICE_W: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the west side of a large frozen surface.\n";
		Go:
			travel(W,LO_JCT, E,ICE_RINK, CROSS,ICE_E);
		}

	ICE_NW: {
		Furnish:
			furnish(ICE);
		Look:
			"You are on the northwest side of a large frozen surface.\n";
		Go:
			travel(NW,ICE_DEAD_END, SE,ICE_RINK, CROSS,ICE_SE);
		}

	ICE_RINK: {
		Furnish:
			furnish(ICE);
		Enter1:
		U:
		CLIMB:
		JUMP:
			rope(ICE_RINK, SKATE_RM, fn);
		Enter2:
			if(who!=ME || wearing(SKATES,who) || at(who,FROG) ||
				agent_clockp[3]<ICE_N || agent_clockp[3]>ICE_NW)
					0;
			else {
				putstr("Since the surface is slippery, you slide to the other side\n");
				move((agent_clockp[3]-ICE_N+4)%8+ICE_N);
			}
		Look:
			"You are in the middle of a large frozen surface.
			There is a hole in the cavern ceiling high above.\n";
		Brief:
			"You are in the middle of a large frozen surface.\n";
		Go:
			travel(N,ICE_N, NE,ICE_NE, E,ICE_E, SE,ICE_SE, S,ICE_S,
				SW,ICE_SW, W,ICE_W, NW,ICE_NW, U,CLIMB,SKATE_RM);
		N::NW:
			if(who!=ME || wearing(SKATES,who) || at(who,FROG) || at(who,TROLL))
				ICE_N-N+fn;
			else
				"You can't move around on the slippery surface!\n";
		THROW:
			if(wearing(SKATES,who) || !carrying(obj,who))
				arg;
			else if((i=uargn[2]) == 0) {
				"I assume that you are just throwing the object around. If you want
				to get off the ice, please throw something in a particular direction.\n";
				arg;
			} else if(i<N || i>NW) {
				"Please specify a direction.\n";
			} else {
				"";
				putstr(objexec(who, ICE_N+i-N, DROP, obj, 1));
				putstr("The reaction sends you in the opposite direction.\n");
				move(ICE_N + (i-N+4)%8);
			}
		}

	SKATE_RM: {
		Furnish:
			furnish(ICE);
		Enter1:
		D:
		CLIMB:
		JUMP:
			rope(SKATE_RM, ICE_RINK, fn);
		Look:
			"You are in a small room of ice. The only passage leads to the north,
			but you can see a large frozen surface through a hole in the floor.
			It is a long way down\n";
		Brief:
			"You are in small room of ice.\n";
		Go:
			travel(N,OUT,ICE_PSG, D,CLIMB,ICE_RINK);
		}

	ICE_DEAD_END: {
		Look:
			"Dead end\n";
		Go:
			travel(SE,OUT, ICE_NW);
		}

	FOUNT_RM: {
		Furnish:
			furnish(WATER,FOUNTAIN);
		Look:
			"You are in a small east-west passage with a drinking fountain in the middle.\n";
		Go:
			travel(W,ICE_E, E,HAY_RM);
		}

	HAY_RM: {
		Furnish:
			furnish(RAFTER);
		Look:
			"You are in a large room with wooden rafters. Exits are to the north and south.\n";
		Go:
			travel(N,FOUNT_RM, S,FIRE_RM);
		SLEEP:
			if(at(HAY,loc))
				"A sharp object punctures your rear end, and you quickly
				wake up again.\n";
			else
				locexec(who, 1, fn, obj, arg);
		}

	FIRE_RM: {
		Furnish:
			furnish(INSCRIPTION);
			if(at(FIRE,loc))
				furnish(LIGHT);
		Look:
			"You are in a dark room with evil-looking inscriptions engraved
			into the stone walls. Exits are to the north and east.\n";
		Brief:
			"You are in a dark room with evil inscriptions.\n";
		Inread:
			"The inscriptions tell of the One Ring of power, which was forged in 
			volcanic fires. This fabled ring enables the wearer to become
			invisible, even in broad daylight.\n";
		Go:
			travel(N,ICE_S, E,HAY_RM);
		}

	STONE_DOOR: {
		Furnish:
			furnish(MESSAGE,WALL);
		Look:
			if(.STONE_DOOR)
				"The way east is through an open door\n";
			else
				"The way east is barred by what appears to be a solid stone wall.
				There is a message written high up on the wall.\n";
		Go:
			travel(W,ICE_NE, E,IN,STATUE_RM);
		E:
		IN:
			if(.STONE_DOOR)
				STATUE_RM;
			else
				"There is no way to go in that direction.\n";
		Inread:
			"The message reads, 'Speak, friend, and enter.'\n";
		FRIEND:
			if(.STONE_DOOR ^= 1)
				"A door opens in the wall.\n";
			else
				"The door vanishes, leaving a wall of solid rock.\n";
		}

	STATUE_RM: {
		Furnish:
			furnish(STATUE,NOTE);
		Look:
			putstr("You are in a large east-west corridor, with many statues of men
			crumbling to dust on either side. A note is tacked up on the
			east entrance, reading, \"I am asleep, please do not disturb -- Medusa\".\n");
		if(!at(MEDUSA,0))
		if(!carrying(GLASSES,who)) "I would advise your not entering. \n";
		Go:
			travel(W,STONE_DOOR, E,MEDUSA_RM);
		W:
			if(.STONE_DOOR == 0)
				putstr("You slip out through a door in a stone wall, which snaps shut again.\n");
			STONE_DOOR;
		Inread:
			"The note says \"I am asleep, please do not disturb -- Medusa\".\n";
		}

	MEDUSA_RM: {
		Furnish:
			furnish(STATUE);
		Look:
			"You are in Medusa's lair. The exit is to the west, and there are
			chambers off to the north and south. There are several statues here.\n";
		Brief:
			"You are in Medusa's lair.\n";
		Go:
			travel(W,STATUE_RM, N,LAB, S,LAB2);
		}

	LAB: {
		Look:
			"You are in a large medieval laboratory. There are many strange
			and bizarre things in this room, far too difficult for any
			layman to understand, so I will not bother even trying.
			The only exit is to the south.\n";
		Brief:
			"You are in a large medieval laboratory.\n";
		Go:
			travel(S,OUT, MEDUSA_RM);
		}

	LAB2: {
		Look:
			"You are in the ruins of a large medieval laboratory. Everything is
			in ruins, and appears to be the aftermath of a recent explosion.
			the only way out is to the north.\n";
		Brief:
			"You are in the ruins of a large medieval laboratory.\n";
		Go:
			travel(N,OUT, MEDUSA_RM);
		}

	DOOR_HOLE: {
		Look:
			"There is a hole in the ground, and a passage to the northwest.\n";
		Go:
			travel(NW,ICE_SE, D,HOLE,DOOR_RM);
		}

	DOOR_RM: {
		Enter1:
			.DOOR_RM = N + rand(8);
		Enter2:
			if(at(PRINCESS,DOOR_RM)) {
				putstr("The princess, for luck, suggests you try door #");
				putnum(.DOOR_RM-N+1);
				".\n";
			}
		Look:
			putstr("You are in a large room, with doors on all 8 walls, labelled
			1 through 8, starting clockwise form the north. An inscription
			on the floor says 'Behind one of these doors is the key to success,
			but all others lead to instant death!'
			The only other way out is through the hole in the ceiling.\n");
		if(!carrying(PRINCESS,who)) "I would not advise trying a 1 in 8 chance!\n";
		Brief:
			"You are in room with eight doors.\n";
		Go:
			travel(U,DOOR_HOLE, BACK,N,NE,E,SE,S,SW,W,NW,KEY_RM);
		BACK:
			my_back;
		N::NW:
			if(fn == .DOOR_RM)
				KEY_RM;
			else
				die(DOOR_RM,"As soon as you open the door, a seven-headed monster
				jumps out and devours you!\n");
		SAVE :
			.DOOR_RM = N + rand(8);
			"2 llinks saved\n";
		}

	KEY_RM: {
		Furnish:
			furnish(DOOR,ELEVATOR);
		Look:
			"You are in a pit. There is a hole in the wall three feet up, and a small hole
			in the east wall. There is a door with numbers above it on the west wall.\n";
		Brief:
			"You are in pit with elevator.\n";
		Go:
			travel(U,DOOR_RM, E,-LAIR_0, W,IN,ELEV_RM);
		W:
		IN:
			elev(4);
		}

	LAIR_0: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_1, S,-LAIR_2, E,-LAIR_3, W,-LAIR_6, NE,-LAIR_4,
				SE,-LAIR_0, NW,-LAIR_7, SW,KEY_RM);
		}

	LAIR_1: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_2, S,-LAIR_0, E,-LAIR_4, W,-LAIR_7, NE,-LAIR_1,
				SE,-LAIR_3, NW,-LAIR_8, SW,-LAIR_5);
		}

	LAIR_2: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_0, S,-LAIR_1, E,-LAIR_5, W,-LAIR_8, NE,-LAIR_2,
				SE,-LAIR_7, NW,-LAIR_3, SW,-LAIR_6);
		}

	LAIR_3: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_4, S,-LAIR_5, E,-LAIR_6, W,-LAIR_0, NE,-LAIR_8,
				SE,-LAIR_2, NW,-LAIR_1, SW,-LAIR_3);
		}

	LAIR_4: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_5, S,-LAIR_3, E,-LAIR_7, W,-LAIR_1, NE,-LAIR_6,
				SE,-LAIR_8, NW,-LAIR_2, SW,-LAIR_4);
		}

	LAIR_5: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_3, S,-LAIR_4, E,-LAIR_8, W,-LAIR_2, NE,-LAIR_0,
				SE,-LAIR_6, NW,-LAIR_5, SW,-LAIR_7);
		}

	LAIR_6: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_7, S,-LAIR_8, E,-LAIR_0, W,-LAIR_3, NE,-LAIR_5,
				SE,-LAIR_4, NW,-LAIR_6, SW,-LAIR_1);
		}

	LAIR_7: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_8, S,-LAIR_6, E,-LAIR_1, W,-LAIR_4, NE,-LAIR_7,
				SE,-LAIR_5, NW,-LAIR_0, SW,-LAIR_2);
		}

	LAIR_8: {
		Furnish:
		Look:
			"You are in a spider's lair.\n";
		Go:
			travel(N,-LAIR_6, S,-LAIR_7, E,-LAIR_2, W,-LAIR_5, NE,-LAIR_3,
				SE,-LAIR_1, NW,-LAIR_4, SW,LAIR_9);
		}

	LAIR_9: {
		Look:
			"Dead end.\n";
		Go:
			travel(NE,OUT, -LAIR_8);
		}

	TREE_PSG: {
		Look:
			"You are in a long east-west passage.\n";
		Go:
			travel(E,ICE_SW, W,TREE_BTM);
		}

	TREE_BTM: {
		Furnish:
			furnish(LIGHT);
		Enter1:
		U:
		CLIMB:
		JUMP:
			if(.ACORN == 0)
				rope(TREE_BTM, ORCH_4, fn);
			else if(fn == U)
				TREE_MID;
			else
				arg;
		Look:
			"You are in a small fertile cavern. There are passages to the east and west.
			Light is coming from a hole in the ceiling about 30 feet up. There appear
			to be some holes up the sides of the walls, but they are too high to reach.\n";
		Brief:
			"You are in a small fertile cavern.\n";
		Go:
			travel(E,TREE_PSG, W,TREE_W, U,CLIMB,.ACORN?TREE_MID:-ORCH_4);
		}

	TREE_MID: {
		Furnish:
			furnish(LIGHT);
		Enter1:
		JUMP:
		U:
			if(.ACORN <= 1)
				rope(TREE_MID, ORCH_4, fn);
			else if(fn == U)
				TREE_TOP;
			else
				arg;
		Look:
			if(.ACORN == 1)
				"You are at the top of a tree. There is a hole in the cave wall on the
				south side, and another one high up on the north wall. Light is shining
				through a hole in the ceiling.\n";
			else
				"You are in the middle of a tree. There is a hole in the south cave wall.\n";
		S:
			putstr("You slide down a slippery tunnel.\n");
			TREE_BTM;
		Go:
			travel(D,TREE_BTM, S,TREE_BTM, U,.ACORN>1?TREE_TOP:-ORCH_4);
		}

	TREE_TOP: {
		Furnish:
			furnish(LIGHT);
		Enter1:
		CLIMB:
		JUMP:
			rope(TREE_TOP, ORCH_4, fn);
		Look:
			"You are at the top of a tree. There is a hole on the north cave wall.
			Light is streaming in through a hole in the ceiling just above your head.\n";
		Go:
			travel(D,TREE_MID, U,-ORCH_4, N,BEDCHAMBER);
		}

	BEDCHAMBER: {
		Look:
			"You are in an ornate chamber with passages leading to the north and south.\n";
		Go:
			travel(S,OUT,TREE_BTM+.ACORN, N,IN,BED_RM);
		}

	BED_RM: {
		Furnish:
			destroy(RAFT);
			furnish(BED);
		Look:
			"You are in a glamourous royal bedroom. There is a plush bed in the centre
			of the room, freshly made.\n";
		Brief:
			"You're in bedroom.\n";
		Go:
			travel(S,OUT, BEDCHAMBER);
		SLEEP:
			.X = 0;
			.Y = 0;
			drop(RAFT,ISLAND);
			ISLAND;
		}

	TREE_W: {
		Furnish:
			furnish(LIGHT);
		Look:
			"You have come to the brink of an extremely large pit which
			extends as far down as you can see. You can try climbing down,
			but you will probably not be able to get back up.
			A passage leads back to the east.\n";
		Go:
			travel(E,OUT,TREE_BTM, D,CLIMB,BIG_PIT);
		JUMP:
			jump(BIG_PIT);
		}

	BIG_PIT: {
		Furnish:
			furnish(PIT,SKELETON);
		Look:
			"You are at the bottom of the pit. The walls are too steep to climb.\n";
		Go:
			travel(U, TREE_W);
		JUMP:
		CLIMB:
		U:
		OUT:
			"The walls are too steep. There is no way out of the pit.\n";
		}

	ISLAND: {
		Locate:
			10;
		Furnish:
			furnish(LIGHT,WATER);
		Look:
			"You are on a tropical desert island in the middle of a large ocean.\n";
		Go:
			travel();
		N::NW:
			if(arg == 1)
				"I don't suggest swimming in the shark-infested waters.\n";
			else
				arg;
		}

	SEA: {
		Furnish:
			furnish(LIGHT,WATER);
		Look:
			"You are in the middle of a large shark-infested ocean.\n";
		Go:
			travel();
		JUMP:
		SWIM:
		OVERBOARD:
			die(ISLAND,"As you jump in, you are devoured by man-eating sharks!\n");
		}


	LO_STEPS: {
		Furnish:
			furnish(LIGHT,STAIR);
		Look:
			"You are at the bottom of a large staircase. You can see daylight
			at the top of the stairs.\n";
		Brief:
			"You are at bottom of large staircase.\n";
		Go:
			travel(N,U,OUT,HI_STEPS, S,IN,CAVE_NN);
		}

	HI_STEPS: {
		Furnish:
			furnish(LIGHT,STAIR);
		Look:
			"You are outside the cave. There is a staircase going
			underground to the south.\n";
		Go:
			travel(S,D,IN,LO_STEPS, N,PLATEAU_A);
		}

	PLATEAU_A: {
		Furnish:
			furnish(LIGHT);
		Look:
			"You are at the top of a plateau which is surrounded by a sea of
			dense weeds. South of you there is an entrance to the cave.
			Several tall ruined towers can be seen in the distance.\n";
		Brief:
			"You're at top of plateau near cave.\n";
		Go:
			travel(S,HI_STEPS, D,-WEED_1A);
		}

	WEED_1A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_1A, N,-WEED_3A, S,PLATEAU_A, E,-WEED_4A, W,-WEED_2A);
		}

	WEED_2A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_3A, N,S,-WEED_2A, E,-WEED_1A);
		}

	WEED_3A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,N,-WEED_2A, S,-WEED_1A, E,W,-WEED_3A);
		}

	WEED_4A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_1A, N,-WEED_7A, S,-WEED_5A, E,-WEED_6A, W,-WEED_4A);
		}

	WEED_5A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,S,-WEED_5A, N,-WEED_4A, E,W,-WEED_6A);
		}

	WEED_6A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_6A, N,S,-WEED_5A, W,-WEED_4A);
		}

	WEED_7A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_7A, N,-WEED_8A, S,-WEED_10A, E,-WEED_4A, W,-WEED_9A);
		}

	WEED_8A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_9A, N,E,-WEED_8A, S,-WEED_7A);
		}

	WEED_9A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,S,-WEED_8A, N,E,-WEED_9A, W,-WEED_7A);
		}

	WEED_10A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_10A, N,S,W,-WEED_11A, E,TOWER_A);
		}

	WEED_11A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_11A, N,S,W,-WEED_10A);
		}

	WEED_12A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_13A, N,S,-WEED_12A, E,-WEED_7A);
		}

	WEED_13A: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_12A, N,S,W,-WEED_13A);
		}

	TOWER_A: {
		Furnish:
			furnish(LIGHT,TOWER);
		Look:
			"You are at the base of a ruined tower surrounded by dense weeds.\n";
		Go:
			travel(U,IN,TOWERT_A, W,-WEED_12B);
		}

	TOWERT_A: {
		Furnish:
			furnish(LIGHT,TOWER);
		Look:
			"You are at the top of a ruined tower. Another similar tower can be
			seen in the distance.\n";
		Go:
			travel(D,OUT, TOWER_A);
		}

	WEED_1B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_1B, N,-WEED_3B, S,PLATEAU_B, E,-WEED_4B, W,-WEED_2B);
		}

	WEED_2B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_3B, N,S,-WEED_2B, E,-WEED_1B);
		}

	WEED_3B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,N,-WEED_2B, S,-WEED_1B, E,W,-WEED_3B);
		}

	WEED_4B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_1B, N,-WEED_7B, S,-WEED_5B, E,-WEED_6B, W,-WEED_4B);
		}

	WEED_5B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,S,-WEED_5B, N,-WEED_4B, E,W,-WEED_6B);
		}

	WEED_6B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_6B, N,S,-WEED_5B, W,-WEED_4B);
		}

	WEED_7B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_7B, N,-WEED_8B, S,-WEED_10B, E,-WEED_4B, W,-WEED_9B);
		}

	WEED_8B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_9B, N,E,-WEED_8B, S,-WEED_7B);
		}

	WEED_9B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,S,-WEED_8B, N,E,-WEED_9B, W,-WEED_7B);
		}

	WEED_10B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,-WEED_10B, N,S,W,-WEED_11B, E,TOWER_B);
		}

	WEED_11B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_11B, N,S,W,-WEED_10B);
		}

	WEED_12B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,W,-WEED_13B, N,S,-WEED_12B, E,-WEED_7B);
		}

	WEED_13B: {
		Furnish:
			furnish(LIGHT,WEED);
		Look:
			"You are in a sea of dense weeds.\n";
		Go:
			travel(D,E,-WEED_12B, N,S,W,-WEED_13B);
		}

	TOWER_B: {
		Furnish:
			furnish(LIGHT,TOWER);
		Look:
			"You are at the base of a ruined tower surrounded by dense weeds.\n";
		Go:
			travel(U,IN, TOWERT_B,
				W, -WEED_12A);
		}

	TOWERT_B: {
		Furnish:
			furnish(LIGHT,TOWER);
		Look:
			"You are at the top of a ruined tower. Another similar tower can be
			seen in the distance.\n";
		Go:
			travel(D,OUT, TOWER_B);
		}

	PLATEAU_B: {
		Furnish:
			furnish(LIGHT);
		Look:
			"You are at the top of a plateau which is surrounded by a sea of
			dense weeds. East of you there is a large grove of trees.
			Several tall ruined towers can be seen in the distance.\n";
		Brief:
			"You are at top of plateau near grove.\n";
		Go:
			travel(E,TREE_FRONT, D,-WEED_1B);
		}

	TREE_FRONT: {
		Furnish:
			furnish(LIGHT);
		Look:
			"There is a plateau to the west, and a grove to the east.\n";
		Go:
			travel(W,PLATEAU_B, E,TREE_4);
		}

	TREE_0::TREE_11: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are in a grove of beautiful mallorn trees.\n";
		Go:
			j = 0;
			if(loc == TREE_4) {
				travtab[j++] = W;
				travtab[j++] = OUT;
				travtab[j++] = TREE_FRONT;
			}
			for(i=0; i<12; ++i)
				if(k = which((loc-TREE_0)*5/4-i*5/4+13,
					0,0,0,0,0,0,0,SE,S,SW,0,0,E,0,W,0,0,NE,N,NW,0,0,0,0,0,0,0)) {
						travtab[j++] = k;
						travtab[j++] = TREE_0+i;
					}
			travtab[j++] = U;
			travtab[j++] = CLIMB;
			travtab[j++] = TREET_0-TREE_0+loc;
			travtab[j++] = 0;
		}

	TREET_0::TREET_11: {
		Furnish:
			furnish(LIGHT,TREES);
		Look:
			"You are at the top of a large mallorn tree.\n";
		Go:
			travel(D,OUT, loc-TREET_0+TREE_0);
		}

	REPOS_NW: {
		Furnish:
			furnish(LIGHT,BED);
		Enter1:
		U:
		CLIMB:
		JUMP:
			rope(REPOS_NW, REPOS_NE, fn);
		Look:
			"You are in a huge dimly-lit room below the repository.
			There are several beds here, each with a medusa sleeping on it.
			Off to the side is a pile of brooms, and a colossal haystack.
			There are several grandfather clocks here, none of which is
			ticking, and each of which seems to show a different time.
			Some vending machines can be seen seen at the other end of the room.\n";
		Brief:
			"You are beneath repository.\n";
		Go:
			travel(U,CLIMB,REPOS_NE, SE,CROSS,REPOS_SE);
		}

	REPOS_SE: {
		Furnish:
			furnish(LIGHT,DOOR);
		Look:
			"You are at the southeast end of the dimly lit room.
			There are several vending machines along one wall, each
			with the picture of a different treasure on it. On the
			north wall is an iron door which parts in the center.
			There are several numbers above the door, but none of
			them is lit up.\n";
		Brief:
			"You're near vending machines.\n";
		Go:
			travel(NW,CROSS,REPOS_NW, N,IN,DOOR,REPOS_ELEV);
		}

	REPOS_ELEV: {
		Furnish:
			furnish(LIGHT);
		Look:
			"You are in a small room, which is featurless except for
			two arrow-like markings above, one pointing up, and one down.
			Niether of the markings is lit up. The only way out is through
			an iron door which parts in the center.\n";
		Go:
			travel(S,OUT, REPOS_SE);
		OUT:
		S:
			"Sorry, but the door refuses to open. I suggest using the elevator.\n";
		D:
			quit("The elevator plunges down into the depths of the earth for
			what seems like forever, and the temperature keeps rising with
			the depth. Eventually, long before the elevator is melted into
			molten slag, you are fried to a crisp.\n",10);
		U:
			quit("The elevator rises for what seems like forever, and
			eventually reaches the surface, where you find you are in
			an earthly paradise, where a band of merry elves carries
			you off into the sunset.\n",30);
		}

	REPOS_NE: {
		Furnish:
			furnish(LIGHT,CHANDELIER,RAFTER,TABLE);
		Enter1:
		JUMP:
		CLIMB:
			if(.GRATE != 2)
				locexec(who, 0, fn, obj, arg);
			else
				rope(REPOS_NE, REPOS_NW, fn);
		Look:
			"You are in an immense room. It appears to be a repository for the
			\"Adventure\" program. A huge chandelier hanging from rafters above
			fills the room with a bright yellow light. Scattered about you can
			be seen a pile of empty bottles, a bundle of wooden stakes, and
			some long ropes. To one side you can see several old wooden tables.
			Some freshly-folded handkerchiefs are lying under the table.
			Off in the distance you can see other sundry objects at the other
			end of the room. There is a small iron grate in the floor.\n";
		Brief:
			"You're at northeast end of repository.\n";
		Go:
			travel(SW,CROSS,REPOS_SW, D,REPOS_NW);
		D:
			switch(.GRATE) {
			default:
				"You can't go through a locked steel grate!\n";
			1:
				"You can't go through a closed steel grate!\n";
			2:
				rope(REPOS_NE, REPOS_NW, fn);
			}
		}

	REPOS_SW: {
		Furnish:
			furnish(LIGHT,CHANDELIER,RAFTER,STONE,TOMB);
		Look:
			"You are at the southwest end of the repository. Off to one side
			are several freshly-cut tombs, near which lay many skeletons,
			each of which is wearing glasses and skates.
			Several sets of keys are strewn about on the floor.\n";
		Brief:
			"You're at southwest end of repository.\n";
		Go:
			travel(NE,CROSS, REPOS_NE);
		Inread:
			"The tombs have been freshly hewn, and have not yet been engraved.\n";
		}

	}
}
