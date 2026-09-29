/*
 * Amiga MUD
 *
 * Copyright (c) 1991 by Chris Gray
 */

/*
 * code - define the verbs for the world.
 */

/* some properties for rooms */

private p_rName CreateStringProp()$		/* the name of a room */
private p_rNameAction CreateActionProp()$	/* do instead */
private p_rDesc CreateStringProp()$		/* full description of room */
private p_rDescAction CreateActionProp()$	/* do instead */
private p_rContents CreateThingListProp()$	/* the room's contents */
private p_rDark CreateBoolProp()$		/* is it dark here? */
private p_rVisited CreateBoolProp()$		/* true if been visited */

private p_rAnyEnterCheck CreateActionProp()$	/* on any enter of room */
private p_rAnyLeaveCheck CreateActionProp()$	/* on any leave of room */

private p_rNorth CreateThingProp()$		/* actual north connection */
private p_rNorthCheck CreateActionProp()$	/* go-north check */

private p_rSouth CreateThingProp()$
private p_rSouthCheck CreateActionProp()$

private p_rEast CreateThingProp()$
private p_rEastCheck CreateActionProp()$

private p_rWest CreateThingProp()$
private p_rWestCheck CreateActionProp()$

private p_rNorthEast CreateThingProp()$
private p_rNorthEastCheck CreateActionProp()$

private p_rNorthWest CreateThingProp()$
private p_rNorthWestCheck CreateActionProp()$

private p_rSouthEast CreateThingProp()$
private p_rSouthEastCheck CreateActionProp()$

private p_rSouthWest CreateThingProp()$
private p_rSouthWestCheck CreateActionProp()$

private p_rUp CreateThingProp()$
private p_rUpCheck CreateActionProp()$

private p_rDown CreateThingProp()$
private p_rDownCheck CreateActionProp()$

private p_rEnter CreateThingProp()$
private p_rEnterCheck CreateActionProp()$

private p_rExit CreateThingProp()$
private p_rExitCheck CreateActionProp()$

/* some properties for the person */

private p_pCarrying CreateThingListProp()$
private p_pBookCount CreateIntProp()$
private p_pBookCombination CreateIntProp()$	/* for opening the bookcase */
private p_pJarCount CreateIntProp()$
private p_pSpells CreateThingListProp()$	/* spells ready to cast */
private p_pLight CreateBoolProp()$		/* player is glowing */

/* some properties for objects */

private p_oName CreateStringProp()$		/* encoded name of object */
private p_oDesc CreateStringProp()$		/* long description of it */
private p_oVisible CreateBoolProp()$		/* show up on look */
private p_oCarryable CreateBoolProp()$		/* can pick it up */
private p_oText CreateStringProp()$		/* text of books */
private p_oFixedText CreateStringProp()$	/* text on scenery */
private p_oReplaceLoc CreateThingProp()$	/* where item belongs */
private p_oEncyclopedia CreateBoolProp()$	/* mark the volumes */
private p_oJar CreateBoolProp()$		/* mark the jars */
private p_oPortionCount CreateIntProp()$	/* count of portions left */
private p_oMixCode CreateStringProp()$		/* code for contents */
private p_oCauldron CreateBoolProp()$		/* mark it */
private p_oCurrentMix CreateStringProp()$	/* now in cauldron */
private p_oCode CreateStringProp()$		/* code on actual spell */

private p_oLookAction CreateActionProp()$
private p_oGetAction CreateActionProp()$
private p_oDropAction CreateActionProp()$
private p_oSpellAction CreateActionProp()$

/* now some general utility routines */

/*
 * showList - print out a contents/carrying list. Return 'true' if nothing
 *	was printed.
 */

private proc showList(list thing lt; string starter)bool:
    int count, i;
    thing object;
    string s;
    bool first;

    first := true;
    count := Count(lt);
    if count ~= 0 then
	i := 0;
	while i ~= count do
	    object := lt[i];
	    if object@p_oVisible then
		if first then
		    first := false;
		    Print(starter);
		fi;
		Print("  " + FormatName(object@p_oName) + "\n");
	    fi;
	    i := i + 1;
	od;
    fi;
    first
corp;

/*
 * enterRoom - called whenever a player finally enters a given room.
 */

