C  Portable replacements for the original Univac 1100 MASM routines
C  INITDB, SAVEDB and SAVEMA.
C
C  The originals worked by dumping/restoring a raw memory image (the
C  "D-bank") to/from a word-addressable file named via the shared
C  computer's account system (<user id>*CAVE$ for a player's own
C  suspended game, ISD*CAVE$*CAVE$ as a pre-parsed "master" image other
C  players started from).  None of that accounting/multi-user machinery
C  exists any more, so this port keeps only what actually matters for a
C  standalone single-player game:
C
C    CAVE.SAV -- one player's suspended game (INITDB/SAVEDB)
C    CAVE.CFG -- the wizard's persisted MAINT settings (SAVEMA/LOADCFG),
C                applied as an override right after POOF sets defaults.
C
C  All the state that needs to survive a SUSPEND/RESTORE now lives in
C  named COMMON blocks (see gamecom.fi) so it can be reached from here
C  with a plain WRITE/READ, instead of a raw memory-image copy.

      SUBROUTINE INITDB
C  Stage 1 of restore: just ask whether to resume a suspended game, and
C  remember the answer in PENDR (comwiz.fi).  This is called *before*
C  MAIN re-parses the text database, so it must not touch SETUP or any
C  of the state that parse is about to (re)populate -- see RESTOREDB,
C  which is called *after* the parse, and does the actual restore.
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'
      CHARACTER*1 ANS

      PENDR = 0
      OPEN(9,FILE='CAVE.SAV',FORM='UNFORMATTED',STATUS='OLD',
     1     IOSTAT=IOS)
      IF (IOS.NE.0) RETURN

      PRINT 10
10    FORMAT(' Previous game found.  Play it (Y,N)?')
20    READ(5,'(A1)') ANS
      IF (ANS.EQ.'y'.OR.ANS.EQ.'Y') GOTO 30
      IF (ANS.EQ.'n'.OR.ANS.EQ.'N') GOTO 40
      PRINT 25
25    FORMAT(' Please answer Y or N.')
      GOTO 20

30    CLOSE(9)
      PENDR = 1
      RETURN

40    CLOSE(9)
      RETURN
      END


      SUBROUTINE RESTOREDB
