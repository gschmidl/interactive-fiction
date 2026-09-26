#!/usr/bin/env python3
"""
The Sewers of Piing Poong
-------------------------
A Windows-playable port of a tiny 1980s text adventure originally written
in TECO (sewer.tec) driving a hand-rolled flat-file database (sewer.dat),
recovered from a PDP tape image collection.

This is a reimplementation of the game's logic and data in Python, based
on reverse-engineering the original TECO macros and the caret-delimited
records in sewer.dat. See README.md in this folder for how the format
was decoded. Room text, item text, and special messages are reproduced
verbatim from the original data file.

Run with:  python sewer.py
"""

import sys
import re

# ---------------------------------------------------------------------------
# Data: rooms
#
# Each room has a description, a list of exits, and an initial item list.
# An exit is (direction_letter, gate, dest_room, arrival).
#   gate     : None, or ("room", thing_id) -- thing_id must be in this room
#                      , or ("held", thing_id) -- thing_id must be carried
#   arrival  : None, or ("Y", msg_id) -- show flavor message, then LOOK
#                      , or ("X", msg_id) -- show message, then end game
# A room's `blocked_msg` is the special message shown (instead of the
# generic "You can't move that direction.") when a gated exit's condition
# fails -- this mirrors the original CANT GO routine, which shows whatever
# single "F<n>" message is attached to the room's exit data.
# ---------------------------------------------------------------------------

