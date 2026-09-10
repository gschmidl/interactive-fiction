# The Monastery -- map
Reconstructed from the `MOV`, `LOCS` and `DOORS` tables of `SAVE.TBA`
and the entry text of `TEXT.GME`.  Room numbers are the values the
program itself uses in `P`.

Directions in `MOV` are indexed 1-10: 1=NORTH, 2=NORTHWEST, 3=NORTHEAST, 4=SOUTH, 5=SOUTHWEST, 6=SOUTHEAST, 7=WEST, 8=EAST, 9=UP, 10=DOWN.

`-1` in the movement table means "blocked, and say nothing"; `0` means
"blocked, print *YOU ARE UNABLE TO TRAVEL IN THAT DIRECTION*" (or, if a
door record covers that wall, *THERE IS A CLOSED DOOR...*).

## Rooms

### Room 1
- text: day 1 / night 2 / brief 3
- YOU HAVE COME UPON AN OLD TWO STORY BRICK BUILDING.THE BUILDING IS
- exits: NORTH -> 2, SOUTH -> 37, SOUTHWEST -> 49, SOUTHEAST -> 38, WEST -> 49
- objects: SWORD, LAMP, BACKPACK, BACKPACK

### Room 2
- text: day 4 / night 4 / brief 5
- THIS IS THE ENTRANCE HALL OF THIS HOLY BUILDING. IT CONTINUES TO
- exits: NORTH -> 3, SOUTH -> 1
- doors: door 1 NORTH wall -> room 2 (closed); door 1 SOUTH wall -> room 2 (closed)

### Room 3
- text: day 6 / night 6 / brief 7
- YOU ARE IN A HALLWAY THAT EXTENDS TO THE NORTH AND SOUTH. THERE
- exits: NORTH -> 6, SOUTH -> 2
- doors: door 2 WEST wall -> room 4 (closed); door 3 EAST wall -> room 20 (closed)

### Room 4
- text: day 8 / night 8 / brief 10
- ASHES AND BITS OF CHARRED PAPER AND WOOD INFORM YOU THAT THIS
- exits: none in MOV
- doors: door 2 EAST wall -> room 3 (closed); door 4 NORTH wall -> room 5 (closed)
- objects: BOOK, HOBBIT

### Room 5
- text: day 11 / night 11 / brief 12
- THIS IS AN ALMOST DIAGONAL SHAPED ROOM.IT IS RATHER SMALL WITH
- exits: WEST -> 8, UP -> 22
- doors: door 4 SOUTH wall -> room 4 (closed); door 5 NORTH wall -> room 9 (SECRET); door 6 EAST wall -> room 6 (closed)

### Room 6
- text: day 25 / night 25 / brief 26
- YOU ARE IN THE MAIN HALL OF THE MONASTERY. IT LEADS BOTH NORTH AND
- exits: NORTH -> 7, SOUTH -> 3
- doors: door 6 WEST wall -> room 5 (closed)

### Room 7
- text: day 15 / night 15 / brief 15
- YOU ARE STANDING BEFORE THE ENTRANCE TO WHAT APPEARS TO BE 
- exits: NORTH -> 16, SOUTH -> 6

### Room 8
- text: day 16 / night 16 / brief 16
- YOU ARE IN A PASSAGE THAT LEADS FURTHER WEST, BUT IS
- exits: EAST -> 5

### Room 9
- text: day 13 / night 13 / brief 14
- THIS APPEARS TO BE A COAT CLOSET. IT IS VERY SMALL AND HAS
- exits: none in MOV
- doors: door 5 SOUTH wall -> room 5 (SECRET); door 7 NORTH wall -> room 14 (SECRET)

### Room 10
- text: day 71 / night 71 / brief 72
- BEHIND THE STATUE YOU DISCOVER A SQUARE ROOM WITH WOODEN
- exits: none in MOV
- objects: ZOMBIE

### Room 11  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV

### Room 12  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV

### Room 13  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV

### Room 14
- text: day 17 / night 17 / brief 18
- YOU HAVE COME UPON A SORT OF PRIVATE TEMPLE OR MEDITATION ROOM.
- exits: none in MOV
- doors: door 7 SOUTH wall -> room 9 (SECRET)
- objects: POUCH (inside TABLE), STICK (inside TABLE), ROCK

### Room 15  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV

### Room 16
- text: day 19 / night 21 / brief 22
- SUNLIGHT FILTERS THROUGH BEUTIFUL STAIN GLASS WINDOWS ON
- exits: SOUTH -> 7
- doors: door 9 WEST wall -> room 17 (LOCKED); door 10 EAST wall -> room 18 (closed)
- objects: GHOUL

