# Makefile to bring up Export Empire
# This makefile must be run from the Empire "home" directory
#
# set STYLE to SEP (11/45 or 11/70), NOSEP (other pdp11s), or VAX
STYLE = VAX
#
# set LDFLAGS to "-i" for SEP, "-n" for NOSEP, "" for VAX
# include "-lm" for V7 or BSD systems, and "-lS" for V6 systems
# also include "-f" if you have no floating point unit
LDFLAGS = -lm


all: empire BIN/empcre BIN/empchk BIN/empfix BIN/visifix empbins DATA install

# for VAX
empbins: BIN/emp1
# for pdp11 with separated I&D
#empbins: BIN/emp1 BIN/emp2 BIN/emp3
# for pdp11 without separated I&D
#empbins: BIN/emp1 BIN/emp2 BIN/emp3 BIN/emp4 BIN/emp5 BIN/emp6 BIN/emp7

install: empire
	../install empire

empire: empire.o empglb.o empvers.o empsub.a
	$(CC) empire.o empglb.o empvers.o \
	    empsub.a ../glib.a $(LDFLAGS) -o empire
	chmod 4755 empire
	@echo "Move empire to some convenient spot.  /bin/sh?"

BIN/empcre: UTIL/empcre.o UTIL/creglb.o empglb.o empvers.o empsub.a BIN
	$(CC) UTIL/empcre.o UTIL/creglb.o empglb.o empvers.o \
	    empsub.a ../glib.a $(LDFLAGS) -o BIN/empcre

UTIL/creglb.o: UTIL/creglb.c UTIL/credef.h empdef.h
	(cd UTIL; $(CC) $(CFLAGS) -c creglb.c)

BIN/empchk: UTIL/empchk.o empglb.o empvers.o empsub.a BIN
	$(CC) UTIL/empchk.o empglb.o empvers.o empsub.a \
	    ../glib.a $(LDFLAGS) -o BIN/empchk

BIN/empfix: UTIL/empfix.o empglb.o empvers.o empsub.a BIN
	$(CC) UTIL/empfix.o empglb.o empvers.o empsub.a \
	    ../glib.a $(LDFLAGS) -o BIN/empfix

BIN/visifix: UTIL/visifix.o empglb.o empvers.o empsub.a BIN
	$(CC) UTIL/visifix.o empglb.o empvers.o empsub.a \
	    ../glib.a -lcurses -ltermlib $(LDFLAGS) -o BIN/visifix
    
BIN/emp1: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=1 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp1

# stanzas labeled "BIN/emp2" through "BIN/emp7" can be deleted for VAX

BIN/emp2: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=2 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp2

# stanzas labeled "BIN/emp3" through "BIN/emp7" can be deleted for SEP

BIN/emp3: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=3 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp3

BIN/emp4: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=4 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp4

BIN/emp5: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=5 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp5

BIN/emp6: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=6 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp6

BIN/emp7: empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a BIN
	$(CC) $(CFLAGS) -DEMPMOD=7 -D$(STYLE) \
	    empmod.c empdis.o empglb.o empvers.o empcom.a empsub.a \
		../glib.a $(LDFLAGS) -o BIN/emp7

empglb.o: empglb.c
	$(CC) $(CFLAGS) -c empglb.c

DATA:
	mkdir DATA
	chmod 711 DATA

BIN:
	mkdir BIN
	chmod 711 BIN