ROOMS = {
    1: dict(
        desc="You are in a small domed room.  Noxious sludge is flowing out of a pipe\n"
             "in the east wall, through a ditch, and disappears through a vaulted\n"
             "arch in the west wall.",
        exits=[("U", ("room", 2), 1, ("X", 1)),
               ("W", None, 2, None)],
        items=[1],
        blocked_msg=None,
    ),
    2: dict(
        desc="You are in an E/W tunnel, up to your knees in sludge.",
        exits=[("E", None, 1, None), ("W", None, 3, None)],
        items=[], blocked_msg=None,
    ),
    3: dict(
        desc="You are in a cylindrical room.  Sludge is flowing out of a tunnel to\n"
             "the east and used to continue into a tunnel to the west, but now flows\n"
             "out a hole in the wall to the south.  A roaring sound such as a\n"
             "waterfall might make can be heard from the hole.",
        exits=[("S", None, 10, ("Y", 3)), ("D", None, 10, ("Y", 3)),
               ("E", None, 2, None), ("W", None, 4, None)],
        items=[], blocked_msg=None,
    ),
    4: dict(
        desc="You are in a dry E/W tunnel.",
        exits=[("E", None, 3, None), ("W", None, 5, None)],
        items=[], blocked_msg=None,
    ),
    5: dict(
        desc="The tunnel curves north and east here.",
        exits=[("E", None, 4, None), ("N", None, 6, None)],
        items=[], blocked_msg=None,
    ),
    6: dict(
        desc="The original tunnel to the north is blocked by a cave in.  However, you\n"
             "can scramble up the broken rock.  A tunnel exits south.",
        exits=[("N", None, 7, None), ("U", None, 7, None),
               ("D", None, 5, None), ("S", None, 5, None)],
        items=[], blocked_msg=None,
    ),
    7: dict(
        desc="The tunnel opens into a cave.  The mouth of the cave can be seen to the\n"
             "north. The tunnel exits to the south.",
        exits=[("N", None, 8, None), ("S", ("room", 3), 6, None)],
        items=[], blocked_msg=4,
    ),
    8: dict(
        desc="You are in the mouth of the great stone Idol Kiing Koong of the village\n"
             "of Choow Meiin.  Below you can be seen the village.  Behind you to the\n"
             "south is a cave.",
        exits=[("N", None, 9, None), ("D", None, 9, None),
               ("S", ("held", 3), 7, None)],
        items=[], blocked_msg=5,
    ),
    9: dict(
        desc="You are at the base of Kiing Koong.  There are threatening villagers all\n"
             "around you.  Boy, do they look nasty.",
        exits=[("U", None, 8, None), ("S", None, 8, None),
               ("N", None, 1, ("X", 2)), ("W", None, 1, ("X", 2)),
               ("E", None, 1, ("X", 2))],
        items=[3], blocked_msg=None,
    ),
    10: dict(
        desc="You are swimming in a pool of sludge.",
        exits=[("D", None, 16, ("Y", 6)), ("U", None, 11, None),
               ("N", None, 11, None), ("S", None, 11, None),
               ("E", None, 11, None), ("W", None, 11, None)],
        items=[], blocked_msg=None,
    ),
    11: dict(
        desc="You are on a narrow ledge around a pool of greasy brown sludge.\n"
             "The pool is fed by a waterfall of sludge flowing down the north wall.\n"
             "Passages go east and west from here.",
        exits=[("D", None, 10, None), ("E", None, 12, None),
               ("W", None, 14, None), ("N", None, 33, None)],
        items=[], blocked_msg=None,
    ),
    12: dict(
        desc="You are in a dry tunnel that goes west and south.",
        exits=[("W", None, 11, None), ("S", None, 13, None)],
        items=[], blocked_msg=None,
    ),
    13: dict(
        desc="You are in a dry tunnel that goes north and west.",
        exits=[("N", None, 12, None), ("W", None, 20, None)],
        items=[], blocked_msg=None,
    ),
    14: dict(
        desc="You are in a dry tunnel that goes east and south.",
        exits=[("E", None, 11, None), ("S", None, 15, None)],
        items=[], blocked_msg=None,
    ),
    15: dict(
        desc="You are in a dry tunnel that goes north and east.",
        exits=[("N", None, 14, None), ("E", None, 20, None)],
        items=[], blocked_msg=None,
    ),
    16: dict(
        desc="You are swimming in a pipe full of sludge that goes east and west.",
        exits=[("E", None, 10, None), ("W", None, 17, None)],
        items=[], blocked_msg=None,
    ),
    17: dict(
        desc="You are swimming in a pool of sludge.  There is a narrow ledge to the\n"
             "west.",
        exits=[("D", None, 16, ("Y", 6)), ("W", None, 18, None)],
        items=[], blocked_msg=None,
    ),
    18: dict(
        desc="You are on a narrow ledge next to a pool of sludge.  There is a passage\n"
             "to the west.",
        exits=[("E", None, 17, None), ("D", None, 17, None),
               ("W", None, 19, None)],
        items=[], blocked_msg=None,
    ),
    19: dict(
        desc="You are in an ancient catacomb with piles of bones everywhere. A large\n"
             "vaulted passage west is now blocked by a cave-in.  There is a small\n"
             "hole in the east wall.  The skeletons are wearing various pieces of\n"
             "jewelry, armor, etc.",
        exits=[("E", ("room", 5), 18, None)],
        items=[4, 5], blocked_msg=7,
    ),
    20: dict(
        desc="You are in a high vaulted chamber. Passages exit east and west.",
        exits=[("E", None, 13, None), ("W", None, 15, None),
               ("D", ("room", 8), 21, None)],
        items=[7], blocked_msg=8,
    ),
    21: dict(
        desc="You are climbing in a vertical pipe.",
        exits=[("U", None, 20, None), ("D", None, 22, None)],
        items=[], blocked_msg=None,
    ),
    22: dict(
        desc="You are at an up-east-west-south pipe junction.",
        exits=[("U", None, 21, None), ("W", None, 23, None),
               ("S", None, 32, None), ("E", None, 31, None)],
        items=[], blocked_msg=None,
    ),
    23: dict(
        desc="You are at an east-west-south pipe junction.  The way west is blocked\n"
             "by a screen.",
        exits=[("E", None, 22, None), ("S", None, 24, None)],
        items=[], blocked_msg=None,
    ),
    24: dict(
        desc="You are at a north-south-east-west pipe junction.",
        exits=[("N", None, 23, None), ("E", None, 32, None),
               ("S", None, 25, None), ("W", None, 30, None)],
        items=[], blocked_msg=None,
    ),
    25: dict(
        desc="You are in a north-south pipe.",
        exits=[("N", None, 24, None), ("S", None, 26, None)],
        items=[], blocked_msg=None,
    ),
    26: dict(
        desc="You are in a north-south-east pipe junction.",
        exits=[("N", None, 25, None), ("S", None, 27, None),
               ("E", None, 32, None)],
        items=[], blocked_msg=None,
    ),
    27: dict(
        desc="You are in a north-east pipe junction.",
        exits=[("N", None, 26, None), ("E", None, 28, None)],
        items=[6], blocked_msg=None,
    ),
    28: dict(
        desc="You are in an north-east-west pipe junction.",
        exits=[("N", None, 37, None), ("W", None, 27, None),
               ("E", None, 29, None)],
        items=[], blocked_msg=None,
    ),
    29: dict(
        desc="You are in a north-south-west pipe junction.",
        exits=[("W", None, 28, None), ("N", None, 30, None),
               ("S", None, 31, None)],
        items=[], blocked_msg=None,
    ),
    30: dict(
        desc="You are in a north-south-east-west pipe junction.",
        exits=[("S", None, 29, None), ("N", None, 31, None),
               ("W", None, 32, None), ("E", None, 24, None)],
        items=[], blocked_msg=None,
    ),
    31: dict(
        desc="You are in a north-south-west pipe junction.",
        exits=[("S", None, 30, None), ("W", None, 22, None),
               ("N", None, 29, None)],
        items=[], blocked_msg=None,
    ),
    32: dict(
        desc="You are in a north-south-east-west pipe junction.",
        exits=[("N", None, 22, None), ("S", None, 26, None),
               ("W", None, 24, None), ("E", None, 30, None)],
        items=[], blocked_msg=None,
    ),
    33: dict(
        desc="You are covered with slime(yum-yum). There is a roaring 'sludge fall'\n"
             "to the south and a sloping cave to the north.",
        exits=[("S", None, 11, ("Y", 9)), ("N", None, 35, None),
               ("D", None, 35, None)],
        items=[], blocked_msg=None,
    ),
    34: dict(
        desc="There is a roaring 'sludge fall' to the south and a sloping cave to the\n"
             "north.",
        exits=[("S", None, 11, ("Y", 9)), ("U", None, 11, None),
               ("N", None, 35, None), ("D", None, 35, None)],
        items=[], blocked_msg=None,
    ),
    35: dict(
        desc="You are in a chamber with a large, mean looking rat.  There is a\n"
             "passage to the south and an arched doorway to the north through which\n"
             "light is shining.",
        exits=[("S", None, 34, None), ("N", ("room", 6), 36, None)],
        items=[], blocked_msg=10,
    ),
    36: dict(
        desc="You are in a room filled with a heavenly light.  Numberless concourses\n"
             "of angels are singing heavenly hymns of praise on both sides.  As\n"
             "this ethereal music floats about you, you see a velvet pillow upon\n"
             "which rests",
        exits=[("S", None, 35, None)],
        items=[9], blocked_msg=None,
    ),
    37: dict(
        desc="You are in a circular stairway.",
        exits=[("U", None, 38, None), ("D", None, 28, None)],
        items=[], blocked_msg=None,
    ),
    38: dict(
        desc="You are on the south edge of a stream of sludge flowing from east to\n"
             "west.  Behind you to the south is a circular stairway.",
        exits=[("S", None, 37, None), ("N", None, 1, ("Y", 11))],
        items=[], blocked_msg=None,
    ),
}

