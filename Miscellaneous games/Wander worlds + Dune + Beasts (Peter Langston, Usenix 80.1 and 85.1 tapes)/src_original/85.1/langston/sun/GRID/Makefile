INCL = -I..

all: grid daemon install

install: grid
	../install grid

grid: grid.o gridglb.o
	$(CC) $(CFLAGS) grid.o gridglb.o ../glib.a ../libtcap.a -o $@
	chmod 4755 $@
	@echo "$@ is ready"

gridglb.o: gridglb.c grid.h
	$(CC) $(CFLAGS) $(INCL) -c gridglb.c

daemon: daemon.o dsubs.o dviz.o dtal.o dglb.o gridglb.o
	$(CC) $(CFLAGS) $(INCL) $@.o dsubs.o dviz.o dtal.o dglb.o gridglb.o \
	    ../glib.a ../libtcap.a -o $@
	@echo "$@ is ready"

dviz.o: dviz.c daemon.h grid.h
	$(CC) $(CFLAGS) $(INCL) -c dviz.c

dglb.o: dglb.c daemon.h grid.h
	$(CC) $(CFLAGS) $(INCL) -c dglb.c
