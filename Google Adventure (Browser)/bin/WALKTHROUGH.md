# GoogleAdventure - full walkthrough

A complete win path, 71 commands. Verified byte-for-byte identical against
the original `GoogleAdventure.exe` (see `README.md` - Verification).

Goal: find all 5 friends - **red o**, **yellow o**, **blue g**, **green l**,
**red e** - scattered around the Google campus. Each is guarded by a
fetch-quest chain of items and obstacles.

## Item / obstacle map

| Item          | Found at | Used on             | Unlocks |
|---------------|----------|----------------------|---------|
| map           | 5251     | horde (5151)         | exit north to 5051 |
| banana peel   | 5249     | robot (5248)         | exit west to 5247 → **red e** |
| costume       | 5040     | closed door (5045)   | exit north to 4845 → 4849 → **green l** |
| flyswatter    | 5245     | bug (5435)            | exit west to 5434 → **blue g** |
| sword         | 4840-2   | precision (5038-4)    | leaves "recall" obstacle |
| shield        | 5240-4   | recall (5038-4)       | exit west to 5037-4 → stickers, and west again → **yellow o** |
| stickers      | 5037-4   | excited intern (5636) | gives banana |
| (banana)      | -        | happy noogler (5636)  | gives quinoa |
| (quinoa)      | -        | VP (5636)              | gives diet soda |
| (diet soda)   | -        | grumpy engineer (5636) | gives latte |
| (latte)       | -        | twitchy googler (5636) | gives sticky note |
| -             | -        | `e1337` at lockbox (5736), obstacle must still be present | **red o** → win |

Note: typing `use` at the lockbox instead of `e1337` reads the clue text
but *burns* the obstacle without giving up red o - this is an original
game-design quirk, not a bug (see README). The walkthrough below always
uses `e1337` there.

## Command list

```
yes
north
grab
north
use
north
west
south
grab
west
use
west
east
east
north
west
west
south
grab
north
west
west
grab
east
east
use
north
east
east
west
west
south
west
west
north
up
grab
up
south
up
grab
down
north
west
up
use
use
west
grab
west
east
east
down
down
down
west
south
south
west
use
west
east
east
south
use
use
use
use
use
south
e1337
```

## Annotated

```
yes                     start the game
north                   5450 -> 5251 (map on a bench)
grab                    pick up the map
north                   5251 -> 5151 (horde of lost nooglers blocks north)
use                     give them the map -> opens north exit to 5051
north                   5151 -> 5051
west                    5051 -> 5049
south                   5049 -> 5249 (banana peel here)
grab                    pick up the banana peel
west                    5249 -> 5248 (robot dog blocks west)
use                     banana-peel the robot -> opens west exit to 5247
west                    5248 -> 5247  ***red e found***
east                    5247 -> 5248
east                    5248 -> 5249
north                   5249 -> 5049
west                    5049 -> 5047
west                    5047 -> 5045 (alligator blocks north; no costume yet)
south                   5045 -> 5245 (flyswatter here)
grab                    pick up the flyswatter
north                   5245 -> 5045
west                    5045 -> 5043
west                    5043 -> 5040 (costume here)
grab                    pick up the costume
east                    5040 -> 5043
east                    5043 -> 5045
use                     costume defeats the alligator -> opens north exit to 4845
north                   5045 -> 4845
east                    4845 -> 4847
east                    4847 -> 4849  ***green l found***
west                    4849 -> 4847
west                    4847 -> 4845
south                   4845 -> 5045
west                    5045 -> 5043
west                    5043 -> 5040
north                   5040 -> 4840
up                      4840 -> 4840-2 (sword here)
grab                    pick up the sword
up                      4840-2 -> 4840-3
south                   4840-3 -> 5240-3
up                      5240-3 -> 5240-4 (shield here)
grab                    pick up the shield
down                    5240-4 -> 5240-3
north                   5240-3 -> 4840-3
west                    4840-3 -> 5038-3
up                      5038-3 -> 5038-4 (twin ghouls Precision & Recall)
use                     sword strikes Precision -> Recall remains
use                     shield strikes Recall -> opens west exit to 5037-4
west                    5038-4 -> 5037-4 (doodle stickers here)
grab                    pick up the stickers
west                    5037-4 -> 5036-4  ***yellow o found***
east                    5036-4 -> 5037-4
east                    5037-4 -> 5038-4
down                    5038-4 -> 5038-3
down                    5038-3 -> 5038-2
down                    5038-2 -> 5038
west                    5038 -> 5035
south                   5035 -> 5236
south                   5236 -> 5436
west                    5436 -> 5435 (giant bug blocks west)
use                     flyswatter squashes the bug -> opens west exit to 5434
west                    5435 -> 5434  ***blue g found***
east                    5434 -> 5435
east                    5435 -> 5436
south                   5436 -> 5636 (5 googlers, each wants a different item)
use                     stickers -> excited intern gives a banana
use                     banana -> happy noogler gives quinoa
use                     quinoa -> VP gives diet soda
use                     diet soda -> grumpy engineer gives a latte
use                     latte -> twitchy googler gives a sticky note
south                   5636 -> 5736 (lockbox with a keypad)
e1337                   enter the code -> lockbox opens  ***red o found -> WIN***
```

At that point all 5 friends are found and the game prints the win screen
(action count, elapsed time, quack count, "Now get back to work.").
