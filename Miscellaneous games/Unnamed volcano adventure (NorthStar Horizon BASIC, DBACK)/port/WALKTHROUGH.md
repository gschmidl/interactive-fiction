# The volcano adventure - walkthrough

The game has no win. Its author stopped writing it: one way on ends at a sign reading "DEAD END, DUNGEON UNDER
CONSTRUCTION, PLEASE COME BACK NEXT WEEK". The furthest you can get is the orcs' prison above the trap-door room.
This walkthrough gets you there and explains every way to die on the way.

## What the game understands

At each `MOVE?`, type one of these and press Enter. Lower case is fine.

| Move | Meaning |
|---|---|
| `U` `D` | up, down |
| `B` | back |
| `R` | right |
| `F` | forward |
| `T` | take |
| `LIFT` | lift (the trap door) |
| `INVENTORY` | list what you carry |

Nothing else is understood. Mostly a move the game doesn't expect is ignored and you are asked again. **On the
slopes and at the rim, though, any wrong move is fatal**, and that is where most of the sudden deaths come from.

## Map

```
 the slopes --U--> the rim --D--> the lava tube --D--> under the shaft
                                                            |
                                                            D   (you find the lantern)
                                                            v
                              the dirt passage <---R--- the junction ---B---> back under the shaft
                              (gold brick: T)  ---U--->     |
                                     |                      D or F
                                     D                      |
                                     +------> THE MAZE <----+
                                                  |
                  after a few moves it lets you out, at random, in one of four places:
          +---------------------+-----------------+------------------------+
          v                     v                 v                        v
      the pit              the dirt passage   a narrow passage         the crevice
      (the end)            (D: the maze)      (D: the maze,            -> the trap-door room
                                               U: the junction)             |
                                                                            U
                                                                            v
                                                          the orcs: their prison, or their feast
```

## The sure way: a fixed seed

The maze decides where it lets you out from the timing of the computer's disk, and that depends on how long you
took to type. `--seed` fixes that time, so the same keys give the same game. Start the game with

```bash
run.bat --seed 5
```

and type exactly these twelve moves, with no other keys, no INVENTORY and no corrections:

| # | You are | Type | What happens |
|---|---|---|---|
| 1 | on the slopes | `U` | you climb to the rim |
| 2 | at the rim | `D` | you jump into an old lava tube; an earthquake shuts it behind you |
| 3 | in the lava tube | `D` | you follow it down, under a small shaft |
| 4 | under the shaft | `D` | you find a lantern and come to a junction |
| 5 | at the junction | `R` | a dirt passage, where you nearly trip over a gold brick |
| 6 | in the dirt passage | `T` | you take the gold brick |
| 7 | in the dirt passage | `D` | into the maze |
| 8-10 | in the maze | `D` `D` `D` | three twisty tunnels, then a crevice leads to the trap-door room |
| 11 | "PRESS ENTER TO CONTINUE" | Enter | a skeleton, and a spiral staircase to a hole in the ceiling |
| 12 | in the trap-door room | `U` | the trap door opens, and the orcs drag you up the staircase to their prison: the end |

Seeds 25, 45 and so on are the same game. For these twelve moves, 8.25, 10.25, 11.25, 12.25, 13.75, 14.5,
16.25, 16.75, 17.5 and 19.75 reach the prison too; seed 6 reaches the trap-door room, but there the orcs eat you.

## Playing without a seed

1. **On the slopes: `U`.** Anything else, and the headhunters cut off your head.
2. **At the rim: `D`.** Anything else, and the headhunters.
3. **In the lava tube: `D`.** Any other move just describes the tube again.
4. **Under the shaft: `D`.** `U` says "TOO SMALL", `B` says "NO EXIT.. CAVE IN", and anything else asks again.
   Going on, you find the lantern (you keep it; `INVENTORY` shows it) and reach the junction.
5. **At the junction:** `R` takes you down the side passage to the gold brick; there `T` takes it, and `U` brings
   you back to the junction. The brick counts for nothing, since there is no score. `D` or `F`, at the junction or
   in the dirt passage, goes into the maze; `B` goes back under the shaft.
6. **The maze.** Whatever you type, you take a step through its "twisty little tunnels", and after a few steps it
   lets you out somewhere at random:

   | Where it lets you out | How often | What to do |
   |---|---|---|
   | the pit | about 29% | nothing: the game is over ("DUNGEON UNDER CONSTRUCTION") |
   | the dirt passage | about 28% | `D`, back into the maze |
   | a narrow, smooth-walled passage | about 29% | `D`, back into the maze (`U` is the junction) |
   | the crevice | about 14% | the way on |

   So keep going back into the maze until it lets you out at the crevice. The pit is twice as likely as the
   crevice, so without a seed about one game in three gets through.
7. **In the trap-door room:** press Enter at "PRESS ENTER TO CONTINUE", then type **`U`**. The trap door bursts
   open. Most of the time (85%) the orcs chain you up in their prison, the end of the game; otherwise they eat
   you.
   - Anything else - `LIFT` ("TOO HEAVY"), `T` ("WHY DO YOU WANT A SKELETON?"), any other move - does nothing,
     but each time there is a 30% chance that the trap door opens by itself, and half of those times the orcs eat
     you.
   - `B` goes back into the maze, safely.

## The four endings

| Ending | How |
|---|---|
| the headhunters | a wrong move on the slopes or at the rim |
| the pit | the maze lets you out there: "DEAD END, DUNGEON UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK" |
| the orcs' feast | the trap door opens and they "PLAY SOCCER WITH IT FOR A WHILE BEFORE DINING ON YOUR PUNY BODY" |
| the orcs' prison | the trap door opens and they chain you up among half a dozen luckless creatures: the furthest ending |

## Shortcuts

The author's own test warps work at any `MOVE?`:

| Warp | Where it takes you |
|---|---|
| `MAZE1` | the maze |
| `TRAPDOOR` | the trap-door room |
| `PIT` | the pit, which ends the game |
| `SHAFT` | under the shaft |

`TRAPDOOR`, Enter, `U`, typed as the very first moves, always ends in the prison. The game's dice restart
with every game, and at that point they always fall the same way.
