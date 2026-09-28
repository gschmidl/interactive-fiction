# Handwritten annotations on the adventure_.fortran printout (blue ink, Jim Lippard, c. 1980)

Transcribed separately from the line-printer text. Location = page / nearest printed line.

## Page 1 (declarations)
- Line `&spices,stick,verb,wd1,wd1x,wd2,wd2x,wzdark,`: after "wzdark," there is a short handwritten
  insertion that has been heavily struck through (illegible in the photo), followed by
  **`objcount, all,`** written to the right. A curved stroke runs from the struck text down toward the
  next line (near "tk,"), possibly an insertion caret.
  Reading: add variables `objcount` and `all` to common block /msccom/ (plausibly for an "all" object
  / object-count feature, e.g. TAKE ALL / DROP ALL).
  Jim (2026-09-26): the struck-through text is illegible on paper too; the curved stroke is part of the
  scratch-out, not a caret. Treat the annotation as simply: append `objcount, all,` after `wzdark,`.

## Page 6 (object mnemonics)
- In the right margin beside `batter=vocab(code1('batte'),1)` / `c  objects from 50 through whatever are treasures.`:
  **`all=vocab(code1('all  '),1)`** — written `'all` with two small carets (^^) under the gap, marking two blanks
  (i.e. the 5-character word "all  "). The final argument is **1** (confirmed by Jim on paper): "all" is looked up as an object word.
  Goes with the page-1 note adding `objcount, all` to /msccom/: "all" was to be looked up as a vocabulary word.
  (Note: "all" is not in adventure.data's vocabulary, so the data file would need a new entry too.)

## Plan
- Jim (2026-09-26): when building the working copy, implement "all" fully (TAKE ALL / DROP ALL etc.),
  using these notes (objcount, all) as the starting point. Needs a vocabulary entry in adventure.data too.

## Page 9 (location description, label 2000)
- A long curly bracket in the left margin spans the new Platt block inserted after
  `if(wzdark.and.pct(35))goto 90` (from about `fog=0` down to the goblin lines), labelled
  **"possible problem"** (second word abbreviated, reads like "pblm"/"prblm").
  Jim's instinct was right: see NOTES (this whole block is reachable only when the room is DARK).

## Page 13 (carry / drop): the ALL implementation (most extensive notes so far)

### A. Replacement for 8010 ("carry, no object given yet")
The printed 8010-8012 block and `obj=atloc(loc)` are circled/struck through with loops. Written to the right:

       8010 if(atloc(loc).eq.0)goto 8000
            do 8012 i=1,100
       8012 if(here(i)) objcount=objcount+1
            if(objcount.eq.1) goto 8013
            goto 8000
       8013 do 8014 i=1,100
            if(here(i)) obj=i
       8014 continue

Meaning: TAKE with no object picks up the object if exactly one is here (counting with a loop instead of
Palter's link test, and dropping the dwarf check). NB `objcount` is never reset to 0 before the loop.

### B. TAKE ALL (9010 carry)
In the right margin beside the 9010 bottle lines:   `if(obj.eq.all) goto 9031`   (insertion point not marked;
presumably at the top of 9010).
Block written further down the right margin:

       9031 spk=                       <- arrow from note "(nothing here to get)"
            do 9033 i=1,100
       9033 if(here(i))goto 9034       [label 9033 is written beside this line; intended for the continue]
            continue
            [spk=... erased/illegible]
            goto 2011
       9034 if(fixed(i).ne.0) goto 9033
            [spk=... struck through]
            if(holdng.eq.6) goto 2011       [confirmed by Jim]  note: "(can't carry no more)"
            call carry(i,loc)    spk=0
            print, dcode1(i), ": taken."    [Jim confirms "dcode1"]
            goto 9033

### C. DROP ALL (9020 discard)
Beside `if(obj.ne.bird.or..not.here(snake))goto 9024`:   `if(obj.eq.all) goto 9022`
Block to the right of 9021-9024:

            spk=                     note: "(you aren't carrying anything)"   [a faint earlier "spk=0." above]
       9022 do 9023 i=1,100
            if(toting(i)) goto 9036
       9023 continue
            goto 2011
       9036 call drop(i)    spk=0          [drop(i) missing the location argument]
            print, dcode1(i), ": dropped."
            goto 9023

### Observations for the working copy
- New labels 8013, 8014, 9022, 9031, 9033, 9034, 9036 don't collide with Palter/Woods labels
  (9030 SAY, 9032 format and 9035 do exist, so 9031/9033/9034 sit between them).
- Needs: `all` vocabulary entry (object word, per page 6), objcount reset, message numbers for the three notes,
  a routine to turn an object number into its name (the "dcode1" idea; the inventory code uses pspeak(obj,-1)),
  holding limit consistent with 9017 (7, or 12 with mushroom strength), drop(i,loc).

- Jim: the erased spk= lines are unrecoverable; reconstruct intent from the three marginal notes.

## Page 23 (dcode1, sixbit -> characters)
Beside `chridx=mod(valcpy,64)+1` / `valcpy=valcpy/64`:
- an arrow from the 64 in `mod(valcpy,64)` to **"90"**, and an arrow from the 64 in `valcpy/64` to **"90"**;
- note: **"try several combinations of 90 and 64, 64 and 90"**.
Jim was experimenting with how dcode1 should decode against the 90-entry chrset (mod 90 vs mod 64 for the digit,
/90 vs /64 for the shift). Since code1/code2 pack 6-bit fields (shift(result,6)), 64/64 is the consistent choice;
the lowercase table puts every packable character in entries 1-64, so no change is needed. (The uppercase entries
65-90 only matter for input, which cvltuc lowercases first.)
2026: implemented the other way round in the working copy: code1/code2/dcode1 now pack in base 90 (mod 90, /90),
so capitals fit and the database was converted to mixed case. See Multics/CHANGES.md.
