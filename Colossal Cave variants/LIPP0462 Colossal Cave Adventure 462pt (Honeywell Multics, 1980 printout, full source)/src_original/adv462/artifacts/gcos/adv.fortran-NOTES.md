# adv.fortran transcription notes

**Jim's handwritten note on the cover sheet reads "GCOS version".** So this is a port of Don Woods' 350-point Adventure to Honeywell GCOS. It is separate from the Palter-based Multics adventure_.fortran.

26 photos. Photo 1 is a single sheet, photos 2–25 are two sheets each, and photo 26 is a final sheet with two lines.

## Layout (measured from glyph positions)
- Sheets are exactly **60 lines**, counting blank lines. Photos 1 and 2 check out: 60 + 60 + 60.
- Columns:
  - "c" is in column 1 and statements start in column 7.
  - The continuation mark "1" is in column 6.
  - The column where continuation text starts varies, so it was measured line by line: 17 on page 1, and 15 or 20 on page 2.
- Tabs in comments print at stops **every 7 columns: 7, 14, 21, 28, 35, 42**. This is how Woods' `C<tab>`, `C<tab><tab>` and `<tab>` text lines come out.
- **A tab past column 42 prints as a small raised mark**, transcribed as `^`. Jim confirmed these are carets. See `34~35~23~43` in the travel-table example, where Woods has four more tabs.
- The statement-function comment lines ("toting(obj) = ...") have "=" in column 16 on every line, including pct(n). The 7-column tab rule does not produce this. These lines must have been retyped with spaces.
- 12000 in the limits comment is in column 8, because Woods used spaces on that line.

## Page 1 (photo 1, single sheet): header comments, common blocks, dimensions, start of statement-function comments
- **This is Don Woods' original main program, not Gary Palter's.**
  - The common blocks are Woods' own: /txtcom/, /blkcom/, /voccom/, /placom/, /mtxcom/, /ptxcom/, /abbcom/ and /wizcom/ (wkday … setup).
  - None of Palter's /msccom/, /ioscom/ or /mtdcom/ appears.
  - So adv.fortran is a separate lineage from adventure_.fortran, not another module of it.
- Text is identical to WOOD0350 advent.for, lowercased, except for:
  - Message-text limit 9650 → **12000**, in both the comment and `dimension lines(12000)`.
  - `logical ... start,zzz`: **zzz** is added.
  - **New:** `character*4 wd1,wd2,iz`. This is Fortran 77, so words are held as 4-character strings rather than A5 words packed into integers.
  - **New:** `equivalence(iz,izz)`.
- Woods has a form feed (^L) before "c  statement functions". There is no page break or blank line there on the paper.
- The page ends after the `liq(dummy)` comment line.

## Page 2 (photo 2, two sheets): statement functions, DATA statements, database-format comments through section 10
- The text is Woods' text, lowercased, with these changes:
  - `bitset(l,n)=and(cond(l),shift(1,n)).ne.0`. Woods used the DEC `.and.` operator on integers; this uses an and() function instead.
  - `data linsiz/12000/`.
  - **New:** `data bl/' '/,izz/0/`.
  - The continuation lines (logical, dark, data) are re-indented. Woods' `1  vrbsiz` becomes column 15.
- Woods has a form feed before "c  description of the database format". There is no page break on the paper.
- The first sheet ends at "if 100<m<=200 he must be carrying object m-100."
- The second sheet ends at "... a classification of player.  the scoring section".

## Page 3 (photo 3, two sheets): section 11–12 comments, database read setup, section 1/2/5/6/10/12 reader, start of section 3
- Each sheet is 60 lines; the running total is 300 lines, i.e. 5 sheets.
- Changes from Woods:
  - `type 1000` becomes **`print 1000`**. `format(' initializing...')` is lowercase; Woods has a capital I. The whole file, strings included, seems to have been lowercased.
  - `open(unit=1,name='advent',access='seqin')` becomes **`call attach (1,"/adv.data;",1,0,stat,)`**. This is the GCOS file-attach call. The data file is `/adv.data`, and the stray trailing comma is as printed.
  - `1003 format(g)` becomes **`format(i4)`**.
  - Woods' `call bug(9)` after the computed goto is moved in front of it as **`if(sect.gt.12) call bug(9)`**.
  - The computed goto continuation has a comma: `1080,1004),(sect+1)`.
  - The message reader is rewritten for 4-character words: `read(1,1005)loc,(lines(j),j=linuse+1,linuse+18),kk`, `1005 format(i4,19a4)` and `do 1006 k=1,18`. Other changes: `kk=linuse+19-k`, `.ne.bl` in place of `.ne.' '`, and `if(linuse+18.gt.linsiz)`. Woods used 14 five-character words with `format(1g,15a5)`.
  - Woods' F40 workarounds are removed: `if(loc.eq.0)goto 1004` and its "above kluge…" comment in the reader, and `if(loc.eq.0)goto 1030` and its comment in section 3.
  - Section 3 now reads `read(1,1031)loc,newloc,(tk(l),l=1,9)` with **`1031 format(i4,10i7)`**. Woods used list-directed `(99g)`.
- The first sheet ends with the `(0) (1) ... (10)` comment. The second sheet starts with `(11) (12)` and ends at `key(loc)=trvs`.