private proc enterRoom(thing dest)void:
    action a;
    bool f;
    string s;

    a := dest@p_rAnyEnterCheck;
    if a = nil or call(a, status)() = continue then
	/* nothing funny like an instant teleport (which would likely
	   end up calling 'enterRoom' recursively for the destination!) */
	if dest@p_rDark and not Me()@p_pLight then
	    Print("It is dark.\n");
	else
	    a := dest@p_rNameAction;
	    if a = nil then
		Print("You are " + dest@p_rName + ".\n");
	    else
		ignore call(a, status)();
	    fi;
	    if not dest@p_rVisited then
		dest@p_rVisited := true;
		a := dest@p_rDescAction;
		if a = nil then
		    /* go ahead and give the normal desciption */
		    s := dest@p_rDesc;
		    if s = "" then
			Print("You see nothing special here.\n");
		    else
			Print(s + "\n");
		    fi;
		else
		    ignore call(a, status)();
		fi;
	    fi;
	    f := showList(dest@p_rContents, "Nearby:\n");
	fi;
	SetLocation(dest);
    fi;
corp;

/*
 * Die - the player did something not good.
 */

private proc dieHandler(string s)void:
    Quit();
corp;

private proc Die()void:
    Print("You have died.\n");
    ignore SetPrompt("Press RETURN to exit: ");
    ignore SetCharacterInputAction(dieHandler);
corp;

/*
 * doMove - bottom level routine to attempt to move in the given direction.
 */

private proc doMove(property thing direction; property action check)bool:
    action a;
    thing here, dest;

    here := Here();
    dest := here@direction;
    if dest = nil then
	Print("You can't go in that direction.\n");
	false
    else
	a := here@check;
	if a = nil or call(a, status)() = continue then
	    /* nothing was blocking the exit, and nothing abnormal happened
	       when we tried to go through */
	    a := here@p_rAnyLeaveCheck;
	    if a = nil or call(a, status)() = continue then
		/* nothing funny like player being chained to the floor */
		enterRoom(dest);
		true
	    else
		false
	    fi
	else
	    false
	fi
    fi
corp;

/* routines for going in specific directions (these are actual verbs) */

private proc v_north()bool:
    doMove(p_rNorth, p_rNorthCheck)
corp;

private proc v_south()bool:
    doMove(p_rSouth, p_rSouthCheck)
corp;

private proc v_east()bool:
    doMove(p_rEast, p_rEastCheck)
corp;

private proc v_west()bool:
    doMove(p_rWest, p_rWestCheck)
corp;

private proc v_northEast()bool:
    doMove(p_rNorthEast, p_rNorthEastCheck)
corp;

private proc v_northWest()bool:
    doMove(p_rNorthWest, p_rNorthWestCheck)
corp;

private proc v_southEast()bool:
    doMove(p_rSouthEast, p_rSouthEastCheck)
corp;

private proc v_southWest()bool:
    doMove(p_rSouthWest, p_rSouthWestCheck)
corp;

private proc v_up()bool:
    doMove(p_rUp, p_rUpCheck)
corp;

private proc v_down()bool:
    doMove(p_rDown, p_rDownCheck)
corp;

private proc v_enter()bool:
    doMove(p_rEnter, p_rEnterCheck)
corp;

private proc v_exit()bool:
    doMove(p_rExit, p_rExitCheck)
corp;

private proc v_quit()bool:
    Quit();
    false
corp;

private proc DirMatch(string name)bool:
    MatchName(
	"north,n.northeast,ne.east,e.southeast,se."
	"south,s.southwest,sw.west,w.northwest,nw."
	"up,u.down,d.enter,in.exit,out,leave", name) ~= -1
corp;

/* some standard verb routines and the corresponding verbs */

private proc v_go(string direction)bool:
    if DirMatch(direction) then
	Parse(G, direction) ~= 0
    else
	Print("I don't understand that direction.\n");
	false
    fi
corp;

