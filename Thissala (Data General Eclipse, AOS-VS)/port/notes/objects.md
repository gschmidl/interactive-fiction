# THISSALA — objects and where they start

All 211 objects, with the room each is in at the start of a new game. Read out
of the running game: names from `ASSIST:6`, locations from the object array, room
names from the `EXPRESS` sweep in [rooms.md](rooms.md).

Take object numbers from here or from `ASSIST:6`, never from arithmetic on
THISSALA.DB6 string numbers — DB6's stale record tails hide strings, so that
count comes out 17 too high.

The `-g` verbs that take these numbers:

    #where <obj>        where is it now
    #bring <obj>        put it in the current room
    #move <obj> <room>  put it in an absolute room number
    #state <obj> [n]    show or set its state

`ASSIST:6` then a number prints the game's own version, with weight, size,
contents and every state string. The arrays are one word per object, indexed by
the object number: `27EC + o` the room (`0` carried, `-1` not in play, bit 15 set
= inside the object in the low bits), `29EC + o` flags with the state in bits
13-15, `2BEC + o` the string base.

The name shown is the shortest description the object carries, which is usually
its short name. Objects that only have a room-listing line show that line, and a
few slots have no text at all.

Nothing is carried at the start.

## Summary

    211  objects defined
    163  placed in a room
      4  inside another object
     44  never placed at all

## Inside another object

    11   a CHEVY key                         in 85   a veneer box with a slot cut in the 
    17   A large hen, quietly staring at yo  in 209  a small nest
    161  a withdrawl slip                    in 108  an envelope with a rare stamp
    168  a geological hammer                 in 167  Against the wall is a broken display

The nest (209) holds the hen without appearing in the CONTENTS table that
`ASSIST:22` prints, so contents can be set outside it. The post office box (84)
is the reverse: flagged in that table as starting occupied, and empty. See
file-formats.md.

## In LIMBO (location -1)

That is the authors' own word for it. THISSALA.DB1 holds one record per object —
section, room, string base, then thirteen flags — and for these 44 the section
column reads literally `LIMBO` with no room at all:

    object 1     1       34              NNNNNNNNNNNNN12
    object 84    1       32      537     NNNNENGNNJNNN10
    object 18    LIMBO                   503

So limbo is a holding area, not an error, and things do come out of it: running
`ASSIST:28` (set up for end game) moves object 18, the golden egg, into room 43,
the Bank Vault. Most of the rest read as event-spawned too — "There is a newly
formed passage heading northeast" (102) is what the vault explosion opens, "some
rungs give way" (178, 179) is the ladder breaking, "There is a shallow hole in
the gravel" (148) follows digging, and the two VS+EL trains (36, 37) arrive at
the station.

Two groups cannot be waiting for anything. Objects 186-190 and 201-205 carry no
text at all, and 14 and 35 are marked spare — 35's own string is "This space for
rent". And two that do matter look stranded: the small and large combination
scraps (70, 72), leaving only the middle one in the world.

Which of the others a finished game would have reached is not recorded anywhere;
the list below is what is in limbo, not a claim that each is unreachable.

    7    a brass key
    9    a silver key
    12   a golden key
    13   a beautiful bird
    14   (no description)
    18   a golden egg
    35   This space for rent
    36   Sitting at the station is the VS+EL local train
    37   Sitting at the station is the VS+EL express train
    47   a pirate
    50   a dime
    51   a copper penny
    52   a quarter
    53   a strange coin
    70   a small piece of paper
    72   a large piece of paper
    101  To the distant north, in the direction of minor mountain, you can
    102  There is a newly formed passage heading northeast into the side
    105  a crystal wine-glass
    107  Propped up against one wall of the cave is a small point
    110  Parked alone in this part of the lot, is a shiney but old 1968 Chevy
    134  Blocking your access to the station is a high gate with a sign
    146  the lost statue
    147  a wooden board
    148  There is a shallow hole in the gravel
    150  a Havanna Cigar
    153  a golden spyglass
    154  The knob for the door
    156  a thick vine, with one end attached above the center of the chasm
    169  a map
    170  an ivory necklace
    178  Someone apparently attempted to climb up the ladder, for some rungs
    179  Someone apparently attempted to climb down the ladder, for some rungs
    181  In the middle of the room is an open trap door
    186  A door 6
    187  A door 7
    188  A door 8
    189  A door 9
    190  A door 10
    201  Sign 6
    202  Sign 7
    203  Sign 8
    204  Sign 9
    205  Sign 10