## Page 4 (photo 4, two sheets): end of section 3 reader, sections 4, 7, 8, 9 and 11, and the start of internal setup
- Each sheet is 60 lines; the running total is 420 lines.
- Changes from Woods:
  - Travel loop: `do 1037 l=1,9` (Woods: 1,20). This matches the 9 `tk` values now read per line.
  - Vocabulary reader:
    - It reads `format(i4,a4)`.
    - The F40 workaround (`if(ktab(tabndx).eq.0)goto 1043` and its comment) is removed.
    - **Woods' hash, `1042 atab(tabndx)=atab(tabndx).xor.'phrog'`, is replaced by `1042 continue`.** Words are stored in plain text. Woods' comment about "a minimal hash to make reading the core-image harder" and '/7-08' was left in place, so it is now stale.
  - Section 9 reads `k,(tk(i),i=1,10)` with `do 1071 i=1,10` (Woods: `k,tk` and 1,20).
  - Section 11 reads `k,(tk(i),i=1,4)`. The F40 workaround `if(k.eq.0)goto 1081` is removed.
  - **New: `call detach(1,stat,)` after `if(setup.eq.-1)goto 8305`.** It takes the place of Woods' blank line there, so the data file is released once it has been read.
- The first sheet ends at `1083  hints(k,i)=tk(i)`. The second sheet ends at `do 1200 i=50,maxtrs`.

## Page 5 (photo 5, two sheets): treasure/hint init, vocabulary mnemonics, dwarf init, start of the flag/counter comments
- Each sheet is 60 lines; the running total is 540 lines.
- **Mnemonics use 4-character strings in place of Woods' `0+'xxxxx'`**, e.g. `grate=vocab('grat',1)` and `dprssn=vocab('depr',0)`.
- Three calls still pass **5-character** strings: `knife=vocab('knife',1)`, `coins=vocab('coins',1)` and `chest=vocab('chest',1)`. They are transcribed as printed. With `character*4` words they would presumably be truncated to 'knif', 'coin' and 'ches', or fail to match. Whether this is a bug depends on how vocab handles its argument; that should be checked when we reach vocab.
- **New:** `spices=vocab('spic',1)` after chain. Woods has no spices mnemonic, yet uses `prop(spices)` in the closing-time code (`if(prop(spices).lt.0)tally2=tally2+1`). In Woods, spices is therefore an uninitialized variable. **The GCOS porter fixed this Woods bug.** This line is the first line of the second sheet.
- **`#1700` label:** the line is printed as ` #1700    dseen(i)=.false.`. Column 1 is blank, "#" is in column 2, 1700 is in columns 3–6, and the statement starts in column 11 (measured). Woods has `1700<tab>DSEEN(I)=.FALSE.`. Where the "#" came from and what it means is not known. Possibly an editor marker, or a disabled label? It should be asked about or checked against the paper.
- The dwarf-activation and counter tables follow the 7-column tab rule, confirmed by measurement: the value/name is in column 7 and the description in column 14.
- The first sheet ends at `chain=vocab('chai',1)`. The second sheet ends at the `clock1` comment line.

## Page 6 (photo 6, two sheets): counter init, table-space report, start-up, the dwarf-blocking check
- Each sheet is 60 lines; the running total is 660 lines. The last line of the second sheet is blank, which is Woods' blank line before the "when we encounter the first dwarf" comment.
- Changes from Woods:
  - `do 1800 i=1,5` / `1800 if(rtext(2*i+79).ne.0)maxdie=i`. Woods has `i=0,4` / `rtext(2*i+81)` / `maxdie=i+1`. The meaning is the same, rewritten with a 1-based loop.
  - `type 1999` becomes `print 1999`. The continuations are `     1 ,locsiz,...`, with the text in column 8.
  - The 1999 format is entirely lowercase: 'table space used:', ' of ', 'rtext messages', 'class messages', 'magic messages'. Woods has mixed case ('Table space used:', ' OF ', 'RTEXT', 'CLASS', 'MAGIC').
  - `pause 'init done'` becomes **`print,'init done'`**, list-directed with a comma after print.
  - Woods' form feed before "c  start-up, dwarf stuff" is dropped, with no blank line in its place, as in Woods.
- The first sheet ends at format continuation 3 (vocabulary words). The second sheet ends with the blank line after `goto 2000`.

## Page 7 (photo 7, two sheets): first dwarf encounter, dwarf movement, pirate, knife attacks
- Each sheet is 60 lines; the running total is 780 lines.
- Changes from Woods:
  - Woods' logical continuation `dseen(i)=(dseen(i).and.loc.ge.15) / 1 .or.(...)` becomes two statements using the new `zzz` variable:
    `zzz=(dseen(i).and.loc.ge.15)` / `dseen(i)=zzz.or.(dloc(i).eq.loc.or.odloc(i).eq.loc)`.
    The line count is unchanged.
  - `type 67` and `type 78` become `print`. The formats are lowercased: ' there are ' (Woods: ' There are ').
  - The comment "dwarves get *very* mad" has **no final "!"** (Woods: "MAD!"). It is not a column-72 cutoff, because longer comment lines print in full elsewhere.
- Continuation text columns measured on this page:
  - `     1   .or....` and `     1   .and....` have their text in column 10.
  - `     1  call carry(...)` is in column 9.
  - `     1,' room with you.')` is in column 7.
- Photo quirk: the paper is wavy near the 6012 block, so the lines tilt and some appear to overlap in the photo. They read exactly as Woods.
- The first sheet ends with the "pirate won't take pyramid" comment. The second sheet ends at `call rspeak(k+stick)`.

