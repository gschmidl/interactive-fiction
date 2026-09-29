CFLAGS = -O
OPT =
# use "OPT = -DNOFTELL" if ftell() doesn't work
DEFWRLD = a3

all: a3 advent castle library wander install

install: a3 castle library
	../install a3 castle library

a3: wander.c wanddef.h ../glib.a
	cp wander.c $@.c
	../replace -q $(DEFWRLD) $@ $@.c
	$(CC) $(CFLAGS) $@.c ../glib.a -o $@
	chmod 4755 $@
	rm $@.c
	@echo "$@ is ready"

advent: wander.c wanddef.h ../glib.a
	cp wander.c $@.c
	../replace -q $(DEFWRLD) $@ $@.c
	$(CC) $(CFLAGS) $@.c ../glib.a -o $@
	chmod 4755 $@
	rm $@.c
	@echo "$@ is ready"

castle: wander.c wanddef.h ../glib.a
	cp wander.c $@.c
	../replace -q $(DEFWRLD) $@ $@.c
	$(CC) $(CFLAGS) $@.c ../glib.a -o $@
	chmod 4755 $@
	rm $@.c
	@echo "$@ is ready"

library: wander.c wanddef.h ../glib.a
	cp wander.c $@.c
	../replace -q $(DEFWRLD) $@ $@.c
	$(CC) $(CFLAGS) $@.c ../glib.a -o $@
	chmod 4755 $@
	rm $@.c
	@echo "$@ is ready"

misc.doc: misc.nr
	nroff misc.nr >misc.doc

wander.doc: wander.nr
	nroff wander.nr >wander.doc

wander: wand1.o wand2.o wandglb.o wandsys.o
	$(CC) $(CFLAGS) wand1.o wand2.o wandglb.o wandsys.o \
	    ../glib.a -o wander
	what wander

wandglb.o: wandglb.c wanddef.h
	$(CC) $(CFLAGS) -c wandglb.c

wandsys.o: wandsys.c wanddef.h
	$(CC) $(CFLAGS) $(OPT) -c wandsys.c

wrld.doc: wrld.nr
	nroff wrld.nr >wrld.doc
