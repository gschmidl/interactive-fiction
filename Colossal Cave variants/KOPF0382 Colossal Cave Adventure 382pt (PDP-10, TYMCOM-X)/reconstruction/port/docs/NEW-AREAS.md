# The rope, and the two places it takes you

The 32 points Tymshare added to Woods' 350 sit behind one new object and
one new idea: **a rope you can tie, untie and cut**, and pits deep
enough that you need it.

Everything below was walked in the port; `tests\coat.in` and
`tests\ring.in` replay it and `docs\transcript-coat.txt` /
`docs\transcript-ring.txt` are what came back.

## The rope

A **50 foot coil of rope** lies at the Window on the Pit — from `Y2`, go
`W`.

```
TIE ROPE     anchors it, if there is anything here to anchor it to;
             otherwise "THERE IS NOTHING HERE TO TIE THE ROPE AROUND"
UNTIE ROPE   recovers it, from above
CUT ROPE     needs the dwarf's axe -- "YOU HAVE NOTHING TO CUT WITH" --
             and gives you two 25-foot coils
```

The cut is reversible: drop one coil and `TIE ROPE` while holding the
other and they knot back together into *"a 50 foot coil of rope with a
knot in the middle"*. That is not free. Among the messages compiled into
the image is

```
 DUE TO YOUR SLOPPY WORKMANSHIP, THE KNOT IN THE ROPE HAS
 COME UNTIED WHILE YOU WERE HALFWAY DOWN. YOU HAVE FALLEN
 15 FEET ONTO SOME VERY SHARP ROCKS.
```

alongside `YOUR ROPE WILL NOT REACH` — so length matters, and so does
which rope you are hanging from.

Getting the axe is the ordinary Adventure business of waiting for a
dwarf to throw one and miss, then picking it up off the floor.

## The mail coat — one rope, tied once

```
Secret canyon, junction of three canyons  --SW-->  the narrow crack
                                                  (about 40 feet deep)
        TIE ROPE
                                          --D -->  bottom of the crack
                                          --E -->  small room with wet walls
                                                   NEAR YOU IS A SMALL MAIL
                                                   COAT MADE OF MITHRIL
```

The whole route from the well house, as `tests\coat.in` plays it:

```
IN, GET LAMP, GET KEYS, ON, PLUGH, W, GET ROPE, E,
S, D, W, D, W, W, W, W, W, U, N, N, W, W, SW,
TIE ROPE, D, E, GET COAT
```

— out through Y2, the dirty passage and the dusty rock room to Bedquilt,
across the Swiss Cheese and Twopit rooms to the Slab Room, up the secret
N/S canyon past the Mirror Canyon and the Reservoir, then west into the
three-canyon junction. Nothing there is new; the new part starts at
`SW`.

## The ring — one rope, cut in two

The second area needs the rope in both places at once, which is what the
axe is for.

```
Crossover of a high N/S and a low E/W passage   --N-->  narrow ledge over
   (west of the Hall of the Mountain King's             a 20 foot pit
    west side chamber)                                  "THERE SEEMS TO BE
                                                        SOMETHING AT THE
                                                        BOTTOM OF THE PIT"
        CUT ROPE            two 25-foot coils
        S, DROP ROPE, N     park one below, come back
        TIE ROPE            anchor the other here
        S, GET ROPE, N      collect the parked coil
                                          --D -->  bottom of the 20 foot pit
                                          --S -->  the egg room, a long room
                                                   full of egg shaped rocks
                                                   with a 20 foot pit in it
        TIE ROPE
                                          --D -->  bottom of the pit in the
                                                   egg room
                                          --E -->  a large room with
                                                   "INTICATE MYSTICAL
                                                   CARVINGS" [sic]
                                                   THERE IS A RING OF
                                                   ADAMANT HERE
```

The shuffle in the middle is the puzzle. The game cannot tell two coils
apart in one room: drop one and `TIE ROPE` while holding the other and
they knot back together into a single 50-foot rope instead of anchoring
anything. So the spare coil has to be parked one room away and fetched
after the first is tied. The tape's own walkthrough gives the same
dance —

> CUT ROPE / S / DROP ROPE / N / TIE ROPE / S / GET ROPE / N / D / S /
> TIE ROPE / D / E / GET RING

— which is how this route was found.

## The eight new rooms

Text that appears in this image and in no other Adventure in the
collection:

```
YOU ARE ON A NARROW LEDGE OVER A 20 FOOT PIT. THE WALLS OF THE PIT
ARE TOO STEEP TO CLIMB. THERE SEEMS TO BE SOMETHING AT THE BOTTOM
OF THE PIT.

YOU ARE AT THE BOTTOM OF A 20 FOOT PIT
A NARROW PASSAGE LEADS SOUTH

YOU ARE IN A LONG ROOM FULL OF EGG SHAPED ROCKS. A PASSAGE LEADS
OFF TO THE WEST AND ANOTHER TO THE EAST. IN THE CENTER OF THE ROOM
IS A 20 FOOT PIT WITH WALLS TOO STEEP TO CLIMB. WHEN YOU HOLD
YOUR LIGHT OVER THE PIT, THERE SEEMS TO BE AN OPENING IN THE
EAST WALL

YOU ARE AT THE BOTTOM OF THE PIT IN THE EGG ROOM. A LOW NARROW
CRAWL LEADS EAST

YOU ARE IN A LARGE ROOM WITH INTICATE MYSTICAL CARVINGS ON ALL
THE WALLS. THE ONLY EXIT IS BACK TO THE SOUTH.

THERE IS A NARROW CRACK IN THE FLOOR HERE AND A PASSAGE LEADING
BACK TO THE EAST. THE CRACK APPEARS TO BE ABOUT 40 FEET DEEP.

YOU ARE AT THE BOTTOM OF THE CRACK. A HIGH PASSAGE LEADS EAST.

YOU ARE IN A SMALL ROOM WITH WET WALLS. THE ONLY EXIT IS BACK TO
THE SOUTH.
```

Both dead ends really do exit south, as they say: `S` from the carvings
room returns you to the bottom of the egg-room pit, `S` from the
wet-walls room to the bottom of the crack, and `U` from either pit floor
climbs the rope you left hanging. The misspelling in "INTICATE" is the
author's, and it comes out of all five tape copies identically.

## How the map was recovered

Not by wandering. The image's own tables were read out of core:

* `KEY(1..150)` at `017207` — one index per location
* `TRAVEL` at `015011` — entries of the form
  `dest*1000 + motion`, negative on the last entry for a location
* the message text at `021313`, stored as a chain in which each line's
  first word points at the next and a negative pointer starts a new
  message

Motion codes were calibrated against rooms whose exits are known from
the 350-point game — `43=E 44=W 45=N 46=S 29=U 30=D`, confirmed at the
End of the Road, the well house, the valley and the Hall of Mists —
after which a breadth-first search over the decoded graph gave the
routes above, and the port was used to check that they play.
