# AMAZON — Windows port

> A game of fun, daring, suprises, and multiple players
> — `AMAZON.TXT`, Carl Baltrunas, Tymshare

AMAZON was a multiplayer text adventure for the Tymshare PDP-10s, written
between 1977 and 1981. It was never finished. What survives is a set of
design documents: a room list, an object and creature catalogue, the MACRO-10
data structures, a command library, a multi-process framework, and a parser
test harness.

This is a playable Windows build of that design.

**It is not a decompilation.** There is no compiled AMAZON on any tape.
`AMAZOT.SAV`, which looks like one, is the compiled form of the 20-line
parser test `AMAZOT.SAI` — its only non-runtime string is the `SETBREAK`
argument from that file, and the two carry the same timestamp. See
[docs/RECONSTRUCTION.md](docs/RECONSTRUCTION.md) for the evidence and for the
line-by-line accounting of what is original and what I wrote.

The game itself will tell you: type `SOURCE` at the prompt.

## Build

MinGW-w64 gcc:

```bash
build.bat
```

or

```bash
make
```

No dependencies beyond the Win32 API.

## Play

```bash
bin\amazon.exe
```

Options:

| Switch | Meaning |
|---|---|
| `-name NAME` | your name (otherwise it asks) |
| `-code CODE` | your six-character `CODE`, as in the statistics block |
| `-world FILE` | which valley to play in |
| `-solo` | a private throwaway valley |
| `-new` | discard the existing valley and start over |

## Multiple players

This is the part of AMAZON that was never like anything else. `PRTEST.SAI`
sprouted one SAIL process per TTY, sixteen maximum, with a forty-line ring
carrying messages between them. Both numbers are kept.

Run `bin\amazon.exe` in two console windows and you are in the same valley:
you will see each other in the room listings and in `WHO`, `TELL` reaches
everyone, objects one player picks up are gone for the others, `STEAL` and
`ROB` take things straight out of another player's hands — and if a Policeman
sees you do it, you go to Jail. Names added to the Bar Room's
"Examiners & Collectors" sign are shared, and stay there.

By default everyone on the machine shares `amazon.wld` next to the
executable. Point `-world` at a file on a network share and the valley
spreads across machines.

## Where to start

You begin on the Amazon River Bank with nothing, and the Equipment Shop wants
points. `OBJECT.TYP` is full of things that belong to somebody — the GREEN
Apple belongs to Johnny Appleseed, who is standing beside it; the baby in the
basket belongs to Jane; the white mice belong in Dr. Livingston's laboratory.
Return things, earn points, buy a light, and only then go down into the caves.

Before you try to get past the Phantom's Wolf, find out what the Phantom
wants.

- [docs/COMMANDS.txt](docs/COMMANDS.txt) — the verbs, and which are original
- [docs/MAP.txt](docs/MAP.txt) — the valley
- [docs/RECONSTRUCTION.md](docs/RECONSTRUCTION.md) — provenance
- [data/riddles.txt](data/riddles.txt), [data/barexam.txt](data/barexam.txt) —
  the two reconstructed 36-item tables, in plain text

## Layout

```
port/
  src/       amazon.h  main.c  game.c  world.c  parse.c  riddles.c  share.c
  data/      riddles.txt  barexam.txt        (plain-text copies)
  docs/      RECONSTRUCTION.md  COMMANDS.txt  MAP.txt
  bin/       amazon.exe  amazon.wld          (the world file appears on first run)
../src_original/   the tape files this was built from, untouched
../work/           the tape decoder and the data-file generator
```