## Page 8 (photo 8, two sheets): knife results, location description, hints check, main input and command preprocessing
- Each sheet is 60 lines; the running total is 900 lines.
- Changes from Woods:
  - `type 68` becomes `print 68`. The form feed before "c  describe the current location" is dropped.
  - **Comment wording:** "put down **seperate** from their **seperate** piles" (Woods: "separate from their respective piles"). None of the ~10 versions in the Advent archive has this wording. It may come from an earlier Woods revision, or from whoever typed or edited this copy.
  - 4-character word tests:
    - `wd1.eq.'magi'.and.wd2.eq.'mode'` (Woods: 'MAGIC').
    - `'ente'`, `'stre'`, `'wate'`, `'plan'`, `wd2='pour'`.
    - `'oil'`, `'door'` and `'west'` are unchanged.
  - Blank-word tests use `iz`: `wd2.ne.iz` (Woods: `wd2.ne.0`) in the SAY check and the ENTER check.
  - **A second `#` label: ` #19999  k=43`.** "#" is in column 2, 19999 in columns 3–7, and `k=43` in column 10 (measured). It is like ` #1700` on page 5, and its meaning is still unknown.
- Continuation text columns measured on this page:
  - Column 10: `1   call pspeak`, `1   prop(idondx)`, `1   goto 2010`.
  - Column 8: `1 .and.here(lamp)`, `1 .or.(wd2.ne....`.
- The first sheet ends with "c  check if this loc is eligible for any hints…". The second sheet ends at `2630 i=vocab(wd1,-1)`.

## Page 9 (photo 9, two sheets): word dispatch, verb/object analysis, start of motion code
- Each sheet is 60 lines; the running total is 1020 lines.
- Changes from Woods:
  - **Range checks moved in front of the computed gotos.** The pattern is the same as `if(sect.gt.12) call bug(9)` on page 3.
    - `if(kq.gt.4) call bug(22)` now comes before `goto (8,5000,4000,2010),kq`. That goto has a comma before `kq`.
    - `if(verb.gt.31)call bug(23)` is inserted in the 4000 block before `if(obj.ne.0)goto 4090`. Woods' `call bug(23)` after the 4080 goto is removed.
    - **Woods' `call bug(24)` after the 4090 goto is removed with no replacement**, so a transitive verb > 31 is not caught. The 4080 and 4090 gotos both end `),verb`.
  - **`if(verb.eq.say)obj=conv(wd2)`** (Woods: `obj=wd2`). A new `conv` function presumably converts the character*4 word into an integer so it can be kept in `obj`. It should be looked for later.
  - `wd2=iz`, `wd2.ne.iz` and `wd2.eq.iz` replace Woods' `0`.
  - `a5toa1` gets an extra argument. The 4-character port splits Woods' 5-character suffix across two words:
    - `call a5toa1(wd1,wd1x,'?',' ',tk,k)` (Woods: `'?'`).
    - `call a5toa1(wd1,wd1x,' her','e.',tk,k)` (Woods: `'here.'`).
  - `type 5015/5199` becomes `print`. The formats are lowercase: ' what do you want to do with the ', ' i see no '.
  - The form feed before "c  figure out the new location" is dropped.
- The verb-name comment rows follow the tab rule: `C<tab>     TAKE` puts "take" in column 12. The goto continuations are `     1    2011,...`, with the text in column 11.
- The first sheet ends at `5000 obj=k`. That is one Woods line further than a straight count, because `call bug(24)` is gone. The second sheet ends at `if(newloc.le.300)goto 13`.

## Page 10 (photo 10, two sheets): travel resolution, special motions 301–303, go back, look, cave
- Each sheet is 60 lines; the running total is 1140 lines.
- **Every 5-digit label is printed with "#":** ` #30000`, ` #30100`, ` #30200`, ` #30300`, ` #30310`, like ` #19999` on page 8.
  - Five digits plus "#" in column 2 run to column 7. That overlaps the statement field of standard fixed form.
  - So "#" is probably the GCOS compiler's way of marking a statement label that doesn't fit in columns 1–5. A guess: GCOS time-sharing Fortran's line format. This needs confirming.
  - ` #1700` on page 5 is the only 4-digit label marked this way.
- Statement start columns after a "#" label (measured):
  - ` #30000 newloc=...` has one space, so the statement is in column 9.
  - `#30100`, `#30200`, `#30300` and `#30310` have their statement in column 15.
  - The 4-digit ` #1700` puts its statement in column 11, and ` #19999` in column 10.
- **Two Woods comment lines merged, and the line cut off at column 80.** The printout shows
  `c  special motions come here.  labelling convention: statement numbers nnnxxc  (`.
  Woods' next line, "C  (XX=00-99) ARE USED FOR SPECIAL CASE NUMBER NNN (NNN=301-500).", has been appended and cut off after column 80 at the "(". So the source file (or the listing) was limited to 80 columns. That also explains why no other line runs past column 79/80.
- `goto (30100,30200,30300)newloc` / `call bug(20)` becomes `if(newloc.gt.3)call bug(20)` / `goto (...),newloc`. This is the same bug-check-first pattern.
- The comment reads "c  travel302." with no space (Woods: "TRAVEL 302.").
- ` call drop(bear,newloc)` in 30310 is indented one extra column: column 8, measured.
- Each sheet is 60 lines. The first sheet ends with the blank line after `goto 2` (end of 30300). The second sheet runs from `#30310` to `40 ... goto 2` (59 lines) plus Woods' following blank line.

