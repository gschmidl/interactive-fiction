C  Portable replacements for the original Univac 1100 MASM routines
C  INITDB, SAVEDB and SAVEMA.
C
C  The originals worked by dumping/restoring a raw memory image (the
C  "D-bank") to/from a word-addressable file named via the shared
C  computer's account system (<user id>*ADV$$ for a player's own
C  suspended game, ISD*ADV$$*ADV$$ as a pre-parsed "master" image other
C  players started from).  None of that accounting/multi-user machinery
C  exists any more, so this port keeps only what actually matters for a
C  standalone single-player game:
C
C    ADV.SAV -- one player's suspended game (INITDB/SAVEDB)
C    ADV.CFG -- the wizard's persisted MAINT settings (SAVEMA/LOADCFG),
C               applied as an override right after POOF sets defaults.
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
      OPEN(9,FILE='ADV.SAV',FORM='UNFORMATTED',STATUS='OLD',
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
C  the 1210 resume point instead of a normal new-game start.
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'gamecom.fi'
      include 'comabb.fi'
      include 'compla.fi'
      include 'comwiz.fi'

      IF (PENDR.EQ.0) RETURN

      OPEN(9,FILE='ADV.SAV',FORM='UNFORMATTED',STATUS='OLD',
     1     IOSTAT=IOS)
      IF (IOS.NE.0) RETURN
      READ(9)
     1 a, a1, a2, a4, a5, abbnum, achieve, attack, axe, back,
     1 batter, be, bear, before, bird, bonus, bottle, but, ca,
     1 cage, cave, ced, chain, chasm, chest, chloc, chloc2, clam,
     1 class, clock1, clock2, clos, coins, congratulations,
     1 continuing, daltlc, detail, dflag, dkill, door, dprssn,
     1 dragon, dtotal, dwarf, e, ed, egg, eggs, emrald, entrnc,
     1 find, fissur, foo, foobar, food, for, g, grate, have,
     1 higher, hint, hntmax, i1, i12, i2, i3, i4, i5, i6, invent,
     1 iwest, k, k2, keys, kluge, knfloc, knife, kq, la, lamp,
     1 later, least, levs, limit, linuse, ll, lock, look, magzin,
     1 maxdie, maxtrs, messag, minutes, mirror, more, moves, mp,
     1 mxscor, neat, newloc, next, nugget, null, numdie, o, of,
     1 oil, oldlc2, oldloc, oldvrb, oom, out, oyster, p, pearl,
     1 pillow, plant, plant2, point, points, possible, pyram, r,
     1 rating, resume, rinit, rod, rod2, rop, rope, rope2, round,
     1 ruby, rug, s, say, score, sect, snake, snake2, spices,
     1 spk, steps, stick, tablet, tabndx, tally, tally2, the,
     1 throw, trick, tridnt, troll, troll2, trvs, turns, vase,
     1 vend, verb, wait, water, will, with, would, x, you, z,
     1 loc, obj,
     1 wzdark, lmwarn, closng, panic, closed, gaveup, scorng,
     1 demo, yea, ltext, stext, key, cond, plac, fixd, prop,
     1 actspk, ctext, cval, hintlc, hints, tk, dloc, odloc,
     1 dseen, hinted, wd1, wd2, wd1x, wd2x, chr, star, travel,
     1 abb, atloc, place, fixed, link, holdng, wkday, wkend,
     1 holid, hbegin, hend, hname, short, magic, magnm, latncy,
     1 saved, savet, setup
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

      OPEN(9,FILE='ADV.SAV',FORM='UNFORMATTED',STATUS='UNKNOWN')
      WRITE(9)
     1 a, a1, a2, a4, a5, abbnum, achieve, attack, axe, back,
     1 batter, be, bear, before, bird, bonus, bottle, but, ca,
     1 cage, cave, ced, chain, chasm, chest, chloc, chloc2, clam,
     1 class, clock1, clock2, clos, coins, congratulations,
     1 continuing, daltlc, detail, dflag, dkill, door, dprssn,
     1 dragon, dtotal, dwarf, e, ed, egg, eggs, emrald, entrnc,
     1 find, fissur, foo, foobar, food, for, g, grate, have,
     1 higher, hint, hntmax, i1, i12, i2, i3, i4, i5, i6, invent,
     1 iwest, k, k2, keys, kluge, knfloc, knife, kq, la, lamp,
     1 later, least, levs, limit, linuse, ll, lock, look, magzin,
     1 maxdie, maxtrs, messag, minutes, mirror, more, moves, mp,
     1 mxscor, neat, newloc, next, nugget, null, numdie, o, of,
     1 oil, oldlc2, oldloc, oldvrb, oom, out, oyster, p, pearl,
     1 pillow, plant, plant2, point, points, possible, pyram, r,
     1 rating, resume, rinit, rod, rod2, rop, rope, rope2, round,
     1 ruby, rug, s, say, score, sect, snake, snake2, spices,
     1 spk, steps, stick, tablet, tabndx, tally, tally2, the,
     1 throw, trick, tridnt, troll, troll2, trvs, turns, vase,
     1 vend, verb, wait, water, will, with, would, x, you, z,
     1 loc, obj,
     1 wzdark, lmwarn, closng, panic, closed, gaveup, scorng,
     1 demo, yea, ltext, stext, key, cond, plac, fixd, prop,
     1 actspk, ctext, cval, hintlc, hints, tk, dloc, odloc,
     1 dseen, hinted, wd1, wd2, wd1x, wd2x, chr, star, travel,
     1 abb, atloc, place, fixed, link, holdng, wkday, wkend,
     1 holid, hbegin, hend, hname, short, magic, magnm, latncy,
     1 saved, savet, setup
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

      OPEN(9,FILE='ADV.CFG',FORM='UNFORMATTED',STATUS='UNKNOWN')
      WRITE(9) SHORT,MAGIC,MAGNM,LATNCY,HBEGIN,HEND,HNAME,
     1          WKDAY,WKEND,HOLID
      CLOSE(9)
      RETURN
      END


      SUBROUTINE LOADCFG
      IMPLICIT INTEGER(A-Z)
      include 'params.fi'
      include 'comwiz.fi'

      OPEN(9,FILE='ADV.CFG',FORM='UNFORMATTED',STATUS='OLD',
     1     IOSTAT=IOS)
      IF (IOS.NE.0) RETURN
      READ(9) SHORT,MAGIC,MAGNM,LATNCY,HBEGIN,HEND,HNAME,
     1         WKDAY,WKEND,HOLID
      CLOSE(9)
      RETURN
      END
