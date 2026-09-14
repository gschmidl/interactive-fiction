# Repairs to VENTUR, and the debug mode

VENTUR, the 1984 set's stand-alone game (its banner: *WELCOME TO THE LAND OF
FRED!*), has bugs a player runs into within an hour. They are the program's
own, not the port's: each one below was traced in the disassembly of the
original image and reproduced on the emulator. The one fault that *was* the
port's, 8-bit console characters crashing the runtime, is fixed for good and
needs no switch.

`--fix` repairs them without touching the image. Every TYMBASIC statement
starts `JSP 16,<statement routine>` followed by its line number, so
`src/fixes.c` sees each line before it runs. At the lines listed here it reads
or writes the program's own variables, or continues at another line as a
GOTO would. Line numbers below are VENTUR's. Without `--fix` none of this
runs, and the earlier transcripts replay byte for byte.

The addresses were read out of the image with the tools in `work/`, and
`fixes_image_loaded()` checks a sample of them against the loaded program
before switching anything on.

## The repairs

| what players saw | why | the repair |
|---|---|---|
| SAVE does nothing | Verb code 25 is in the vocabulary, but the dispatcher has no branch for it | SAVE writes `ventur.sav` at the next prompt, and RESTORE at the prompt brings it back. The file is the low segment, the channel table and the repairs' own state. Restoring re-describes where you are, even in a new session |
| OFF does nothing | Line 13620 leaves unless the lamp is *in the room*, so OFF only worked on a lamp on the floor | Line 13600 goes on to the switch-off for a carried lamp too. `OFF TORCH` puts out a torch |
| an unlit lamp lights the dungeon | Lines 2260 and 2580 call a room dark only if the lamp is off **and** not carried **and** not here | A dungeon room is lit by a lit lamp or torch that is carried or here. Line 4640, which lists objects and monsters, uses the same rule. Room 11, which described itself only for a carried lit lamp, follows it too |
| torches can't be lit | LIGHT (13360) accepts only the lamp | `LIGHT TORCH` works. A torch burns 100 moves (the lamp lasts 255), then burns out and goes back into the store's stock |
| "YOUR LAMP IS DEAD." every turn | The death check is `counter = 255`, and a dead lamp stops counting | Once said, the counter moves to 256 |
| can't buy another lamp | There is one of each object, and BUY only sells what is in stock (location -1) | BUY LAMP with a dead lamp: the merchant takes the old one in trade, and it is sold fresh. BUY also refuses to charge again for something already on the store floor, which it used to do |
| a thrown flask stays in your hands, and the last monster it killed comes back | The fire loop (25110–25180) reuses NNUM, the thrown object's number, for each victim. Line 26095 then put "the thrown object", the last victim, back in the room | NNUM is kept at 26087 and put back at 26090. The flask leaves your hands, goes back into stock as line 26097 intended, and frees its inventory slot. Thrown objects never freed their slot before either |
| the oil fuse carries into the next game | "Play again" is GOTO 380, after the lines that zero the lamp counter, turn count and strength pool. Nothing ever reset the fuse | Line 380 zeroes them, and BURN restarts the fuse |
| no way to get gold | Gold is set once (line 1600), and only BUY changes it | A monster you kill carries 1 to *its maximum hit points* in gold. This is the one new rule, not a repair of an existing one |
| a dropped mace attacks, and the titan is flimsy | The hit-point column has one value too many among the monsters and one too few among the objects. The titan reads 27, the mace, first of the objects, reads 55, and anything with 2 or more counts as a monster | Line 1740, after the DATA is read, gives the titan its 55 and the mace 1. Every other column (adjectives, dropped items, damage) lines up noun for noun. The shift cannot start before noun 81: the three trolls' 26/32/41 rise with their ratings and damage, and the mezzodemon's 49 fits its kind. So the titan's slot is the smallest change that fits. At 55 the titan can only be hit by a natural 20, or with the axe on 15 or better |
| the axe is no better than its damage | Line 20700 was meant to give the axe +5 damage and +5 to hit, but `A=A+5 AND B=B+5` compiles as one assignment of a comparison. The damage bonus became 0 and the to-hit bonus stayed as it was | With the axe, both bonuses are added and the broken line is skipped. That applies to a thrown axe too, which uses the same attack routine |
| a critical hit always kills | A natural 20 picks one of three criticals with ON A GOTO: split in two, head chopped off, or "YOU GOT A GOOD HIT!" for 1–30 extra damage. Line 22140 rolls `INT(RND*2+1)`, which is never 3 | The choice is rolled 1 to 3, so a third of criticals are the good hit. Criticals kill outright less often than before, as the author wrote them |
| "TBA system error in line 21340" | The kill list is DIM 84, and the 85th kill writes past it | Names past the 84th are not kept, and the count goes on. The end-of-game total is still right |

The stairs in room 44 are scenery. The map gives the room no DOWN exit, and
VENTUR never runs another program: the only RUN UUO in it is the runtime's
`RUN SYS:LOGOUT`. The game is won on its one level: wave the wand while
carrying it, in the 30' by 40' room where a voice says *HY GUY!*, with the
star crystal on the floor there.

## The message file

Independently of `--fix`, a program that stops on a TYMBASIC run-phase error
now says what the error was. The runtime looks up `SYS:TBAMSG.SHR` for the
text, found nothing on the port, and printed `TBA system error`. The port
carries that file: `data/tbamsg.shr`, from `ftsys/tbamsg.shr.4` on the
Tymshare tapes, 1981-08-10, embedded by `tools/mkmsg.py`. The copies in
`fttba/`, `spunkdev/` and `tbatlib/` give the same messages with this
runtime. A `tbamsg.shr` in the save directory takes precedence.

## Debug mode

`--debug` takes commands beginning with `#` at any prompt. The program never
sees them, and the prompt is repeated after the reply.

| command | VENTUR | DUNGEN, PUB and B |
|---|---|---|
| `#GOD` | Strength is held where it was, so blows do nothing, and a lit flask held too long burns out harmlessly | For every character in play: hit points held full and level never drained. A death that zeroes strength directly (poison, for one) is caught as QDONE starts |
| `#STATS` | strength 9,999,999 | All six abilities 18, the most CHARC rolls. Hit points and mana refilled |
| `#GOLD` | 9,999,999 gold | 9,999,999 gold for each character in play |
| `#HELP` | lists them | lists them |

9,999,999 is the largest number TYMBASIC prints whole: 16,777,215 comes out
as `.1677722E+08`. It is also the largest that fits the seven-digit `DDDDDDD`
field the dungeon game writes gold and abilities into. It is exact in a
single-precision float. With `--debug` on, gold and VENTUR's strength are
kept from growing past it.

DUNGEN, PUB and B share every address debug mode uses. Only the statement
routine differs, and that is found when the image loads.