## Page 11 (photo 11, two sheets): non-applicable motion, death and reincarnation, start of action verbs (8000, carry)
- Each sheet is 60 lines; the running total is 1260 lines. Photo 11 opens with "c  non-applicable motion". That confirms page 10's second sheet ends with Woods' blank line.
- Changes from Woods:
  - **Another "!" dropped from a comment:** "...back into the cave without the lamp)." (Woods: "LAMP!)."). Earlier: "dwarves get *very* mad" (Woods "MAD!").
    - Not every "!" is gone: "(too easy!)" and "don't steal chest back from troll!" keep theirs.
    - Jim's view of the comment variants, "seperate/seperate" included: either this copy predates Woods' corrections, or someone introduced the errors.
  - `8000 call a5toa1(wd1,wd1x,' wha','t?',tk,k)` (Woods: `'What?'`). The 4-character split is the same as `' her','e.'`. Also `type 8002` becomes `print 8002`.
  - The form feeds before `"you're dead, jim."` and "routines for performing the various action verbs" are dropped.
- Continuation `     1   call carry(bird+cage-obj,loc)`: the text is in column 10.
- The first sheet ends with "c  routines for performing the various action verbs". The second sheet starts with Woods' blank line and ends with the second line of the "discard object" comment.

## Page 12 (photo 12, two sheets): drop/throw special cases, say, lock/unlock, clam/oyster, chain
- Each sheet is 60 lines; the running total is 1380 lines.
- Only the SAY code (9030–9035) differs from Woods, apart from the lowercasing:
  - The `a5toa1` calls take the extra argument: `call a5toa1(wd2,wd2x,'".',' ',tk,k)` (Woods: `'".'`).
  - `if(wd2.eq.iz)` / `if(wd2.ne.iz)` / `9035 wd2=iz` (Woods: 0).
  - `type 9032` becomes `print 9032`. The format reads `' okay, "'` (Woods: `' Okay, "'`).
- The first sheet ends at `9032 format(...)`. The second sheet ends at `fixed(chain)=0`.

## Page 13 (photo 13, two sheets): chain lock, lamp on/off, wave, attack (incl. dragon), start of pour
- Each sheet is 60 lines; the running total is 1500 lines.
- The code is Woods' code, lowercased. The only textual change is `if(wd1.ne.'y'.and.wd1.ne.'yes')goto 2608` (Woods 'Y'/'YES'). These are plain lowercase strings, and 'yes' still fits in 4 characters.
- Continuations in 9090 and the dragon loop are written with their text in column 10, measured: `1   spk=29`, `1   .or.closng)`, `1   place(idondx)...`, `2   call move(idondx,k)`. Woods had `<tab>1 PLACE` / `<tab>2 CALL` with single spaces.
- Here "win!" keeps its "!" ("if he insists on attacking it, win!"). So the missing "!"s elsewhere are not a blanket removal.
- The first sheet ends with "c  clam and oyster both treated as clam…". The second sheet ends at `if(obj.ne.water)goto 2011` in 9130.

## Page 14 (photo 14, two sheets): pour (door), eat, drink, rub, throw, quit, find, start of inventory
- Each sheet is 60 lines; the running total is 1620 lines.
- The code is Woods' code, lowercased.
- **Woods' blank line between `goto 2011` (end of 8140/8142) and `9140` is absent.** The first sheet only reaches 60 lines if this is so.
- **Another dropped "!":** "(only way to do so)" (Woods: "(ONLY WAY TO DO SO!)"). "this'll teach him to throw the axe at the bear!" keeps its "!".
  - Dropped so far: "mad!", "lamp!)", "so!)".
  - Kept so far: "too easy!", "troll!", "win!", "bear!".
- Continuations in 9140, 9150 and 9190 are transcribed with their text in column 10 (`1   .or....`), the same as the measured instances elsewhere.
- The first sheet ends at `obj=0` in 9170. The second sheet ends at `if(spk.eq.98)call rspeak(99)` in 8200.

## Page 15 (photo 15, two sheets): end of inventory, feed, fill, blast, score, fee-fie-foe-foo, brief
- Each sheet is 60 lines; the running total is 1740 lines.
- The code is Woods' code, lowercased, except for:
  - `type 8243` becomes `print 8243`. The format text is lowercase: ' if you were to quit now, you would score' (Woods 'If'). The continuation is `     1 ,' out of…'`, with the text in column 8.
- The 8252 continuations are indented further than elsewhere: `     1       .or.(toting(eggs)...` and `     1       prop(troll)=1`. Their text is in **column 13**, measured on both lines.
- "which is a neat trick!" keeps its "!".
- The first sheet ends at `goto 2011` (end of 9220). The second sheet starts with Woods' blank line before 9222 and ends at `detail=3` in 8260.

## Page 16 (photo 16, two sheets): read, break, wake, suspend, hours, hints, start of cave-closing comments
- Each sheet is 60 lines; the running total is 1860 lines.
- **Suspend is kept:**
  - `type 8302` becomes `print 8302`, and the format text is lowercase ("i can suspend your adventure…").
  - `call datime(saved,savet)` / `setup=-1` are the same as Woods.
  - **`call ciao` becomes `call ciao(setup)`.** The GCOS `ciao` is passed `setup`, presumably to save the core image or state.
  - `8305 yea=start(0)` is the restart entry, as in Woods.
  - `ciao`, `datime`, `start`, `mspeak` and `hours` should be looked for later in this file.
