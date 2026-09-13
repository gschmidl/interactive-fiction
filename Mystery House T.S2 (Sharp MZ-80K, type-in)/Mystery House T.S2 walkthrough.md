# Mystery House T.S2 — walkthrough

MYSTERY HOUSE T.S ｿﾞｸﾍﾝ (sequel), T. Shimoyama, 1983.3.29, ver 1.3, Sharp MZ-80K/K2/C, MZ-1200, MZ-700.
Goal (title screen): 「ｲｴ ﾉ ﾅｶﾆ ｶｸｻﾚﾃｲﾙ ﾀﾞｲﾔ ｦ 3ﾂ ｻｶﾞｼﾀﾞｾ!」 — find the three diamonds hidden in the house,
then leave. The escape screen shows how long you took (the clock starts when the game starts),
so the route below is the shortest one: 48 commands.

## How to type commands

Every command is typed in two steps:

1. At the prompt `]ﾄﾞｳｽﾙ?` (what do you do?) type a VERB in capitals and press **CR** (Enter).
2. If the verb was **GO**, the game asks `]ﾄﾞｺﾍ ｲｸ ???` (where to?). Type **one digit** and press **CR**.
   The digit is one of the numbers drawn on the current screen.

Only GO takes a second input. MOVE, TAKE, OPEN, PUSH, SHAKE and KICK act on the one thing in
the room they can act on, so just type the verb and press CR. In the steps below,
"GO 3" means: type `GO`, CR, type `3`, CR.

Other input rules, from the parser at lines 430-530 and 5210-5214:

- Verbs: `GO MOVE TAKE OPEN PUSH SHAKE KICK HELP LIST`. Anything else, or a seventh
  character, answers `]NO!!`.
- **DEL** erases the last letter. At the GO prompt, DEL after the digit restarts the prompt.
- At the GO prompt you must press CR straight after the single digit. A second digit,
  or CR with no digit, gives `]NO!!`.
- `LIST` prints the nine verbs. `HELP` only says `KEY ｦ ｻｶﾞｾ` (look for keys).
- Line 446 maps the MZ-80K display code that the `H` key returns back to `H`, so H works
  on every listed machine.

## The house

Fifteen screens (variable PA). The numbers are the exits drawn on each screen.

| PA | What you see | Exits |
|----|--------------|-------|
| 1 | Front of the house, three doors 1 2 3 | 1 → 2. 2 → 4 and 3 → 6, but only after you hold the key from screen 3 (`]ｶｷﾞｶﾞ ｶｶｯﾃﾙ`, locked) |
| 2 | Entrance passage: a door and a wall button | 1 → 3, 2 → 1 |
| 3 | Small room with a cabinet, door on the right | 2 → 2. **1 = death** |
| 4 | Main hall with the staircase, exits 1–7 | 1 → 7, 2 → 8, 3 → 9, 4 → 10, 5 → 12, 7 → 1. **6 = up the stairs: escape with three diamonds, death without** |
| 5 | Room with a chest of drawers | 2 → 6. **1 = death** |
| 6 | Back hall with stairs, something on the floor | 2 → 10, 4 → 1. 1 → 5 only after SHAKE. **3 = death** |
| 7 | Room with a floor lamp and a wall button | 2 → 4, 3 → 8. **1 = death** |
| 8 | Room with a locked box | 1 → 7, 2 → 4. **3 = death** |
| 9 | Room with a big table | 2 → 4. 3 → 11 appears after MOVE. **1 = death** |
| 10 | Passage with a wall button (same view as 2) | 2 → 6, 3 → 4. **1 = death** |
| 11 | Room with a locked box and a wall button | 2 → 9. **1 and 3 = death** |
| 12 | Library, bookshelf in the way | 3 → 4. 1 → 13 after MOVE. **2 = death** |
| 13 | Room with a low cabinet | 1 → 12, 2 → 14 |
| 14 | Room with a heap on the floor | 1 → 13. 3 → 15 needs the key from screen 6. **2 = death** |
| 15 | Room with a blank panel | 2 → 14. **1 = death** |

Death is `ｱﾅﾀﾊ ｲｴﾆ ﾀﾍﾞﾗﾚﾃ ｼﾏｲﾏｼﾀ...` (you were eaten by the house) and a restart from the
title, with everything reset.

## The route