### Room 17  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV
- doors: door 9 EAST wall -> room 16 (LOCKED)

### Room 18
- text: day 23 / night 23 / brief 23
- YOU HAVE ENTERED THE KITCHEN.LIKE THE DINING ROOM IT HAS
- exits: none in MOV
- doors: door 10 WEST wall -> room 16 (closed); door 12 EAST wall -> room 19 (SECRET)

### Room 19
- text: day 24 / night 24 / brief 24
- THIS IS A VERY SMALL,VERY BARE CLOSET.VERY NARROW STAIRS
- exits: UP -> 32
- doors: door 12 WEST wall -> room 18 (SECRET)

### Room 20
- text: day 27 / night 27 / brief 27
- YOU ARE IN A SIDE HALLWAY WHICH LEADS TO A DOOR TO THE EAST. 
- exits: none in MOV
- doors: door 3 WEST wall -> room 3 (closed); door 11 EAST wall -> room 21 (closed)

### Room 21
- text: day 28 / night 28 / brief 29
- THIS ROOM IS CERTAINLY A LAVISH BEDROOM. MOST LIKELY IT WAS
- exits: none in MOV
- doors: door 11 WEST wall -> room 20 (closed)

### Room 22
- text: day 32 / night 32 / brief 33
- THE STAIRS RISE UP INTO THE CENTER OF WHAT WAS A GREAT CATHEDRAL.
- exits: DOWN -> 5
- doors: door 13 NORTH wall -> room 23 (closed)

### Room 23
- text: day 34 / night 34 / brief 35
- YOU ARE STANDING IN A PASSAGE OUTSIDE OF THE CATHEDRAL TO YOUR SOUTH.
- exits: WEST -> 24, EAST -> 28
- doors: door 13 SOUTH wall -> room 22 (closed); door 14 NORTH wall -> room 35 (LOCKED)

### Room 24
- text: day 37 / night 37 / brief 37
- THE PASSAGE TURNS HERE GOING SOUTH AND ALSO EAST.THE PASSAGE
- exits: SOUTH -> 25, EAST -> 23

### Room 25
- text: day 38 / night 38 / brief 38
- THE PASSAGE DEAD ENDS HERE. THERE IS A LITTLE DOOR TO THE SOUTH.
- exits: NORTH -> 24
- doors: door 15 SOUTH wall -> room 26 (closed)

### Room 26
- text: day 39 / night 39 / brief 39
- THIS IS A SMALL CLOSET. IT IS BARE AND VERY DUST.
- exits: none in MOV
- doors: door 15 NORTH wall -> room 25 (closed)
- objects: ARMOR

### Room 27
- text: day 40 / night 40 / brief 40
- THIS ROOM IS VERY,VERY HOT. SO HOT THAT YOUR EYES BLUR SLIGHTLY.
- exits: none in MOV
- doors: door 18 NORTH wall -> room 32 (SECRET)

### Room 28
- text: day 36 / night 36 / brief 36
- THE CORRIDOR ENDS HERE.THERE IS A SMALL DOOR TO THE SOUTH.
- exits: WEST -> 23
- doors: door 16 SOUTH wall -> room 29 (closed)

### Room 29
- text: day 56 / night 56 / brief 56
- THIS WAS ONCE SOME SORT OF EXCERCISE OR TRAINING ROOM. THERE ARE
- exits: none in MOV
- doors: door 16 NORTH wall -> room 28 (closed); door 17 EAST wall -> room 30 (closed)

### Room 30
- text: day 39 / night 39 / brief 39
- THIS IS A SMALL CLOSET. IT IS BARE AND VERY DUST.
- exits: none in MOV
- doors: door 17 WEST wall -> room 29 (closed)

### Room 31
- text: day 73 / night 73 / brief 73
- YOU ARE STANDING ON A LEDGE ON THE SECOND STORY OF THE MONASTERY.
- exits: NORTH -> (wall), NORTHWEST -> (wall), NORTHEAST -> (wall), SOUTH -> 33, SOUTHWEST -> (wall), WEST -> (wall), EAST -> 32

### Room 32
- text: day 74 / night 74 / brief 74
- YOU ARE STANDING ON A LEDGE OF THE SECOND STORY OF THIS MONASTERY.
- exits: NORTH -> (wall), NORTHWEST -> (wall), NORTHEAST -> (wall), WEST -> 31, EAST -> (wall), DOWN -> 19
- doors: door 18 SOUTH wall -> room 27 (SECRET)