## All objects

    obj  name                                            EXPRESS  location

    1    a long rope with knots                          1/34     Inside the hardware store
    2    There is a rope hanging down from the top of th 1/61     Ledge in the clouds
    3    There is a candleholder loosly attached to the  5/37     Emperor's Bedroom
    4    a beautiful oak pendant with a cloud carved on  1/96     Middle of nowhere
    5    There is a stake deeply imbedded in the ground  1/60     Acme of the minor mountain
    6    a bronze lance                                  5/75     Amer-Indian Chamber
    7    a brass key                                     --       not in play
    8    a copper key                                    1/34     Inside the hardware store
    9    a silver key                                    --       not in play
    10   a rusty key                                     4/64     Gully edge above rockslide
    11   a CHEVY key                                     --       in obj 85, a veneer box with a slot cut in t
    12   a golden key                                    --       not in play
    13   a beautiful bird                                --       not in play
    14   (no description)                                --       not in play
    15   some small pieces of bread                      1/63     In the small room
    16   some dried out corn                             1/68     Clearing in the woods
    17   A large hen, quietly staring at you.            --       in obj 209, a small nest
    18   a golden egg                                    --       not in play
    19   a pair of large signal flags                    1/49     Inside the little shack
    20   The drawbridge is in the closed position, formi 1/54     Highest plateau
    21   The drawbridge is in the closed position, formi 1/66     Major mountain ledge
    22   a miner's helmet                                1/34     Inside the hardware store
    23   Against the wall is an open metal grating.      1/64     On the bed
    24   The monument -- imbedded desc.                  1/58     Caldera (west)
    25   A moving walkway heading in from the Stone Ring 3/12     The inner ring (Soil Ring)
    26   A moving walkway heading out toward the Stone R 3/14     The inner ring (Soil Ring)
    27   a pewter tea-set                                2/6      In the large dining hall
    28   the wine rack                                   2/10     North chamber
    29   a can opener                                    1/15     Kitchen
    30   a corkscrew                                     2/11     Pantry
    31   the faceplate                                   1/32     Inside the Old Post Office
    32   Next to the curtains, on each side, are small h 2/13     Library
    33   a small dark brown tablet with white lettering  3/1      The city's periphery (Stone Ring) and the wh
    34   the curtain hooks - imbed. desc.                2/13     Library
    35   This space for rent                             --       not in play
    36   Sitting at the station is the VS+EL local train --       not in play
    37   Sitting at the station is the VS+EL express tra --       not in play
    38   There is a large archway leading in through the 3/17     The inner ring (Soil Ring)
    39   a silver bracelet                               2/44     Stark Beach
    40   Inside the dumbwaiter, you can see the platform 2/11     Pantry
    41   a jewelled scepter                              5/20     Display Room
    42   a platinum shield                               5/10     Treasure Room
    43   a silken robe                                   5/10     Treasure Room
    44   a metal broadaxe                                5/10     Treasure Room
    45   a crystal cube                                  5/10     Treasure Room
    46   The implicit silver cube                        1/25     Silver Cube
    47   a pirate                                        --       not in play
    48   Resting on the tracks is an ore car, large enou 2/50     Ore Car Room
    49   a crystal vase                                  2/57     Supply room
    50   a dime                                          --       not in play
    51   a copper penny                                  --       not in play
    52   a quarter                                       --       not in play
    53   a strange coin                                  --       not in play
    54   a $10 bill                                      1/10     Dirt trail
    55   There is a well lit passage to the east.        5/82     Plain Cavern
    56   cockpit dial.                                   5/81     Machine Cave
    57   a spacesuit                                     5/83     Air Lock
    58   a blank scroll                                  5/32     Scroll Room
    59   aztec mask                                      5/77     Toltec Cavern
    60   a tiny ring.                                    5/69     Ledgge in the Pit
    61   a pearshaped pearl                              5/103    Air Pocket
    62   an obsidian blade.                              5/58     Flint Cave
    63   a titanium machete.                             5/23     Subterranean Jungle Path
    64   laser pistol                                    5/90     Port Engine Cowl
    65   a burned-out flashlight                         5/85     Starboard Side
    66   a magic wooden raft                             2/71     On the rocks
    67   a small golden '500' medallion                  2/140    Milestone Cave
    68   a WWII tommygun                                 5/97     Machine gunner's pit
    69   a piece of yellowed paper                       2/47     Inside Main Station
    70   a small piece of paper                          --       not in play
    71   a medium sized piece of paper                   2/140    Milestone Cave
    72   a large piece of paper                          --       not in play
    73   There are three sandy holes in the wall where c 5/11     Northern Wall
    74   On your right to the east which contains a rece 5/11     Northern Wall
    75   the flask - imbedded desc                       5/11     Northern Wall
    76   the flask - imbedded desc                       5/11     Northern Wall
    77   the flask - imbedded desc                       5/11     Northern Wall
    78   the small hole, imbedded description            6/28     Inside the silver room.
    79   the medium slot - imbedded description          6/28     Inside the silver room.
    80   the large slot - imbedded description           6/28     Inside the silver room.
    81   Bura slot in the wall - imbedded description    1/45     Inside the Bura di change
    82   The slot in the pay phone                       3/90     MacDonald's Alcove
    83   From inside the ore car, you can see that the c 2/51     In the ore car
    84   The post office box is securly closed.          1/32     Inside the Old Post Office
    85   a veneer box with a slot cut in the top         1/39     Inside the old church
    86   a full bottle of Brador beer                    1/37     Inside the restaurant
    87   a full bottle of Grolsch beer                   1/37     Inside the restaurant
    88   a bottle of Beaujolais                          2/10     North chamber
    89   a tuna syrian                                   1/37     Inside the restaurant
    90   the blue button in the ore car -- imbed desc    2/51     In the ore car
    91   the yellow button in the ore car -- imbed desc  2/51     In the ore car
    92   the green button in the signal tower - imbed de 2/142    Inside the VS+EL signal tower
    93   the red button in the signal tower - imbed desc 2/142    Inside the VS+EL signal tower
    94   the white button in the van - imbed desc        1/113    Inside the Chevy Van
    95   the black button in the van - imbed desc        1/113    Inside the Chevy Van
    96   the service button in the bank                  1/42     Inside the local small bank
    97   the "T" button in the riverworld boat           6/29     Inside the little boat
    98   the "S" button in the riverworld boat           6/29     Inside the little boat
    99   The orange button                               2/11     Pantry
    100  the purple button                               2/11     Pantry
    101  To the distant north, in the direction of minor --       not in play
    102  There is a newly formed passage heading northea --       not in play
    103  an interperters guide                           2/15     Hidden room
    104  a silver pyramid                                2/15     Hidden room
    105  a crystal wine-glass                            --       not in play
    106  a mailing label                                 3/1      The city's periphery (Stone Ring) and the wh
    107  Propped up against one wall of the cave is a sm --       not in play
    108  an envelope with a rare stamp                   3/1      The city's periphery (Stone Ring) and the wh
    109  The flute - imbedded in the description.        6/51     Large field
    110  Parked alone in this part of the lot, is a shin --       not in play
    111  Resting in the water against the shore is a sma 6/20     Shore of the lake
    112  Surrounding the stairway down, is a metal cage, 1/42     Inside the local small bank
    113  nasty pirate                                    6/56     Monster home
    114  giant rat                                       6/56     Monster home
    115  hairy spider                                    6/56     Monster home
    116  a chimera                                       6/56     Monster home
    117  a silver eye                                    5/136    Hall of Physics
    118  a replica of the Calypso                        5/134    Hall of Oceanography
    119  a mail shirt                                    5/116    Babalonian War Room
    120  marble pieta                                    5/127    Michaelangelo Room
    121  a Stradavarius                                  5/122    Room of the String Section
    122  a golden trumpet                                5/125    Brass Room
    123  a cloak                                         5/117    Room of Devestation
    124  the Codex                                       5/138    Da Vinci Display Room
    125  an abacus                                       5/139    Hall of Mathematics
    126  Koala bears, and Kukaburas amuse you with their 5/119    Zoo Room
    127  a lemon tort                                    5/137    Rest Area
    128  a scimitar                                      1/53     Smaller plateau
    129  a oval broach                                   5/41     Number 1 Wife's Chamber
    130  a gold amulet                                   5/66     Bone Pit
    131  a speargun                                      5/51     Pool Room
    132  an amazonite band                               5/34     Priests' Chamber
    133  a coral bangle                                  5/29     Prayer Room
    134  Blocking your access to the station is a high g --       not in play
    135  a Martin 5K                                     5/121    Room of Strings
    136  a green emerald                                 5/36     Emperor's Room
    137  a red ruby                                      5/40     Harem Room East
    138  a diamond                                       5/24     Subterranean Field
    139  a moonrock                                      5/133    Astronomy Room
    140  a science magazine                              5/132    Hall of Science.
    141  a bottle of cognac                              5/112    Thissala Room
    142  a gold watch                                    5/136    Hall of Physics
    143  the hole in the wall for the pryramid           2/15     Hidden room
    144  As you scan your surroundings you notice an ins 5/2      Pit Room
    145  a metal shovel                                  2/50     Ore Car Room
    146  the lost statue                                 --       not in play
    147  a wooden board                                  --       not in play
    148  There is a shallow hole in the gravel           --       not in play
    149  a cloudy vessel                                 3/32     Special room
    150  a Havanna Cigar                                 --       not in play
    151  a round cylinder                                2/6      In the large dining hall
    152  a square dowel                                  1/1      Middle of the parking lot
    153  a golden spyglass                               --       not in play
    154  The knob for the door                           --       not in play
    155  Hanging down from the center of the chasm is a  2/66     Rock ledge
    156  a thick vine, with one end attached above the c --       not in play
    157  a walking cane                                  2/58     Living room
    158  an escutcheon                                   2/66     Rock ledge
    159  Upon the wall is what appears to be an impressi 5/31     Exchange room
    160  The light is flashing on and off                2/142    Inside the VS+EL signal tower
    161  a withdrawl slip                                --       in obj 108, an envelope with a rare stamp
    162  The fuse box                                    2/56     Utility room
    163  a fuse                                          2/61     The music room
    164  a metal washer                                  2/61     The music room
    165  a rubber glove                                  2/11     Pantry
    166  a canvas sack                                   4/17     Box canyon
    167  Against the wall is a broken display case       4/76     Inside the store
    168  a geological hammer                             --       in obj 167, Against the wall is a broken dis
    169  a map                                           --       not in play
    170  an ivory necklace                               --       not in play
    171  a voodoo doll                                   2/80     Souvenir Stand
    172  The viewing pond                                2/87     Viewing Pond
    173  a sheet of parchment                            1/16     Living room
    174  The hyyptix                                     3/22     Inside the loquanta
    175  some golden fries                               3/27     Inside MacDonalds
    176  a Big Mac                                       3/27     Inside MacDonalds
    177  a telephone handset dangling by its cord        3/90     MacDonald's Alcove
    178  Someone apparently attempted to climb up the la --       not in play
    179  Someone apparently attempted to climb down the  --       not in play
    180  In the very center of the room is an oriental r 1/16     Living room
    181  In the middle of the room is an open trap door  --       not in play
    182  the strange door in the living room             1/16     Living room
    183  In front of you is a door with a knob sticking  5/100    Museum Entrance
    184  The door is currently open                      1/44     Modern building
    185  The door is currently open                      1/45     Inside the Bura di change
    186  A door 6                                        --       not in play
    187  A door 7                                        --       not in play
    188  A door 8                                        --       not in play
    189  A door 9                                        --       not in play
    190  A door 10                                       --       not in play
    191  an open famulus' notebook                       6/32     The Adytum
    192  a hardbound book                                2/13     Library
    193  Off to one corner, is a large mailbox           6/35     Divagaters Rest
    194  The tree in 4 - 3                               4/3      Shade tree
    195  The pine tree at 4-41                           4/41     Lone Pine Promentory
    196  Sign 1                                          4/15     Sign junction
    197  Sign 2                                          4/37     Big Sign
    198  Sign 3                                          3/10     On the walkway.
    199  Sign 4                                          1/11     Junction in the paths
    200  Sign 5                                          1/10     Dirt trail
    201  Sign 6                                          --       not in play
    202  Sign 7                                          --       not in play
    203  Sign 8                                          --       not in play
    204  Sign 9                                          --       not in play
    205  Sign 10                                         --       not in play
    206  Kneeling in the cave, next to the far northern  6/60     Checkpoint
    207  a sharp stick                                   6/47     Eerie tunnel
    208  a turquoise charm                               1/5      Parking lot (NE)
    209  a small nest                                    1/70     Inside the hen house
    210  a white disk with black writing                 1/3      Parking lot (SE)
    211  a black disk with white writing                 1/4      Parking lot (E)