C  Stage 2 of restore: called right after MAIN's fresh database parse
C  finishes.  If INITDB (stage 1) recorded that the player wants their
C  suspended game back, load it now -- this overwrites SETUP with the
C  -1 that was in effect at SUSPEND time, which is what sends MAIN to
C  the 8305 resume point instead of a normal new-game start.
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'comabb.fi'
      include 'compla.fi'
      include 'comwiz.fi'

      IF (PENDR.EQ.0) RETURN

      OPEN(9,FILE='CAVE.SAV',FORM='UNFORMATTED',STATUS='OLD',
     1     IOSTAT=IOS)
      IF (IOS.NE.0) RETURN
      READ(9)
     1 a, a1, a2, a4, a5, abbnum, attack, axe, back, balrog,
     1 banish, batter, bear, boat, bottle, bridge, cape, carpet,
     1 chain, chest, chloc, class, climb, clock1, clock2, clsses,
     1 cola, column, compas, crap, crown, cup, daltlc, dam,
     1 descrb, detail, dflag, djinn, dkill, door, down, dragon,
     1 drop, dtotal, dwarf, enter, erope1, erope2, find, food,
     1 gate, giant, hammer, harp, helm, hint, hntmax, hope, i1,
     1 i12, i2, i3, i4, i5, i7, i9, idol, ii, inorth, invent,
     1 jewlry, k, k2, keg, keys, knfloc, knife, kobold, kq, lamp,
     1 limit, linuse, ll, lock, maxart, maxcre, maxdie, maxdwr,
     1 maxmon, maxtrs, maxwpn, medal, minart, mincre, mindwr,
     1 minmon, mintrs, minwpn, mirror, misfor, mncomp, mxcomp,
     1 mxscor, newloc, null, numdie, oldlc2, oldloc, oldvrb, orb,
     1 orc, out, rctrvs, rick, ring, rope, rope2, ruby, rugrop,
     1 sand, say, scept, score, scroll, scrwup, sears, searsl,
     1 sect, self, shelf, shower, skeltn, spice, spider, spk,
     1 stick, stone, sword, tabndx, take, tally, tally2, throne,
     1 throw, toad, tomb, touch, trvs, turns, unicrn, up, vend,
     1 verb, wallet, water, wine, x, z, loc, obj, wzdark, lmwarn,
     1 closng, panic, bonus, closed, gaveup, scorng, demo, yea,
     1 travel, rtravl, ctravl, ltext, stext, key, cond, plac,
     1 fixd, prop, iprop, actspk, ctext, cval, hintlc, hints,
     1 dloc, odloc, itk, dseen, hinted, wd1, wd2, tk, wd1x, wd2x,
     1 chr, star, abb, atloc, place, fixed, link, holdng, wkday,
     1 wkend, holid, hbegin, hend, hname, short, magic, magnm,
     1 latncy, saved, savet, setup, check
      CLOSE(9)
      RETURN
      END


      SUBROUTINE SAVEDB
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'comabb.fi'
      include 'compla.fi'
      include 'comwiz.fi'

      OPEN(9,FILE='CAVE.SAV',FORM='UNFORMATTED',STATUS='UNKNOWN')
      WRITE(9)
     1 a, a1, a2, a4, a5, abbnum, attack, axe, back, balrog,
     1 banish, batter, bear, boat, bottle, bridge, cape, carpet,
     1 chain, chest, chloc, class, climb, clock1, clock2, clsses,
     1 cola, column, compas, crap, crown, cup, daltlc, dam,
     1 descrb, detail, dflag, djinn, dkill, door, down, dragon,
     1 drop, dtotal, dwarf, enter, erope1, erope2, find, food,
     1 gate, giant, hammer, harp, helm, hint, hntmax, hope, i1,
     1 i12, i2, i3, i4, i5, i7, i9, idol, ii, inorth, invent,
     1 jewlry, k, k2, keg, keys, knfloc, knife, kobold, kq, lamp,
     1 limit, linuse, ll, lock, maxart, maxcre, maxdie, maxdwr,
     1 maxmon, maxtrs, maxwpn, medal, minart, mincre, mindwr,
     1 minmon, mintrs, minwpn, mirror, misfor, mncomp, mxcomp,
     1 mxscor, newloc, null, numdie, oldlc2, oldloc, oldvrb, orb,
     1 orc, out, rctrvs, rick, ring, rope, rope2, ruby, rugrop,
     1 sand, say, scept, score, scroll, scrwup, sears, searsl,
     1 sect, self, shelf, shower, skeltn, spice, spider, spk,
     1 stick, stone, sword, tabndx, take, tally, tally2, throne,
     1 throw, toad, tomb, touch, trvs, turns, unicrn, up, vend,
     1 verb, wallet, water, wine, x, z, loc, obj, wzdark, lmwarn,
     1 closng, panic, bonus, closed, gaveup, scorng, demo, yea,
     1 travel, rtravl, ctravl, ltext, stext, key, cond, plac,
     1 fixd, prop, iprop, actspk, ctext, cval, hintlc, hints,
     1 dloc, odloc, itk, dseen, hinted, wd1, wd2, tk, wd1x, wd2x,
     1 chr, star, abb, atloc, place, fixed, link, holdng, wkday,
     1 wkend, holid, hbegin, hend, hname, short, magic, magnm,
     1 latncy, saved, savet, setup, check
      CLOSE(9)
      RETURN
      END


      SUBROUTINE SAVEMA
C  Originally saved a "master" D-bank snapshot other players' cold
C  starts would load instead of re-parsing the text database (a
C  startup-speed optimisation with no purpose on modern hardware,
C  since a fresh parse of the database takes milliseconds).  What
C  actually matters here is that the wizard's MAINT tuning (short
C  game length, magic word/number, latency, cave hours) persists for
C  future games, so this just writes those settings out to a small
C  config file that LOADCFG applies after POOF sets its defaults.
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'

      OPEN(9,FILE='CAVE.CFG',FORM='UNFORMATTED',STATUS='UNKNOWN')
      WRITE(9) SHORT,MAGIC,MAGNM,LATNCY,HBEGIN,HEND,HNAME,
     1          WKDAY,WKEND,HOLID
      CLOSE(9)
      RETURN
      END


      SUBROUTINE LOADCFG
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'

      OPEN(9,FILE='CAVE.CFG',FORM='UNFORMATTED',STATUS='OLD',
     1     IOSTAT=IOS)
      IF (IOS.NE.0) RETURN
      READ(9) SHORT,MAGIC,MAGNM,LATNCY,HBEGIN,HEND,HNAME,
     1         WKDAY,WKEND,HOLID
      CLOSE(9)
      RETURN
      END