- **Hints dispatcher:**
  - `40000 goto (...)(hint-3)` / `call bug(27)` becomes ` #40000 if(hint.lt.4.or.hint.gt.9) call bug(27)` / **`go to (…),(hint-3)`**. "go to" is written with a space here, unlike everywhere else.
  - `type 40012` becomes `print 40012`, with lowercase text.
- **Every 5-digit label has "#":** #40000, #40010, #40012, #40020, #40030, #40400–#40900. The layout is the same as page 10:
  - Most have the statement in column 15.
  - ` #40000 if(...)` and ` #40012 format(...)` have a single space, so the statement is in column 9.
- Form feeds before "c  hints" and "c  cave closing and scoring" are dropped, with no blank line in their place.
- Continuations: `1   .or..not.closed)` and `1   .and.atloc(oldlc2)…` have their text in column 10. `1 ' resume later…'`, `2 i3,…` and `1 i2,' points.')` have it in column 8.
- The first sheet ends with Woods' blank line after `goto 8` (8305). The second sheet ends mid-comment at "…for instance, there must be no water or".

## Page 17 (photo 17, two sheets): closing-time setup, final puzzle setup, lamp dying, demo end, disturbed dwarves
- Each sheet is 60 lines; the running total is 1980 lines.
- This page is **identical to Woods, lowercased**, except for the "#" labels: #10000, #10010, #11000, #11010, #12000, #12200, #12400, #12600, #13000 and #19000. Each has its statement in column 15.
- "(having moved the fixed object!)" keeps its "!".
- The first sheet ends with Woods' blank line after `newloc=115`. The second sheet ends with the blank line after `#19000 call rspeak(136)`.

## Page 18 (photo 18, two sheets): scoring and end of the main program; start of subroutine speak
- Each sheet is 60 lines; the running total is 2100 lines.
- The scoring logic is Woods' logic. Output changes:
  - `type` becomes `print` for 20100, 20202, 20212 and 20222.
  - **`20100 format(///' You scored'…` keeps its capital "Y"**. It is the only uppercase letter in a string so far. The other formats are lowercased: ' you just went off my scale!!', ' to achieve the next higher rating…', ' congratulations!!'.
  - Woods' `kk='s.'` / `kk='. '` / `type 20212,k,kk` becomes **`iz='s.'` / `iz='. '` / `print 20212,k,iz`**. This uses the `character*4 iz` variable, because `kk` is an integer in this port.
- Label layout, measured:
  - The "#" labels mostly have their statement in column 15.
  - **` #20212   format(...)` has its statement in column 11.**
  - Format continuations (`1 ', using'…`, `1 ' more point'…`, `1 'would be a neat trick!'…`) have their text in column 8.
- The main program ends with ` #25000       stop`, two blank lines, and `      end` in column 7.
- The first sheet ends with Woods' blank line after "c  did he come to witt's end as he should?". The second sheet ends at `common /blkcom/ blklin` in subroutine speak.

## Page 19 (photo 19, two sheets): speak, pspeak, rspeak, mspeak, getin
- Each sheet is 60 lines; the running total is 2220 lines. The first sheet is badly blurred in the photo. It was read with contrast enhancement and checked against Woods' structure.
- **speak:**
  - The dimension is still **`lines(9650)`**, not 12000. The subroutines keep Woods' old size even though the main program's array is 12000. That is harmless for a common-block array in practice, but an inconsistency.
  - **New `data np/'>$<'/`**, with `if(lines(n+1).eq.np)return` (Woods compares with the literal).
  - `type 2` becomes `print 2`, and **`2 format(' ',18a4)`** (Woods `14a5`), matching the 18 words per text line read by the main program.
- **pspeak:** `do 3 i=1,skip+1` (Woods `i=0,skip`). The count is the same, with 1-based loop bounds.
- **getin is rewritten for 9-bit characters in 36-bit words** (4 characters per word):
  - Comments: "chars 5 thru 8 are returned in word1x", "word2 (chars 5 thru 8 in word2x)" (Woods: 6 thru 10).
  - `dimension a(5),masks(5)`.
  - `data masks,blanks/o1000000000,o1000000,o1000,1,0,'    '/`. These are 9-bit character position values: 8^9, 8^6, 8^3, 1.
  - `2 read 3,(a(i),i=1,5)` / `3 format(5a4)`. Woods has `accept 3,(a(i),i=1,4)` / `format(4a5)`.
  - Case folding uses `and(a(i),xor(shift(and(a(i),'@@@@'),-1),-1))`. Woods' DEC `.and./.xor.` operators become function calls.
  - The word-splitting loop is `do 10 k=1,4` with `msk=127*masks(k)`, shifts of `9*(k-1)` / `9*(k-5)`, and `masks(5-k)`. The upper-bits mask is written `masks(5-k)-1` (Woods `-2-msk`).
  - **New:** `if(j.eq.2)word1x=or(and(word1x,-masks(k)),and(blanks,xor(-1,-masks(k))))`. Woods only blank-pads word1.
  - Continuations: `     1 + and(...)` and `     1 and(blanks,...)` have their text in column 8.
