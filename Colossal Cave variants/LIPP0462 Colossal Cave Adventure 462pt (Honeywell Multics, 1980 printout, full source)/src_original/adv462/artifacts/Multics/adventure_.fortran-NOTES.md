# adventure_.fortran transcription notes

Layout assumptions (provisional, pending adv.fortran / more pages):
- Confirmed by Jim on paper: comment "c" in column 1; statements start in column 7 (standard fixed form);
  continuation "&" in column 6.
- Comment text is mixed case ("Adventure", "Current Limits:") — the file is *not* entirely lowercase.

## Page 1 (single sheet): header comments, common blocks, first dimensions
- Header comment rewritten from Woods ("C  ADVENTURES"): now "Adventure" framed by "=====" rules,
  with enlarged limits: 22000 words of text (Woods 9650), 1500 travel (750), 500 vocab (300),
  250 locations (150), 100 objects, 50 action verbs (35), 450 random messages (205), 12 classes,
  20 hints, 35 magic messages. Upper structural limit on locations shown as 250 (Woods: 300).
- The limits-table spacing (numbers left-aligned in col 8, descriptions col 14) is a best guess from a tilted photo.
- **Common blocks are Gary Palter's, not Woods'.** /ioscom/ttyi,ttyo,blklin,dbfi, /mtdcom/mtdtxt,
  /rancom/r and the big /msccom/ list, and /placom/ including cond,prop,loc,lamp, are all identical to the
  Palter-derived SEL-32 port (HORV0350, 1979) and absent from Woods. This confirms the Palter (MIT-Multics) base.
- /msccom/ matches HORV0350 exactly, plus new variables for the Platt material:
  bullet, slime (end of the dkill line), slime2 (end of the demo line), and three new lines:
  sword, ogre, ogre2, ring / wall, cavdst, wall2, teeth, goblins, basil, basl2, plate, basilisk /
  sceptre, skeleton, yacht, flask, pentagram, fog, djinn.
- /txtcom/rtext,lines kept (HORV0350 commented out "lines").
- Page ends mid-DIMENSION (`&atloc(250)`).

## Page 2 (two sheets): dimensions, statement functions, common-block setup, database-format comments
- Remaining DIMENSIONs match HORV0350 (Palter) with the new sizes: actspk(50), rtext(450), abb(250) etc.;
  hname(20), mtdtxt(100), cmadrs(4,11),cmszes(11),text(70),fname(10),fdummy(10) as in Palter.