private proc v_look(string what)bool:
    thing here, object;
    string name, s;
    action a;
    status st;

    here := Here();
    if what = "" or what = "around" then
	/* assume the player wants to look around the current room. */
	if here@p_rDark and not Me()@p_pLight then
	    Print("It is dark here - you can't see anything.\n");
	    true
	else
	    a := here@p_rNameAction;
	    if a = nil then
		Print("You are " + here@p_rName + ".\n");
	    else
		ignore call(a, status)();
	    fi;
	    a := here@p_rDescAction;
	    if a = nil then
		/* nothing stopped the player from looking around */
		s := here@p_rDesc;
		if s = "" then
		    Print("You see nothing special here.\n");
		else
		    Print(s + "\n");
		fi;
		ignore showList(here@p_rContents, "Nearby:\n");
		true
	    else
		call(a, status)() ~= fail
	    fi
	fi
    elif DirMatch(what) then
	Print("You can't look in specific directions. Just look at the "
	    "named things you see, or move in the directions of interest.\n");
	true
    else
	/* player is examining an object */
	name := FormatName(what);
	st := FindName(Me()@p_pCarrying, p_oName, what);
	if st = fail then
	    st := FindName(here@p_rContents, p_oName, what);
	fi;
	if st = fail then
	    Print(IsAre("There", "no", name,
		"here, or there is nothing of interest about it.\n"));
	    true
	elif st = continue then
	    Print(name + " is ambiguous here.\n");
	    false
	else
	    object := FindResult();
	    a := object@p_oLookAction;
	    if a = nil then
		s := object@p_oDesc;
		if s = "" then
		    Print("You see nothing special about the " + name +
			".\n");
		else
		    Print(s);
		fi;
		true
	    else
		call(a, status)(object) ~= fail
	    fi
	fi
    fi
corp;

private proc v_inventory()bool:

    if showList(Me()@p_pCarrying, "You are carrying:\n") then
	Print("You are not carrying anything.\n");
    fi;
    ignore showList(Me()@p_pSpells, "Active spells:\n");
    true
corp;

private proc v_get(string name)bool:
    string whatName;
    thing me, here, what;
    action a;
    status st;

    if name = "" then
	Print("Get what?\n");
	false
    elif name == "inventory" then
	v_inventory()
    else
	me := Me();
	here := Here();
	whatName := FormatName(name);
	st := FindName(here@p_rContents, p_oName, name);
	if st = continue then
	    Print(whatName + " is ambiguous here.\n");
	    false
	elif st = succeed then
	    what := FindResult();
	    if not what@p_oCarryable then
		Print("The " + whatName + " cannot be taken.\n");
		false
	    else
		a := what@p_oGetAction;
		if a ~= nil then
		    st := call(a, status)(what);
		else
		    st := continue;
		fi;
		if st = continue then
		    DelElement(here@p_rContents, what);
		    AddTail(me@p_pCarrying, what);
		    Print(whatName + ": taken.\n");
		    true
		else
		    st ~= fail
		fi
	    fi
	else
	    if FindName(me@p_pCarrying, p_oName, name) ~= fail then
		Print("You are already carrying the " + whatName + ".\n");
		false
	    else
		Print(IsAre("There", "no", whatName, "here.\n"));
		false
	    fi
	fi
    fi
corp;

private proc v_drop(string name)bool:
    action a;
    thing me, what;
    status st;

    me := Me();
    name := FormatName(name);
    st := FindName(me@p_pCarrying, p_oName, name);
    if st = continue then
	Print(name + " is ambiguous.\n");
	false
    elif st = succeed then
	what := FindResult();
	a := what@p_oDropAction;
	if a ~= nil then
	    st := call(a, status)(what);
	else
	    st := continue;
	fi;
	if st = continue then
	    DelElement(me@p_pCarrying, what);
	    AddTail(Here()@p_rContents, what);
	    Print(name + ": dropped.\n");
	    true
	else
	    st ~= fail
	fi
    else
	Print(AAn("You are not carrying", name) + ".\n");
	false
    fi
corp;

private proc v_read(string what)bool:
    thing object;
    string name, text;
    status st;

    if what = "" then
	Print("Read what?\n");
	false
    else
	name := FormatName(what);
	st := FindName(Me()@p_pCarrying, p_oName, what);
	if st = continue then
	    Print(name + " is ambiguous.\n");
	    false
	elif st = fail then
	    st := FindName(Here()@p_rContents, p_oName, what);
	    if st = fail then
		Print(IsAre("There", "no", name, "here.\n"));
		false
	    elif st = continue then
		Print(name + " is ambiguous here.\n");
		false
	    else
		object := FindResult();
		text := object@p_oFixedText;
		if text = "" then
		    text := object@p_oText;
		    if text = "" then
			Print("There is nothing to read on the " + name +
			    ".\n");
		    else
			Print("You could read " + name +
			    " if you were holding it.\n");
		    fi;
		    false
		else
		    Print(text);
		    true
		fi
	    fi
	else
	    object := FindResult();
	    text := object@p_oText;
	    if text = "" then
		Print(AAn("You cannot read", name) + ".\n");
		false
	    else
		Print(text);
		true
	    fi
	fi
    fi
corp;

