# adventure.data — transcription of the Multics printout (Adventure 1.2, May 1980)

Files:
- `adventure.data` — the transcription, laid out as printed (2,699 lines, matching the original file's line count).
- `transcription-source/adventure.src` — the same content in Woods-style TAB-separated form (easier to diff against WOOD0350/advent.dat).
- `transcription-source/render.py`      — regenerates adventure.data from adventure.src (first field right-justified in 8 columns;
                     numeric fields 8 columns each; '@<col>' places text at a fixed column).
- this file        — page-by-page notes, uncertainties, and the ERRATA list for a future working copy.

Summary of findings:
- Base is Woods' 350-point data (via Gary Palter's MIT-Multics port, confirmed by message 1's credits and by the
  code's common blocks), run through a conversion program that wrote fixed-format fields: first field i8 (numbers
  right-justified in 8 columns, text from column 9), numeric tables 8 columns per field (tabs expanded; empty tab
  fields became "0"; "000" became "0").
- "Platt 1984" throughout means the surviving A-code source of Platt's 550-point game, dated 18 Sep 1984
  (PLAT0550 in Quuxplusone/Advent). The game itself dates from 1979 (CP-V at Honeywell LADC, then CP-6); the
  1984 file is simply the earliest copy that survives.
- Platt material, apparently from an earlier stage of his work than that source (how it reached Multics is unknown,
  possibly via Honeywell Multics/CP-6/GCOS contacts, possibly from before the 1979 release; some of it may be
  local), merged in: rooms 141-217, objects 65-98, messages 202-270,
  vocabulary additions; several pieces predate Platt 1984 (ice tunnels "under construction", no safe puzzle,
  permanent wheat-coloured bridge, fog rooms as separate numbered rooms).
- Lines not in Platt 1984 at all: marble corridor/staircase/garden (143-147), crystal medallion, bulletin board.
- Some lines were hand-typed after the conversion (literal "000", new vocab comments at column 12).
- Version "adventure 1.2 (462 points)"; maintainer jjl (Lippard.Scouting@PCO-Multics; printed in lowercase).
- Known data errors kept as printed: see ERRATA (location 185 run-on row; fog room 204 direction 47 twice).
- 2026-09-27: the 35/110 "25 feet away" lines were re-transcribed (ERRATA 5, withdrawn).

Remaining paper checks (optional): nothing blocking. Spacing reconstructed by rule + Jim's counts.

---
# Transcription notes: adventure.data (Multics, May 1980)

Format: line number immediately followed by text, no separator; all lowercase.

## Page 1 (section 1 header through 32)
- Matches Woods 350 (WOOD0350/advent.dat) verbatim apart from case and the dropped number/tab separator, except:
- **29**: "you are in the south side chamber of the hall of the mountain king."
  Woods and every other catalogued version: "you are in the south side chamber."  Genuine variant.
- **15** ("...almost as if alive.  a cold wind..."): Woods has a TAB here, not spaces.
  The printout shows a gap that looks like two spaces; transcribed as two spaces. Unverifiable from paper.

## Pages 2-3 (33 through 97)
- Matches Woods 350 apart from case, except two rooms whose Woods text is replaced by Platt-style text:
- **63**: Woods "dead end" becomes three lines:
  `63dead end` / `63scratched upon a rock is the message "stand where the statue gazes,` / `63and make use of the proper tool".`
  Platt 1984 (DEADEND1) has the same clue in different words: 'Dead end passage. Scratched on a rock is the message, "Stand where the statue gazes, and make use of the proper tool."'  This copy's wording looks earlier.
- **93**: Woods "the passage here is blocked by a recent cave-in." is replaced by Platt's TUNNEL.1 text, identical to Platt 1984:
  "you are in a low tunnel with an irregular ceiling.  to the north, the tunnel is partially blocked by a recent cave-in, but you can probably get past the blockage without too much trouble."
  So the cave-in beyond the giant room is passable in this version, leading to Platt's new area.
- Woods has mid-line TABs in 35, 57, 64 (twice), 68, 97; transcribed as two spaces (same caveat as 15).