# ---------------------------------------------------------------------------
# Data: things
#
# `group` links the two states of an openable object (e.g. MANHOLE closed
# <-> open). `requires` is the thing-id you must be holding to open it.
# Only the currently *active* id of a group is visible/interactable.
# ---------------------------------------------------------------------------

THINGS = {
    1: dict(name="MANHOLE", takeable=False, openable=True, requires=9,
            counterpart=2,
            desc="Small patches of light are showing through the holes in a manhole\noverhead."),
    2: dict(name="MANHOLE", takeable=False, openable=False, requires=None,
            counterpart=1,
            desc="Daylight is streaming through a manhole overhead."),
    3: dict(name="GIRL", takeable=True, openable=False, requires=None,
            counterpart=None,
            desc="There is an exceptionally beautiful girl here."),
    4: dict(name="KEYS", takeable=True, openable=False, requires=None,
            counterpart=None,
            desc="There is a set of keys here."),
    5: dict(name="ARMOR", takeable=True, openable=False, requires=None,
            counterpart=None,
            desc="There is a suit of armor here."),
    6: dict(name="CHEESE", takeable=True, openable=False, requires=None,
            counterpart=None,
            desc="There is a lump of moldy cheese here."),
    7: dict(name="HATCH", takeable=False, openable=True, requires=4,
            counterpart=8,
            desc="There is a closed hatch in the floor."),
    8: dict(name="HATCH", takeable=False, openable=False, requires=None,
            counterpart=7,
            desc="There is an open hatch in the floor."),
    9: dict(name="CROWBAR", takeable=True, openable=False, requires=None,
            counterpart=None,
            desc="A brilliantly shining golden crowbar."),
}