### Room 33
- text: day 73 / night 73 / brief 73
- YOU ARE STANDING ON A LEDGE ON THE SECOND STORY OF THE MONASTERY.
- exits: NORTH -> 31, NORTHWEST -> (wall), SOUTH -> (wall), SOUTHWEST -> (wall), SOUTHEAST -> (wall), WEST -> (wall), EAST -> 34

### Room 34
- text: day 73 / night 73 / brief 73
- YOU ARE STANDING ON A LEDGE ON THE SECOND STORY OF THE MONASTERY.
- exits: NORTH -> 32, NORTHEAST -> (wall), SOUTH -> (wall), SOUTHWEST -> (wall), SOUTHEAST -> (wall), WEST -> 33, EAST -> (wall)

### Room 35  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV
- doors: door 14 SOUTH wall -> room 23 (LOCKED)

### Room 36  *(no description -- unimplemented)*
- text: day 0 / night 0 / brief 0
- exits: none in MOV

### Room 37
- text: day 75 / night 75 / brief 75
- YOU ARE STANDING BEFORE A VERY SMALL BUILDING. THE DOORS
- exits: NORTH -> 1, NORTHWEST -> 49, NORTHEAST -> 38, SOUTH -> 39, SOUTHEAST -> 40, WEST -> 49, EAST -> 38

### Room 38
- text: day 75 / night 75 / brief 75
- YOU ARE STANDING BEFORE A VERY SMALL BUILDING. THE DOORS
- exits: NORTH -> 1, NORTHWEST -> 1, SOUTH -> 40, SOUTHWEST -> 39, WEST -> 37, EAST -> 40

### Room 39
- text: day 93 / night 90 / brief 90
- THIS IS MORE FLAT GRASSY AREA ON THE BOTTOM OF THE DEPRESSION.
- exits: NORTH -> 37, NORTHWEST -> 50, NORTHEAST -> 38, SOUTH -> 42, SOUTHWEST -> 48, SOUTHEAST -> 41, WEST -> 49, EAST -> 40

### Room 40
- text: day 94 / night 94 / brief 94
- YOU ARE WALKING IN THE GRASS AREA BETWEEN THE MOUNTAIN JUST
- exits: NORTH -> 38, NORTHWEST -> 37, SOUTH -> 41, SOUTHWEST -> 42, WEST -> 39

### Room 41
- text: day 80 / night 80 / brief 81
- AS YOU TRAVEL NORTH OF THE BEACH, THE SAND TURNS TO MOSTLY
- exits: NORTH -> 40, NORTHWEST -> 39, NORTHEAST -> 40, SOUTH -> (wall), SOUTHWEST -> 44, SOUTHEAST -> (wall), WEST -> 43

### Room 42
- text: day 82 / night 82 / brief 83
- THIS IS THE CENTER OF A SMALL GROVE OF PRETTY DOGWOOD
- exits: NORTH -> 39, NORTHWEST -> 49, NORTHEAST -> 40, SOUTH -> 43, SOUTHWEST -> 47, SOUTHEAST -> 44, WEST -> 48, EAST -> 41
- objects: DWARF

### Room 43
- text: day 80 / night 80 / brief 81
- AS YOU TRAVEL NORTH OF THE BEACH, THE SAND TURNS TO MOSTLY
- exits: NORTH -> 42, NORTHWEST -> 48, NORTHEAST -> 41, SOUTH -> (wall), SOUTHWEST -> (wall), SOUTHEAST -> 45, WEST -> 47, EAST -> 44

### Room 44
- text: day 78 / night 78 / brief 79
- YOU ARE ON THE BEACH OF THE RIVER THAT FLOWS ALMOST SOUTHEAST TO
- exits: NORTH -> 41, NORTHWEST -> 42, NORTHEAST -> 41, SOUTH -> 45, SOUTHWEST -> (wall), SOUTHEAST -> (wall), WEST -> 34, EAST -> (wall)

### Room 45
- text: day 76 / night 76 / brief 77
- YOU ARE ON A LANDING DOCK MADE OF WOODEN PLANKS. THIS IS THE NORTH
- exits: NORTH -> 44, NORTHWEST -> 43, NORTHEAST -> 41, SOUTH -> (wall), SOUTHWEST -> (wall), SOUTHEAST -> (wall), WEST -> 43, EAST -> 41

### Room 46
- text: day 86 / night 86 / brief 87
- YOU ARE STANDING ON A HILL WHICH LEADS DOWN INTO A CIRCULAR
- exits: NORTH -> 52, NORTHWEST -> 52, NORTHEAST -> 15