## Index by name

    (no description)                                  14   --       not in play
    a $10 bill                                        54   1/10     Dirt trail
    a beautiful bird                                  13   --       not in play
    a beautiful oak pendant with a cloud carved on it 4    1/96     Middle of nowhere
    a Big Mac                                         176  3/27     Inside MacDonalds
    a black disk with white writing                   211  1/4      Parking lot (E)
    a blank scroll                                    58   5/32     Scroll Room
    a bottle of Beaujolais                            88   2/10     North chamber
    a bottle of cognac                                141  5/112    Thissala Room
    a brass key                                       7    --       not in play
    a bronze lance                                    6    5/75     Amer-Indian Chamber
    a burned-out flashlight                           65   5/85     Starboard Side
    a can opener                                      29   1/15     Kitchen
    a canvas sack                                     166  4/17     Box canyon
    a CHEVY key                                       11   --       in obj 85, a veneer box with a slo
    a chimera                                         116  6/56     Monster home
    a cloak                                           123  5/117    Room of Devestation
    a cloudy vessel                                   149  3/32     Special room
    a copper key                                      8    1/34     Inside the hardware store
    a copper penny                                    51   --       not in play
    a coral bangle                                    133  5/29     Prayer Room
    a corkscrew                                       30   2/11     Pantry
    a crystal cube                                    45   5/10     Treasure Room
    a crystal vase                                    49   2/57     Supply room
    a crystal wine-glass                              105  --       not in play
    a diamond                                         138  5/24     Subterranean Field
    a dime                                            50   --       not in play
    A door 10                                         190  --       not in play
    A door 6                                          186  --       not in play
    A door 7                                          187  --       not in play
    A door 8                                          188  --       not in play
    A door 9                                          189  --       not in play
    a full bottle of Brador beer                      86   1/37     Inside the restaurant
    a full bottle of Grolsch beer                     87   1/37     Inside the restaurant
    a fuse                                            163  2/61     The music room
    a geological hammer                               168  --       in obj 167, Against the wall is a 
    a gold amulet                                     130  5/66     Bone Pit
    a gold watch                                      142  5/136    Hall of Physics
    a golden egg                                      18   --       not in play
    a golden key                                      12   --       not in play
    a golden spyglass                                 153  --       not in play
    a golden trumpet                                  122  5/125    Brass Room
    a green emerald                                   136  5/36     Emperor's Room
    a hardbound book                                  192  2/13     Library
    a Havanna Cigar                                   150  --       not in play
    a jewelled scepter                                41   5/20     Display Room
    A large hen, quietly staring at you.              17   --       in obj 209, a small nest
    a large piece of paper                            72   --       not in play
    a lemon tort                                      127  5/137    Rest Area
    a long rope with knots                            1    1/34     Inside the hardware store
    a magic wooden raft                               66   2/71     On the rocks
    a mail shirt                                      119  5/116    Babalonian War Room
    a mailing label                                   106  3/1      The city's periphery (Stone Ring) 
    a map                                             169  --       not in play
    a Martin 5K                                       135  5/121    Room of Strings
    a medium sized piece of paper                     71   2/140    Milestone Cave
    a metal broadaxe                                  44   5/10     Treasure Room
    a metal shovel                                    145  2/50     Ore Car Room
    a metal washer                                    164  2/61     The music room
    a miner's helmet                                  22   1/34     Inside the hardware store
    a moonrock                                        139  5/133    Astronomy Room
    A moving walkway heading in from the Stone Ring i 25   3/12     The inner ring (Soil Ring)
    A moving walkway heading out toward the Stone Rin 26   3/14     The inner ring (Soil Ring)
    a oval broach                                     129  5/41     Number 1 Wife's Chamber
    a pair of large signal flags                      19   1/49     Inside the little shack
    a pearshaped pearl                                61   5/103    Air Pocket
    a pewter tea-set                                  27   2/6      In the large dining hall
    a piece of yellowed paper                         69   2/47     Inside Main Station
    a pirate                                          47   --       not in play
    a platinum shield                                 42   5/10     Treasure Room
    a quarter                                         52   --       not in play
    a red ruby                                        137  5/40     Harem Room East
    a replica of the Calypso                          118  5/134    Hall of Oceanography
    a round cylinder                                  151  2/6      In the large dining hall
    a rubber glove                                    165  2/11     Pantry
    a rusty key                                       10   4/64     Gully edge above rockslide
    a science magazine                                140  5/132    Hall of Science.
    a scimitar                                        128  1/53     Smaller plateau
    a sharp stick                                     207  6/47     Eerie tunnel
    a sheet of parchment                              173  1/16     Living room
    a silken robe                                     43   5/10     Treasure Room
    a silver bracelet                                 39   2/44     Stark Beach
    a silver eye                                      117  5/136    Hall of Physics
    a silver key                                      9    --       not in play
    a silver pyramid                                  104  2/15     Hidden room
    a small dark brown tablet with white lettering    33   3/1      The city's periphery (Stone Ring) 
    a small golden '500' medallion                    67   2/140    Milestone Cave
    a small nest                                      209  1/70     Inside the hen house
    a small piece of paper                            70   --       not in play
    a spacesuit                                       57   5/83     Air Lock
    a speargun                                        131  5/51     Pool Room
    a square dowel                                    152  1/1      Middle of the parking lot
    a Stradavarius                                    121  5/122    Room of the String Section
    a strange coin                                    53   --       not in play
    a telephone handset dangling by its cord          177  3/90     MacDonald's Alcove
    a thick vine, with one end attached above the cen 156  --       not in play
    a tiny ring.                                      60   5/69     Ledgge in the Pit
    a titanium machete.                               63   5/23     Subterranean Jungle Path
    a tuna syrian                                     89   1/37     Inside the restaurant
    a turquoise charm                                 208  1/5      Parking lot (NE)
    a veneer box with a slot cut in the top           85   1/39     Inside the old church
    a voodoo doll                                     171  2/80     Souvenir Stand
    a walking cane                                    157  2/58     Living room
    a white disk with black writing                   210  1/3      Parking lot (SE)
    a withdrawl slip                                  161  --       in obj 108, an envelope with a rar
    a wooden board                                    147  --       not in play
    a WWII tommygun                                   68   5/97     Machine gunner's pit
    Against the wall is a broken display case         167  4/76     Inside the store
    Against the wall is an open metal grating.        23   1/64     On the bed
    an abacus                                         125  5/139    Hall of Mathematics
    an amazonite band                                 132  5/34     Priests' Chamber
    an envelope with a rare stamp                     108  3/1      The city's periphery (Stone Ring) 
    an escutcheon                                     158  2/66     Rock ledge
    an interperters guide                             103  2/15     Hidden room
    an ivory necklace                                 170  --       not in play
    an obsidian blade.                                62   5/58     Flint Cave
    an open famulus' notebook                         191  6/32     The Adytum
    As you scan your surroundings you notice an inscr 144  5/2      Pit Room
    aztec mask                                        59   5/77     Toltec Cavern
    Blocking your access to the station is a high gat 134  --       not in play
    Bura slot in the wall - imbedded description      81   1/45     Inside the Bura di change
    cockpit dial.                                     56   5/81     Machine Cave
    From inside the ore car, you can see that the car 83   2/51     In the ore car
    giant rat                                         114  6/56     Monster home
    hairy spider                                      115  6/56     Monster home
    Hanging down from the center of the chasm is a th 155  2/66     Rock ledge
    In front of you is a door with a knob sticking ou 183  5/100    Museum Entrance
    In the middle of the room is an open trap door    181  --       not in play
    In the very center of the room is an oriental rug 180  1/16     Living room
    Inside the dumbwaiter, you can see the platform t 40   2/11     Pantry
    Kneeling in the cave, next to the far northern ex 206  6/60     Checkpoint
    Koala bears, and Kukaburas amuse you with their a 126  5/119    Zoo Room
    laser pistol                                      64   5/90     Port Engine Cowl
    marble pieta                                      120  5/127    Michaelangelo Room
    nasty pirate                                      113  6/56     Monster home
    Next to the curtains, on each side, are small hoo 32   2/13     Library
    Off to one corner, is a large mailbox             193  6/35     Divagaters Rest
    On your right to the east which contains a recess 74   5/11     Northern Wall
    Parked alone in this part of the lot, is a shiney 110  --       not in play
    Propped up against one wall of the cave is a smal 107  --       not in play
    Resting in the water against the shore is a small 111  6/20     Shore of the lake
    Resting on the tracks is an ore car, large enough 48   2/50     Ore Car Room
    Sign 1                                            196  4/15     Sign junction
    Sign 10                                           205  --       not in play
    Sign 2                                            197  4/37     Big Sign
    Sign 3                                            198  3/10     On the walkway.
    Sign 4                                            199  1/11     Junction in the paths
    Sign 5                                            200  1/10     Dirt trail
    Sign 6                                            201  --       not in play
    Sign 7                                            202  --       not in play
    Sign 8                                            203  --       not in play
    Sign 9                                            204  --       not in play
    Sitting at the station is the VS+EL express train 37   --       not in play
    Sitting at the station is the VS+EL local train   36   --       not in play
    some dried out corn                               16   1/68     Clearing in the woods
    some golden fries                                 175  3/27     Inside MacDonalds
    some small pieces of bread                        15   1/63     In the small room
    Someone apparently attempted to climb down the la 179  --       not in play
    Someone apparently attempted to climb up the ladd 178  --       not in play
    Surrounding the stairway down, is a metal cage, w 112  1/42     Inside the local small bank
    the "S" button in the riverworld boat             98   6/29     Inside the little boat
    the "T" button in the riverworld boat             97   6/29     Inside the little boat
    the black button in the van - imbed desc          95   1/113    Inside the Chevy Van
    the blue button in the ore car -- imbed desc      90   2/51     In the ore car
    the Codex                                         124  5/138    Da Vinci Display Room
    the curtain hooks - imbed. desc.                  34   2/13     Library
    The door is currently open                        184  1/44     Modern building
    The door is currently open                        185  1/45     Inside the Bura di change
    The drawbridge is in the closed position, forming 20   1/54     Highest plateau
    The drawbridge is in the closed position, forming 21   1/66     Major mountain ledge
    the faceplate                                     31   1/32     Inside the Old Post Office
    the flask - imbedded desc                         75   5/11     Northern Wall
    the flask - imbedded desc                         76   5/11     Northern Wall
    the flask - imbedded desc                         77   5/11     Northern Wall
    The flute - imbedded in the description.          109  6/51     Large field
    The fuse box                                      162  2/56     Utility room
    the green button in the signal tower - imbed desc 92   2/142    Inside the VS+EL signal tower
    the hole in the wall for the pryramid             143  2/15     Hidden room
    The hyyptix                                       174  3/22     Inside the loquanta
    The implicit silver cube                          46   1/25     Silver Cube
    The knob for the door                             154  --       not in play
    the large slot - imbedded description             80   6/28     Inside the silver room.
    The light is flashing on and off                  160  2/142    Inside the VS+EL signal tower
    the lost statue                                   146  --       not in play
    the medium slot - imbedded description            79   6/28     Inside the silver room.
    The monument -- imbedded desc.                    24   1/58     Caldera (west)
    The orange button                                 99   2/11     Pantry
    The pine tree at 4-41                             195  4/41     Lone Pine Promentory
    The post office box is securly closed.            84   1/32     Inside the Old Post Office
    the purple button                                 100  2/11     Pantry
    the red button in the signal tower - imbed desc   93   2/142    Inside the VS+EL signal tower
    the service button in the bank                    96   1/42     Inside the local small bank
    The slot in the pay phone                         82   3/90     MacDonald's Alcove
    the small hole, imbedded description              78   6/28     Inside the silver room.
    the strange door in the living room               182  1/16     Living room
    The tree in 4 - 3                                 194  4/3      Shade tree
    The viewing pond                                  172  2/87     Viewing Pond
    the white button in the van - imbed desc          94   1/113    Inside the Chevy Van
    the wine rack                                     28   2/10     North chamber
    the yellow button in the ore car -- imbed desc    91   2/51     In the ore car
    There are three sandy holes in the wall where con 73   5/11     Northern Wall
    There is a candleholder loosly attached to the wa 3    5/37     Emperor's Bedroom
    There is a large archway leading in through the w 38   3/17     The inner ring (Soil Ring)
    There is a newly formed passage heading northeast 102  --       not in play
    There is a rope hanging down from the top of the  2    1/61     Ledge in the clouds
    There is a shallow hole in the gravel             148  --       not in play
    There is a stake deeply imbedded in the ground he 5    1/60     Acme of the minor mountain
    There is a well lit passage to the east.          55   5/82     Plain Cavern
    This space for rent                               35   --       not in play
    To the distant north, in the direction of minor m 101  --       not in play
    Upon the wall is what appears to be an impression 159  5/31     Exchange room