- **Review item for the working copy:** getin sets `word2=0` (integer zero), but the main program tests `wd2.ne.iz` / `wd2.eq.iz`, where `iz` is a blank-initialized `character*4` (`izz/0/` equivalenced to `iz`). Since `izz=0` is equivalenced to `iz`, `iz` actually holds zero bits, so the tests may be consistent. Check when building.
- The second sheet ends with getin's `end` followed by two of Woods' three blank lines.

## Page 20 (photo 20, two sheets): yes, yesm, yesx, a5toa1, vocab, start of dstroy
- Each sheet is 60 lines; the running total is 2340 lines.
- **yesx:**
  - Woods' blank line after `logical function yesx(x,y,z,spk)` is **absent**. It is offset by the new `character*4 reply` declaration.
  - The replies are tested as lowercase strings: 'yes', 'y', 'no', 'n'.
  - `type 9` becomes `print 9`, and the format reads ' please answer the question.'.
- **a5toa1 is rewritten for four characters per word** and gains a fourth input word:
  - `subroutine a5toa1(a,b,c,d,chars,leng)`.
  - Comment: "a and b contain a 1- to 8-character word in a4 format, c & d contain another". Woods: "1- TO 9-CHARACTER WORD IN A5 FORMAT, C CONTAINS ANOTHER". The rest of the comment is unchanged. It still says "between b and c (or none, if c >= 0)", which no longer matches the code.
  - Data and loops:
    - `dimension chars(20),words(4)` / `data mask,blank/o177000000000,' '/`.
    - `words(4)=d`.
    - `do 1 word=1,4`, and `posn.ne.5` (Woods 6).
    - `do 2 ch=1,4`.
    - `shift(words(word),9)` (Woods 7).
    - `and()` calls replace `.and.`.
  - **Woods' `if(word.eq.3.and.c.lt.0)posn=posn+1` is removed.** The blank test becomes `if(chars(posn).eq.blank.and.(word.ne.3.or.ch.ne.1))goto 1`, so a leading blank in the third word is kept. That is how callers get the separating space, e.g. `' her','e.'` and `' wha','t?'`.
- The form feed before "c  data structure routines" is dropped.
- **vocab:**
  - The hash is removed: `hash=id` (Woods `id.xor.'phrog'`). This matches the removed hashing in the data reader (page 4).
  - **New:** `10 format(1x,i4,2x,a4)` after `1 continue`.
  - **New:** `print 10,init,id` before `call bug(5)`, so the word that wasn't found is printed before the bug stop. Useful for the 5-character mnemonics ('knife', 'coins', 'chest') noted on page 5.
- `id` is an implicit integer in vocab, while callers pass character constants such as 'grat'. On GCOS a 4-character constant fits one 36-bit word. What happens to 'knife' (5 characters) depends on the compiler, probably truncation to 'knif'. Check when building.
- The first sheet ends with a5toa1's comment block plus Woods' blank line. The second sheet ends with dstroy's comment line.

## Page 21 (photo 21, two sheets): dstroy, juggle, move, put, carry, drop; start of the wizardry routines
- Each sheet is 60 lines; the running total is 2460 lines.
- dstroy, juggle, move, put, carry and drop are **identical to Woods, lowercased**.
- The form feed before "c  wizardry routines (start, maint, wizard, hours(x), newhrs(x), motd, poof)" is dropped.
- start: **`logical ptime,soon,yesm,wizard`**. Woods declares `ptime,soon,yesm`. `wizard` is added because start calls the logical function wizard.
- The first sheet ends with the blank line after put's `end`. The second sheet ends at that `logical` line.

## Page 22 (photo 22, two sheets): rest of start, maint, head of wizard
- Each sheet is 60 lines; the running total is 2580 lines. The second sheet's last line is Woods' blank line after the wizard comment.
- **start:**
  - `ptime=and(primtm,shift(1,t/60)).ne.0` (Woods `.and.`).
  - `type 10` becomes `print 10`. **The format keeps its capital: `' This adventure was suspended a mere'`.**
  - `     1 short,magic,...` has its continuation text in column 8.
- **maint:**
  - `logical yesm,blklin,wizard` (Woods: `yesm,blklin`).
  - **New:** `data bl/' '/`, with `if(x.ne.bl)magic=x` (Woods `x.ne.' '`).
  - `accept` becomes `read`:
    - `read 1,hbegin` / `read 1,hend` / `read 1,x` use **`1 format(v)`**, GCOS's free-format ("variable") descriptor, in place of Woods' `format(g)`.
    - `read 2,hname` with `2 format(4a4)` (Woods 4a5).
  - `type 12/16` becomes `print 12/16`. These formats *are* lowercased: ' length of short game…', ' latency for restart…'.
  - **Woods' `call mspeak(15)` before `blklin=.true.` is removed.** Woods' `call ciao` becomes `call ciao(setup)`, followed by a **new `return`**. Woods relied on ciao never returning.
  - `     1      short,magic,...` has its continuation text in column 13.
- Capitalization in format strings is inconsistent: "This adventure…" and "You scored…" keep Woods' capitals, while most others are lowercased.
- **Review item:** Woods' mspeak(15) is presumably the "maintenance complete / save the core image" message. Check the data file's section 12 against this removal.

