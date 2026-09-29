# Changes from the 1980 listings

This directory holds the working copies. The transcriptions in [`../artifacts/Multics/`](../artifacts/Multics/)
stay exactly as printed. Every change to `adventure_.fortran` is marked in the source with a comment beginning
`c  2026:`. To see all the changes:

```sh
diff ../artifacts/Multics/adventure_.fortran adventure_.fortran
diff ../artifacts/Multics/adventure.data adventure.data
```

## Jim's unfinished enhancement: TAKE ALL / DROP ALL

Jim's 1980 ink notes on the listing sketch an `all` object word, `objcount`, and new code at 8010, 9010 and
9020 (see [`adventure_.fortran-handwritten-notes.md`](../artifacts/Multics/adventure_.fortran-handwritten-notes.md)).
They are finished here, using his variable names and statement labels:

- **Declarations.** `objcount` and `all` are added to `/msccom/`, and `all=vocab(code1('all  '),1)` is looked up
  as written in the margin.
- **Database.** A new vocabulary entry `1099 all` makes "all" an object word. Object 99 was unused.
- **TAKE ALL (9031–9034).** Each loose object here is sent through the ordinary carry code, so all the special
  cases still apply:
  - the bird still needs the cage and flees from the rod;
  - the sword still won't come out of the stone before the mushroom;
  - the sceptre still comes from the skeleton.
  Each object taken is listed as "brass lantern: taken." If the load gets full, message 92 ends the list. If
  nothing can be taken, the game says so with new message 272.
- **DROP ALL (9022–9036).** Each object carried goes through the ordinary drop code, so:
  - the vase still breaks unless the pillow is there;
  - the coins still buy batteries at the vending machine;
  - the bear still scares the troll;
  - the flask still lands in the pentagram.
  Plain drops are listed as "set of keys: dropped." With nothing carried, the game prints message 98.
- **Object names.** A new subroutine `objnam` prints an object's name from the first line of its inventory
  message. The notes wrote "dcode1" here. Vocabulary words would have been cut to five letters ("plati"), so the
  inventory message is used instead.
- **TAKE with no object (8010).** Jim's rewrite counts the objects here. `objcount` is now zeroed first, and
  `at()` replaces `here()` so that things already carried don't count. Palter's dwarf test is kept.
- **Other verbs.** Any verb other than take or drop given "all" gets that verb's usual refusal.

## Bugs fixed in `adventure_.fortran`

