&version 2
&-
&- build.ec -- compile Adventure 1.2 (462 points) on Multics and bind it
&- into bound_adv462_.
&- Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.
&-
&- Run it in a directory holding this directory's files:
&-      exec_com build
&- It compiles the six programs, archives the objects (deleting them) and the
&- bindfile into bound_adv462_.archive, and binds bound_adv462_, whose
&- added name adv462 is the command.  adventure.data must be in the same
&- directory as bound_adv462_ or in >site>adv462_dir; see README.md.
&-
&trace &command on
fortran adventure_
pl1 adv462
pl1 adv462_io_
pl1 addr
pl1 size
pl1 getime
archive rd bound_adv462_.archive adventure_ adv462 adv462_io_ addr size getime
archive r bound_adv462_.archive bound_adv462_.bind
bind bound_adv462_
&print Built bound_adv462_.  Type "adv462" to play.