## Page 23 (photo 23, two sheets): wizard, hours, hoursx, head of newhrs
- Each sheet is 60 lines; the running total is 2700 lines. The second sheet's last line is Woods' blank line after newhrs' common statement.
- **wizard** (heavily changed):
  - `     1   short,magic,...` has its continuation text in column 10. **New:** `data ats/'@@@@'/`.
  - **New backdoor:** `if (word.eq."fiomir") goto 20`, inserted before `if(word.ne.magic)goto 99`. It is written with double quotes and spaces around `if (`…`)`, unlike anything else in the file. It jumps to the new label `20` on `call mspeak(19)` ("by george, he really *is* a wizard!"), skipping the magic-word check and the challenge. The spelling "fiomir" is read from a blurry photo and needs confirming. As a 6-character literal compared against a 4-character word, it would probably only need its first four characters to match.
  - The challenge is rewritten for 4-character words:
    - `call datime(d,it)` / `t=it*2+1` (Woods `datime(d,t)` / `t=t*2+1`).
    - `word=ats`, `do 15 y=1,4`, `shift(...,36-9*y)`.
    - `do 19 y=1,4`, `z=mod(y,4)+1`, `if(word.ne.ats)`.
  - **`print 18,word,it+7000` with `18 format(/1x,a4,i4)`**. Woods prints only the challenge word. This port also prints `it+7000`, which reveals the time value the response is computed from. It looks like a deliberate aid for the local wizard.
  - Woods' three blank lines before `subroutine hours` are two here.
- **hours:**
  - **Holiday reporting is removed.** Woods' `hoursx(holid,…)` call, the `datime` check and formats 5/15 ("Today is a holiday…", "The next holiday will be in…") are gone.
  - hours now just prints the weekday and weekend lines: `call hoursx(wkday,'mon - fri:')` / `call hoursx(wkend,'sat - sun:')`. The dimension line still declares `val(5)`, as in Woods.
  - `     end` is followed **immediately** by `subroutine hoursx(h,day)`, with no blank lines.
- **hoursx:**
  - It takes one 10-character argument: `subroutine hoursx(h,day)` with `character*10 day` (Woods `day1,day2`, 2a5).
  - The formats use `a10` and lowercase text: '  open all day', '  closed all day'.
  - `and(h,shift(…))` replaces `.and.`.
- The first sheet ends with the first line of hours' comment. The second sheet ends with newhrs' declarations plus a blank line.

## Page 24 (photo 24, two sheets): newhrs, newhrx, **conv**, motd, poof, start of shift
- Each sheet is 60 lines; the running total is 2820 lines.
- Jim confirmed the backdoor word on page 23 reads **"fiomir"**.
- **newhrs:** it sets only weekday and weekend hours, `newhrx('weekdays:')` / `newhrx('weekends:')`. The holiday line is removed, matching the holiday-less hours.
- **newhrx(day):**
  - One `character*10 day` argument (Woods `day1,day2`).
  - `print` replaces `type`, `read 3,…` with `3 format(v)` replaces `accept`, and `or()` replaces `.or.`.
  - The format text is lowercased: ' prime time on '.