# thing-id -> whether this specific record is the one currently in play
ACTIVE = {1: True, 2: False, 3: True, 4: True, 5: True, 6: True, 7: True,
          8: False, 9: True}

MESSAGES = {
    1: "You emerge into the blinding sunlight, greet your wife, kids, dog,\n"
       "brothers, sisters, aunts, cousins, uncles, grandparents, etc. etc.\n"
       "affectionately, and go back to your little mud hut to await the next\n"
       "typhoon.",
    2: "You are immediately devoured by the hungry villagers.",
    3: "You slide down a waterfall of sludge into a pool of slime.",
    4: "As you move toward the exit of the cave, a steel grate suddenly drops\n"
       "from the ceiling, barring further progress.  When you back up, it moves\n"
       "out of the way.",
    5: "The Idol belches smoke and fire from its mouth as you try to enter it.",
    6: "Plug your nose!!!!",
    7: "You can't fit through the hole.",
    8: "The hatch isn't open.",
    9: "You are covered with green slime (yum-yum).",
    10: "The mean looking rat bars your way.",
    11: "You step into the sludge and are swept off of your feet.  The next\n"
        "thing you know, you shoot out of a pipe and smack into the far wall.",
}

INTRO = """You are sitting in your little mud hut in Piing Poong eating your bowl
of rice when all of a sudden a typhoon blows in.  The resulting flood
washes you out of your hut and into *TA DA (ominous music, etc. etc.)*

                    ***The Sewers of Piing Poong***


Your mission, should you decide to accept it (and I certainly hope you
do. The sewers are not the nicest place to live), is to escape from the
sewers and return to your little mud hut and wait for the next typhoon.

Commands are;

\tMOVE <direction>  (NORTH, SOUTH, EAST, WEST, UP, DOWN)
\tTAKE <thing>
\tDROP <thing>
\tLOOK
\tINVENTORY
\tOPEN

Everything can be abbreviated to one letter,  so "M E" is the same
as "MOVE EAST"."""

VERBS = {"MOVE": 1, "TAKE": 2, "DROP": 3, "LOOK": 4, "INVENTORY": 5, "OPEN": 6}
DIRECTIONS = {"NORTH": "N", "SOUTH": "S", "EAST": "E", "WEST": "W", "UP": "U", "DOWN": "D"}


def match_prefix(word, candidates):
    """Case-insensitive, unique-prefix match, as promised by the game's
    own instructions ('Everything can be abbreviated to one letter')."""
    if not word:
        return None
    word = word.upper()
    exact = [c for c in candidates if c == word]
    if exact:
        return exact[0]
    hits = [c for c in candidates if c.startswith(word)]
    if len(hits) == 1:
        return hits[0]
    return None


