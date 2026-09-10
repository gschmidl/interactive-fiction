#!/usr/bin/env python3
"""
build_ground_dat.py — emit GROUND.DAT for UNDERGROUND (Gary Kleppe, V2, 1980).

GROUND.DAT is the initial-state file. It has the SAME layout as a saved game,
which UNDERG.BAS writes at line 5000:

    DIM #3%, M9%(107%,15%), O9%(65%), O8%(65%), X%(5%)
    ...
    MAT O8%=O1% : MAT O9%=O% : MAT M9%=M%

so the file is just those four BASIC-PLUS integer virtual arrays, in that order.

WHAT THIS SCRIPT DOES
  Emits a structurally correct file with every value the surviving source and
  text file actually pin down, and 0 everywhere else. It is NOT a playable
  game: the room-to-room movement table M(x,1..12) is not in any scan and is
  left empty. See GROUND-DAT-NOTES.md. Nothing here is invented — if a value
  could not be derived it is left at 0 and listed as unknown by --report.

Usage:
    python build_ground_dat.py            # writes GROUND.DAT + prints a report
    python build_ground_dat.py --report   # report only, no file written
"""
import sys, struct

BLOCK = 512
WORD  = 2

# ---------------------------------------------------------------- layout ---
# BASIC-PLUS integer virtual arrays: 2 bytes/element, little-endian (PDP-11),
# and each array in a DIM # list starts on a block boundary.
# A 2-D array DIM A(m,n) is stored row-major: A(i,j) at element i*(n+1)+j.
ARRAYS = [
    ("M9", (107, 15)),   # 108 x 16 = 1728 elements = 3456 bytes -> 7 blocks
    ("O9", (65,)),       #       66 elements =  132 bytes -> 1 block
    ("O8", (65,)),       #       66 elements =  132 bytes -> 1 block
    ("X",  (5,)),        #        6 elements =   12 bytes -> 1 block
]

def n_elements(dims):
    n = 1
    for d in dims:
        n *= (d + 1)
    return n

def layout():
    off, out = 0, {}
    for name, dims in ARRAYS:
        out[name] = (off, dims, n_elements(dims))
        nbytes = n_elements(dims) * WORD
        blocks = (nbytes + BLOCK - 1) // BLOCK
        off += blocks * BLOCK
    return out, off

# ------------------------------------------------------------ known data ---
# M%(room, col): col 0 = seen flag, 1..12 = movement, 13/14 = long desc first
# and last text field, 15 = short desc field.  Directions 1..12 come from the
# vocabulary codes 201..212:
DIRS = {1:"N", 2:"E", 3:"W", 4:"S", 5:"UP", 6:"DOWN",
        7:"IN", 8:"OUT", 9:"NE", 10:"NW", 11:"SE", 12:"SW"}

# Movement entries the SOURCE proves, because it toggles them against a
# constant -- a toggle "v = k - v" only works if v is 0 or k.
#   9300  M%(18%,8%)=63%-M%(18%,8%)      computer switch, room 18 OUT
#   9300  M%(30%,3%)=50%-M%(30%,3%)      computer switch, room 30 W
#   9300  M%(33%,7%)=18%-M%(33%,7%)      computer switch, room 33 IN
# The switch starts in ONE of its two states; which one is not recorded, so
# these are listed as constrained-but-unknown rather than written blind.
M_TOGGLES = {(18, 8): (0, 63), (30, 3): (0, 50), (33, 7): (0, 18)}

# Movement entries the game SETS at run time (so their initial value is the
# other state, i.e. 0 = no exit):
#    9100  M%(X0%,4%)=61%-M%(X0%,4%)   lever toggles S exit <-> 61
#    2800  M%(X%,4%)=X0%               entering room 38 rewires its S exit
#   12700  M%(X0%,3%)=37%              room 98 W exit opens to 37
#   15200  M%(X0%,1%)=40%              N exit opens to 40
M_OPENED_IN_PLAY = [(38, 4), (98, 3)]