### Room 47
- text: day 88 / night 88 / brief 88
- HERE THE RIVER BEHIND YOU AND THE CLIFFS TO THE LEFT FORM
- exits: NORTH -> 48, NORTHWEST -> 48, NORTHEAST -> 42, SOUTH -> 47, SOUTHEAST -> 43, WEST -> 47, EAST -> 43

### Room 48
- text: day 89 / night 90 / brief 89
- THIS IS FLAT GRASSY AREA NEAR THE TOP OF THE DEPRESSION.TO THE
- exits: NORTH -> 49, NORTHWEST -> 49, NORTHEAST -> 39, SOUTH -> 47, SOUTHWEST -> 47, SOUTHEAST -> 42, WEST -> 48, EAST -> 42

### Room 49
- text: day 89 / night 90 / brief 89
- THIS IS FLAT GRASSY AREA NEAR THE TOP OF THE DEPRESSION.TO THE
- exits: NORTH -> 50, NORTHWEST -> 50, NORTHEAST -> 1, SOUTH -> 48, SOUTHWEST -> 48, SOUTHEAST -> 42, EAST -> 39

### Room 50
- text: day 91 / night 92 / brief 92
- HERE A CUL-DE-SAC IS FORMED BY THE MOUNTAIN TO YOUR RIGHT AND THE
- exits: SOUTH -> 49, SOUTHWEST -> 1, EAST -> 1

## Objects

| # | noun | synonym | start | takeable | monster | type | desc |
|---|------|---------|-------|----------|---------|------|------|
| 1 | ARMOR | * | room 26 | yes | no | armor | 42 |
| 2 | SWORD | BLADE | room 1 | yes | no | sword/belt | 43 |
| 3 | BELL | * | scenery | no | no | - | 0 |
| 4 | POUCH | BAG | room 14 (inside TABLE) | yes | no | pouch | 44 |
| 5 | TABLE | * | scenery | no | no | light source | 0 |
| 6 | STICK | STAFF | room 14 (inside TABLE) | yes | no | 4 | 45 |
| 7 | BOOK | * | room 4 | yes | no | light source | 46 |
| 8 | DOOR | DOORS | nowhere | no | no | - | 0 |
| 9 | LAMP | LANTERN | room 1 | yes | no | light source | 47 |
| 10 | GHOUL | * | room 16 | no | yes | undead | 48 |
| 11 | ASHES | ASH | nowhere | no | no | - | 0 |
| 12 | WALL | WALLS | nowhere | no | no | - | 0 |
| 13 | ZOMBIE | MONSTER | room 10 | no | yes | undead | 57 |
| 14 | ROCK | STONE | room 14 | yes | no | 8 | 61 |
| 15 | BACKPACK | PACK | room 1 | yes | no | backpack / demi-human | 62 |
| 16 | BACKPACK | PACK | room 1 | yes | no | backpack / demi-human | 69 |
| 17 | STATUE | * | scenery | no | no | - | 0 |
| 18 | HAND | HANDS | scenery | no | no | - | 0 |
| 19 | HOBBIT | HALFLING | room 4 | no | yes | backpack / demi-human | 70 |
| 20 | DWARF | * | room 42 | no | yes | backpack / demi-human | 85 |

## Doors

| # | side A | wall | side B | wall | state |
|---|--------|------|--------|------|-------|
| 1 | 2 | NORTH | 2 | SOUTH | closed |
| 2 | 4 | EAST | 3 | WEST | closed |
| 3 | 20 | WEST | 3 | EAST | closed |
| 4 | 4 | NORTH | 5 | SOUTH | closed |
| 5 | 5 | NORTH | 9 | SOUTH | SECRET |
| 6 | 5 | EAST | 6 | WEST | closed |
| 7 | 9 | NORTH | 14 | SOUTH | SECRET |
| 8 | 0 | - | 0 | - | closed |
| 9 | 16 | WEST | 17 | EAST | LOCKED |
| 10 | 16 | EAST | 18 | WEST | closed |
| 11 | 20 | EAST | 21 | WEST | closed |
| 12 | 18 | EAST | 19 | WEST | SECRET |
| 13 | 22 | NORTH | 23 | SOUTH | closed |
| 14 | 23 | NORTH | 35 | SOUTH | LOCKED |
| 15 | 25 | SOUTH | 26 | NORTH | closed |
| 16 | 28 | SOUTH | 29 | NORTH | closed |
| 17 | 29 | EAST | 30 | WEST | closed |
| 18 | 32 | SOUTH | 27 | NORTH | SECRET |