private o_kludgeJar CreateThing(nil)$
o_kludgeJar@p_oName := "jar"$
o_kludgeJar@p_oDesc := "You must specify a specific jar.\n"$
o_kludgeJar@p_oText := "You must read a specific jar.\n"$
private proc kludgeJarDrop(thing jar)status:
    Print("You must specify a specific jar.\n");
    fail
corp;
o_kludgeJar@p_oDropAction := kludgeJarDrop$

private o_kludgeBook CreateThing(nil)$
o_kludgeBook@p_oName := "book"$
o_kludgeBook@p_oDesc := "You must specify a specific book.\n"$
o_kludgeBook@p_oText := "You must read a specific book.\n"$
private proc kludgeBookDrop(thing book)status:
    Print("You must specify a specific book.\n");
    fail
corp;
o_kludgeBook@p_oDropAction := kludgeBookDrop$

private proc dropAJar()void:
    thing me;
    int count;

    me := Me();
    count := me@p_pJarCount;
    count := count - 1;
    me@p_pJarCount := count;
    if count = 0 then
	DelElement(me@p_pCarrying, o_kludgeJar);
    fi;
corp;

private proc dropABook()void:
    thing me;
    int count;

    me := Me();
    count := me@p_pBookCount;
    count := count - 1;
    me@p_pBookCount := count;
    if count = 0 then
	DelElement(me@p_pCarrying, o_kludgeBook);
    fi;
corp;

private proc v_replace(string what)bool:
    thing object, home;
    string name;
    status st;

    if what = "" then
	Print("Replace what?\n");
	false
    else
	name := FormatName(what);
	st := FindName(Me()@p_pCarrying, p_oName, what);
	if st = fail then
	    Print(AAn("You are not carrying", name) + ".\n");
	    false
	elif st = continue then
	    Print(name + " is ambiguous.\n");
	    false
	else
	    object := FindResult();
	    home := object@p_oReplaceLoc;
	    if home = nil then
		if object = o_kludgeBook then
		    Print("You must specify a specific book.\n");
		elif object = o_kludgeJar then
		    Print("You must specify a specific jar.\n");
		else
		    Print(AAn("You cannot replace", name) + ".\n");
		fi;
		false
	    elif home ~= Here() then
		Print(name + " does not belong here.\n");
		false
	    else
		/* avoid problems with 'jar' */
		name := FormatName(object@p_oName);
		home := Me();
		object@p_oVisible := false;
		DelElement(home@p_pCarrying, object);
		AddTail(Here()@p_rContents, object);
		Print("You replace ");
		if object@p_oJar then
		    Print("the " + name);
		    dropAJar();
		else
		    Print(name);
		    dropABook();
		fi;
		Print(" in its place on the shelf.\n");
		if object@p_oEncyclopedia then
		    if home@p_pBookCombination = 2 then
			Print("The high bookcase slides back into place, "
				"closing off the doorway to the east.\n");
			home@p_pBookCombination := 0;
		    fi;
		fi;
		true
	    fi
	fi
    fi
corp;

/* some routines to help us build a little world */

private proc setupRoom(thing room; string name, desc)void:
    if name ~= "" then
	room@p_rName := name;
    fi;
    if desc ~= "" then
	room@p_rDesc := desc;
    fi;
    room@p_rContents := CreateThingList();
    room@p_rVisited := false;
corp;

private proc darkRoom(thing room; string name, desc)void:
    if name ~= "" then
	room@p_rName := name;
    fi;
    if desc ~= "" then
	room@p_rDesc := desc;
    fi;
    room@p_rContents := CreateThingList();
    room@p_rVisited := false;
    room@p_rDark := true;
corp;

private proc setupObject(thing object, where; string name, desc)void:
    object@p_oName := name;
    if desc ~= "" then
	object@p_oDesc := desc;
    fi;
    if where ~= nil then
	AddTail(where@p_rContents, object);
    fi;
corp;

private proc scenery(thing where; string name)void:
    thing item;

    item := CreateThing(nil);
    item@p_oName := name;
    AddTail(where@p_rContents, item);
corp;

private proc sceneryDesc(thing where; string name, desc)void:
    thing item;

    item := CreateThing(nil);
    item@p_oName := name;
    item@p_oDesc := desc;
    AddTail(where@p_rContents, item);
corp;

private proc fullObject(thing object)void:
    object@p_oCarryable := true;
    object@p_oVisible := true;
corp;

private proc uniconnect(thing r1, r2; property thing prp)void:
    r1@prp := r2;
corp;

private proc biconnect(thing r1, r2; property thing p1, p2)void:
    r1@p1 := r2;
    r2@p2 := r1;
corp;