# Rooms the source names explicitly, with what it tells us about them.
ROOM_FACTS = {
     1: "scores 50 points when first seen (16200)",
     3: "reincarnation room after death (15700 X0%=3%); has the great door (10800)",
    18: "boundary: rooms >18 are dark/underground (800); computer switch target",
    19: "scores 50 points when first seen (16200)",
    24: "one of rooms 24-26, gold-digging area (10200)",
    25: "one of rooms 24-26, gold-digging area (10200)",
    26: "one of rooms 24-26, gold-digging area (10200)",
    27: "drowning room if bottle emptied here (10100)",
    30: "computer switch target (W exit)",
    32: "computer room itself (9300)",
    33: "computer switch target (IN exit)",
    37: "door room; leads to room 3 with the key (10900)",
    38: "S exit rewired on entry (2800)",
    39: "reached only with >=380 points (3500)",
    40: "scores 50 points when first seen (16200)",
    41: "destination of 14000",
    54: "lit even though >18 (800)",
    61: "lever-controlled destination (9100)",
    63: "the 2-7-3 / Gnome safe room (4300)",
    68: "space warp entrance -> room 95 (4700)",
    83: "the 'Who?' room (4500)",
    84: "has the great door (10800); destination of 11000",
    88: "green ring room (11700)",
    89: "porcelain room / toilet (5400)",
    90: "room where object 38 works (7400)",
    95: "space warp exit (4700)",
    98: "vault-lever room; W exit opens to 37 (12700)",
    99: "inside the vault (10600); its exits get set to -7 (2800)",
   105: "one of 105/106 - sets O%(49%) (2850)",
   106: "one of 105/106 - sets O%(49%) (2850)",
   108: "pseudo-room: entering it triggers the endgame (2800 -> 16500)",
}

# O1%(obj) = text field describing that object when it lies in a room.
# Values marked (src) are proved by the source; the rest are read off the
# object-description block of UNDERG.TXT (fields 0-46) matched to the object
# names in DATA 174-185.
O1_INITIAL = {
     1: (1,   "src", "gold still buried; becomes 0 when dug (6700/10200)"),
     2: (40,  "txt", "black pearl"),
     3: (9,   "txt", "solid gold record"),
     4: (3,   "txt", "large emerald"),
     5: (7,   "txt", "solid gold garbage can"),
     6: (2,   "txt", "silver hammer"),
     7: (14,  "txt", "coil of platinum wire"),
     8: (16,  "txt", "pile of uranium"),
     9: (17,  "txt", "solid gold telescope"),
    10: (4,   "txt", "ivory chess set"),
    11: (5,   "txt", "silver trophy"),
    12: (33,  "txt", "ancient statue"),
    13: (8,   "txt", "Gnomes' silver"),
    14: (13,  "txt", "gold pet's dish"),
    15: (11,  "txt", "rare jewels"),
    16: (21,  "txt", "rare stamps"),
    17: (6,   "txt", "fortune in money"),
    18: (10,  "txt", "rare coins"),
    19: (12,  "txt", "jewel-encrusted wand"),
    20: (15,  "txt", "golden key"),
    21: (18,  "src", "flashlight off; 24 when lit (9700/9800/1200)"),
    22: (22,  "src", "empty bottle; 23 when full of water (10000/10100)"),
    23: (19,  "txt", "rock"),
    24: (20,  "txt", "laser pistol"),
    25: (61,  "txt", "lump of coal"),
    26: (39,  "txt", "old piece of cheese"),
    27: (38,  "txt", "skeleton's bone"),
    28: (37,  "txt", "small wooden flute"),
    29: (214, "txt", 'book, "Computer Chess"'),
    30: (175, "txt", "hole in the ground"),
    31: (35,  "txt", "pair of sunglasses"),
    32: (26,  "txt", "remote-control box"),
    33: (29,  "txt", "radiation-proof container"),
    34: (30,  "txt", "astronauts' gloves"),
    35: (212, "src", "green ring; 213 when glowing (11600/11700)"),
    36: (211, "txt", "small white pill"),
    37: (34,  "txt", "miniature sun"),
    38: (31,  "txt", "penny"),
    40: (50,  "src", "hungry rat; 51 when dead (13000)"),
    41: (52,  "src", "hungry cat; 225 after (13200)"),
    42: (56,  "src", "oaken chest; 57 when open (10400)"),
    43: (53,  "txt", "old watchdog"),
    44: (54,  "txt", "woman standing (55 when sitting on the chest)"),
    45: (41,  "txt", "Gnome"),
    46: (42,  "txt", "Dwarf"),
    48: (58,  "txt", "savage-looking tiger"),
    49: (215, "txt", "another adventurer"),
    50: (25,  "txt", "robot"),
    51: (32,  "txt", "cart stuck in the bog"),
    52: (27,  "src", "giant computer; 28 when melted (15300)"),
    53: (59,  "src", "vulnerable north wall; 237 after (15200)"),
    54: (44,  "src", "locked safe; 45 when open (4400)"),
    55: (43,  "txt", "message written here"),
    56: (60,  "txt", "sheet of ice"),
    62: (183, "src", "vault closed; 182 when open (12700)"),
    63: (36,  "txt", "lever on the wall"),
    64: (176, "txt", "green globe holding the jewels"),
    65: (216, "txt", "empty barrel fastened to the floor"),
}
# Objects with no derivable description field: 39 (cat's leash), 47 (monster),
# 57 (mist), 58 (field), 59 (lamp), 60 (door), 61 (self), plus 65-range gaps.

