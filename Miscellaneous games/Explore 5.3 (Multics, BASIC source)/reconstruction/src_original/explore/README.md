# Explore — a cave adventure game for Multics

*Version 5.3 (reconstructed)*

Explore is a cave-exploration adventure in the Colossal Cave /
Adventure tradition, written in Multics BASIC with PL/I helper
subroutines. You wander a cave of 58 rooms, collect treasures, defeat
or evade its creatures, learn its magic words, and try to carry
everything home for a perfect score of 500 and the rank of Grand
Master Explorer.

## History

Explore was written in 1980 by Jim Lippard, then a 14-year-old high-school
freshman, as a project for learning about computers and Multics. It ran on
Multics System M in Phoenix and was mainly used by his fellow members of
the Honeywell-sponsored Explorers Post 414. A simpler version for the personal
computers of the day was created by Dale Buhanan. A hall-of-fame tablet in
the game still lists its 1979-1980 winners including a few who played the
PC version. The game went through multiple revisions (4.3, 5.3, and an
anticipated 6.0 version for which the game database was created but no
code was written).

In 2026 the game was reconstructed for the Multics simulator from 1980 line
printer output of the BASIC source (5.3), the game database (6.0), and a
complete play-through (version 4.3). The PL/I and BASIC helper subroutines
were reconstructed (no original code remained), and a number of bugs in the
original were fixed and it was made slightly more robust (see CHANGES).
The cross-checking of source, data, and the play-through identified some
vestigial remains of prior versions in the code which have been retained,
and confirmed the room topology, the magic-word system, the creature/weapon
pairings, and the 500-point scoring.

The Multics reconstruction can be found in the Multics subdirectory with
its own README with instructions for setting it up on a Multics running
on the DPS8 simulator.

To make the game also available on macOS, *BSD, and Linux, I've created
a Multics Basic interpreter (see https://github.com/lippard661/MBasic)
and perl modules to support the game's calls to PL/I handlers with
perl extensions to that interpreter's execution environment.

The basic source files are identical between the Multics/src and
perl/share directories, except that the Multics/src also contains PL/I
helpers and alternative PL/I versions of the basic helpers. The data
files are likewise identical between the Multics/install and
perl/share directories.

The perl version can be installed on OpenBSD using pkg_add and the
p5-Explore-1.1.tgz package in this repo, which is a Legion of Dynamic
Discord-signed package. The same package can be installed on macOS or
Linux using my install.pl script found in this repo:

https://github.com/lippard661/distribute

The public key for the signing key for the package is:

https://www.discord.org/lippard/software/discord.org-2026-pkg.pub

## Artifacts

The artifacts/ directory contains historical materials: the 6.0 game
database (aspirational; no 6.0 version of the game was made), the
original transcribed BASIC 5.3 source, preserved for reference, and a
complete playthrough of an earlier 4.3 version, which has some minor
differences but major spoilers.

## Credits

Explore was written by Jim Lippard (October 1979-June
1980). Reconstructed for the Multics simulator in September 2026 with
assistance from Claude Opus 4.8.

The Multics Basic interpreter and Explore in perl was created on 14
September 2026 by Claude Opus 4.8 with direction and initial design
from Jim Lippard.

