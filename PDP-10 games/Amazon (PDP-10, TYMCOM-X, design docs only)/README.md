# AMAZON (Carl Baltrunas, Tymshare, 1977-81)

*AMAZON*, "a game of fun, daring, surprises, and multiple players", Carl Baltrunas's multiplayer text
adventure for the Tymshare PDP-10s, written between 1977 and 1981 and never finished. Only the design
survives - room list, object and creature catalogue, data structures, command library, a
multi-process framework - and this is a playable build of that design, not a decompilation (type
`SOURCE` in the game for what is original). You start on the Amazon River Bank with nothing: return
things to their owners, earn points, buy a light, then go down into the caves. Several players can
share a valley: run the game in several windows.

## Command line

`amazon.exe [options]`

| Option | Effect |
| --- | --- |
| `-name NAME` | your name (otherwise the game asks) |
| `-code CODE` | your six-character code, as in the statistics block |
| `-world FILE` | the valley to play in (default: `amazon.wld` beside the program, shared by everyone on the machine; a file on a network share spreads it across machines) |
| `-solo` | a private, throwaway valley |
| `-new` | discard the existing valley and start over |

## Recommended start

`amazon.exe`