- DATA linsiz/22000/,trvsiz/1500/,locsiz/250/, vrbsiz/50/,rtxsiz/450/,clsmax/12/,hntsiz/20/,magsiz/35/.
- Statement functions = Palter's (bitset uses and(...,shift(1,n)); liqloc on one line here). LOGICAL list adds
  **cavdst** (cave-destroyed flag, for Platt's jellyfish/Ralph ending).
- **Contact comment changed**: Palter's "GARY M. PALTER, MIT (617) 253-7728 (PALTER@MIT-MULTICS)" is replaced by
  "James J. Lippard (Lippard.Scouting) / for modifications to the original / program by G. Palter (MIT)"
  (mixed case in the original).
- addr/size setup identical to Palter except sizes (lines(22000), abb(250)). No `call ioinit(0)` (HORV0350 has one;
  probably a SEL-32 addition).
- Database-format comments = Woods/Palter text, lowercase. Page ends mid-sentence in the section 3 description.
- Comment-text indentation follows Palter's layout (spacing within comments inferred from photo + HORV0350).

## Page 3 (two sheets): database-format comments (cont.), start of database read
- Comments = Woods/Palter text in lowercase, except:
  - travel-table description: "if n<=250  it is the location to go to" (Palter: n<=300), matching locsiz 250.
  - cond-bit list adds "10  in fog-filled room" (the new hint bit, cf. data section 9/11).
  - (Checked on paper by Jim: no extra "c" line before "4 trying to get into cave"; the c there belongs to that line.)
- 8500: `call ioinit(0)` now sits just before `write(ttyo,1000)` (Palter/HORV0350: ioinit(0) earlier, ioinit(1) here).
  "Initializing..." keeps its capital I.
- tabsiz=500 and `do 1001 i=1,500` (Palter 300). Page ends at `stext(i)=0`.
- One blank line after the text-pointer comment block (HORV0350 has two); from photo, check if it matters.

## Page 4 (two sheets): data-reading loops (1001-1050)
- Matches Palter's logic, with Woods-style direct assignments to lines() (HORV0350's `call setlines` is a SEL-32 change).
- No range check on sect1 before the computed goto (HORV0350 has one, plus Norwood's 25001 label); here the goto
  is followed directly by `call bug(9)`.
- Read formats: i8 / (1i8,70a1,a1) / 22i8 / (i8,5a1) — settles the data-file layout (8-column number fields).
- 1030 reads `loc,newloc,tk` with format(22i8) (tk is dimensioned 20).
- Page ends at `goto 1050` (object-location loop).

## Page 5 (two sheets): 1060-1300 and start of object mnemonics
- Matches Palter (HORV0350 text lowercased) except:
  - 1070 and 1081 read `k,tk` whole-array (HORV0350: implied-do (tk(i),i=1,15) / (i=1,4)).
  - bit set with `shift(1,k)` (HORV0350: ishft).
  - 1100: `if(setup.eq.-1)goto 8305` (= Palter; photo looked like 3305, Jim confirmed 8305 on paper).
  - maxtrs=79 block at normal indentation (HORV0350 has two extra spaces).
- Blank-line placement on the tilted top sheet follows HORV0350 where the photo is ambiguous.
- Page ends at `bottle=vocab(code1('bottl'),1)`.

## Page 6 (two sheets): object mnemonics, dwarves, counters
- Woods/Palter mnemonics plus the Platt objects, interleaved in the order printed: after troll2 come slime, slime2,
  vial, mushroom, then bear, then sword, ogre, ogre2, ring, wall, wall2, goblins, basil, basl2, plate, sceptre,
  skeleton, yacht, flask, pentagram, djinn, teeth, then messag, vend, batter. Variable names longer than 6 characters
  (mushroom, skeleton, pentagram) — fine in Multics Fortran, not standard F66/F77.
- basl2=basil+1 (the common block also declares an unused-looking `basilisk`).
- `bullet=vocab(code1('bulle'),1)` appended to the "action verbs" group but with type 1 (object).
- Dwarf initialization and counter comments = Palter. New local `mushturn=0` after turns=0 (mushroom effect timer).
- Blank lines between groups follow HORV0350 (the top sheet is too foreshortened to be sure).
- Page ends at `detail=0`.

## Page 7 (two sheets): counters, table-space report, start-up (label 1), first dwarf code
- Same as Palter except:
  - 1999 format: "' Table space used:'" (capital T) and every line ends ",/" (HORV0350 has a missing comma after
    "words of messages'"); lowercase text; closing `&)`.
  - 19991 format(' Initialization completed.') — leading blank and mixed case (HORV0350: 'INITIALIZATION COMPLETED.').
  - **limit=500** (lamp life; Woods/Palter 330), still 1000 if instructions were requested.
- Page ends at `6000 if(dflag.ne.1)goto 6010`.

## Page 8 (two sheets): dwarf movement, pirate, dwarf attacks (6000-84)
- Palter's code, except: dwarf movement excludes `newloc.gt.250` (Palter 300); format 67
  "(/,' There are ',..." (mixed case, comma after /); format 68 "(/,' ',i1,...". Page ends at `goto 99`.

## Page 9 (two sheets): 79, location description 2000-2017, hints 2600, input 2605-2608
- New Platt code inserted in the 2000 block between `if(wzdark.and.pct(35))goto 90` and `kk=rtext(16)`:
  fog-room glow messages (rspeak(256+loc-200) for rooms 201-208), deadly slime (2013), yacht prop, ogre (2014),
  minotaur's wall (2015, msg 243 "you walked right into the minotaur's wall!"), basilisks (2016/2017, metal plate),
  goblins (moved to oldloc at 166, escalating 245-247, kill at prop 3).
- **Logic bug (matches Jim's "possible problem" annotation):** the line before it,
  `if(forced(loc).or..not.dark(0))goto 2001`, skips the whole new block whenever the room is lit. So the block runs
  only in darkness, where `fog.eq.1.and..not.dark(0)` can never be true, and slime/ogre/wall/basilisk/goblin checks
  never fire with a working lamp. It then falls into `kk=rtext(16)` (pitch dark). In the working copy the block
  should run before the dark test (e.g., move it to just after label 2000 / before 2001).
- Comments "He ran into the slime. He's dead." and "Basilisk." are mixed case (new material).
- `2012 ... goto 2600` added (Palter falls through).
- Uses undeclared-in-common locals `fog`, `basilisk` (basilisk is in /msccom/; fog too).
- Page ends mid-2608 after the magic-mode check (`code1('mode ')`).

## Page 10 (two sheets): 2608 (cont.) through 5110
- **RESTORE check lost its turn-0 test**: `if(wd1.eq.code1('resto'))goto 8400` (Palter: `if(turns.eq.0.and.wd1.eq...)`),
  so "restore" works at any point in the game, consistent with the new INFO text (message 142). (Confirmed on paper by Jim.)
- New mushroom timer: `if(mushturn.ne.0) mushturn=mushturn+1` / `if(mushturn.gt.40)goto 2631`; 2631 prints message 220
  (strength wears off), resets mushturn and moves the mushroom back to room 160 (the dank cubicle). `call move` is
  indented one extra column on paper; kept.
- No range checks before the computed gotos (bug(22)/(23)/(24) calls follow the gotos instead, Woods-style).
- **4080 intransitive verb table extended** with 8320 fast, 8330 full, 8340 listen, 8350 turns, 8360 phugg, 8370 melen
  (comment row "hour fast full lstn turn phg mlnk").
- **4090 transitive table NOT extended** (still 31 entries), so verbs 32-37 with an object (e.g. "listen stream",
  "turns x", or "phuggg" followed by a word) fall through to `call bug(24)`. Probable bug; working copy should add
  entries (e.g. 2011 or the intransitive handlers).
- 5015 format: "' What do you want to do with the '" (mixed case).
- Page ends inside the 5112 dwarf loop.
- (Jim confirmed on paper: no turns.eq.0 test on the RESTORE line.)

## Page 11 (two sheets): 5112-5199, travel (8-16), specials 30000-30310
- Identical to Palter apart from lowercase, "' I see no '" (mixed case), and the computed goto at 30000 followed by
  `call bug(20)` (no range test). Travel code still uses 300 as the location/special boundary (fine with 250 rooms).
- No new special-travel cases (still only 301-303), so none of the new Platt rooms use special motions.
- Page ends at `goto 99` after the troll-bridge/bear special (30310).

## Page 12 (two sheets): go back (20-25), look (30), cave (40), non-applicable motion (50), death (90-98), 95, 8000
- Palter's code, lowercase, plus after `99 if(closng)goto 95`:
      c   If he destroyed the cave, no reincarnation option.
      if(cavdst)goto 20000
  (PHUGG near water -> Ralph's cave destruction -> game over.)
- Page ends at `write(ttyo,8002)(tk(i),i=1,k)`.

## Page 13 (two sheets): 8002-8010, carry (9010-9014), drop (9020-9029), SAY start (9030)
- 9010 adds: sword (9218 if weak and sword still in stone; else prop(sword)=1) and sceptre (9221) checks.
- 9017: `if(holdng.lt.12.and.mushturn.ne.0) goto 9016` — the mushroom lets you carry 11 items instead of 6.
- 9027 adds bird-at-basilisk (9029: bird destroyed, msg 253, death) and flask-in-pentagram (prop(flask)=1).
- SAY uses code1('".   ') (Palter's SEL-32 transcription shows '"".   ').
- Extensive handwritten ALL implementation on this page: see handwritten-notes.md.
- Page ends mid-9030 at the magic-word check.

## Page 14 (two sheets): SAY end (9032-9035), lock/unlock (8040-9049), djinn (2047-2049), lamp on/off, wave
- 9032 format(/,' Okay, "',20a1) (mixed case; single " where HORV0350 shows "").
- 8040 also assumes flask or pentagram as the object; 9040 sends flask/pentagram to the new djinn routines 2047:
  OPEN FLASK in the pentagram (prop 1) -> 266 polite djinn (prop(djinn)=1); already open -> 267; flask elsewhere -> 268
  (rude djinn, but nothing else happens: flask prop not changed, so it can be repeated); OPEN PENTAGRAM with djinn
  here -> 2049 destroy djinn, 270 (phuggg lore); otherwise 269.
  Note 2047-2049 is a new 4-digit label range inside the 9000s code (Woods' 2000s are the main loop).
- Note place(djinn): the djinn starts at 211 (the pentagram room) fixed, so "place(djinn).eq.loc" is true in that
  room even before he's released — OPEN PENTAGRAM there before opening the flask would skip straight to message 270.
  Probable bug; working copy should also require prop(djinn).eq.1.
- Rest (grate, clam/oyster, chain, lamp, wave) = Palter. Page ends at `goto 2012` after wave.
- Also in 2047: `if(obj.eq.flask)spk=268` runs after the prop=2 test and overwrites spk=267, so "the flask is already
  open!" (267) can never print; an opened flask gives the rude-djinn message (268) again. Probable bug.

## Page 15 (two sheets): attack (9120-9126), pour (9130-9132), eat (8140-9140), drink (9150)
- Palter's code with: killing the dragon also moves the dragon's teeth to the dragon's central location
  (`call move(teeth,k)`); EAT with no object prefers the mushroom (8140 -> 8143), and EAT MUSHROOM -> 8143:
  destroy mushroom, mushturn=1, message 221 (muscles bulge). Comment reads "fun  for dragon." (Palter: "fun stuff
  for dragon."), transcribed as printed.
- Ogre, goblins, basilisk and djinn are not in the attack target list; attacking them falls to default messages.
- Page ends at `goto 2011` after DRINK.
- REVIEW LIST (Jim): attack code (9120-9125) doesn't know the ogre, goblins, basilisks or djinn; decide in the working
  copy what ATTACK/KILL should do for each (Platt 1984 has OGRE.TOO.TOUGH / OGRE.RIPS.HEAD.OFF messages, not in this data file).

## Page 16 (two sheets): rub, throw (9170-9179), quit, find, inventory, feed start
- Throw comment extended: "...unless axe or vial..." + "The vial is for the slime.  The sword is for the ogre."
- 9170 adds: vial -> 9179; sword at ogre -> 9217; teeth at goblins -> 9219; `if(obj.ne.axe.and.obj.ne.sword)goto 9020`,
  so a thrown sword elsewhere behaves like the axe (can kill dwarves, etc.); 9175 drops `obj` (Palter: axe).
- 9179 vial: if slime here obj=slime; `call rspeak(ran(4)+214)`; destroy vial; slime -> 9215.
  **Probable off-by-one:** the four vial messages are 215-218 but ran(4)+214 gives 214-217, so 1 time in 4 you get
  message 214 ("the slime ... blackens and shrivels") instead of a vial cloud, and 218 (chlorine and apples) never
  appears. Working copy: ran(4)+215. (Printed value confirmed from photo as 214.)
- **8185 `if(gaveup)goto 20000` has no following `goto 2012`** (Palter has one): answering "no" to QUIT falls
  through into the FIND code (9190) with obj=0. Bug; restore `goto 2012`.
- Find, inventory = Palter. Page ends at `if(.not.here(food))goto 2011` in 9213 (feed dwarf).

## Page 17 (two sheets): feed (9213-9216), Platt handlers 9215-9221, fill, blast, score, fee-fie, brief
- 9214 bear branch now `goto 9216` (old 9215 spk=14 renumbered to 9216); new 9215 = kill slime (destroy slime,
  slime2, message 214).
- 9217 kill ogre: destroy ogre, ogre2, sword; ring appears here; message 222 (sword melts into a silvery ring).
- 9219 teeth at goblins: destroy goblins and teeth; message 249 (skeletal warriors).
- 9221 sceptre: if skeleton here -> prop(skeleton)=1, prop(sceptre)=1, message 254 ("it's not a bug, it's a feature!").
  **Probable bug:** 9010 sends every TAKE SCEPTRE (when the sceptre is here) to 9221, which never calls carry(), so
  the sceptre can never actually be picked up (and if the skeleton is gone, nothing at all is printed except the
  default spk=25). Working copy: 9221 should fall through to the normal carry code (e.g. goto 9017) after the
  skeleton effect.
- 9218 sword in vain: message 223.
- **SCORE (8241-8243)**: "' Your current score is',i4,' out of a possible',i4,' points.'" then goto 2012, no quit
  prompt (matches news message 202: "score no longer asks you if you want to quit"; message 143 removed from data).
- Fill, blast, fee-fie-foe-foo = Palter. Page ends mid-8260 (brief) at abbnum=10000.

## Page 18 (two sheets): brief end, read, break, wake, suspend, hours, and the new FAST/FULL/LISTEN/TURNS
- READ: bulletin board added (`if(here(bullet))obj=bullet`, `if(obj.eq.bullet)spk=202` -> the news message).
  (202 confirmed by Jim.)
- SUSPEND: "' I can suspend your adventure...'" (capital I); Palter's `call shutdown` is replaced by Fortran `stop`.
- No 8400 RESTORE block after 8310 hours on this page (Palter has one here); presumably elsewhere, check later pages.
- New verbs:
  - 8320 FAST: msg 54 (ok); do 8321 i=1,250: abb(i)=1; `8321 detail = 3` (label on the assignment, so detail=3
    is executed inside the loop 250 times; harmless); abbnum=10000.
  - 8330 FULL: abb(i)=0 for all rooms; `8331 abbnum=5` is the loop terminal (executed each pass; harmless).
  - 8340 LISTEN: bird 208, snake 209, dragon 210 (note: 210 is the *dead* dragon message, printed even if alive),
    water 207, one dwarf 211, several 212, pirate nearby 127, slime 219, else 206 "all is silent".
    Uses dtotal from the last dwarf pass.
  - 8350 TURNS: "' You have taken a total of',i4,' turns.'" (page ends at format 8355; the goto presumably follows).
- Mixed-case comments for all the new verbs ("fast.  Intransitive only.", "Tell him how many turns...").

## Page 19 (two sheets): TURNS end, PHUGGG (8360-8367), MELENKURION (8370), RESTORE (8400), hints (40000-40900)
- 8350 turns ends `goto 2012`.
- 8360 PHUGGG ("kills dwarves unless carrying or near weapon or near water"):
  near water -> 8363: death messages 233-235 (one dwarf), 236-238 (several), 239-241 (none: jellyfish, cave destroyed
  by Ralph, tentacle), then goto 99 (death); carrying/near axe -> 225 axe vanishes; sword -> 226 sword disintegrates;
  one dwarf -> 227-229 dwarf removed; several -> 8366 all dwarves here removed, 230-232; no dwarves, no water -> 42.
  Bugs:
  - `if(dtotal.eq.0.and.i.eq.2)cavdst=.true.`: i is 239 there, so cavdst is never set; Ralph's "cave destroyed"
    (240) is followed by the normal reincarnation offer. Intended: `spk.eq.240`.
  - single-dwarf case uses a stale `i` (dseen(i)/dloc(i)) instead of finding the dwarf at loc.
  - 8367 tests dloc(i).eq.loc after dloc(i) was already set to 0, so dseen is never cleared (harmless).
  - 8361 spk=42 is overwritten/fallen through when there are no dwarves (falls into 8362 anyway); fine in effect.
- 8370 MELENKURION: if the minotaur's wall is here, destroy wall2, prop(wall)=1, message 242 (wall crumbles);
  else 42.
- 8400 RESTORE = Palter (moved here after the new verbs).
- 40000 hints: computed goto for hints 4-9 only, then `call bug(27)`.
  **Bug:** adventure.data adds hint 10 (fog rooms, section 11 `10 25 5 255 256`), so after 25 turns in the fog
  hintm3=7 falls out of the goto and the program stops with bug 27. Working copy: add a 7th target (e.g. 40900-style
  `goto 40010`).
- 40012 format mixed case "' I am prepared to give you a hint, but it will cost you'". Rest = Palter.
- Page ends mid cave-closing comment ("...we can have no keys, since there is a").

## Page 20 (two sheets): cave closing (10000-11010), lamp (12000-12600), demo end 13000, 19000
- Identical to Palter (lowercase). Page ends at "c  exit code.  will eventually include scoring.  for now, however, ...".
- Closing/storage room code knows nothing about the new objects (none needed; the repository contents are Woods objects).

## Page 21 (two sheets): scoring (20000-25000), end of main program, start of code1
- Scoring table comment: "each treasure > chest  16  256" (16 treasures above the chest: Woods' 9 + 7 new) and
  "total: 462" — matching message 244 "adventure 1.2 (462 points)". Algorithm itself unchanged from Palter; maxtrs=79
  covers the new treasures 65-71. Table column layout taken from Palter (photo too tilted to measure).
  "(max-num)*10" as printed (HORV0350 has "max0-num").
- After `if(scorng)goto 8241`: `c  If the fool destroyed the cave, take away all his points` / `if(cavdst)score=0`
  (never effective because of the PHUGGG cavdst bug).
- Final formats mixed case ("You scored", "You just went off my scale!!", "To achieve the next higher rating...").
  Rank thresholds still Woods' (data section 10), topping out at 349.
- `25000 stop` (Palter: call shutdown), then `end` of the main program.
- Start of the utilities: comment "(code1, code2, dcode1, cvltuc, cvstb)" and `integer function code1(words)`.
  cvltuc/cvstb aren't in Palter's list — probably Multics character-case conversion helpers (convert lowercase to
  uppercase? / convert string to bits?). Page ends in code1's header comment.

## Page 22 (two sheets): code1, code2, start of dcode1 (character conversion)
- Multics (36-bit, 9-bit ASCII) adaptation of Palter's utilities:
  `data nwords/2/,nchars/4/,chrsiz/9/,chrmsk/o777000000000/` (SEL-32: 8-bit, hex mask);
  `and()`/`shift()` instead of iand/ishft; `implicit integer(a-z)`.
- **chrset is 90 entries**: Palter's 64-character sixbit table with the letters in lowercase (1ha..1hz at the
  positions of A..Z), `'\'` for backslash, `1h^`, `1h_`, then 26 extra entries 1hA..1hZ (65-90).
  So lowercase text maps to the same sixbit codes as Palter's uppercase; uppercase letters get chridx 65-90 and
  `result=shift(result,6)+chridx-1` then overflows the 6-bit field (codes 64-89 corrupt the neighbouring character).
  Presumably input is lowercased (cvltuc?) before code1/code2 so this never happens; check getin later.
- The caret entry prints like a raised bar; `1h^` confirmed by Jim. Lowercase alignment/indentation
  of the data continuation lines inferred.
- Page ends inside dcode1's chrset data statement (after the 1h8..1h? row, one more `&` line started).

## Page 23 (two sheets): dcode1 (end), cvltuc, cvstb, start of ioinit
- dcode1: `chridx=mod(valcpy,64)+1` (Palter: lowsix(valcpy)+1) — Multics has no lowsix. Handwritten notes on
  trying 90 vs 64 (see handwritten-notes.md).
- **cvltuc is inverted for Multics**: `upper` holds lowercase letters and `lower` holds uppercase (Palter: upper=A..Z,
  lower=blanks), so the routine actually converts UPPER to lower case, despite its comment. That is how player input
  is made to match the lowercase data file (resolves the page 22 overflow concern, assuming getin calls it).
- cvstb: `2 do 10 i=s,10` (HORV0350: i=8,10 — the SEL-32 transcription, likely a typo there). Otherwise = Palter.
- Section comment "i/o routines (ioinit, speak, pspeak, rspeak, getin, yes, a5toa1)" then Palter's generic ioinit
  header comments ("on lu tty" sic). Page ends at "c  (see conversion guide)" in ioinit.

## Page 24 (two sheets): ioinit (Multics body), speak, pspeak, rspeak, mspeak, start of getin
- **ioinit is Multics-specific**: ttyi=5, ttyo=6, dbfi=1;
  `external attach_fortran_ssfile_(descriptors),com_err_(descriptors)`;
  `call attach_fortran_ssfile_(1,'>udd>MED>Kaiser>Lippard>adventure.data',error)`; on error
  `call com_err_(error,'adventure','adventure.data')` and stop.
  **The data file path is >udd>MED>Kaiser>Lippard>adventure.data** — project MED, user Kaiser (Garry Kaiser, whose
  quota was used), subdirectory Lippard. Same directory as message 203's ">udd>med>wgk>jjl>adventure" (the GAMES list
  uses the short names). Mixed case in the path.
  (ttyo value read as 6 from a blurred line; standard Multics Fortran unit for output. Check if needed.)
- speak/pspeak/rspeak/mspeak = Palter, with dimension rtext(450), lines(22000) and /txtcom/ rtext,lines
  (HORV0350's SEL-32 setlines/!!! changes absent).
- Page ends in getin's header comment.

## Page 25 (two sheets): getin (rest), logical function yes
- getin = Palter's code minus the SEL-32 NEWLINE handling (no `newline` data item or `line(i).eq.newline` test);
  `call cvltuc(line,70)` after the null check lowercases input (see page 23), so typed capitals match the data file.
- `logical function yes(x,y,z)` = Palter (calls yesx with rspeak). Page ends at its `end`.
- Blank-line spacing inside getin follows the photo where visible, Palter elsewhere.

## Page 26 (two sheets): yesm, yesx, a5toa1, vocab, start of dstroy
- Palter's routines. yesx drops the `logical blklin` declaration (blklin is then implicitly integer in this routine;
  harmless since yesx doesn't use it); message "' Please answer the question.'" mixed case.
- vocab: adds `common /ioscom/`, ktab/atab(500); `3 vocab=ktab(i)` / `if(init.ge.0)vocab=mod(vocab,1000)`
  (HORV0350 goes through a temporary v). Page ends at `subroutine dstroy(object)`.

## Page 27 (two sheets): dstroy, juggle, move, put, carry, drop
- = Palter with atloc/cond sized 250. move still tests from.le.300 (fine). Page ends at the "wizardry routines (start, maint, wizard, hours(x), newhrs(x), motd, poof)" comment.

## Page 28 (two sheets): start, maint (to latency prompt)
- Palter's prime-time/wizard logic. Differences: and()/shift(); `stop` for `call shutdown` (x3);
  formats mixed case ("This adventure was suspended a mere", "Length of short game", "Latency for restart");
  start's logical list omits blklin; abb(250).
- **Minimum restart latency lowered to 30 minutes** (`x.lt.30` -> mspeak(30), `max0(30,x)`; Palter 45),
  matching data message 30 "too small! assuming minimum value (30 minutes)."
- Format 12 has "(/," (Palter: no /). Page ends at `if(x.gt.0)latncy=max0(30,x)`.

## Page 29 (two sheets): maint (end), wizard, hours, hoursx (start)
- Palter's code, with shift() and mixed-case holiday messages ("Today is a holiday, namely", "The next holiday...").
- **wizard: `mword=magic`** (Palter: `mword=xor(magic,x)`). The random 10-digit challenge is still printed, but the
  correct reply is just the magic word again, so the challenge/response is effectively disabled (anyone who knows
  the magic word passes). Possibly because Multics Fortran lacked xor(), or a deliberate simplification.
- hoursx day-type table in lowercase ("mon - fri:", "sat & sun:", "holidays: "). Page ends at the end of that data statement.

## Page 30 (two sheets): hoursx (rest), newhrs, newhrx, start of motd
- Palter with and()/shift()/or(); newhrx accumulates directly in `newhrx` (Palter uses nx); prompts mixed case
  ("Prime time on", "From:", "Till:", "Closed all day", "Open all day" — confirmed by Jim).
- motd: no NEWLINE data item (as in getin); `logical alter`. Page ends at `if(nwords.eq.0)goto 40`.

## Page 31 (two sheets): motd (rest), poof, scrmbl, shift (start)
- motd = Palter minus NEWLINE handling and **without Palter's `call cvltuc(text,k)`** before encoding: a wizard's
  message of the day typed in capitals would reach code2 with chrset indices 65-90 and overflow the 6-bit fields
  (garbled MOTD). Minor bug; working copy should call cvltuc here.
- poof = Palter (weekday prime time 9-18, short game 30, latency 90, magnm 11111) with `magic=code1('dwarf')`:
  **the wizard magic word is "dwarf"** unless changed in maintenance mode.
- Utility list comment adds shift and and ("scrmbl, shift, and, ran, datime, ciao, bug"); or/xor do exist (next page), so wizard's
  mword=magic was a deliberate change, not a missing function.
- **shift implemented in Fortran for 36-bit Multics**: maxpos=34359738367 (2**35-1), maxneg=o400000000000,
  mxpsr1=17179869183, mxnsr1=17179869184; arithmetic-IF on dist; right shift loop at 10-11. Page ends at `20 return`
  (left-shift branch 30 presumably follows).

## Page 32 (two sheets): shift (left branch), and, or, xor, ran, datime (start)
- and/or/xor implemented via LOGICAL equivalence (Multics Fortran treats logicals as full words); xor via
  (a.and..not.b).or.(.not.a.and.b) using one's-complement -x-1. So xor() was available: wizard's `mword=magic` was a
  deliberate simplification.
- ran = Woods' generator ("written by don@sail"). 
- datime: Multics `call clock_(ftime)` and `call decode_clock_value_(ftime,month,day,year,ftime,dummy,dummy)`
  (real*8 ftime; days table for month lengths). Page ends there; conversion to days since 07/01/77 follows.

## Page 33 (single sheet): datime (end), bug
- datime converts Multics month/day/year to days since 07/01/77 (as the comment says it should be relative to
  7/1/77, but the code computes from 1977-01-01: d=day-1+days of prior months+365*(year-1977)+leap days; fine for
  prime-time weekday tests only if mod(d,7) aligns — Palter used 07/01/77 as a Saturday; here day 0 = Sat 01/01/77,
  which was also a Saturday, so mod(d,7).le.1 still means Sat/Sun). `t=getime(ftime)` (minutes past midnight).
- bug: Palter's list; "' Fatal error, ...'" mixed case; `if(num.lt.20)pause` then `stop` (Palter: call abort).

# END OF adventure_.fortran

## Summary checks
- Transcribed: 3,756 lines, of which 653 are blank and 3,103 are non-blank. Jim's count for the file is 3,110, very
  close to the non-blank count: either the original count excluded blank lines, or blank-line placement (often
  reconstructed from Palter's layout where the photo was ambiguous) is overstated and ~7 printed lines are missing.
  Worth checking against the real file/listing line numbers if possible.
- Routines called but NOT defined in this file: addr, size, ldcomn, svcomn (Palter's site-supplied save/restore of
  common blocks) and getime (minutes past midnight). They are presumably in adv.fortran or another Multics module;
  the working copy will need them.
- Listed in the utility comment but not present: ciao. Not needed (no calls).

# CONSOLIDATED FIX LIST for the working copy (code)
1. Location description block (2000): move the Platt checks (fog glow, slime, ogre, wall, basilisk, goblins) before
   the dark test; currently they only run in darkness. [Jim's "possible problem" annotation]
2. 4090 transitive verb table: add entries for verbs 32-37 (otherwise bug(24)).
3. Djinn 2047: "already open" (267) overwritten by 268; OPEN PENTAGRAM should require prop(djinn)=1.
4. Sceptre 9221: never carries the sceptre; fall through to normal carry.
5. Vial 9179: ran(4)+214 should be ran(4)+215.
6. QUIT "no" (8185) falls into FIND: restore `goto 2012`.
7. PHUGGG: cavdst test uses i.eq.2 (should be spk.eq.240); single-dwarf case uses stale i.
8. Hints: 40000 goto list needs a 7th entry for hint 10 (fog), else bug(27).
9. LISTEN at dragon always gives the "dead dragon" message.
10. MOTD: call cvltuc before code2.
11. Implement ALL (take/drop) per handwritten notes; add "all" to vocabulary.
12. Review: ATTACK against ogre/goblins/basilisk/djinn.
13. Supply addr/size/ldcomn/svcomn/getime (or modern equivalents).
Plus data-file errata 1-4 in adventure.data NOTES.md.

## Completeness check (static analysis of the transcription)
- 40 program units: main program + 39 subroutines/functions. Every statement label referenced inside each unit is
  defined. (The one apparent exception, `goto 3305` at 1100, was a photo misreading; Jim confirmed 8305 on paper.)
- External routines called but not defined in this file:
    addr(x,cmadrs(1,n)), size(first,last)  -> build the table of common-block addresses/sizes (PL/I-style builtins;
                                               Palter's comment calls them "site-supplied")
    ldcomn(l,fname,cmadrs,cmszes)          -> load common blocks: called at startup with .true. (the initialized
                                               "system" game) and by RESTORE with the player's save name
    svcomn(l,fname,cmadrs,cmszes)          -> save common blocks: SUSPEND, and MAGIC MODE (wizard saves new version)
    getime(ftime)                          -> minutes past midnight, used by datime
  All are system/site-dependent, so on Multics they were probably a small PL/I module (or are in adv.fortran).
  Without them the game can't start (ldcomn is the first executable call). Working copy: implement these.
- `ciao` is listed in the utilities comment but neither defined nor called; not needed.
