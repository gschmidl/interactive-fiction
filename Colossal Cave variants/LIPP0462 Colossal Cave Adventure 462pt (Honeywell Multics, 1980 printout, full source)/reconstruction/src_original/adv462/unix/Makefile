# Makefile for adv462 on Unix (gfortran).  Works with GNU make and BSD make.
# Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.
#
#   make                  build ./adv462 and adventure.newgame
#   make install          install under $(PREFIX) (default /usr/local)
#
# adventure.newgame is the game's "system image": the database already read
# in, saved by the wizard's magic mode.  It is made here by running the new
# binary once and answering the wizard's questions (magic word "dwarf").
# Without it the game reads adventure.data at every start and asks
# "are you a wizard?" each time.
#
# adv462.f is generated from ../Multics/adventure_.fortran by mkunix.py and
# is checked in, so Python is needed only after changing the Multics source
# ("make regen").

PREFIX?=	/usr/local
BINDIR?=	$(PREFIX)/bin
SHAREDIR?=	$(PREFIX)/share/adv462
# The writable game directory: magic mode saves adventure.newgame here if the
# directory exists, and files here are used in preference to SHAREDIR's.
GAMEDIR?=	/usr/games/adv462
MANDIR?=	$(PREFIX)/man/man6
DESTDIR?=

FC=		gfortran
CC?=		cc
PYTHON?=	python3
FFLAGS?=	-O
CFLAGS?=	-O
# -fdefault-integer-8 gives the 36-bit Multics integers room; the rest let
# gfortran accept 1970s Fortran.
ADVFFLAGS=	-std=legacy -fdec-char-conversions -fdefault-integer-8 \
		-ffixed-line-length-none -fno-range-check -w
ADVDEFS=	-DADV462_DIR=\"$(SHAREDIR)\" -DADV462_GAMEDIR=\"$(GAMEDIR)\"

DATA=		../Multics/adventure.data

all: adv462 adventure.newgame

adv462: adv462.o adv462_util.o
	$(FC) $(LDFLAGS) -o adv462 adv462.o adv462_util.o

adv462.o: adv462.f
	$(FC) $(FFLAGS) $(ADVFFLAGS) -c adv462.f

adv462_util.o: adv462_util.c
	$(CC) $(CFLAGS) $(ADVDEFS) -c adv462_util.c

# wizard? yes / magic word / "do you know...?" no / magic word again /
# see hours? no / change hours? no / holiday? no / short game, magic word,
# latency: keep / new message of the day? no
adventure.newgame: adv462 $(DATA)
	rm -f adventure.newgame
	printf 'yes\ndwarf\nno\ndwarf\nno\nno\nno\n\n\n\nno\n' | \
	    ADV462_DATA=$(DATA) ADV462_DIR=. ./adv462 >/dev/null
	test -s adventure.newgame

regen:
	$(PYTHON) mkunix.py

install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(SHAREDIR) $(DESTDIR)$(MANDIR)
	install -m 755 adv462 $(DESTDIR)$(BINDIR)/adv462
	install -m 644 $(DATA) $(DESTDIR)$(SHAREDIR)/adventure.data
	install -m 644 adventure.newgame $(DESTDIR)$(SHAREDIR)/adventure.newgame
	install -m 644 adv462.6 $(DESTDIR)$(MANDIR)/adv462.6
	install -d -m 775 $(DESTDIR)$(GAMEDIR)

clean:
	rm -f adv462 *.o adventure.newgame

.PHONY: all regen install clean