| Where | Problem | Fix |
|---|---|---|
| 2000 (describe location) | Platt's hazards (slime, ogre, minotaur's wall, basilisks, goblins) and the fog glow were placed after the dark test, so they only happened in the dark. Jim marked this "possible problem" in 1980. | The hazards come first. The glow is printed after the room description, and only when the room is lit. |
| 2000, goblins | The goblins killed without printing their last message (248). | The message is printed, then the goblins kill. |
| 2000, yacht | The yacht's prop was set to 1 on arrival, before it had ever been seen. It was never tallied as found, so the cave could never close. | Set prop to 1 only once it has been seen at prop 0. |
| 20000 (score) | The sceptre and yacht sit at prop 1, so they never scored as deposited. | Accept prop 1 for those two. |
| 9221 (take sceptre) | Printed the skeleton's message and stopped; the sceptre was never picked up. | Falls through to the normal carry code. Taken unseen in the dark, it is still tallied as found. |
| 2047 (open flask / pentagram) | "Already open" (267) was overwritten by 268. The djinn could be released without being trapped. Opening the pentagram did not check for a trapped djinn. LOCK FLASK opened it too. | Rewritten: 267 for an already-open flask; 266 (djinn trapped) only if the flask was left in the pentagram, otherwise 268; 270 only with a trapped djinn. LOCK gets its usual reply. Dropping the sealed flask in the pentagram now says so (265). Taking it out resets it. |
| 9179 (vial) | `ran(4)+214` could pick message 214, which belongs to something else. | `ran(4)+215` (messages 215–218). |
| 8185 (QUIT, answer no) | Fell through into FIND. | Woods' `goto 2012` restored. |
| 4090 (verb with object) | No entries for the new verbs 32–37, so "fast x" or "turns x" stopped the game with bug 24. | Entries added; the object is ignored. |
| 8340 (LISTEN) | The dragon always got the dead-dragon message (210). `here()` missed the dragon from its second location. | Live dragon: new message 271. `at()` is used instead of `here()`. |
| 8360 (PHUGGG) | One dwarf: used a stale loop index `i`. Several dwarves: cleared `dloc` before testing it for `dseen`. Cave-destroying message: tested `i.eq.2`, which is never true. | Loop to find the dwarf; clear `dseen` first; test `spk.eq.240`; zero `dtotal` afterwards, so that a second PHUGGG or LISTEN doesn't find the dead dwarves. |
| 40000 (hints) | Hint 10 (the fog rooms, in the database) had no dispatch entry, so the game stopped with bug 27 after 25 turns in the fog. | Entry 41000 added. |
| 9120 (ATTACK) | With the ogre, goblins, a basilisk or the djinn present, plain ATTACK said "there is nothing here to attack". | They count as targets. The replies are unchanged ("don't be ridiculous!"). |
| `/msccom/` and `cmszes` | The save area ended at `maxdie`, before the Platt variables. `mushturn` was not in the block at all. SUSPEND/RESTORE lost the state of the whole Platt area. | Save through `djinn`, the new last variable; `mushturn` added. |
| `cmszes` | `mtext` was saved to 34 of 35 and `mtdtxt` to 90 of 100. | Whole arrays. |
| After `ldcomn(.true.)` | The image saved in magic mode carries the random seed, so every game replayed the same dwarves. | `r=0` after loading. |
| getin | Read `line(71)` at the end of a full line. `lgword` was not cleared after a word of exactly five letters. | Both fixed. |
| 8243 (score format) | Line longer than 72 columns. | Split. |
| ioinit | Called `attach_fortran_ssfile_`, a 1980 routine that no longer exists, with a hard-coded path. | Calls `advatt` (below). |

## Mixed case: Jim's other 1980 note

Words were packed five characters at a time, six bits each. That gave room for only 64 of the 90 characters in
`chrset`, so a capital letter spilled into its neighbour. The one capital in the 1980 database, "Over" in the
bulletin (message 202), printed as `!.ver`.

Beside `dcode1`, Jim wrote "try several combinations of 90 and 64, 64 and 90".

- **The packing change.** `code1`, `code2` and `dcode1` now pack in base 90 (`result*90+index`, `mod 90`,
  `/90`). All 90 characters fit, capitals included. 90⁵−1 = 5,904,899,999 still fits in a positive 36-bit
  integer.
- **What stays the same.** Nothing else depends on the six-bit layout.
  - Commands are still lowercased by `cvltuc` before packing, so the vocabulary (section 4) stays lowercase.
  - A new message of the day keeps whatever case the wizard types.
  - `adventure.newgame` and saved games made before the change are not compatible. None existed yet.

The database text (sections 1, 2, 5, 6, 10 and 12) is now in mixed case. Only letter case changed: every line
has the same characters and length as before.

