# Two 382-point Adventures that share nothing but the number

This collection now holds two different games that both announce
`OUT OF A POSSIBLE 382`:

* `..\..\Colossal Cave Adventure 382pt (PL1, mainframe, full source)\` —
  Gary Palter's PL/I **Version 4.0**, off the SHARE/CBT tapes.
* this one — **Tymshare's** DECsystem-10 FORTRAN build, off the TYMCOM-X
  tapes, 1978–79.

Both start from Don Woods' 350-point FORTRAN game and both add 32
points. Neither knows about the other's additions.

## What each one added

|  | Tymshare 382 | PL/I "Version 4.0" 382 |
|---|---|---|
| extra treasures | a **ring of adamant**, a **small mail coat made of mithril** | a **bloody great red ruby**, **Orac the super computer** |
| extra tool | a 50-foot coil of rope, which can be cut in two | a teleport bracelet |
| extra obstacle | pits and a crack that need the rope | a force field ("the ship's anti-intruder defences bar the way") |
| new verbs | `TIE`, `UNTIE`, `CUT` | — |
| flavour | Tolkien | Blake's 7 |
| hollow voice says | `PLUGH` | `PLUGHOLE` |
| locations | 150 | 146 |

Searched in each other's databases, the additions are simply absent:

```
mithril / adamant / "coil of rope" / "tie the rope"   in the PL/I database:  0 hits
                                                      in the Tymshare image: 7 hits
```

Tymshare kept Woods' credit block word for word and did not sign its own
work anywhere:

> THIS PROGRAM WAS ORIGINALLY DEVELOPED BY WILLIE CROWTHER. MOST OF THE
> FEATURES OF THE CURRENT PROGRAM WERE ADDED BY DON WOODS (DON @ SU-AI).
> CONTACT DON IF YOU HAVE ANY QUESTIONS, COMMENTS, ETC.

The PL/I version rewrote the block and added its own author to it —
Crowther, Woods "AND GARY PALTER (PALTER@MIT-MULTICS)", plus
"SOURCERER FOR THIS VERSION IS GP@SECV". So no name attaches to
Tymshare's 32 extra points at all — not in the program, and not in the
unsigned walkthrough elsewhere on the tapes that describes them.

## What Tymshare left alone

The class-rating ladder is the 350-point game's, untouched:

```
35  100  130  200  250  300  330  349
```

with the same nine ranks from *RANK AMATEUR* to *ADVENTURER
GRANDMASTER*. So the two new treasures were dropped in without
rebalancing anything: a score that made you a Junior Master in the
350-point game still does here, it is just further from the top.

Everything else that distinguishes Woods' 1977 FORTRAN release is
present and unmodified — the wizard's hours, the holiday scheduler, the
`SUSPEND`/magic-number machinery, the closing sequence in the
repository, the "MAIN OFFICE" endgame, `SPELUNKER TODAY`, Witt's End.

## The one other change to the old cave

The Plover Room gained a sign. In Woods' game it is bare; here it reads

```
 A SIGN ON THE WALL READS
 "OTHERS HAVE EXPLORED THE DARK ROOM, Y NOT YOU 2"
```

which is a hint, in the same register as the note that says
`MAGIC WORD XYZZY`. The PL/I version has no such sign; its Dark-Room is
Woods' plain *"YOU'RE IN THE DARK-ROOM. A CORRIDOR LEADING SOUTH IS THE
ONLY EXIT."*

## Where the number 382 is asserted

Not by the program's banner — by the tape. `..\dump_original\
advent.hlp.jms-info` and `..\dump_original\advent.txstext` are two
copies of a player-written walkthrough found in other users' directories
in the same collection:

> ALSO, THESE INSTRUCTIONS ARE ALSO FOR THE EXPANDED VERSION TO BE FOUND
> AT TYMSHARE, INC. OF CUPERTINO, CALIFORNIA, AND THERE IS A GRAND TOTAL
> OF 382 POINTS THAT YOU MUST GET TO BECOME AN ADVENTURE GRANDMASTER.
> HOWEVER, THESE DIRECTIONS ADAPT THEMSELVES EASILY TO THE 350 POINT
> VERSION.

and, part-way down, it marks off the section that only the bigger game
needs:

> THE NEXT SERIES OF ___ STEPS NEEDS ONLY TO BE EXECUTED IN THE GAME'S
> 382 PT. VERSION.
> ```
> W
> GET ROPE
> ```

followed later by the `TO GET COAT:` and `TO GET RING:` sequences. That
is a contemporary description of exactly the content this image has, by
someone who had played both sizes, and it is why the identification does
not rest on the program alone.