class Game:
    def __init__(self):
        self.room = 1
        self.inventory = set()
        self.room_items = {rid: list(data["items"]) for rid, data in ROOMS.items()}
        self.active = dict(ACTIVE)
        self.over = False

    # -- helpers -----------------------------------------------------

    def find_by_name(self, word, ids):
        """Match a typed noun against the (already-active) things among `ids`."""
        if not word:
            return None
        word = word.upper()
        candidates = [tid for tid in ids
                      if self.active.get(tid) and THINGS[tid]["name"].startswith(word)]
        candidates = list(dict.fromkeys(candidates))
        return candidates[0] if len(candidates) == 1 else None

    def thing_here(self, word):
        return self.find_by_name(word, self.room_items[self.room])

    def thing_held(self, word):
        return self.find_by_name(word, self.inventory)

    def gate_ok(self, gate):
        if gate is None:
            return True
        kind, tid = gate
        if kind == "room":
            return tid in self.room_items[self.room]
        else:  # "held"
            return tid in self.inventory

    # -- verbs ---------------------------------------------------------

    def do_look(self):
        print(ROOMS[self.room]["desc"])
        for tid in self.room_items[self.room]:
            print(THINGS[tid]["desc"])

    def do_inventory(self):
        if not self.inventory:
            print("You aren't carrying anything.")
            return
        for tid in self.inventory:
            print(THINGS[tid]["desc"])

    def do_move(self, noun):
        direction = match_prefix(noun, DIRECTIONS.keys())
        if not direction:
            print("I don't understand that direction.")
            return
        letter = DIRECTIONS[direction]
        room = ROOMS[self.room]
        exit_ = next((e for e in room["exits"] if e[0] == letter), None)
        if exit_ is None:
            print("There is no way to go that direction.")
            return
        _, gate, dest, arrival = exit_
        if not self.gate_ok(gate):
            if room["blocked_msg"] is not None:
                print(MESSAGES[room["blocked_msg"]])
            else:
                print("You can't move that direction.")
            return
        self.room = dest
        if arrival is None:
            self.do_look()
        else:
            kind, msg_id = arrival
            print(MESSAGES[msg_id])
            if kind == "X":
                self.over = True
            else:
                self.do_look()

    def do_take(self, noun):
        if not noun:
            print("What?")
            return
        if self.thing_held(noun) is not None:
            print("You are carrying it!")
            return
        tid = self.thing_here(noun)
        if tid is None:
            print(f"I see no {noun} here.")
            return
        if not THINGS[tid]["takeable"]:
            print("You can't take it.")
            return
        self.room_items[self.room].remove(tid)
        self.inventory.add(tid)

    def do_drop(self, noun):
        if not noun:
            print("What?")
            return
        tid = self.thing_held(noun)
        if tid is None:
            print("You do not have it.")
            return
        self.inventory.discard(tid)
        self.room_items[self.room].append(tid)

    def do_open(self, noun):
        if not noun:
            print("What?")
            return
        tid = self.thing_here(noun)
        if tid is None:
            tid = self.thing_held(noun)
        if tid is None:
            print(f"I see no {noun} here.")
            return
        info = THINGS[tid]
        if not info["openable"]:
            print("You can't open it.")
            return
        req = info["requires"]
        if req is not None and req not in self.inventory:
            print("You can't open it.")
            return
        # succeed: flip active state, replacing the closed thing with its
        # open counterpart whereever it currently sits (room or inventory)
        cp = info["counterpart"]
        self.active[tid] = False
        self.active[cp] = True
        if tid in self.room_items[self.room]:
            self.room_items[self.room].remove(tid)
        self.inventory.discard(tid)
        if cp not in self.room_items[self.room]:
            self.room_items[self.room].append(cp)
        self.do_look()

    # -- main loop -------------------------------------------------------

    def parse_and_run(self, line):
        words = line.strip().split()
        if not words:
            return
        verb_word = words[0]
        noun_word = words[1] if len(words) > 1 else ""
        verb = match_prefix(verb_word, VERBS.keys())
        if verb_word.upper() in ("QUIT", "EXIT"):
            print("Bye.")
            self.over = True
            return
        if verb is None:
            print("What?")
            return
        vnum = VERBS[verb]
        if vnum == 1:
            self.do_move(noun_word)
        elif vnum == 2:
            self.do_take(noun_word)
        elif vnum == 3:
            self.do_drop(noun_word)
        elif vnum == 4:
            self.do_look()
        elif vnum == 5:
            self.do_inventory()
        elif vnum == 6:
            self.do_open(noun_word)

    def run(self):
        print(INTRO)
        print()
        self.do_look()
        while not self.over:
            try:
                line = input("\n> ")
            except (EOFError, KeyboardInterrupt):
                print("\nBye.")
                break
            print()
            self.parse_and_run(line)


def main():
    Game().run()


if __name__ == "__main__":
    main()
