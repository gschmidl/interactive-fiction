/*
 * Amiga MUD
 *
 * Copyright (c) 1991 by Chris Gray
 */

/*
 * spells - code, etc. for the magic spells.
 */

private o_spellHolder CreateThing(nil)$
o_spellHolder@p_pSpells := CreateThingList()$

private proc v_add(string what, target)bool:
    thing container, object;
    string name;
    int count;
    status st;

    if target = "" then
	target := "cauldron";
    fi;
    if what = "" then
	Print("Add what?\n");
	false
    else
	name := FormatName(target);
	st := FindName(Here()@p_rContents, p_oName, target);
	if st = fail then
	    Print("There is no " + name + " here.\n");
	    false
	elif st = continue then
	    Print(name + " is ambiguous here.\n");
	    false
	else
	    container := FindResult();
	    if not container@p_oCauldron then
		Print(AAn("You can't add things to", name) + ".\n");
		false
	    else
		name := FormatName(what);
		st := FindName(Me()@p_pCarrying, p_oName, what);
		if st = fail then
		    Print("You are not carrying any " + name + ".\n");
		    false
		elif st = continue then
		    Print(name + " is ambiguous.\n");
		    false
		else
		    object := FindResult();
		    if not object@p_oJar then
			if object = o_kludgeJar then
			    Print("You must specify a specific jar.\n");
			else
			    Print("The " + name +
				" is not something you want to "
				"add to the cauldron.\n");
			fi;
			false
		    else
			count := object@p_oPortionCount;
			if count = 0 then
			    Print("There is no " + name + " left!\n");
			    false
			else
			    Print("You add a portion of " + name +
				" to the cauldron.\n");
			    count := count - 1;
			    if count = 0 then
				Print("That's the last of it!\n");
			    fi;
			    object@p_oPortionCount := count;
			    name := container@p_oCurrentMix;
			    name := name + object@p_oMixCode;
			    container@p_oCurrentMix := name;
			    true
			fi
		    fi
		fi
	    fi
	fi
    fi
corp;

private proc v_brew(string what)bool:
    string mix;
    thing cauldron, model, spell;
    status st;

    if what = "" or MatchName("mess;vile,looking,vile-looking", what) = 0 then
	what := "cauldron";
    fi;
    mix := FormatName(what);
    st := FindName(Here()@p_rContents, p_oName, what);
    if st = fail then
	Print("There is no " + mix + " here.\n");
	false
    elif st = continue then
	Print(mix + " is ambiguous here.\n");
	false
    else
	cauldron := FindResult();
	if not cauldron@p_oCauldron then
	    Print(AAn("You can't brew", mix) + ".\n");
	    false
	else
	    mix := cauldron@p_oCurrentMix;
	    if mix = "" then
		Print("There is nothing in the cauldron to brew!\n");
		false
	    else
		cauldron@p_oCurrentMix := "";
		Print("As you stir the mass in the cauldon, it begins to "
			"bubble, with thick, slow-popping bubbles. As each "
			"bubble pops, a horrible stench is released, each one "
			"slightly different. Eventually, the mass settles "
			"down and starts to \"work\". A nacreous glow begins "
			"to emanate from deep within the now pudding-like "
			"mess. The glow brightens, until it is too bright to "
			"look at. Then, without a sound, the glare vanishes, "
			"taking the contents of the cauldron with it! ");
		spell := CreateThing(nil);
		spell@p_oVisible := true;
		st := FindName(o_spellHolder@p_pSpells, p_oCode, mix);
		if st ~= succeed then
		    Print("It isn't entirely clear what you have just "
			    "brewed up. Hopefully, it isn't dangerous!\n");
		    spell@p_oName := mix;
		else
		    model := FindResult();
		    what := model@p_oName;
		    spell@p_oName := what;
		    spell@p_oSpellAction := model@p_oSpellAction;
		    Print("You have successfully brewed a " + what +
			" spell!\n");
		fi;
		AddTail(Me()@p_pSpells, spell);
		true
	    fi
	fi
    fi
corp;

private proc v_cast(string what)bool:
    thing spell, me;
    action a;
    status st;

    if what = "" then
	Print("Cast what?\n");
	false
    else
	me := Me();
	st := FindName(me@p_pSpells, p_oName, what);
	what := FormatName(what);
	if st = fail then
	    Print("You don't have a " + what + " spell ready to cast.\n");
	    false
	elif st = continue then
	    Print("Spell name " + what + " is ambiguous.\n");
	    false
	else
	    spell := FindResult();
	    a := spell@p_oSpellAction;
	    if a = nil then
		Print("The " + what + " spell seems to have no effect.\n");
	    else
		ignore call(a, status)();
		spell -- p_oSpellAction;
	    fi;
	    spell -- p_oName;
	    spell -- p_oVisible;
	    /* should delete the spell thing altogether */
	    DelElement(me@p_pSpells, spell);
	    true
	fi
    fi
corp;

private proc createSpell(string code, name; action effect)void:
    thing spell;

    spell := CreateThing(nil);
    spell@p_oCode := code;
    spell@p_oName := name;
    spell@p_oSpellAction := effect;
    AddTail(o_spellHolder@p_pSpells, spell);
corp;

/* remember the individual letter codes:

    'f' - fillet of fenny snake 	- darkness, hiding
    'e' - eye of newt			- sight, light
    't' - toe of frog			- travel, speed
    'w' - wool of bat			- clothing, hiding
    'u' - tongue of dog 		- sound
    'd' - scale of dragon		- protection, fire
    'w' - tooth of wolf 		- attack, harm
    'r' - root of hemlock		- earth, solidity
    'g' - gall of goat			- pain, harm
    's' - slips of yew			- plants, growth
 */

createSpell("edu", "light",
    proc
	if Me()@p_pLight then
	    Print("The light spell seems to have no effect.\n");
	else
	    Me()@p_pLight := true;
	    Print("There is a soft POOF sound and "
		    "a gentle glow begins to emanate from your body! It is "
		    "enough to light your way through darkness.\n");
	fi;
	continue
    corp)$