- **New `integer function conv(a)`**, between newhrx and motd, with the comment `c     convert from a to int` (text in column 7). It just does `conv=a`. It exists so a `character*4` word can be stored in the integer `obj` (the SAY code: `obj=conv(wd2)`).
- **motd:**
  - **New:** `data bl/' '/`.
  - `print 20,…` with `20 format(1x,18a4)`.
  - `55 read 56,(msg(i),i=m+1,m+18),k` with `56 format(19a4)`.
  - Comparisons are `.eq.bl` / `.ne.bl`, and the test is `m+18.lt.100`.
  - **Review item (probable bug): the trailing-blank scan was not updated.** It still reads `60 do 62 i=1,14` / `k=m+15-i` (Woods' 14-word values), while each line now holds 18 words. Words 15–18 of a message-of-the-day line would be ignored when finding the line's end, so long lines are truncated.
- **poof:**
  - `data ptime/o777400/,mwd/'dwar'/`, then `wkday=ptime`, `magic=mwd` (Woods: `"00777400`, `'DWARF'`).
  - `holid=0`, `hbegin=0` and `hend=-1` are removed.
  - The default magic word is now 'dwar'.
  - `     1      short,…` has its continuation text in column 13.
- The form feed before "c  utility routines (shift, ran, datime, ciao, bug)" is dropped.
- **shift is rewritten for GCOS.** The right-shift branch is `10 idist=-dist` / `shift=irl(shift,idist)`, using the GCOS intrinsic `irl` (logical right shift) in place of Woods' bit loop.
- **`#20` again: ` #20  return`.** "#" is in column 2 on a 2-digit label, and the statement is in column 9 (measured). So "#" is not only for labels too long for the label field. Its purpose is still unknown.
- The first sheet ends at motd's `k=1`. The second sheet ends at ` #20  return`.

## Page 25 (photo 25, two sheets): end of shift, ran, datime, **ciao**, bug
- Each sheet is 60 lines; the running total is 2940 lines. Photo 26's two lines should be bug's `stop` and `end`.
- **shift** ends with ` #30  shift=ils(shift,dist)` (GCOS intrinsic `ils`, logical left shift; statement in column 9) / `return` / `end`. The whole routine is now just: `shift=val`, `if(dist)10,20,30`, `10 idist=-dist / shift=irl(shift,idist)`, `#20 return`, `#30 shift=ils(shift,dist)`, `return`, `end`.
- **ran:** identical to Woods, lowercased. It still has the "lib40" comment.
- **datime** is rewritten for GCOS:
  - `real tim`, `call datim(dat,tim)`.
  - The date is decoded from BCD-style digit fields in `dat(1)`/`dat(2)` with `and(shift(…),15)`: month from `dat(1)` bits, day and year from `dat(2)`.
  - The month is found by a `do 1 i=1,12` / `if(i.eq.mon) go to 2` loop over `hath`.
  - Time: `t=tim*60.` (tim is presumably hours as a real).
  - **The labels are printed ` #1     d=d+hath(i)` and ` #2     d=d+year*365+year/4`.** Those are 1-digit labels with "#", and the statement is in column 9 (measured).
  - Woods' `months` table and the "above funny expression" comment are gone. `dimension … months(12)` remains, unused. Woods' `call bug(28)` (invalid month) is gone: if no month matches, the loop falls through to `2` with all 12 months added.
  - The comment still says "finagled dec functions return the values only as ascii strings!".
- **ciao is entirely new; this is the GCOS save routine.**
  - `subroutine ciao(setup)`. Comments: "exits, after saving the appropriate core image. used when / suspending and when creating a new version via magic mode" / "the user may only create a file of the form - / /catalog/adventur".
  - `dimension iname(3),name(2)` / `data j/12/`.
  - `if(setup.eq.2) go to 1000`: maint's "new version" path skips the prompt. Otherwise:
    - `call mspeak(15)`. This is the message removed from maint; it presumably asks for the catalog name.
    - `10 format(" please enter your catalog name")` is **never referenced**, a leftover.
    - `read 20,iname` / `20 format(3a4)` / `call ascbcd(iname,name,j)` converts ASCII to BCD for the GCOS file system.
  - `1000 call ciaowr(name,rtn)` is a system (or companion) routine that writes the saved core image.
    - If `rtn.ne.0`, it goes to `1010`: "your program has been lost due to file system error - code ",o12.
    - `if (name(1).eq.0) stop`, i.e. the setup=2 path stops silently.
    - Otherwise `print 30,iname`: "your copy of adventure has been saved with the name / adventur under your catalog -/",3a4, then `stop`.
  - The strings here use **double quotes**, like the "fiomir" backdoor line in wizard. That suggests the same hand wrote both.
  - Continuations `     1" adventur…` and `     1" error - code ",o12)` have their text in column 7. `if (`, `go to` and spaced `stop` differ from the rest of the file's style.
- **bug:**
  - The comment table is Woods' ("message line > 70 characters" etc., unchanged).
  - `type 1, num` becomes `print 1, num`, and the format is lowercased: ' fatal error, see source code…', ' probable cause:…', ' error code ='. Continuations have their text in column 8.
- **Save/restore conclusion:** the GCOS version saves by writing a core image with `ciaowr` into the user's catalog as `adventur`. Restoring means running that saved image, which re-enters at `8305 yea=start(0)`, with setup=-1 triggering the latency check. It is not a data-file save format, so it doesn't carry over directly to the Multics Palter engine.

## Page 26 (photo 26, single sheet): `      stop` / `      end` (end of bug)

## Completeness check (whole file)
- **2942 lines = 49 full sheets of 60 + 2 lines.** This matches 26 photos: a single first sheet, 24 two-sheet photos, and a two-line last sheet.
- There are **32 program units**. The main program is followed by speak, pspeak, rspeak, mspeak, getin, yes, yesm, yesx, a5toa1, vocab, dstroy, juggle, move, put, carry, drop, start, maint, wizard, hours, hoursx, newhrs, newhrx, **conv (new)**, motd, poof, shift, ran, datime, ciao and bug.
- **Every statement label referenced within a unit is defined.** The one defined-but-unused label is `10` in ciao, the orphaned "please enter your catalog name" format.
- External routines not defined in this file:
  - `datim` (GCOS date/time).
  - `ascbcd` (ASCII to BCD).
  - `ciaowr` (write the saved core image).
  - Bit intrinsics `irl`, `ils`, `and`, `or`, `xor`.
  These are presumably GCOS library routines, or a companion file not in the printout.
- Nothing from Woods is missing apart from the deliberate removals already noted: holidays, the vocabulary hash, the F40 workarounds, bug(24), bug(28), and mspeak(15) in maint.

## Consolidated review list (for a working build)
1. motd's trailing-blank scan still uses 14 words (`do 62 i=1,14`, `k=m+15-i`) on 18-word lines.
2. 5-character literals passed to `vocab`: 'knife', 'coins', 'chest'.
3. `word2=0` in getin versus `wd2.eq.iz` tests in the main program. `iz` is equivalenced to `izz=0`, so this is probably consistent.
4. Transitive-verb range check: Woods' `call bug(24)` was removed without replacement.
5. datime: no invalid-month check (`bug(28)` removed); `months(12)` is dimensioned but unused.
6. `lines(9650)` in speak and pspeak versus `lines(12000)` in the main program.
7. The a5toa1 header comment still describes Woods' 3-argument semantics.
8. The mspeak(15) move from maint to ciao: check that section 12 message 15 now reads as a catalog-name prompt.
9. The backdoor `if (word.eq."fiomir") goto 20` in wizard, and `print 18,word,it+7000`, which leaks the challenge seed. Decide whether to keep them in a playable build. They are artifacts either way.