## Pages 4-5 (98 through 134)
- Matches Woods 350 apart from case, except:
- **126** (volcano view): one extra final line, `126a wheat-colored stone bridge arches over the gorge.`
  In Platt 1984 the wheat-colored bridge is a separate VOLCANO object that the volcano creates on the fly ("...suddenly solidifies into a fragile-looking arch of wheat-colored stone that bridges the gorge") and can later destroy. Here it is a permanent part of the room description: apparently an earlier, simpler design of Platt's bridge.
- Woods mid-line TABs in 98, 99, 100, 103, 106, 108, 109, 110, 111, 113, 124, 125, 126, 127 transcribed as two spaces (same caveat as 15).

## Pages 6-7 (135 through 178) -- first new rooms
- 135-140: Woods maze text, unchanged.
- 141-178: new rooms (Woods ends at 140). Nearly all correspond to Platt 1984 rooms, usually verbatim.
- Sentence spacing (one vs two spaces) is transcribed as it appears on the paper; it is mixed, and often differs from Platt 1984.
  Spacing on the upper sheet (141-154) was read from a tilted photo and is less certain than on the lower sheet.
- Differences from Platt 1984 worth noting:
  - **142** (minotaur): lacks Platt's final sentence about a ten-foot rock wall below the statue's feet.
  - **143-147**: marble-columned corridor (3 rooms), collapsed stone staircase, overgrown garden. **Not in Platt 1984 at all.** Later cut, or local additions?
    2026-09-27: more likely earlier Platt material, later cut, than a local addition, though not certain. The rooms
    (143-147) and motion words (78, 79) are numbered inside the Platt block, ahead of rooms and words that are in the
    1984 source. The corridor is entered north from Woods' Y2 (33 -> 143), whose description was not updated to
    mention it. (Platt's fake Y2, 177, mentions "a passage to the north", but that is its own north exit to the
    catacombs, 177 -> 179, not evidence about the real Y2. In the 1984 source FAKE.Y2 has no north exit.)
  - **148**: "a deep resonant chanting" (Platt: "a deep, resonant").
  - **150**: "dead.  later" (two spaces; Platt one).
  - **151**: "the ledge once continues south" [sic] (Platt: "continued").
  - **157** Ice room: after "leading downwards to the east", this copy has icicles blocking the slide and a sign reading "ice tunnels under construction - keep out", with sounds of chopping ice. Platt 1984 has the finished ice tunnels. So this copy predates the ice-cave area.
  - **172** Peelgrunt room: adds "there is a large square doorway to the north." (Platt: just "You are in the Peelgrunt room.")
  - **174**: "small empty cubical chamber... a very large safe once rested here". Platt 1984 has the safe as an object instead.
  - **176** vaulted room: "a tunnel leads upwards, and there is a large square doorway to the south." (Platt: "upwards and to the north.")
  - **177**: fake Y2 room adds "a passage to the north" to the exits (Platt FAKE.Y2 copies Woods' 33 exactly).

## Pages 8-9 (179 through end of section 1; section 2 up to 110)
- 179-188 and 191-199: catacombs, 19 identical rooms ("you are in the catacombs. enchanted tunnels lead in all directions."). Platt 1984 has the same text in CATACOMBS.n rooms (two spaces there, one here).
- 189-190 audience hall, 200-208 fog room and its eight colored fogs, 209 cairn, 210 nondescript chamber, 211 pentagram, 212-214 chimney/lava tube, 215-216 wide corridor, 217 Witt Company tool room: all match Platt 1984 in wording, reflowed differently, with single spaces between sentences throughout.
  (In Platt 1984 the fog-room descriptions live in a FOG object instead of in numbered rooms.)
- 189: "egyptian" lowercase, as is everything in this file.
- Section 1 ends at 217 with `-1end` (Woods: `-1<TAB>END`).
- Section 2 (short descriptions) so far matches Woods except:
  - **19**: "you're in hall of mt king." has a period (Woods has none).
  - **28, 29, 30, 93**: new short descriptions not in Woods.
    28 "you're in n/s passage." (Platt: "You're in a low N/S passage.")
    29 "you are in the south side chamber." (= Platt)
    30 "you are in the west side chamber." (Platt: "You're in the west side chamber.")
    93 "you're in low tunnel with irregular ceiling." (= Platt).

## LAYOUT CORRECTION (applies to whole file) -- superseded: see the i8 correction further down
Measuring character positions in the photos shows the line numbers are **right-justified in a 3-column field**
("  1you are...", " 10you are...", "100you are...", " -1end"), not simply run together with the text.
The travel table (section 3) uses the same 3-column first field followed by **8-column right-justified fields**
(e.g. `  8  303009       3      19      30`). That is what a fixed-format Fortran write such as (i3,a) / (i3,7i8)
would produce, rather than Woods' tab-separated free format. Whoever converted the file apparently rewrote it
in fixed columns (plausibly for GCOS fixed-format READs). Check the READ/FORMAT statements in adv.fortran.
- Workflow: I now keep `adventure.src` in Woods' tab-separated form and generate `adventure.data` from it with
  `render.py`, so the layout is applied consistently.

## Pages 10-11 (section 2: 111 to end; section 3 starts)
- Section 2 111-130 matches Woods. New short descriptions for 141-149, 151-177, 179-200, 209-214, 216-217.
  No short descriptions for 150 (death), 201-208 (fog rooms), 215 (corridor), as in the printout.
- **177** short: `you're at "y2"?` (question mark; Platt 1984 has the same).
- **166** short: "room with translucent white walls" (Platt: "translucent walls"; long desc says "translucent whitish").
- Section 2 ends ` -1`; section 3 header `  3`.
- Section 3 rows for locations 1-9 are identical to Woods.
- **Check on paper**: row `6  5  6  46` shows what looks like a dot before the 5 (".5"). Almost certainly a speck; Woods has 5.

## Page 12 (section 3, locations 10-45)
Identical to Woods except (verb numbers: 29 up, 30 down, 43 e, 44 w, 45 n, 46 s, 47 ne, 48 se, 49 sw, 50 nw):
- **15** (hall of mists): Woods `15 150022 29 31 34 35 23 43` loses its final 43, and a new row `15 163 43` follows `15 34 55`.
  So EAST from the hall of mists now leads to 163, the sandstone chamber "to the east of the hall of mists",
  instead of into the dome (22).
- **19** (hall of the mountain king), new rows:
  `19 176 30` (down to the vaulted room), `19 158 50 81` (nw / verb 81 to the division in the passage),
  `19 161 47 82` (ne / verb 82 to the morion room), and `19 215 48` (se to the wide corridor).
  Verbs 81 and 82 are new; section 4 should say what they are.
- **33** (y2): new row `33 143 45` (north to the marble corridor). Note room 33's own description (Woods text) doesn't
  mention a north exit, although the fake y2 (177) does.
- Page ends at `45 43 45`.

## Page 13 (section 3, 45-86)
- Identical to Woods, all 120 rows (`45 46 43` through `86 52 29 11`).

## Page 14 (section 3, 87-126)
Identical to Woods except two added rows:
- `93 162 45`: north from the low tunnel (93, Woods' cave-in dead end) to the glassy-walled room (162,
  "passage enters from the south and exits to the north").
- `126 141 45`: north from the volcano view across the (permanent) wheat-colored bridge to the south end of the
  valley of the stone faces (141). Unconditional here; in Platt 1984 the bridge must first be created.

## Page 15 (section 3, 127-148)
- 127-140 identical to Woods (including the whole different-maze block).
- First rows for the new rooms:
  - 141 (south end of stone-faces valley): n 142, s 126.
  - 142 (minotaur): s 141, nw 200 (fog room), n 168 (narrow rough passage), ne 164 (winding passage).
    No rock wall below the statue here (cf. Platt 1984's extra sentence), so the valley's north end is simply open.
  - 143-145 marble corridor: 143 s->33 (y2), n->144; 144 s->143, n->145; 145 s->144,
    e/verb 78 ->146 (blocked staircase), w/verb 79 ->147 (garden). 146 w->145, 147 e->145.
    So the corridor runs north from y2; this is the not-in-Platt-1984 section.
  - 148 (sorcerer's lair): w->162 (glassy room). Page ends here; lair's east exit presumably continues overleaf.
- Verbs 78, 79 are new (probably "staircase"/"garden"-type words; check section 4).

## Page 16 (section 3, 148-203)
All new rooms; no Woods equivalent. The connections are internally consistent with the room descriptions
(e.g. golden chamber 165: s/ne/nw; peelgrunt 172 doorway n to 174; 174 n to vaulted 176; 176 up to 19).
- `148 92 27 23`: from the sorcerer's lair, GIANT or PASSAGE (Woods verbs 27, 23) takes you to the giant room.
- 177 (fake y2): s 173, e 178 (jumble), w/n 179 (catacombs).
- Catacombs 179-199: each room has one "forward" exit and one row sending every other direction back
  (a one-way chain 179->199->... ; 188 branches to 189-190, the audience hall).
- **Probable data error, check on paper**: the line for 185 reads
  `185 184 29 30 43 44 46 47 48 49 50 199 180 49` (14 fields).
  The last three numbers look like a separate row `199 180 49` (sw from 199 to 180) run onto the end of 185's line:
  199's own row (`199 179 29 30 44 45 46 47 48 50 43`) is the only catacomb row lacking one direction, and it is
  exactly 49. If the program reads this line as written, 199 and 180 become bogus "verbs" for 185->184 and
  the 199 sw exit is lost. Transcribed exactly as printed.
- 200 (fog room entrance): s 142, n 201. 201/202 fan out to the colored-fog rooms 202-209. Page ends at `203 201 43`.

## Page 17 (section 3, 203-217; section 4 starts)
- Fog rooms 201-208: each colored-fog room maps the eight compass directions (43-50) to the other fog rooms
  and the cairn (209), a randomized-feeling "fog maze".
- **204 has direction 47 twice and no 48** (`204 208 47` and `204 207 47`). Confirmed by zoom: both print as 47.
  Every other fog room (203, 205-208) uses each of 43-50 exactly once, so `204 207 47` is probably meant to be 48.
- 201 (previous page) has no north (45) exit and no route to 208; may be intentional, since you enter it from 200 by going north.
- 209 (cairn): down to 210; every compass direction to fog room 206.
- 210-214: nondescript chamber, pentagram, chimney, lava tube; 214 slides down (30, 46) into the fog room entrance 200.
- 215-217: wide corridor from the hall of the mountain king (19) to the bend and the tool room.
- Section 3 ends ` -1`; section 4 (vocabulary) header `  4`. First 55 words (2 road through 36 left) identical to Woods.
  Vocabulary numbers are right-justified in the same 3-column field so far; watch for 4-digit codes (1001 etc.).

# ERRATA (candidate corrections for a working copy; the transcription keeps the printout as-is)
1. Section 3, location 185: `185 184 29 30 43 44 46 47 48 49 50 199 180 49` ->
   `185 184 29 30 43 44 46 47 48 49 50` plus a row `199 180 49` placed with the other 199 rows.
   (Jim checked the paper: the three numbers really are on 185's printed line.)
2. Section 3, location 204: `204 207 47` -> `204 207 48` (47 appears twice for 204, 48 not at all).

3. Message 82: remove the two "0" fields: `      82                      --- poof!! ---` (Woods: three tabs).
4. Section 7, object 89 (wall2) listed three times; only the last line (200 -1) takes effect as the code reads it.
   Decide in the working copy how the three tunnel blockers should work (probably needs code support).
5. (Withdrawn 2026-09-27: a transcription error, not an erratum.) Section 1, locations 35 and 110, the lines
   beginning "25 feet away there is a similar window...". These are the only two message lines in Woods whose text
   begins with a digit (Woods: `35<tab>25 FEET AWAY...`, `110<tab>25 FEET AWAY...`). On paper the digits run
   together ("3525", "11025"). The transcription first right-justified the whole run of digits as though it were
   the location number, giving `    3525 feet away...` and `   11025 feet away...`, and this was listed as a
   conversion bug that the i8 read would misparse as location 3525.
   Arthur O'Dwyer questioned the whitespace, and Jim agreed it was a transcription error. The simpler reading is
   that the file has the location number in its usual 8-column field and the text "25 feet away..." from column 9:
   `      3525 feet away...` and `     11025 feet away...`. That makes the text exactly 70 characters, the full
   `70a1` width, like Woods' line, and the program reads it correctly. adventure.src and adventure.data are
   corrected; the working database never needed a change for this.
## Page 18 (section 4, 37 right through 1054 coins)
- Identical to Woods except seven new motion words after 77 fork:
  `78 stairc`, `78 stairs`, `79 garden`, `80 ice`, `81 divis`, `82 morio`, `83 peelg`.
  These match the new travel verbs: 78/79 marble corridor to staircase/garden, 80 ice room, 81 division,
  82 morion room, 83 peelgrunt. Note "garden" and "stairs" are 6 letters; the program presumably reads only 5.
- 4-digit codes (1001keys...) extend one column further left than 3-digit ones, so the first field is at least
  4 wide. render.py now uses width 4 for the whole file (relative layout unchanged).
- Woods' comments (`(must be next object after "real" rod)` etc.) survive, but at a fixed column instead of after tabs.
  Gap between the 5-char word field and the comment is provisional (4 spaces); check on paper
  (e.g. count spaces between "geyse" and "(same as volcano)").

## Page 19 (section 4, 1055 chest through 2025 fee)
- Woods' objects end at 1064 chain; new object words 1065-1098:
  medal/cryst, opals/caske, helme, ring/mithr, jade/brace, scept, yacht, bulle/board (1080), vial, mushr, slime,
  slime2 (real slime), ogre, sword, ogre2 (real ogre), wall (wall description), wall2 (real wall), flask, teeth,
  gobli/goose, basil, basl2, skele, penta, djinn, plate.
  Treasures appear to be 1065-1071 (medal/crystal, opals/casket, helmet, mithril ring, jade bracelet, sceptre, yacht);
  1080+ are non-treasure props. "slime2", "ogre2", "wall2" are 5+ letters.
- Verbs 2001-2025 identical to Woods except two added synonyms: **2012 slay** (after strik) and **2018 stop** (after quit).
  (Both are in Platt 1984: `VERB KILL,ATTACK,FIGHT,HIT,STRIKE,SLAY` and `VERB QUIT,STOP,Q`. SLAY is also already in
  Palter's port as preserved in HORV0350 (`2012 SLAY`); STOP is not, HORV0350 keeps Woods' `3139 STOP`.)
- **Comment columns**: the new comments `(real slime)`, `(real ogre)`, `(wall description)`, `(real wall)` all start at
  column 12 (right after "slime2 "), measured cleanly. Woods' four comments on the previous sheet measure at about
  column 13-14; set to 14 (Jim leans toward 4 spaces after "geyse"). So the two batches were likely typed separately.
  render.py now stores an explicit comment column per line.

## Page 20 (end of section 4; section 5 objects 1-24)
- Vocabulary 2025 fie ... 3147 swim matches Woods except:
  - new action verbs **2032 fast, 2033 full, 2034 listen, 2035 turns** (the "new commands" advertised in message 202),
    of which FAST and FULL are in Platt 1984 (same meanings); LISTEN and TURNS are not,
    plus **2036 phugg** and **2037 melen** (melenkurion; cf. the tool room's "melenkurion division").
  - **3139 stop is gone** (it became 2018 stop, a synonym of quit).
  - "3079 fucke" added after assho.
  - new special words: **3203 games/game**, **3204 wumpu/exorc/erolp** ("explore" backwards), **3205 nosid/samoh/djinn**
    ("noside samoht" from the sorcerer's lair = "thomas edison" backwards), and **3244 .** (the version-number command
    from message 202; "." for identification/version is a Multics subsystem convention, cf. `ssu_`'s "." request).
  - "3050 abra" has a speck after it on the photo; transcribed as "abra" (Woods). Check paper.
- Section 4 ends `  -1`, section 5 header `   5`.
- Section 5 objects 1-24 identical to Woods; Woods' "000" state numbers print as "0" in the right-justified field.

## Page 21 (section 5, objects 24-85)
- Objects through 64 (golden chain) identical to Woods. Woods' internal TABs (500 "...plant!<TAB>it's...",
  100 "hands!<TAB>(unbelievable...") transcribed as two spaces.
- New objects 65-71 (treasures) and 80-85, mostly matching Platt 1984 wording, with differences:
  - **65 crystal medallion** ("on a silver chain"): not in Platt 1984.
  - 70 sceptre: this copy's comment "(sceptre's 0-mode description is part of skeleton)", one space after >$<;
    state 100 ends with "." (Platt "!").
  - 71 yacht state 100: "the ruby-covered toy yacht of omar khayyam is here!" (Platt: "The ruby yacht of Omar Khayyam is sitting on the floor!").
  - **80 *bulletin board** "nailed to a tree near the building": not in Platt 1984. Presumably where the news goes.
  - 81 vial, 82 mushroom, 83 slime (= Platt), 84 real slime, 85 ogre (= Platt).
- **Literal "000"**: object 80's line and both of 83's lines print `000` (right-justified: ` 000`), while every
  Woods-derived line and the other new objects print `0`. So the file is not a uniform machine reformat:
  Woods' lines (and some additions) went through a conversion that turned 000 into 0, while at least these
  lines were typed in afterwards following Woods' "000" convention. Kept as printed (src stores "000").

## Page 22 (objects 86-98; section 6 messages 1-47)
- New objects 86-98: singing sword, *ogre2, *wall (the rock wall under the minotaur is an OBJECT here, with state 100
  "dark tunnels lead northeast, north, and northwest."; so room 142's missing sentence lives in the object),
  *wall2, earthenware flask ("london dry"), dragon's teeth, *gooseberry goblins, *basilisk, *basilisk2,
  *skeleton (holds the sapphire sceptre), *pentagram, *djinn (in the flask), metal plate.
  Another literal `000` line: object 87's `000>$< (never printed)`.
- Section 5 ends `  -1`; section 6 (messages) header `   6`.
- **Message 1 (intro)**: Woods' first eight lines unchanged; the "- - -" separator is positioned by column
  (transcribed with 30 spaces, i.e. Woods' 3 tabs + 6 spaces expanded from the text start; photo measurement
  suggests about one column further right, so ±1). Woods' closing credit
  ("this program was originally developed by willie crowther ... contact don ...") is REPLACED by:
      if you have any problems, suggestions, or questions, please contact
      jim lippard (lippard.scouting@pco-multics). you may also
      contact the developers:
      willie crowther (crowther@parc-maxc), don woods (don@su-ai), and
      gary palter (palter.multics@mit-multics).
      if you have any problems, contact any of us...
  **This names Gary Palter (MIT-Multics) as a developer: strong evidence the base is Palter's Multics port (PALT0350).**
  Crowther's address "parc-maxc" (Xerox PARC) is also new relative to Woods.
- Messages 2-47 identical to Woods (message 11's internal TAB transcribed as two spaces).

## Page 23 (messages 48-124, first part)
- Message 1 separator corrected per Jim: the "- - -" dashes sit under the n, u, e of "adventure" in the line above.
- Messages 48-124 identical to Woods except:
  - **54**: "okay" (Woods "ok").
  - **66**: Woods' typo "ddigging" corrected to "digging".
  - **82**, the POOF line. Woods: `82<TAB><TAB><TAB>    --- POOF!! ---` (empty tab fields).
    Printout: ` 82       0       0    --- poof!! ---`, i.e. each empty field became a right-justified 8-column "0".
    **This is the signature of a conversion program** that split Woods' lines on tabs and wrote each field in a
    fixed-format integer slot, printing blank fields as 0. It also explains the "000" -> "0" change on every
    Woods-derived object line: the state number was parsed as an integer. Lines still reading "000" must have been
    typed in by hand after that conversion. (Message 1's "- - -" line has no zeros, so it was apparently hand-fixed
    or converted differently.)
- Internal TABs in 51, 63, 64, 65, 84, 92, 121, 124 printed as (apparently) two spaces.
- Page ends mid-message 124 ("...(i never was very good at identifying").

## Page 24 (messages 124-185)
- Identical to Woods except:
  - **139 removed** (Woods: 'i don't know the word "stop".  use "quit" if you want to give up.'), consistent with
    "stop" becoming a synonym of quit in the vocabulary.
  - **142 (info) rewritten**: the one-sentence suspend/hours lines become a longer passage explaining
    suspend/pause/save, restoring by starting a new game and typing "restore" after "you are standing...",
    and named saves ("suspend mine" / "restore mine"); and three new closing lines about "fast", "full" and "turns".
    The body of Woods' 142 (scoring, hints, "brief") is unchanged.
  - **143 removed** (Woods: "do you indeed wish to quit now?").
- **132**: Woods has a TAB after "smoke). . . ."; the gap on paper looks wider than two spaces. Transcribed
  provisionally with 5 spaces ("smoke). . . .     as your eyes"). **Check on paper.**
- Other internal TABs (136 twice, 153, 163) transcribed as two spaces.

## Tab-expansion rule (resolves the spacing questions)
Jim counted 4 spaces in message 132 where Woods has a TAB. Expanding Woods' TABs to 8-column stops *measured from
Woods' own layout* (number, TAB, text starting at column 9) gives exactly 4 there, and exactly 2 spaces for every
other internal TAB in Woods' text (rooms 15, 35, ... messages 11, 51, 63, ...). So the converter expanded tabs
before re-laying the lines out. The same rule puts Woods' four vocabulary comments at column 13
(e.g. "1037geyse   (same as volcano)" = 3 spaces), so they are now set to column 13, not 14.
The new vocabulary comments (real slime etc.) are at column 12, i.e. one column left of Woods'. Worth a paper check.

## Page 25 (messages 186-233)
- 186-201 identical to Woods; 201 is Woods' last message.
- 202 (news, "translated from 'spelunker today', may 1980 issue ... version 1.2 ... please contact jjl.sct"):
  first four lines are indented; positions estimated from the photo and **need a paper check** (count leading
  spaces after "202" on each): translated (13), "-   -   -" (29, with 3 spaces between dashes, unlike message 1's "- - -"),
  "*** more new features ***" (21), "the cave's most recent" (5).
  Note "Over 50 new" has a capital O in an otherwise all-lowercase file.
- 203 GAMES list; 204 "wrong game."; 205 "wrong version."; 206-212 LISTEN responses; 213-233 Platt material
  (slime, vials x4, mushroom, sword/ogre, axe/sword zot, dwarf-banishing effects).
  Sentence spacing is mixed (one or two spaces), as printed.
- 202 header positions set from Jim's counts plus line-to-line comparison: translated at +13, dashes at +31
  ("-  -  -", two spaces between; they sit under the p, u, e of "spelunker"), "***" at +20, "the cave's" at +5.

## Page 26 (messages 234-260)
- 234-238: the rest of Platt's dwarf-banishing effects (fireball, sabre-toothed tiger, toothy mouth, goo).
- 239 jellyfish and 240 "cave destroyed" / Ralph the elf: Platt 1984 text reflowed, blank lines dropped,
  single-spaced, and Platt's typo "that work near water" (twice) reads "that word near water" here.
- 241 giant green tentacle; 242-243 the minotaur's wall.
- **244 "adventure 1.2 (462 points)"**: the version string printed by "." — **the maximum score is 462.**
- 245-249 gooseberry goblins and dragon's teeth; 250-253 basilisks.
- **254**: skeleton's dying whisper is **"it's not a bug, it's a feature!"**; Platt 1984 has two variants,
  "Remember - #!" (the safe combination) and "You blew it!". So in this version the sceptre/safe puzzle doesn't exist yet
  (consistent with the "safe once rested here" room).
- 255-256 fog-room hint (= Platt, reflowed); 257-260 first four of Platt's eight fog-glow directions.
- 202 dashes corrected per Jim: under the s, l, k of "spelunker" (+30).

## Page 27 (messages 261-270; section 7 object locations 1-96)
- 261-264: remaining fog-glow directions. **262 "a dim light is visible in the southeast" has no final period** (Platt has one).
- 265-270 flask/djinn/pentagram (= Platt text reflowed, lowercase, single-spaced). Differences:
  - 267 "the flask is already open!" and 269 "there's nothing to let out of the pentagram!" are this version's own wording
    (Platt: "The # is already open!", "The pentagram is empty - there's nothing to let out!").
  - 270: the grateful djinn gives the PHUGGG lore immediately (Platt 1984 instead gives a Ralph Witt "history lesson"
    and delivers the phuggg lore later as a separate PHUGGG.DATA message). "should never be used near water or any
    sharp weapon" (Platt: "near water or near any sharp weapon").
- Section 6 ends ` -1`; section 7 header `   7`.
- Section 7, objects 1-64: identical to Woods. New objects:
  65 medallion @147 (garden), 66 opals @155 (very small chamber), 67 helmet @161 (morion room), 68 ring @0 (created by the sword),
  69 jade bracelet @166 (translucent room), 70 sceptre @190 (audience hall east, with skeleton), 71 yacht @210 (nondescript chamber),
  80 bulletin board @1 fixed, 81 vial @159, 82 mushroom @160, 83 slime @153 fixed, 84 real slime @154 fixed,
  85 ogre @162 fixed, 86 sword @163, 87 ogre2 @148 fixed, 88 wall @142 fixed,
  89 wall2 fixed in **three** rows (164, 168, 200: the three dark tunnels beyond the minotaur's wall),
  90 flask @167, 91 teeth @0, 92 goblins @0 fixed, 93 basilisk @168, 94 basilisk2 @169, 95 skeleton @190, 96 pentagram @211.
  (Object 89 appearing three times is unusual for the Woods engine; check how the code reads it.)
- Page ends at 96; 97 djinn and 98 plate presumably follow.

## Page 28 (end of section 7; sections 8-12; end of file)
- Section 7 ends: `97 211 -1` (djinn at the pentagram room, fixed; the -1 measured onto the 97 line, not 98), `98 175` (metal plate in the storage room), ` -1`.
- Section 8 (action defaults) identical to Woods (1-31; no entries for new verbs 32-37).
- Section 9 (location conditions):
  - lit rooms: `0 100 115 116 126` extended with **166 209** (translucent room lit through its wall; the cairn of glowing rocks).
  - bit 7 (maze-hint) adds the catacombs: `7 180 ... 188 191` and `7 192 ... 199` (189-190 audience hall excluded, 179 not listed).
  - **new bit 10** for the fog rooms 201-208.
- Section 10 (rankings): Woods thresholds unchanged (35 ... 349, 9999) despite the 462 maximum;
  100: "your score qualifies you as a novice adventurer." (Woods "novice class adventurer").
- Section 11 (hints): Woods 2-9 plus **`10 25 5 255 256`**: hint 10 (fog rooms) after 25 turns, costs 5 points,
  question 255 "having problems?", answer 256 (the philosophy hint).
- Section 12 (magic messages) = Woods except:
  15 "the new version has been saved, thank you..." (Woods: "okay.  you can save this version now.");
  23 rewritten: messages now end with a line containing a single period (Woods: "end with null line");
  30 minimum value "(30 minutes)" (Woods 45).
  These fit a Multics adaptation (saving happens in the program; terminating input with "." is Multics-style).
- File ends ` -1` / `   0`. Total 2,697 lines on paper per Jim's count of 2,699: see line-count check below.

## Line-count check
Jim's count for adventure.data is 2,699 lines. After the last page the transcription had 2,697. Comparing per-room
line counts against the photo of rooms 135-178 showed rooms **139** ("you are in a maze of little twisty passages,
all different.") and **140** ("dead end") were missing (dropped when copying Woods' text). Restored; the
transcription now has **2,699 lines, matching the file**. Each two-sheet photo holds 120 lines (60 per sheet).

## CORRECTION from the code (adventure_.fortran page 4): first field is 8 columns
The read formats are `format(i8)` (section headers), `format(1i8,70a1,a1)` (text), `format(22i8)` (travel,
object locations etc.) and `format(i8,5a1)` (vocabulary). So every line's leading number is right-justified in
**8** columns and text starts in column 9. The paper can't show leading blanks against the page margin, which is
why the photos looked like a 3/4-column field. adventure.data has been regenerated with an 8-column first field;
vocabulary comments move to column 17 (Woods' four, exactly where Woods' tab would put them) and 16 (the new ones).
Consequences:
- Woods' tab expansion rule (tabs to 8-column stops, text starting at column 9) now matches the file's real columns.
- **Message 82** is read with 70a1 after the i8, so its text is literally `       0       0    --- poof!! ---`:
  the game would print "0 0 --- poof!! ---" with the zeros. Conversion bug; add to ERRATA.
- Section 7 is read as (obj, j, k) only. Object 89's three lines (164, 168, 200) simply overwrite each other; only
  the last (200, fixed) takes effect. Add to ERRATA as a probable bug (the wall2 object was meant to be in three places).
- Section 3 line for 185 has 14 numbers: 22i8 reads it fine, so 199 and 180 are taken as verbs for 185->184 (as feared).
