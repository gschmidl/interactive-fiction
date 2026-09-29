/*
 * Amiga MUD
 *
 * Copyright (c) 1991 by Chris Gray
 */

/*
 * words - define the words for the world.
 */

/* verbs */

Verb0(G, "north", 0, v_north)$
Verb0(G, "south", 0, v_south)$
Verb0(G, "east", 0, v_east)$
Verb0(G, "west", 0, v_west)$
Verb0(G, "northeast", 0, v_northEast)$
Verb0(G, "northwest", 0, v_northWest)$
Verb0(G, "southeast", 0, v_southEast)$
Verb0(G, "southwest", 0, v_southWest)$
Verb0(G, "up", 0, v_up)$
Verb0(G, "down", 0, v_down)$
Verb0(G, "enter", 0, v_enter)$
Verb0(G, "exit", 0, v_exit)$

Synonym(G, "north", "n")$
Synonym(G, "south", "s")$
Synonym(G, "east", "e")$
Synonym(G, "west", "w")$
Synonym(G, "northeast", "ne")$
Synonym(G, "northwest", "nw")$
Synonym(G, "southeast", "se")$
Synonym(G, "southwest", "sw")$
Synonym(G, "up", "u")$
Synonym(G, "down", "d")$
Synonym(G, "enter", "in")$
Synonym(G, "exit", "out")$
Synonym(G, "exit", "leave")$

Verb1(G, "go", 0, v_go)$
Synonym(G, "go", "walk")$
Synonym(G, "go", "head")$
Synonym(G, "go", "move")$

Verb1(G, "look", Word(G, "at"), v_look)$
Synonym(G, "look", "examine")$
Synonym(G, "look", "l")$

Verb1(G, "pick", FindWord(G, "up"), v_get)$
Synonym(G, "pick", "get")$
Synonym(G, "pick", "take")$
Synonym(G, "pick", "carry")$
Synonym(G, "pick", "g")$

Verb0(G, "inventory", 0, v_inventory)$
Synonym(G, "inventory", "inv")$
Synonym(G, "inventory", "i")$

Verb1(G, "put", FindWord(G, "down"), v_drop)$
Synonym(G, "put", "drop")$

Verb0(G, "quit", 0, v_quit)$
Synonym(G, "quit", "bye")$

Verb1(G, "read", 0, v_read)$
Synonym(G, "read", "r")$

Verb1(G, "replace", 0, v_replace)$

Verb2(G, "add", Word(G, "to"), v_add)$
Synonym(G, "add", "pour")$

Verb1(G, "brew", 0, v_brew)$
Synonym(G, "brew", "mix")$

Verb1(G, "cast", 0, v_cast)$
Synonym(G, "cast", "throw")$