- **Tools.** The conversion was done by [`recase/recase.py`](recase/recase.py). Where three consecutive words
  match a mixed-case edition, it copies that edition's casing:
  - Woods' text is taken from Arthur O'Dwyer's faithful C translation of Woods 350 and from Knuth's
    `advent.w`;
  - Platt's text is taken from his 1984 `ADVENTURE.ACODE` and O'Dwyer's translation of it;
  - all of these come from [Quuxplusone/Advent](https://github.com/Quuxplusone/Advent) at commit `2532f15`.

  Everything else gets sentence capitalization and "I".
- **Hand corrections.** [`recase/overrides.py`](recase/overrides.py) holds about 50 corrections:
  - the local texts: the credits in message 1, the 1980 news and the list of Multics games;
  - quoted commands in the help text, made "QUIT", "FAST" etc. to match "SUSPEND";
  - dialogue tags ("...idiot!" he storms);
  - the djinn's speeches. Platt 1984 has them all in capitals, which matched only piecemeal, so they are in
    ordinary sentence case.
- **Conventions.** Signs follow Woods' mixed-case editions ("MAGIC WORD XYZZY", "STOP! PAY TROLL!"). Platt's
  rooms follow Platt ("Twopit Room", "Audience Hall"). Multics pathnames and `jjl.sct` stay as printed.
  `>$<` comment lines stay lowercase.

## Database errata fixed in `adventure.data`

The numbers match the ERRATA list in
[`adventure.data-NOTES.md`](../artifacts/Multics/adventure.data-NOTES.md).

1. **Location 185.** The row's last three numbers (`199 180 49`) belong to a separate row for 199. They are now
   one.
2. **Fog room 204.** Direction 47 appeared twice and 48 not at all. The second one is now `204 207 48`.
3. **Message 82.** The two spurious "0" fields are removed.
4. **The minotaur's wall (object 89).** It was placed at 164, 168 and 200, but an object can only be in one
   place, so only 200 took effect.
   - Message 242 says the wall hides passages northwest, north and northeast. The code killed anyone who
     arrived where the wall was ("you walked right into the minotaur's wall!").
   - Neither placement works. Room 168 is also reached from the basilisks' corridor (169), and room 200 from
     the lava tube (214), so the wall would kill players arriving from behind.
   - The wall is now in the travel table, in Woods' usual way (`388xxx`: prop of object 88 not 0).
   - Until MELENKURION, the moves between 142 and 164, 168 and 200 are blocked in both directions with
     message 243 as a plain refusal. After it, they are open.
   - Object 89 is placed nowhere, so the code's wall2 check (2015) is no longer reached.
5. **Locations 35 and 110.** Withdrawn. This was first taken for a conversion error ("25 feet away..." merged
   into the location number), but it was a transcription error, now corrected in the artifact. The database was
   right as printed.
6. **`>$<` markers.** Six of Platt's objects had a single space between `>$<` and a comment. The program only
   recognizes `>$<` followed by two blanks, so "`>$< (part of location)`" was printed to the player. A second
   space is added, as in Woods' entries.
7. **New entries.**
   - Vocabulary: `1099 all`.
   - Message 271: LISTEN at a live dragon.
   - Message 272: TAKE ALL with nothing to take.

## New files: the missing site routines

Palter's program leaves some routines to the site: `addr`, `size`, `ldcomn`, `svcomn` and `getime`. The PL/I
command that ran `adventure_` did not survive either. They are written anew for current Multics (MR12.x):

| File | Entries | What it does |
|---|---|---|
| `adv462.pl1` | `adv462` | The command. It calls `adventure_$main_`: the Fortran compiler names a main program's entry `main_`, which is why `adventure_` could never be typed as a command. On the way out it detaches the database switch. |
| `adv462_io_.pl1` | `ldcomn`, `svcomn`, `advatt`, `advdet` | Save and load the common blocks, and attach or detach `file01`. <ul><li>The new-game image `adventure.newgame` and the database live in the directory holding this segment.</li><li>Suspended games are `name.adv462` in the player's home directory.</li></ul> |
| `addr.pl1` | `addr` | Stores a common block's address (a packed pointer) in `cmadrs`. |
| `size.pl1` | `size` | Words from one variable to another. |
| `getime.pl1` | `getime` | Time of day (microseconds) to minutes. |
| `build.ec`, `bound_adv462_.bind` | | Compile everything and bind it into `bound_adv462_`. The bindfile makes `ldcomn`, `svcomn`, `advatt` and `advdet` synonyms of `adv462_io_`, so the calls are resolved inside the bound segment, and adds only the name `adv462`. |

## Unix-only differences

The Unix source `../unix/adv462.f` is generated from `adventure_.fortran` by `../unix/mkunix.py`, so every fix
above is in both versions. The generator changes only these things:

- `code1` takes a character string;
- `and`, `or`, `xor` and `shift` use the gfortran intrinsics;
- `ioinit` opens the database with OPEN;
- `datime` uses DATE_AND_TIME;
- `ran` and `size` are declared EXTERNAL (both are gfortran intrinsics);
- getin stops at end of input;
- the writable game directory is `/usr/games/adv462`, the counterpart of `>site>adv462_dir`;
- the default hours have no prime time. In 1980 the cave was closed to all but wizards 8:00–16:59 on weekdays.
  Magic mode can still set hours.

`addr`, `size`, `ldcomn`, `svcomn` and the default data path are in C (`../unix/adv462_util.c`).