# X%(0..5): all flags start at 0 except X%(0), the starting room.
# The RUN transcript opens on the room whose long description is fields
# 184-186 ("You are on a path near a small building..."), so X%(0) is that
# room's number -- which the surviving material does not give.
X_INITIAL = {1: 0, 2: 0, 3: 0, 4: 0, 5: 0}   # X%(0) deliberately absent

# ---------------------------------------------------------------- build ----
def build():
    lay, total = layout()
    buf = bytearray(total)

    def put(arr, idx, val):
        off, dims, n = lay[arr]
        assert 0 <= idx < n, (arr, idx)
        struct.pack_into("<h", buf, off + idx * WORD, val)

    # M9%(room, col) -> element room*16 + col.  Everything starts 0, which is
    # already correct for the seen flags M(x,0) and for "no exit".
    for (room, col), _ in M_TOGGLES.items():
        pass  # start state unknown - deliberately left 0

    # O8% is O1% (object descriptions)
    for obj, (field, _src, _why) in O1_INITIAL.items():
        put("O8", obj, field)

    # O9% is O% (object locations). Not derivable - see notes.
    # X%(0..5)
    for i, v in X_INITIAL.items():
        put("X", i, v)

    return bytes(buf), lay, total

def report(lay, total):
    print("GROUND.DAT layout (BASIC-PLUS integer virtual arrays, little-endian)")
    print("-" * 68)
    for name, dims in ARRAYS:
        off, d, n = lay[name]
        print("  %-4s %-14s %5d elements  offset %5d  block %d"
              % (name, str(dims), n, off, off // BLOCK + 1))
    print("  total %d bytes = %d blocks" % (total, total // BLOCK))
    print()
    print("FILLED IN")
    print("  O1%% (object descriptions): %d of 65 objects" % len(O1_INITIAL))
    print("     %d proved by the source, %d read off the text file"
          % (sum(1 for v in O1_INITIAL.values() if v[1] == "src"),
             sum(1 for v in O1_INITIAL.values() if v[1] == "txt")))
    print("  X%   (flags 1-5 = turns/lamp/deaths/book/laser): all 0, correct")
    print("  M%(x,0) seen flags: all 0, correct")
    print()
    print("NOT FILLED IN - not present in any surviving scan")
    print("  M%(x,1..12)  movement table, 108 rooms x 12 directions = 1296 entries")
    print("               of which the source constrains exactly 3:")
    for (room, col), (a, b) in sorted(M_TOGGLES.items()):
        print("                 M(%d,%d) [%s] is either %d or %d (9300 toggle)"
              % (room, col, DIRS[col], a, b))
    print("  M%(x,13..15) description field indices, 108 rooms x 3")
    print("  O%(x)        object starting locations, 65 objects")
    print("  X%(0)        starting room (its long description is fields 184-186)")
    print("  O1%() for objects 39,47,57,58,59,60,61 - no description field found")
    print()
    print("%d rooms are named by the source; see ROOM_FACTS in this script."
          % len(ROOM_FACTS))

if __name__ == "__main__":
    data, lay, total = build()
    if "--report" not in sys.argv:
        with open("GROUND.DAT", "wb") as f:
            f.write(data)
        print("wrote GROUND.DAT (%d bytes)\n" % len(data))
    report(lay, total)