| # | Screen | Type | Game says |
|---|--------|------|-----------|
| 1 | 1 | GO 1 | (entrance passage) |
| 2 | 2 | GO 1 | (cabinet room) |
| 3 | 3 | MOVE | `]ｷｰｶﾞ ﾊｲｯﾃﾙ` — the cabinet slides aside, a drawer with a key |
| 4 | 3 | TAKE | key 1 taken (unlocks doors 2 and 3 at the front) |
| 5 | 3 | GO 2 | |
| 6 | 2 | GO 2 | (front of the house) |
| 7 | 1 | GO 3 | `]ﾅﾆｶ ｵﾁﾃﾙ` — something is lying on the floor of the back hall |
| 8 | 6 | TAKE | `]ｶｷﾞﾀﾞ..` — key 2 (opens the door to screen 15) |
| 9 | 6 | GO 2 | (passage 10 — do NOT push the button here) |
| 10 | 10 | GO 3 | (main hall) |
| 11 | 4 | GO 5 | (library) |
| 12 | 12 | MOVE | the bookshelf moves off exit 1 |
| 13 | 12 | GO 1 | |
| 14 | 13 | TAKE | `]ｽﾊﾟﾅｶﾞ ﾊｲｯﾃﾙ` — a spanner in the cabinet |
| 15 | 13 | GO 2 | |
| 16 | 14 | GO 3 | (without key 2 this is `]ﾊｲﾚﾅｲ!`) |
| 17 | 15 | KICK | `]ﾃﾝｼﾞｮｳﾀﾞ` — a ceiling panel on four bolts; because you hold the spanner it continues `]ﾊｽﾞｼﾃﾐﾖｳ` … `]ﾊｺｶﾞ ｱﾙ`, a box |
| 18 | 15 | TAKE | `]ﾀﾞｲﾔ ﾐﾂｹﾀ!!` `]ｱﾄ 2ｺ!!` — **diamond 1** |
| 19 | 15 | GO 2 | |
| 20 | 14 | GO 1 | |
| 21 | 13 | GO 1 | |
| 22 | 12 | GO 3 | (main hall) |
| 23 | 4 | GO 1 | (lamp room) |
| 24 | 7 | PUSH | the wall button; nothing is shown, but a key has dropped |
| 25 | 7 | TAKE | key 3 taken, silently (opens the box on screen 11) |
| 26 | 7 | GO 2 | (main hall) |
| 27 | 4 | GO 3 | (table room) |
| 28 | 9 | MOVE | `]ｱﾅ ｶﾞｱﾙ` — a hole under the table, exit 3 |
| 29 | 9 | GO 3 | |
| 30 | 11 | OPEN | `ﾊｺｶﾞ ﾊｲｯﾃﾙ` — a smaller box inside the box |
| 31 | 11 | TAKE | `]ﾀﾞｲﾔ ﾐﾂｹﾀ!` `]ｱﾄ 1ｺ ﾀﾞｹ!` — **diamond 2** |
| 32 | 11 | GO 2 | |
| 33 | 9 | GO 2 | (main hall) |
| 34 | 4 | GO 4 | (passage 10 — don't push) |
| 35 | 10 | GO 2 | (back hall) |
| 36 | 6 | SHAKE | the corner at the bottom right changes: door 1 is now open |
| 37 | 6 | GO 1 | (chest of drawers) |
| 38 | 5 | TAKE | `]ﾅﾆﾓ ﾊｲｯﾃﾅｲ` — first drawer, empty |
| 39 | 5 | TAKE | `]ﾅﾆﾓ ﾊｲｯﾃﾅｲ` — second drawer, empty |
| 40 | 5 | TAKE | `ｶｷﾞｶﾞ ﾊｲｯﾃﾙ` — key 4 in the third drawer |
| 41 | 5 | GO 2 | |
| 42 | 6 | GO 2 | |
| 43 | 10 | GO 3 | (main hall) |
| 44 | 4 | GO 2 | (box room) |
| 45 | 8 | OPEN | the box opens (needs key 4 and the SHAKE) |
| 46 | 8 | TAKE | `ﾀﾞｲﾄ ﾐﾂｹﾀ!` `ｾﾞﾝﾌﾞ ﾐﾂｹﾀ!` — **diamond 3**, all found |
| 47 | 8 | GO 2 | (main hall) |
| 48 | 4 | GO 6 | `]ﾔｯﾀｧ ﾀﾞｯｼｭﾂ ｾｲｺｳ!!!` — escaped; your time is shown as hh╱mm分ss |

Steps 9-10 and 34-35 can go through the front instead (6 → 4 → 1, then 1 → 2 → 4); same length.

## Things that kill you

- **PUSH** on screens 2, 10 and 11 (the wall button) and on screen 14 after KICK has
  revealed the switch. The only button that does anything useful is the one on screen 7.
- **GO** through the wrong exit: 3-1, 5-1, 6-3, 7-1, 8-3, 9-1, 10-1, 11-1, 11-3, 12-2, 14-2, 15-1.
- **GO 6** in the main hall before the third diamond.

## Order of the puzzles

The order is forced by the code:

- Doors 2 and 3 at the front need the key from screen 3 (RO(3)=3).
- Screen 15 is only enterable while you hold the key from screen 6 and have not yet shaken
  (RO(6)=2).
- Screen 15 gives its diamond only after KICK with the spanner from screen 13.
- The box on screen 11 opens only with the key from screen 7 and gives its diamond only
  once you hold diamond 1.
- SHAKE on screen 6 works only with diamonds 1 and 2, and it is what unlocks screen 5 and,
  together with the screen-5 key, the box on screen 8.
- The stairs (4-6) need diamond 3.

## Quirks in the listing

- Screen 7: after PUSH the key is drawn only when the screen is redrawn, so nothing changes
  on screen. Leave and come back and you get `]ｷｰｶﾞ ｵﾁﾃﾙ` with the key drawn; TAKE works
  either way, and gives no message.
- Screen 7: MOVE removes the floor lamp from the picture and nothing else.
- Screen 11: the TAKE test at line 15400 passes whenever you already hold diamond 1, so the
  second diamond can be taken without OPEN at all (steps 30 and 31 collapse into TAKE).
- Screen 11: if you OPEN the box before diamond 1 it is drawn empty; once you have diamond 1
  the inner box appears on the next redraw and TAKE still works.
- Screen 15: OPEN after KICK repeats the bolt check, useful if you kicked before fetching
  the spanner (`]ﾎﾞﾙﾄﾃﾞ ﾄﾒﾃｱﾙ`, bolted).
- Screen 8 line 12420 says `ﾀﾞｲﾄ` where every other message says `ﾀﾞｲﾔ`; this is in the
  printed listing.
- Screen 15 GO 2 uses GOSUB 400 instead of GOTO 400 (line 19220), so every trip out of
  screen 15 leaves a return address on the BASIC stack. Harmless for one game.
